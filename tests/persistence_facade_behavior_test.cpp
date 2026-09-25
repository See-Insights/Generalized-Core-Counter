// WO-2026-09-23-001 (Step 5) - backend/facade behavior test.
//
// Links against the real src/MyPersistentData.cpp, so every facade call below
// runs the production forwarder and the production class method behind it.

#include "Particle.h"
#include "StorageHelperRK.h"

#include "persist/CurrentReadings.h"
#include "persist/PowerConfig.h"
#include "persist/RecoveryState.h"
#include "persist/SystemConfig.h"

#include "MyPersistentData.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <vector>

// Recorders declared by the StorageHelperRK host stub.
namespace StorageHelperRK {
std::vector<FlushRecord> flushCalls;
std::vector<size_t> validateSizes;
int initializeCalls = 0;
}  // namespace StorageHelperRK

using StorageHelperRK::flushCalls;
using StorageHelperRK::validateSizes;

// src/MyPersistentData.cpp forward-declares these; supply host definitions.
bool publishDiagnosticSafe(const char *, const char *, PublishFlags) { return true; }
bool isClockTrusted() { return true; }
namespace Clock {
bool isTimeValid() { return true; }
}

static int failures = 0;
#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << "FAIL: " << (msg) << " (" #cond ")\n";                      \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

// --- 1. Shared state: a facade must be a view onto the store, not a copy. ---
static void testSharedState() {
  // Two different facades over the SAME sysStatus store.
  SystemConfig::set_openTime(7);
  PowerConfig::set_solarPowerMode(true);
  RecoveryState::set_resetCount(9);

  CHECK(SystemConfig::get_openTime() == 7, "SystemConfig read-back");
  CHECK(PowerConfig::get_solarPowerMode() == true, "PowerConfig read-back");
  CHECK(RecoveryState::get_resetCount() == 9, "RecoveryState read-back");

  // Writing through a facade must be visible on the underlying object, and
  // vice versa - proving one store, not three shadows.
  CHECK(sysStatus.get_openTime() == 7, "facade write visible on the store");
  sysStatus.set_openTime(11);
  CHECK(SystemConfig::get_openTime() == 11, "store write visible through facade");

  // Decision A: RecoveryState's alert fields are physically CurrentData, so a
  // write through RecoveryState must land on `current`, not on `sysStatus`.
  RecoveryState::set_alertCode(12);
  CHECK(current.get_alertCode() == 12, "RecoveryState alert write lands on CurrentData");
  current.set_alertCode(13);
  CHECK(RecoveryState::get_alertCode() == 13, "CurrentData alert visible via RecoveryState");

  // CurrentReadings and RecoveryState share the current store without
  // colliding: an alert write must not disturb a reading and vice versa.
  CurrentReadings::set_hourlyCount(42);
  RecoveryState::set_alertCode(7);
  CHECK(CurrentReadings::get_hourlyCount() == 42, "alert write does not clobber a reading");
  CHECK(RecoveryState::get_alertCode() == 7, "reading write does not clobber the alert");
}

// --- 2. Alert severity arbitration survives the decision-A relocation. ---
static void testAlertSeverityArbitration() {
  RecoveryState::set_alertCode(0);
  RecoveryState::set_lastAlertTime(0);

  RecoveryState::raiseAlert(40);
  const int8_t afterFirst = RecoveryState::get_alertCode();
  CHECK(afterFirst == 40, "first raiseAlert records the alert");
  CHECK(RecoveryState::get_lastAlertTime() == Time.now(), "raiseAlert stamps lastAlertTime");

  // A second, less severe alert must not mask the more severe one already set.
  RecoveryState::raiseAlert(1);
  CHECK(RecoveryState::get_alertCode() == afterFirst,
        "a less severe alert must not overwrite a more severe one");

  // Upward escalation. Without this case an implementation that only ever
  // writes when no alert is set (`if (existing == 0)`) satisfies both checks
  // above while silently dropping every escalation - Stage 7 review's first
  // finding on this test.
  RecoveryState::set_alertCode(0);
  RecoveryState::set_lastAlertTime(0);
  RecoveryState::raiseAlert(44);  // severity 1 (minor)
  CHECK(RecoveryState::get_alertCode() == 44, "a minor alert is recorded when nothing is set");

  RecoveryState::set_lastAlertTime(0);  // so a re-stamp is observable
  RecoveryState::raiseAlert(30);        // severity 2 (major) - must escalate
  CHECK(RecoveryState::get_alertCode() == 30,
        "a MORE severe alert must overwrite a less severe one (upward escalation)");
  CHECK(RecoveryState::get_lastAlertTime() == Time.now(),
        "an upward escalation must re-stamp lastAlertTime");

  RecoveryState::set_lastAlertTime(0);
  RecoveryState::raiseAlert(19);  // severity 4 (watchdog) - must supersede tier 2
  CHECK(RecoveryState::get_alertCode() == 19,
        "a watchdog alert must supersede an already-set major alert");
  CHECK(RecoveryState::get_lastAlertTime() == Time.now(),
        "the watchdog escalation must re-stamp lastAlertTime");

  // Going through the facade must be identical to going through the object.
  current.set_alertCode(0);
  current.raiseAlert(40);
  const int8_t viaObject = current.get_alertCode();
  RecoveryState::set_alertCode(0);
  RecoveryState::raiseAlert(40);
  CHECK(RecoveryState::get_alertCode() == viaObject,
        "facade raiseAlert matches the object's own arbitration");
}

