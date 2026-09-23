#pragma once

// Facade stub (WO-2026-09-23-001 Step 5), scoped to what the real
// src/power/PowerManager.cpp, src/cloud/DeviceStatusPublisher.cpp and
// src/cloud/Cloud.cpp call in this harness. All pass-throughs onto the test
// objects in MyPersistentData.h; no production logic is reimplemented.

#include "MyPersistentData.h"

namespace SystemConfig {

// Relocated from the old global scope: Cloud::getWebhookName()'s switch needs
// these constants to resolve at compile time even though this harness never
// calls it.
enum SensorMode {
  COUNTING    = 0,
  OCCUPANCY   = 1,
  MEASUREMENT = 2
};

inline uint16_t get_connectAttemptBudgetSec() { return testSysStatus.get_connectAttemptBudgetSec(); }
inline uint8_t get_connectionAttemptCounter() { return testSysStatus.get_connectionAttemptCounter(); }
inline time_t get_lastReport() { return testSysStatus.get_lastReport(); }
inline time_t get_lastTimeSync() { return testSysStatus.get_lastTimeSync(); }
inline bool get_verboseMode() { return testSysStatus.get_verboseMode(); }
inline uint16_t get_verboseTimeoutMin() { return testSysStatus.get_verboseTimeoutMin(); }
inline const char *get_timeZoneStrCStr() { return testSysStatus.get_timeZoneStrCStr(); }
inline uint16_t get_reportingInterval() { return testSysStatus.get_reportingInterval(); }
inline uint8_t get_openTime() { return testSysStatus.get_openTime(); }
inline uint8_t get_closeTime() { return testSysStatus.get_closeTime(); }
inline bool get_serialConnected() { return testSysStatus.get_serialConnected(); }
inline uint8_t get_sensorMode() { return testSysStatus.get_sensorMode(); }
inline uint8_t get_connectionMode() { return testSysStatus.get_connectionMode(); }
inline uint8_t get_reportingMode() { return testSysStatus.get_reportingMode(); }
inline uint8_t get_samplingMode() { return testSysStatus.get_samplingMode(); }
inline uint16_t get_cloudDisconnectBudgetSec() { return testSysStatus.get_cloudDisconnectBudgetSec(); }
inline uint16_t get_modemOffBudgetSec() { return testSysStatus.get_modemOffBudgetSec(); }
inline bool get_enableHibernateSleep() { return testSysStatus.get_enableHibernateSleep(); }
inline const char *get_webhookNameCStr() { return testSysStatus.get_webhookNameCStr(); }
inline bool get_webhookEnabled() { return testSysStatus.get_webhookEnabled(); }
inline uint32_t get_webhookTimeoutMs() { return testSysStatus.get_webhookTimeoutMs(); }

namespace SensorSettings {

inline uint8_t get_sensorType() { return testSensorConfig.get_sensorType(); }
inline uint32_t get_sensorSetting1() { return testSensorConfig.get_sensorSetting1(); }
inline uint32_t get_sensorSetting2() { return testSensorConfig.get_sensorSetting2(); }
inline uint32_t get_sensorSetting3() { return testSensorConfig.get_sensorSetting3(); }
inline uint32_t get_sensorSetting4() { return testSensorConfig.get_sensorSetting4(); }

}  // namespace SensorSettings

}  // namespace SystemConfig
