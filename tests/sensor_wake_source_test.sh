#!/bin/zsh
set -euo pipefail
# WO-2026-10-14-004 (Step 6 2b-4): compiles the real PIRSensor, SensorManager::activeSensor() and the
# sensorWake/wakePin/wakeReturnReason/pirWake statements of State_Sleep.cpp (extracted verbatim;
# EXTRACT_FAILED if a marker is missing or not unique), then runs mutants on private copies of src/ in
# $TMPDIR (never the worktree); each must fail a check. WAKE_SRC_DIR points a run at a copy of src/.
tests_dir="${0:A:h}"; self="${0:A}"; repo_root="${tests_dir:h}"
src_dir="${WAKE_SRC_DIR:-$repo_root/src}"
work="$(mktemp -d "${TMPDIR:-/tmp}/sensor_wake_source.XXXXXX")"; trap 'rm -rf "$work"' EXIT
python3 - "$src_dir" "$work" <<'PY'
import re, sys
src, out = sys.argv[1:3]
sleep = open(f"{src}/state/State_Sleep.cpp").read()
# WO-2026-10-21-001: the statements live in sleepAndResume(); the handler's unpack line
# (bool pirWake = wake.pirWake;) is outside the extraction scope and must not match.
fi = sleep.find("bool sleepAndResume(")
if fi < 0 or sleep.count("bool sleepAndResume(") != 1: sys.exit("EXTRACT_FAILED: sleepAndResume() not found once")
fe = sleep.find("\n}\n", fi)
if fe < 0: sys.exit("EXTRACT_FAILED: end of sleepAndResume() not found")
sleep = sleep[fi:fe + 3]
for name, rx in {"sensor_wake": r"const SensorWakeSource sensorWake =[^;]*;\s*const bool sensorCanWake =[^;]*;",
                 "wake_pin": r"\w+ wakePin = result\.wakeupPin\(\);",
                 "wake_reason": r"const char \*wakeReturnReason =[^;]*;", "pirwake": r"bool pirWake =[^;]*;"}.items():
    m = re.findall(rx, sleep)
    if len(m) != 1: sys.exit(f"EXTRACT_FAILED: {len(m)} matches in State_Sleep.cpp for {rx}")
    open(f"{out}/gen_{name}.inc", "w").write(m[0] + "\n")
mgr = open(f"{src}/sensors/SensorManager.cpp").read()
i = mgr.index("ISensor* SensorManager::activeSensor() {")
open(f"{out}/gen_active_sensor.inc", "w").write(mgr[i:mgr.index("\n}\n", i) + 3])
PY
c++ -std=c++17 -Wall -Wextra -I"$work" -I"$tests_dir/stubs/pir_lifecycle" -I"$src_dir" -I"$repo_root/src" \
  "$tests_dir/sensor_wake_source_test.cpp" "$src_dir/sensors/PIRSensor.cpp" -o "$work/bin"
"$work/bin"
[[ -n "${WAKE_SRC_DIR:-}" ]] && exit 0
mutant() {  # $1 label, then (file under src/, old, new) triples
  local label="$1" d="$work/mut" f text out; shift; rm -rf "$d"; mkdir -p "$d"; cp -R "$repo_root/src/." "$d/"
  while (( $# )); do
    f="$d/$1"; text="$(<"$f")"; [[ "$text" == *"$2"* ]] || { echo "BROKEN_MUTATION: $label" >&2; exit 1; }
    print -r -- "${text/"$2"/$3}" > "$f"; shift 3
  done
  out="$(WAKE_SRC_DIR="$d" zsh "$self" 2>&1)" && { echo "MUTANT SURVIVED: $label" >&2; exit 1; }
  [[ "$out" == *"FAIL: "* ]] || { echo "BROKEN_MUTATION (did not run): $label" >&2; exit 1; }
  echo "mutant caught: $label ($(print -r -- "$out" | grep -m1 'FAIL: '))"
}
n8=(sensors/ISensor.h "    pin_t pin;" "    uint8_t pin;" sensors/PIRSensor.h "{intPin, RISING}" "{(uint8_t)intPin, RISING}")
mutant "stored wake-source pin narrowed to uint8_t" $n8
mutant "stored pin and the wakePin local narrowed to uint8_t" $n8 state/State_Sleep.cpp "pin_t wakePin = result" "uint8_t wakePin = result"
mutant "PIR edge FALLING" sensors/PIRSensor.h "{intPin, RISING}" "{intPin, FALLING}"
mutant "pirWake compares against intPin again" state/State_Sleep.cpp "(wakePin == sensorWake.pin);" "(wakePin == intPin);"
mutant "pirWake drops sensorCanWake" state/State_Sleep.cpp "bool pirWake = sensorCanWake && " "bool pirWake = "
echo "sensor_wake_source_test (with mutants) passed"
