#pragma once

#include <stdint.h>

/**
 * @brief Battery tier used by the cloud-reporting backoff policy.
 */
enum BatteryTier : uint8_t {
	TIER_HEALTHY    = 0,
	TIER_CONSERVING = 1,
	TIER_CRITICAL   = 2,
	TIER_SURVIVAL   = 3
};

namespace BatteryBackoff {

/**
 * @brief Resolves the battery tier without reading or changing runtime state.
 */
inline BatteryTier calculateTier(float currentSoC, BatteryTier previousTier) {
	if (currentSoC >= 75.0f) {
		return TIER_HEALTHY;
	}
	if (currentSoC >= 70.0f) {
		return previousTier == TIER_HEALTHY ? TIER_HEALTHY : TIER_CONSERVING;
	}
	if (currentSoC >= 55.0f) {
		return TIER_CONSERVING;
	}
	if (currentSoC >= 50.0f) {
		return previousTier <= TIER_CONSERVING ? TIER_CONSERVING : TIER_CRITICAL;
	}
	if (currentSoC >= 35.0f) {
		return TIER_CRITICAL;
	}
	if (currentSoC >= 30.0f) {
		return previousTier <= TIER_CRITICAL ? TIER_CRITICAL : TIER_SURVIVAL;
	}
	return TIER_SURVIVAL;
}

/**
 * @brief Returns the authoritative cloud-reporting multiplier for a tier.
 */
inline uint16_t intervalMultiplier(BatteryTier tier) {
	switch (tier) {
		case TIER_CONSERVING:
			return 2;
		case TIER_CRITICAL:
			return 4;
		case TIER_SURVIVAL:
			return 12;
		case TIER_HEALTHY:
		default:
			return 1;
	}
}

} // namespace BatteryBackoff
