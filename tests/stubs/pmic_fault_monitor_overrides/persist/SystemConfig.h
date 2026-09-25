#pragma once

// Facade stub (WO-2026-09-23-001 Step 5), scoped to what
// src/power/PmicFaultMonitor.cpp and the real src/power/PowerManager.cpp it
// links against actually call. Trivial pass-throughs onto the test objects;
// no production decision logic is reimplemented here.

#include "MyPersistentData.h"

namespace SystemConfig {

inline bool get_verboseMode() { return testSysStatus.get_verboseMode(); }

}  // namespace SystemConfig
