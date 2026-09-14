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
// WO-2026-09-14-001: this used to be a relative include of the main
// persistence header. That header does not have the name-collision problem
// above, so the relative path bought nothing here except the same
// unshadowable-include defect. Replaced with a narrow, non-relative seam
// exposing only the one accessor this file needs; see
// reporting/BatteryTierStore.h for the full rationale.
#include "reporting/BatteryTierStore.h"
#include "power/ConnectivityPolicy.h"
#include "reporting/BatteryTierGuard.h"
#include "sensors/SensorManager.h"
#include "state/State_Common.h"

namespace {

bool runtimeWindowOpenAt(time_t epoch, void *) {
	return isWithinOpenHoursAt(epoch);
}

} // namespace

namespace ReportingPolicyResolver {

ReportingPolicy resolveRuntime(float currentSoC, time_t nowEpoch) {
	uint8_t previousTierValue = BatteryTierStore::currentBatteryTier();
	BatteryTier previousTier = previousTierValue <= TIER_SURVIVAL
		? static_cast<BatteryTier>(previousTierValue)
		: TIER_HEALTHY;

	// WO-2026-08-25-001 Amendment B, Decision B1 (AC-B1/AC-B2/AC-B3), revised
	// by Amendment C, Decision C2 (AC-C6): BatteryBackoff remains the sole
	// tier authority for the Known-vcell case (hysteresis, persistence,
	// breakpoints, unchanged). The guard is now TOTAL over all three
	// SensorManager::VcellSampleState values instead of collapsing
	// Invalid and Unavailable into the same "skip the guard" branch - that
	// collapse is exactly what let an INVALID vcell (<=2.5V, >=5V, NaN)
	// bypass both the trust substitution and the 3.5V floor and drive
	// cadence off raw, untrusted SoC (Blocker B / Codex Stage 7 R3 AC-B2,
	// AC-B3 findings).
	float vcell = 0.0f;
	const SensorManager::VcellSampleState vcellState =
		SensorManager::instance().cachedBatteryVoltageState(vcell);
	const BatteryHealth::SocTrust trust = SensorManager::instance().cachedSocTrust();

	BatteryTier tier;
	switch (vcellState) {
	case SensorManager::VcellSampleState::Known: {
		// Normal case: trust-gated input, then the unconditional vcell
		// floor, exactly as before.
		const float socForTier = BatteryTierGuard::socForTier(currentSoC, vcell, trust);
		tier = BatteryTierGuard::applyVcellFloor(
			BatteryBackoff::calculateTier(socForTier, previousTier),
			vcell, currentSoC, trust);
		break;
	}
	case SensorManager::VcellSampleState::Invalid:
		// A vcell sample WAS taken this call but is not physically
		// plausible (<=2.5V, >=5V, or NaN). This is not evidence the
		// battery is healthy - it is evidence the reading cannot be
		// trusted at all, which is at least as bad as a Known critical
		// vcell. Do not feed it (or the untrusted raw SoC) into
		// calculateTier(); force the same conservative floor a Known
		// vcell at/below PowerTier::kCriticalVcell would produce.
		tier = TIER_SURVIVAL;
		break;
	case SensorManager::VcellSampleState::Unavailable:
	default:
		// No vcell sample has been taken yet this boot (e.g. before the
		// first Boron sample, or a non-Boron platform that never
		// populates vcell at all).
		//
		// WO-2026-08-25-001 Amendment C, Decision C2 (AC-C6): the guard
		// must be total, so Unavailable is no longer a single branch that
		// always trusts the raw gauge SoC. `trust` still needs to be
		// consulted, because SensorManager::cachedSocTrust() can already
		// be Untrusted here (e.g. a platform/build that ran F1's trust
		// evaluation without ever populating vcell). An untrusted SoC must
		// not be allowed to act as though it were trusted just because
		// there is no vcell reading to floor it against - there is also no
		// vcell to substitute via BatteryHealth::restingSocFromVcell(), so
		// the only safe choice is the same conservative floor Invalid
		// uses. When trust IS (still) Trusted - the pre-first-sample
		// bootstrap default - this intentionally matches legacy behavior
		// (SoC-driven tiering) until a real sample arrives, per
		// SensorManager::cachedSocTrust()'s header comment.
		tier = (trust == BatteryHealth::SocTrust::Trusted)
			? BatteryBackoff::calculateTier(currentSoC, previousTier)
			: TIER_SURVIVAL;
		break;
	}

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
