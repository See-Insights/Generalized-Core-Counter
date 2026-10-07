#!/bin/zsh
set -euo pipefail

# WO-2026-10-07-002 (Step 6 WO 1a): the configured connection mode is never
# changed by a battery downgrade; the mode in use is derived by
# PowerManager::effectiveConnectionMode().
#
# Links the REAL src/MyPersistentData.cpp, src/power/BatteryAuthorityCommand.cpp
# and src/power/PowerManager.cpp, and compiles the connection-mode block of
# Cloud::applyModesConfig() extracted verbatim from src/cloud/ConfigApply.cpp
# (that file needs Device OS LedgerData/Variant, so the block is the part that
# can run on the host).
#
# MUTATIONS: after the clean run, the same test is rebuilt against deliberately
# broken COPIES (production sources are never modified). Each must make the
# test FAIL at run time; a mutant that does not compile is a broken mutation,
# not a caught one.

repo_root="${0:A:h:h}"
binary="$TMPDIR/connection_mode_downgrade_test"
work="$TMPDIR/connection_mode_downgrade_mutations_$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

config_apply_src="$repo_root/src/cloud/ConfigApply.cpp"

extract_block() {
  # $1 = file, $2 = literal that starts the block's first line
  python3 - "$1" "$2" <<'PY'
import sys

path, needle = sys.argv[1], sys.argv[2]
lines = open(path).readlines()
start = next((i for i, l in enumerate(lines) if needle in l), None)
if start is None:
    sys.exit(f"EXTRACT_FAILED: {needle!r} not found in {path}")
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
        sys.stdout.write("".join(lines[start:i + 1]))
        break
else:
    sys.exit(f"EXTRACT_FAILED: no closing brace for {needle!r} in {path}")
PY
}

extract_block "$config_apply_src" 'if (getMergedIntValue(defaultModes, deviceModes, "connectionMode", connectionMode)) {' \
  > "$work/connection_mode_block.inc"
if ! grep -q 'set_connectionMode' "$work/connection_mode_block.inc"; then
  echo "FAILED: could not extract the connection-mode block from ConfigApply.cpp" >&2
  exit 1
fi

build() {
  # $1 = output, $2 = BatteryAuthorityCommand.cpp, $3 = PowerManager.cpp, $4 = block include dir
  clang++ -std=c++17 -Wall -Wextra -pedantic \
    -I"$4" \
    -I"$repo_root/tests/stubs/connection_mode_overrides" \
    -I"$repo_root/tests/stubs/persistence_backend" \
    -I"$repo_root/src" \
    "$repo_root/tests/connection_mode_downgrade_test.cpp" \
    "$repo_root/src/MyPersistentData.cpp" \
    "$2" "$3" \
    "$repo_root/src/power/BatteryAuthority.cpp" \
    "$repo_root/src/reporting/BatteryTierGuard.cpp" \
    "$repo_root/src/power/PowerTier.cpp" \
    "$repo_root/src/power/BatteryHealth.cpp" \
    -o "$1"
}

real_bac="$repo_root/src/power/BatteryAuthorityCommand.cpp"
real_pm="$repo_root/src/power/PowerManager.cpp"

# --- Clean run -------------------------------------------------------------
build "$binary" "$real_bac" "$real_pm" "$work"
"$binary"

# --- Mutation runs ---------------------------------------------------------
mutate_and_expect_failure() {
  local name="$1" bac="$2" pm="$3" incdir="$4"
  local mutant_bin="$work/mutant"
  if ! build "$mutant_bin" "$bac" "$pm" "$incdir" 2>"$work/build.log"; then
    echo "FAIL (mutation harness): mutant '$name' did not compile - the mutation is malformed" >&2
    cat "$work/build.log" >&2
    exit 1
  fi
  if "$mutant_bin" >/dev/null 2>&1; then
    echo "FAIL (mutation): '$name' was NOT detected - the behavior test has a blind spot" >&2
    exit 1
  fi
  echo "OK (mutation): '$name' correctly detected"
}

plant() {
  # $1 = source, $2 = dest, $3 = old literal, $4 = new literal
  python3 - "$1" "$2" "$3" "$4" <<'PY'
import sys

src, dst, old, new = sys.argv[1:5]
text = open(src).read()
if text.count(old) < 1:
    sys.exit(f"MUTATION_FAILED: {old!r} not found in {src}")
open(dst, "w").write(text.replace(old, new, 1))
PY
}

# Mutation 1: the downgrade writes the configured mode again (the old :83).
mkdir -p "$work/m1"
plant "$real_bac" "$work/m1/BatteryAuthorityCommand.cpp" \
  "      commitLowBatteryMode(true);" \
  "      SystemConfig::set_connectionMode(SystemConfig::INTERMITTENT);
      commitLowBatteryMode(true);"
mutate_and_expect_failure "downgrade overwrites the configured mode" \
  "$work/m1/BatteryAuthorityCommand.cpp" "$real_pm" "$work"

# Mutation 2: the derivation drops its OCCUPANCY term.
mkdir -p "$work/m2"
plant "$real_pm" "$work/m2/PowerManager.cpp" \
  "      SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY &&
" ""
mutate_and_expect_failure "mode in use ignores the OCCUPANCY condition" \
  "$real_bac" "$work/m2/PowerManager.cpp" "$work"

# Mutation 3: config apply compares against the flag again.
mkdir -p "$work/m3"
plant "$work/connection_mode_block.inc" "$work/m3/connection_mode_block.inc" \
  "static_cast<SystemConfig::ConnectionMode>(connectionMode)) {" \
  "static_cast<SystemConfig::ConnectionMode>(connectionMode) || PowerConfig::get_lowBatteryMode()) {"
mutate_and_expect_failure "config apply compares against lowBatteryMode" \
  "$real_bac" "$real_pm" "$work/m3"

# Mutation 4: a connect-path reader reads the configured getter (structural).
mkdir -p "$work/m4"
cp -R "$repo_root/src" "$work/m4/src"
plant "$repo_root/src/state/State_Sleep.cpp" "$work/m4/src/state/State_Sleep.cpp" \
  "const bool reportNow = (PowerManager::instance().effectiveConnectionMode() == SystemConfig::INTERMITTENT_KEEP_ALIVE);" \
  "const bool reportNow = (SystemConfig::get_configuredConnectionMode() == SystemConfig::INTERMITTENT_KEEP_ALIVE);"
python3 "$repo_root/tests/connection_mode_downgrade_structural_test.py" "$repo_root/src" >/dev/null
if python3 "$repo_root/tests/connection_mode_downgrade_structural_test.py" "$work/m4/src" >/dev/null 2>&1; then
  echo "FAIL (mutation): 'connect-path reader uses the configured getter' was NOT detected" >&2
  exit 1
fi
echo "OK (mutation): 'connect-path reader uses the configured getter' correctly detected"

echo "connection_mode_downgrade_test: clean run passed and all mutations detected"
