#include "power/ConnectivityFailsafe.h"
#include "state/StateMachine.h"
#include "power/ConnectivityPolicy.h"
#include "power/PowerPlatform.h"
#include "power/BatteryAuthority.h"
#include "diagnostics/ConnectivityFailsafeTest.h"
#include "observability/LoopForensics.h"
#include "reporting/ReportingPolicy.h"
#include "time/DailyBoundary.h"
#include "persist/PowerConfig.h"
#include "ResetCause.h"
namespace {
const char *batteryTierShortName(BatteryTier tier) {
  switch (tier) {
  case TIER_HEALTHY:
    return "H";
  case TIER_CONSERVING:
    return "C";
  case TIER_CRITICAL:
    return "CR";
  case TIER_SURVIVAL:
    return "S";
  default:
    return "?";
  }
}

BatteryTier currentBatteryTierForFailsafe() {
  const uint8_t tierValue = PowerConfig::get_currentBatteryTier();
  if (tierValue <= TIER_SURVIVAL) {
    return static_cast<BatteryTier>(tierValue);
  }
  // WO-2026-09-21 Step 4 (corrected same day, twice):
  // BatteryAuthority::evaluateCurrent() - the pure guarded pipeline (vcell
  // floor + trust substitution), fed by the one input-gathering path all
  // read paths share - not the deleted, unguarded Cloud::calculateBatteryTier().
  // Read-only: this fallback never commits. The failsafe's low-battery block
  // must be reachable only through this guarded path.
  return BatteryAuthority::evaluateCurrent(PowerManager::instance().soc()).tier;
}

bool connectivityFailsafeHasExternalPower() {
  const PowerPlatform::PowerSourceSnapshot snapshot = PowerPlatform::readPowerSource();
  if (snapshot.status == PowerAvailability::NotAvailable) {
    return false;
  }

  switch (snapshot.source) {
  case 1:
  case 2:
  case 3:
  case 4:
    return true;
  default:
    return false;
  }
}

uint32_t connectivityFailsafeJitterSec(uint8_t stage) {
#if CONNECTIVITY_FAILSAFE_TEST_MODE
  return ConnectivityFailsafeTest::jitterSec(stage);
#else
  if (ConnectivityPolicy::CONNECTIVITY_FAILSAFE_JITTER_MAX_SEC <= 0) {
    return 0;
  }

  String deviceId = System.deviceID();
  uint32_t hash = 5381u;
  for (size_t index = 0; index < deviceId.length(); ++index) {
    hash = ((hash << 5) + hash) ^ (uint8_t)deviceId.charAt(index);
  }
  hash ^= (uint32_t)stage * 2654435761u;
  return hash % (uint32_t)ConnectivityPolicy::CONNECTIVITY_FAILSAFE_JITTER_MAX_SEC;
#endif
}

void persistConnectivityFailsafeState(uint8_t stage, time_t actionTime, bool incrementCount) {
  RecoveryState::set_connectivityRecoveryStage(stage);
  RecoveryState::set_lastConnectivityRecoveryAction(actionTime);
  if (incrementCount) {
    uint8_t count = RecoveryState::get_connectivityRecoveryCount();
    if (count < 0xFF) {
      RecoveryState::set_connectivityRecoveryCount(count + 1);
    }
  }
  SystemConfig::flushNow();
}

const char *failsafeDeferReasonNameLocal(FailsafeDeferReason reason) {
  switch (reason) {
  case FAILSAFE_DEFER_INVALID_TIME:
    return "invalid-time";
  case FAILSAFE_DEFER_NO_LAST_CONNECTION:
    return "no-last-connection";
  case FAILSAFE_DEFER_DISCONNECTED_MODE:
    return "disconnected-mode";
  case FAILSAFE_DEFER_UPDATE_PENDING:
    return "update-pending";
  case FAILSAFE_DEFER_LOW_BATTERY_HARD_STAGE_SUPPRESSED:
    return "low-battery-hard-stage-suppressed";
  case FAILSAFE_DEFER_CLOSED_HOURS_LONG_SLEEP:
    return "closed-hours-long-sleep";
  default:
    return "none";
  }
}

bool claimFailsafeDeferLogLocal(FailsafeDeferReason reason) {
  if (reason == FAILSAFE_DEFER_NONE) {
    return false;
  }

  const uint8_t shift = (uint8_t)reason - 1;
  const uint32_t bit = (shift < 32) ? (1UL << shift) : 0UL;
  if (bit == 0UL) {
    return false;
  }
  if (session.failsafeDeferLogMask & bit) {
    return false;
  }
  session.failsafeDeferLogMask |= bit;
  return true;
}

} // namespace

