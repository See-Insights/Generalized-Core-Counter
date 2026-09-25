#pragma once

// Facade stub (WO-2026-09-23-001 Step 5) - see persist/SystemConfig.h.

#include "MyPersistentData.h"

namespace PowerConfig {

inline uint8_t get_currentBatteryTier() { return testSysStatus.get_currentBatteryTier(); }

}  // namespace PowerConfig
