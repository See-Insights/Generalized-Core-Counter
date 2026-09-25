#pragma once

// Facade stub (WO-2026-09-23-001 Step 5) - see persist/SystemConfig.h.
// resetCount is recovery/reset accounting, so it is reached through
// RecoveryState here exactly as the real facade exposes it.

#include "MyPersistentData.h"

namespace RecoveryState {

inline uint8_t get_resetCount() { return testSysStatus.get_resetCount(); }

}  // namespace RecoveryState
