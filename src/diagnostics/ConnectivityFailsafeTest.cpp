#include "diagnostics/ConnectivityFailsafeTest.h"

#include "Config.h"
#include "MyPersistentData.h"
#include "cloud/Cloud.h"
#include "power/ConnectivityPolicy.h"
#include "power/PowerPlatform.h"
#include "state/StateMachine.h"

#if CONNECTIVITY_FAILSAFE_TEST_MODE
#warning "CONNECTIVITY_FAILSAFE_TEST_MODE=1 (bench timings enabled)"
#else
#warning "CONNECTIVITY_FAILSAFE_TEST_MODE=0 (production timings enabled)"
#endif

#if CONNECTIVITY_FAILSAFE_TEST_MODE

namespace {

const char *stateShortNameLocal(State value) {
  switch (value) {
  case INITIALIZATION_STATE:
    return "Init";
  case ERROR_STATE:
    return "Error";
  case IDLE_STATE:
    return "Idle";
  case SLEEPING_STATE:
    return "Sleep";
  case CONNECTING_STATE:
    return "Connect";
  case REPORTING_STATE:
    return "Report";
  case FIRMWARE_UPDATE_STATE:
    return "FW";
  default:
    return "?";
  }
}

const char *batteryTierShortNameLocal(BatteryTier tier) {
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

BatteryTier currentBatteryTierForFailsafeLocal() {
  const uint8_t tierValue = sysStatus.get_currentBatteryTier();
  if (tierValue <= TIER_SURVIVAL) {
    return static_cast<BatteryTier>(tierValue);
  }
  return Cloud::calculateBatteryTier(current.get_stateOfCharge());
}

bool connectivityFailsafeHasExternalPowerLocal() {
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

long connectivityFailsafeAgeSecOrNegative(time_t timestamp) {
  if (!Time.isValid() || timestamp == 0) {
    return -1L;
  }

  const time_t now = Time.now();
  if (now <= timestamp) {
    return -1L;
  }
  return (long)(now - timestamp);
}

void formatFailsafeAgeField(char *buffer, size_t bufferSize, time_t timestamp) {
  const long ageSec = connectivityFailsafeAgeSecOrNegative(timestamp);
  if (timestamp == 0) {
    snprintf(buffer, bufferSize, "0");
  } else if (ageSec >= 0) {
    snprintf(buffer, bufferSize, "%lds", ageSec);
  } else {
    snprintf(buffer, bufferSize, "%ld", (long)timestamp);
  }
}

void formatFailsafeConnectionAgeField(char *buffer, size_t bufferSize, time_t lastConnection) {
  const long ageSec = connectivityFailsafeAgeSecOrNegative(lastConnection);
  if (ageSec >= 0) {
    snprintf(buffer, bufferSize, "%lds", ageSec);
  } else {
    snprintf(buffer, bufferSize, "na");
  }
}

int plannedClosedHoursSleepSec() {
  if (!Time.isValid() || !Config::isValid(false) || isWithinOpenHours()) {
    return -1;
  }

  int nightSleepSec = secondsUntilNextOpen();
  if (nightSleepSec <= 0) {
    nightSleepSec = Config::DEFAULT_REPORT_INTERVAL_SEC;
  }

  const int maxSleepSec = 546 * 60;
  if (nightSleepSec > maxSleepSec) {
    nightSleepSec = maxSleepSec;
  }
  return nightSleepSec;
}

FailsafeDeferReason currentFailsafeEligibilityReason() {
  if (sysStatus.get_connectionMode() == DISCONNECTED) {
    return FAILSAFE_DEFER_DISCONNECTED_MODE;
  }

  if (state == FIRMWARE_UPDATE_STATE || System.updatesPending()) {
    return FAILSAFE_DEFER_UPDATE_PENDING;
  }

  if (!Time.isValid()) {
    return FAILSAFE_DEFER_INVALID_TIME;
  }

  const time_t lastConnection = sysStatus.get_lastConnection();
  if (lastConnection == 0) {
    return FAILSAFE_DEFER_NO_LAST_CONNECTION;
  }

  const time_t now = Time.now();
  if (now > lastConnection) {
    const time_t connectionAgeSec = now - lastConnection;
    uint8_t currentStage = sysStatus.get_connectivityRecoveryStage();
    if (currentStage > 3) {
      currentStage = 0;
    }

    if (connectionAgeSec >= ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC && currentStage < 3) {
      time_t lastAction = sysStatus.get_lastConnectivityRecoveryAction();
      if (lastAction > now) {
        lastAction = 0;
      }

      const uint8_t nextStage = currentStage + 1;
      time_t requiredDelay = ConnectivityPolicy::CONNECTIVITY_FAILSAFE_COOLDOWN_SEC;
      if (nextStage >= 2) {
        requiredDelay += (time_t)ConnectivityPolicy::CONNECTIVITY_FAILSAFE_JITTER_MAX_SEC;
      }

      const bool cooldownComplete =
          (currentStage == 0 || lastAction == 0 || (now - lastAction) >= requiredDelay);

      if (cooldownComplete) {
        const BatteryTier tier = currentBatteryTierForFailsafeLocal();
        const bool externalPowerPresent = connectivityFailsafeHasExternalPowerLocal();
        const bool lowBatteryHardActionBlocked =
            nextStage >= 2 && !externalPowerPresent &&
            (sysStatus.get_lowBatteryMode() || tier == TIER_SURVIVAL);

        if (lowBatteryHardActionBlocked) {
          return FAILSAFE_DEFER_LOW_BATTERY_HARD_STAGE_SUPPRESSED;
        }
      }
    }
  }

  return FAILSAFE_DEFER_NONE;
}

void logConnectivityFailsafeBootTestContext() {
  const int closedSleepSec = plannedClosedHoursSleepSec();
  if (closedSleepSec >
      (int)ConnectivityPolicy::CONNECTIVITY_FAILSAFE_TEST_MAX_CLOSED_SLEEP_SEC) {
    Log.info("FailsafeTest: closed-hours sleep cap active planned=%ds cap=%lds",
             closedSleepSec,
             (long)ConnectivityPolicy::CONNECTIVITY_FAILSAFE_TEST_MAX_CLOSED_SLEEP_SEC);
  }
}

} // namespace

namespace ConnectivityFailsafeTest {

uint32_t jitterSec(uint8_t stage) {
  (void)stage;
  return 0;
}

void logBootDiagnostics() {
  char lastActionField[24];
  char connectionAgeField[24];
  formatFailsafeAgeField(lastActionField, sizeof(lastActionField),
                         sysStatus.get_lastConnectivityRecoveryAction());
  formatFailsafeConnectionAgeField(connectionAgeField, sizeof(connectionAgeField),
                                   sysStatus.get_lastConnection());

  const FailsafeDeferReason reason = currentFailsafeEligibilityReason();
  if (reason == FAILSAFE_DEFER_NONE) {
    Log.info("FailsafeBoot: test=1 stale=%lds cd=%lds jit=%lds stage=%u cnt=%u lastAct=%s connAge=%s eligible=1",
             (long)ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC,
             (long)ConnectivityPolicy::CONNECTIVITY_FAILSAFE_COOLDOWN_SEC,
             (long)ConnectivityPolicy::CONNECTIVITY_FAILSAFE_JITTER_MAX_SEC,
             (unsigned)sysStatus.get_connectivityRecoveryStage(),
             (unsigned)sysStatus.get_connectivityRecoveryCount(),
             lastActionField,
             connectionAgeField);
  } else {
    Log.info("FailsafeBoot: test=1 stale=%lds cd=%lds jit=%lds stage=%u cnt=%u lastAct=%s connAge=%s eligible=0 reason=%s",
             (long)ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC,
             (long)ConnectivityPolicy::CONNECTIVITY_FAILSAFE_COOLDOWN_SEC,
             (long)ConnectivityPolicy::CONNECTIVITY_FAILSAFE_JITTER_MAX_SEC,
             (unsigned)sysStatus.get_connectivityRecoveryStage(),
             (unsigned)sysStatus.get_connectivityRecoveryCount(),
             lastActionField,
             connectionAgeField,
             failsafeDeferReasonName(reason));
  }

  logConnectivityFailsafeBootTestContext();
}

void logDeferDisconnectedMode() {
  if (claimFailsafeDeferLog(FAILSAFE_DEFER_DISCONNECTED_MODE)) {
    Log.info("FailsafeDefer: reason=%s mode=%u",
             failsafeDeferReasonName(FAILSAFE_DEFER_DISCONNECTED_MODE),
             (unsigned)sysStatus.get_connectionMode());
  }
}

void logDeferUpdatePending() {
  if (claimFailsafeDeferLog(FAILSAFE_DEFER_UPDATE_PENDING)) {
    Log.info("FailsafeDefer: reason=%s state=%s pending=%d",
             failsafeDeferReasonName(FAILSAFE_DEFER_UPDATE_PENDING),
             stateShortNameLocal(state),
             System.updatesPending() ? 1 : 0);
  }
}

void logDeferInvalidTime() {
  if (claimFailsafeDeferLog(FAILSAFE_DEFER_INVALID_TIME)) {
    Log.info("FailsafeDefer: reason=%s",
             failsafeDeferReasonName(FAILSAFE_DEFER_INVALID_TIME));
  }
}

void logDeferNoLastConnection() {
  if (claimFailsafeDeferLog(FAILSAFE_DEFER_NO_LAST_CONNECTION)) {
    Log.info("FailsafeDefer: reason=%s",
             failsafeDeferReasonName(FAILSAFE_DEFER_NO_LAST_CONNECTION));
  }
}

void logDeferLowBatteryHardStageSuppressed(uint8_t nextStage, BatteryTier tier) {
  if (claimFailsafeDeferLog(FAILSAFE_DEFER_LOW_BATTERY_HARD_STAGE_SUPPRESSED)) {
    Log.info("FailsafeDefer: reason=%s stage=%u tier=%s",
             failsafeDeferReasonName(FAILSAFE_DEFER_LOW_BATTERY_HARD_STAGE_SUPPRESSED),
             (unsigned)nextStage,
             batteryTierShortNameLocal(tier));
  }
}

} // namespace ConnectivityFailsafeTest

#endif
