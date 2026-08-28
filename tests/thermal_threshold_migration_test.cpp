// F2a stored/migrated thermal-threshold resolution (WO-2026-08-25-001, Round
// 4a). Exercises ChargeInhibitPolicy::resolveStoredThermalThresholds(), the
// exact function sysStatusData::resolveThermalThresholds()
// (src/MyPersistentData.cpp) calls to resolve the raw four floats read back
// from persistent storage. This is the read-time gate that closes the gap:
// a zero-padded/migrated record must never be honoured as a configured set,
// but a legitimately-configured 0.0f field (armLowC's own compiled default)
// must never be treated as "unconfigured" just because it is zero.
#include "power/ChargeInhibitPolicy.h"

#include <cassert>
#include <iostream>

namespace {

using ChargeInhibitPolicy::ThermalThresholds;
using ChargeInhibitPolicy::resolveStoredThermalThresholds;

// 1. A zero-padded/migrated record (all four fields 0.0f, exactly what
// StorageHelperRK::validate() produces when it zero-fills an appended
// structure extension for an already-provisioned device) must resolve to
// the complete compiled default set, not the zero-padded values.
void testZeroPaddedMigratedRecordFallsBackToDefaults() {
  ThermalThresholds zeroPadded;
  zeroPadded.armHighC = 0.0f;
  zeroPadded.armLowC = 0.0f;
  zeroPadded.releaseHighC = 0.0f;
  zeroPadded.releaseLowC = 0.0f;

  const ThermalThresholds resolved = resolveStoredThermalThresholds(zeroPadded);

  const ThermalThresholds compiledDefault; // 37.0f / 0.0f / 35.0f / 3.0f
  assert(resolved.armHighC == compiledDefault.armHighC);
  assert(resolved.armLowC == compiledDefault.armLowC);
  assert(resolved.releaseHighC == compiledDefault.releaseHighC);
  assert(resolved.releaseLowC == compiledDefault.releaseLowC);
  assert(resolved.armHighC == 37.0f);
  assert(resolved.armLowC == 0.0f);
  assert(resolved.releaseHighC == 35.0f);
  assert(resolved.releaseLowC == 3.0f);
}

// 2. A validly-configured, non-default stored set must be honoured
// unchanged - the fix must not clobber real per-device configuration.
void testValidNonDefaultStoredSetIsHonouredUnchanged() {
  ThermalThresholds configured;
  configured.armHighC = 40.0f;
  configured.armLowC = -5.0f;
  configured.releaseHighC = 38.0f;
  configured.releaseLowC = -2.0f;
  assert(ChargeInhibitPolicy::isValidThermalThresholds(configured));

  const ThermalThresholds resolved = resolveStoredThermalThresholds(configured);

  assert(resolved.armHighC == 40.0f);
  assert(resolved.armLowC == -5.0f);
  assert(resolved.releaseHighC == 38.0f);
  assert(resolved.releaseLowC == -2.0f);
}

// 3. armLowC == 0.0f within an OTHERWISE VALID set must be preserved, not
// treated as "unconfigured". Regression guard for the trap in the brief:
// armLowC's own compiled default IS 0.0f, so a per-field zero check would
// wrongly "fix" this case. armHighC/releaseHighC/releaseLowC are chosen to
// differ from the compiled defaults so any accidental full-set fallback
// (e.g. from a latent per-field check) is visible in the assertions below.
void testArmLowZeroWithinValidSetIsPreserved() {
  ThermalThresholds configured;
  configured.armHighC = 40.0f;     // differs from compiled default 37.0f
  configured.armLowC = 0.0f;       // same as compiled default, but CONFIGURED here
  configured.releaseHighC = 38.0f; // differs from compiled default 35.0f
  configured.releaseLowC = 2.0f;   // differs from compiled default 3.0f
  assert(ChargeInhibitPolicy::isValidThermalThresholds(configured));

  const ThermalThresholds resolved = resolveStoredThermalThresholds(configured);

  assert(resolved.armLowC == 0.0f);
  // If armLowC == 0.0f had incorrectly triggered ANY fallback, armHighC
  // would come back 37.0f (the compiled default) instead of 40.0f.
  assert(resolved.armHighC == 40.0f);
  assert(resolved.releaseHighC == 38.0f);
  assert(resolved.releaseLowC == 2.0f);
}

// 4. A partially-invalid stored set (inverted hysteresis on one side only)
// must fall back to the COMPLETE default set, never a mixture of stored and
// default fields.
void testPartiallyInvalidSetFallsBackToCompleteDefaultSet() {
  ThermalThresholds invertedHighSide;
  invertedHighSide.armHighC = 30.0f;     // valid-looking on its own
  invertedHighSide.armLowC = -5.0f;      // valid-looking on its own
  invertedHighSide.releaseHighC = 35.0f; // armHighC(30) > releaseHighC(35) is FALSE: inverted
  invertedHighSide.releaseLowC = -2.0f;
  assert(!ChargeInhibitPolicy::isValidThermalThresholds(invertedHighSide));

  const ThermalThresholds resolved = resolveStoredThermalThresholds(invertedHighSide);

  const ThermalThresholds compiledDefault;
  assert(resolved.armHighC == compiledDefault.armHighC);
  assert(resolved.armLowC == compiledDefault.armLowC);
  assert(resolved.releaseHighC == compiledDefault.releaseHighC);
  assert(resolved.releaseLowC == compiledDefault.releaseLowC);
  // In particular, armLowC/releaseLowC (individually "valid-looking") must
  // NOT survive into the resolved set alongside defaulted high-side fields.
  assert(resolved.armLowC != -5.0f);
  assert(resolved.releaseLowC != -2.0f);
}

} // namespace

int main() {
  testZeroPaddedMigratedRecordFallsBackToDefaults();
  testValidNonDefaultStoredSetIsHonouredUnchanged();
  testArmLowZeroWithinValidSetIsPreserved();
  testPartiallyInvalidSetFallsBackToCompleteDefaultSet();
  std::cout << "F2a stored/migrated thermal threshold resolution tests passed\n";
  return 0;
}
