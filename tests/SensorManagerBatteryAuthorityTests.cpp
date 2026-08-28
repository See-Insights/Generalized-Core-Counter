#include "sensors/BatteryAuthorityPolicy.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

namespace {

// NOTE: This file previously also exercised the STALE_SOC latch/resync
// machinery (staleSocConditionsMet, quietForFuelGaugeResync,
// shouldResyncFuelGauge, resolveSocCommit, ResyncActions, noteWakeCycle).
// That machinery was retired under WO-2026-08-25-001: F1 (BatteryHealth,
// see src/power/BatteryHealth.h/.cpp and tests/battery_health_test.cpp)
// replaces its purpose with a pure trust signal instead of an in-place
// gauge-correction latch. The tests below cover only the separate,
// still-active WO-2026-08-05-002 post-connect delta-guard and stuck-charging
// features, which are unaffected by that retirement.

void testSagArtifactDecreaseStillRejected() {
  assert(BatteryAuthorityPolicy::batterySampleHasUnrealisticDelta(78.0f, 52.0f));
  assert(BatteryAuthorityPolicy::batterySampleDelta(78.0f, 52.0f) == -26.0f);
}

void testPostConnectIncreaseWithVcellCorroborationAccepted() {
  assert(!BatteryAuthorityPolicy::batterySampleHasUnrealisticDelta(12.6f, 78.6f));
  assert(std::fabs(BatteryAuthorityPolicy::batterySampleDelta(12.6f, 78.6f) - 66.0f) < 0.001f);
  assert(!BatteryAuthorityPolicy::batterySampleHasUncorroboratedIncrease(
      12.6f, 78.6f, 4.028f));
}

void testPostConnectIncreaseWithoutVcellCorroborationRejected() {
  assert(!BatteryAuthorityPolicy::batterySampleHasUnrealisticDelta(12.0f, 78.0f));
  assert(BatteryAuthorityPolicy::batterySampleHasUncorroboratedIncrease(
      12.0f, 78.0f, 3.500f));
}

void testRadioOnCloudDisconnectedStillEvaluatesDelta() {
    assert(BatteryAuthorityPolicy::shouldEvaluatePostConnectDelta(
            false, true, false, true));
    assert(!BatteryAuthorityPolicy::shouldEvaluatePostConnectDelta(
            false, true, false, false));
    assert(!BatteryAuthorityPolicy::shouldEvaluatePostConnectDelta(
            true, true, false, true));
}

void testStuckChargingUsesAcceptedSocAndRawVcellProgress() {
    assert(!BatteryAuthorityPolicy::stuckChargingHasMeaningfulProgress(
            50.0f, 50.0f, 3.800f, 3.800f));
    assert(BatteryAuthorityPolicy::stuckChargingHasMeaningfulProgress(
            50.0f, 50.0f, 3.800f, 3.816f));
}

void testRecoveredIncidentSequence() {
    // Values are literal transcriptions of the recovered 2026-07-31 raw
    // serial log (see docs/work-orders/WO-2026-08-05-002-battery-authority-delta-guard.md).
    // No log line exists between 09:00 and 20:00 (11-hour low-power gap,
    // no ChargeDiag sample), so this fixture does not invent one. Only the
    // delta-guard assertions survive the STALE_SOC retirement; the
    // per-point staleSocConditionsMet expectations were removed along with
    // that machinery.
    assert(!BatteryAuthorityPolicy::batterySampleHasUnrealisticDelta(12.59f, 78.63f));
    assert(!BatteryAuthorityPolicy::batterySampleHasUncorroboratedIncrease(
            12.59f, 78.63f, 4.028f));
}

} // namespace

int main() {
  testSagArtifactDecreaseStillRejected();
  testPostConnectIncreaseWithVcellCorroborationAccepted();
  testPostConnectIncreaseWithoutVcellCorroborationRejected();
  testRadioOnCloudDisconnectedStillEvaluatesDelta();
  testStuckChargingUsesAcceptedSocAndRawVcellProgress();
  testRecoveredIncidentSequence();
  std::cout << "SensorManager battery authority tests passed\n";
  return 0;
}
