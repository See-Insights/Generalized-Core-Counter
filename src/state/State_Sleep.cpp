#include "BuildProfile.h"
#include "ResetCause.h"
#include "state/State_Common.h"
#include "../Config.h"
#include "cloud/Cloud.h"
#include "power/Connectivity.h"
#include "power/ConnectivityPolicy.h"
#include "power/ModemHealth.h"
#include "power/PowerDiagnostics.h"
#include "power/PowerManager.h"
#include "power/BatteryAuthority.h"
#include "LocalTimeRK.h"
#include "persist/CurrentReadings.h"
#include "persist/PowerConfig.h"
#include "persist/RecoveryState.h"
#include "persist/SystemConfig.h"
#include "PublishQueuePosixRK.h"
#include "sensors/SensorManager.h"
#include "device_pinout.h"
#include "sensors/SensorDefinitions.h"
#include "AB1805_RK.h"
#include "observability/AwakeCycleCounters.h"
#include "observability/WakeCycleStats.h"
#include "ThrashGuard.h"
#include "state/SleepPrepSpanTiming.h"
#include "state/SleepTeardown.h"
#include "time/HibernateCycle.h"
#include "time/HibernateEligibility.h"
#include "time/Clock.h"
#include "time/DailyBoundary.h"

namespace {

void logWakeSummary(const char *reason) {
  Log.info("Wake: reason=%s open=%d ready=%d occ=%d led=%lus",
           reason,
           isWithinOpenHours() ? 1 : 0,
           SensorManager::instance().isSensorReady() ? 1 : 0,
           CurrentReadings::get_occupied() ? 1 : 0,
           (unsigned long)(signalLEDTimeRemaining() / 1000UL));
}

// Drain USB CDC serial buffer before sleep to prevent split log lines (bench testing only)
void drainSerialBeforeSleep() {
  if (Serial.isConnected()) {
    Serial.flush();
    delay(75);
  }
}

unsigned long computeCloudSyncTimeoutMs(uint16_t queueDepth) {
  const unsigned long baseTimeoutMs = ConnectivityPolicy::CLOUD_OPS_GATE_TIMEOUT_MS;
  if (queueDepth == 0) {
    return baseTimeoutMs;
  }

  const unsigned long publishStartupMs = 2000UL;
  const unsigned long perEventDrainMs = 1000UL;
  const unsigned long queueDrainCushionMs = 5000UL;
  const unsigned long maxTimeoutMs = 120000UL;

  unsigned long queueAwareTimeoutMs =
      publishStartupMs + ((unsigned long)queueDepth * perEventDrainMs) + queueDrainCushionMs;
  if (queueAwareTimeoutMs < baseTimeoutMs) {
    queueAwareTimeoutMs = baseTimeoutMs;
  }
  if (queueAwareTimeoutMs > maxTimeoutMs) {
    queueAwareTimeoutMs = maxTimeoutMs;
  }
  return queueAwareTimeoutMs;
}

constexpr unsigned long kGateBlockLogThresholdMs = 10000UL;

constexpr uint8_t kGateBlockerQueue = 0x01;
constexpr uint8_t kGateBlockerLedger = 0x02;
constexpr uint8_t kGateBlockerWebhook = 0x04;
constexpr uint8_t kGateBlockerUpdate = 0x08;
constexpr uint8_t kGateBlockerUninitialized = 0xFF;

const char *gateBlockerReasonLabel(uint8_t blockerMask) {
  switch (blockerMask) {
  case 0x00:
    return "none";
  case kGateBlockerQueue:
    return "queue";
  case kGateBlockerLedger:
    return "ledger";
  case kGateBlockerQueue | kGateBlockerLedger:
    return "queue+ledger";
  case kGateBlockerWebhook:
    return "webhook";
  case kGateBlockerQueue | kGateBlockerWebhook:
    return "queue+webhook";
  case kGateBlockerLedger | kGateBlockerWebhook:
    return "ledger+webhook";
  case kGateBlockerQueue | kGateBlockerLedger | kGateBlockerWebhook:
    return "queue+ledger+webhook";
  case kGateBlockerUpdate:
    return "update";
  case kGateBlockerQueue | kGateBlockerUpdate:
    return "queue+update";
  case kGateBlockerLedger | kGateBlockerUpdate:
    return "ledger+update";
  case kGateBlockerQueue | kGateBlockerLedger | kGateBlockerUpdate:
    return "queue+ledger+update";
  case kGateBlockerWebhook | kGateBlockerUpdate:
    return "webhook+update";
  case kGateBlockerQueue | kGateBlockerWebhook | kGateBlockerUpdate:
    return "queue+webhook+update";
  case kGateBlockerLedger | kGateBlockerWebhook | kGateBlockerUpdate:
    return "ledger+webhook+update";
  case kGateBlockerQueue | kGateBlockerLedger | kGateBlockerWebhook |
      kGateBlockerUpdate:
    return "queue+ledger+webhook+update";
  default:
    return "unknown";
  }
}

struct {
  bool operationsCompleteLogged = false;  // Track if we've logged completion message
  bool gateWasBlocking = false;
  unsigned long lastGateStatusLogMs = 0;
  uint8_t lastGateBlockerMask = kGateBlockerUninitialized;
  uint8_t lastGateBlockerBeforeRelease = 0;
  bool gateReleaseReasonKnown = false;
  unsigned long cloudSyncStartMs = 0;
  unsigned long cloudSyncBudgetMs = 0;
  uint16_t cloudSyncMaxQueueDepth = 0;
  uint16_t lastQueueDepth = 0xFFFF;
} cloudGate;

bool cloudGatePass(bool useNetworkStandbyEffective) {
  // Only wait for cloud operations *before* requesting disconnect.
  if (Particle.connected() && !SleepTeardown::disconnectRequested()) {
    // Initialize timer on first check
    if (cloudGate.cloudSyncStartMs == 0) {
      cloudGate.cloudSyncStartMs = millis();
    }
    // Check prerequisite completion
    bool queueEmpty = PublishQueuePosix::instance().getCanSleep();
    bool configLedgersSynced = Cloud::instance().areLedgersSynced();
    bool outputLedgersSynced = !Cloud::instance().hasPendingOutputLedgerSync();
    Cloud::LedgerSyncDiagnostics ledgerDiagnostics = Cloud::instance().ledgerSyncDiagnostics();
  #if !ENABLE_GATE_TRACE
    (void)ledgerDiagnostics;
  #endif
    bool ledgersSynced = configLedgersSynced && outputLedgersSynced;
    bool updatesChecked = !System.updatesPending();
    bool webhookConfirmed = !session.awaitingWebhookResponse;
    uint16_t queueDepth = (uint16_t)PublishQueuePosix::instance().getNumEvents();
    if (queueDepth > cloudGate.cloudSyncMaxQueueDepth) {
      cloudGate.cloudSyncMaxQueueDepth = queueDepth;
    }
    unsigned long queueAwareBudgetMs = computeCloudSyncTimeoutMs(cloudGate.cloudSyncMaxQueueDepth);
    if (queueAwareBudgetMs > cloudGate.cloudSyncBudgetMs) {
      cloudGate.cloudSyncBudgetMs = queueAwareBudgetMs;
    }
    if (!outputLedgersSynced &&
        ConnectivityPolicy::OUTPUT_LEDGER_SYNC_TIMEOUT_MS > cloudGate.cloudSyncBudgetMs) {
      cloudGate.cloudSyncBudgetMs = ConnectivityPolicy::OUTPUT_LEDGER_SYNC_TIMEOUT_MS;
    }

    {
      if (cloudGate.lastQueueDepth != 0xFFFF && queueDepth < cloudGate.lastQueueDepth) {
        thrashGuard.markProgress("QUEUE_DRAIN");
      }
      cloudGate.lastQueueDepth = queueDepth;
    }

    bool allComplete = queueEmpty && ledgersSynced && updatesChecked && webhookConfirmed;

    if (!allComplete) {
      unsigned long elapsedMs = millis() - cloudGate.cloudSyncStartMs;
      const int queuePending = queueEmpty ? 0 : 1;
      const int ledgerPending = ledgersSynced ? 0 : 1;
      const int webhookPending = webhookConfirmed ? 0 : 1;
      const int updatePending = updatesChecked ? 0 : 1;
      uint8_t blockerMask = 0;
      if (queuePending) {
        blockerMask |= kGateBlockerQueue;
      }
      if (ledgerPending) {
        blockerMask |= kGateBlockerLedger;
      }
      if (webhookPending) {
        blockerMask |= kGateBlockerWebhook;
      }
      if (updatePending) {
        blockerMask |= kGateBlockerUpdate;
      }
      if (blockerMask != 0) {
        cloudGate.lastGateBlockerBeforeRelease = blockerMask;
        cloudGate.gateReleaseReasonKnown = true;
      }

      if (elapsedMs > kGateBlockLogThresholdMs &&
          cloudGate.lastGateBlockerMask != blockerMask) {
        if (ledgerPending) {
#if ENABLE_GATE_TRACE
          Log.info("GateDetail: wait=%lu ledger=%d pending=%u syncing=%d inflight=%d reason=%s",
                   elapsedMs,
                   ledgerPending,
                   ledgerDiagnostics.pendingCount,
                   ledgerDiagnostics.syncing ? 1 : 0,
                   ledgerDiagnostics.inflight ? 1 : 0,
                   gateBlockerReasonLabel(blockerMask));
          Log.info("GateDetailState: config=%d output=%d queued=%d statusSync=%d dataSync=%d statusInflight=%d dataInflight=%d",
                   configLedgersSynced ? 1 : 0,
                   outputLedgersSynced ? 1 : 0,
                   ledgerDiagnostics.pendingStatusPublish ? 1 : 0,
                   ledgerDiagnostics.pendingDeviceStatusSync ? 1 : 0,
                   ledgerDiagnostics.pendingDeviceDataSync ? 1 : 0,
                   ledgerDiagnostics.statusInflight ? 1 : 0,
                   ledgerDiagnostics.dataInflight ? 1 : 0);
#endif
        }
#if ENABLE_GATE_TRACE
        Log.info("GateBlock: wait=%lu reason=%s q=%d ledger=%d webhook=%d update=%d",
                 elapsedMs,
                 gateBlockerReasonLabel(blockerMask),
                 (int)queueDepth,
                 ledgerPending,
                 webhookPending,
                 updatePending);
#endif
        cloudGate.lastGateBlockerMask = blockerMask;
      }

      if (elapsedMs < cloudGate.cloudSyncBudgetMs) {
        if (!cloudGate.gateWasBlocking ||
            (millis() - cloudGate.lastGateStatusLogMs) >= ConnectivityPolicy::CLOUD_OPS_STATUS_LOG_INTERVAL_MS) {
#if ENABLE_GATE_TRACE
          Log.info("Gate: q=%d ledger=%d webhook=%d update=%d wait=%lu/%lu",
                   (int)queueDepth,
                   ledgerPending,
                   webhookPending,
                   updatePending,
                   elapsedMs,
                   cloudGate.cloudSyncBudgetMs);
#endif
          cloudGate.gateWasBlocking = true;
          cloudGate.lastGateStatusLogMs = millis();
        }
        // Mark progress to prevent ThrashGuard timeout during legitimate cloud operations wait
        thrashGuard.markProgress("CLOUD_OPS_WAIT");
        serviceAwakeWatchdog();
        Particle.process(); // Keep connection alive
        return true; // Stay in SLEEPING_STATE until complete or timeout
      }

      // Budget exceeded - log incomplete operations and raise ONE alert (priority order)
       const char *gateFailReason = !queueEmpty ? "queue" :
        (!ledgersSynced ? "ledger" : (!webhookConfirmed ? "webhook" : "update"));
       Log.warn("GateFail: reason=%s timeout=%lu q=%d ledger=%d webhook=%d update=%d",
          gateFailReason,
          cloudGate.cloudSyncBudgetMs,
          (int)queueDepth,
          ledgerPending,
          webhookPending,
          updatePending);
       cloudGate.gateWasBlocking = false;
       cloudGate.lastGateStatusLogMs = 0;
       cloudGate.lastGateBlockerMask = kGateBlockerUninitialized;
       cloudGate.lastGateBlockerBeforeRelease = 0;
       cloudGate.gateReleaseReasonKnown = false;

      // Only raise one alert - check in priority order (queue > ledger > updates > webhook)
      if (!queueEmpty) {
        Log.warn("SLEEP: Publish queue not empty - raising alert 43");
        RecoveryState::raiseAlert(43); // Queue drainage failure (highest priority)
      } else if (!ledgersSynced) {
        Log.warn("SLEEP: ledger sync incomplete after %lu ms (budget=%lu ms) - raising alert 44",
                 elapsedMs,
                 cloudGate.cloudSyncBudgetMs);
        Cloud::instance().logLedgerSleepTimeoutState();
        RecoveryState::raiseAlert(44); // Ledger sync timeout before sleep (minor - config already applied)
      } else if (!updatesChecked) {
        Log.warn("SLEEP: OTA updates pending - raising alert 42");
        RecoveryState::raiseAlert(42); // OTA updates pending
      } else if (!webhookConfirmed) {
        Log.warn("SLEEP: Webhook response not received - raising alert 40");
        RecoveryState::raiseAlert(40); // Webhook response timeout
      }

      // Proceed with disconnect despite incomplete operations
      cloudGate.cloudSyncStartMs = 0;
      cloudGate.cloudSyncBudgetMs = 0;
      cloudGate.cloudSyncMaxQueueDepth = 0;
    } else {
      unsigned long gateWaitMs = millis() - cloudGate.cloudSyncStartMs;
      if (gateWaitMs > kGateBlockLogThresholdMs && cloudGate.lastGateBlockerMask != 0) {
#if ENABLE_GATE_TRACE
        Log.info("GateBlock: wait=%lu reason=none q=%u ledger=0 webhook=0 update=0",
                 gateWaitMs, (unsigned)PublishQueuePosix::instance().getNumEvents());
#endif
        cloudGate.lastGateBlockerMask = 0;
      }
      if (cloudGate.gateWasBlocking) {
        Cloud::LedgerSyncDiagnostics ledgerDiagnosticsRelease = Cloud::instance().ledgerSyncDiagnostics();
#if ENABLE_GATE_TRACE
        Log.info("GateReleaseDetail: wait=%lu pending=%u syncing=%d inflight=%d reason=%s",
                 gateWaitMs,
                 ledgerDiagnosticsRelease.pendingCount,
                 ledgerDiagnosticsRelease.syncing ? 1 : 0,
                 ledgerDiagnosticsRelease.inflight ? 1 : 0,
                 cloudGate.gateReleaseReasonKnown
                     ? gateBlockerReasonLabel(cloudGate.lastGateBlockerBeforeRelease)
                     : "unknown");
#endif
        (void)ledgerDiagnosticsRelease;
        Log.info("GateRelease: wait=%lu reason=%s",
                 gateWaitMs,
                 cloudGate.gateReleaseReasonKnown
                     ? gateBlockerReasonLabel(cloudGate.lastGateBlockerBeforeRelease)
                     : "unknown");
        Log.info("Gate: ok wait=%lu", gateWaitMs);
      }
      cloudGate.gateWasBlocking = false;
      cloudGate.lastGateStatusLogMs = 0;
      cloudGate.lastGateBlockerMask = kGateBlockerUninitialized;
      cloudGate.lastGateBlockerBeforeRelease = 0;
      cloudGate.gateReleaseReasonKnown = false;
      // All operations complete - reset timer and proceed
      cloudGate.cloudSyncStartMs = 0;
      cloudGate.cloudSyncBudgetMs = 0;
      cloudGate.cloudSyncMaxQueueDepth = 0;
    }

    if (!cloudGate.operationsCompleteLogged) {
      if (SystemConfig::get_verboseMode()) {
        Log.info("SLEEP: Cloud operations gate passed - ready to disconnect");
        Log.info("SLEEP: disconnect context connected=%d radioOn=%d standbyEffective=%d occupied=%d",
                 Particle.connected(), Connectivity::isRadioPoweredOn(), useNetworkStandbyEffective, CurrentReadings::get_occupied());
      }

      // Observability: end of service window (ledger sync + queue drain + OTA check + webhook).
      Observability::cycleStats().markServiceEnd(millis());

      cloudGate.operationsCompleteLogged = true;
    }
  } else {
    // Not connected - reset timer
    cloudGate.cloudSyncStartMs = 0;
    cloudGate.cloudSyncBudgetMs = 0;
    cloudGate.cloudSyncMaxQueueDepth = 0;
    cloudGate.lastGateBlockerMask = kGateBlockerUninitialized;
    cloudGate.lastGateBlockerBeforeRelease = 0;
    cloudGate.gateReleaseReasonKnown = false;
  }
  return false;
}

struct SleepPlan { int wakeInSeconds; const char *sleepReason; bool overnightFallbackSleep; };

bool sleepPlan(SleepPlan &plan) {
  // WO-2026-09-19 Step 3b: the behavioral core of this step. parkOpenness is
  // computed ONCE here and reused below (line ~1006 used to re-derive
  // isWithinOpenHours() a second time, redundantly) - Closed is the only
  // verdict that commits the device to the night-sleep branch below. An
  // untrusted clock (Unknown) must NOT commit to an overnight hibernate
  // based on a wrong belief about what time it is - see Clock::openness()'s
  // doc comment. logTimeDiag()'s isOpen= stays sourced from the raw,
  // fail-open isWithinOpenHours() (not parkOpenness) so its openness= field
  // in the same log line can be compared against it - the two are expected
  // to diverge exactly when the clock is untrusted.
  int nightSleepSec = -1;
  const Clock::Openness parkOpenness = Clock::openness();
  logTimeDiag(isWithinOpenHours());
  if (parkOpenness == Clock::Openness::Closed) {
    // WO-2026-09-30-001: never commit to night sleep while the close is due.
    if (DailyBoundary::check(Time.now()).due) {
      transitionTo(REPORTING_STATE, "close due before night sleep");
      return true;
    }

    // Notify sensor layer we are entering full night sleep so sensors and
    // indicator LEDs can be powered down. During daytime naps we keep
    // interrupt-driven sensors (like PIR) powered so they can wake the
    // device from ULTRA_LOW_POWER sleep.
    SensorManager::instance().onEnterSleep();
    signalLED(false);

    // ********** Night sleep (outside opening hours) **********
    nightSleepSec = secondsUntilNextOpen();
    if (nightSleepSec <= 0) {
      nightSleepSec = Config::DEFAULT_REPORT_INTERVAL_SEC;
    }

    // Device OS maximum sleep duration is 546 minutes (~9.1 hours).
    // Clamp our requested night sleep to this limit so the
    // underlying platform will reliably honour it.
    const int MAX_SLEEP_SEC = 546 * 60;
    if (nightSleepSec > MAX_SLEEP_SEC) {
      Log.info("Clamping night sleep duration to max supported %d seconds (requested=%d)", MAX_SLEEP_SEC, nightSleepSec);
      nightSleepSec = MAX_SLEEP_SEC;
    }

#if CONNECTIVITY_FAILSAFE_TEST_MODE
    const int requestedNightSleepSec = nightSleepSec;
    if (claimFailsafeDeferLog(FAILSAFE_DEFER_CLOSED_HOURS_LONG_SLEEP)) {
      Log.info("FailsafeDefer: reason=%s open=0 sleep=%ds",
               failsafeDeferReasonName(FAILSAFE_DEFER_CLOSED_HOURS_LONG_SLEEP),
               requestedNightSleepSec);
    }
    if (nightSleepSec > (int)ConnectivityPolicy::CONNECTIVITY_FAILSAFE_TEST_MAX_CLOSED_SLEEP_SEC) {
      Log.info("FailsafeTest: cap closed sleep %ds->%lds",
               requestedNightSleepSec,
               (long)ConnectivityPolicy::CONNECTIVITY_FAILSAFE_TEST_MAX_CLOSED_SLEEP_SEC);
      nightSleepSec = (int)ConnectivityPolicy::CONNECTIVITY_FAILSAFE_TEST_MAX_CLOSED_SLEEP_SEC;
    }
#endif

  }

  // ********** ULTRA_LOW_POWER sleep (daytime or night fallback) **********
  // During opening hours we use the reportingIntervalSec as before.
  // Outside opening hours,
  // fall back to ULTRA_LOW_POWER with a long sleep equal to the
  // time until next open to avoid rapid wake/sleep thrashing.
  uint16_t intervalSec = Config::reportingIntervalSecForRuntime();

  const char *sleepReason = "scheduled";
  int wakeInSeconds;
  // Reuses parkOpenness computed above rather than re-deriving
  // isWithinOpenHours() a second time - both checks are the identical
  // question, and re-deriving it separately is exactly how this site could
  // silently drift from the commitment decision above.
  const bool overnightFallbackSleep = (parkOpenness == Clock::Openness::Closed && nightSleepSec > 0);
  if (overnightFallbackSleep) {
    wakeInSeconds = nightSleepSec;
    sleepReason = "closed";
  } else {
    // Within opening hours (or clock Unknown - the short-nap path), align
    // wake to the reporting boundary. Add 1 second margin to ensure we wake
    // slightly after the boundary.
    // WO-2026-09-19 Step 3b: Clock::isTrusted(), not isTimeValid() - this is
    // a plain "can I trust Time.now() for this arithmetic" question, not an
    // open/closed one, so it stays a boolean rather than routing through
    // openness().
    if (Clock::isTrusted() && intervalSec > 0) {
      int boundary = (int)intervalSec;
      time_t now = Time.now();
      int offset = (int)(now % boundary);
      int aligned = boundary - offset;
      if (aligned < 1) {
        aligned = 1;
      } else if (aligned > boundary) {
        aligned = boundary;
      }
      wakeInSeconds = aligned + 1;
    } else {
      wakeInSeconds = (int)intervalSec;
    }
  }

  // COUNTING MODE: Defer sleep until LED flash completes
  if (SystemConfig::get_sensorMode() == SystemConfig::COUNTING) {
    uint32_t ledRemaining = signalLEDTimeRemaining();
    if (ledRemaining > 0) {
      Log.info("COUNTING: Deferring sleep - LED remaining=%lu sec", 
               ledRemaining);
      transitionTo(IDLE_STATE, "sleep-counting-led-defer");
      return true;
    }
    // Turn off LED before sleep (should already be off)
    if (signalLEDStatus()) {
      signalLED(false);
    }
  }

  // OCCUPANCY MODE: Calculate sleep duration based on debounce timer or scheduled wake
  if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY) {
    if (CurrentReadings::get_occupied()) {
      // Occupied - wake for debounce timeout check
      uint32_t setting1Raw = SystemConfig::SensorSettings::get_sensorSetting1();
      uint32_t debounceSeconds = (setting1Raw > 0) ? (setting1Raw / 1000) : 60;
      if (debounceSeconds < (uint32_t)wakeInSeconds) {
        wakeInSeconds = (int)debounceSeconds;
      }
      sleepReason = "debounce";
    } else {
      // Unoccupied - use scheduled wake time
      sleepReason = "scheduled";
    }
  }
  plan = {wakeInSeconds, sleepReason, overnightFallbackSleep};
  return false;
}

