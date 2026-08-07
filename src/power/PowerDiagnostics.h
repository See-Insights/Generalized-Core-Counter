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

/**
 * @brief Bench-only: capture a ChargeDiag reading into the diagnostics batch.
 *
 * No-op unless ENABLE_DIAGNOSTICS_PUBLISH_MODE=1. Call alongside (not instead
 * of) the existing ChargeDiag Log.info emission.
 */
void recordChargeDiagEvent(uint8_t chargeStatus, uint8_t faultReg, bool charging,
                            float vcell, float soc, int powerSource,
                            PowerInputProfile profile);

/**
 * @brief Bench-only: capture a stale-SOC fuel-gauge resync into the diagnostics batch.
 *
 * No-op unless ENABLE_DIAGNOSTICS_PUBLISH_MODE=1. Call alongside (not instead
 * of) the existing STALE_SOC resync Log.warn emission.
 */
void recordResyncEvent(float soc, float vcell);

/**
 * @brief Bench-only: serialize and enqueue the accumulated diagnostics batch.
 *
 * No-op unless ENABLE_DIAGNOSTICS_PUBLISH_MODE=1, and a no-op if the batch is
 * empty. Must be called only at true cycle-ending points (immediately before
 * a call that may reset the MCU, or once after a sleep-attempt cascade
 * resolves) - never once per pre-sleep call site, since those are sequential
 * fallback attempts within a single cycle, not mutually exclusive branches.
 */
void flushDiagBatch();

} // namespace PowerDiagnostics