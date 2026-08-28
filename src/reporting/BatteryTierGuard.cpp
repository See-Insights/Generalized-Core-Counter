#include "reporting/BatteryTierGuard.h"

namespace BatteryTierGuard {

float socForTier(float gaugeSoc, float vcell, BatteryHealth::SocTrust trust) {
  if (trust != BatteryHealth::SocTrust::Trusted) {
    return BatteryHealth::restingSocFromVcell(vcell);
  }
  return gaugeSoc;
}

BatteryTier applyVcellFloor(BatteryTier tier, float vcell, float gaugeSoc, BatteryHealth::SocTrust trust) {
  const PowerTier::Tier guardTier = PowerTier::evaluate(vcell, gaugeSoc, trust);
  if (guardTier == PowerTier::Tier::Critical && vcell <= PowerTier::kCriticalVcell) {
    return TIER_SURVIVAL;
  }
  return tier;
}

} // namespace BatteryTierGuard