// --- 3. validate() sizing (Finding 2) and the published capacities. ---
static void testValidateAndCapacities() {
  // The capacities ConfigApply.cpp sizes its buffers from must equal the real
  // persisted field widths. MyPersistentData.cpp static_asserts this, so a
  // successful build already proves it; assert at runtime too so the
  // expectation is visible in this test's output, not only in the compiler's.
  CHECK(SystemConfig::kTimeZoneCapacity == sizeof(sysStatusData::SysData::timeZoneStr),
        "kTimeZoneCapacity equals the real field width");
  CHECK(SystemConfig::kWebhookNameCapacity == sizeof(sysStatusData::SysData::webhookName),
        "kWebhookNameCapacity equals the real field width");

  // A string exactly at capacity must be rejected (no room for the
  // terminator) and must leave the stored value untouched - the property
  // ConfigApply.cpp's length pre-check depends on.
  SystemConfig::set_timeZoneStr("EST5EDT");
  char tooLong[SystemConfig::kTimeZoneCapacity + 1];
  std::memset(tooLong, 'x', sizeof(tooLong) - 1);
  tooLong[sizeof(tooLong) - 1] = '\0';
  CHECK(SystemConfig::set_timeZoneStr(tooLong) == false, "over-capacity timezone is rejected");
  CHECK(std::string(SystemConfig::get_timeZoneStrCStr()) == "EST5EDT",
        "a rejected write leaves the stored timezone intact");

  // Finding 2: validateStoredData() must pass the same size the pre-split
  // call site passed - sizeof(sysStatusData), the class object, NOT
  // sizeof(SysData) and not sizeof() of any facade type.
  validateSizes.clear();
  SystemConfig::validateStoredData();
  CHECK(validateSizes.size() == 1, "validateStoredData calls validate exactly once");
  CHECK(validateSizes.at(0) == sizeof(sysStatusData),
        "sysStatus validate() receives sizeof(sysStatusData), as before the split");
  CHECK(validateSizes.at(0) != sizeof(sysStatusData::SysData),
        "validate() must NOT be narrowed to sizeof(SysData) by the split");

  validateSizes.clear();
  SystemConfig::SensorSettings::validateStoredData();
  CHECK(validateSizes.size() == 1, "sensor validateStoredData calls validate exactly once");
  CHECK(validateSizes.at(0) == sizeof(sensorConfigData),
        "sensorConfig validate() receives sizeof(sensorConfigData), as before the split");
}

// --- 4. Deferred vs forced persistence timing, and WHICH store is serviced. ---
static const char *const kSysPath = "/usr/sysStatus.dat";
static const char *const kCurrentPath = "/usr/current.dat";
static const char *const kSensorPath = "/usr/sensor.dat";

static bool onlyFlush(const char *expectedPath, bool expectedForced) {
  if (flushCalls.size() != 1) return false;
  if (flushCalls.at(0).forced != expectedForced) return false;
  return std::strcmp(flushCalls.at(0).path, expectedPath) == 0;
}

static void testDeferredVsForcedPersistence() {
  // Each facade's loop() is the deferred path AND must service its OWN store.
  // Asserting the store identity is what catches a facade that services a
  // sibling instead - invisible when only the forced/deferred bool is recorded.
  flushCalls.clear();
  SystemConfig::loop();
  CHECK(onlyFlush(kSysPath, false),
        "SystemConfig::loop() deferred-flushes sysStatus.dat and nothing else");

  flushCalls.clear();
  CurrentReadings::loop();
  CHECK(onlyFlush(kCurrentPath, false),
        "CurrentReadings::loop() deferred-flushes current.dat and nothing else");

  flushCalls.clear();
  SystemConfig::SensorSettings::loop();
  CHECK(onlyFlush(kSensorPath, false),
        "SensorSettings::loop() deferred-flushes sensor.dat and nothing else");

  // flushNow() is the forced path, and it too must touch only sysStatus.
  // Losing this distinction would silently turn every pre-sleep forced write
  // into a deferred one.
  flushCalls.clear();
  SystemConfig::flushNow();
  CHECK(onlyFlush(kSysPath, true),
        "SystemConfig::flushNow() FORCE-flushes sysStatus.dat and nothing else");

  // The save delays setup() installs are part of the persistence contract.
  flushCalls.clear();
  SystemConfig::setup();
  CHECK(sysStatus.saveDelayMs == 100, "sysStatus keeps its 100ms save delay");
  CurrentReadings::setup();
  CHECK(current.saveDelayMs == 250, "current keeps its 250ms save delay");
  SystemConfig::SensorSettings::setup();
  CHECK(sensorConfig.saveDelayMs == 250, "sensorConfig keeps its 250ms save delay");
}

