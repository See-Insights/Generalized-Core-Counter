#pragma once

// Facade stub (WO-2026-09-23-001 Step 5), scoped to what
// src/power/PmicFaultMonitor.cpp and the real src/power/PowerManager.cpp it
// links against actually call. Trivial pass-throughs onto the test objects;
// no production decision logic is reimplemented here.

#include "MyPersistentData.h"

namespace SystemConfig {

enum SensorMode { COUNTING = 0, OCCUPANCY = 1, MEASUREMENT = 2 };
enum ConnectionMode { CONNECTED = 0, INTERMITTENT = 1, DISCONNECTED = 2, INTERMITTENT_KEEP_ALIVE = 3 };

inline bool get_verboseMode() { return testSysStatus.get_verboseMode(); }
inline uint8_t get_sensorMode() { return testSysStatus.get_sensorMode(); }
inline uint8_t get_configuredConnectionMode() { return testSysStatus.get_configuredConnectionMode(); }

}  // namespace SystemConfig
