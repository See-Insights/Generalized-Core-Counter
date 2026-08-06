#include "sensors/BatteryAuthorityPolicy.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

namespace {

using BatteryAuthorityPolicy::ChargeStatus;
using BatteryAuthorityPolicy::StaleSocSample;

constexpr uint8_t chargeStatus(ChargeStatus status) {
  return static_cast<uint8_t>(status);
}

StaleSocSample sample(float soc, float vcell, ChargeStatus charge,
                      uint8_t vbusStatus = 2, bool powerGood = true,
                      uint8_t faultReg = 0) {
  return {soc, vcell, chargeStatus(charge), vbusStatus, powerGood, faultReg};
}

void testNewBandAcrossPmicStates() {
  assert(BatteryAuthorityPolicy::staleSocConditionsMet(
      sample(11.8f, 4.028f, ChargeStatus::Pre)));
  assert(BatteryAuthorityPolicy::staleSocConditionsMet(
      sample(11.8f, 4.028f, ChargeStatus::Fast)));
  assert(BatteryAuthorityPolicy::staleSocConditionsMet(
      sample(11.8f, 4.028f, ChargeStatus::Done)));
  assert(BatteryAuthorityPolicy::staleSocConditionsMet(
      sample(11.8f, 4.028f, ChargeStatus::NotCharging)));
  assert(BatteryAuthorityPolicy::staleSocConditionsMet(
      sample(11.8f, 4.028f, ChargeStatus::NotCharging, 0, false)));

  assert(!BatteryAuthorityPolicy::quietForFuelGaugeResync(
      false, chargeStatus(ChargeStatus::Pre)));
  assert(!BatteryAuthorityPolicy::quietForFuelGaugeResync(
      false, chargeStatus(ChargeStatus::Fast)));
  assert(BatteryAuthorityPolicy::quietForFuelGaugeResync(
      false, chargeStatus(ChargeStatus::Done)));
  assert(BatteryAuthorityPolicy::quietForFuelGaugeResync(
      false, chargeStatus(ChargeStatus::NotCharging)));
  assert(!BatteryAuthorityPolicy::quietForFuelGaugeResync(
      true, chargeStatus(ChargeStatus::Done)));
}

void testConsistentSampleLeftAlone() {
  assert(!BatteryAuthorityPolicy::staleSocConditionsMet(
      sample(82.0f, 4.028f, ChargeStatus::Done)));
}

void testLegacyTierKeepsChargingContextGate() {
  assert(!BatteryAuthorityPolicy::staleSocConditionsMet(
      sample(28.0f, 4.120f, ChargeStatus::NotCharging, 0, false)));
  assert(BatteryAuthorityPolicy::staleSocConditionsMet(
      sample(28.0f, 4.120f, ChargeStatus::Done, 2, true)));
  assert(BatteryAuthorityPolicy::staleSocConditionsMet(
      sample(11.8f, 4.028f, ChargeStatus::NotCharging, 0, false)));
}

void testNewBandBoundaries() {
  assert(!BatteryAuthorityPolicy::staleSocConditionsMet(
      sample(25.0f, 3.999f, ChargeStatus::NotCharging, 0, false)));
  assert(BatteryAuthorityPolicy::staleSocConditionsMet(
      sample(25.0f, 4.000f, ChargeStatus::NotCharging, 0, false)));
  assert(!BatteryAuthorityPolicy::staleSocConditionsMet(
      sample(25.1f, 4.000f, ChargeStatus::NotCharging, 0, false)));
}

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

void testDebounceRequiresTwoConsecutiveSamples() {
  const bool stale = BatteryAuthorityPolicy::staleSocConditionsMet(
      sample(11.8f, 4.028f, ChargeStatus::Done));
  const uint8_t cooldown = BatteryAuthorityPolicy::STALE_SOC_RESYNC_COOLDOWN_WAKE_CYCLES;

  assert(!BatteryAuthorityPolicy::shouldResyncFuelGauge(
      stale, 1, cooldown, false, chargeStatus(ChargeStatus::Done)));
  assert(BatteryAuthorityPolicy::shouldResyncFuelGauge(
      stale, 2, cooldown, false, chargeStatus(ChargeStatus::Done)));
  assert(!BatteryAuthorityPolicy::shouldResyncFuelGauge(
      false, 2, cooldown, false, chargeStatus(ChargeStatus::Done)));
}

void testCooldownRequiresThreeWakeCycles() {
  const bool stale = BatteryAuthorityPolicy::staleSocConditionsMet(
      sample(11.8f, 4.028f, ChargeStatus::Done));
  uint8_t wakeCycles = 0;

  assert(!BatteryAuthorityPolicy::shouldResyncFuelGauge(
      stale, 2, wakeCycles, false, chargeStatus(ChargeStatus::Done)));
  wakeCycles = BatteryAuthorityPolicy::noteWakeCycle(wakeCycles);
  assert(!BatteryAuthorityPolicy::shouldResyncFuelGauge(
      stale, 2, wakeCycles, false, chargeStatus(ChargeStatus::Done)));
  wakeCycles = BatteryAuthorityPolicy::noteWakeCycle(wakeCycles);
  assert(!BatteryAuthorityPolicy::shouldResyncFuelGauge(
      stale, 2, wakeCycles, false, chargeStatus(ChargeStatus::Done)));
  wakeCycles = BatteryAuthorityPolicy::noteWakeCycle(wakeCycles);
  assert(BatteryAuthorityPolicy::shouldResyncFuelGauge(
      stale, 2, wakeCycles, false, chargeStatus(ChargeStatus::Done)));
  assert(BatteryAuthorityPolicy::noteWakeCycle(0xFF) == 0xFF);
}

void testRecoveredIncidentSequence() {
    struct IncidentPoint {
        const char *time;
        StaleSocSample battery;
        bool staleSocExpected;
    };

    // Values are literal transcriptions of the recovered 2026-07-31 raw
    // serial log (see docs/work-orders/WO-2026-08-05-002-battery-authority-delta-guard.md).
    // No log line exists between 09:00 and 20:00 (11-hour low-power gap,
    // no ChargeDiag sample), so this fixture does not invent one.
    const IncidentPoint recoveredSequence[] = {
            {"08:00:19", sample(11.8f, 4.028f, ChargeStatus::Done, 2, true), true},
            {"20:00 pre-radio (kept by authority guard)", sample(12.59f, 4.028f, ChargeStatus::Done, 2, true), true},
            {"20:00:27 raw fuel-gauge value (rejected that cycle)", sample(78.63f, 4.028f, ChargeStatus::Done, 2, true), false},
            {"21:00 ChargeDiag (accepted next wake)", sample(78.6f, 4.028f, ChargeStatus::Done, 2, true), false},
            {"22:00 ChargeDiag", sample(78.6f, 4.026f, ChargeStatus::Done, 2, true), false},
  };

    for (const IncidentPoint &point : recoveredSequence) {
        (void)point.time;
        assert(BatteryAuthorityPolicy::staleSocConditionsMet(point.battery) ==
                     point.staleSocExpected);
    }
    assert(!BatteryAuthorityPolicy::batterySampleHasUnrealisticDelta(12.59f, 78.63f));
  assert(!BatteryAuthorityPolicy::batterySampleHasUncorroboratedIncrease(
            12.59f, 78.63f, 4.028f));
}

} // namespace

int main() {
  testNewBandAcrossPmicStates();
  testConsistentSampleLeftAlone();
  testLegacyTierKeepsChargingContextGate();
  testNewBandBoundaries();
  testSagArtifactDecreaseStillRejected();
  testPostConnectIncreaseWithVcellCorroborationAccepted();
  testPostConnectIncreaseWithoutVcellCorroborationRejected();
  testDebounceRequiresTwoConsecutiveSamples();
  testCooldownRequiresThreeWakeCycles();
  testRecoveredIncidentSequence();
  std::cout << "SensorManager battery authority tests passed\n";
  return 0;
}