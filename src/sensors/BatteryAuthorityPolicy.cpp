#include "sensors/BatteryAuthorityPolicy.h"

namespace BatteryAuthorityPolicy {

namespace {

bool batterySocIsValid(float soc) {
  return (soc == soc) && soc >= 0.0f && soc <= 100.0f;
}

bool batteryVoltageLooksUsable(float vcell) {
  return (vcell == vcell) && vcell > 2.5f && vcell < 5.0f;
}

} // namespace

float batterySampleDelta(float authoritativeSoc, float soc) {
  return soc - authoritativeSoc;
}

bool batterySampleHasUnrealisticDelta(float authoritativeSoc, float soc) {
  return batterySampleDelta(authoritativeSoc, soc) <= -POST_CONNECT_DELTA_THRESHOLD;
}

bool batterySampleHasUncorroboratedIncrease(float authoritativeSoc, float soc, float vcell) {
  const bool largeIncrease =
      batterySampleDelta(authoritativeSoc, soc) >= POST_CONNECT_DELTA_THRESHOLD;
  const bool vcellCorroboratesIncrease = vcell >= 4.00f;
  return largeIncrease && !vcellCorroboratesIncrease;
}

bool shouldEvaluatePostConnectDelta(bool shouldStabilize,
                                    bool authoritativeSampleActive,
                                    bool cloudConnected,
                                    bool radioPoweredOn) {
  return !shouldStabilize &&
      authoritativeSampleActive &&
      (cloudConnected || radioPoweredOn);
}

bool stuckChargingHasMeaningfulProgress(float baselineSoc,
                                        float acceptedSoc,
                                        float baselineVcell,
                                        float vcell) {
  const float socGain = batterySocIsValid(baselineSoc) ? acceptedSoc - baselineSoc : 0.0f;
  const float vcellGain = batteryVoltageLooksUsable(baselineVcell) ? vcell - baselineVcell : 0.0f;
  return socGain >= 0.5f || vcellGain >= 0.015f;
}

} // namespace BatteryAuthorityPolicy