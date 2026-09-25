#pragma once

// Facade stub (WO-2026-09-23-001 Step 5) - see persist/SystemConfig.h.

#include "MyPersistentData.h"

namespace CurrentReadings {

inline float get_internalTempC() { return testCurrent.get_internalTempC(); }
inline float get_stateOfCharge() { return testCurrent.get_stateOfCharge(); }
inline uint8_t get_batteryState() { return testCurrent.get_batteryState(); }
inline void set_batteryState(uint8_t v) { testCurrent.set_batteryState(v); }

}  // namespace CurrentReadings
