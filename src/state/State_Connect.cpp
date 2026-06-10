#include "state/State_Common.h"
#include "../Config.h"
#include "cloud/Cloud.h"
#include "DeviceInfoLedger.h"
#include "MyPersistentData.h"
#include "PublishQueuePosixRK.h"
#include "sensors/SensorManager.h"
#include "device_pinout.h"
#include "sensors/SensorDefinitions.h"
#include "power/ConnectivityPolicy.h"
#include "power/Connectivity.h"
#include "observability/WakeCycleStats.h"
#include "ThrashGuard.h"

// NOTE:
// This file was split from StateHandlers.cpp as a mechanical refactor.
// No behavioral changes were made.

namespace {

constexpr uint8_t MODEM_UNSTABLE_CONNECT_TIMEOUT_THRESHOLD = 2;

enum class ConnAcquirePhase : uint8_t {
  CELLULAR_ACQUIRE = 0,
  NETWORK_ACQUIRE,
  CLOUD_ACQUIRE,
  CONNECTED,
};

const char *connAcquirePhaseLabel(ConnAcquirePhase phase) {
  switch (phase) {
  case ConnAcquirePhase::CELLULAR_ACQUIRE:
    return "CELLULAR_ACQUIRE";
  case ConnAcquirePhase::NETWORK_ACQUIRE:
    return "NETWORK_ACQUIRE";
  case ConnAcquirePhase::CLOUD_ACQUIRE:
    return "CLOUD_ACQUIRE";
  case ConnAcquirePhase::CONNECTED:
    return "CONNECTED";
  default:
    return "?";
  }
}

ConnAcquirePhase classifyConnAcquirePhase(bool cellularReady,
                                          bool networkReady,
                                          bool cloudConnected) {
  if (cloudConnected) {
    return ConnAcquirePhase::CONNECTED;
  }
  if (!cellularReady) {
    return ConnAcquirePhase::CELLULAR_ACQUIRE;
  }
  if (!networkReady) {
    return ConnAcquirePhase::NETWORK_ACQUIRE;
  }
  return ConnAcquirePhase::CLOUD_ACQUIRE;
}

void sampleConnectionSignal(int &strengthPct, int &qualityPct, bool &valid) {
  valid = false;
  strengthPct = -1;
  qualityPct = -1;
#if Wiring_Cellular
  CellularSignal sig = Cellular.RSSI();
  strengthPct = (int)(sig.getStrength() + 0.5f);
  qualityPct = (int)(sig.getQuality() + 0.5f);
  valid = true;
#elif Wiring_WiFi
  WiFiSignal sig = WiFi.RSSI();
  strengthPct = (int)(sig.getStrength() + 0.5f);
  qualityPct = (int)(sig.getQuality() + 0.5f);
  valid = true;
#endif
}

void requestCloudAcquireStage1Recovery() {
  Connectivity::requestCloudDisconnectOnly();
  Particle.connect();
}

void requestCloudAcquireStage2Recovery() {
#if Wiring_Cellular
  Connectivity::requestRadioPowerOff();
  Cellular.on();
  Particle.connect();
#else
  Particle.disconnect();
  Particle.connect();
#endif
}

struct ConnectBudgetContext {
  unsigned long budgetMs = ConnectivityPolicy::CONNECT_BUDGET_DEFAULT_MS;
  uint16_t configuredBudgetSec = 0;
  uint8_t attemptCounter = 0;
  float currentSoC = 0.0f;
  bool allowDeepAttempt = false;
  bool configuredBudgetBelowMinimum = false;
};

ConnectBudgetContext evaluateConnectBudget() {
  ConnectBudgetContext context;
  context.configuredBudgetSec = sysStatus.get_connectAttemptBudgetSec();
  if (context.configuredBudgetSec >= ConnectivityPolicy::CONNECT_BUDGET_CONFIG_MIN_SEC &&
      context.configuredBudgetSec <= ConnectivityPolicy::CONNECT_BUDGET_CONFIG_MAX_SEC) {
    context.budgetMs = (unsigned long)context.configuredBudgetSec * 1000UL;
  } else if (context.configuredBudgetSec > 0 &&
             context.configuredBudgetSec < ConnectivityPolicy::CONNECT_BUDGET_CONFIG_MIN_SEC) {
    context.configuredBudgetBelowMinimum = true;
    context.budgetMs = ConnectivityPolicy::CONNECT_BUDGET_DEFAULT_MS;
  }

  context.attemptCounter = sysStatus.get_connectionAttemptCounter();
  context.currentSoC = current.get_stateOfCharge();
  context.allowDeepAttempt =
      (context.attemptCounter >= ConnectivityPolicy::DEEP_ATTEMPT_COUNTER_THRESHOLD) ||
      (context.currentSoC > ConnectivityPolicy::DEEP_ATTEMPT_SOC_THRESHOLD);
  if (context.allowDeepAttempt) {
    context.budgetMs = ConnectivityPolicy::CONNECT_BUDGET_DEEP_MS;
  }

  return context;
}

void armActiveConnectAttempt(unsigned long budgetMs) {
  session.connectAttemptActive = true;
  session.connectAttemptBudgetMs = budgetMs;
}

void clearActiveConnectAttempt() {
  session.connectAttemptActive = false;
  session.connectAttemptBudgetMs = 0;
}

void markModemUnstableFromConnectTimeout() {
  if (session.modemConnectTimeoutCount < 0xFF) {
    session.modemConnectTimeoutCount++;
  }

  if (session.modemConnectTimeoutCount >= MODEM_UNSTABLE_CONNECT_TIMEOUT_THRESHOLD) {
    const bool reasonChanged = !session.modemUnstable || session.modemUnstableReason != 2;
    session.modemUnstable = true;
    session.modemUnstableReason = 2;
    if (reasonChanged) {
      Log.warn("MODEM_HEALTH: unstable reason=connect_timeout count=%u",
               (unsigned)session.modemConnectTimeoutCount);
    }
    if (!session.modemStandbySuppressed) {
      Log.warn("MODEM_POLICY: standby temporarily disabled reason=unstable_modem");
      session.modemStandbySuppressed = true;
    }
  }
}

} // namespace

