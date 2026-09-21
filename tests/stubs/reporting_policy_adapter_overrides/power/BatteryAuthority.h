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
// Only currentTier() - the narrow read seam onto the persisted previous
// tier, which evaluate() cannot read itself without depending on
// MyPersistentData.h - is faked, via a test-controllable global. Same
// pattern reporting/BatteryTierStore.h's stub used before that seam folded
// into this module.
namespace BatteryAuthority {

struct Verdict {
  BatteryTier tier;
  bool lowBatteryMode;
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

} // namespace BatteryAuthority
