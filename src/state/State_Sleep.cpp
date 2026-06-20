#include "BuildProfile.h"
#include "state/State_Common.h"
#include "../Config.h"
#include "cloud/Cloud.h"
#include "power/Connectivity.h"
#include "power/ConnectivityPolicy.h"
#include "power/PowerDiagnostics.h"
#include "LocalTimeRK.h"
#include "MyPersistentData.h"
#include "PublishQueuePosixRK.h"
#include "sensors/SensorManager.h"
#include "device_pinout.h"
#include "sensors/SensorDefinitions.h"
#include "AB1805_RK.h"
#include "observability/WakeCycleStats.h"
#include "ThrashGuard.h"

namespace {

constexpr unsigned long MODEM_UNSTABLE_SLOW_TEARDOWN_MS = 10000UL;
constexpr unsigned long MODEM_UNSTABLE_RECOVERY_TEARDOWN_MS = 5000UL;

enum ModemUnstableReason : uint8_t {
  MODEM_UNSTABLE_REASON_NONE = 0,
  MODEM_UNSTABLE_REASON_SLOW_TEARDOWN = 1,
  MODEM_UNSTABLE_REASON_CONNECT_TIMEOUT = 2,
};

const char *connectionModeLabel(ConnectionMode mode) {
  switch (mode) {
  case CONNECTED:
    return "CONN";
  case INTERMITTENT:
    return "INT";
  case DISCONNECTED:
    return "DISC";
  case INTERMITTENT_KEEP_ALIVE:
    return "IKA";
  default:
    return "?";
  }
}

const char *batteryTierLabel(uint8_t tier) {
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

void logWakeSummary(const char *reason) {
  Log.info("Wake: reason=%s open=%d ready=%d occ=%d led=%lus",
           reason,
           isWithinOpenHours() ? 1 : 0,
           SensorManager::instance().isSensorReady() ? 1 : 0,
           current.get_occupied() ? 1 : 0,
           (unsigned long)(signalLEDTimeRemaining() / 1000UL));
}

void logTeardownContext(const char *prefix,
                       bool standbyRequested,
                       bool standbyEffective) {
  if (!sysStatus.get_verboseMode()) {
    return;
  }

  Log.info("%s mode=%s standby=%d/%d tier=%s occ=%d",
           prefix,
           connectionModeLabel(static_cast<ConnectionMode>(sysStatus.get_connectionMode())),
           standbyRequested ? 1 : 0,
           standbyEffective ? 1 : 0,
           batteryTierLabel(sysStatus.get_currentBatteryTier()),
           current.get_occupied() ? 1 : 0);
}

// Drain USB CDC serial buffer before sleep to prevent split log lines (bench testing only)
void drainSerialBeforeSleep() {
  if (Serial.isConnected()) {
    Serial.flush();
    delay(75);
  }
}

void markModemUnstable(ModemUnstableReason reason, unsigned long elapsedMs) {
  const bool reasonChanged = !session.modemUnstable ||
                             session.modemUnstableReason != (uint8_t)reason;
  session.modemUnstable = true;
  session.modemUnstableReason = (uint8_t)reason;

  if (reasonChanged) {
    switch (reason) {
    case MODEM_UNSTABLE_REASON_SLOW_TEARDOWN:
      Log.warn("MODEM_HEALTH: unstable reason=slow_teardown elapsed=%lu", elapsedMs);
      break;
    case MODEM_UNSTABLE_REASON_CONNECT_TIMEOUT:
      Log.warn("MODEM_HEALTH: unstable reason=connect_timeout count=%u",
               (unsigned)session.modemConnectTimeoutCount);
      break;
    default:
      break;
    }
  }

  if (!session.modemStandbySuppressed) {
    Log.warn("MODEM_POLICY: standby temporarily disabled reason=unstable_modem");
    session.modemStandbySuppressed = true;
  }
}

void maybeClearModemUnstable(unsigned long teardownElapsedMs) {
  if (!session.modemUnstable) {
    return;
  }

  if (Observability::cycleStats().connect_result !=
          Observability::WakeCycleStats::ConnectResult::SUCCESS ||
      teardownElapsedMs > MODEM_UNSTABLE_RECOVERY_TEARDOWN_MS) {
    return;
  }

  session.modemUnstable = false;
  session.modemUnstableReason = MODEM_UNSTABLE_REASON_NONE;
  session.modemConnectTimeoutCount = 0;
  Log.info("MODEM_HEALTH: recovered");
  if (session.modemStandbySuppressed) {
    session.modemStandbySuppressed = false;
    Log.info("MODEM_POLICY: standby restored");
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

#if PLATFORM_ID == PLATFORM_BORON
bool isDeviceOsAtLeast6400() {
  int major = 0;
  int minor = 0;
  int patch = 0;
  const String version = System.version();
  if (sscanf(version.c_str(), "%d.%d.%d", &major, &minor, &patch) < 2) {
    return false;
  }

  if (major > 6) {
    return true;
  }
  if (major < 6) {
    return false;
  }
  return minor >= 4;
}

bool isRtcTimeValidForHibernate(time_t rtcNow) {
  static const time_t kRtcMin = 1704067200; // 2024-01-01 00:00:00 UTC
  static const time_t kRtcMax = 2051222400; // 2035-01-01 00:00:00 UTC
  return rtcNow >= kRtcMin && rtcNow < kRtcMax;
}

bool clearAb1805StaleAlarmInterrupts() {
  static const uint8_t kRegStatus = 0x0f;
  static const uint8_t kRegIntMask = 0x12;
  static const uint8_t kMaskAieTie = 0x0c;

  uint8_t intMask = 0;
  if (!ab1805.readRegister(kRegIntMask, intMask)) {
    return false;
  }

  // Clear stale interrupt flags before arming the next one-shot alarm.
  if (!ab1805.writeRegister(kRegStatus, 0x00)) {
    return false;
  }

  intMask = (uint8_t)(intMask & (uint8_t)~kMaskAieTie);
  if (!ab1805.writeRegister(kRegIntMask, intMask)) {
    return false;
  }

  if (!ab1805.maskRegister(
        AB1805::REG_TIMER_CTRL,
        (uint8_t)~AB1805::REG_TIMER_CTRL_RPT_MASK,
        AB1805::REG_TIMER_CTRL_RPT_DIS)) {
    return false;
  }

  return true;
}

bool shouldUseBoronRtcAlarmHibernate(uint32_t requestedSleepSec, time_t &rtcNow) {
  Log.info("HibernateDiag: check enabled=%d requested=%lu",
           sysStatus.get_enableHibernateSleep() ? 1 : 0,
           (unsigned long)requestedSleepSec);
  if (!sysStatus.get_enableHibernateSleep()) {
    Log.info("HibernateDiag: fail=disabled");
    return false;
  }

  if (!isDeviceOsAtLeast6400()) {
    Log.info("HibernateDiag: fail=device_os");
    return false;
  }

  static const uint32_t kMinHibernateSleepSec = 900;
  static const uint32_t kMaxHibernateSleepSec = 36000;
  if (requestedSleepSec < kMinHibernateSleepSec || requestedSleepSec > kMaxHibernateSleepSec) {
    Log.info("HibernateDiag: fail=duration requested=%lu", (unsigned long)requestedSleepSec);
    return false;
  }

  if (!ab1805.isRTCSet()) {
    Log.info("HibernateDiag: fail=rtc_not_set");
    return false;
  }

  if (!ab1805.getRtcAsTime(rtcNow)) {
    Log.info("HibernateDiag: fail=rtc_read");
    return false;
  }

  if (!isRtcTimeValidForHibernate(rtcNow)) {
    Log.info("HibernateDiag: fail=rtc_invalid epoch=%ld", (long)rtcNow);
    return false;
  }

  Log.info("HibernateDiag: pass epoch=%ld", (long)rtcNow);
  return true;
}
#endif

} // namespace

// NOTE:
// This file was split from StateHandlers.cpp as a mechanical refactor.
// No behavioral changes were made.

/**
 * @brief SLEEPING_STATE: deep sleep between reporting intervals.
 * ...
 */
void handleSleepingState() {
  setLoopStage(LOOP_STAGE_SLEEP_PREP);

  bool enteredState = (state != oldState);
  static bool operationsCompleteLogged = false;  // Track if we've logged completion message
  static bool gateWasBlocking = false;
  static unsigned long lastGateStatusLogMs = 0;
  static uint8_t lastGateBlockerMask = kGateBlockerUninitialized;
  static uint8_t lastGateBlockerBeforeRelease = 0;
  static bool gateReleaseReasonKnown = false;
  static bool cloudDisconnectCompleteLogged = false;
#if HAL_PLATFORM_CELLULAR
  static bool preserveStandbyAfterNormalReturn = false;
  static bool forceModemOffAfterStandbyFailure = false;
  static int standbySleepFailureError = SYSTEM_ERROR_NONE;
#endif
#if Wiring_Cellular
  static bool radioOffCompleteLogged = false;
#endif

  if (enteredState) {
    setAppBreadcrumb(3);
    publishStateTransition();
    operationsCompleteLogged = false;  // Reset flag on state entry
    gateWasBlocking = false;
    lastGateStatusLogMs = 0;
    lastGateBlockerMask = kGateBlockerUninitialized;
    lastGateBlockerBeforeRelease = 0;
    gateReleaseReasonKnown = false;
    cloudDisconnectCompleteLogged = false;
  #if Wiring_Cellular
    radioOffCompleteLogged = false;
  #endif
  }

  // Determine if we will use *effective* network standby for the upcoming sleep.
  // Only cellular platforms actually apply standby; on WiFi, treat as disabled
  // so disconnect logic doesn't wait for a radio state that won't change.
  // Standby is only useful after a successful cloud session in the current
  // wake cycle; if connect never succeeded, preserving the modem state buys
  // nothing and can carry a bad NCP state into the next wake.
  bool useNetworkStandbyRequested =
      (sysStatus.get_connectionMode() == INTERMITTENT_KEEP_ALIVE) &&
      isWithinOpenHours();
  bool preserveStandbyForThisPass = false;
#if HAL_PLATFORM_CELLULAR
  if (preserveStandbyAfterNormalReturn &&
      (!useNetworkStandbyRequested || session.modemUnstable || Cellular.isOff())) {
    preserveStandbyAfterNormalReturn = false;
  }
  preserveStandbyForThisPass = preserveStandbyAfterNormalReturn;
#endif
  bool useNetworkStandbyEffective =
      useNetworkStandbyRequested &&
      (preserveStandbyForThisPass ||
       Observability::cycleStats().connect_result ==
           Observability::WakeCycleStats::ConnectResult::SUCCESS);
#if HAL_PLATFORM_CELLULAR
  useNetworkStandbyEffective = useNetworkStandbyEffective && !Cellular.isOff();
  if (session.modemUnstable || forceModemOffAfterStandbyFailure) {
    useNetworkStandbyEffective = false;
  }
#else
  useNetworkStandbyEffective = false;
#endif

  // If a ledger update (or time progression) moves the park into OPEN hours
  // while we are in SLEEPING_STATE, abort sleeping immediately in CONNECTED
  // mode so we stay awake/connected and resume counting.
  if (Time.isValid() && sysStatus.get_connectionMode() == CONNECTED && isWithinOpenHours()) {
    ensureSensorEnabled("SLEEP abort: CONNECTED+OPEN");
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
  static unsigned long cloudSyncStartMs = 0;
  static unsigned long cloudSyncBudgetMs = 0;
  static uint16_t cloudSyncMaxQueueDepth = 0;
  static uint16_t lastQueueDepth = 0xFFFF;

  // ********** Non-blocking disconnect + modem power-down **********
  // Device OS already manages the asynchronous cloud session teardown once
  // Particle.disconnect() has been requested. Here, we keep application
  // logic minimal: request cloud disconnect and radio-off once, then wait
  // (bounded) until both cloud and modem are actually off before sleeping.
  static bool disconnectRequested = false;
  static unsigned long disconnectRequestStartMs = 0;
  static unsigned long cloudDisconnectStartMs = 0;
  static unsigned long cloudDisconnectElapsedMs = 0;
  static unsigned long modemOffElapsedMs = 0;
  static unsigned long lastDisconnectProgressMs = 0;
  static unsigned long lastGateProgressMs = 0;
#if Wiring_WiFi
  static bool wifiOffGuardActive = false;
  static unsigned long wifiOffGuardStartMs = 0;
  static unsigned long wifiOffGuardLastRetryMs = 0;
  static uint8_t wifiOffGuardRetries = 0;
#endif

  if (enteredState) {
    disconnectRequested = false;
    disconnectRequestStartMs = 0;
    cloudDisconnectStartMs = 0;
    cloudDisconnectElapsedMs = 0;
    modemOffElapsedMs = 0;
    cloudSyncBudgetMs = 0;
    cloudSyncMaxQueueDepth = 0;
    lastQueueDepth = 0xFFFF;
    lastDisconnectProgressMs = 0;
    lastGateProgressMs = 0;
#if Wiring_WiFi
    wifiOffGuardActive = false;
    wifiOffGuardStartMs = 0;
    wifiOffGuardLastRetryMs = 0;
    wifiOffGuardRetries = 0;
#endif
  }

  // Safety: if disconnectRequested was latched but the timestamp got lost
  // (for example, due to an unexpected state-machine re-entry), reset so we
  // can actually issue the request instead of waiting forever.
  if (disconnectRequested && disconnectRequestStartMs == 0) {
    Log.warn("SLEEP: disconnectRequested latched without timestamp - resetting");
    disconnectRequested = false;
    cloudDisconnectStartMs = 0;
    cloudDisconnectElapsedMs = 0;
    modemOffElapsedMs = 0;
  }

  // Only wait for cloud operations *before* requesting disconnect.
  if (Particle.connected() && !disconnectRequested) {
    // Initialize timer on first check
    if (cloudSyncStartMs == 0) {
      cloudSyncStartMs = millis();
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
    if (queueDepth > cloudSyncMaxQueueDepth) {
      cloudSyncMaxQueueDepth = queueDepth;
    }
    unsigned long queueAwareBudgetMs = computeCloudSyncTimeoutMs(cloudSyncMaxQueueDepth);
    if (queueAwareBudgetMs > cloudSyncBudgetMs) {
      cloudSyncBudgetMs = queueAwareBudgetMs;
    }
    if (!outputLedgersSynced &&
        ConnectivityPolicy::OUTPUT_LEDGER_SYNC_TIMEOUT_MS > cloudSyncBudgetMs) {
      cloudSyncBudgetMs = ConnectivityPolicy::OUTPUT_LEDGER_SYNC_TIMEOUT_MS;
    }

    {
      if (lastQueueDepth != 0xFFFF && queueDepth < lastQueueDepth) {
        thrashGuard.markProgress("QUEUE_DRAIN");
      }
      lastQueueDepth = queueDepth;
    }

    bool allComplete = queueEmpty && ledgersSynced && updatesChecked && webhookConfirmed;

    if (!allComplete) {
      unsigned long elapsedMs = millis() - cloudSyncStartMs;
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
        lastGateBlockerBeforeRelease = blockerMask;
        gateReleaseReasonKnown = true;
      }

      if (elapsedMs > kGateBlockLogThresholdMs &&
          lastGateBlockerMask != blockerMask) {
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
                 queuePending,
                 ledgerPending,
                 webhookPending,
                 updatePending);
#endif
        lastGateBlockerMask = blockerMask;
      }

      if (elapsedMs < cloudSyncBudgetMs) {
        if (!gateWasBlocking ||
            (millis() - lastGateStatusLogMs) >= ConnectivityPolicy::CLOUD_OPS_STATUS_LOG_INTERVAL_MS) {
#if ENABLE_GATE_TRACE
          Log.info("Gate: q=%d ledger=%d webhook=%d update=%d wait=%lu/%lu",
                   queuePending,
                   ledgerPending,
                   webhookPending,
                   updatePending,
                   elapsedMs,
                   cloudSyncBudgetMs);
#endif
          gateWasBlocking = true;
          lastGateStatusLogMs = millis();
        }
        // Mark progress to prevent ThrashGuard timeout during legitimate cloud operations wait
        thrashGuard.markProgress("CLOUD_OPS_WAIT");
        serviceAwakeWatchdog();
        Particle.process(); // Keep connection alive
        return; // Stay in SLEEPING_STATE until complete or timeout
      }

      // Budget exceeded - log incomplete operations and raise ONE alert (priority order)
       const char *gateFailReason = !queueEmpty ? "queue" :
        (!ledgersSynced ? "ledger" : (!webhookConfirmed ? "webhook" : "update"));
       Log.warn("GateFail: reason=%s timeout=%lu q=%d ledger=%d webhook=%d update=%d",
          gateFailReason,
          cloudSyncBudgetMs,
          queuePending,
          ledgerPending,
          webhookPending,
          updatePending);
       gateWasBlocking = false;
       lastGateStatusLogMs = 0;
       lastGateBlockerMask = kGateBlockerUninitialized;
       lastGateBlockerBeforeRelease = 0;
       gateReleaseReasonKnown = false;

      // Only raise one alert - check in priority order (queue > ledger > updates > webhook)
      if (!queueEmpty) {
        Log.warn("SLEEP: Publish queue not empty - raising alert 43");
        current.raiseAlert(43); // Queue drainage failure (highest priority)
      } else if (!ledgersSynced) {
        Log.warn("SLEEP: ledger sync incomplete after %lu ms (budget=%lu ms) - raising alert 44",
                 elapsedMs,
                 cloudSyncBudgetMs);
        current.raiseAlert(44); // Ledger sync timeout before sleep (minor - config already applied)
      } else if (!updatesChecked) {
        Log.warn("SLEEP: OTA updates pending - raising alert 42");
        current.raiseAlert(42); // OTA updates pending
      } else if (!webhookConfirmed) {
        Log.warn("SLEEP: Webhook response not received - raising alert 40");
        current.raiseAlert(40); // Webhook response timeout
      }

      // Proceed with disconnect despite incomplete operations
      cloudSyncStartMs = 0;
      cloudSyncBudgetMs = 0;
      cloudSyncMaxQueueDepth = 0;
    } else {
      unsigned long gateWaitMs = millis() - cloudSyncStartMs;
      if (gateWaitMs > kGateBlockLogThresholdMs && lastGateBlockerMask != 0) {
#if ENABLE_GATE_TRACE
        Log.info("GateBlock: wait=%lu reason=none q=0 ledger=0 webhook=0 update=0",
                 gateWaitMs);
#endif
        lastGateBlockerMask = 0;
      }
      if (gateWasBlocking) {
        Cloud::LedgerSyncDiagnostics ledgerDiagnosticsRelease = Cloud::instance().ledgerSyncDiagnostics();
#if ENABLE_GATE_TRACE
        Log.info("GateReleaseDetail: wait=%lu pending=%u syncing=%d inflight=%d reason=%s",
                 gateWaitMs,
                 ledgerDiagnosticsRelease.pendingCount,
                 ledgerDiagnosticsRelease.syncing ? 1 : 0,
                 ledgerDiagnosticsRelease.inflight ? 1 : 0,
                 gateReleaseReasonKnown
                     ? gateBlockerReasonLabel(lastGateBlockerBeforeRelease)
                     : "unknown");
#endif
        (void)ledgerDiagnosticsRelease;
        Log.info("GateRelease: wait=%lu reason=%s",
                 gateWaitMs,
                 gateReleaseReasonKnown
                     ? gateBlockerReasonLabel(lastGateBlockerBeforeRelease)
                     : "unknown");
        Log.info("Gate: ok wait=%lu", gateWaitMs);
      }
      gateWasBlocking = false;
      lastGateStatusLogMs = 0;
      lastGateBlockerMask = kGateBlockerUninitialized;
      lastGateBlockerBeforeRelease = 0;
      gateReleaseReasonKnown = false;
      // All operations complete - reset timer and proceed
      cloudSyncStartMs = 0;
      cloudSyncBudgetMs = 0;
      cloudSyncMaxQueueDepth = 0;
    }

    if (!operationsCompleteLogged) {
      if (sysStatus.get_verboseMode()) {
        Log.info("SLEEP: Cloud operations gate passed - ready to disconnect");
        Log.info("SLEEP: disconnect context connected=%d radioOn=%d standbyEffective=%d occupied=%d",
                 Particle.connected(), Connectivity::isRadioPoweredOn(), useNetworkStandbyEffective, current.get_occupied());
      }

      // Observability: end of service window (ledger sync + queue drain + OTA check + webhook).
      Observability::cycleStats().markServiceEnd(millis());

      operationsCompleteLogged = true;
    }
  } else {
    // Not connected - reset timer
    cloudSyncStartMs = 0;
    cloudSyncBudgetMs = 0;
    cloudSyncMaxQueueDepth = 0;
    lastGateBlockerMask = kGateBlockerUninitialized;
    lastGateBlockerBeforeRelease = 0;
    gateReleaseReasonKnown = false;
  }

  // If we're going to sleep with cellular network standby, do NOT wait for radio-off.
  // We only need to tear down the cloud session (quickly) before sleeping.
  bool stillOn;
#if Wiring_Cellular
  stillOn = useNetworkStandbyEffective ? Particle.connected() : (Particle.connected() || Connectivity::isRadioPoweredOn());
#else
  // On WiFi platforms, waiting for WiFi.isOn() to flip false can take a long
  // time and is not required for safe low-power sleep. Only gate on cloud.
  stillOn = Particle.connected();
#endif

  // Compute disconnect budget once per loop to avoid duplicated logic.
  auto computeDisconnectBudgetMs = [&]() -> unsigned long {
    // Use ledger-configured budgets when available, with conservative defaults.
    uint16_t cloudBudgetSec = sysStatus.get_cloudDisconnectBudgetSec();
    if (cloudBudgetSec < ConnectivityPolicy::DISCONNECT_BUDGET_MIN_SEC ||
        cloudBudgetSec > ConnectivityPolicy::DISCONNECT_BUDGET_MAX_SEC) {
      cloudBudgetSec = ConnectivityPolicy::DISCONNECT_CLOUD_DEFAULT_SEC;
    }

    uint16_t modemBudgetSec = sysStatus.get_modemOffBudgetSec();
    if (modemBudgetSec < ConnectivityPolicy::DISCONNECT_BUDGET_MIN_SEC ||
        modemBudgetSec > ConnectivityPolicy::DISCONNECT_BUDGET_MAX_SEC) {
      modemBudgetSec = ConnectivityPolicy::DISCONNECT_MODEM_DEFAULT_SEC;
    }

    unsigned long budgetMs = (unsigned long)((modemBudgetSec > cloudBudgetSec) ? modemBudgetSec : cloudBudgetSec) * 1000UL;

    // When using network standby, shorten the disconnect grace period: we don't
    // want to burn connection budget waiting for a perfect teardown.
    if (useNetworkStandbyEffective) {
      budgetMs = (unsigned long)cloudBudgetSec * 1000UL;
      if (budgetMs > ConnectivityPolicy::DISCONNECT_STANDBY_MAX_MS) {
        budgetMs = ConnectivityPolicy::DISCONNECT_STANDBY_MAX_MS;
      }
    }
    return budgetMs;
  };

  unsigned long disconnectBudgetMs = computeDisconnectBudgetMs();

  // If cloud is already offline but the radio is still on (common after a transient
  // cloud drop), power down the radio before sleeping (unless using standby).
#if Wiring_Cellular
  if (!disconnectRequested && !useNetworkStandbyEffective && !Particle.connected() && Connectivity::isRadioPoweredOn()) {
    unsigned long requestMs = millis();
    cloudDisconnectCompleteLogged = true;
    cloudDisconnectElapsedMs = 0;
    if (sysStatus.get_verboseMode()) {
      Log.info("SLEEP: cloud disconnect complete elapsed=0 ms");
    }
  #if HAL_PLATFORM_CELLULAR
    if (forceModemOffAfterStandbyFailure) {
      Log.warn("SLEEP: standby sleep failed; forcing modem-off reason=sleep_error err=%d",
               standbySleepFailureError);
      forceModemOffAfterStandbyFailure = false;
      standbySleepFailureError = SYSTEM_ERROR_NONE;
    }
  #endif
    logTeardownContext("SLEEP: modem-off start", useNetworkStandbyRequested, useNetworkStandbyEffective);
    Connectivity::requestRadioPowerOff();
    disconnectRequested = true;
    disconnectRequestStartMs = requestMs;
    Observability::cycleStats().markTeardownStart((uint32_t)disconnectRequestStartMs, useNetworkStandbyEffective);
    return;
  }
#endif

  // Once cloud operations are satisfied (or timed out), initiate disconnect exactly once.
  if (Particle.connected() && !disconnectRequested) {
    unsigned long requestMs = millis();
    logTeardownContext("SLEEP: cloud disconnect start", useNetworkStandbyRequested, useNetworkStandbyEffective);
    cloudDisconnectStartMs = requestMs;
    cloudDisconnectElapsedMs = 0;
    if (useNetworkStandbyEffective) {
      Connectivity::requestCloudDisconnectOnly();
    } else {
      logTeardownContext("SLEEP: modem-off start", useNetworkStandbyRequested, useNetworkStandbyEffective);
      modemOffElapsedMs = 0;
      Connectivity::requestFullDisconnectAndRadioOff();
    }
    disconnectRequested = true;
    disconnectRequestStartMs = requestMs;

    // Observability: teardown window start.
    Observability::cycleStats().markTeardownStart((uint32_t)disconnectRequestStartMs, useNetworkStandbyEffective);

    return;
  }

  // If disconnect was requested, wait (bounded) for it to take effect before sleeping.
  if (disconnectRequested && !cloudDisconnectCompleteLogged && !Particle.connected()) {
    cloudDisconnectElapsedMs = (cloudDisconnectStartMs == 0) ? 0UL : (millis() - cloudDisconnectStartMs);
    if (sysStatus.get_verboseMode()) {
      Log.info("SLEEP: cloud disconnect complete elapsed=%lu ms standby=%d",
               cloudDisconnectElapsedMs,
               useNetworkStandbyEffective ? 1 : 0);
    }
    cloudDisconnectCompleteLogged = true;
  }
#if Wiring_Cellular
  if (disconnectRequested && !useNetworkStandbyEffective &&
      !radioOffCompleteLogged && !Connectivity::isRadioPoweredOn()) {
    modemOffElapsedMs = (disconnectRequestStartMs == 0) ? 0UL : (millis() - disconnectRequestStartMs);
    if (sysStatus.get_verboseMode()) {
      Log.info("SLEEP: modem-off complete elapsed=%lu ms", modemOffElapsedMs);
    }
    radioOffCompleteLogged = true;
  }
#endif

  if (disconnectRequested && stillOn) {
    if (disconnectRequestStartMs != 0 && (millis() - disconnectRequestStartMs) > disconnectBudgetMs) {
      Log.warn("SLEEP: disconnect/modem-off exceeded budget (%lu ms, cloud=%d radioOn=%d standby=%d) - raising alert 15",
               (unsigned long)(millis() - disconnectRequestStartMs),
               (int)Particle.connected(),
               (int)Connectivity::isRadioPoweredOn(),
               useNetworkStandbyEffective ? 1 : 0);
      markModemUnstable(MODEM_UNSTABLE_REASON_SLOW_TEARDOWN,
                        (unsigned long)(millis() - disconnectRequestStartMs));
      current.raiseAlert(15);
      transitionTo(ERROR_STATE, "sleep-disconnect-timeout");
      disconnectRequested = false;
      disconnectRequestStartMs = 0;
      cloudDisconnectStartMs = 0;
      cloudDisconnectElapsedMs = 0;
      modemOffElapsedMs = 0;
      cloudDisconnectCompleteLogged = false;
    #if Wiring_Cellular
      radioOffCompleteLogged = false;
    #endif
      return;
    }

    // Help DeviceOS make progress on the disconnect.
    // Throttle progress markers to avoid noisy per-loop updates.
    unsigned long nowMs = millis();
    if ((nowMs - lastDisconnectProgressMs) > 1000UL) {
      thrashGuard.markProgress("DISCONNECT_WAIT");
      lastDisconnectProgressMs = nowMs;
    }
    serviceAwakeWatchdog();
    Particle.process();
    return;
  }

  // Observability: teardown completed (bounded wait finished and we are proceeding to sleep).
  if (disconnectRequested && !stillOn) {
    const unsigned long teardownElapsedMs =
        (disconnectRequestStartMs == 0) ? 0UL : (millis() - disconnectRequestStartMs);
    Log.info("Sleep: td=%lums cloud=%lums modem=%lums standby=%d/%d mode=%s tier=%s occ=%d",
             teardownElapsedMs,
             cloudDisconnectElapsedMs,
             modemOffElapsedMs,
             useNetworkStandbyRequested ? 1 : 0,
             useNetworkStandbyEffective ? 1 : 0,
         connectionModeLabel(static_cast<ConnectionMode>(sysStatus.get_connectionMode())),
             batteryTierLabel(sysStatus.get_currentBatteryTier()),
             current.get_occupied() ? 1 : 0);
    session.lastCloudDisconnectElapsedMs = cloudDisconnectElapsedMs;
    session.lastModemOffElapsedMs = modemOffElapsedMs;
    session.lastTotalTeardownElapsedMs = teardownElapsedMs;
    session.lastTeardownEndMs = millis();
    if (teardownElapsedMs > MODEM_UNSTABLE_SLOW_TEARDOWN_MS) {
      markModemUnstable(MODEM_UNSTABLE_REASON_SLOW_TEARDOWN, teardownElapsedMs);
    } else {
      maybeClearModemUnstable(teardownElapsedMs);
    }
    Observability::cycleStats().markTeardownEnd(millis());
    disconnectRequested = false;
    disconnectRequestStartMs = 0;
    cloudDisconnectStartMs = 0;
    cloudDisconnectElapsedMs = 0;
    modemOffElapsedMs = 0;
  }

  setAppBreadcrumb(18);

  // Enforce sleep preconditions before any final sleep call in this state.
  // Non-standby cellular sleep requires both cloud disconnect and radio/modem off.
  // Standby sleep requires cloud disconnect only.
  auto sleepPreconditionsSatisfied = [&]() -> bool {
#if Wiring_Cellular
    if (!useNetworkStandbyEffective) {
      return !Particle.connected() && !Connectivity::isRadioPoweredOn();
    }
#endif
    return !Particle.connected();
  };

  if (!sleepPreconditionsSatisfied()) {
    bool cloudConnected = Particle.connected();

    // Request teardown exactly once, then wait with existing budget handling.
    if (!disconnectRequested) {
      if (useNetworkStandbyEffective) {
        Log.warn("SLEEP: pre-sleep gate blocked (cloud=%d standby=1) - requesting disconnect",
                 (int)cloudConnected);
        logTeardownContext("SLEEP: cloud disconnect start", useNetworkStandbyRequested, useNetworkStandbyEffective);
        cloudDisconnectStartMs = millis();
        cloudDisconnectElapsedMs = 0;
        Connectivity::requestCloudDisconnectOnly();
      } else if (cloudConnected) {
        Log.warn("SLEEP: pre-sleep gate blocked (cloud=1 standby=0) - requesting disconnect + modem off");
        logTeardownContext("SLEEP: cloud disconnect start", useNetworkStandbyRequested, useNetworkStandbyEffective);
        logTeardownContext("SLEEP: modem-off start", useNetworkStandbyRequested, useNetworkStandbyEffective);
        cloudDisconnectStartMs = millis();
        cloudDisconnectElapsedMs = 0;
        modemOffElapsedMs = 0;
        Connectivity::requestFullDisconnectAndRadioOff();
      } else {
        Log.warn("SLEEP: pre-sleep gate blocked (cloud=0 radioOn=1 standby=0) - requesting modem off");
        cloudDisconnectCompleteLogged = true;
        cloudDisconnectElapsedMs = 0;
        if (sysStatus.get_verboseMode()) {
          Log.info("SLEEP: cloud disconnect complete elapsed=0 ms");
        }
        logTeardownContext("SLEEP: modem-off start", useNetworkStandbyRequested, useNetworkStandbyEffective);
        modemOffElapsedMs = 0;
        Connectivity::requestRadioPowerOff();
      }

      disconnectRequested = true;
      disconnectRequestStartMs = millis();
      Observability::cycleStats().markTeardownStart((uint32_t)disconnectRequestStartMs, useNetworkStandbyEffective);
      return;
    }

    if (disconnectRequestStartMs == 0) {
      disconnectRequestStartMs = millis();
    }

    unsigned long elapsedMs = millis() - disconnectRequestStartMs;
    if (elapsedMs > disconnectBudgetMs) {
      Log.warn("SLEEP: pre-sleep teardown exceeded budget (%lu ms, cloud=%d radioOn=%d standby=%d) - raising alert 15",
               elapsedMs,
               (int)Particle.connected(),
               (int)Connectivity::isRadioPoweredOn(),
               useNetworkStandbyEffective ? 1 : 0);
      markModemUnstable(MODEM_UNSTABLE_REASON_SLOW_TEARDOWN, elapsedMs);
      current.raiseAlert(15);
      transitionTo(ERROR_STATE, "sleep-precondition-timeout");
      disconnectRequested = false;
      disconnectRequestStartMs = 0;
      cloudDisconnectStartMs = 0;
      cloudDisconnectElapsedMs = 0;
      modemOffElapsedMs = 0;
      cloudDisconnectCompleteLogged = false;
    #if Wiring_Cellular
      radioOffCompleteLogged = false;
    #endif
      return;
    }

    unsigned long nowMs = millis();
    if ((nowMs - lastGateProgressMs) > 1000UL) {
      thrashGuard.markProgress("DISCONNECT_WAIT");
      lastGateProgressMs = nowMs;
    }
    serviceAwakeWatchdog();
    Particle.process();
    return;
  }

  setAppBreadcrumb(19);

  int nightSleepSec = -1;
  const bool openNow = isWithinOpenHours();
  logTimeDiag(openNow);
  if (!openNow) {
    // Notify sensor layer we are entering full night sleep so sensors and
    // indicator LEDs can be powered down. During daytime naps we keep
    // interrupt-driven sensors (like PIR) powered so they can wake the
    // device from ULTRA_LOW_POWER sleep.
    SensorManager::instance().onEnterSleep();

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
  const bool overnightFallbackSleep = (!isWithinOpenHours() && nightSleepSec > 0);
  if (overnightFallbackSleep) {
    wakeInSeconds = nightSleepSec;
    sleepReason = "closed";
  } else {
    // Within opening hours, align wake to the reporting boundary.
    // Add 1 second margin to ensure we wake slightly after the boundary.
    if (Time.isValid() && intervalSec > 0) {
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
  if (sysStatus.get_sensorMode() == COUNTING) {
    uint32_t ledRemaining = signalLEDTimeRemaining();
    if (sensorDetect || ledRemaining > 0) {
      Log.info("COUNTING: Deferring sleep - sensor=%d LED remaining=%lu sec", 
               sensorDetect, ledRemaining);
      transitionTo(IDLE_STATE, "sleep-counting-led-defer");
      return;
    }
    // Turn off LED before sleep (should already be off)
    if (signalLEDStatus()) {
      signalLED(false);
    }
  }

  // OCCUPANCY MODE: Calculate sleep duration based on debounce timer or scheduled wake
  if (sysStatus.get_sensorMode() == OCCUPANCY) {
    if (current.get_occupied()) {
      // Occupied - wake for debounce timeout check
      uint32_t setting1Raw = sensorConfig.get_sensorSetting1();
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

#if PLATFORM_ID == PLATFORM_BORON
  if (overnightFallbackSleep) {
    time_t rtcNow = 0;
    if (shouldUseBoronRtcAlarmHibernate((uint32_t)wakeInSeconds, rtcNow)) {
      pinMode(WAKEUP_PIN, INPUT_PULLUP);
      if (digitalRead(WAKEUP_PIN) == HIGH) {
        if (clearAb1805StaleAlarmInterrupts()) {
          const time_t wakeTime = rtcNow + (time_t)wakeInSeconds;
          if (ab1805.interruptAtTime(wakeTime)) {
            setAppBreadcrumb(20);
            config = SystemSleepConfiguration();
            config.mode(SystemSleepMode::HIBERNATE)
              .gpio(WAKEUP_PIN, FALLING)
              .gpio(BUTTON_PIN, FALLING);

            const bool watchdogStopped = ab1805.stopWDT();
            pauseAwakeWatchdogForSleep("hibernate");
            if (!watchdogStopped) {
              Log.warn("SleepCall: watchdog-stop failed mode=HIBERNATE dur=%ds", wakeInSeconds);
            }

            retainedHibernateRtcBefore = rtcNow;
            retainedHibernateWakeTime = wakeTime;
            retainedHibernateRequestedSleep = (uint32_t)wakeInSeconds;
            retainedHibernateCount++;
            retainedHibernatePending = true;

            Log.info("Sleep: HIBERNATE reason=closed dur=%ds wakePin=%d", wakeInSeconds, (int)WAKEUP_PIN);
            PowerDiagnostics::logPowerState("pre-hibernate");
            thrashGuard.markProgress("SLEEP_ATTEMPT");
            Cloud::instance().logLedgerSleepState();
            setAppBreadcrumb(21);
            drainSerialBeforeSleep();
            const SystemSleepResult hibernateResult = System.sleep(config);

            // HIBERNATE should reset the MCU. If we return here, treat it as failed and fall back.
            retainedHibernatePending = false;
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

  // Before a long overnight
  // ULTRA_LOW_POWER fallback sleep, optionally reset first when heap has
  // drifted low. This gives the next day a clean start without affecting
  // open-hours behavior.
  if (overnightFallbackSleep) {
    const unsigned long freeHeap = (unsigned long)System.freeMemory();

    if (freeHeap <= ConnectivityPolicy::NIGHTLY_HEAP_RESET_THRESHOLD_BYTES) {
      Log.warn("Nightly heap guard: freeHeap=%lu <= %lu before overnight ULTRA_LOW_POWER fallback - resetting for clean next-day start",
               freeHeap,
               ConnectivityPolicy::NIGHTLY_HEAP_RESET_THRESHOLD_BYTES);
      System.reset();
      return;
    }

    if (freeHeap <= ConnectivityPolicy::NIGHTLY_HEAP_WARN_THRESHOLD_BYTES) {
      Log.warn("Nightly heap guard: freeHeap=%lu <= %lu before overnight ULTRA_LOW_POWER fallback - monitoring only",
               freeHeap,
               ConnectivityPolicy::NIGHTLY_HEAP_WARN_THRESHOLD_BYTES);
    }
  }

  // Reset sleep configuration on each sleep so GPIO selections do not
  // accumulate across calls.
  setAppBreadcrumb(20);
  config = SystemSleepConfiguration();

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
    const float soc = current.get_stateOfCharge();
    const uint16_t socTenths = (soc >= 0.0f && soc <= 100.0f) ? (uint16_t)(soc * 10.0f + 0.5f) : 0xFFFF;
    const uint8_t battState = (uint8_t)current.get_batteryState();
    const uint8_t isCharging = (battState == 0) ? 0xFF : ((battState == 2) ? 1 : 0); // 0=Unknown, 2=Charging

    Observability::cycleStats().finalizeBeforeSleep(
        millis(),
        Particle.connected(),
        Connectivity::isRadioPoweredOn(),
        qDepth,
        socTenths,
        isCharging,
        sysStatus.get_lastConnection());

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
    if (sysStatus.get_verboseMode()) {
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
      if (sysStatus.get_verboseMode()) {
        Log.info("SLEEP: WiFi radio powered off before sleep (%lu ms)", guardElapsedMs);
      }
      wifiOffGuardActive = false;
      wifiOffGuardStartMs = 0;
      wifiOffGuardLastRetryMs = 0;
      wifiOffGuardRetries = 0;
    }
  }
#endif
  
  config.mode(SystemSleepMode::ULTRA_LOW_POWER)
    .gpio(BUTTON_PIN, FALLING)     // Service button wake (active-low)
    .gpio(intPin, RISING);         // PIR sensor wake
    
  // Add network standby for occupancy mode to avoid reconnection delays
  // Note: Network standby is only supported/needed on cellular devices to prevent
  // carrier blacklisting. WiFi reconnection is already fast enough (~2-5s).
  if (useNetworkStandby) {
#if HAL_PLATFORM_CELLULAR
    // Keep cellular modem in low-power standby so we can quickly reconnect
    // when occupancy changes. This prevents 30-60s reconnection overhead and
    // potential carrier blacklisting from rapid disconnect/reconnect cycles.
    config.network(NETWORK_INTERFACE_CELLULAR, SystemSleepNetworkFlag::INACTIVE_STANDBY);
#endif
  }
  
  config.duration((uint32_t)wakeInSeconds * 1000UL);  // Timer-based wake at reporting boundary

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
           current.get_occupied() ? 1 : 0,
           (double)current.get_stateOfCharge());
#else
  Log.info("Sleep: ULP standby=%d reason=%s dur=%ds occ=%d soc=%.1f",
           useNetworkStandby ? 1 : 0,
           sleepReason,
           wakeInSeconds,
           current.get_occupied() ? 1 : 0,
           (double)current.get_stateOfCharge());
#endif

  thrashGuard.markProgress("SLEEP_ATTEMPT");
  PowerDiagnostics::logPowerState("pre-sleep-ulp");
  const unsigned long sleepCallStartMs = millis();
  Cloud::instance().logLedgerSleepState();
  setAppBreadcrumb(21);
  drainSerialBeforeSleep();
  SystemSleepResult result = System.sleep(config);
  const unsigned long sleepElapsedMs = millis() - sleepCallStartMs;
  pin_t wakePin = result.wakeupPin();
  const char *wakeReturnReason = (result.error() != SYSTEM_ERROR_NONE) ? "ERROR" :
      ((wakePin == BUTTON_PIN) ? "BUTTON" : ((wakePin == intPin) ? "PIR" : "TIMER"));

#if HAL_PLATFORM_CELLULAR
  if (useNetworkStandby) {
    if (result.error() == SYSTEM_ERROR_NONE) {
      preserveStandbyAfterNormalReturn = true;
      forceModemOffAfterStandbyFailure = false;
      standbySleepFailureError = SYSTEM_ERROR_NONE;
      if (sysStatus.get_verboseMode()) {
        Log.info("SLEEP: standby sleep returned normally; preserving modem standby");
      }
    } else {
      preserveStandbyAfterNormalReturn = false;
      forceModemOffAfterStandbyFailure = true;
      standbySleepFailureError = (int)result.error();
      Log.warn("SLEEP: standby sleep failed; forcing modem-off reason=sleep_error err=%d",
               standbySleepFailureError);
    }
  } else {
    preserveStandbyAfterNormalReturn = false;
    forceModemOffAfterStandbyFailure = false;
    standbySleepFailureError = SYSTEM_ERROR_NONE;
  }
#endif

  // If Device OS rejects the sleep configuration, System.sleep() returns
  // immediately and the wake pin will be invalid (65535). Without handling
  // that error, the state machine will interpret it as a timer wake and can
  // thrash in a tight loop.
  if (result.error() != SYSTEM_ERROR_NONE) {
    thrashGuard.markProgress("SLEEP_RETURNED");
    Log.error("ULTRA_LOW_POWER sleep failed err=%d (wakeIn=%d sec, button=%d pir=%d) - falling back to STOP", (int)result.error(), wakeInSeconds, (int)BUTTON_PIN, (int)intPin);
    current.raiseAlert(16);

   // STOP generally supports a wider set of wake pins on some platforms.
    setAppBreadcrumb(20);
    config = SystemSleepConfiguration();
    config.mode(SystemSleepMode::STOP)
      .gpio(BUTTON_PIN, FALLING)
      .gpio(intPin, RISING)
      .duration((uint32_t)wakeInSeconds * 1000UL);
    PowerDiagnostics::logPowerState("pre-sleep-stop-fallback");
    Cloud::instance().logLedgerSleepState();
    setAppBreadcrumb(21);
    drainSerialBeforeSleep();
    result = System.sleep(config);

    if (result.error() != SYSTEM_ERROR_NONE) {
      Log.error("STOP sleep fallback failed err=%d (wakeIn=%d sec) - using timer-only STOP sleep", (int)result.error(), wakeInSeconds);
      setAppBreadcrumb(20);
      config = SystemSleepConfiguration();
      config.mode(SystemSleepMode::STOP)
        .duration((uint32_t)wakeInSeconds * 1000UL);
      PowerDiagnostics::logPowerState("pre-sleep-stop-timer-only");
      Cloud::instance().logLedgerSleepState();
      setAppBreadcrumb(21);
      drainSerialBeforeSleep();
      result = System.sleep(config);

      if (result.error() != SYSTEM_ERROR_NONE) {
        // All sleep modes failed - this indicates a device state corruption
        // that requires immediate reset (alert 16 already raised on first failure).
        Log.error("All sleep attempts failed err=%d - immediate reset required", (int)result.error());
        ab1805.resumeWDT();
        
        // Brief delay to allow log output to flush before reset
        delay(2000);
        
        // Reset device to clear corrupted state
        Log.info("Resetting device to clear sleep failure state");
        System.reset();
        
        // Should never reach here, but set ERROR_STATE as fallback
        transitionTo(ERROR_STATE, "sleep-all-attempts-failed");
        return;
      }
    }
  }
  
  // Clear any pending interrupts on wake pins after sleep to ensure clean state
  // This is critical for proper interrupt handling after wake
  pinMode(BUTTON_PIN, INPUT);    // Reset pin mode
  pinMode(intPin, INPUT);         // Reset pin mode
  
  // Resume hardware watchdog immediately after wake
  ab1805.resumeWDT();
  restoreAwakeWatchdogAfterWake("ulp");

  // Mark progress immediately after wake to reset ThrashGuard timer
  thrashGuard.markProgress("WAKE_FROM_SLEEP");
  setAppBreadcrumb(4);

  if (sysStatus.get_serialConnected() || (ALLOW_BLOCKING_SERIAL_WAITS != 0)) {
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
  bool pirWake = (wakePin == intPin);
  bool buttonWake = (wakePin == BUTTON_PIN);
  bool timerWake = !pirWake && !buttonWake;  // If no GPIO woke us, it's the timer
  const char *wakeReasonLabel = buttonWake ? "BUTTON" : (pirWake ? "PIR" : "TIMER");

  // Observability: start of a new wake cycle (ULP return path).
  Observability::cycleStats().resetOnWake(millis());

  // Battery must be sampled before any later report/connect logic turns on the
  // modem, so mark the first post-wake sample for fuel-gauge stabilization now.
  measure.noteWakeFromLowPowerSleep();
  PowerDiagnostics::logPowerState("post-wake");
  
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
      if (sysStatus.get_lowBatteryMode()) {
        applyBatteryAwareConnectionModePolicy(current.get_stateOfCharge());
      }

      // In CONNECTED operating mode, the device should reconnect at the
      // start of open hours so it can resume normal connected behavior.
      if (sysStatus.get_connectionMode() == CONNECTED && !Particle.connected()) {
        logWakeSummary(wakeReasonLabel);
        transitionTo(CONNECTING_STATE, "sleep-open-hours-reconnect");
        return;
      }
    }

    // Check if LED timeout expired FIRST (before processing new PIR wake)
    // This ensures we don't immediately undo state changes from PIR processing
    if (sysStatus.get_sensorMode() == OCCUPANCY && signalLEDTimeRemaining() == 0 && signalLEDStatus()) {
      // LED timeout expired - debounce period elapsed without motion
      const bool reportNow = (sysStatus.get_connectionMode() == INTERMITTENT_KEEP_ALIVE);
      const OccupancyCloseResult closeResult = closeOccupancySessionSafely("sleep");
      signalLED(false);  // Turn off LED
      if (closeResult.valid) {
        logUnoccupiedEvent("debounce", closeResult.sessionSeconds, closeResult.totalSeconds, reportNow);
      }
      
      // Match the rest of the occupancy state machine: only KEEP_ALIVE mode
      // forces an immediate report/connect on occupancy transitions.
      if (reportNow) {
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
      if (sysStatus.get_sensorMode() == COUNTING) {
        current.set_hourlyCount(current.get_hourlyCount() + 1);
        current.set_dailyCount(current.get_dailyCount() + 1);
        current.set_lastCountTime(Time.now());
        signalLED(true, 1000);  // Flash for 1 second
        Log.info("Count detected from PIR wake - Hourly: %d, Daily: %d",
                 current.get_hourlyCount(), current.get_dailyCount());
        if (Serial && Serial.isConnected()) {
          Serial.printf("Count detected from PIR wake - Hourly: %d, Daily: %d\r\n",
                        current.get_hourlyCount(), current.get_dailyCount());
        }
      } else if (sysStatus.get_sensorMode() == OCCUPANCY) {
        // Occupancy mode: PIR wakes are expected behavior, not thrashing
        thrashGuard.markProgress("PIR_WAKE_OCCUPANCY");

        // If KEEP_ALIVE was temporarily disabled for low battery, refresh the
        // battery policy on wake so recovered power can restore the intended
        // occupancy behavior before we decide whether to report or sleep again.
        if (sysStatus.get_lowBatteryMode()) {
          applyBatteryAwareConnectionModePolicy(current.get_stateOfCharge());
        }
        
        if (!current.get_occupied()) {
          current.set_occupied(true);
          current.set_occupancyStartTime(Time.now());
          // Treat the PIR wake as an occupancy event for debounce purposes.
          // If lastOccupancyEvent is left at 0, the debounce logic that uses
          // millis()-based timing will immediately expire and mark UNOCCUPIED.
          current.set_lastOccupancyEvent(millis());

          // Keep LED on for the debounce window (sensor.setting1 is milliseconds).
          uint32_t debounceMs = sensorConfig.get_sensorSetting1();
          if (debounceMs == 0) {
            debounceMs = Config::occupancyDebounceMsForRuntime();
          }
          signalLED(true, debounceMs);
          const bool reportNow = (sysStatus.get_connectionMode() == INTERMITTENT_KEEP_ALIVE);
          logOccupiedEvent("pir-wake", debounceMs / 1000UL, reportNow);
          
          // Match the rest of the occupancy state machine: only KEEP_ALIVE mode
          // forces an immediate report/connect on occupancy transitions.
          if (reportNow) {
            session.occupancyChangeTriggered = true;
            setAppBreadcrumb(5);
            transitionTo(REPORTING_STATE, "sleep-pir-occupancy-report");
            return;
          }
        } else {
          // Already occupied - motion detected during debounce, restart timer
          uint32_t debounceMs = sensorConfig.get_sensorSetting1();
          if (debounceMs == 0) {
            debounceMs = Config::occupancyDebounceMsForRuntime();
          }
          signalLED(true, debounceMs);  // Restart LED timer
          logOccupiedEvent("pir-wake", debounceMs / 1000UL, false, true);
        }
        current.set_lastOccupancyEvent(millis());
      }
    }

    logWakeSummary(wakeReasonLabel);

    // For COUNTING mode: Turn off LED if flash completed during sleep
    if (sysStatus.get_sensorMode() == COUNTING && signalLEDTimeRemaining() == 0 && signalLEDStatus()) {
      signalLED(false);
    }

    // Timer wake = scheduled report. No checks, no gates.
    // We trust that the system timer woke us at the correct boundary.
    if (timerWake) {
      // In OCCUPANCY + INTERMITTENT_KEEP_ALIVE mode, suppress periodic reports
      // while occupied so occupancy=1 is only reported on 0->1 transition.
      if (sysStatus.get_sensorMode() == OCCUPANCY && current.get_occupied() &&
          sysStatus.get_connectionMode() == INTERMITTENT_KEEP_ALIVE) {
        transitionTo(SLEEPING_STATE, "sleep-timer-occupied-suppress-report");
        return;
      }

      transitionTo(REPORTING_STATE, "sleep-timer-report");
      return;
    }

    // For PIR wakes, check if reporting is also due (opportunistic reporting)
    if (pirWake && Time.isValid() && isWithinOpenHours()) {
      // In OCCUPANCY + INTERMITTENT_KEEP_ALIVE mode, do not opportunistically
      // report while occupied; PIR hits should only reset debounce.
      if (sysStatus.get_sensorMode() == OCCUPANCY && current.get_occupied() &&
          sysStatus.get_connectionMode() == INTERMITTENT_KEEP_ALIVE) {
        // Skip overdue-report check while occupied.
      } else {
      uint16_t intervalSec = Config::reportingIntervalSecForRuntime();

      time_t now = Time.now();
      time_t lastReport = sysStatus.get_lastReport();
      if (lastReport > 0 && (now - lastReport) >= intervalSec) {
        transitionTo(REPORTING_STATE, "sleep-pir-overdue-report");
        return;
      }
      }
    }

    // If PIR woke us in INTERMITTENT or DISCONNECTED mode and no report is needed,
    // return immediately to sleep. This check comes AFTER opportunistic reporting
    // so overdue reports are not missed.
    if (pirWake && sysStatus.get_connectionMode() != CONNECTED) {
      transitionTo(SLEEPING_STATE, "sleep-pir-return-to-sleep");
      return;
    }

    transitionTo(IDLE_STATE, "sleep-wake-idle");
  }
}
