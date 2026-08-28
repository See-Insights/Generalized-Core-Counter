#pragma once

/**
 * @file PowerTier.h
 * @brief F3 - power state / tier (WO-2026-08-25-001).
 *
 * Pure. No state, no I/O, no previous-tier input. `evaluate()` reuses Phase
 * 2's already-shipped SoC breakpoints (see BatteryBackoff::calculateTier in
 * src/cloud/BatteryBackoffPolicy.h: 75/70/55/50/35/30) collapsed 1:1 onto the
 * four tiers here (Healthy->Full, Conserving->Reduced, Critical->Low,
 * Survival->Critical). Because this function takes no previous-tier
 * argument, it necessarily drops Phase 2's hysteresis dead-zones (70-75,
 * 50-55, 30-35) that exist to stop the *cloud reporting cadence* from
 * chattering across a boundary; that hysteresis still runs unchanged in
 * BatteryBackoff for reporting cadence. This function is a second, purely
 * advisory signal, not a replacement for it - see the work order's "Phase
 * 2's design, reused unchanged" note.
 *
 * The one new rule not in Phase 2: Critical is gated on vcell (<= 3.5 V),
 * never on soc - a low-but-plausible SoC reading during a real brownout must
 * not be trusted to say "not critical". This is checked unconditionally,
 * regardless of trust.
 *
 * When trust != Trusted, soc is not used at all; the tier is derived from
 * BatteryHealth::restingSocFromVcell(vcell) through the same breakpoints
 * instead. This is F1's signal being consumed exactly as the WO specifies -
 * F3 stops depending on soc rather than trying to repair it.
 */

#include "power/BatteryHealth.h"

namespace PowerTier {

enum class Tier : uint8_t { Full, Reduced, Low, Critical };

/// Vcell at/below this forces Critical unconditionally, regardless of soc or trust.
constexpr float kCriticalVcell = 3.5f;

/// SoC breakpoints reused from BatteryBackoff::calculateTier (Phase 2), without
/// its hysteresis (this function has no previous-tier input).
constexpr float kFullSocThreshold = 75.0f;
constexpr float kReducedSocThreshold = 55.0f;
constexpr float kLowSocThreshold = 35.0f;

/**
 * @brief Converts a tier value to a stable log label.
 */
const char *label(Tier tier);

/**
 * @brief Pure tier evaluation from cell voltage, gauge SoC, and F1's trust signal.
 *
 * @param vcell Measured cell voltage
 * @param soc Gauge-reported state of charge (ignored unless trust == Trusted)
 * @param trust F1's SocTrust signal for this measurement
 * @return The evaluated tier
 */
Tier evaluate(float vcell, float soc, BatteryHealth::SocTrust trust);

} // namespace PowerTier
