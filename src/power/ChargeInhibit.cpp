#include "power/ChargeInhibit.h"

#include "power/PowerPlatform.h"

namespace ChargeInhibit {

ApplyResult apply(bool inhibited, PowerInputProfile activeProfile) {
  ApplyResult result;
  result.inhibited = inhibited;

#if HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM)
  result.supported = true;

  SystemPowerConfiguration conf = PowerPlatform::baseConfigurationForProfile(activeProfile);
  if (inhibited) {
    conf.feature(SystemPowerFeature::DISABLE_CHARGING);
  } else {
    conf.clearFeature(SystemPowerFeature::DISABLE_CHARGING);
  }

  // Retry once on EITHER failure mode - an immediate System.setPowerConfiguration()
  // failure, or a successful write that reads back wrong (WO-2026-08-25-001
  // Amendment B ALSO FIX: the prior version only retried the latter, silently
  // reporting verified=false forever after a single immediate write failure,
  // e.g. transient DCT/flash contention, with no second attempt). Same
  // "retry once, then report; do not silently continue" pattern as
  // PowerConfig::verifyApplied() in the WO.
  for (int attempt = 0; attempt < 2; attempt++) {
    result.systemResult = System.setPowerConfiguration(conf);
    if (result.systemResult == SYSTEM_ERROR_NONE) {
      const SystemPowerConfiguration readBack = System.getPowerConfiguration();
      result.configReadbackVerified =
          readBack.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING) == inhibited;
      if (result.configReadbackVerified) break;
    }
  }
#else
  (void)activeProfile;
#endif

  return result;
}

} // namespace ChargeInhibit
