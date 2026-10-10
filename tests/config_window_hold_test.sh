#!/bin/zsh
set -euo pipefail

# WO-2026-10-09-002: the once-a-day config window.
#
# Part 1 extracts the real Cloud::areLedgersSynced() from
# src/cloud/LedgerClient.cpp, checks the copy byte-for-byte against the source
# (COPY_MISMATCH on any difference) and compiles it into
# config_window_hold_test.cpp. Each behaviour case runs in its own process,
# because the function keeps its state in function-local statics.
# Part 2 checks the structure of the CONNECTED+open abort in State_Sleep.cpp
# (it resets cloudSyncStartMs, declared before the abort).
# Part 3 plants mutations on COPIES of the two files; each must be caught by a
# run-time or structure check, not by a compile error.
#
# Usage: config_window_hold_test.sh [LedgerClient.cpp [State_Sleep.cpp]]
# (default: this repo's src/)

script="${0:A}"
repo_root="${script:h:h}"
ledger_src="${1:-$repo_root/src/cloud/LedgerClient.cpp}"
sleep_src="${2:-$repo_root/src/state/State_Sleep.cpp}"
work="${TMPDIR:-/tmp}/config_window_hold_$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

fail() {
  echo "FAILED: $1" >&2
  exit 1
}

extract() {
  python3 - "$ledger_src" "$work" <<'PY'
import re, sys

path, work = sys.argv[1], sys.argv[2]
app = open(path).read()
m = re.search(r"^bool Cloud::areLedgersSynced\(\) const \{.*?^\}\n", app, re.S | re.M)
if not m:
    sys.exit("EXTRACT_FAILED: Cloud::areLedgersSynced() not found")
block = m.group(0)
if block != app[m.start():m.end()] or block not in app:
    print("COPY_MISMATCH: extracted block differs from the source", file=sys.stderr)
    sys.exit(1)
open(work + "/are_ledgers_synced.h", "w").write(block)
open(work + "/copy_back.h", "w").write(app[m.start():m.end()])
PY
  cmp -s "$work/are_ledgers_synced.h" "$work/copy_back.h" || { echo "COPY_MISMATCH" >&2; return 1; }
}

build() {
  clang++ -std=c++17 -Wall -Wextra -pedantic -Wno-unused-variable -Wno-unused-function \
    -DAREA_LEDGERS_SYNCED_H="\"$work/are_ledgers_synced.h\"" \
    "$repo_root/tests/config_window_hold_test.cpp" -o "$work/hold_test"
}

cases=(boot_trusted boot_untrusted boot_no_change anchor same_day first_after_open
       first_after_open_untrusted cut_short backward_clock_unequal zero_connection_epoch warm_outside_hold partial_outside_hold
       empty_device_outside_hold cold_boot)

run_cases() {
  extract
  build
  local c rc=0
  for c in $cases; do
    "$work/hold_test" "$c" || rc=1
  done
  return $rc
}

structural_abort() {
  python3 - "$sleep_src" <<'PY'
import re, sys
text = open(sys.argv[1]).read()
decl = text.find("static unsigned long cloudSyncStartMs = 0;")
m = re.search(r'ensureSensorEnabled\("SLEEP abort: CONNECTED\+OPEN"\);\n(.*?)transitionTo\(IDLE_STATE, "sleep-abort-open-hours"\);', text, re.S)
if not m:
    sys.exit("the CONNECTED+open abort was not found")
if "cloudSyncStartMs = 0;" not in m.group(1):
    sys.exit("the CONNECTED+open abort does not reset cloudSyncStartMs")
if decl < 0 or decl > m.start():
    sys.exit("cloudSyncStartMs is not declared before the CONNECTED+open abort")
PY
}

echo "=== Part 1: the real areLedgersSynced(), one process per case ==="
run_cases || fail "a behaviour case failed"
echo ""
echo "=== Part 2: the CONNECTED+open abort resets cloudSyncStartMs ==="
structural_abort || fail "the abort does not reset cloudSyncStartMs"
echo "OK: the abort resets cloudSyncStartMs"

# When called on a mutated copy, stop here: the caller wants the exit status.
[[ -n "${1:-}" ]] && { echo "config_window_hold_test passed"; exit 0; }