bool nightlyHeapGuard(bool overnightFallbackSleep) {
  // Before a long overnight
  // ULTRA_LOW_POWER fallback sleep, optionally reset first when heap has
  // drifted low. This gives the next day a clean start without affecting
  // open-hours behavior.
  if (overnightFallbackSleep) {
    const unsigned long freeHeap = (unsigned long)System.freeMemory();

    if (freeHeap <= ConnectivityPolicy::NIGHTLY_HEAP_RESET_THRESHOLD_BYTES) {
      Log.info("ModemTeardown: radioOn=%d point=heap-guard-reset", (int)Connectivity::isRadioPoweredOn());
      Log.warn("Nightly heap guard: freeHeap=%lu <= %lu before overnight ULTRA_LOW_POWER fallback - resetting for clean next-day start",
               freeHeap,
               ConnectivityPolicy::NIGHTLY_HEAP_RESET_THRESHOLD_BYTES);
      // Reset drops RAM - flush the diagnostics batch now, since this is
      // the only chance for this cycle's accumulated entries.
#if defined(ENABLE_DIAGNOSTICS_PUBLISH_MODE) && ENABLE_DIAGNOSTICS_PUBLISH_MODE
      PowerDiagnostics::flushDiagBatch();
#endif
      System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);
      return true;
    }

    if (freeHeap <= ConnectivityPolicy::NIGHTLY_HEAP_WARN_THRESHOLD_BYTES) {
      Log.warn("Nightly heap guard: freeHeap=%lu <= %lu before overnight ULTRA_LOW_POWER fallback - monitoring only",
               freeHeap,
               ConnectivityPolicy::NIGHTLY_HEAP_WARN_THRESHOLD_BYTES);
    }
  }
  return false;
}

