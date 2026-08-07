#include "power/PowerManager.h"
#include "MyPersistentData.h"

#include <cassert>
#include <cmath>
#include <iostream>

TestCurrentStatus testCurrent;
TestSystemStatus testSysStatus;

namespace {

void resetLedger(float soc, uint8_t batteryState) {
  testCurrent.socValue = soc;
  testCurrent.batteryStateValue = batteryState;
  testCurrent.socReadCount = 0;
  testCurrent.batteryStateReadCount = 0;
}

void testSocIsLiveSingleReadPassThrough() {
  resetLedger(50.0f, 0);
  assert(PowerManager::instance().soc() == 50.0f);
  assert(testCurrent.socReadCount == 1);

  testCurrent.socValue = 50.1f;
  assert(PowerManager::instance().soc() == 50.1f);
  assert(testCurrent.socReadCount == 2);
}

void testSocPreservesInvalidValues() {
  resetLedger(NAN, 0);
  assert(std::isnan(PowerManager::instance().soc()));
  assert(testCurrent.socReadCount == 1);
}

void testBatteryStateIsRawSingleReadPassThrough() {
  resetLedger(0.0f, 6);
  assert(PowerManager::instance().batteryState() == 6);
  assert(testCurrent.batteryStateReadCount == 1);

  testCurrent.batteryStateValue = 0xFF;
  assert(PowerManager::instance().batteryState() == 0xFF);
  assert(testCurrent.batteryStateReadCount == 2);
}

void testAccessorsDoNotRequireSetup() {
  resetLedger(42.5f, 1);
  assert(PowerManager::instance().soc() == 42.5f);
  assert(PowerManager::instance().batteryState() == 1);
  assert(testCurrent.socReadCount == 1);
  assert(testCurrent.batteryStateReadCount == 1);
}

void testAccessorsDoNotPopulateCachedReport() {
  resetLedger(42.5f, 1);
  (void)PowerManager::instance().soc();
  (void)PowerManager::instance().batteryState();

  const PowerReading &reading = PowerManager::instance().latestReport().reading;
  assert(std::isnan(reading.soc));
  assert(reading.batteryContext == PowerBatteryContext::Unknown);
  assert(!reading.fallbackUsed);
  assert(!reading.sampledPreRadio);
  assert(!reading.quickStartUsed);
  assert(reading.stabilizationAttempts == 0);
}

} // namespace

int main() {
  testSocIsLiveSingleReadPassThrough();
  testSocPreservesInvalidValues();
  testBatteryStateIsRawSingleReadPassThrough();
  testAccessorsDoNotRequireSetup();
  testAccessorsDoNotPopulateCachedReport();
  std::cout << "PowerManager accessor tests passed\n";
  return 0;
}