#pragma once

#include <cstdint>
#include <ctime>

// Minimal host-side stand-in for src/MyPersistentData.h, scoped to what
// src/power/PmicFaultMonitor.cpp and the real src/power/PowerManager.cpp it
// links against actually call (WO-2026-08-25-001 Amendment C, Decision C3
// host test). Every getter/setter is a trivial test-controlled pass-through;
// none reimplement production decision logic.
struct TestCurrentStatus {
  int8_t alertCodeValue = 0;
  time_t lastAlertTimeValue = 0;
  float internalTempCValue = 0.0f;
  uint8_t batteryStateValue = 0;
  int raiseAlertCallCount = 0;
  int lastRaisedAlertCode = -1;

  int8_t get_alertCode() const { return alertCodeValue; }
  void set_alertCode(int8_t v) { alertCodeValue = v; }
  void set_lastAlertTime(time_t v) { lastAlertTimeValue = v; }
  float get_internalTempC() const { return internalTempCValue; }
  void set_batteryState(uint8_t v) { batteryStateValue = v; }
  float get_stateOfCharge() const { return 50.0f; }
  uint8_t get_batteryState() const { return batteryStateValue; }

  void raiseAlert(int8_t code) {
    alertCodeValue = code;
    raiseAlertCallCount++;
    lastRaisedAlertCode = code;
  }
};

struct TestSystemStatus {
  bool verboseMode = false;
  bool solarPowerMode = false;

  bool get_verboseMode() const { return verboseMode; }
  uint8_t get_currentBatteryTier() const { return 0; }
  bool get_solarPowerMode() const { return solarPowerMode; }
};

inline TestCurrentStatus testCurrent;
inline TestSystemStatus testSysStatus;

#define current testCurrent
#define sysStatus testSysStatus
