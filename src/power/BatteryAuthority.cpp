// WO-2026-09-21 Step 4 (corrected same day): this file is the QUERY half of
// BatteryAuthority - evaluate() only. It must stay host-compilable with
// zero stubbing, exactly like BatteryTierGuard/PowerTier/BatteryHealth
// already are, so it deliberately does NOT include Particle.h,
// MyPersistentData.h, or StorageHelperRK.h. The COMMAND half (commit(),
// currentTier(), clearLowBatteryMode() - all of which need real sysStatus
// access) lives in the sibling BatteryAuthorityCommand.cpp, in its own
// translation unit, specifically so linking this file alone (as
// tests/reporting_policy_adapter_test.sh does) never pulls in the
// persistence stack.
#include "power/BatteryAuthority.h"

#include "reporting/BatteryTierGuard.h" // socForTier()/applyVcellFloor() - the Known-vcell guard stage

namespace BatteryAuthority {

Verdict evaluate(float currentSoC, SensorManager::VcellSampleState vcellState,
                  float vcell, BatteryHealth::SocTrust trust, BatteryTier previousTier) {
  Verdict verdict;
  verdict.vcellState = vcellState;
  verdict.trust = trust;

  // The guarded pipeline - PowerTier::evaluate() (via BatteryTierGuard, the
  // stateless vcell/trust floor) and BatteryBackoff::calculateTier() (the
  // hysteresis stage) as two stages of one pipeline, not two independently
  // callable authorities. Ported verbatim from
  // ReportingPolicyResolver::resolveRuntime() (WO-2026-08-25-001 Amendment
  // C, Decision C2/AC-C6) - the guard is total over all three
  // SensorManager::VcellSampleState values, not a two-way collapse that lets
  // an implausible vcell bypass both the trust substitution and the 3.5V
  // floor.
  switch (vcellState) {
  case SensorManager::VcellSampleState::Known: {
    const float socForTier = BatteryTierGuard::socForTier(currentSoC, vcell, trust);
    verdict.tier = BatteryTierGuard::applyVcellFloor(
        BatteryBackoff::calculateTier(socForTier, previousTier),
        vcell, currentSoC, trust);
    break;
  }
  case SensorManager::VcellSampleState::Invalid:
    // A vcell sample WAS taken but is not physically plausible (<=2.5V,
    // >=5V, or NaN) - evidence the reading cannot be trusted at all, at
    // least as bad as a Known critical vcell. Force the same conservative
    // floor a Known vcell at/below PowerTier::kCriticalVcell would produce.
    verdict.tier = TIER_SURVIVAL;
    break;
  case SensorManager::VcellSampleState::Unavailable:
  default:
    // No vcell sample has been taken yet this boot. trust still needs
    // consulting - there is no vcell to substitute a resting estimate from,
    // so an untrusted SoC gets the same conservative floor Invalid uses.
    // When trust IS (still) Trusted - the pre-first-sample bootstrap
    // default - this intentionally matches legacy behavior (SoC-driven
    // tiering) until a real sample arrives.
    verdict.tier = (trust == BatteryHealth::SocTrust::Trusted)
        ? BatteryBackoff::calculateTier(currentSoC, previousTier)
        : TIER_SURVIVAL;
    break;
  }

  return verdict;
}

} // namespace BatteryAuthority
