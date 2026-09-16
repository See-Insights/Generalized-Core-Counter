#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
binary="$TMPDIR/watchdog_ab1805_classification_test"

# --- Part 1: compile and run the host-side logic-mirror test ---
clang++ -std=c++17 -Wall -Wextra -pedantic \
  "$repo_root/tests/watchdog_ab1805_classification_test.cpp" \
  -o "$binary"
"$binary"

# --- Part 2: fidelity checks against the real source, so the mirror above
# cannot silently drift without this test failing. WO-2026-09-16 Step 2
# moved this classification out of Generalized-Core-Counter.cpp into
# time/HibernateCycle.cpp's classifyWake() (absorbed alongside the
# hibernate wake-validation gate, since both read the same
# AB1805::getWakeReason() getter) - re-pointed here to the new owner. ---
src="$repo_root/src/time/HibernateCycle.cpp"

check() {
  local desc="$1"
  local pattern="$2"
  if ! grep -q -- "$pattern" "$src"; then
    echo "FIDELITY CHECK FAILED: $desc (pattern not found: $pattern)" >&2
    exit 1
  fi
}

check "PIN_RESET gate present" "if (osResetReason == RESET_REASON_PIN_RESET) {"
check "getWakeReason() called" "const AB1805::WakeReason pinResetWakeReason = rtc.getWakeReason();"
check "WATCHDOG confirms v.pinResetConfirmedWatchdog" "pinResetWakeReason == AB1805::WakeReason::WATCHDOG) {"
check "UNKNOWN branch present and treated as inconclusive" "pinResetWakeReason == AB1805::WakeReason::UNKNOWN) {"

# The combined watchdogClassified condition and the "runs before setWDT()"
# ordering are Generalized-Core-Counter.cpp's job now - classifyWake() only
# produces v.pinResetConfirmedWatchdog; the caller combines it with
# watchdogResetDetected and still calls classifyWake() before setWDT(). Both
# checked against the call site, not the classification itself.
gcc_src="$repo_root/src/Generalized-Core-Counter.cpp"
if ! grep -q -- "const bool watchdogClassified = watchdogResetDetected || ab1805ConfirmedWatchdog;" "$gcc_src"; then
  echo "FIDELITY CHECK FAILED: combined watchdogClassified condition, at the call site (pattern not found in $gcc_src)" >&2
  exit 1
fi
if ! grep -q -- "const HibernateCycle::WakeVerdict wakeVerdict = HibernateCycle::classifyWake(reason, ab1805);" "$gcc_src"; then
  echo "FIDELITY CHECK FAILED: classifyWake() call site (pattern not found in $gcc_src)" >&2
  exit 1
fi
awk '
  /const HibernateCycle::WakeVerdict wakeVerdict = HibernateCycle::classifyWake\(reason, ab1805\);/ { classify=NR }
  classify && /ab1805\.setWDT\(AB1805::WATCHDOG_MAX_SECONDS\);/ && !wdt { wdt=NR; exit }
  END {
    if (!classify || !wdt) {
      print "FIDELITY CHECK FAILED: could not locate classifyWake()/setWDT() anchors" > "/dev/stderr";
      exit 1;
    }
    if (!(classify < wdt)) {
      print "FIDELITY CHECK FAILED: classification must run before setWDT()" > "/dev/stderr";
      exit 1;
    }
  }
' "$gcc_src"
echo "Fidelity check passed: classification runs before setWDT() in $gcc_src"

# The redundant second updateWakeReason() call (the Stage 7 bug) must NOT be
# present in the classification block. AB1805::setup() (called earlier, not
# shown in this excerpt) already calls it once internally; a second call here
# would re-read the status register after the first call destructively
# cleared whichever bit it classified, and could silently overwrite a correct
# WATCHDOG classification on a combined-bit status read (WDT+TIMER/WDT+ALARM).
if grep -v '^\s*//' "$src" | grep -q -- "rtc.updateWakeReason()"; then
  echo "FIDELITY CHECK FAILED: redundant rtc.updateWakeReason() call found in $src - AB1805::setup() already calls it once internally; a second call here reintroduces the destructive double-read bug (WO-2026-08-10-001 Status section)" >&2
  exit 1
fi
echo "Fidelity check passed: no redundant rtc.updateWakeReason() call in $src"

echo "Fidelity checks passed: classification mirror matches key control-flow markers in $src"
echo "AB1805 watchdog classification test suite passed"
