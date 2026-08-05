#pragma once

#include <stdint.h>
#include <time.h>

#include "cloud/BatteryBackoffPolicy.h"

enum ReportingAdjustmentReason : uint8_t {
	REPORTING_ADJUSTMENT_NONE = 0,
	REPORTING_ADJUSTMENT_LOW_BATTERY = 1,
};

struct ReportingPolicyInputs {
	uint32_t configuredIntervalSec = 0;
	BatteryTier batteryTier = TIER_HEALTHY;
	uint16_t batteryMultiplier = 1;
	time_t nowEpoch = 0;
	bool timeValid = false;
	bool windowOpen = true;
	uint32_t alignmentToleranceSec = 0;
};

struct ReportingPolicy {
	uint32_t configuredIntervalSec = 0;
	BatteryTier batteryTier = TIER_HEALTHY;
	uint16_t batteryMultiplier = 1;
	uint32_t effectiveIntervalSec = 0;
	time_t nextReportEpoch = 0;
	bool windowOpen = true;
	bool cadenceDue = false;
	ReportingAdjustmentReason adjustmentReason = REPORTING_ADJUSTMENT_NONE;
};

using ReportingWindowPredicate = bool (*)(time_t epoch, void *context);

namespace ReportingPolicyResolver {

/**
 * @brief Resolves reporting cadence from explicit inputs without side effects.
 *
 * Future opportunities retain the existing epoch-boundary alignment and are
 * filtered through the supplied open-hours predicate. A zero next epoch means
 * time is invalid, the interval is unusable, or no valid opportunity was found
 * in the bounded search horizon.
 */
ReportingPolicy resolve(const ReportingPolicyInputs &inputs,
		ReportingWindowPredicate windowPredicate = nullptr,
		void *windowContext = nullptr);

/**
 * @brief Resolves policy from the current persisted configuration and runtime state.
 */
ReportingPolicy resolveRuntime(float currentSoC, time_t nowEpoch);

const char *adjustmentReasonName(ReportingAdjustmentReason reason);
const char *batteryTierName(BatteryTier tier);

} // namespace ReportingPolicyResolver
