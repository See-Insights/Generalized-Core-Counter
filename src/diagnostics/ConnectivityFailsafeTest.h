#pragma once

#include "Particle.h"
#include "BuildProfile.h"
#include "state/State_Common.h"

namespace ConnectivityFailsafeTest {

#if CONNECTIVITY_FAILSAFE_TEST_MODE
uint32_t jitterSec(uint8_t stage);
void logBootDiagnostics();
void logDeferDisconnectedMode();
void logDeferUpdatePending();
void logDeferInvalidTime();
void logDeferNoLastConnection();
void logDeferLowBatteryHardStageSuppressed(uint8_t nextStage, BatteryTier tier);
#endif

} // namespace ConnectivityFailsafeTest