struct WakeContext { bool pirWake, buttonWake, timerWake; const char *wakeReasonLabel; };
struct { bool preserveAfterNormalReturn = false; bool forceModemOff = false; int error = SYSTEM_ERROR_NONE; } standbyReturn;

bool effectiveStandby(bool useNetworkStandbyRequested) {
  bool preserveStandbyForThisPass = false;
#if HAL_PLATFORM_CELLULAR
  if (standbyReturn.preserveAfterNormalReturn &&
      (!useNetworkStandbyRequested || session.modemUnstable || Cellular.isOff())) {
    standbyReturn.preserveAfterNormalReturn = false;
  }
  preserveStandbyForThisPass = standbyReturn.preserveAfterNormalReturn;
#endif
  bool useNetworkStandbyEffective =
      useNetworkStandbyRequested &&
      (preserveStandbyForThisPass ||
       Observability::cycleStats().connect_result ==
           Observability::WakeCycleStats::ConnectResult::SUCCESS);
#if HAL_PLATFORM_CELLULAR
  useNetworkStandbyEffective = useNetworkStandbyEffective && !Cellular.isOff();
  if (session.modemUnstable || standbyReturn.forceModemOff) {
    useNetworkStandbyEffective = false;
  }
#else
  useNetworkStandbyEffective = false;
#endif
  return useNetworkStandbyEffective;
}

