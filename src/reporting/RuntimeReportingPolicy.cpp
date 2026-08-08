#include "reporting/ReportingPolicy.h"

#include "../Config.h"
#include "../MyPersistentData.h"
#include "power/ConnectivityPolicy.h"
#include "state/State_Common.h"

namespace {

bool runtimeWindowOpenAt(time_t epoch, void *) {
	return isWithinOpenHoursAt(epoch);
}

} // namespace

namespace ReportingPolicyResolver {

ReportingPolicy resolveRuntime(float currentSoC, time_t nowEpoch) {
	uint8_t previousTierValue = sysStatus.get_currentBatteryTier();
	BatteryTier previousTier = previousTierValue <= TIER_SURVIVAL
		? static_cast<BatteryTier>(previousTierValue)
		: TIER_HEALTHY;
	const BatteryTier tier = BatteryBackoff::calculateTier(currentSoC, previousTier);

	ReportingPolicyInputs inputs;
	inputs.configuredIntervalSec = Config::reportingIntervalSecForRuntime();
	inputs.batteryTier = tier;
	inputs.batteryMultiplier = BatteryBackoff::intervalMultiplier(tier);
	inputs.nowEpoch = nowEpoch;
	inputs.timeValid = Time.isValid();
	inputs.windowOpen = isWithinOpenHoursAt(nowEpoch);
	inputs.alignmentToleranceSec = ConnectivityPolicy::CONNECT_ALIGNMENT_TOLERANCE_SEC;
	return resolve(inputs, runtimeWindowOpenAt, nullptr);
}

} // namespace ReportingPolicyResolver
