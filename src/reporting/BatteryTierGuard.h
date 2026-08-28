#pragma once

/**
 * @file BatteryTierGuard.h
 * @brief WO-2026-08-25-001 Amendment B, Decision B1: F1/F3 applied as a guard
 *        on BatteryBackoff::calculateTier()'s input and a floor on its
 *        output, NOT a replacement for it.
 *
 * BatteryBackoff remains the sole authority for tier, hysteresis, and
 * persistence (AC-B1). This module is pure and stateless; it does not read
 * or persist anything. `RuntimeReportingPolicy.cpp` is the only production
 * caller, sourcing vcell/trust from `SensorManager`'s cached values.
 *
 * 1. socForTier(): trust-gated input. When F1's trust signal is not Trusted,
 *    the gauge SoC is not fed to calculateTier() at all - it is replaced by
 *    BatteryHealth::restingSocFromVcell(vcell). Hysteresis, persistence, and
 *    BatteryBackoff's breakpoints are untouched; only the *input* value
 *    changes.
 * 2. applyVcellFloor(): unconditional floor. PowerTier::evaluate() applies
 *    PowerTier::kCriticalVcell (3.5 V) as a floor regardless of soc or trust
 *    - a low-but-plausible SoC during a real brownout must not be able to
 *    report anything better than TIER_SURVIVAL. This is checked in addition
 *    to (not instead of) calculateTier()'s own hysteresis-based result.
 */

#include "cloud/BatteryBackoffPolicy.h"
#include "power/BatteryHealth.h"
#include "power/PowerTier.h"

namespace BatteryTierGuard {

/**
 * @brief Resolves the SoC value to feed into BatteryBackoff::calculateTier().
 *
 * @param gaugeSoc Raw fuel-gauge SoC as reported.
 * @param vcell Measured cell voltage.
 * @param trust F1's SocTrust signal for this sample.
 * @return gaugeSoc when trust == Trusted; BatteryHealth::restingSocFromVcell(vcell) otherwise.
 */
float socForTier(float gaugeSoc, float vcell, BatteryHealth::SocTrust trust);

/**
 * @brief Applies the unconditional vcell floor on top of an already-resolved tier.
 *
 * Calls PowerTier::evaluate(vcell, gaugeSoc, trust) - the one production call
 * site for that function - to determine whether the floor applies. Only the
 * kCriticalVcell floor is adopted here; PowerTier's non-hysteretic SoC-driven
 * tiers are intentionally NOT substituted for BatteryBackoff's result (see
 * Decision B1: doing so would reintroduce reporting-cadence chatter across
 * the 70-75/50-55/30-35 dead-zones).
 *
 * @param tier Tier already resolved by BatteryBackoff::calculateTier(socForTier(...), previousTier).
 * @param vcell Measured cell voltage.
 * @param gaugeSoc Raw fuel-gauge SoC (unsubstituted - PowerTier::evaluate() does its own substitution).
 * @param trust F1's SocTrust signal for this sample.
 * @return TIER_SURVIVAL if vcell is at/below PowerTier::kCriticalVcell, else `tier` unchanged.
 */
BatteryTier applyVcellFloor(BatteryTier tier, float vcell, float gaugeSoc, BatteryHealth::SocTrust trust);

} // namespace BatteryTierGuard