// --- 5. The cross-store reset in resetEverything() (Finding 4). ---
static void testResetEverythingCrossStore() {
  RecoveryState::set_resetCount(5);
  CurrentReadings::set_hourlyCount(17);
  CurrentReadings::set_dailyCount(99);
  CurrentReadings::set_occupied(true);
  CurrentReadings::set_totalOccupiedSeconds(1234);

  CurrentReadings::resetEverything();

  CHECK(CurrentReadings::get_hourlyCount() == 0, "resetEverything clears hourlyCount");
  CHECK(CurrentReadings::get_dailyCount() == 0, "resetEverything clears dailyCount");
  CHECK(CurrentReadings::get_occupied() == false, "resetEverything clears occupied");
  CHECK(CurrentReadings::get_totalOccupiedSeconds() == 0, "resetEverything clears occupied seconds");
  // The cross-store part: it reaches into the sysStatus reset count too.
  CHECK(RecoveryState::get_resetCount() == 0,
        "resetEverything still clears the sysStatus reset count (cross-store)");
}

// --- 6. The two sensorType fields stay distinct (Finding 3). ---
static void testSensorTypeFieldsStayDistinct() {
  SystemConfig::set_sensorType(1);
  SystemConfig::SensorSettings::set_sensorType(3);
  CHECK(SystemConfig::get_sensorType() == 1, "sysStatus sensorType is unaffected by the sensor.dat write");
  CHECK(SystemConfig::SensorSettings::get_sensorType() == 3, "sensor.dat sensorType holds its own value");

  SystemConfig::set_sensorType(2);
  CHECK(SystemConfig::SensorSettings::get_sensorType() == 3,
        "sensor.dat sensorType is unaffected by the sysStatus write");
}

// --- 7. Thermal threshold set-wise fallback still resolves through the facade. ---
static void testThermalThresholdFallback() {
  // A zero-padded (migrated) record is invalid AS A SET; all four getters must
  // fall back to the compiled defaults atomically, never report {0,0,0,0}.
  PowerConfig::set_thermalChargeArmHighC(0.0f);
  PowerConfig::set_thermalChargeArmLowC(0.0f);
  PowerConfig::set_thermalChargeReleaseHighC(0.0f);
  PowerConfig::set_thermalChargeReleaseLowC(0.0f);
  CHECK(PowerConfig::get_thermalChargeArmHighC() == 37.0f, "armHighC falls back to the compiled default");
  CHECK(PowerConfig::get_thermalChargeReleaseHighC() == 35.0f, "releaseHighC falls back to the compiled default");
  CHECK(PowerConfig::get_thermalChargeReleaseLowC() == 3.0f, "releaseLowC falls back to the compiled default");

  // A valid stored set must be honoured as stored.
  PowerConfig::set_thermalChargeArmHighC(40.0f);
  PowerConfig::set_thermalChargeArmLowC(1.0f);
  PowerConfig::set_thermalChargeReleaseHighC(36.0f);
  PowerConfig::set_thermalChargeReleaseLowC(4.0f);
  CHECK(PowerConfig::get_thermalChargeArmHighC() == 40.0f, "a valid stored set is honoured");
  CHECK(PowerConfig::get_thermalChargeReleaseLowC() == 4.0f, "a valid stored set is honoured (low side)");
}

int main() {
  testSharedState();
  testAlertSeverityArbitration();
  testValidateAndCapacities();
  testDeferredVsForcedPersistence();
  testResetEverythingCrossStore();
  testSensorTypeFieldsStayDistinct();
  testThermalThresholdFallback();

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    return 1;
  }
  std::cout << "OK: facades are views onto the shared stores, not copies\n";
  std::cout << "OK: alert severity arbitration holds in both directions (suppress lower, escalate higher)\n";
  std::cout << "OK: validate() receives the pre-split size; capacities match the real field widths\n";
  std::cout << "OK: each facade's loop() services its OWN store; flushNow() stays forced\n";
  std::cout << "OK: resetEverything() still performs its cross-store reset-count clear\n";
  std::cout << "OK: the two sensorType fields remain distinct\n";
  std::cout << "OK: thermal threshold set-wise fallback resolves through PowerConfig\n";
  std::cout << "persistence_facade_behavior_test: all checks passed\n";
  return 0;
}
