#!/bin/zsh
set -euo pipefail

# WO-2026-10-15-001: PIRSensor::resetTotals() setter sequences, central-order match, ISR-pending,
# and the hook-only (no caller in src/) check. Set PIR_RESET_SRC_DIR to a copy of src/ to test
# private sources; the mutation runs below do exactly that, in $TMPDIR.

tests_dir="${0:A:h}"
repo_root="${tests_dir:h}"
src_dir="${PIR_RESET_SRC_DIR:-$repo_root/src}"
work="$(mktemp -d "${TMPDIR:-/tmp}/pir_reset_totals.XXXXXX")"
trap 'rm -rf "$work"' EXIT

run() {  # $1 = src dir; prints failures, returns non-zero if any check failed
  local s="$1"
  # Central order: the sensor-total setters of the real resetEverything(), minus lastCountTime.
  python3 - "$s/MyPersistentData.cpp" "$work/central.txt" <<'PY'
import re, sys
t = open(sys.argv[1]).read()
b = t[t.index("void currentStatusData::resetEverything()"):]
b = b[:b.index("\n}\n")]
calls = re.findall(r"current\.(set_\w+\([^)]*\));", b)
open(sys.argv[2], "w").write("\n".join(c for c in calls if "lastCountTime" not in c) + "\n")
PY
  c++ -std=c++17 -Wall -Wextra -pedantic -I"$tests_dir/stubs/pir_lifecycle" -I"$s" -I"$repo_root/src" \
    "$tests_dir/pir_sensor_reset_totals_test.cpp" "$s/sensors/PIRSensor.cpp" -o "$work/bin"
  "$work/bin" "$work/central.txt" || return 1
  # Hook only: every resetTotals( outside its declarations and the PIR definition is a caller.
  if grep -rn "resetTotals(" "$s" | grep -v -E "virtual void resetTotals\(ResetPeriod\)|void resetTotals\(ResetPeriod period\) override|PIRSensor::resetTotals\(ResetPeriod period\)"; then
    echo "FAIL: caller of resetTotals( in src/"
    return 1
  fi
  echo "ok: no caller of resetTotals( in src/"
}

run "$src_dir"
[[ -n "${PIR_RESET_SRC_DIR:-}" ]] && exit 0

mutant() {  # $1 = label, $2 = file under src/, $3 = old, $4 = new
  local dir="$work/mut"
  rm -rf "$dir"; mkdir -p "$dir"; cp -R "$repo_root/src/." "$dir/"
  python3 - "$dir/$2" "$3" "$4" <<'PY'
import sys
t = open(sys.argv[1]).read()
if t.count(sys.argv[2]) < 1: sys.exit("BROKEN_MUTATION: " + sys.argv[2])
open(sys.argv[1], "w").write(t.replace(sys.argv[2], sys.argv[3], 1))
PY
  if PIR_RESET_SRC_DIR="$dir" "$tests_dir/pir_sensor_reset_totals_test.sh" >"$dir.out" 2>&1; then echo "MUTANT_SURVIVED: $1" >&2; exit 1; fi
  echo "mutant caught: $1 ($(grep -m1 FAIL "$dir.out" || echo build/run failure))"
}

mutant "swap two Daily setters" sensors/PIRSensor.cpp \
  "CurrentReadings::set_dailyCount(0);
        CurrentReadings::set_occupied(false);" \
  "CurrentReadings::set_occupied(false);
        CurrentReadings::set_dailyCount(0);"
mutant "drop a Daily setter" sensors/PIRSensor.cpp "CurrentReadings::set_totalOccupiedSeconds(0);" ""
mutant "resetTotals clears _motionDetected" sensors/PIRSensor.cpp \
  "CurrentReadings::set_hourlyCount(0);
    if (period" "CurrentReadings::set_hourlyCount(0);
    _motionDetected = false;
    if (period"
mutant "caller added in State_Report.cpp" state/State_Report.cpp "CurrentReadings::set_hourlyCount(0);" \
  "CurrentReadings::set_hourlyCount(0); sensor->resetTotals(ResetPeriod::Hourly);"
echo "pir_sensor_reset_totals_test (with mutants) passed"
