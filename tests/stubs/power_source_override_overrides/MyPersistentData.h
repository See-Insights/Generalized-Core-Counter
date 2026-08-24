#pragma once

#include <cstdint>
#include <ctime>

// Cloud.h (included by DeviceStatusPublisher.cpp) references BatteryTier in
// its declarations. In the real firmware, src/MyPersistentData.h supplies
// this via `#include "cloud/BatteryBackoffPolicy.h"`; that header is pure,
// dependency-free enum/inline-math (no Particle/hardware dependencies), so
// this stub includes the REAL production header rather than reimplementing
// the tier enum or its logic.
#include "cloud/BatteryBackoffPolicy.h"

// Extended for the Finding-1 remediation round to also support linking the
// real src/cloud/DeviceStatusPublisher.cpp (which reads several more
// `current`/`sysStatus`/`sensorConfig` getters than the original
// PowerManager-only harness needed). All getters are trivial pass-throughs of
// test-controlled state; none reimplement production decision logic.
struct TestCurrentStatus {
  float socValue = 0.0f;
  uint8_t batteryStateValue = 0;
  bool occupiedValue = false;
  uint32_t totalOccupiedSecondsValue = 0;
  float internalTempCValue = 0.0f;
  mutable unsigned socReadCount = 0;
  mutable unsigned batteryStateReadCount = 0;

  float get_stateOfCharge() const {
    socReadCount++;
    return socValue;
  }

  uint8_t get_batteryState() const {
    batteryStateReadCount++;
    return batteryStateValue;
  }

  bool get_occupied() const { return occupiedValue; }
  uint32_t get_totalOccupiedSeconds() const { return totalOccupiedSecondsValue; }
  float get_internalTempC() const { return internalTempCValue; }
};

struct TestSystemStatus {
  uint16_t connectAttemptBudgetSec = 0;
  uint8_t connectionAttemptCounter = 0;
  uint8_t currentBatteryTier = 0;
  uint8_t resetCount = 0;
  time_t lastReport = 0;
  bool verboseMode = false;
  uint16_t verboseTimeoutMin = 0;
  const char *timeZoneStrCStr = "UTC0";
  uint16_t reportingInterval = 3600;
  uint8_t openTime = 6;
  uint8_t closeTime = 22;
  bool serialConnected = false;
  uint8_t sensorMode = 0;
  uint8_t connectionMode = 0;
  uint8_t reportingMode = 0;
  uint8_t samplingMode = 0;
  uint16_t cloudDisconnectBudgetSec = 15;
  uint16_t modemOffBudgetSec = 30;
  bool enableHibernateSleep = false;
  const char *webhookNameCStr = "test-webhook";
  bool webhookEnabled = false;
  uint32_t webhookTimeoutMs = 10000;
  // WO-2026-08-24-001: including the real src/BuildProfile.h (via Config.h)
  // now makes FIELD_BUILD defined for this host build, activating
  // PowerManager.cpp's configuredFallbackProfile() FIELD_BUILD branch, which
  // reads this getter. Defaults to false so configuredFallbackProfile()
  // keeps returning UsbBench here, matching this harness's pre-existing
  // behavior (previously the undefined-FIELD_BUILD `#else` branch, which
  // always returned UsbBench unconditionally).
  bool solarPowerMode = false;

  uint16_t get_connectAttemptBudgetSec() const { return connectAttemptBudgetSec; }
  uint8_t get_connectionAttemptCounter() const { return connectionAttemptCounter; }
  uint8_t get_currentBatteryTier() const { return currentBatteryTier; }
  uint8_t get_resetCount() const { return resetCount; }
  time_t get_lastReport() const { return lastReport; }
  bool get_verboseMode() const { return verboseMode; }
  uint16_t get_verboseTimeoutMin() const { return verboseTimeoutMin; }
  const char *get_timeZoneStrCStr() const { return timeZoneStrCStr; }
  uint16_t get_reportingInterval() const { return reportingInterval; }
  uint8_t get_openTime() const { return openTime; }
  uint8_t get_closeTime() const { return closeTime; }
  bool get_serialConnected() const { return serialConnected; }
  uint8_t get_sensorMode() const { return sensorMode; }
  uint8_t get_connectionMode() const { return connectionMode; }
  uint8_t get_reportingMode() const { return reportingMode; }
  uint8_t get_samplingMode() const { return samplingMode; }
  uint16_t get_cloudDisconnectBudgetSec() const { return cloudDisconnectBudgetSec; }
  uint16_t get_modemOffBudgetSec() const { return modemOffBudgetSec; }
  bool get_enableHibernateSleep() const { return enableHibernateSleep; }
  const char *get_webhookNameCStr() const { return webhookNameCStr; }
  bool get_webhookEnabled() const { return webhookEnabled; }
  uint32_t get_webhookTimeoutMs() const { return webhookTimeoutMs; }
  bool get_solarPowerMode() const { return solarPowerMode; }
};

struct TestSensorConfig {
  uint8_t sensorType = 1;
  uint32_t sensorSetting1 = 0;
  uint32_t sensorSetting2 = 0;
  uint32_t sensorSetting3 = 0;
  uint32_t sensorSetting4 = 0;

  uint8_t get_sensorType() const { return sensorType; }
  uint32_t get_sensorSetting1() const { return sensorSetting1; }
  uint32_t get_sensorSetting2() const { return sensorSetting2; }
  uint32_t get_sensorSetting3() const { return sensorSetting3; }
  uint32_t get_sensorSetting4() const { return sensorSetting4; }
};

extern TestCurrentStatus testCurrent;
extern TestSystemStatus testSysStatus;
extern TestSensorConfig testSensorConfig;

#define current testCurrent
#define sysStatus testSysStatus
#define sensorConfig testSensorConfig