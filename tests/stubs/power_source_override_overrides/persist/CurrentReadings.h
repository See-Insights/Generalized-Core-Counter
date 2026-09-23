#pragma once

// Facade stub (WO-2026-09-23-001 Step 5) - see persist/SystemConfig.h.
// The SoC/battery-state read counters live on the test object, so routing
// through the facade keeps this harness's read-count assertions meaningful.

#include "MyPersistentData.h"

namespace CurrentReadings {

inline float get_stateOfCharge() { return testCurrent.get_stateOfCharge(); }
inline uint8_t get_batteryState() { return testCurrent.get_batteryState(); }
inline bool get_occupied() { return testCurrent.get_occupied(); }
inline uint32_t get_totalOccupiedSeconds() { return testCurrent.get_totalOccupiedSeconds(); }
inline float get_internalTempC() { return testCurrent.get_internalTempC(); }

}  // namespace CurrentReadings
