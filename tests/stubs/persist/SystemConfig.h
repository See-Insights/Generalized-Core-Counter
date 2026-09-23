#pragma once

// Facade stub (WO-2026-09-23-001 Step 5). Production code calls
// SystemConfig::/PowerConfig::/CurrentReadings::, never a `sysStatus`/`current`
// receiver, so the shared host stub exposes the same narrow surface. Only the
// accessors this harness's sources actually call are declared - a production
// file that starts reading a new field fails to link here rather than silently
// picking up a stale shadow of the full interface.

#include "MyPersistentData.h"

namespace SystemConfig {

inline uint16_t get_connectAttemptBudgetSec() { return testSysStatus.get_connectAttemptBudgetSec(); }
inline uint8_t get_connectionAttemptCounter() { return testSysStatus.get_connectionAttemptCounter(); }

}  // namespace SystemConfig