void hibernateAttempt(bool overnightFallbackSleep, int wakeInSeconds) {
#if PLATFORM_ID == PLATFORM_BORON
  if (overnightFallbackSleep) {
    time_t rtcNow = 0;
    if (shouldUseBoronRtcAlarmHibernate((uint32_t)wakeInSeconds, rtcNow)) {
      pinMode(WAKEUP_PIN, INPUT_PULLUP);
      if (digitalRead(WAKEUP_PIN) == HIGH) {
        if (clearAb1805StaleAlarmInterrupts()) {
          const time_t wakeTime = rtcNow + (time_t)wakeInSeconds;
          if (ab1805.interruptAtTime(wakeTime)) {
            setAppBreadcrumb(24); // BREADCRUMB_SLEEP_CONFIG_START (was stale literal 20, colliding with BREADCRUMB_REPORT_EXIT)
            // Fresh local configuration per sleep: Device OS move-assigns
            // without freeing the previous wake-source list, so a reused
            // long-lived object orphans that list on every cycle.
            SystemSleepConfiguration hibernateConfig;
            hibernateConfig.mode(SystemSleepMode::HIBERNATE)
              .gpio(WAKEUP_PIN, FALLING)
              .gpio(BUTTON_PIN, FALLING);

            const bool watchdogStopped = ab1805.stopWDT();
            pauseAwakeWatchdogForSleep("hibernate");
            if (!watchdogStopped) {
              Log.warn("SleepCall: watchdog-stop failed mode=HIBERNATE dur=%ds", wakeInSeconds);
            }

            // WO-2026-09-16 Step 2: HibernateCycle now owns the retained
            // hibernate-cycle fields; wakeTime itself is used above for
            // ab1805.interruptAtTime() only - nothing reads it back on
            // wake (confirmed dead as a retained field), so it is not
            // passed through here.
            HibernateCycle::armForSleep(rtcNow, (uint32_t)wakeInSeconds);

            Log.info("ModemTeardown: radioOn=%d point=hibernate", (int)Connectivity::isRadioPoweredOn());
            Log.info("Sleep: HIBERNATE reason=closed dur=%ds wakePin=%d", wakeInSeconds, (int)WAKEUP_PIN);
            PowerDiagnostics::logPowerState("pre-hibernate");
            thrashGuard.markProgress("SLEEP_ATTEMPT");
            Cloud::instance().logLedgerSleepState();
            setAppBreadcrumb(25); // BREADCRUMB_SLEEP_SYSTEM_CALL (was stale literal 21, colliding with BREADCRUMB_IDLE_ENTRY) - entering final pre-sleep sequence
            drainSerialBeforeSleep();
            setAppBreadcrumb(26); // BREADCRUMB_SLEEP_SERIAL_DRAIN_DONE
            // HIBERNATE may reset the MCU without returning - flush the
            // diagnostics batch now, since this is the only chance.
#if defined(ENABLE_DIAGNOSTICS_PUBLISH_MODE) && ENABLE_DIAGNOSTICS_PUBLISH_MODE
            PowerDiagnostics::flushDiagBatch();
            setAppBreadcrumb(27); // BREADCRUMB_SLEEP_DIAG_FLUSH_DONE
#endif
            setAppBreadcrumb(28); // BREADCRUMB_SLEEP_CALL_ENTER - immediately before System.sleep()
            const SystemSleepResult hibernateResult = System.sleep(hibernateConfig);
            AwakeCycles::recordSleepReturn(hibernateResult.error() == SYSTEM_ERROR_NONE);

            // HIBERNATE should reset the MCU. If we return here, treat it as failed and fall back.
            HibernateCycle::abandon();
            ab1805.resumeWDT();
            restoreAwakeWatchdogAfterWake("hibernate");
            Log.warn("HIBERNATE sleep failed err=%d - falling back to ULTRA_LOW_POWER",
                     (int)hibernateResult.error());
          } else {
            Log.warn("HIBERNATE bypass: failed to program AB1805 alarm - falling back to ULTRA_LOW_POWER");
          }
        } else {
          Log.warn("HIBERNATE bypass: failed stale interrupt cleanup - falling back to ULTRA_LOW_POWER");
        }
      } else {
        Log.warn("HIBERNATE bypass: wake pin held low before sleep - falling back to ULTRA_LOW_POWER");
      }
    }
  }
#endif
}

