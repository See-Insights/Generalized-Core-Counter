// WO-2026-09-21 Step 4 (corrected same day): this file is the COMMAND half
// of BatteryAuthority - commit(), currentTier(), and clearLowBatteryMode().
// Unlike the sibling BatteryAuthority.cpp (evaluate() only, pure, zero
// stubbing needed), everything here genuinely needs real sysStatus access
// and lives in its own translation unit specifically so linking
// BatteryAuthority.cpp alone (as tests/reporting_policy_adapter_test.sh
// does) never pulls this in.
//
// evaluateCurrent() also lives here, alongside the genuinely stateful
// functions - not because it writes anything (it doesn't; it's a query),
// but because it needs SensorManager, which evaluate()'s own translation
// unit must not depend on.
#include "Particle.h"
#include "power/BatteryAuthority.h"

#include "MyPersistentData.h"      // sysStatus (currentBatteryTier/lowBatteryMode/connectionMode/sensorMode)
#include "sensors/SensorManager.h" // evaluateCurrent()'s vcell/trust source

namespace BatteryAuthority {

namespace {

const char *tierName(BatteryTier tier) {
  static const char *kTierNames[] = {"HEALTHY", "CONSERVING", "CRITICAL", "SURVIVAL"};
  return (tier <= TIER_SURVIVAL) ? kTierNames[tier] : "UNKNOWN";
}

// The one and only call site for sysStatus.set_lowBatteryMode() outside
// MyPersistentData.cpp - both commit()'s own tier-driven decision and
// clearLowBatteryMode() (ConfigApply.cpp's operator-override case) commit
// through this single wrapper.
void commitLowBatteryMode(bool value) {
  sysStatus.set_lowBatteryMode(value);
}

} // namespace

BatteryTier currentTier() {
  const uint8_t tierValue = sysStatus.get_currentBatteryTier();
  return tierValue <= TIER_SURVIVAL ? static_cast<BatteryTier>(tierValue) : TIER_HEALTHY;
}

Verdict evaluateCurrent(float currentSoC) {
  float vcell = 0.0f;
  const SensorManager::VcellSampleState vcellState =
      SensorManager::instance().cachedBatteryVoltageState(vcell);
  const BatteryHealth::SocTrust trust = SensorManager::instance().cachedSocTrust();
  return evaluate(currentSoC, vcellState, vcell, trust, currentTier());
}

void clearLowBatteryMode() {
  commitLowBatteryMode(false);
}

void commit(const Verdict &verdict, float currentSoC) {
  const uint8_t prevTierValue = sysStatus.get_currentBatteryTier();
  const BatteryTier previousTier = prevTierValue <= TIER_SURVIVAL
      ? static_cast<BatteryTier>(prevTierValue)
      : TIER_HEALTHY;

  if (verdict.tier != prevTierValue) {
    Log.info("Battery tier transition: %s -> %s (SoC=%.1f%%)",
             tierName(previousTier), tierName(verdict.tier), (double)currentSoC);
    sysStatus.set_currentBatteryTier(static_cast<uint8_t>(verdict.tier));
  }

  // Sticky low-battery connection-mode downgrade/recovery, OCCUPANCY mode
  // only - ported verbatim from the retired applyBatteryAwareConnectionModePolicy().
  // Downgrades are sticky: the device will not auto-upgrade even if SoC
  // recovers, until the tier is confirmed HEALTHY again while still in the
  // downgraded (INTERMITTENT) mode. This is exactly why the decision lives
  // here, in command, and not in the pure evaluate(): it depends on the
  // CURRENT persisted connectionMode/sensorMode/lowBatteryMode, none of
  // which a pure function may read.
  if (sysStatus.get_sensorMode() == OCCUPANCY) {
    const ConnectionMode currentMode = static_cast<ConnectionMode>(sysStatus.get_connectionMode());
    const bool lowBatteryDowngradeActive = sysStatus.get_lowBatteryMode();

    if (currentMode == INTERMITTENT_KEEP_ALIVE && verdict.tier >= TIER_CONSERVING) {
      Log.info("Battery conservation: Disabling KEEP_ALIVE mode (tier=%s, SoC=%.1f%%) - switching to INTERMITTENT",
               tierName(verdict.tier), (double)currentSoC);
      sysStatus.set_connectionMode(INTERMITTENT);
      commitLowBatteryMode(true);
    } else if (currentMode == INTERMITTENT && lowBatteryDowngradeActive && verdict.tier == TIER_HEALTHY) {
      Log.info("Battery recovery: clearing lowBatteryMode (tier=HEALTHY, SoC=%.1f%%)",
               (double)currentSoC);
      Log.info("Battery recovery: restoring INTERMITTENT_KEEP_ALIVE (tier=HEALTHY, SoC=%.1f%%)",
               (double)currentSoC);
      sysStatus.set_connectionMode(INTERMITTENT_KEEP_ALIVE);
      commitLowBatteryMode(false);
    } else if (currentMode != INTERMITTENT && lowBatteryDowngradeActive) {
      Log.info("Battery recovery: clearing lowBatteryMode (tier=%s, SoC=%.1f%%)",
               tierName(verdict.tier), (double)currentSoC);
      commitLowBatteryMode(false);
    }
  } else if (sysStatus.get_lowBatteryMode()) {
    Log.info("Battery recovery: clearing lowBatteryMode (tier=%s, SoC=%.1f%%)",
             tierName(verdict.tier), (double)currentSoC);
    commitLowBatteryMode(false);
  }
}

} // namespace BatteryAuthority
