#pragma once

// Facade stub (WO-2026-09-23-001 Step 5) - see persist/SystemConfig.h.
// Decision A: alert state is reached through RecoveryState even though it is
// physically CurrentData, so the alert-severity assertions in
// pmic_fault_monitor_test.cpp exercise the same path production now uses.

#include "MyPersistentData.h"

namespace RecoveryState {

inline int8_t get_alertCode() { return testCurrent.get_alertCode(); }
inline void set_alertCode(int8_t v) { testCurrent.set_alertCode(v); }
inline void set_lastAlertTime(time_t v) { testCurrent.set_lastAlertTime(v); }
inline void raiseAlert(int8_t code) { testCurrent.raiseAlert(code); }

}  // namespace RecoveryState