const char *failsafeDeferReasonName(FailsafeDeferReason reason) {
  return failsafeDeferReasonNameLocal(reason);
}

bool claimFailsafeDeferLog(FailsafeDeferReason reason) {
  return claimFailsafeDeferLogLocal(reason);
}

void clearConnectivityFailsafeRecovery(const char *reason) {
  const bool hadRecoveryState =
      RecoveryState::get_connectivityRecoveryStage() != 0 ||
      RecoveryState::get_lastConnectivityRecoveryAction() != 0 ||
      RecoveryState::get_connectivityRecoveryCount() != 0;
  const bool hadAlert = (RecoveryState::get_alertCode() == ConnectivityPolicy::CONNECTIVITY_FAILSAFE_ALERT);

  if (!hadRecoveryState && !hadAlert) {
    return;
  }

  RecoveryState::set_connectivityRecoveryStage(0);
  RecoveryState::set_lastConnectivityRecoveryAction(0);
  RecoveryState::set_connectivityRecoveryCount(0);
  SystemConfig::flushNow();

  if (hadAlert) {
    RecoveryState::set_alertCode(0);
    RecoveryState::set_lastAlertTime(0);
  }

  Log.info("Failsafe: cleared reason=%s", reason ? reason : "unknown");
}

/**
 * @brief Monitors connection health and triggers escalating recovery when stale.
 *
 * Implements escalating recovery when connection attempts exceed policy thresholds:
 * Stage 2 (system reset), then Stage 3 (deep power-down, Boron only). Stage 1
 * (radio reset) is retired (WO-2026-10-02-001 item B); the first action is stage 2.
 * Prevents indefinite modem-on battery drain and recovers from radio firmware wedges.
 *
 * Acts unless Closed (Unknown counts as open), measuring the age from the later of lastConnection
 * and today's opening. Defers if disconnected, firmware updating, time invalid,
 * recently connected,
 * within cooldown period, or jitter delay not elapsed (prevents fleet-wide simultaneous resets).
 *
 * @note Stage 3 deep power-down will not wake for hours/days - last resort only.
 */
