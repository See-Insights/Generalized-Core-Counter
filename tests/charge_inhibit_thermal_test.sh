#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
binary="$TMPDIR/charge_inhibit_thermal_test"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$repo_root/src" \
  "$repo_root/tests/charge_inhibit_thermal_test.cpp" \
  -o "$binary"

"$binary"

# Fidelity checks: the mechanism file must use the corrected API (not the
# bare PMIC call that Device OS quietly reverts), and the arm/release
# thresholds must be the field-proven 37/0/35/3 values, not the 45C LiPo-spec
# value from the pre-existing (incorrect) isItSafeToCharge() implementation.
mechanism="$repo_root/src/power/ChargeInhibit.cpp"
policy="$repo_root/src/power/ChargeInhibitPolicy.h"

if ! grep -q "setPowerConfiguration" "$mechanism"; then
  echo "FAIL: ChargeInhibit.cpp does not call System.setPowerConfiguration()" >&2
  exit 1
fi

if ! grep -q "SystemPowerFeature::DISABLE_CHARGING" "$mechanism"; then
  echo "FAIL: ChargeInhibit.cpp does not reference SystemPowerFeature::DISABLE_CHARGING" >&2
  exit 1
fi

if grep -q "disableCharging()" "$mechanism"; then
  echo "FAIL: ChargeInhibit.cpp calls a bare PMIC disableCharging() - Device OS reverts this" >&2
  exit 1
fi

if ! grep -q "armHighC = 37.0f" "$policy" || ! grep -q "armLowC = 0.0f" "$policy" \
   || ! grep -q "releaseHighC = 35.0f" "$policy" || ! grep -q "releaseLowC = 3.0f" "$policy"; then
  echo "FAIL: ChargeInhibitPolicy.h thresholds do not match 37/0 arm, 35/3 release" >&2
  exit 1
fi

# AC-B5 (WO-2026-08-25-001 Amendment B): SensorManager::isItSafeToCharge()
# must actually call the validity-gated decision function under test here,
# not a hand-written mirror of it that could silently drift.
sensor_manager="$repo_root/src/sensors/SensorManager.cpp"
if ! grep -q "ChargeInhibitPolicy::evaluateThermalWithValidity" "$sensor_manager"; then
  echo "FAIL: SensorManager.cpp does not call ChargeInhibitPolicy::evaluateThermalWithValidity()" >&2
  exit 1
fi

echo "Fidelity checks passed: real ChargeInhibit.cpp/ChargeInhibitPolicy.h match the corrected mechanism and thresholds"
