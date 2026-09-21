#pragma once

#include <stdint.h>

namespace BatteryAuthorityPolicy {

constexpr float POST_CONNECT_DELTA_THRESHOLD = 20.0f;

float batterySampleDelta(float authoritativeSoc, float soc);
bool batterySampleHasUnrealisticDelta(float authoritativeSoc, float soc);
bool batterySampleHasUncorroboratedIncrease(float authoritativeSoc, float soc, float vcell);
bool shouldEvaluatePostConnectDelta(bool shouldStabilize,
                                    bool authoritativeSampleActive,
                                    bool cloudConnected,
                                    bool radioPoweredOn);
bool stuckChargingHasMeaningfulProgress(float baselineSoc,
                                        float acceptedSoc,
                                        float baselineVcell,
                                        float vcell);

} // namespace BatteryAuthorityPolicy