bool activeConnectAttemptWithinBudget() {
  if (state != CONNECTING_STATE) {
    return false;
  }

  if (oldState != CONNECTING_STATE) {
    return true;
  }

  if (!session.connectAttemptActive) {
    return false;
  }

  const unsigned long budgetMs = session.connectAttemptBudgetMs;
  if (budgetMs == 0UL) {
    return true;
  }

  const unsigned long elapsedMs =
      (unsigned long)sysStatus.get_lastConnectionDuration() * 1000UL;
  return elapsedMs <= budgetMs;
}

// Maximum amount of time to remain in FIRMWARE_UPDATE_STATE before
// giving up and returning to normal low-power operation. Mirrors the
// Wake-Publish-Sleep Cellular example, which uses a 5 minute budget
// for firmware updates before going back to sleep.
static const unsigned long firmwareUpdateMaxMs = ConnectivityPolicy::FIRMWARE_UPDATE_MAX_MS;

/**
 * @brief CONNECTING_STATE: establish cloud connection using a phased,
 *        non-blocking state machine.
 *
 * @details Uses an internal ConnectPhase enum to break connection
 *          into small steps that each complete within a single loop()
 *          iteration:
 *            - CONN_PHASE_START: log signal strength and request
 *              Particle.connect().
 *            - CONN_PHASE_WAIT_CLOUD: poll Particle.connected() up to
 *              connectAttemptBudgetSec (from sysStatus / Ledger),
 *              raising alert 31 on timeout.
 *            - CONN_PHASE_LOAD_CONFIG: load configuration from cloud
 *              ledgers and raise alert 41 on failure.
 *            - CONN_PHASE_PUBLISH_LEDGER: optionally publish
 *              device-data to the ledger (skipped when entered from
 *              REPORTING_STATE to avoid clobbering hourlyCount), log
 *              queue depth, and transition to FIRMWARE_UPDATE_STATE
 *              when updates are pending or back to IDLE_STATE.
 *          Connection duration is tracked in sysStatus so budgets and
 *          field behaviour can be analysed from device-status data.
 */
