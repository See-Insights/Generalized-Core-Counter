#!/bin/zsh
set -euo pipefail

# WO-2026-10-08-002 fix A: sensor creation reads the sensor store
# (SensorSettings::get_sensorType), not sysStatus.sensorType, and sensor.type
# is validated to exactly 1. Real code is extracted from src/ (never copied by
# hand): SensorManager::initializeFromConfig(), the LED-power block of
# setup(), the SensorType enum, and the sensor.type block of ConfigApply. A
# failed extraction aborts loudly (EXTRACT_FAILED). Each mutation (on a COPY)
# must fail at run time, not at compile time.
#
# Usage: sensor_type_source_test.sh
# Overrides (used by the mutation runs): SENSOR_MANAGER_SRC, MAIN_SRC, CONFIG_APPLY_SRC

script="${0:A}"
repo_root="${script:h:h}"
sm_src="${SENSOR_MANAGER_SRC:-$repo_root/src/sensors/SensorManager.cpp}"
main_src="${MAIN_SRC:-$repo_root/src/Generalized-Core-Counter.cpp}"
ca_src="${CONFIG_APPLY_SRC:-$repo_root/src/cloud/ConfigApply.cpp}"
factory_hdr="$repo_root/src/sensors/SensorFactory.h"
work="${TMPDIR:-/tmp}/sensor_type_source_$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

fail() {
  echo "FAILED: $1" >&2
  exit 1
}

# extract <file> <needle> <output> <mode>
#   mode "braces": from the needle line to the line that closes its braces
#   mode "block":  from the needle line to the first line that is exactly "  }"
extract() {
  python3 - "$1" "$2" "$3" "$4" <<'PY'
import sys
path, needle, out, mode = sys.argv[1:5]
lines = open(path).readlines()
hits = [i for i, l in enumerate(lines) if needle in l]
if len(hits) != 1:
    sys.exit("EXTRACT_FAILED: %r found %d times in %s" % (needle, len(hits), path))
start = hits[0]
if needle.startswith("bool Cloud::validateRange"):
    start -= 1  # include the template<typename T> line
if mode == "block":
    for i in range(start, len(lines)):
        if lines[i].rstrip("\n") == "  }":
            open(out, "w").write("".join(lines[start:i + 1]))
            break
    else:
        sys.exit("EXTRACT_FAILED: no closing line")
    sys.exit(0)
depth = 0
opened = False
for i in range(start, len(lines)):
    for ch in lines[i]:
        if ch == '{':
            depth += 1
            opened = True
        elif ch == '}':
            depth -= 1
    if opened and depth == 0:
        open(out, "w").write("".join(lines[start:i + 1]))
        break
else:
    sys.exit("EXTRACT_FAILED: no closing brace")
PY
}

run_all() {
  extract "$factory_hdr" 'enum class SensorType' "$work/sensor_type.inc" braces
  extract "$sm_src" 'void SensorManager::initializeFromConfig() {' "$work/initialize_from_config.inc" braces
  extract "$main_src" 'SensorType configuredType = ' "$work/led_block.inc" block
  extract "$ca_src" 'if (getMergedIntValue(defaultSensor, deviceSensor, "type", sensorType)) {' "$work/type_block.inc" braces
  extract "$ca_src" 'bool Cloud::validateRange(T value' "$work/validate_range.inc" braces
  grep -q "get_sensorType" "$work/initialize_from_config.inc" || fail "could not extract initializeFromConfig"
  grep -q "SensorDefinitions::getDefinition" "$work/led_block.inc" || fail "could not extract the LED block"
  grep -q "set_sensorType" "$work/type_block.inc" || fail "could not extract the sensor.type block"
  clang++ -std=c++17 -Wall -Wextra -pedantic -I"$work" -I"$repo_root/src" \
    "$repo_root/tests/sensor_type_source_test.cpp" -o "$work/source_bin"
  clang++ -std=c++17 -Wall -Wextra -pedantic -I"$work" \
    "$repo_root/tests/sensor_type_validation_test.cpp" -o "$work/validation_bin"
  "$work/source_bin"
  "$work/validation_bin"
}

run_all

[[ -n "${SENSOR_TYPE_TEST_MUTATION_RUN:-}" ]] && exit 0

mutate() {
  # $1 = source file, $2 = destination copy, $3 = old text, $4 = new text
  python3 - "$1" "$2" "$3" "$4" <<'PY'
import sys
src, dst, old, new = sys.argv[1:5]
text = open(src).read()
if text.count(old) != 1:
    sys.exit("MUTATION_FAILED: %r not found exactly once" % old)
open(dst, "w").write(text.replace(old, new))
PY
}

expect_caught() {
  # $1 = description; the remaining environment overrides are already exported
  if SENSOR_TYPE_TEST_MUTATION_RUN=1 "$script" >/dev/null 2>"$work/err"; then
    fail "mutation '$1' was NOT detected"
  fi
  grep -q "error:" "$work/err" && { cat "$work/err" >&2; fail "mutation '$1' broke the build instead of being caught"; }
  echo "OK (mutation): '$1' detected"
}

echo "--- mutation: revert A1 (SensorManager reads the sysStatus getter) ---"
mutate "$sm_src" "$work/SensorManager.mut.cpp" \
  "static_cast<SensorType>(SystemConfig::SensorSettings::get_sensorType());" \
  "static_cast<SensorType>(SystemConfig::get_sensorType());"
SENSOR_MANAGER_SRC="$work/SensorManager.mut.cpp" expect_caught "revert A1"

echo "--- mutation: revert A2 (setup reads the sysStatus getter) ---"
mutate "$main_src" "$work/Main.mut.cpp" \
  "configuredType = static_cast<SensorType>(SystemConfig::SensorSettings::get_sensorType());" \
  "configuredType = static_cast<SensorType>(SystemConfig::get_sensorType());"
MAIN_SRC="$work/Main.mut.cpp" expect_caught "revert A2"

echo "--- mutation: restore the 0..255 range ---"
mutate "$ca_src" "$work/ConfigApply.mut.cpp" \
  'validateRange(sensorType, 1, 1, "sensor.type")' \
  'validateRange(sensorType, 0, 255, "sensor.type")'
CONFIG_APPLY_SRC="$work/ConfigApply.mut.cpp" expect_caught "restore 0..255"

echo "sensor_type_source_test passed"