echo ""
echo "=== Part 3: mutations (planted on copies) ==="
plant() {
  # $1 = name, $2 = file (ledger|sleep), then pairs of: old literal, new literal
  local name="$1" which="$2"; shift 2
  local dir="$work/mut_${name// /_}"
  mkdir -p "$dir"
  cp "$repo_root/src/cloud/LedgerClient.cpp" "$dir/LedgerClient.cpp"
  cp "$repo_root/src/state/State_Sleep.cpp" "$dir/State_Sleep.cpp"
  local target="$dir/LedgerClient.cpp"
  [[ "$which" == sleep ]] && target="$dir/State_Sleep.cpp"
  python3 - "$target" "$@" <<'PY'
import sys
path, pairs = sys.argv[1], sys.argv[2:]
text = open(path).read()
for old, new in zip(pairs[::2], pairs[1::2]):
    if text.count(old) != 1:
        sys.exit("MUTATION_FAILED: literal not found exactly once: %r" % old)
    text = text.replace(old, new, 1)
open(path, "w").write(text)
PY
  if "$script" "$dir/LedgerClient.cpp" "$dir/State_Sleep.cpp" >"$dir/out" 2>"$dir/err"; then
    rm -rf "$dir"
    fail "mutation '$name' was NOT detected"
  fi
  if grep -q "error:\|MUTATION_FAILED\|EXTRACT_FAILED" "$dir/err"; then
    cat "$dir/err" >&2
    fail "mutation '$name' broke the build or the mutation instead of being caught by a check"
  fi
  local caught
  caught="$(grep -h -m1 '^FAIL:\|^the \|^cloudSyncStartMs' "$dir/out" "$dir/err" | head -1 | cut -d: -f2-)"
  rm -rf "$dir"
  echo "OK (mutation): '$name' detected ($caught)"
}

plant "boot hold gated by clock trust" ledger \
  "configHold = (holdDoneEpoch == 0) ||
                         (Clock::isTrusted() && holdDoneEpoch < DailyBoundary::todayAt(SystemConfig::get_openTime()));" \
  "configHold = (Clock::isTrusted() && holdDoneEpoch < DailyBoundary::todayAt(SystemConfig::get_openTime()));"
plant "remove the anchor" ledger \
  "        if (configHold && hasPendingOutputLedgerSync()) {
            firstConnectedTime = nowMs;
        }" \
  "        (void)0;"
plant "end the hold on timeout only" ledger \
  "(inputLanded || nowMs - firstConnectedTime" \
  "(nowMs - firstConnectedTime"
plant "set holdDoneEpoch when cut short" ledger \
  "            holdBaseDevice = deviceSync;" \
  "            holdBaseDevice = deviceSync;
            holdDoneEpoch = currentConnectionEpoch;"
plant "early return not gated by the hold" ledger \
  "if (defaultSynced && deviceSynced && !configHold) {" \
  "if (defaultSynced && deviceSynced) {"
plant "F1: one maximum for both input ledgers" ledger \
  "            holdBaseDefault = defaultSync;
            holdBaseDevice = deviceSync;" \
  "            holdBaseDefault = std::max(defaultSync, deviceSync);
            holdBaseDevice = holdBaseDefault;" \
  "(defaultSync != holdBaseDefault) || (deviceSync != holdBaseDevice)" \
  "(std::max(defaultSync, deviceSync) != holdBaseDefault)"
plant "F1: != replaced by > (backward clock step)" ledger \
  "(defaultSync != holdBaseDefault) || (deviceSync != holdBaseDevice)" \
  "(defaultSync > holdBaseDefault) || (deviceSync > holdBaseDevice)"
plant "F2: done stored as a zero epoch" ledger \
  "holdDoneEpoch = currentConnectionEpoch ? currentConnectionEpoch : 1;" \
  "holdDoneEpoch = currentConnectionEpoch;"
plant "remove the CONNECTED+open abort reset" sleep \
  "    ensureSensorEnabled(\"SLEEP abort: CONNECTED+OPEN\");
    cloudSyncStartMs = 0;" \
  "    ensureSensorEnabled(\"SLEEP abort: CONNECTED+OPEN\");"

echo ""
echo "config_window_hold_test passed (real areLedgersSynced() cases, abort structure, mutations)"
