/**
 * WO-2026-08-25-001 Decision C5 host test (AC-C12/AC-C13).
 *
 * Exercises PowerPlatform::applyInputProfile() (Fix 2) together with
 * ChargeInhibit::apply() against the same in-process stub
 * System.getPowerConfiguration()/setPowerConfiguration() state, so
 * composition (not replacement) of the DISABLE_CHARGING bit can actually be
 * observed across a profile transition.
 *
 * WHAT THIS PROVES: that applyInputProfile() preserves a previously-set
 * DISABLE_CHARGING bit across both USB->Solar and Solar->USB transitions
 * (AC-C12), and that this preservation does not create a latch - a
 * subsequent genuine cool ChargeInhibit::apply(false, ...) call still clears
 * the bit (AC-C13).
 *
 * WHAT THIS DOES NOT PROVE: System.getPowerConfiguration() here is an
 * in-process stub returning exactly what was last written by
 * setPowerConfiguration() in this same process - it cannot model a failed
 * DCT read being sanitized to defaults, nor Device OS's separate
 * power-manager worker thread reloading a profile-transition config and
 * calling enableCharging() before this process's next coupled call runs.
 * That asynchronous race (AC-C14) is explicitly out of reach for a host
 * test and remains an on-device validation item.
 */

#include "power/PowerPlatform.h"
#include "power/ChargeInhibit.h"
#include "MyPersistentData.h"

#include <cassert>
#include <iostream>

TestCurrentStatus testCurrent;
TestSystemStatus testSysStatus;

namespace {

void resetSystemState() {
  System.setPowerConfiguration(SystemPowerConfiguration());
}

// AC-C12: a profile transition while DISABLE_CHARGING is set retains the
// bit, in both directions (USB->Solar and Solar->USB).
void testProfileTransitionPreservesDisableChargingBit() {
  resetSystemState();

  // Arm the thermal inhibit (as ChargeInhibit::apply(true, ...) would on a
  // hot reading) while the active profile is UsbBench.
  ChargeInhibit::apply(true, PowerInputProfile::UsbBench);
  assert(System.getPowerConfiguration().isFeatureSet(SystemPowerFeature::DISABLE_CHARGING));

  // USB -> Solar transition.
  const auto solarResult = PowerPlatform::applyInputProfile(PowerInputProfile::Solar35W);
  assert(solarResult.applied);
  assert(System.getPowerConfiguration().isFeatureSet(SystemPowerFeature::DISABLE_CHARGING));
  // The profile's own limits must still have been applied - composition,
  // not a no-op.
  assert(System.getPowerConfiguration().powerSourceMinVoltage_ == 5080);

  // Solar -> USB transition.
  const auto usbResult = PowerPlatform::applyInputProfile(PowerInputProfile::UsbBench);
  assert(usbResult.applied);
  assert(System.getPowerConfiguration().isFeatureSet(SystemPowerFeature::DISABLE_CHARGING));
  assert(System.getPowerConfiguration().powerSourceMinVoltage_ == 3880);

  std::cout << "PASS: testProfileTransitionPreservesDisableChargingBit\n";
}

// AC-C13: preservation must not create a latch - a subsequent genuine cool
// coupled decision (ChargeInhibit::apply(false, ...)) still clears the bit.
void testSubsequentCoolDecisionStillClearsBit() {
  resetSystemState();

  ChargeInhibit::apply(true, PowerInputProfile::UsbBench);
  assert(System.getPowerConfiguration().isFeatureSet(SystemPowerFeature::DISABLE_CHARGING));

  PowerPlatform::applyInputProfile(PowerInputProfile::Solar35W);
  assert(System.getPowerConfiguration().isFeatureSet(SystemPowerFeature::DISABLE_CHARGING));

  // Enclosure cools; the coupled decision now says charging is safe again.
  const auto releaseResult = ChargeInhibit::apply(false, PowerInputProfile::Solar35W);
  assert(releaseResult.configReadbackVerified);
  assert(!System.getPowerConfiguration().isFeatureSet(SystemPowerFeature::DISABLE_CHARGING));

  // And a further profile transition with the bit clear must not
  // resurrect it.
  PowerPlatform::applyInputProfile(PowerInputProfile::UsbBench);
  assert(!System.getPowerConfiguration().isFeatureSet(SystemPowerFeature::DISABLE_CHARGING));

  std::cout << "PASS: testSubsequentCoolDecisionStillClearsBit\n";
}

} // namespace

int main() {
  testProfileTransitionPreservesDisableChargingBit();
  testSubsequentCoolDecisionStillClearsBit();
  std::cout << "All power_composition_test cases passed.\n";
  return 0;
}
