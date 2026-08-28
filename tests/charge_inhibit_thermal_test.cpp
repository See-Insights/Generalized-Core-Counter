#include "power/ChargeInhibitPolicy.h"

#include <cassert>
#include <iostream>

namespace {

using ChargeInhibitPolicy::ThermalThresholds;
using ChargeInhibitPolicy::evaluateThermal;

void testDefaultsMatchWorkOrder() {
  const ThermalThresholds t;
  assert(t.armHighC == 37.0f);
  assert(t.armLowC == 0.0f);
  assert(t.releaseHighC == 35.0f);
  assert(t.releaseLowC == 3.0f);
}

void testArmsOnHeatFromNotInhibited() {
  assert(!evaluateThermal(false, 36.9f));
  assert(evaluateThermal(false, 37.1f));  // strictly above arm ceiling
  assert(!evaluateThermal(false, 37.0f)); // at threshold, not yet armed
}

void testArmsOnColdFromNotInhibited() {
  assert(!evaluateThermal(false, 0.1f));
  assert(evaluateThermal(false, -0.1f));
  assert(!evaluateThermal(false, 0.0f));
}

void testStaysInhibitedUntilFullyInsideReleaseBand() {
  // Was inhibited for heat; stays inhibited above the tighter release ceiling.
  assert(evaluateThermal(true, 36.0f));
  assert(evaluateThermal(true, 35.1f));
  assert(!evaluateThermal(true, 35.0f)); // at/below release ceiling AND above release floor
}

void testStaysInhibitedUntilFullyInsideReleaseBandCold() {
  assert(evaluateThermal(true, 2.9f));
  assert(!evaluateThermal(true, 3.0f));
}

void testNoChatterInHysteresisGap() {
  // Between release (35) and arm (37), a device that is already inhibited
  // must stay inhibited (no chatter); a device that is not inhibited must
  // not newly arm.
  for (float t = 35.1f; t < 37.0f; t += 0.3f) {
    assert(evaluateThermal(true, t));
    assert(!evaluateThermal(false, t));
  }
  for (float t = 0.1f; t < 3.0f; t += 0.3f) {
    assert(evaluateThermal(true, t));
    assert(!evaluateThermal(false, t));
  }
}

void testSelfClearingSequence() {
  // Simulates "every measurement" re-assertion across a heat event.
  bool inhibited = false;
  const float sequenceC[] = {30.0f, 38.0f, 40.0f, 36.0f, 34.0f, 30.0f};
  const bool expected[] = {false, true, true, true, false, false};
  for (size_t i = 0; i < sizeof(sequenceC) / sizeof(sequenceC[0]); i++) {
    inhibited = evaluateThermal(inhibited, sequenceC[i]);
    assert(inhibited == expected[i]);
  }
}

// ---------------------------------------------------------------------------
// AC-B5 (WO-2026-08-25-001 Amendment B): validity-gated arm/hold/release.
// ---------------------------------------------------------------------------

using ChargeInhibitPolicy::evaluateThermalWithValidity;
using ChargeInhibitPolicy::ThermalInhibitDecision;

void testUnmeasuredTempCannotArm() {
  // Not currently inhibited, no fresh reading yet this boot, and a stale
  // value that WOULD arm if trusted (38C > 37C arm-high) - must NOT arm.
  const ThermalInhibitDecision decision =
      evaluateThermalWithValidity(false, /*temperatureMeasuredThisBoot=*/false, 38.0f);
  assert(!decision.inhibited);
  assert(!decision.heldWithoutFreshTemp);
}

void testUnmeasuredTempHoldsAnAlreadyArmedInhibitButFlagsIt() {
  // Already inhibited (e.g. synced from a DCT flag left set by a prior
  // boot), no fresh reading yet - stays held, but visibly flagged rather
  // than silently continuing unobserved.
  const ThermalInhibitDecision decision =
      evaluateThermalWithValidity(true, /*temperatureMeasuredThisBoot=*/false, 20.0f);
  assert(decision.inhibited);
  assert(decision.heldWithoutFreshTemp);
}

void testFreshMeasurementCanArmAndCanRelease() {
  // A genuine this-boot reading is free to do either, exactly like
  // evaluateThermal() alone, with heldWithoutFreshTemp always false.
  const ThermalInhibitDecision armed =
      evaluateThermalWithValidity(false, /*temperatureMeasuredThisBoot=*/true, 38.0f);
  assert(armed.inhibited);
  assert(!armed.heldWithoutFreshTemp);

  const ThermalInhibitDecision released =
      evaluateThermalWithValidity(true, /*temperatureMeasuredThisBoot=*/true, 20.0f);
  assert(!released.inhibited);
  assert(!released.heldWithoutFreshTemp);
}

// Reproduces the exact reset/hibernate field scenario Codex Stage 7 found:
// a hot brownout leaves internalTempC persisted at 38C and DCT charging
// disabled; the device reboots, and a closed-hours hibernate keeps
// interrupting the TMP36 8-sample average before it completes, so
// temperatureMeasuredThisBoot never becomes true across several "boots".
// The device is physically cool (as a fresh reading would show, once one
// completes) but must not re-arm from the stale 38C on any of the
// interrupted boots, and must not spuriously look "released" either - it
// should visibly HOLD until a real reading lands.
void testResetHibernateScenarioNeverArmsFromStaleTempAndEventuallyReleasesOnFreshReading() {
  const float staleHotTempC = 38.0f; // persisted from the pre-brownout state
  bool inhibited = true;              // DCT was left disabled by the prior boot

  for (int simulatedBoot = 0; simulatedBoot < 5; simulatedBoot++) {
    // Each "boot" here models isItSafeToCharge() being called repeatedly
    // before hibernate interrupts sampling again - temperatureMeasuredThisBoot
    // stays false the whole time, as it would if sampleIndex never reaches 8.
    const ThermalInhibitDecision decision =
        evaluateThermalWithValidity(inhibited, /*temperatureMeasuredThisBoot=*/false, staleHotTempC);
    assert(decision.inhibited);            // stays held...
    assert(decision.heldWithoutFreshTemp); // ...but visibly, not silently
    inhibited = decision.inhibited;
  }

  // Eventually a real reading completes (device actually is cool, e.g. 20C).
  const ThermalInhibitDecision finalDecision =
      evaluateThermalWithValidity(inhibited, /*temperatureMeasuredThisBoot=*/true, 20.0f);
  assert(!finalDecision.inhibited);
  assert(!finalDecision.heldWithoutFreshTemp);
}

// ---------------------------------------------------------------------------
// AC-B6: threshold-ordering and 45C ceiling validation.
// ---------------------------------------------------------------------------

using ChargeInhibitPolicy::isValidThermalThresholds;
using ChargeInhibitPolicy::kCellChargeMaxC;

void testDefaultThresholdsAreValid() {
  assert(isValidThermalThresholds(ThermalThresholds()));
}

void testInvertedHighSideHysteresisIsRejected() {
  ThermalThresholds t;
  t.armHighC = 34.0f;    // at/below releaseHighC (35.0) - inverted
  t.releaseHighC = 35.0f;
  assert(!isValidThermalThresholds(t));
}

void testInvertedLowSideHysteresisIsRejected() {
  ThermalThresholds t;
  t.armLowC = 5.0f;      // at/above releaseLowC (3.0) - inverted
  t.releaseLowC = 3.0f;
  assert(!isValidThermalThresholds(t));
}

void testArmHighAboveCellChargeMaxIsRejected() {
  ThermalThresholds t;
  t.armHighC = kCellChargeMaxC + 0.1f;
  t.releaseHighC = 35.0f;
  assert(!isValidThermalThresholds(t));

  ThermalThresholds atCeiling;
  atCeiling.armHighC = kCellChargeMaxC;
  atCeiling.releaseHighC = 35.0f;
  assert(isValidThermalThresholds(atCeiling));
}

} // namespace

int main() {
  testDefaultsMatchWorkOrder();
  testArmsOnHeatFromNotInhibited();
  testArmsOnColdFromNotInhibited();
  testStaysInhibitedUntilFullyInsideReleaseBand();
  testStaysInhibitedUntilFullyInsideReleaseBandCold();
  testNoChatterInHysteresisGap();
  testSelfClearingSequence();
  testUnmeasuredTempCannotArm();
  testUnmeasuredTempHoldsAnAlreadyArmedInhibitButFlagsIt();
  testFreshMeasurementCanArmAndCanRelease();
  testResetHibernateScenarioNeverArmsFromStaleTempAndEventuallyReleasesOnFreshReading();
  testDefaultThresholdsAreValid();
  testInvertedHighSideHysteresisIsRejected();
  testInvertedLowSideHysteresisIsRejected();
  testArmHighAboveCellChargeMaxIsRejected();
  std::cout << "ChargeInhibit (F2a) thermal hysteresis tests passed\n";
  return 0;
}
