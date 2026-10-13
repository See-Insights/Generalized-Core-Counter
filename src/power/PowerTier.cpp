#include "power/PowerTier.h"

namespace PowerTier {

namespace {

Tier tierFromSoc(float soc) {
  if (soc >= kFullSocThreshold) {
    return Tier::Full;
  }
  if (soc >= kReducedSocThreshold) {
    return Tier::Reduced;
  }
  if (soc >= kLowSocThreshold) {
    return Tier::Low;
  }
  return Tier::Critical;
}

} // namespace

Tier evaluate(float vcell, float soc, BatteryHealth::SocTrust trust) {
  if (vcell <= kCriticalVcell) {
    return Tier::Critical;
  }

  if (trust != BatteryHealth::SocTrust::Trusted) {
    return tierFromSoc(BatteryHealth::restingSocFromVcell(vcell));
  }

  return tierFromSoc(soc);
}

} // namespace PowerTier
