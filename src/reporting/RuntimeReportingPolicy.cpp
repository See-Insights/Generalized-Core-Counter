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
// WO-2026-09-21 Step 4 (corrected same day): reporting/BatteryTierStore.h -
// Step 0's narrow read seam onto sysStatus.get_currentBatteryTier() - folds
// into power/BatteryAuthority.h's currentTier(), this file's own equally-
// narrow, equally-shadowable seam onto the persisted tier. The guard
// pipeline itself (BatteryTierGuard, PowerTier, BatteryBackoff) now lives in
// BatteryAuthority::evaluate() - a PURE function - so this file gathers the
// vcell/trust inputs itself (via SensorManager, as it always has) and passes
// them in, rather than evaluate() reaching for SensorManager internally.
// This is a read-only call: resolveRuntime() never commits anything.
#include "power/BatteryAuthority.h"
#include "power/ConnectivityPolicy.h"
#include "sensors/SensorManager.h"
#include "state/State_Common.h"

namespace {

bool runtimeWindowOpenAt(time_t epoch, void *) {
	return isWithinOpenHoursAt(epoch);
}

} // namespace

namespace ReportingPolicyResolver {

ReportingPolicy resolveRuntime(float currentSoC, time_t nowEpoch) {
	float vcell = 0.0f;
	const SensorManager::VcellSampleState vcellState =
		SensorManager::instance().cachedBatteryVoltageState(vcell);
	const BatteryHealth::SocTrust trust = SensorManager::instance().cachedSocTrust();

	const BatteryTier tier = BatteryAuthority::evaluate(
		currentSoC, vcellState, vcell, trust, BatteryAuthority::currentTier()).tier;

	ReportingPolicyInputs inputs;
	inputs.configuredIntervalSec = ReportingIntervalStore::reportingIntervalSec();
	inputs.batteryTier = tier;
	inputs.batteryMultiplier = BatteryBackoff::intervalMultiplier(tier);
	inputs.nowEpoch = nowEpoch;
	inputs.timeValid = Time.isValid();
	inputs.windowOpen = isWithinOpenHoursAt(nowEpoch);
	inputs.alignmentToleranceSec = ConnectivityPolicy::CONNECT_ALIGNMENT_TOLERANCE_SEC;
	return resolve(inputs, runtimeWindowOpenAt, nullptr);
}

} // namespace ReportingPolicyResolver
