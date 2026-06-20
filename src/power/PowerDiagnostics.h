#pragma once

#include "power/PowerManager.h"

namespace PowerDiagnostics {

const char *availabilityLabel(PowerAvailability availability);
const char *batteryContextLabel(PowerBatteryContext context);
const char *tierLabel(PowerTier tier);
const char *inputProfileLabel(PowerInputProfile profile);
const char *powerSourceLabel(int powerSource);
const char *profileSelectionReasonLabel(PowerProfileSelectionReason reason);

/**
 * @brief Emit compact diagnostic log of current power state.
 *
 * Logs System.powerSource(), active input profile, PMIC VBUS/power-good state,
 * battery SOC, and Nordic USB registers (when supported by platform).
 *
 * @param reason Context label for this diagnostic emission
 * @param forceLog If true, log even if state hasn't changed since last call
 */
void logPowerState(const char *reason, bool forceLog = false);

} // namespace PowerDiagnostics