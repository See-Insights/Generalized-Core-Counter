#!/bin/zsh
set -euo pipefail
# WO-2026-10-15-002 (Step 6 2b-6): the daily and hourly resets, end to end. Compiles the real PIRSensor and
# activeSensor() with dailyCleanup(), the hourly block of handleReportingState(), the central
# resetEverything(bool) and its wrapper, all extracted verbatim (EXTRACT_FAILED if a marker is missing or
# not unique), then runs mutants on private copies of src/ in $TMPDIR (never the worktree); each must fail
# a named check. RESET_WIRING_SRC_DIR points a run at a copy of src/.
tests_dir="${0:A:h}"; self="${0:A}"; repo_root="${tests_dir:h}"
src_dir="${RESET_WIRING_SRC_DIR:-$repo_root/src}"
work="$(mktemp -d "${TMPDIR:-/tmp}/reset_wiring.XXXXXX")"; trap 'rm -rf "$work"' EXIT
python3 - "$src_dir" "$work" <<'PY'
import re, sys
src, out = sys.argv[1:3]

def one(text, rx, what, flags=0):
    m = list(re.finditer(rx, text, flags))
    if len(m) != 1: sys.exit(f"EXTRACT_FAILED: {len(m)} matches for {what}")
    return m[0].group(0)

main = open(f"{src}/Generalized-Core-Counter.cpp").read()
pers = open(f"{src}/MyPersistentData.cpp").read()
report = open(f"{src}/state/State_Report.cpp").read()
mgr = open(f"{src}/sensors/SensorManager.cpp").read()
def marker(text, lit, what, kind="start"):  # a boundary line must start exactly once, or the region is ambiguous
    n = len(re.findall("^" + re.escape(lit), text, re.M))
    if n != 1: sys.exit(f"EXTRACT_FAILED: {kind} marker for {what} found {n} times (expected 1): '{lit}'")

marker(main, "void dailyCleanup() {", "dailyCleanup()")
marker(report, "  // After each hourly report, reset the hourly counter so", "hourly block")
marker(report, "  // Long-term webhook supervision", "hourly block", "end")
marker(pers, "void currentStatusData::resetEverything(bool includeSensorTotals) {", "central resetEverything(bool)")
marker(pers, "void resetEverything(bool includeSensorTotals)", "CurrentReadings wrapper")
marker(mgr, "ISensor* SensorManager::activeSensor() {", "activeSensor()")
pieces = {
    "gen_daily_cleanup.inc": one(main, r"void dailyCleanup\(\) \{.*?\n\}\n", "dailyCleanup()", re.S),
    "gen_hourly_block.inc": one(report, r"  // After each hourly report, reset the hourly counter so.*?\n(?=\n  // Long-term webhook supervision)", "hourly block", re.S),
    "gen_central_reset.inc": one(pers, r"void currentStatusData::resetEverything\(bool includeSensorTotals\) \{.*?\n\}\n", "central resetEverything(bool)", re.S),
    "gen_wrapper.inc": one(pers, r"void resetEverything\(bool includeSensorTotals\)[^\n]*\n", "CurrentReadings wrapper"),
    "gen_active_sensor.inc": one(mgr, r"ISensor\* SensorManager::activeSensor\(\) \{.*?\n\}\n", "activeSensor()", re.S),
}
for name, body in pieces.items():
    open(f"{out}/{name}", "w").write(body)

# initialize() keeps the default full reset; the declarations keep the default argument.
init = one(pers, r"void currentStatusData::initialize\(\) \{.*?\n\}\n", "initialize()", re.S)
if not re.search(r"currentStatusData::resetEverything\(\);", init):
    print("FAIL: initialize() must call resetEverything() with the default full reset"); sys.exit(1)
for f in ("MyPersistentData.h", "persist/CurrentReadings.h"):
    if "void resetEverything(bool includeSensorTotals = true);" not in open(f"{src}/{f}").read():
        print(f"FAIL: {f} must declare resetEverything(bool includeSensorTotals = true)"); sys.exit(1)
print("ok: initialize() keeps the full reset; both declarations default to true")
PY
c++ -std=c++17 -Wall -Wextra -I"$work" -I"$tests_dir/stubs/pir_lifecycle" -I"$src_dir" -I"$repo_root/src" \
  "$tests_dir/reset_wiring_test.cpp" "$src_dir/sensors/PIRSensor.cpp" -o "$work/bin"
"$work/bin"
[[ -n "${RESET_WIRING_SRC_DIR:-}" ]] && exit 0
mutant() {  # $1 label, $2 expected failing text, then (file under src/, old, new) triples
  local label="$1" want="$2" d="$work/mut" f text out; shift 2; rm -rf "$d"; mkdir -p "$d"; cp -R "$repo_root/src/." "$d/"
  while (( $# )); do
    f="$d/$1"; text="$(<"$f")"; [[ "$text" == *"$2"* ]] || { echo "BROKEN_MUTATION: $label" >&2; exit 1; }
    print -r -- "${text/"$2"/$3}" > "$f"; shift 3
  done
  out="$(RESET_WIRING_SRC_DIR="$d" zsh "$self" 2>&1)" && { echo "MUTANT SURVIVED: $label" >&2; exit 1; }
  [[ "$out" == *"FAIL: $want"* ]] || { echo "BROKEN_MUTATION (wrong or no check failed): $label" >&2; print -r -- "$out" >&2; exit 1; }
  echo "mutant caught: $label (FAIL: $want...)"
}
mutant "dailyCleanup() calls resetEverything() with no argument (double reset)" "daily (not connected, PIR)" \
  Generalized-Core-Counter.cpp "CurrentReadings::resetEverything(false);" "CurrentReadings::resetEverything();"
mutant "dailyCleanup() drops the sensor's Daily hook" "daily (not connected, PIR)" \
  Generalized-Core-Counter.cpp "  SensorManager::instance().activeSensor()->resetTotals(ResetPeriod::Daily);
" ""
mutant "hourly hook moved outside the COUNTING guard" "hourly OCCUPANCY clears nothing" \
  state/State_Report.cpp "  if (SystemConfig::get_sensorMode() == SystemConfig::COUNTING) {
    SensorManager::instance().activeSensor()->resetTotals(ResetPeriod::Hourly);
  }" "  SensorManager::instance().activeSensor()->resetTotals(ResetPeriod::Hourly);"
mutant "resetEverything(false) returns before resetCount" "resetEverything(false) stamps" \
  MyPersistentData.cpp "  sysStatus.set_resetCount(0);                                          // Reset the reset count as well
  if (!includeSensorTotals) return;                                     // The active sensor clears its own totals" \
  "  if (!includeSensorTotals) return;
  sysStatus.set_resetCount(0);"
mutant "initialize() passes false" "initialize() must call resetEverything() with the default" \
  MyPersistentData.cpp "    currentStatusData::resetEverything();" "    currentStatusData::resetEverything(false);"
echo "reset_wiring_test (with mutants) passed"
