#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
binary="$TMPDIR/thermal_threshold_migration_test"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$repo_root/src" \
  "$repo_root/tests/thermal_threshold_migration_test.cpp" \
  -o "$binary"

"$binary"

# Fidelity checks (WO-2026-08-25-001, Round 4a): the production migrated-read
# path must actually call the set-wise validator under test here, not a
# hand-written mirror of it that could silently drift, and the unsafe
# per-field plausibility substitution it replaces must be gone.
policy="$repo_root/src/power/ChargeInhibitPolicy.h"
persistent_h="$repo_root/src/MyPersistentData.h"
persistent_cpp="$repo_root/src/MyPersistentData.cpp"

if ! grep -q "resolveStoredThermalThresholds" "$policy"; then
  echo "FAIL: ChargeInhibitPolicy.h does not define resolveStoredThermalThresholds()" >&2
  exit 1
fi

if ! grep -q "resolveThermalThresholds" "$persistent_h"; then
  echo "FAIL: MyPersistentData.h does not declare sysStatusData::resolveThermalThresholds()" >&2
  exit 1
fi

if ! grep -q "ChargeInhibitPolicy::resolveStoredThermalThresholds(candidate)" "$persistent_cpp"; then
  echo "FAIL: MyPersistentData.cpp does not resolve stored thermal thresholds via ChargeInhibitPolicy::resolveStoredThermalThresholds()" >&2
  exit 1
fi

if grep -q "plausibleOrDefault" "$persistent_cpp"; then
  echo "FAIL: MyPersistentData.cpp still uses the per-field plausibleOrDefault() this fix replaces" >&2
  exit 1
fi

# All four get_ accessors must resolve through the single set-wise function -
# they must not independently re-derive per-field defaults.
for accessor in get_thermalChargeArmHighC get_thermalChargeArmLowC get_thermalChargeReleaseHighC get_thermalChargeReleaseLowC; do
  if ! grep -A2 "float sysStatusData::${accessor}() const" "$persistent_cpp" | grep -q "resolveThermalThresholds()"; then
    echo "FAIL: sysStatusData::${accessor}() does not resolve via resolveThermalThresholds()" >&2
    exit 1
  fi
done

echo "Fidelity checks passed: MyPersistentData resolves stored thermal thresholds via the shared set-wise validator"
