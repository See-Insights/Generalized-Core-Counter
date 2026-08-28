#include "reporting/BatteryTierGuard.h"

#include <cassert>
#include <iostream>

using BatteryHealth::SocTrust;

namespace {

// AC-B2: an untrusted gauge SoC must not drive cadence. This directly
// reproduces the Dev-14 defect shape (0.4% -> 97.8%): with trust lost, the
// wildly-wrong gauge SoC must have zero effect on the tier fed to
// BatteryBackoff::calculateTier() - only vcell (via
// BatteryHealth::restingSocFromVcell) may.
void testUntrustedSocDoesNotDriveCadence() {
  const float vcell = 3.9f; // resting-OCV ~ high SoC region
  const float restingEstimate = BatteryHealth::restingSocFromVcell(vcell);

  const float socWithBadGauge = BatteryTierGuard::socForTier(0.4f, vcell, SocTrust::Untrusted);
  const float socWithGoodGauge = BatteryTierGuard::socForTier(97.8f, vcell, SocTrust::Untrusted);
  assert(socWithBadGauge == socWithGoodGauge);
  assert(socWithBadGauge == restingEstimate);

  const BatteryTier tierWithBadGauge =
      BatteryBackoff::calculateTier(socWithBadGauge, TIER_SURVIVAL);
  const BatteryTier tierWithGoodGauge =
      BatteryBackoff::calculateTier(socWithGoodGauge, TIER_SURVIVAL);
  assert(tierWithBadGauge == tierWithGoodGauge);

  // Trusted input is untouched - the gauge value passes straight through.
  assert(BatteryTierGuard::socForTier(42.0f, vcell, SocTrust::Trusted) == 42.0f);
}

// AC-B3: vcell at/below PowerTier::kCriticalVcell (3.5V) must clamp the tier
// to at least TIER_SURVIVAL regardless of soc or trust - even a high,
// trusted soc during a real brownout must not report "healthy".
void testVcellFloorClampsToSurvivalRegardlessOfSocOrTrust() {
  const BatteryTier startingTier =
      BatteryBackoff::calculateTier(95.0f, TIER_HEALTHY); // would resolve HEALTHY on its own

  assert(BatteryTierGuard::applyVcellFloor(startingTier, 3.5f, 95.0f, SocTrust::Trusted) == TIER_SURVIVAL);
  assert(BatteryTierGuard::applyVcellFloor(startingTier, 3.4f, 95.0f, SocTrust::Trusted) == TIER_SURVIVAL);
  assert(BatteryTierGuard::applyVcellFloor(startingTier, 3.5f, 95.0f, SocTrust::Untrusted) == TIER_SURVIVAL);

  // Above the floor threshold, the already-resolved tier passes through
  // unchanged (BatteryBackoff's hysteresis stays authoritative).
  assert(BatteryTierGuard::applyVcellFloor(startingTier, 3.51f, 95.0f, SocTrust::Trusted) == startingTier);
}

void testFloorDoesNotOverrideAlreadyWorseTier() {
  // A tier already at/worse than SURVIVAL is unaffected either way.
  assert(BatteryTierGuard::applyVcellFloor(TIER_SURVIVAL, 3.9f, 95.0f, SocTrust::Trusted) == TIER_SURVIVAL);
}

} // namespace

int main() {
  testUntrustedSocDoesNotDriveCadence();
  testVcellFloorClampsToSurvivalRegardlessOfSocOrTrust();
  testFloorDoesNotOverrideAlreadyWorseTier();
  std::cout << "BatteryTierGuard (RuntimeReportingPolicy guard/floor) tests passed\n";
  return 0;
}
