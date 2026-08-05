#include "reporting/ReportingPolicy.h"

#include <limits>

namespace {

constexpr uint8_t kMaxFutureBoundaryChecks = 64;

uint32_t effectiveInterval(uint32_t configuredIntervalSec, uint16_t multiplier) {
	if (configuredIntervalSec == 0 || multiplier == 0) {
		return 0;
	}
	if (configuredIntervalSec > UINT32_MAX / multiplier) {
		return UINT32_MAX;
	}
	return configuredIntervalSec * multiplier;
}

bool isBoundaryDue(time_t nowEpoch, uint32_t intervalSec, uint32_t toleranceSec) {
	if (nowEpoch < 0 || intervalSec == 0) {
		return false;
	}
	const uint32_t tolerance = toleranceSec < intervalSec ? toleranceSec : intervalSec - 1;
	const uint32_t offset = (uint32_t)((uint64_t)nowEpoch % intervalSec);
	return offset <= tolerance || offset >= intervalSec - tolerance;
}

time_t nextValidBoundary(const ReportingPolicyInputs &inputs,
		uint32_t intervalSec,
		ReportingWindowPredicate windowPredicate,
		void *windowContext) {
	if (!inputs.timeValid || inputs.nowEpoch < 0 || intervalSec == 0) {
		return 0;
	}

	const uint64_t earliest = (uint64_t)inputs.nowEpoch + inputs.alignmentToleranceSec;
	uint64_t candidate = ((earliest / intervalSec) + 1ULL) * intervalSec;
	const uint64_t maxEpoch = (uint64_t)std::numeric_limits<time_t>::max();
	for (uint8_t i = 0; i < kMaxFutureBoundaryChecks && candidate <= maxEpoch; ++i) {
		const time_t epoch = (time_t)candidate;
		if (!windowPredicate || windowPredicate(epoch, windowContext)) {
			return epoch;
		}
		if (candidate > maxEpoch - intervalSec) {
			break;
		}
		candidate += intervalSec;
	}
	return 0;
}

} // namespace

namespace ReportingPolicyResolver {

ReportingPolicy resolve(const ReportingPolicyInputs &inputs,
		ReportingWindowPredicate windowPredicate,
		void *windowContext) {
	ReportingPolicy policy;
	policy.configuredIntervalSec = inputs.configuredIntervalSec;
	policy.batteryTier = inputs.batteryTier;
	policy.batteryMultiplier = inputs.batteryMultiplier == 0 ? 1 : inputs.batteryMultiplier;
	policy.effectiveIntervalSec = effectiveInterval(
		inputs.configuredIntervalSec, policy.batteryMultiplier);
	policy.windowOpen = inputs.windowOpen;
	policy.cadenceDue = inputs.windowOpen && isBoundaryDue(
		inputs.nowEpoch, policy.effectiveIntervalSec, inputs.alignmentToleranceSec);
	policy.adjustmentReason = policy.batteryMultiplier > 1
		? REPORTING_ADJUSTMENT_LOW_BATTERY
		: REPORTING_ADJUSTMENT_NONE;
	policy.nextReportEpoch = nextValidBoundary(
		inputs, policy.effectiveIntervalSec, windowPredicate, windowContext);
	return policy;
}

const char *adjustmentReasonName(ReportingAdjustmentReason reason) {
	return reason == REPORTING_ADJUSTMENT_LOW_BATTERY ? "low-battery" : "none";
}

const char *batteryTierName(BatteryTier tier) {
	switch (tier) {
		case TIER_CONSERVING:
			return "CONSERVING";
		case TIER_CRITICAL:
			return "CRITICAL";
		case TIER_SURVIVAL:
			return "SURVIVAL";
		case TIER_HEALTHY:
		default:
			return "HEALTHY";
	}
}

} // namespace ReportingPolicyResolver