void handleConnectingState() {
  setLoopStage(LOOP_STAGE_CONNECTIVITY);

  static unsigned long connectionStartTimeStamp; // When this connect attempt started
  static bool lastEnteredFromReporting = false;  // Whether we came from REPORTING_STATE
  static bool connectRequested = false;
  static bool postConnectDone = false;
  static unsigned long budgetMs = ConnectivityPolicy::CONNECT_BUDGET_DEFAULT_MS; // Connection timeout budget
  static bool deepAttemptThisConnect = false;
  static unsigned long lastConnectHeartbeatMs = 0;
  static unsigned long lastConnDiagLogMs = 0;
  static bool connPhaseInitialized = false;
  static ConnAcquirePhase lastConnPhase = ConnAcquirePhase::CELLULAR_ACQUIRE;
  static unsigned long phaseStartMs = 0;
  static uint32_t connPhaseCellMs = 0;
  static uint32_t connPhaseNetMs = 0;
  static uint32_t connPhaseCloudMs = 0;
  static uint8_t cloudRecoverStage = 0;
  static uint8_t cloudRecoverCount = 0;
  static bool cloudRecoverStage1Done = false;
  static bool cloudRecoverStage2Done = false;
  constexpr unsigned long CONNECT_HEARTBEAT_MS = 30000UL;
  constexpr unsigned long CONNECT_DIAG_LOG_MS = 30000UL;
  constexpr unsigned long CLOUD_RECOVER_STAGE1_MS = 60000UL;
  constexpr unsigned long CLOUD_RECOVER_STAGE2_MS = 120000UL;

  if (state != oldState) {
    publishStateTransition();
    lastEnteredFromReporting = (oldState == REPORTING_STATE);
    sysStatus.set_lastConnectionDuration(0);
    connectionStartTimeStamp = millis();
    connectRequested = false;
    postConnectDone = false;
    deepAttemptThisConnect = false;
    lastConnectHeartbeatMs = 0;
    lastConnDiagLogMs = 0;
    connPhaseInitialized = false;
    lastConnPhase = ConnAcquirePhase::CELLULAR_ACQUIRE;
    phaseStartMs = connectionStartTimeStamp;
    connPhaseCellMs = 0;
    connPhaseNetMs = 0;
    connPhaseCloudMs = 0;
    cloudRecoverStage = 0;
    cloudRecoverCount = 0;
    cloudRecoverStage1Done = false;
    cloudRecoverStage2Done = false;

    // ********** Connection Budget with Periodic Deep Attempts **********
    // Per Particle cellular docs: Must allow at least 5 minutes for IMSI cycling,
    // and periodically allow 11 minutes for full modem reset (occurs at 10 min mark).
    // Base budget: 300s (5 minutes) - adequate for normal connection + IMSI cycling
    // Deep budget: 660s (11 minutes) - allows full modem reset periodically
    
    const ConnectBudgetContext budgetContext = evaluateConnectBudget();
    budgetMs = budgetContext.budgetMs;
    deepAttemptThisConnect = budgetContext.allowDeepAttempt;
    if (budgetContext.configuredBudgetBelowMinimum) {
      Log.warn("Configured budget %us below 120s minimum, using 300s default",
               budgetContext.configuredBudgetSec);
    }

    if (budgetContext.allowDeepAttempt) {
      // Every 4th attempt (counter 0-3, resets at 3) OR when battery >50%,
      // allow 11 minutes for full modem reset
      if (budgetContext.attemptCounter >= ConnectivityPolicy::DEEP_ATTEMPT_COUNTER_THRESHOLD) {
#if ENABLE_CONNECT_DECISION_TRACE
        Log.info("Deep connection attempt #%d - allowing 11 min for modem reset",
                 budgetContext.attemptCounter + 1);
#endif
        sysStatus.set_connectionAttemptCounter(0);  // Reset counter after deep attempt
      } else {
#if ENABLE_CONNECT_DECISION_TRACE
        Log.info("Healthy battery (%.1f%%) - allowing 11 min connection budget",
                 (double)budgetContext.currentSoC);
#endif
      }
    } else {
#if ENABLE_CONNECT_DECISION_TRACE
      Log.trace("Normal connection attempt #%d - 5 min budget",
                budgetContext.attemptCounter + 1);
#endif
    }

    armActiveConnectAttempt(budgetMs);

  #if ENABLE_CONNECT_TRACE
    Log.info("CloudRecoverCfg: stage1=%lus stage2=%lus budget=%lus",
       (unsigned long)(CLOUD_RECOVER_STAGE1_MS / 1000UL),
       (unsigned long)(CLOUD_RECOVER_STAGE2_MS / 1000UL),
       (unsigned long)(budgetMs / 1000UL));
  #endif

    // Observability: record connect attempt type + final effective budget.
    {
      const uint16_t pending = (uint16_t)PublishQueuePosix::instance().getNumEvents();
      Observability::cycleStats().markConnectAttempt(
          budgetContext.allowDeepAttempt
              ? Observability::WakeCycleStats::ConnectAttemptType::DEEP
              : Observability::WakeCycleStats::ConnectAttemptType::NORMAL,
          (uint32_t)budgetMs,
          pending);
    }
  }

  unsigned long elapsedMs = millis() - connectionStartTimeStamp;
  sysStatus.set_lastConnectionDuration(int(elapsedMs / 1000));

  bool cellularReady = true;
#if Wiring_Cellular
  cellularReady = Cellular.ready();
#endif
  const bool networkReady = Network.ready();
  const bool cloudConnected = Particle.connected();
  const ConnAcquirePhase currentConnPhase =
      classifyConnAcquirePhase(cellularReady, networkReady, cloudConnected);
  const unsigned long phaseNowMs = millis();
  const unsigned long phaseElapsedMs = phaseNowMs - phaseStartMs;
  unsigned long cloudAcquireElapsedMs = connPhaseCloudMs;
  if (currentConnPhase == ConnAcquirePhase::CLOUD_ACQUIRE) {
    cloudAcquireElapsedMs = phaseElapsedMs;
  }

  auto addPhaseElapsed = [&](ConnAcquirePhase phase, uint32_t deltaMs) {
    switch (phase) {
    case ConnAcquirePhase::CELLULAR_ACQUIRE:
      connPhaseCellMs += deltaMs;
      break;
    case ConnAcquirePhase::NETWORK_ACQUIRE:
      connPhaseNetMs += deltaMs;
      break;
    case ConnAcquirePhase::CLOUD_ACQUIRE:
      connPhaseCloudMs += deltaMs;
      break;
    case ConnAcquirePhase::CONNECTED:
      break;
    }
  };

  if (!connPhaseInitialized) {
    connPhaseInitialized = true;
    lastConnPhase = currentConnPhase;
    phaseStartMs = phaseNowMs;
  } else if (currentConnPhase != lastConnPhase) {
    const uint32_t phaseDeltaMs = (uint32_t)(phaseNowMs - phaseStartMs);
    addPhaseElapsed(lastConnPhase, phaseDeltaMs);
#if ENABLE_CONNECT_TRACE
    Log.info("ConnPhase: %s->%s elapsed=%lu phaseElapsed=%lu",
             connAcquirePhaseLabel(lastConnPhase),
             connAcquirePhaseLabel(currentConnPhase),
             (unsigned long)elapsedMs,
             (unsigned long)phaseDeltaMs);
#endif
    lastConnPhase = currentConnPhase;
    phaseStartMs = phaseNowMs;
  }

  if (!connectRequested) {
    // Log signal strength at start of connection attempt for field
    // correlation with connectivity failures (alert 31). On cellular
    // platforms this gives us a baseline RSSI before the modem fully
    // connects, helping diagnose poor-reception issues.
#if Wiring_Cellular
    CellularSignal sig = Cellular.RSSI();
    float strengthPct = sig.getStrength();
    float qualityPct = sig.getQuality();
    (void)strengthPct;
    (void)qualityPct;
#else
#endif
    Log.info("Connect: start budget=%lus heap=%lu",
         (unsigned long)(budgetMs / 1000UL),
         (unsigned long)System.freeMemory());
    setAppBreadcrumb(6);
    Particle.connect();
    connectRequested = true;
    thrashGuard.markProgress("CONNECT_START");

    // Observability: mark connect request start timestamp (millis-based).
    Observability::cycleStats().markConnectRequested(connectionStartTimeStamp);
  }

  if (state == CONNECTING_STATE && connectRequested && !cloudConnected &&
      currentConnPhase == ConnAcquirePhase::CLOUD_ACQUIRE) {
    const bool stage1WasDoneAtLoopStart = cloudRecoverStage1Done;
    if (!cloudRecoverStage1Done && phaseElapsedMs >= CLOUD_RECOVER_STAGE1_MS) {
      cloudRecoverStage1Done = true;
      cloudRecoverStage = 1;
      cloudRecoverCount = 1;
      Log.warn("CloudRecover: stage=1 action=particle-reconnect phaseElapsed=%lu elapsed=%lu",
               (unsigned long)phaseElapsedMs,
               (unsigned long)elapsedMs);
      requestCloudAcquireStage1Recovery();
      lastConnectHeartbeatMs = phaseNowMs;
      lastConnDiagLogMs = phaseNowMs;
    }

    if (stage1WasDoneAtLoopStart && !cloudRecoverStage2Done &&
        phaseElapsedMs >= CLOUD_RECOVER_STAGE2_MS) {
      cloudRecoverStage2Done = true;
      cloudRecoverStage = 2;
      cloudRecoverCount = 2;
      Log.warn("CloudRecover: stage=2 action=network-reset phaseElapsed=%lu elapsed=%lu",
               (unsigned long)phaseElapsedMs,
               (unsigned long)elapsedMs);
      requestCloudAcquireStage2Recovery();
      lastConnectHeartbeatMs = phaseNowMs;
      lastConnDiagLogMs = phaseNowMs;
    }
  }

  // Keep ThrashGuard informed during intentional long cellular connects.
  // Heartbeat is emitted only while the connect attempt is active and still
  // within the selected budget, so genuine over-budget stalls still timeout.
  if (state == CONNECTING_STATE && connectRequested && !cloudConnected && elapsedMs < budgetMs) {
    const unsigned long nowMs = millis();
    if (lastConnectHeartbeatMs == 0 || (nowMs - lastConnectHeartbeatMs) >= CONNECT_HEARTBEAT_MS) {
      thrashGuard.markProgress("CONNECT_WAIT");
      lastConnectHeartbeatMs = nowMs;
    }
    if (lastConnDiagLogMs == 0 || (nowMs - lastConnDiagLogMs) >= CONNECT_DIAG_LOG_MS) {
      int sigStrength = -1;
      int sigQuality = -1;
      bool sigValid = false;
      sampleConnectionSignal(sigStrength, sigQuality, sigValid);
  #if ENABLE_CONNECT_TRACE
      const uint16_t queueDepth = (uint16_t)PublishQueuePosix::instance().getNumEvents();
      const bool standbyRequested =
          (sysStatus.get_connectionMode() == INTERMITTENT_KEEP_ALIVE) &&
          isWithinOpenHours();
      const bool standbyEffective = standbyRequested && !session.modemStandbySuppressed;
  #else
      (void)sigStrength;
      (void)sigQuality;
      (void)sigValid;
      (void)deepAttemptThisConnect;
  #endif

      if (sigValid) {
    #if ENABLE_CONNECT_TRACE
        Log.info("ConnDiag: elapsed=%lu/%lu phase=%s phaseElapsed=%lu cell=%d net=%d cloud=%d sig=%d/%d heap=%lu q=%u deep=%d standby=%d/%d",
                 (unsigned long)elapsedMs,
                 (unsigned long)budgetMs,
                 connAcquirePhaseLabel(currentConnPhase),
           (unsigned long)phaseElapsedMs,
                 cellularReady ? 1 : 0,
                 networkReady ? 1 : 0,
                 cloudConnected ? 1 : 0,
                 sigStrength,
                 sigQuality,
                 (unsigned long)System.freeMemory(),
                 (unsigned)queueDepth,
                 deepAttemptThisConnect ? 1 : 0,
                 standbyRequested ? 1 : 0,
                 standbyEffective ? 1 : 0);
#endif
      } else {
#if ENABLE_CONNECT_TRACE
        Log.info("ConnDiag: elapsed=%lu/%lu phase=%s phaseElapsed=%lu cell=%d net=%d cloud=%d sig=na heap=%lu q=%u deep=%d standby=%d/%d",
                 (unsigned long)elapsedMs,
                 (unsigned long)budgetMs,
                 connAcquirePhaseLabel(currentConnPhase),
           (unsigned long)phaseElapsedMs,
                 cellularReady ? 1 : 0,
                 networkReady ? 1 : 0,
                 cloudConnected ? 1 : 0,
                 (unsigned long)System.freeMemory(),
                 (unsigned)queueDepth,
                 deepAttemptThisConnect ? 1 : 0,
                 standbyRequested ? 1 : 0,
                 standbyEffective ? 1 : 0);
#endif
      }
      lastConnDiagLogMs = nowMs;
    }
  }

  if (cloudConnected) {
    if (!postConnectDone) {
      const uint32_t phaseDeltaMs = (uint32_t)(millis() - phaseStartMs);
      addPhaseElapsed(lastConnPhase, phaseDeltaMs);

      if (cloudRecoverCount > 0) {
        Log.info("CloudRecover: success stage=%u elapsed=%lu cloudMs=%lu",
                 (unsigned)cloudRecoverStage,
                 (unsigned long)elapsedMs,
                 (unsigned long)cloudAcquireElapsedMs);
      }

      int summarySigStrength = -1;
      int summarySigQuality = -1;
      bool summarySigValid = false;
      sampleConnectionSignal(summarySigStrength, summarySigQuality, summarySigValid);
      const uint16_t summaryQueueDepth = (uint16_t)PublishQueuePosix::instance().getNumEvents();
      if (summarySigValid) {
        Log.info("ConnSummary: ok elapsed=%lu last=%s cellMs=%lu netMs=%lu cloudMs=%lu cloudRecoverStage=%u cloudRecoverCount=%u sig=%d/%d heap=%lu q=%u",
                 (unsigned long)elapsedMs,
                 connAcquirePhaseLabel(lastConnPhase),
                 (unsigned long)connPhaseCellMs,
                 (unsigned long)connPhaseNetMs,
                 (unsigned long)connPhaseCloudMs,
                 (unsigned)cloudRecoverStage,
                 (unsigned)cloudRecoverCount,
                 summarySigStrength,
                 summarySigQuality,
                 (unsigned long)System.freeMemory(),
                 (unsigned)summaryQueueDepth);
      } else {
        Log.info("ConnSummary: ok elapsed=%lu last=%s cellMs=%lu netMs=%lu cloudMs=%lu cloudRecoverStage=%u cloudRecoverCount=%u sig=na heap=%lu q=%u",
                 (unsigned long)elapsedMs,
                 connAcquirePhaseLabel(lastConnPhase),
                 (unsigned long)connPhaseCellMs,
                 (unsigned long)connPhaseNetMs,
                 (unsigned long)connPhaseCloudMs,
                 (unsigned)cloudRecoverStage,
                 (unsigned)cloudRecoverCount,
                 (unsigned long)System.freeMemory(),
                 (unsigned)summaryQueueDepth);
      }

      thrashGuard.markProgress("CLOUD_CONNECTED");
      setAppBreadcrumb(7);
      connectedStartMs = millis();
      sysStatus.set_lastConnection(Time.now());
      clearConnectivityFailsafeRecovery("cloud-ok");

      // Observability: connect succeeded + begin service window.
      Observability::cycleStats().markConnectSuccess(
          (uint32_t)(connectedStartMs - connectionStartTimeStamp),
          (uint16_t)PublishQueuePosix::instance().getNumEvents(),
          sysStatus.get_lastConnection());
      Observability::cycleStats().markServiceStart((uint32_t)connectedStartMs);

      // Short-term webhook supervision: start the response window only after
      // a successful cloud connection, and only if a webhook publish was queued.
      if (session.webhookExpectedOnConnect) {
        session.webhookExpectedOnConnect = false;
        session.awaitingWebhookResponse = true;
        session.webhookAwaitStartMs = millis();
      }
      
      // Increment connection attempt counter for periodic deep attempts
      // (unless we just did a deep attempt which already reset counter to 0)
      uint8_t attemptCounter = sysStatus.get_connectionAttemptCounter();
      if (attemptCounter < ConnectivityPolicy::DEEP_ATTEMPT_COUNTER_THRESHOLD) {
        sysStatus.set_connectionAttemptCounter(attemptCounter + 1);
        Log.trace("Connection attempt counter: %d → %d", attemptCounter, attemptCounter + 1);
      }
      
      if (current.get_alertCode() == 31) {
        current.set_alertCode(0);
      }
      measure.batteryState();
      if (sysStatus.get_verboseMode()) {
        char data[64];
        snprintf(data, sizeof(data), "Connected in %i secs", sysStatus.get_lastConnectionDuration());
        publishDiagnosticSafe("Cellular", data, PRIVATE);
      }

      Cloud::instance().logLedgerConnectState();

      bool configOk = Cloud::instance().loadConfigurationFromCloud();
      if (!configOk) {
        // Enhanced diagnostics for Alert 41 troubleshooting
        bool ledgersSynced = Cloud::instance().areLedgersSynced();
        unsigned long connectDuration = millis() - connectedStartMs;
        Log.warn("Configuration apply failed (will raise alert 41)");
        Log.warn("Alert 41 context: ledgersSynced=%s connectDuration=%lu ms",
                 ledgersSynced ? "true" : "false",
                 connectDuration);
        current.raiseAlert(41);
      } else {
        thrashGuard.markProgress("LEDGER_SYNC_OK");
      }

      bool ledgerPublishOk = true;
      if (!lastEnteredFromReporting) {
        if (!Cloud::instance().publishDataToLedger("ConnectState")) {
          current.raiseAlert(42); // data ledger publish failure
          ledgerPublishOk = false;
        }
      }

      // Boot-storm alert is intended as a one-shot incident marker.
      // After one successful post-boot cloud service pass, clear it.
      if (current.get_alertCode() == 17 && configOk && ledgerPublishOk) {
        Log.info("Boot-storm recovery verified - clearing alert 17");
        current.set_alertCode(0);
        current.set_lastAlertTime(0);
      }

      size_t pending = PublishQueuePosix::instance().getNumEvents();
    #if Wiring_Cellular
      {
        CellularSignal sig = Cellular.RSSI();
        Log.info("Connect: ok elapsed=%lums sig=%.0f/%.0f q=%u heap=%lu",
             (unsigned long)(connectedStartMs - connectionStartTimeStamp),
             (double)sig.getStrength(),
             (double)sig.getQuality(),
             (unsigned)pending,
             (unsigned long)System.freeMemory());
      }
    #elif Wiring_WiFi
      {
        WiFiSignal sig = WiFi.RSSI();
        Log.info("Connect: ok elapsed=%lums sig=%.0f/%.0f q=%u heap=%lu",
             (unsigned long)(connectedStartMs - connectionStartTimeStamp),
             (double)sig.getStrength(),
             (double)sig.getQuality(),
             (unsigned)pending,
             (unsigned long)System.freeMemory());
      }
    #else
      Log.info("Connect: ok elapsed=%lums q=%u heap=%lu",
           (unsigned long)(connectedStartMs - connectionStartTimeStamp),
           (unsigned)pending,
           (unsigned long)System.freeMemory());
    #endif

      if (!session.firstConnectionObserved) {
        session.firstConnectionObserved = true;
        session.firstConnectionQueueDrainedLogged = false;
      }

      postConnectDone = true;

      if (session.modemConnectTimeoutCount != 0) {
        session.modemConnectTimeoutCount = 0;
      }
    }

    thrashGuard.markProgress("OTA_CHECKED");
    clearActiveConnectAttempt();

    if (System.updatesPending()) {
      transitionTo(FIRMWARE_UPDATE_STATE, "updates-pending");
    } else if (session.returnToSleepAfterReport) {
      // After reporting occupied state in low-power mode, return to sleep to check debounce timeout
      session.returnToSleepAfterReport = false;
      transitionTo(SLEEPING_STATE, "return-to-sleep-after-report");
    } else {
      transitionTo(IDLE_STATE, "connect-complete");
    }
    return;
  }

  if (elapsedMs > budgetMs) {
    const uint32_t phaseDeltaMs = (uint32_t)(millis() - phaseStartMs);
    addPhaseElapsed(lastConnPhase, phaseDeltaMs);

    if (cloudRecoverCount > 0) {
      Log.warn("CloudRecover: exhausted stage=%u elapsed=%lu cloudMs=%lu",
               (unsigned)cloudRecoverStage,
               (unsigned long)elapsedMs,
               (unsigned long)cloudAcquireElapsedMs);
    }

    int summarySigStrength = -1;
    int summarySigQuality = -1;
    bool summarySigValid = false;
    sampleConnectionSignal(summarySigStrength, summarySigQuality, summarySigValid);
    const uint16_t summaryQueueDepth = (uint16_t)PublishQueuePosix::instance().getNumEvents();
    if (summarySigValid) {
      Log.warn("ConnSummary: fail elapsed=%lu last=%s cellMs=%lu netMs=%lu cloudMs=%lu cloudRecoverStage=%u cloudRecoverCount=%u sig=%d/%d heap=%lu q=%u",
               (unsigned long)elapsedMs,
               connAcquirePhaseLabel(lastConnPhase),
               (unsigned long)connPhaseCellMs,
               (unsigned long)connPhaseNetMs,
               (unsigned long)connPhaseCloudMs,
               (unsigned)cloudRecoverStage,
               (unsigned)cloudRecoverCount,
               summarySigStrength,
               summarySigQuality,
               (unsigned long)System.freeMemory(),
               (unsigned)summaryQueueDepth);
    } else {
      Log.warn("ConnSummary: fail elapsed=%lu last=%s cellMs=%lu netMs=%lu cloudMs=%lu cloudRecoverStage=%u cloudRecoverCount=%u sig=na heap=%lu q=%u",
               (unsigned long)elapsedMs,
               connAcquirePhaseLabel(lastConnPhase),
               (unsigned long)connPhaseCellMs,
               (unsigned long)connPhaseNetMs,
               (unsigned long)connPhaseCloudMs,
               (unsigned)cloudRecoverStage,
               (unsigned)cloudRecoverCount,
               (unsigned long)System.freeMemory(),
               (unsigned)summaryQueueDepth);
    }

    Log.warn("Connect: fail elapsed=%lums budget=%lums heap=%lu",
             (unsigned long)elapsedMs,
             (unsigned long)budgetMs,
             (unsigned long)System.freeMemory());

    // Observability: connect attempt timed out.
    Observability::cycleStats().markConnectTimeout((uint32_t)elapsedMs);

    markModemUnstableFromConnectTimeout();

    current.raiseAlert(31);
    Connectivity::requestFullDisconnectAndRadioOff();
    clearActiveConnectAttempt();
    transitionTo(SLEEPING_STATE, "connect-timeout");
  }
}

