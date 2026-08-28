// WO-2026-08-25-001 Amendment C, Decision C2 (AC-C6): exercises the actual
// PRODUCTION adapter (ReportingPolicyResolver::resolveRuntime() in
// src/reporting/RuntimeReportingPolicy.cpp), not merely
// BatteryTierGuard/PowerTier in isolation - Codex Stage 7 Round 3 found that
// AC-B2/AC-B3 unit tests passed while the real adapter still let an invalid
// or unavailable vcell silently bypass the trust substitution and the 3.5V
// floor. This test compiles and calls resolveRuntime() itself, driving the
// stub SensorManager's VcellSampleState the same way the real
// SensorManager::cachedBatteryVoltageState() would report it.

#include <cassert>
#include <cstdio>

#include "Config.h"
#include "MyPersistentData.h"
#include "Particle.h"
#include "reporting/ReportingPolicy.h"
#include "sensors/SensorManager.h"
#include "state/State_Common.h"

namespace {

void resetGlobals() {
	testSysStatus.currentBatteryTier = TIER_HEALTHY;
	Config::testReportingIntervalSec = 3600;
	Time.valid = true;
	testWindowOpen = true;
	SensorManager::instance().testVcellState = SensorManager::VcellSampleState::Unavailable;
	SensorManager::instance().testVcell = 0.0f;
	SensorManager::instance().testTrust = BatteryHealth::SocTrust::Trusted;
}

// --- Known: unchanged legacy-guard behavior (trust-gated input + floor). ---

void testKnownTrustedHighSocResolvesHealthy() {
	resetGlobals();
	SensorManager::instance().testVcellState = SensorManager::VcellSampleState::Known;
	SensorManager::instance().testVcell = 4.0f;
	SensorManager::instance().testTrust = BatteryHealth::SocTrust::Trusted;

	const ReportingPolicy policy = ReportingPolicyResolver::resolveRuntime(90.0f, 43200);
	assert(policy.batteryTier == TIER_HEALTHY);
}

void testKnownCriticalVcellFloorsToSurvivalEvenWithHighSoc() {
	resetGlobals();
	SensorManager::instance().testVcellState = SensorManager::VcellSampleState::Known;
	SensorManager::instance().testVcell = 3.4f; // <= PowerTier::kCriticalVcell (3.5V)
	SensorManager::instance().testTrust = BatteryHealth::SocTrust::Trusted;

	// AC-B3: a measured vcell at/below the critical floor must never resolve
	// better than Survival, regardless of a high raw SoC.
	const ReportingPolicy policy = ReportingPolicyResolver::resolveRuntime(90.0f, 43200);
	assert(policy.batteryTier == TIER_SURVIVAL);
}

void testKnownUntrustedSocIsSubstitutedByRestingEstimate() {
	resetGlobals();
	SensorManager::instance().testVcellState = SensorManager::VcellSampleState::Known;
	SensorManager::instance().testVcell = 3.7f; // plausible mid-range resting voltage
	SensorManager::instance().testTrust = BatteryHealth::SocTrust::Untrusted;

	// AC-B2: an untrusted gauge SoC must not drive cadence directly - a
	// wildly-high raw SoC (99%) at a mid-range vcell should NOT resolve
	// Healthy once substituted by the vcell-derived resting estimate.
	const ReportingPolicy policy = ReportingPolicyResolver::resolveRuntime(99.0f, 43200);
	assert(policy.batteryTier != TIER_HEALTHY);
}

// --- Invalid: the exact production gap Blocker B/Codex Stage 7 R3 found. ---

void testInvalidVcellForcesSurvivalRegardlessOfRawSoc() {
	resetGlobals();
	SensorManager::instance().testVcellState = SensorManager::VcellSampleState::Invalid;
	SensorManager::instance().testVcell = 6.0f; // implausible, >= 5V
	SensorManager::instance().testTrust = BatteryHealth::SocTrust::Trusted; // even if F1 says trusted

	// This is the exact regression AC-C6/Blocker B closes: previously,
	// cachedBatteryVoltage() returned false for this exact case and the
	// adapter silently fell through to the legacy raw-SoC path, letting an
	// implausible vcell reading resolve Healthy from a 99% raw SoC.
	const ReportingPolicy policy = ReportingPolicyResolver::resolveRuntime(99.0f, 43200);
	assert(policy.batteryTier == TIER_SURVIVAL);
}

void testInvalidVcellForcesSurvivalEvenAtLowRawSoc() {
	resetGlobals();
	SensorManager::instance().testVcellState = SensorManager::VcellSampleState::Invalid;
	SensorManager::instance().testVcell = 1.0f; // implausible, <= 2.5V

	const ReportingPolicy policy = ReportingPolicyResolver::resolveRuntime(50.0f, 43200);
	assert(policy.batteryTier == TIER_SURVIVAL);
}

// --- Unavailable: the one state that legitimately keeps legacy behavior. ---

void testUnavailableVcellKeepsLegacyRawSocBehavior() {
	resetGlobals();
	SensorManager::instance().testVcellState = SensorManager::VcellSampleState::Unavailable;

	// Before the first sample this boot (or on a platform that never
	// populates vcell), there is no basis yet to distrust the gauge SoC -
	// this intentionally matches legacy (pre-Amendment-B) behavior.
	const ReportingPolicy policy = ReportingPolicyResolver::resolveRuntime(90.0f, 43200);
	assert(policy.batteryTier == TIER_HEALTHY);
}

void testUnavailableVcellLowSocResolvesAccordingly() {
	resetGlobals();
	SensorManager::instance().testVcellState = SensorManager::VcellSampleState::Unavailable;

	const ReportingPolicy policy = ReportingPolicyResolver::resolveRuntime(20.0f, 43200);
	assert(policy.batteryTier == TIER_SURVIVAL);
}

// --- Unavailable + Untrusted: the missing case Codex Stage 7 Final found
// (AC-C6 totality gap). There is no vcell to substitute a resting estimate
// from (BatteryHealth::restingSocFromVcell() needs a real voltage), so an
// Untrusted SoC signal must not be allowed to drive cadence as though
// trusted just because vcell also happens to be unavailable. ---

void testUnavailableVcellUntrustedSocDoesNotResolveHealthy() {
	resetGlobals();
	SensorManager::instance().testVcellState = SensorManager::VcellSampleState::Unavailable;
	SensorManager::instance().testTrust = BatteryHealth::SocTrust::Untrusted;

	// This is the exact regression Codex Stage 7 Final found: raw SoC 99%
	// with Unavailable + Untrusted previously resolved Healthy.
	const ReportingPolicy policy = ReportingPolicyResolver::resolveRuntime(99.0f, 43200);
	assert(policy.batteryTier == TIER_SURVIVAL);
}

void testUnavailableVcellUntrustedSocForcesSurvivalEvenAtLowRawSoc() {
	resetGlobals();
	SensorManager::instance().testVcellState = SensorManager::VcellSampleState::Unavailable;
	SensorManager::instance().testTrust = BatteryHealth::SocTrust::Untrusted;

	const ReportingPolicy policy = ReportingPolicyResolver::resolveRuntime(20.0f, 43200);
	assert(policy.batteryTier == TIER_SURVIVAL);
}

} // namespace

int main() {
	testKnownTrustedHighSocResolvesHealthy();
	testKnownCriticalVcellFloorsToSurvivalEvenWithHighSoc();
	testKnownUntrustedSocIsSubstitutedByRestingEstimate();
	testInvalidVcellForcesSurvivalRegardlessOfRawSoc();
	testInvalidVcellForcesSurvivalEvenAtLowRawSoc();
	testUnavailableVcellKeepsLegacyRawSocBehavior();
	testUnavailableVcellLowSocResolvesAccordingly();
	testUnavailableVcellUntrustedSocDoesNotResolveHealthy();
	testUnavailableVcellUntrustedSocForcesSurvivalEvenAtLowRawSoc();

	printf("reporting_policy_adapter_test: all tests passed\n");
	return 0;
}
