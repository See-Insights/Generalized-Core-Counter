#pragma once

/**
 * @file BatteryAuthority.h
 * @brief Single owner of the battery tier and low-battery-mode decision,
 *        split into a pure query and a stateful command
 *        (WO-2026-09-21-001, Step 4 of the Structural Ownership Map
 *        roadmap; corrected the same day - see the file-level comments on
 *        BatteryAuthority.cpp and BatteryAuthorityCommand.cpp for why the
 *        split exists).
 *
 * @details Before this step, the tier decision built on top of
 *          state-of-charge (itself already well-concentrated in
 *          `SensorManager::batteryState()`) had four competing answerers:
 *          `ReportingPolicyResolver::resolveRuntime()` (the properly guarded
 *          production path), `Cloud::calculateBatteryTier()` (unguarded - no
 *          vcell floor, no trust check - yet reachable from the
 *          connectivity failsafe's low-battery block),
 *          `applyBatteryAwareConnectionModePolicy()` (the persisted
 *          tier/low-battery-mode writer, plus the connection-mode downgrade/
 *          recovery side effect), and `Cloud::testBatteryBackoffLogic()`
 *          (dead code that used the persisted tier field as scratch space).
 *
 *          `PowerTier::evaluate()` and `BatteryBackoff::calculateTier()`
 *          are UNCHANGED - two pure, independently-tested modules, now
 *          consumed as two stages of ONE pipeline (via the unchanged
 *          `BatteryTierGuard`) from exactly one place instead of being
 *          reachable as two separately-callable authorities.
 *
 *          `evaluate()` is query: pure, no persisted reads or writes, no
 *          connection-mode side effect, depends only on the pipeline stages
 *          and its own parameters - host-compilable with zero stubbing,
 *          same as `BatteryTierGuard`/`PowerTier`/`BatteryHealth` already
 *          are. `commit()` is command: the only function that writes
 *          `set_currentBatteryTier`/`set_lowBatteryMode` and applies the
 *          connection-mode side effect, given a `Verdict` `evaluate()`
 *          already computed. `currentTier()` is the narrow, still-shadowable
 *          read seam onto the persisted previous tier - the input
 *          `evaluate()` needs for hysteresis but cannot read itself without
 *          depending on `MyPersistentData.h`. Read paths (the connectivity
 *          failsafe, the reporting-policy adapter, the device-status
 *          publisher) call only `evaluate()` - never `commit()` - so merely
 *          checking the current tier never has a persisting side effect.
 *          Commit happens only at the deliberate policy-application sites
 *          (`setup()`, the two `State_Sleep.cpp` low-battery-recovery
 *          checks, and `State_Report.cpp`'s reporting cycle).
 */

#include "cloud/BatteryBackoffPolicy.h" // BatteryTier
#include "power/BatteryHealth.h"        // BatteryHealth::SocTrust
#include "sensors/SensorManager.h"      // SensorManager::VcellSampleState (type only - evaluate() does not call SensorManager)

namespace BatteryAuthority {

/**
 * @brief Result of one battery-tier evaluation.
 *
 * @details Deliberately has no lowBatteryMode field. The real low-battery
 *          mode is sticky and connection-mode-aware, computed only inside
 *          commit() (it needs sysStatus's current connectionMode/sensorMode/
 *          lowBatteryMode, none of which a pure evaluate() may read) - it
 *          can disagree with tier alone (the sticky flag stays set after
 *          the tier recovers). A field here that didn't equal the device's
 *          actual low-battery mode would be a trap for the next reader; a
 *          caller that needs the real persisted flag should read
 *          PowerConfig::get_lowBatteryMode() directly.
 */
struct Verdict {
  BatteryTier tier;
  BatteryHealth::SocTrust trust;
  SensorManager::VcellSampleState vcellState;
};

/**
 * @brief The pure battery-tier evaluation. Query, not command.
 *
 * @param currentSoC Current fuel-gauge state of charge (0-100).
 * @param vcellState Current cell-voltage sample state (Known/Invalid/Unavailable).
 * @param vcell Measured cell voltage - meaningful only when vcellState == Known.
 * @param trust F1's SocTrust signal for this sample.
 * @param previousTier The persisted tier from the last commit() (see
 *        currentTier()) - BatteryBackoff::calculateTier()'s hysteresis input.
 * @return The resolved tier (through the guarded pipeline - trust
 *         substitution, then the unconditional vcell floor, then
 *         hysteresis) and the vcell/trust inputs it was resolved from.
 *
 * @details No persisted reads or writes, no SensorManager calls, no
 *          connection-mode side effect - depends only on
 *          BatteryTierGuard/PowerTier/BatteryHealth and its own parameters.
 *          Host-compilable directly, with zero stubbing, exactly like those
 *          three modules already are.
 */
Verdict evaluate(float currentSoC, SensorManager::VcellSampleState vcellState,
                  float vcell, BatteryHealth::SocTrust trust, BatteryTier previousTier);

/**
 * @brief The persisted tier from the last commit() - evaluate()'s
 *        hysteresis input, and the sole narrow read seam onto it.
 *
 * @details Folds Step 0's `reporting/BatteryTierStore.h` seam into this
 *          module: same "one accessor, non-relative, shadowable by a
 *          test's -I override directory" discipline, now for the tier/
 *          low-battery-mode owner instead of a standalone file. Clamped to
 *          TIER_HEALTHY if the persisted value is not yet valid (fresh
 *          device).
 */
BatteryTier currentTier();

/**
 * @brief Gathers evaluate()'s inputs from SensorManager/currentTier() and
 *        evaluates them - the one input-gathering path all read paths share.
 *
 * @param currentSoC Current fuel-gauge state of charge (0-100).
 * @return The same Verdict evaluate() itself would return, given this
 *         moment's SensorManager/persisted-tier snapshot.
 *
 * @details Read-only - a query, despite living in
 *          power/BatteryAuthorityCommand.cpp rather than the pure
 *          power/BatteryAuthority.cpp. It lives there specifically because
 *          it needs SensorManager, which evaluate()'s own translation unit
 *          must not depend on (see battery_authority_seam_structural_test.py) -
 *          not because it writes anything. resolveRuntime(),
 *          currentBatteryTierForFailsafe(), and
 *          currentBatteryTierForFailsafeLocal() all call this instead of
 *          each independently sampling SensorManager, so the three read
 *          paths cannot silently drift from each other on how vcell/trust
 *          are gathered.
 */
Verdict evaluateCurrent(float currentSoC);

/**
 * @brief The only function that writes set_currentBatteryTier/
 *        set_lowBatteryMode and applies the paired connection-mode side
 *        effect. Command, not query.
 *
 * @param verdict A Verdict already computed by evaluate() this cycle.
 * @param currentSoC The same SoC evaluate() was called with - used only for
 *        this function's own transition log line, unchanged in format from
 *        the retired applyBatteryAwareConnectionModePolicy().
 *
 * @details Safe to call repeatedly with the same verdict: the persisted
 *          tier and low-battery-mode fields are written only when they
 *          actually change from what is already persisted.
 */
void commit(const Verdict &verdict, float currentSoC);

/**
 * @brief Clears the sticky low-battery connection-mode downgrade flag.
 *
 * @details Used only by `ConfigApply.cpp`, when an operator explicitly sets
 *          a new `connectionMode` via cloud config - that explicit choice
 *          should override any sticky low-battery downgrade rather than
 *          have it silently reassert itself on the next commit(). Routed
 *          through the same private write path as commit() itself, so the
 *          persisted low-battery-mode field keeps exactly one write site.
 */
void clearLowBatteryMode();

} // namespace BatteryAuthority