// Returns true when the caller must return (all sleep attempts failed).
bool sleepAndResume(int wakeInSeconds, bool useNetworkStandby, const char *sleepReason, WakeContext &wake) {
  const SensorWakeSource sensorWake = SensorManager::instance().activeSensor()->wakeSource();
  const bool sensorCanWake = (sensorWake.pin != PIN_INVALID);
  SystemSleepConfiguration ulpConfig;
  ulpConfig.mode(SystemSleepMode::ULTRA_LOW_POWER)
    .gpio(BUTTON_PIN, FALLING);    // Service button wake (active-low)
  if (sensorCanWake) ulpConfig.gpio(sensorWake.pin, sensorWake.edge);  // Sensor wake
    
  // Add network standby for occupancy mode to avoid reconnection delays
  // Note: Network standby is only supported/needed on cellular devices to prevent
  // carrier blacklisting. WiFi reconnection is already fast enough (~2-5s).
  if (useNetworkStandby) {
#if HAL_PLATFORM_CELLULAR
    // Keep cellular modem in low-power standby so we can quickly reconnect
    // when occupancy changes. This prevents 30-60s reconnection overhead and
    // potential carrier blacklisting from rapid disconnect/reconnect cycles.
    ulpConfig.network(NETWORK_INTERFACE_CELLULAR, SystemSleepNetworkFlag::INACTIVE_STANDBY);
#endif
  }
  
  ulpConfig.duration((uint32_t)wakeInSeconds * 1000UL);  // Timer-based wake at reporting boundary

  // Stop watchdog immediately before sleep (after config is fully built)
  // to minimize time between I2C transaction and sleep entry
  const bool watchdogStopped = ab1805.stopWDT();
  pauseAwakeWatchdogForSleep("ulp");
#if ENABLE_SLEEP_TRACE
  Log.info("SleepCall: standby=%d dur=%d wdt=%s gate=ok",
           useNetworkStandby ? 1 : 0,
           wakeInSeconds,
           watchdogStopped ? "off" : "err");
#else
  if (!watchdogStopped) {
    Log.warn("SleepCall: watchdog-stop failed standby=%d dur=%d",
             useNetworkStandby ? 1 : 0,
             wakeInSeconds);
  }
#endif
#if CONNECTIVITY_FAILSAFE_TEST_MODE
  Log.info("Sleep: ULP standby=%d reason=%s dur=%ds occ=%d soc=%.1f test=1",
           useNetworkStandby ? 1 : 0,
           sleepReason,
           wakeInSeconds,
           CurrentReadings::get_occupied() ? 1 : 0,
           (double)PowerManager::instance().soc());
#else
  Log.info("Sleep: ULP standby=%d reason=%s dur=%ds occ=%d soc=%.1f",
           useNetworkStandby ? 1 : 0,
           sleepReason,
           wakeInSeconds,
           CurrentReadings::get_occupied() ? 1 : 0,
           (double)PowerManager::instance().soc());
#endif

  thrashGuard.markProgress("SLEEP_ATTEMPT");
  PowerDiagnostics::logPowerState("pre-sleep-ulp");
  const unsigned long sleepCallStartMs = millis();
  Cloud::instance().logLedgerSleepState();
  setAppBreadcrumb(25); // BREADCRUMB_SLEEP_SYSTEM_CALL (was stale literal 21, colliding with BREADCRUMB_IDLE_ENTRY) - entering final pre-sleep sequence
  drainSerialBeforeSleep();
  setAppBreadcrumb(26); // BREADCRUMB_SLEEP_SERIAL_DRAIN_DONE
  setAppBreadcrumb(28); // BREADCRUMB_SLEEP_CALL_ENTER - immediately before System.sleep()
  SystemSleepResult result = System.sleep(ulpConfig);
  AwakeCycles::recordSleepReturn(result.error() == SYSTEM_ERROR_NONE);
  const unsigned long sleepElapsedMs = millis() - sleepCallStartMs;
  pin_t wakePin = result.wakeupPin();
  const char *wakeReturnReason = (result.error() != SYSTEM_ERROR_NONE) ? "ERROR" :
      ((wakePin == BUTTON_PIN) ? "BUTTON" : ((wakePin == sensorWake.pin) ? "PIR" : "TIMER"));

#if HAL_PLATFORM_CELLULAR
  if (useNetworkStandby) {
    if (result.error() == SYSTEM_ERROR_NONE) {
      standbyReturn.preserveAfterNormalReturn = true;
      standbyReturn.forceModemOff = false;
      standbyReturn.error = SYSTEM_ERROR_NONE;
      if (SystemConfig::get_verboseMode()) {
        Log.info("SLEEP: standby sleep returned normally; preserving modem standby");
      }
    } else {
      standbyReturn.preserveAfterNormalReturn = false;
      standbyReturn.forceModemOff = true;
      standbyReturn.error = (int)result.error();
      Log.warn("SLEEP: standby sleep failed; forcing modem-off reason=sleep_error err=%d",
               standbyReturn.error);
    }
  } else {
    standbyReturn.preserveAfterNormalReturn = false;
    standbyReturn.forceModemOff = false;
    standbyReturn.error = SYSTEM_ERROR_NONE;
  }
#endif

  // If Device OS rejects the sleep configuration, System.sleep() returns
  // immediately and the wake pin will be invalid (65535). Without handling
  // that error, the state machine will interpret it as a timer wake and can
  // thrash in a tight loop.
  if (result.error() != SYSTEM_ERROR_NONE) {
    thrashGuard.markProgress("SLEEP_RETURNED");
    Log.error("ULTRA_LOW_POWER sleep failed err=%d (wakeIn=%d sec, button=%d pir=%d) - falling back to STOP", (int)result.error(), wakeInSeconds, (int)BUTTON_PIN, (int)sensorWake.pin);
    RecoveryState::raiseAlert(16);

   // STOP generally supports a wider set of wake pins on some platforms.
    setAppBreadcrumb(24); // BREADCRUMB_SLEEP_CONFIG_START (was stale literal 20, colliding with BREADCRUMB_REPORT_EXIT)
    SystemSleepConfiguration stopConfig;
    stopConfig.mode(SystemSleepMode::STOP)
      .gpio(BUTTON_PIN, FALLING);
    if (sensorCanWake) stopConfig.gpio(sensorWake.pin, sensorWake.edge);
    stopConfig.duration((uint32_t)wakeInSeconds * 1000UL);
    PowerDiagnostics::logPowerState("pre-sleep-stop-fallback");
    Cloud::instance().logLedgerSleepState();
    setAppBreadcrumb(25); // BREADCRUMB_SLEEP_SYSTEM_CALL (was stale literal 21, colliding with BREADCRUMB_IDLE_ENTRY) - entering final pre-sleep sequence
    drainSerialBeforeSleep();
    setAppBreadcrumb(26); // BREADCRUMB_SLEEP_SERIAL_DRAIN_DONE
    setAppBreadcrumb(28); // BREADCRUMB_SLEEP_CALL_ENTER - immediately before System.sleep()
    result = System.sleep(stopConfig);
    AwakeCycles::recordSleepReturn(result.error() == SYSTEM_ERROR_NONE);

    if (result.error() != SYSTEM_ERROR_NONE) {
      Log.error("STOP sleep fallback failed err=%d (wakeIn=%d sec) - using timer-only STOP sleep", (int)result.error(), wakeInSeconds);
      setAppBreadcrumb(24); // BREADCRUMB_SLEEP_CONFIG_START
      SystemSleepConfiguration stopTimerConfig;
      stopTimerConfig.mode(SystemSleepMode::STOP)
        .duration((uint32_t)wakeInSeconds * 1000UL);
      PowerDiagnostics::logPowerState("pre-sleep-stop-timer-only");
      Cloud::instance().logLedgerSleepState();
      setAppBreadcrumb(25); // BREADCRUMB_SLEEP_SYSTEM_CALL - entering final pre-sleep sequence
      drainSerialBeforeSleep();
      setAppBreadcrumb(26); // BREADCRUMB_SLEEP_SERIAL_DRAIN_DONE
      setAppBreadcrumb(28); // BREADCRUMB_SLEEP_CALL_ENTER - immediately before System.sleep()
      result = System.sleep(stopTimerConfig);
      AwakeCycles::recordSleepReturn(result.error() == SYSTEM_ERROR_NONE);

      if (result.error() != SYSTEM_ERROR_NONE) {
        // All sleep modes failed - this indicates a device state corruption
        // that requires immediate reset (alert 16 already raised on first failure).
        Log.info("ModemTeardown: radioOn=%d point=sleep-attempts-failed-reset", (int)Connectivity::isRadioPoweredOn());
        Log.error("All sleep attempts failed err=%d - immediate reset required", (int)result.error());
        ab1805.resumeWDT();

        // Brief delay to allow log output to flush before reset
        delay(2000);

        // Reset device to clear corrupted state
        Log.info("Resetting device to clear sleep failure state");
        // Reset drops RAM - flush the diagnostics batch now, since this is
        // the only chance for this cycle's cascade of sleep-attempt entries.
#if defined(ENABLE_DIAGNOSTICS_PUBLISH_MODE) && ENABLE_DIAGNOSTICS_PUBLISH_MODE
        PowerDiagnostics::flushDiagBatch();
#endif
        System.reset(RESET_CAUSE_SLEEP_ATTEMPTS_EXHAUSTED);
        
        // Should never reach here, but set ERROR_STATE as fallback
        transitionTo(ERROR_STATE, "sleep-all-attempts-failed");
        return true;
      }
    }
  }
  
  // Clear any pending interrupts on wake pins after sleep to ensure clean state
  // This is critical for proper interrupt handling after wake
  pinMode(BUTTON_PIN, INPUT);    // Reset pin mode
  if (sensorCanWake) pinMode(sensorWake.pin, INPUT);  // Reset pin mode
  
  // Resume hardware watchdog immediately after wake
  ab1805.resumeWDT();
  restoreAwakeWatchdogAfterWake("ulp");

  // Mark progress immediately after wake to reset ThrashGuard timer
  thrashGuard.markProgress("WAKE_FROM_SLEEP");
  setAppBreadcrumb(4);

  // Single convergence point for the ULP/STOP-fallback/STOP-timer-only sleep
  // cascade - whichever attempt actually succeeded lands here, so this flushes
  // the whole cycle's diagnostics batch exactly once regardless of how many
  // fallback attempts fired. (Hibernate and the total-sleep-failure reset are
  // flushed separately at their own non-returning call sites, above.)
#if defined(ENABLE_DIAGNOSTICS_PUBLISH_MODE) && ENABLE_DIAGNOSTICS_PUBLISH_MODE
  PowerDiagnostics::flushDiagBatch();
#endif

  if (SystemConfig::get_serialConnected() || (ALLOW_BLOCKING_SERIAL_WAITS != 0)) {
    // Re-initialize USB serial after wake and give the host a bounded chance
    // to re-enumerate before we continue through another short wake cycle.
    Serial.begin(9600);
    delay(ConnectivityPolicy::DEBUG_SERIAL_REENUM_DELAY_MS);

    unsigned long serialWaitStart = millis();
    while (!Serial.isConnected() && (millis() - serialWaitStart) < ConnectivityPolicy::DEBUG_SERIAL_WAIT_TIMEOUT_MS) {
      thrashGuard.markProgress("SERIAL_WAIT");
      serviceAwakeWatchdog();
      Particle.process();
      delay(ConnectivityPolicy::DEBUG_SERIAL_WAIT_POLL_DELAY_MS);
    }

    if (Serial.isConnected()) {
      delay(ConnectivityPolicy::DEBUG_SERIAL_POST_CONNECT_DELAY_MS);
    } else {
      Log.warn("Serial did not reconnect within 30s - continuing without serial");
    }
  }

  Log.info("WakeReturn: elapsed=%lu reason=%s",
           sleepElapsedMs,
           wakeReturnReason);

#if Wiring_Watchdog
#if ENABLE_SLEEP_TRACE
  Log.info("AppWDT: active ctx=ulp armed=%d",
           Watchdog.started() ? 1 : 0);
#else
  if (!Watchdog.started()) {
    Log.warn("AppWDT: active abnormal ctx=ulp armed=0");
  }
#endif
#endif

  // Re-attach user button interrupt after sleep (sleep may have detached it)
  attachInterrupt(BUTTON_PIN, userSwitchISR, FALLING);
  userSwitchDetected = false;  // Clear any spurious flag

  // When both GPIO and timer wake are configured, the wake source detection can be
  // ambiguous. The explicit approach: if neither GPIO pin woke us, it's the timer.
  bool pirWake = sensorCanWake && (wakePin == sensorWake.pin);
  bool buttonWake = (wakePin == BUTTON_PIN);
  bool timerWake = !pirWake && !buttonWake;  // If no GPIO woke us, it's the timer
  const char *wakeReasonLabel = buttonWake ? "BUTTON" : (pirWake ? "PIR" : "TIMER");
  wake = {pirWake, buttonWake, timerWake, wakeReasonLabel};

  // Observability: start of a new wake cycle (ULP return path).
  Observability::cycleStats().resetOnWake(millis());

  // Battery must be sampled before any later report/connect logic turns on the
  // modem, so mark the first post-wake sample for fuel-gauge stabilization now.
  measure.noteWakeFromLowPowerSleep();
  PowerDiagnostics::logPowerState("post-wake");
  return false;
}

} // namespace

