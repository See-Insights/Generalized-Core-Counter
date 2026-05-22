#pragma once

#include "power/PowerManager.h"

namespace PowerDiagnostics {

const char *availabilityLabel(PowerAvailability availability);
const char *batteryContextLabel(PowerBatteryContext context);
const char *tierLabel(PowerTier tier);
const char *inputProfileLabel(PowerInputProfile profile);
const char *powerSourceLabel(int powerSource);
const char *profileSelectionReasonLabel(PowerProfileSelectionReason reason);

} // namespace PowerDiagnostics