void connectivityFailsafeSupervisor() {
  if (PowerManager::instance().effectiveConnectionMode() == SystemConfig::DISCONNECTED) {
#if CONNECTIVITY_FAILSAFE_TEST_MODE
    ConnectivityFailsafeTest::logDeferDisconnectedMode();
#endif
    return;
  }

  if (state == FIRMWARE_UPDATE_STATE || System.updatesPending()) {
#if CONNECTIVITY_FAILSAFE_TEST_MODE
    ConnectivityFailsafeTest::logDeferUpdatePending();
#endif
    return;
  }

  // WO-2026-09-19 Step 3b: deliberately NOT converted to Clock::isTrusted(),
  // even though this function's connectionAgeSec math below is exactly the
  // kind of untrusted-clock arithmetic Step 3b otherwise guards against.
  // This escalation ladder (system reset -> deep power-down)
  // exists to recover a wedged modem/radio - and a device with a wedged
  // radio has, by definition, never completed a sync this boot either, so
  // gating the recovery on Clock::isTrusted() would disable the recovery
  // mechanism in exactly the case it exists for (circular: the fault this
  // supervisor recovers from is often the same fault that prevents the
  // trust signal from ever going true). The narrower risk of staying on
  // isTimeValid() (a plausible-but-wrong RTC-seeded epoch skewing
  // connectionAgeSec) is judged acceptable against the alternative of
  // silently disabling hardware recovery. Revisit only with a case that
  // demonstrates the escalation firing on a genuinely wrong clock in the
  // field - do not "fix" this speculatively.
  if (!Clock::isTimeValid()) {
#if CONNECTIVITY_FAILSAFE_TEST_MODE
    ConnectivityFailsafeTest::logDeferInvalidTime();
#endif
    return;
  }

  const time_t lastConnection = SystemConfig::get_lastConnection();
  if (lastConnection == 0) {
#if CONNECTIVITY_FAILSAFE_TEST_MODE
    ConnectivityFailsafeTest::logDeferNoLastConnection();
#endif
    return;
  }

  // WO-2026-10-02-001 item A: act unless Closed (Unknown counts as open), and measure the age
  // from the later of lastConnection and today's opening. Wall-clock age would
  // make every device cross the 3 h threshold on its 06:00 wake, because the
  // overnight gap since the 22:00 close is 7-8 h and this supervisor runs at
  // the top of loop() before the report has started connecting.
  const time_t now = Time.now();
  const time_t openedAt = DailyBoundary::todayAt(SystemConfig::get_openTime());
  const time_t ageBase = (openedAt > lastConnection) ? openedAt : lastConnection;
  // Recovery item 18 (WO-2026-10-24-001): Unknown is treated as Open here. Its own
  // stage-2 reset leaves the clock untrusted, so requiring Open is the circular case above.
  if (Clock::openness() == Clock::Openness::Closed || now <= ageBase) {
    return;
  }

  const time_t connectionAgeSec = now - ageBase;
  if (connectionAgeSec < ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC) {
    return;
  }

  uint8_t currentStage = RecoveryState::get_connectivityRecoveryStage();
  if (currentStage > 3) {
    currentStage = 0;
  }
  if (currentStage >= 3) {
    return;
  }

  time_t lastAction = RecoveryState::get_lastConnectivityRecoveryAction();
  if (lastAction > now) {
    lastAction = 0;
  }

  // WO-2026-10-02-001 item B: stage 1 (radio reset) is retired - it never
  // restored a wedged connection in the field. Stage numbers keep their
  // meaning, so the first action is stage 2 and a stage 1 persisted by older
  // firmware still progresses to 2.
  const uint8_t nextStage = (currentStage <= 1) ? 2 : (uint8_t)(currentStage + 1);
  time_t requiredDelay = ConnectivityPolicy::CONNECTIVITY_FAILSAFE_COOLDOWN_SEC;
  if (nextStage >= 2) {
    requiredDelay += (time_t)connectivityFailsafeJitterSec(nextStage);
  }

  if (currentStage != 0 && lastAction != 0 && (now - lastAction) < requiredDelay) {
    return;
  }

  // WO-2026-10-08-001: a connection is not overdue until one cadence after it
  // was due, so with a cadence of 3 h or more the threshold is cadence + 3 h.
  // Compiled out of the test build, where the 5 min threshold must stay a
  // 5 min threshold for the bench reset.
#if !CONNECTIVITY_FAILSAFE_TEST_MODE
  const time_t cadenceSec = (time_t)ReportingPolicyResolver::resolveRuntime(
      PowerManager::instance().soc(), now).effectiveIntervalSec;
  if (cadenceSec >= ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC &&
      connectionAgeSec < ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC + cadenceSec) {
    return;
  }
#endif

  if (activeConnectAttemptWithinBudget()) {
    return;
  }

  const BatteryTier tier = currentBatteryTierForFailsafe();
  const bool externalPowerPresent = connectivityFailsafeHasExternalPower();
  const bool lowBatteryHardActionBlocked =
      nextStage >= 2 &&
      !externalPowerPresent &&
      (PowerManager::instance().downgradeActive() || tier == TIER_SURVIVAL);

  if (lowBatteryHardActionBlocked) {
#if CONNECTIVITY_FAILSAFE_TEST_MODE
    ConnectivityFailsafeTest::logDeferLowBatteryHardStageSuppressed(nextStage, tier);
#else
    Log.info("Failsafe: defer reason=low-battery stage=%u tier=%s",
             (unsigned)nextStage,
             batteryTierShortName(tier));
#endif
    persistConnectivityFailsafeState(currentStage, now, false);
    return;
  }

  if (nextStage == 2) {
    setAppBreadcrumb(BREADCRUMB_CONNECTIVITY_FAILSAFE_HARD);
    persistConnectivityFailsafeState(2, now, true);
    Log.info("Failsafe: stage=2 action=system-reset age=%lds",
             (long)connectionAgeSec);
    System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);
    return;
  }

  setAppBreadcrumb(BREADCRUMB_CONNECTIVITY_FAILSAFE_HARD);
  persistConnectivityFailsafeState(3, now, true);
  Log.info("Failsafe: stage=3 action=deep-powerdown age=%lds",
           (long)connectionAgeSec);
  ab1805.deepPowerDown();
}
