#pragma once

#include "cloud/BatteryBackoffPolicy.h"
#include "power/BatteryHealth.h"
#include "sensors/SensorManager.h"

// Host-side stand-in for src/power/BatteryAuthority.h, scoped to exactly
// what src/reporting/RuntimeReportingPolicy.cpp calls.
//
// WO-2026-09-21 Step 4 (corrected same day): evaluate() itself is NOT
// stubbed here - it is declared exactly as production does (matching the
// real header byte-for-byte for this one declaration), and
// tests/reporting_policy_adapter_test.sh links the REAL
// src/power/BatteryAuthority.cpp alongside this stub, so the actual guard
// pipeline (BatteryTierGuard/PowerTier/BatteryBackoff, all real, all
// already linked by this test) is what's under test - not a hand-written
// mirror that could silently drift from production. This supersedes the
// prior version of this file, which duplicated the pipeline's
// switch/guard logic inline; that duplication is exactly what Step 4's
// evaluate()/commit() split (query vs command) was corrected to make
// unnecessary, since evaluate() is now pure and needs no persistence-backed
// stubbing at all.
//
// WO-2026-09-21 Step 4 (corrected same day, twice): resolveRuntime() now
// calls evaluateCurrent() (production: power/BatteryAuthorityCommand.cpp)
// instead of gathering vcell/trust/previousTier itself and calling
// evaluate() directly. evaluateCurrent() genuinely needs real
// SensorManager/sysStatus in production, so it cannot be linked here the
// way evaluate() is - it is faked inline instead, using the SAME stubbed
// SensorManager this override directory already provides and the same
// currentTier() test global below, then delegating to the REAL evaluate().
// This keeps the actual guard pipeline under test while faking only the
// "which global object do I read this from" glue, the same scope
// currentTier() alone used to cover.
namespace BatteryAuthority {

struct Verdict {
  BatteryTier tier;
  BatteryHealth::SocTrust trust;
  SensorManager::VcellSampleState vcellState;
};

Verdict evaluate(float currentSoC, SensorManager::VcellSampleState vcellState,
                  float vcell, BatteryHealth::SocTrust trust, BatteryTier previousTier);

inline uint8_t testPreviousBatteryTier = 0;

inline BatteryTier currentTier() {
  return testPreviousBatteryTier <= TIER_SURVIVAL
      ? static_cast<BatteryTier>(testPreviousBatteryTier)
      : TIER_HEALTHY;
}

inline Verdict evaluateCurrent(float currentSoC) {
  float vcell = 0.0f;
  const SensorManager::VcellSampleState vcellState =
      SensorManager::instance().cachedBatteryVoltageState(vcell);
  const BatteryHealth::SocTrust trust = SensorManager::instance().cachedSocTrust();
  return evaluate(currentSoC, vcellState, vcell, trust, currentTier());
}

} // namespace BatteryAuthority