// FIRMWARE_UPDATE_STATE: Stay connected for firmware/config updates
void handleFirmwareUpdateState() {
  // Track how long we've been in update mode so we can mirror the
  // Particle Wake-Publish-Sleep example behaviour: bound the time
  // spent waiting for an update before going back to sleep.
  static unsigned long firmwareUpdateStartMs = 0;
  static bool configLoadedInUpdateMode = false;

  if (state != oldState) {
    publishStateTransition();
    Log.info("Entering FIRMWARE_UPDATE_STATE - keeping device connected for updates");

    firmwareUpdateStartMs = millis();
    configLoadedInUpdateMode = false;

    // Ensure cloud connection is requested
    if (!Particle.connected()) {
      Particle.connect();
    }
  }

  // Once connected, ensure configuration is loaded at least once
  if (Particle.connected()) {
    if (!configLoadedInUpdateMode) {
      Log.info("Connected in FIRMWARE_UPDATE_STATE - loading configuration from cloud");
      Cloud::instance().loadConfigurationFromCloud();
      configLoadedInUpdateMode = true;
    }

    // If no updates are pending anymore and no OTA in progress, exit update mode
    if (!System.updatesPending()) {
      Log.info("No updates pending - leaving FIRMWARE_UPDATE_STATE to IDLE_STATE");
      configLoadedInUpdateMode = false;
      transitionTo(IDLE_STATE, "firmware-update-complete");
      return;
    }
  }

  // Optional escape hatch: user button can also exit update mode
  if (!digitalRead(BUTTON_PIN)) { // Active-low user button
    Log.info("User button pressed - exiting FIRMWARE_UPDATE_STATE to IDLE_STATE");
    transitionTo(IDLE_STATE, "firmware-update-button-exit");
    return;
  }

  // Firmware update timeout: if we've spent more than firmwareUpdateMaxMs
  // in this state, mirror the reference example and go to sleep so we can
  // try again in a future cycle instead of keeping the modem on
  // indefinitely.
  if (firmwareUpdateStartMs != 0 && (millis() - firmwareUpdateStartMs) > firmwareUpdateMaxMs) {
    Log.info("Firmware update timed out after %lu ms in FIRMWARE_UPDATE_STATE - transitioning to SLEEPING_STATE",
             (unsigned long)(millis() - firmwareUpdateStartMs));
    transitionTo(SLEEPING_STATE, "firmware-update-timeout");
  }
}
