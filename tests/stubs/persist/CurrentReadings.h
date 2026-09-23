#pragma once

// Facade stub (WO-2026-09-23-001 Step 5) - see persist/SystemConfig.h.
// The read counters live on the test object, so routing through the facade
// preserves this harness's existing "how many times was SoC read" assertions.

#include "MyPersistentData.h"

namespace CurrentReadings {

inline float get_stateOfCharge() { return testCurrent.get_stateOfCharge(); }
inline uint8_t get_batteryState() { return testCurrent.get_batteryState(); }

}  // namespace CurrentReadings