// NOTE:
// This file was split from StateHandlers.cpp as a mechanical refactor.
// No behavioral changes were made.

/**
 * @brief SLEEPING_STATE: deep sleep between reporting intervals.
 * ...
 */
// Firmware-update record owned by Generalized-Core-Counter.cpp (WO-2026-10-01-001 A).
extern volatile bool firmwareUpdateInProgress;

void handleSleepingState() {
  setLoopStage(LOOP_STAGE_SLEEP_PREP);

  // Zero-gated (not enteredState-gated): two of this function's real exit
  // paths transition SLEEPING_STATE -> SLEEPING_STATE
  // (sleep-timer-occupied-suppress-report, sleep-pir-return-to-sleep) - a
  // real cycle ends and a new one begins, but the FSM `state` value doesn't
  // change, so an enteredState-gated reset would never fire for the new
  // cycle. This mirrors the existing cloudSyncStartMs pattern below.
  maybeStartSleepPrepSpan(retainedLoopForensics.sleepPrepSpanStartMillis, millis());

  bool enteredState = (state != oldState);

  if (enteredState) {
    setAppBreadcrumb(3);
    publishStateTransition();
    cloudGate.operationsCompleteLogged = false;  // Reset flag on state entry
    cloudGate.gateWasBlocking = false;
    cloudGate.lastGateStatusLogMs = 0;
    cloudGate.lastGateBlockerMask = kGateBlockerUninitialized;
    cloudGate.lastGateBlockerBeforeRelease = 0;
    cloudGate.gateReleaseReasonKnown = false;
    SleepTeardown::enter();
  }

  // Determine if we will use *effective* network standby for the upcoming sleep.
  // Only cellular platforms actually apply standby; on WiFi, treat as disabled
  // so disconnect logic doesn't wait for a radio state that won't change.
  // Standby is only useful after a successful cloud session in the current
  // wake cycle; if connect never succeeded, preserving the modem state buys
  // nothing and can carry a bad NCP state into the next wake.
  bool useNetworkStandbyRequested =
      (PowerManager::instance().effectiveConnectionMode() == SystemConfig::INTERMITTENT_KEEP_ALIVE) &&
      isWithinOpenHours();
  bool useNetworkStandbyEffective = effectiveStandby(useNetworkStandbyRequested);

  // If a ledger update (or time progression) moves the park into OPEN hours
  // while we are in SLEEPING_STATE, abort sleeping immediately in CONNECTED
  // mode so we stay awake/connected and resume counting.
  //
  // WO-2026-09-19 Step 3b: Clock::openness() != Closed, not
  // isTimeValid()+isWithinOpenHours(). Unknown is treated as Open here,
  // consistent with every other CONNECTED-mode site this step converts:
  // abort the sleep and stay awake/connected so a resync can occur, rather
  // than committing to a sleep the device can't actually justify.
  if (PowerManager::instance().effectiveConnectionMode() == SystemConfig::CONNECTED && Clock::openness() != Clock::Openness::Closed) {
    ensureSensorEnabled("SLEEP abort: CONNECTED+OPEN");
    cloudGate.cloudSyncStartMs = 0;
    transitionTo(IDLE_STATE, "sleep-abort-open-hours");
    return;
  }

  // If we are connected and the publish queue is not yet in a sleep-safe
  // state (events queued or a publish in progress), we will wait below in the
  // unified cloud-operations gate (with a timeout budget) so we never wait
  // indefinitely on a stuck publish.

  // ********** Cloud Operations Sync Prerequisites (with timeout budget) **********
  // Before disconnecting, ensure critical operations complete:
  //   1) Publish queue drained
  //   2) Ledgers synced from cloud
  //   3) OTA updates checked
  //   4) Webhook response received
  // Use a timeout budget to prevent battery drain if operations hang.
  // NOTE: This code is non-blocking at application level (Particle.process() called)
  // but blocks state transition until prerequisites complete or timeout.

  // ********** Non-blocking disconnect + modem power-down **********
  // Device OS already manages the asynchronous cloud session teardown once
  // Particle.disconnect() has been requested. Here, we keep application
  // logic minimal: request cloud disconnect and radio-off once, then wait
  // (bounded) until both cloud and modem are actually off before sleeping.
#if Wiring_WiFi
  static bool wifiOffGuardActive = false;
  static unsigned long wifiOffGuardStartMs = 0;
  static unsigned long wifiOffGuardLastRetryMs = 0;
  static uint8_t wifiOffGuardRetries = 0;
#endif

  if (enteredState) {
    cloudGate.cloudSyncBudgetMs = 0;
    cloudGate.cloudSyncMaxQueueDepth = 0;
    cloudGate.lastQueueDepth = 0xFFFF;
#if Wiring_WiFi
    wifiOffGuardActive = false;
    wifiOffGuardStartMs = 0;
    wifiOffGuardLastRetryMs = 0;
    wifiOffGuardRetries = 0;
#endif
  }

  SleepTeardown::guardLostTimestamp();

  // Particle's reference pattern (STATE_SLEEP's first check): a firmware
  // download in flight outranks sleep. Sit here on every pass until no
  // teardown has been requested, so a `begin` delivered between loop passes is
  // caught before the cloud disconnect / radio-off requests below, not after
  // the transfer has already been lost (WO-2026-10-01-001 item A).
  if (!SleepTeardown::disconnectRequested() && firmwareUpdateInProgress) {
    cloudGate.cloudSyncStartMs = 0; // leaving mid-gate: a later return starts its gate cleanly
    transitionTo(FIRMWARE_UPDATE_STATE, "firmware update in progress");
    return;
  }

  // An occupancy change latched after the last wake decision (for example by
  // the main-loop handler while already SLEEPING) reports before any
  // sleep-or-suppress decision below can commit to sleep. Once this span has
  // requested teardown it stays latched and reports on the first pass after
  // the wake, so Report/Connect never starts while the disconnect is in flight.
  if (session.occupancyChangeTriggered && !SleepTeardown::disconnectRequested()) {
    cloudGate.cloudSyncStartMs = 0; // leaving mid-gate: a later return starts its gate cleanly
    transitionTo(REPORTING_STATE, "occupancy change pending");
    return;
  }

  if (cloudGatePass(useNetworkStandbyEffective)) return;

  if (SleepTeardown::run(useNetworkStandbyRequested, useNetworkStandbyEffective, standbyReturn.forceModemOff, standbyReturn.error) != SleepTeardown::Result::READY) { return; }

  setAppBreadcrumb(23); // BREADCRUMB_SLEEP_GATE_DONE (was stale literal 19, colliding with BREADCRUMB_REPORT_POST_LEDGER)

  SleepPlan plan;
  if (sleepPlan(plan)) return;
  int wakeInSeconds = plan.wakeInSeconds;
  const char *sleepReason = plan.sleepReason;
  const bool overnightFallbackSleep = plan.overnightFallbackSleep;

  hibernateAttempt(overnightFallbackSleep, wakeInSeconds);

  if (nightlyHeapGuard(overnightFallbackSleep)) return;

  // The sleep configuration is built fresh as a local at each sleep site
  // (below), so GPIO selections cannot accumulate across calls.
  setAppBreadcrumb(24); // BREADCRUMB_SLEEP_CONFIG_START (was stale literal 20, colliding with BREADCRUMB_REPORT_EXIT)

  // ********** WORKING SLEEP CONFIGURATION **********
  // Based on Connected-Counter-Next which uses system timer + GPIO wake.
  // NO AB1805 alarms - AB1805 is only used for watchdog + RTC time sync.
  // This approach works reliably on Photon2!

  // In INTERMITTENT_KEEP_ALIVE mode during open hours on *cellular* devices,
  // use network standby to avoid rapid reconnects that could trigger carrier
  // blacklisting. On WiFi devices, standby is not applied.
  bool useNetworkStandby = useNetworkStandbyEffective;

  // -----------------------------------------------------------------------
  // Minimal end-of-cycle observability (one compact line per wake cycle)
  // -----------------------------------------------------------------------
  {
    measure.batteryState(BatterySampleContext::PreSleep);
    const uint16_t qDepth = (uint16_t)PublishQueuePosix::instance().getNumEvents();
    const float soc = PowerManager::instance().soc();
    const uint16_t socTenths = (soc >= 0.0f && soc <= 100.0f) ? (uint16_t)(soc * 10.0f + 0.5f) : 0xFFFF;
    const uint8_t battState = PowerManager::instance().batteryState();
    const uint8_t isCharging = (battState == 0) ? 0xFF : ((battState == 2) ? 1 : 0); // 0=Unknown, 2=Charging

    Observability::cycleStats().finalizeBeforeSleep(
        millis(),
        Particle.connected(),
        Connectivity::isRadioPoweredOn(),
        qDepth,
        socTenths,
        isCharging,
        SystemConfig::get_lastConnection());

    // Invariants (log-only): detect regressions without affecting behavior.
    // Ceiling is derived from existing budgets and includes firmware update time.
    const unsigned long awakeCeilingMs =
        (unsigned long)ConnectivityPolicy::CONNECT_BUDGET_DEEP_MS +
        (unsigned long)ConnectivityPolicy::FIRMWARE_UPDATE_MAX_MS +
      (unsigned long)ConnectivityPolicy::OUTPUT_LEDGER_SYNC_TIMEOUT_MS +
        (unsigned long)ConnectivityPolicy::DISCONNECT_MODEM_DEFAULT_SEC * 1000UL +
        30000UL; // slack

    // Debug builds may intentionally stay awake longer for diagnostics.
    const bool allowCeilingOver = (DEV_BUILD != 0);
    if (!allowCeilingOver && Observability::cycleStats().total_awake_ms > awakeCeilingMs) {
      Log.warn("INV: awake_ms=%lu > ceiling_ms=%lu", (unsigned long)Observability::cycleStats().total_awake_ms,
               (unsigned long)awakeCeilingMs);
    }

    if (!useNetworkStandby) {
      if (Particle.connected() || Connectivity::isRadioPoweredOn()) {
        Log.warn("INV: entering sleep with cloud=%d radioOn=%d (standby=0)",
                 (int)Particle.connected(), (int)Connectivity::isRadioPoweredOn());
      }
    } else {
      if (Particle.connected()) {
        Log.warn("INV: entering sleep with cloud still connected (standby=1)");
      }
    }

    // Connect duration should be within the selected budget when an attempt was made.
    if (Observability::cycleStats().connect_attempt_type != Observability::WakeCycleStats::ConnectAttemptType::NONE &&
        Observability::cycleStats().connect_budget_ms > 0 &&
        Observability::cycleStats().connect_duration_ms > (Observability::cycleStats().connect_budget_ms + 1000UL)) {
      Log.warn("INV: connect_ms=%lu > budget_ms=%lu", (unsigned long)Observability::cycleStats().connect_duration_ms,
               (unsigned long)Observability::cycleStats().connect_budget_ms);
    }

    // One compact line for field parsing.
    if (SystemConfig::get_verboseMode()) {
      Log.info(
        "CYCLE end awake=%lums conn=%s/%s/%lums svc=%lums td=%lums q=%d/%d/%d soc=%.1f%% chg=%d lastOk=%ld",
          (unsigned long)Observability::cycleStats().total_awake_ms,
          Observability::toString(Observability::cycleStats().connect_attempt_type),
          Observability::toString(Observability::cycleStats().connect_result),
          (unsigned long)Observability::cycleStats().connect_duration_ms,
          (unsigned long)Observability::cycleStats().service_duration_ms,
          (unsigned long)Observability::cycleStats().teardown_duration_ms,
        (int)(Observability::cycleStats().publish_queue_depth_before_connect == 0xFFFF ? -1 : (int)Observability::cycleStats().publish_queue_depth_before_connect),
        (int)(Observability::cycleStats().publish_queue_depth_after_connect == 0xFFFF ? -1 : (int)Observability::cycleStats().publish_queue_depth_after_connect),
        (int)(Observability::cycleStats().publish_queue_depth_before_sleep == 0xFFFF ? -1 : (int)Observability::cycleStats().publish_queue_depth_before_sleep),
          (double)(Observability::cycleStats().battery_soc_tenths == 0xFFFF ? -1.0 : ((double)Observability::cycleStats().battery_soc_tenths / 10.0)),
        (int)(Observability::cycleStats().is_charging == 0xFF ? -1 : (int)Observability::cycleStats().is_charging),
          (long)Observability::cycleStats().last_success_epoch);
    }
  }

  // On WiFi platforms, ensure the radio is actually OFF before entering
  // ULTRA_LOW_POWER. If the cloud/radio was left on, the device can flash
  // green briefly on wake as WiFi attempts to reconnect.
#if Wiring_WiFi
  if (!useNetworkStandby) {
    if (Particle.connected()) {
      Connectivity::requestCloudDisconnectOnly();
    }
    if (WiFi.isOn()) {
      constexpr unsigned long WIFI_OFF_GUARD_MAX_MS = 2000UL;
      constexpr unsigned long WIFI_OFF_GUARD_RETRY_MS = 400UL;
      const unsigned long nowMs = millis();

      if (!wifiOffGuardActive) {
        wifiOffGuardActive = true;
        wifiOffGuardStartMs = nowMs;
        wifiOffGuardLastRetryMs = 0;
        wifiOffGuardRetries = 0;
        Log.info("SLEEP: WiFi still on - starting bounded radio-off guard");
      }

      if (wifiOffGuardLastRetryMs == 0 || (nowMs - wifiOffGuardLastRetryMs) >= WIFI_OFF_GUARD_RETRY_MS) {
        Connectivity::requestRadioPowerOff();
        wifiOffGuardLastRetryMs = nowMs;
        if (wifiOffGuardRetries < 255) {
          wifiOffGuardRetries++;
        }
      }

      unsigned long guardElapsedMs = nowMs - wifiOffGuardStartMs;
      if (guardElapsedMs <= WIFI_OFF_GUARD_MAX_MS) {
        thrashGuard.markProgress("WIFI_OFF_GUARD");
        Particle.process();
        return;
      }

      Log.warn("SLEEP: WiFi radio remained on after %lu ms (%u retries) - proceeding to sleep",
               guardElapsedMs,
               wifiOffGuardRetries);
      wifiOffGuardActive = false;
      wifiOffGuardStartMs = 0;
      wifiOffGuardLastRetryMs = 0;
      wifiOffGuardRetries = 0;
    } else if (wifiOffGuardActive) {
      unsigned long guardElapsedMs = millis() - wifiOffGuardStartMs;
      if (SystemConfig::get_verboseMode()) {
        Log.info("SLEEP: WiFi radio powered off before sleep (%lu ms)", guardElapsedMs);
      }
      wifiOffGuardActive = false;
      wifiOffGuardStartMs = 0;
      wifiOffGuardLastRetryMs = 0;
      wifiOffGuardRetries = 0;
    }
  }
#endif
  
  WakeContext wake;
  if (sleepAndResume(wakeInSeconds, useNetworkStandby, sleepReason, wake)) return;
  bool pirWake = wake.pirWake;
  bool buttonWake = wake.buttonWake;
  bool timerWake = wake.timerWake;
  const char *wakeReasonLabel = wake.wakeReasonLabel;
  
  // LED will be turned on with proper timeout by PIR wake processing below
  // Don't turn it on here without timeout as it causes false LED timeout detection

  if (buttonWake) {
    // User button wake: queue a fresh service report, then force immediate connect.
    SensorManager::instance().onExitSleep();
    measure.batteryState(BatterySampleContext::PostWake);
    logWakeSummary(wakeReasonLabel);
    session.serviceRequestTriggered = true;
    transitionTo(REPORTING_STATE, "sleep-wake-button");
    return;
  } else {
    // In this state the device was awoken for hourly reporting or PIR
    // Re-enable sensors only if within opening hours; otherwise they
    // remain powered down to minimize sleep current.
    if (isWithinOpenHours()) {
      // If the sensor stack was never initialized (for example, device booted
      // while closed and remained asleep), onExitSleep() will be a no-op.
      // Ensure we (re)initialize the active sensor when entering open hours.
      SensorManager::instance().onExitSleep();
      if (!SensorManager::instance().isSensorReady()) {
        SensorManager::instance().initializeFromConfig();
      }

      // Refresh battery state immediately after wake while the radio is still off.
      measure.batteryState(BatterySampleContext::PostWake);

      // If KEEP_ALIVE was previously disabled due to low battery, re-evaluate
      // battery policy immediately after the post-wake sample so this wake's
      // occupancy and sleep decisions do not use a stale downgraded mode.
      // WO-2026-09-21 Step 4 (corrected same day): evaluate() then commit() -
      // one of the four deliberate policy-application sites.
      if (PowerConfig::get_lowBatteryMode()) {
        const float soc = PowerManager::instance().soc();
        float vcell = 0.0f;
        const SensorManager::VcellSampleState vcellState =
            SensorManager::instance().cachedBatteryVoltageState(vcell);
        const BatteryHealth::SocTrust trust = SensorManager::instance().cachedSocTrust();
        BatteryAuthority::commit(
            BatteryAuthority::evaluate(soc, vcellState, vcell, trust, BatteryAuthority::currentTier()),
            soc);
      }

      // In CONNECTED operating mode, the device should reconnect at the
      // start of open hours so it can resume normal connected behavior.
      if (PowerManager::instance().effectiveConnectionMode() == SystemConfig::CONNECTED && !Particle.connected()) {
        logWakeSummary(wakeReasonLabel);
        transitionTo(CONNECTING_STATE, "sleep-open-hours-reconnect");
        return;
      }
    }

    // Check if LED timeout expired FIRST (before processing new PIR wake)
    // This ensures we don't immediately undo state changes from PIR processing
    if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY && signalLEDTimeRemaining() == 0 && signalLEDStatus()) {
      // LED timeout expired - debounce period elapsed without motion
      const bool reportNow = reportsOccupancyChangesNow();
      const OccupancyCloseResult closeResult = closeOccupancySessionSafely("sleep");
      signalLED(false);  // Turn off LED
      if (closeResult.valid) {
        logUnoccupiedEvent("debounce", closeResult.sessionSeconds, closeResult.totalSeconds, reportNow);
      }
      
      // Match the rest of the occupancy state machine: only KEEP_ALIVE and
      // CONNECTED force an immediate report/connect on occupancy transitions.
      if (reportNow && !closeResult.stillOpen) {
        session.occupancyChangeTriggered = true;
        transitionTo(REPORTING_STATE, "sleep-occupancy-debounce-report");
        return;
      }
    }

    // Process PIR wake events (after checking LED timeout above)
    // If this wake was caused by the PIR interrupt, synthesize a single
    // detection event so that the motion that woke the device is counted
    // even if the ISR flag did not survive ULTRA_LOW_POWER sleep.
    if (pirWake) {
      if (SystemConfig::get_sensorMode() == SystemConfig::COUNTING) {
        CurrentReadings::set_hourlyCount(CurrentReadings::get_hourlyCount() + 1);
        CurrentReadings::set_dailyCount(CurrentReadings::get_dailyCount() + 1);
        // WO-2026-08-29-002 item 8: lastCountTime has no consumers anywhere
        // in this codebase (write-only telemetry field) - gating on
        // isClockTrusted() changes only the recorded value, never control
        // flow.
        CurrentReadings::set_lastCountTime(isClockTrusted() ? Time.now() : 0);
        signalLED(true, 1000);  // Flash for 1 second
        Log.info("Count detected from PIR wake - Hourly: %d, Daily: %d",
                 CurrentReadings::get_hourlyCount(), CurrentReadings::get_dailyCount());
        if (Serial && Serial.isConnected()) {
          Serial.printf("Count detected from PIR wake - Hourly: %d, Daily: %d\r\n",
                        CurrentReadings::get_hourlyCount(), CurrentReadings::get_dailyCount());
        }
      } else if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY) {
        // Occupancy mode: PIR wakes are expected behavior, not thrashing
        thrashGuard.markProgress("PIR_WAKE_OCCUPANCY");

        // If KEEP_ALIVE was temporarily disabled for low battery, refresh the
        // battery policy on wake so recovered power can restore the intended
        // occupancy behavior before we decide whether to report or sleep again.
        // WO-2026-09-21 Step 4 (corrected same day): evaluate() then commit() -
        // one of the four deliberate policy-application sites.
        if (PowerConfig::get_lowBatteryMode()) {
          const float soc = PowerManager::instance().soc();
          float vcell = 0.0f;
          const SensorManager::VcellSampleState vcellState =
              SensorManager::instance().cachedBatteryVoltageState(vcell);
          const BatteryHealth::SocTrust trust = SensorManager::instance().cachedSocTrust();
          BatteryAuthority::commit(
              BatteryAuthority::evaluate(soc, vcellState, vcell, trust, BatteryAuthority::currentTier()),
              soc);
        }
        
        if (!CurrentReadings::get_occupied()) {
          CurrentReadings::set_occupied(true);
          // WO-2026-08-29-002 item 8: deliberately NOT gated on
          // isClockTrusted(). Unlike lastCountTime, occupancyStartTime's 0
          // value is a load-bearing sentinel for two consumers:
          // MyPersistentData.cpp's validate() treats occupied=true with
          // occupancyStartTime==0 as corrupt state and forces occupied=false
          // (a real state-machine side effect, not just a recorded value),
          // and State_Modes.cpp computes `Time.now() - occupancyStartTime`
          // for a logged duration, which would become a multi-decade garbage
          // span if this were 0 while occupied. Writing Time.now() here even
          // when untrusted preserves existing behavior; see the
          // Implementation Report for the Chief Engineer's review.
          CurrentReadings::set_occupancyStartTime(Time.now());
          // Treat the PIR wake as an occupancy event for debounce purposes.
          // If lastOccupancyEvent is left at 0, the debounce logic that uses
          // millis()-based timing will immediately expire and mark UNOCCUPIED.
          CurrentReadings::set_lastOccupancyEvent(millis());

          // Keep LED on for the debounce window (sensor.setting1 is milliseconds).
          uint32_t debounceMs = SystemConfig::SensorSettings::get_sensorSetting1();
          if (debounceMs == 0) {
            debounceMs = Config::occupancyDebounceMsForRuntime();
          }
          signalLED(true, debounceMs);
          const bool reportNow = reportsOccupancyChangesNow();
          logOccupiedEvent("pir-wake", debounceMs / 1000UL, reportNow);
          
          // Match the rest of the occupancy state machine: only KEEP_ALIVE and
          // CONNECTED force an immediate report/connect on occupancy transitions.
          if (reportNow) {
            session.occupancyChangeTriggered = true;
            setAppBreadcrumb(5);
            transitionTo(REPORTING_STATE, "sleep-pir-occupancy-report");
            return;
          }
        } else {
          // Already occupied - motion detected during debounce, restart timer
          uint32_t debounceMs = SystemConfig::SensorSettings::get_sensorSetting1();
          if (debounceMs == 0) {
            debounceMs = Config::occupancyDebounceMsForRuntime();
          }
          signalLED(true, debounceMs);  // Restart LED timer
          logOccupiedEvent("pir-wake", debounceMs / 1000UL, false, true);
        }
        CurrentReadings::set_lastOccupancyEvent(millis());
      }
    }

    logWakeSummary(wakeReasonLabel);

    // For COUNTING mode: Turn off LED if flash completed during sleep
    if (SystemConfig::get_sensorMode() == SystemConfig::COUNTING && signalLEDTimeRemaining() == 0 && signalLEDStatus()) {
      signalLED(false);
    }

    // Timer wake = scheduled report. No checks, no gates.
    // We trust that the system timer woke us at the correct boundary.
    if (timerWake) {
      // In OCCUPANCY + INTERMITTENT_KEEP_ALIVE mode, suppress periodic reports
      // while occupied UNLESS one is due for this reporting interval
      // (WO-2026-10-02-002: report every hour, occupied or not).
      if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY && CurrentReadings::get_occupied() &&
          PowerManager::instance().effectiveConnectionMode() == SystemConfig::INTERMITTENT_KEEP_ALIVE && !reportDueThisInterval()) {
        transitionTo(SLEEPING_STATE, "sleep-timer-occupied-suppress-report");
        return;
      }

      transitionTo(REPORTING_STATE, "sleep-timer-report");
      return;
    }

    // For PIR wakes, check if reporting is also due (opportunistic reporting)
    // WO-2026-09-19 Step 3b: Clock::openness() != Closed, not
    // isTimeValid()+isWithinOpenHours() - Unknown is treated as Open, same
    // reasoning as the scheduled-reporting site in State_Idle.cpp: an
    // opportunistic report is a resync opportunity, not a commitment.
    if (pirWake && Clock::openness() != Clock::Openness::Closed) {
      // In OCCUPANCY + INTERMITTENT_KEEP_ALIVE mode, a PIR hit while occupied
      // reports only when one is due for this reporting interval
      // (WO-2026-10-02-002); otherwise it only resets debounce.
      if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY && CurrentReadings::get_occupied() &&
          PowerManager::instance().effectiveConnectionMode() == SystemConfig::INTERMITTENT_KEEP_ALIVE) {
        if (reportDueThisInterval()) { transitionTo(REPORTING_STATE, "sleep-pir-overdue-report"); return; }
      } else {
      uint16_t intervalSec = Config::reportingIntervalSecForRuntime();

      time_t now = Time.now();
      time_t lastReport = SystemConfig::get_lastReport();
      if (lastReport > 0 && (now - lastReport) >= intervalSec) {
        transitionTo(REPORTING_STATE, "sleep-pir-overdue-report");
        return;
      }
      }
    }

    // If PIR woke us in INTERMITTENT or DISCONNECTED mode and no report is needed,
    // return immediately to sleep. This check comes AFTER opportunistic reporting
    // so overdue reports are not missed.
    if (pirWake && PowerManager::instance().effectiveConnectionMode() != SystemConfig::CONNECTED) {
      transitionTo(SLEEPING_STATE, "sleep-pir-return-to-sleep");
      return;
    }

    transitionTo(IDLE_STATE, "sleep-wake-idle");
  }
}
