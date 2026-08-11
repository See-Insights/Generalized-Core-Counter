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
# cannot silently drift from src/Generalized-Core-Counter.cpp without this
# test failing. ---
src="$repo_root/src/Generalized-Core-Counter.cpp"

check() {
  local desc="$1"
  local pattern="$2"
  if ! grep -q -- "$pattern" "$src"; then
    echo "FIDELITY CHECK FAILED: $desc (pattern not found: $pattern)" >&2
    exit 1
  fi
}

check "PIN_RESET gate present" "if (reason == RESET_REASON_PIN_RESET) {"
check "getWakeReason() called" "const AB1805::WakeReason pinResetWakeReason = ab1805.getWakeReason();"
check "WATCHDOG confirms ab1805ConfirmedWatchdog" "pinResetWakeReason == AB1805::WakeReason::WATCHDOG) {"
check "UNKNOWN branch present and treated as inconclusive" "pinResetWakeReason == AB1805::WakeReason::UNKNOWN) {"
check "combined watchdogClassified condition" "const bool watchdogClassified = watchdogResetDetected || ab1805ConfirmedWatchdog;"
check "classification runs before setWDT()" "ab1805.setWDT(AB1805::WATCHDOG_MAX_SECONDS); // Enable watchdog"

# The redundant second updateWakeReason() call (the Stage 7 bug) must NOT be
# present in the classification block. AB1805::setup() (called earlier, not
# shown in this excerpt) already calls it once internally; a second call here
# would re-read the status register after the first call destructively
# cleared whichever bit it classified, and could silently overwrite a correct
# WATCHDOG classification on a combined-bit status read (WDT+TIMER/WDT+ALARM).
if grep -v '^\s*//' "$src" | grep -q -- "ab1805.updateWakeReason()"; then
  echo "FIDELITY CHECK FAILED: redundant ab1805.updateWakeReason() call found in $src - AB1805::setup() already calls it once internally; a second call here reintroduces the destructive double-read bug (WO-2026-08-10-001 Status section)" >&2
  exit 1
fi
echo "Fidelity check passed: no redundant ab1805.updateWakeReason() call in $src"

echo "Fidelity checks passed: classification mirror matches key control-flow markers in $src"
echo "AB1805 watchdog classification test suite passed"
