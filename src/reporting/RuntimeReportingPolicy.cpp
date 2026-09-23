#include "reporting/ReportingPolicy.h"

// WO-2026-09-14-001: this used to be a relative include of the shared
// configuration header directly (relative because Device OS ships its own
// services/inc header of the same name, and a bare non-relative include can
// resolve to that one under the local toolchain's include order - see
// reporting/ReportingIntervalStore.cpp, which still uses the relative form
// for that reason, one file removed from here). The relative form here
// bought nothing beyond that name-collision protection, and cost the same
// unshadowable-include defect Step 0 fixed for the persistence header: a
// test's -I override directory cannot shadow a relative include. Replaced
// with a narrow, non-relative seam exposing only the one accessor this file
// needs; see reporting/ReportingIntervalStore.h for the full rationale.
#include "reporting/ReportingIntervalStore.h"
// WO-2026-09-21 Step 4 (corrected same day, twice): reporting/BatteryTierStore.h -
// Step 0's narrow read seam onto PowerConfig::get_currentBatteryTier() - folds
// into power/BatteryAuthority.h's currentTier(). This file no longer samples
// SensorManager itself either: BatteryAuthority::evaluateCurrent() is the
// one input-gathering path all read paths (this adapter, the connectivity
// failsafe) share, so they cannot silently drift from each other on how
// vcell/trust are gathered. This is a read-only call: resolveRuntime()
// never commits anything.
#include "power/BatteryAuthority.h"
#include "power/ConnectivityPolicy.h"
#include "state/State_Common.h"
// WO-2026-09-22: clockTrusted now comes from the actual owner
// (Clock::isTrusted()), not raw Time.isValid() - the same class of gap
// Step 3b's decision-site sweep closed everywhere else, missed here because
// this file wasn't in that sweep's original ~13-site list.
#include "time/Clock.h"

namespace {

bool runtimeWindowOpenAt(time_t epoch, void *) {
	return isWithinOpenHoursAt(epoch);
}

} // namespace

namespace ReportingPolicyResolver {

ReportingPolicy resolveRuntime(float currentSoC, time_t nowEpoch) {
	const BatteryTier tier = BatteryAuthority::evaluateCurrent(currentSoC).tier;

	ReportingPolicyInputs inputs;
	inputs.configuredIntervalSec = ReportingIntervalStore::reportingIntervalSec();
	inputs.batteryTier = tier;
	inputs.batteryMultiplier = BatteryBackoff::intervalMultiplier(tier);
	inputs.nowEpoch = nowEpoch;
	inputs.clockTrusted = Clock::isTrusted();
	inputs.windowOpen = isWithinOpenHoursAt(nowEpoch);
	inputs.alignmentToleranceSec = ConnectivityPolicy::CONNECT_ALIGNMENT_TOLERANCE_SEC;
	return resolve(inputs, runtimeWindowOpenAt, nullptr);
}

} // namespace ReportingPolicyResolver
