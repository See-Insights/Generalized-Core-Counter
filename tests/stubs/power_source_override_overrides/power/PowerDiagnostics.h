#pragma once

#include "power/PowerManager.h"

namespace PowerDiagnostics {

inline void logPowerState(const char *, bool = false) {}
inline const char *availabilityLabel(PowerAvailability) { return "availability"; }
inline const char *batteryContextLabel(PowerBatteryContext) { return "battery"; }
inline const char *tierLabel(PowerTier) { return "tier"; }
inline const char *inputProfileLabel(PowerInputProfile) { return "profile"; }
inline const char *powerSourceLabel(int) { return "source"; }
inline const char *profileSelectionReasonLabel(PowerProfileSelectionReason) { return "reason"; }

} // namespace PowerDiagnostics