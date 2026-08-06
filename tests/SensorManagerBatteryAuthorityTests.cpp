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

struct FakeAuthorityActions : BatteryAuthorityPolicy::ResyncActions {
    float settledSoc = 0.0f;
    float settledVcell = 0.0f;
    float committedSoc = -1.0f;
    unsigned quickStartCalls = 0;
    unsigned settleCalls = 0;
    unsigned readCalls = 0;
    unsigned commitCalls = 0;

    void quickStart() override {
        quickStartCalls++;
    }

    void settle() override {
        settleCalls++;
    }

    void readSample(float &soc, float &vcell) override {
        readCalls++;
        soc = settledSoc;
        vcell = settledVcell;
    }

    void commitSoc(float soc) override {
        commitCalls++;
        committedSoc = soc;
    }
};

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

void testSingleInconsistentSampleVetoesWithoutResync() {
    FakeAuthorityActions actions;
    const BatteryAuthorityPolicy::SocCommitResolution result =
            BatteryAuthorityPolicy::resolveSocCommit(
                    sample(11.8f, 4.028f, ChargeStatus::Done),
                    false,
                    1,
                    BatteryAuthorityPolicy::STALE_SOC_RESYNC_COOLDOWN_WAKE_CYCLES,
                    false,
                    actions);

    assert(result.initialSampleStale);
    assert(!result.shouldCommit);
    assert(!result.resyncAttempted);
    assert(actions.commitCalls == 0);
    assert(actions.quickStartCalls == 0);
    assert(actions.settleCalls == 0);
    assert(actions.readCalls == 0);
}

void testSecondInconsistentSampleResyncsAndCommitsCorrection() {
    FakeAuthorityActions actions;
    actions.settledSoc = 78.63f;
    actions.settledVcell = 4.028f;
    const StaleSocSample staleSample = sample(11.8f, 4.028f, ChargeStatus::Done);
    const uint8_t cooldown = BatteryAuthorityPolicy::STALE_SOC_RESYNC_COOLDOWN_WAKE_CYCLES;

    const BatteryAuthorityPolicy::SocCommitResolution first =
            BatteryAuthorityPolicy::resolveSocCommit(
                    staleSample, false, 1, cooldown, false, actions);
    assert(!first.shouldCommit);
    assert(actions.commitCalls == 0);
    assert(actions.quickStartCalls == 0);

    const BatteryAuthorityPolicy::SocCommitResolution second =
            BatteryAuthorityPolicy::resolveSocCommit(
                    staleSample, false, 2, cooldown, false, actions);
    assert(second.resyncAttempted);
    assert(!second.settledSampleStale);
    assert(second.shouldCommit);
    assert(actions.quickStartCalls == 1);
    assert(actions.settleCalls == 1);
    assert(actions.readCalls == 1);
    assert(actions.commitCalls == 1);
    assert(std::fabs(actions.committedSoc - 78.63f) < 0.001f);
}

void testResyncNeverRepeatsWithinInvocation() {
    FakeAuthorityActions actions;
    actions.settledSoc = 11.8f;
    actions.settledVcell = 4.028f;
    const BatteryAuthorityPolicy::SocCommitResolution result =
            BatteryAuthorityPolicy::resolveSocCommit(
                    sample(11.8f, 4.028f, ChargeStatus::Done),
                    false,
                    2,
                    BatteryAuthorityPolicy::STALE_SOC_RESYNC_COOLDOWN_WAKE_CYCLES,
                    false,
                    actions);

    assert(result.resyncAttempted);
    assert(result.settledSampleStale);
    assert(!result.shouldCommit);
    assert(actions.quickStartCalls == 1);
    assert(actions.settleCalls == 1);
    assert(actions.readCalls == 1);
    assert(actions.commitCalls == 0);
}

void testConsistentSampleCommitsWithoutExtraWork() {
    FakeAuthorityActions actions;
    const BatteryAuthorityPolicy::SocCommitResolution result =
            BatteryAuthorityPolicy::resolveSocCommit(
                    sample(78.63f, 4.028f, ChargeStatus::Done),
                    false,
                    0,
                    0,
                    true,
                    actions);

    assert(!result.initialSampleStale);
    assert(result.shouldCommit);
    assert(actions.commitCalls == 1);
    assert(std::fabs(actions.committedSoc - 78.63f) < 0.001f);
    assert(actions.quickStartCalls == 0);
    assert(actions.settleCalls == 0);
    assert(actions.readCalls == 0);
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
    FakeAuthorityActions actions;
    const BatteryAuthorityPolicy::SocCommitResolution rejected =
            BatteryAuthorityPolicy::resolveSocCommit(
                    sample(80.0f, 3.800f, ChargeStatus::Fast),
                    true,
                    0,
                    0,
                    true,
                    actions);

    assert(!rejected.shouldCommit);
    assert(actions.commitCalls == 0);
    assert(!BatteryAuthorityPolicy::stuckChargingHasMeaningfulProgress(
            50.0f, 50.0f, 3.800f, 3.800f));
    assert(BatteryAuthorityPolicy::stuckChargingHasMeaningfulProgress(
            50.0f, 50.0f, 3.800f, 3.816f));
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
    testSingleInconsistentSampleVetoesWithoutResync();
    testSecondInconsistentSampleResyncsAndCommitsCorrection();
    testResyncNeverRepeatsWithinInvocation();
    testConsistentSampleCommitsWithoutExtraWork();
    testRadioOnCloudDisconnectedStillEvaluatesDelta();
    testStuckChargingUsesAcceptedSocAndRawVcellProgress();
  testRecoveredIncidentSequence();
  std::cout << "SensorManager battery authority tests passed\n";
  return 0;
}