#include "power/PowerTier.h"

#include "cloud/BatteryBackoffPolicy.h"

#include <cassert>
#include <iostream>

using BatteryHealth::SocTrust;
using PowerTier::Tier;

namespace {

void testCriticalVcellOverridesEverythingRegardlessOfSoc() {
  // Even a high, trusted soc must not mask a real brownout.
  assert(PowerTier::evaluate(3.5f, 90.0f, SocTrust::Trusted) == Tier::Critical);
  assert(PowerTier::evaluate(3.4f, 90.0f, SocTrust::Trusted) == Tier::Critical);
  assert(PowerTier::evaluate(3.5f, 90.0f, SocTrust::Untrusted) == Tier::Critical);
  assert(PowerTier::evaluate(3.51f, 90.0f, SocTrust::Trusted) != Tier::Critical);
}

void testTrustedUsesSocBreakpoints() {
  assert(PowerTier::evaluate(3.9f, 80.0f, SocTrust::Trusted) == Tier::Full);
  assert(PowerTier::evaluate(3.9f, 75.0f, SocTrust::Trusted) == Tier::Full);
  assert(PowerTier::evaluate(3.9f, 60.0f, SocTrust::Trusted) == Tier::Reduced);
  assert(PowerTier::evaluate(3.9f, 40.0f, SocTrust::Trusted) == Tier::Low);
  assert(PowerTier::evaluate(3.9f, 20.0f, SocTrust::Trusted) == Tier::Critical);
}

void testUntrustedIgnoresSocAndUsesVcellOnly() {
  // A wildly wrong soc must have zero effect once trust is lost - the whole
  // point of F1/F3's design is "stop depending on soc", not "average it in".
  const Tier withBadSoc = PowerTier::evaluate(3.9f, 5.0f, SocTrust::Untrusted);
  const Tier withGoodSoc = PowerTier::evaluate(3.9f, 95.0f, SocTrust::Untrusted);
  assert(withBadSoc == withGoodSoc);

  const Tier suspectBadSoc = PowerTier::evaluate(3.9f, 5.0f, SocTrust::Suspect);
  const Tier suspectGoodSoc = PowerTier::evaluate(3.9f, 95.0f, SocTrust::Suspect);
  assert(suspectBadSoc == suspectGoodSoc);
}

void testPurityAndDeterminism() {
  for (int i = 0; i < 5; i++) {
    assert(PowerTier::evaluate(3.7f, 50.0f, SocTrust::Trusted) == Tier::Low);
  }
}

// AC-B1 (WO-2026-08-25-001 Amendment B): PowerTier's SoC breakpoints must not
// independently drift from BatteryBackoff::calculateTier's - the WO
// explicitly requires them "derived from, or asserted equal by test to,
// BatteryBackoff's" rather than existing as two free-standing restatements
// of 75/55/35. Checked from the lowest previous tier (TIER_SURVIVAL) so
// BatteryBackoff's hysteresis re-entry bands (70/50/30) can't shift the
// answer at the exact entry breakpoints under test.
void testBreakpointsMatchBatteryBackoff() {
  assert(PowerTier::kFullSocThreshold == 75.0f);
  assert(PowerTier::kReducedSocThreshold == 55.0f);
  assert(PowerTier::kLowSocThreshold == 35.0f);

  assert(BatteryBackoff::calculateTier(PowerTier::kFullSocThreshold, TIER_SURVIVAL) == TIER_HEALTHY);
  assert(BatteryBackoff::calculateTier(PowerTier::kFullSocThreshold - 0.1f, TIER_SURVIVAL) != TIER_HEALTHY);
  assert(BatteryBackoff::calculateTier(PowerTier::kReducedSocThreshold, TIER_SURVIVAL) == TIER_CONSERVING);
  assert(BatteryBackoff::calculateTier(PowerTier::kReducedSocThreshold - 0.1f, TIER_SURVIVAL) != TIER_CONSERVING);
  assert(BatteryBackoff::calculateTier(PowerTier::kLowSocThreshold, TIER_SURVIVAL) == TIER_CRITICAL);
  assert(BatteryBackoff::calculateTier(PowerTier::kLowSocThreshold - 0.1f, TIER_SURVIVAL) != TIER_CRITICAL);
}

} // namespace

int main() {
  testCriticalVcellOverridesEverythingRegardlessOfSoc();
  testTrustedUsesSocBreakpoints();
  testUntrustedIgnoresSocAndUsesVcellOnly();
  testPurityAndDeterminism();
  testBreakpointsMatchBatteryBackoff();
  std::cout << "PowerTier (F3) tests passed\n";
  return 0;
}
