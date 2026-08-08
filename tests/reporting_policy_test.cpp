#include <assert.h>
#include <stdio.h>

#include "cloud/BatteryBackoffPolicy.h"
#include "reporting/ReportingPolicy.h"

namespace {

bool openAtOrAfter(time_t epoch, void *context) {
	const time_t firstOpenEpoch = *static_cast<time_t *>(context);
	return epoch >= firstOpenEpoch;
}

ReportingPolicy policyForTier(BatteryTier tier, time_t now = 43200) {
	ReportingPolicyInputs inputs;
	inputs.configuredIntervalSec = 3600;
	inputs.batteryTier = tier;
	inputs.batteryMultiplier = BatteryBackoff::intervalMultiplier(tier);
	inputs.nowEpoch = now;
	inputs.timeValid = true;
	inputs.windowOpen = true;
	inputs.alignmentToleranceSec = 30;
	return ReportingPolicyResolver::resolve(inputs);
}

void testTierIntervals() {
	const ReportingPolicy healthy = policyForTier(TIER_HEALTHY);
	assert(healthy.configuredIntervalSec == 3600);
	assert(healthy.effectiveIntervalSec == 3600);
	assert(healthy.adjustmentReason == REPORTING_ADJUSTMENT_NONE);
	assert(policyForTier(TIER_CONSERVING).effectiveIntervalSec == 7200);
	assert(policyForTier(TIER_CRITICAL).effectiveIntervalSec == 14400);
	const ReportingPolicy survival = policyForTier(TIER_SURVIVAL);
	assert(survival.effectiveIntervalSec == 43200);
	assert(survival.adjustmentReason == REPORTING_ADJUSTMENT_LOW_BATTERY);
}

void testSurvivalFieldCaseAndNoDoubleMultiplier() {
	const BatteryTier tier = BatteryBackoff::calculateTier(4.3f, TIER_HEALTHY);
	assert(tier == TIER_SURVIVAL);
	const ReportingPolicy policy = policyForTier(tier);
	assert(policy.batteryMultiplier == 12);
	assert(policy.effectiveIntervalSec == 43200);
	assert(policy.nextReportEpoch == 86400);
	assert(policy.cadenceDue);
}

void testHysteresisAndRecovery() {
	assert(BatteryBackoff::calculateTier(70.0f, TIER_HEALTHY) == TIER_HEALTHY);
	assert(BatteryBackoff::calculateTier(70.0f, TIER_CONSERVING) == TIER_CONSERVING);
	assert(BatteryBackoff::calculateTier(50.0f, TIER_CONSERVING) == TIER_CONSERVING);
	assert(BatteryBackoff::calculateTier(50.0f, TIER_CRITICAL) == TIER_CRITICAL);
	assert(BatteryBackoff::calculateTier(30.0f, TIER_CRITICAL) == TIER_CRITICAL);
	assert(BatteryBackoff::calculateTier(30.0f, TIER_SURVIVAL) == TIER_SURVIVAL);
	assert(BatteryBackoff::calculateTier(80.0f, TIER_SURVIVAL) == TIER_HEALTHY);
	assert(policyForTier(TIER_HEALTHY).effectiveIntervalSec == 3600);
}

void testClosedWindowFindsNextValidBoundary() {
	ReportingPolicyInputs inputs;
	inputs.configuredIntervalSec = 3600;
	inputs.batteryTier = TIER_HEALTHY;
	inputs.batteryMultiplier = BatteryBackoff::intervalMultiplier(TIER_HEALTHY);
	inputs.nowEpoch = 3600;
	inputs.timeValid = true;
	inputs.windowOpen = false;
	inputs.alignmentToleranceSec = 30;
	time_t firstOpenEpoch = 10800;

	const ReportingPolicy policy = ReportingPolicyResolver::resolve(
		inputs, openAtOrAfter, &firstOpenEpoch);
	assert(!policy.windowOpen);
	assert(!policy.cadenceDue);
	assert(policy.nextReportEpoch == firstOpenEpoch);
}

void testConsumersReceiveSameResolvedCadence() {
	ReportingPolicyInputs inputs;
	inputs.configuredIntervalSec = 3600;
	inputs.batteryTier = TIER_SURVIVAL;
	inputs.batteryMultiplier = BatteryBackoff::intervalMultiplier(TIER_SURVIVAL);
	inputs.nowEpoch = 43200;
	inputs.timeValid = true;
	inputs.windowOpen = true;
	inputs.alignmentToleranceSec = 30;

	const ReportingPolicy stateReportView = ReportingPolicyResolver::resolve(inputs);
	const ReportingPolicy deviceStatusView = ReportingPolicyResolver::resolve(inputs);
	assert(stateReportView.effectiveIntervalSec == deviceStatusView.effectiveIntervalSec);
	assert(stateReportView.nextReportEpoch == deviceStatusView.nextReportEpoch);
	assert(stateReportView.effectiveIntervalSec == 43200);
}

} // namespace

int main() {
	testTierIntervals();
	testSurvivalFieldCaseAndNoDoubleMultiplier();
	testHysteresisAndRecovery();
	testClosedWindowFindsNextValidBoundary();
	testConsumersReceiveSameResolvedCadence();
	puts("reporting_policy_test: PASS");
	return 0;
}
