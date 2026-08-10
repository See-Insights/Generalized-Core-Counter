#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
generated="$TMPDIR/power_consumer_regression_test.cpp"
binary="$TMPDIR/power_consumer_regression_test"

extract_braced_block() {
  local file="$1"
  local marker="$2"
  awk -v marker="$marker" '
    !active && index($0, marker) { active = 1 }
    active {
      print
      opens = gsub(/\{/, "{")
      closes = gsub(/\}/, "}")
      depth += opens - closes
      if (seen_open && depth == 0) exit
      if (opens > 0) seen_open = 1
    }
  ' "$file"
}

{
  cat <<'CPP'
#include "power/PowerManager.h"
#include "MyPersistentData.h"
#include "cloud/BatteryBackoffPolicy.h"

#include <cassert>
#include <cmath>
#include <ctime>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "power/ConnectivityPolicy.h"

TestCurrentStatus testCurrent;
TestSystemStatus testSysStatus;
CPP

  cat <<'CPP'
class Cloud {
public:
  static BatteryTier calculateBatteryTier(float currentSoC);
};

struct RecordedValue {
  std::string name;
  float value;
  int precision;
};

struct TestWriter {
  std::vector<RecordedValue> values;
  std::string pendingName;

  TestWriter &name(const char *name) {
    pendingName = name;
    return *this;
  }

  TestWriter &value(float value, int precision) {
    values.push_back({pendingName, value, precision});
    return *this;
  }
};
CPP

  extract_braced_block "$repo_root/src/state/State_Connect.cpp" "struct ConnectBudgetContext"
  extract_braced_block "$repo_root/src/state/State_Connect.cpp" "ConnectBudgetContext evaluateConnectBudget()"
  extract_braced_block "$repo_root/src/cloud/BatteryBackoff.cpp" "BatteryTier Cloud::calculateBatteryTier(float currentSoC)"
  extract_braced_block "$repo_root/src/Generalized-Core-Counter.cpp" "BatteryTier currentBatteryTierForFailsafe()"
  extract_braced_block "$repo_root/src/diagnostics/ConnectivityFailsafeTest.cpp" "BatteryTier currentBatteryTierForFailsafeLocal()"

  cat <<'CPP'
struct NormalizedBatteryPayload {
  uint8_t battState;
  float stateOfCharge;
};

NormalizedBatteryPayload normalizePublishDataBattery() {
CPP
  awk '
    !active && /  uint8_t battState = PowerManager::instance\(\)\.batteryState\(\);/ { active = 1 }
    active {
      if (/  uint8_t sensorMode =/ || /  const int8_t reportedAlertCode =/) next
      print
      opens = gsub(/\{/, "{")
      closes = gsub(/\}/, "}")
      depth += opens - closes
      completed += closes
      if (completed == 2 && depth == 0) exit
    }
  ' "$repo_root/src/Generalized-Core-Counter.cpp"
  cat <<'CPP'
  return {battState, stateOfCharge};
}

struct PreSleepBatteryEncoding {
  uint16_t socTenths;
  uint8_t isCharging;
};

PreSleepBatteryEncoding encodePreSleepBattery() {
CPP
  awk '
    /    const float soc = PowerManager::instance\(\)\.soc\(\);/ { active = 1 }
    active { print }
    active && /    const uint8_t isCharging =/ { exit }
  ' "$repo_root/src/state/State_Sleep.cpp"
  cat <<'CPP'
  return {socTenths, isCharging};
}

std::vector<RecordedValue> writeDeviceStatusSocFields() {
  TestWriter writerBase;
  TestWriter writer;
CPP
  grep -E 'writerBase\.name\("soc"\)|writer\.name\("soc"\)' \
    "$repo_root/src/cloud/DeviceStatusPublisher.cpp"
  cat <<'CPP'
  std::vector<RecordedValue> result = writerBase.values;
  result.insert(result.end(), writer.values.begin(), writer.values.end());
  return result;
}

void resetInputs(float soc, uint8_t batteryState) {
  testCurrent.socValue = soc;
  testCurrent.batteryStateValue = batteryState;
  testCurrent.socReadCount = 0;
  testCurrent.batteryStateReadCount = 0;
  testSysStatus = {};
}

void testConnectBudgetStrictSocBoundaryAndCounterPath() {
  resetInputs(50.0f, 0);
  ConnectBudgetContext atBoundary = evaluateConnectBudget();
  assert(!atBoundary.allowDeepAttempt);
  assert(atBoundary.budgetMs == ConnectivityPolicy::CONNECT_BUDGET_DEFAULT_MS);
  assert(testCurrent.socReadCount == 1);

  resetInputs(50.01f, 0);
  ConnectBudgetContext aboveBoundary = evaluateConnectBudget();
  assert(aboveBoundary.allowDeepAttempt);
  assert(aboveBoundary.budgetMs == ConnectivityPolicy::CONNECT_BUDGET_DEEP_MS);
  assert(testCurrent.socReadCount == 1);

  resetInputs(10.0f, 0);
  testSysStatus.connectionAttemptCounter = ConnectivityPolicy::DEEP_ATTEMPT_COUNTER_THRESHOLD;
  ConnectBudgetContext counterPath = evaluateConnectBudget();
  assert(counterPath.allowDeepAttempt);
  assert(counterPath.budgetMs == ConnectivityPolicy::CONNECT_BUDGET_DEEP_MS);
  assert(testCurrent.socReadCount == 1);
}

void testPublishDataBatteryNormalization() {
  resetInputs(55.0f, 7);
  NormalizedBatteryPayload badState = normalizePublishDataBattery();
  assert(badState.battState == 0);
  assert(badState.stateOfCharge == 55.0f);

  resetInputs(NAN, 3);
  NormalizedBatteryPayload nanSoc = normalizePublishDataBattery();
  assert(nanSoc.stateOfCharge == 0.0f);
  assert(nanSoc.battState == 0);

  resetInputs(-0.1f, 3);
  NormalizedBatteryPayload lowSoc = normalizePublishDataBattery();
  assert(lowSoc.stateOfCharge == 0.0f);
  assert(lowSoc.battState == 0);

  resetInputs(100.1f, 3);
  NormalizedBatteryPayload highSoc = normalizePublishDataBattery();
  assert(highSoc.stateOfCharge == 0.0f);
  assert(highSoc.battState == 0);
}

void assertPreSleepBatteryEncoding(float soc, uint8_t batteryState,
                                   uint16_t expectedSocTenths, uint8_t expectedIsCharging) {
  resetInputs(soc, batteryState);
  const PreSleepBatteryEncoding encoding = encodePreSleepBattery();
  assert(encoding.socTenths == expectedSocTenths);
  assert(encoding.isCharging == expectedIsCharging);
  assert(testCurrent.socReadCount == 1);
  assert(testCurrent.batteryStateReadCount == 1);
}

void testPreSleepBatteryEncoding() {
  assertPreSleepBatteryEncoding(0.0f, 0, 0, 0xFF);
  assertPreSleepBatteryEncoding(100.0f, 2, 1000, 1);
  assertPreSleepBatteryEncoding(47.3f, 1, 473, 0);
  assertPreSleepBatteryEncoding(47.3f, 3, 473, 0);
  assertPreSleepBatteryEncoding(-0.1f, 0, 0xFFFF, 0xFF);
  assertPreSleepBatteryEncoding(100.1f, 2, 0xFFFF, 1);
  assertPreSleepBatteryEncoding(NAN, 1, 0xFFFF, 0);
}

void testFailsafeDuplicatesStayIdentical() {
  for (uint8_t persistedTier = TIER_HEALTHY; persistedTier <= TIER_SURVIVAL; persistedTier++) {
    resetInputs(12.0f, 0);
    testSysStatus.currentBatteryTier = persistedTier;
    assert(currentBatteryTierForFailsafe() == static_cast<BatteryTier>(persistedTier));
    assert(currentBatteryTierForFailsafeLocal() == static_cast<BatteryTier>(persistedTier));
    assert(testCurrent.socReadCount == 0);
  }

  const float fallbackSocValues[] = {29.9f, 30.0f, 50.0f, 70.0f, 75.0f};
  for (float soc : fallbackSocValues) {
    resetInputs(soc, 0);
    testSysStatus.currentBatteryTier = 0xFF;
    const BatteryTier productionTier = currentBatteryTierForFailsafe();
    const BatteryTier diagnosticTier = currentBatteryTierForFailsafeLocal();
    assert(productionTier == diagnosticTier);
    assert(testCurrent.socReadCount == 2);
  }
}

void testDeviceStatusSocWritersUseExactAccessorValueAndPrecision() {
  resetInputs(47.26f, 0);
  const std::vector<RecordedValue> values = writeDeviceStatusSocFields();
  assert(values.size() == 2);
  for (const RecordedValue &recorded : values) {
    assert(recorded.name == "soc");
    assert(recorded.value == 47.26f);
    assert(recorded.precision == 1);
  }
  assert(testCurrent.socReadCount == 2);
}

int main() {
  testConnectBudgetStrictSocBoundaryAndCounterPath();
  testPublishDataBatteryNormalization();
  testPreSleepBatteryEncoding();
  testFailsafeDuplicatesStayIdentical();
  testDeviceStatusSocWritersUseExactAccessorValueAndPrecision();
  std::cout << "Power consumer regression tests passed\n";
  return 0;
}
CPP
} > "$generated"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$repo_root/tests/stubs" -I"$repo_root/src" \
  "$generated" "$repo_root/src/power/PowerManager.cpp" \
  -o "$binary"

"$binary"