#include "state/State_Common.h"
#include "Config.h"
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
  static unsigned long connectionStartTimeStamp; // When this connect attempt started
  static bool lastEnteredFromReporting = false;  // Whether we came from REPORTING_STATE
  static bool connectRequested = false;
  static bool postConnectDone = false;
  static unsigned long budgetMs = ConnectivityPolicy::CONNECT_BUDGET_DEFAULT_MS; // Connection timeout budget
  static unsigned long lastConnectHeartbeatMs = 0;
  static unsigned long lastConnectStatusLogMs = 0;
  constexpr unsigned long CONNECT_HEARTBEAT_MS = 30000UL;
  constexpr unsigned long CONNECT_STATUS_LOG_MS = 60000UL;

  if (state != oldState) {
    publishStateTransition();
    lastEnteredFromReporting = (oldState == REPORTING_STATE);
    sysStatus.set_lastConnectionDuration(0);
    connectionStartTimeStamp = millis();
    connectRequested = false;
    postConnectDone = false;
    lastConnectHeartbeatMs = 0;
    lastConnectStatusLogMs = 0;

    // ********** Connection Budget with Periodic Deep Attempts **********
    // Per Particle cellular docs: Must allow at least 5 minutes for IMSI cycling,
    // and periodically allow 11 minutes for full modem reset (occurs at 10 min mark).
    // Base budget: 300s (5 minutes) - adequate for normal connection + IMSI cycling
    // Deep budget: 660s (11 minutes) - allows full modem reset periodically
    
    const ConnectBudgetContext budgetContext = evaluateConnectBudget();
    budgetMs = budgetContext.budgetMs;
    if (budgetContext.configuredBudgetBelowMinimum) {
      Log.warn("Configured budget %us below 120s minimum, using 300s default",
               budgetContext.configuredBudgetSec);
    }

    if (budgetContext.allowDeepAttempt) {
      // Every 4th attempt (counter 0-3, resets at 3) OR when battery >50%,
      // allow 11 minutes for full modem reset
      if (budgetContext.attemptCounter >= ConnectivityPolicy::DEEP_ATTEMPT_COUNTER_THRESHOLD) {
        Log.info("Deep connection attempt #%d - allowing 11 min for modem reset",
                 budgetContext.attemptCounter + 1);
        sysStatus.set_connectionAttemptCounter(0);  // Reset counter after deep attempt
      } else {
        Log.info("Healthy battery (%.1f%%) - allowing 11 min connection budget",
                 (double)budgetContext.currentSoC);
      }
    } else {
      Log.trace("Normal connection attempt #%d - 5 min budget",
                budgetContext.attemptCounter + 1);
    }

    armActiveConnectAttempt(budgetMs);

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

  // Keep ThrashGuard informed during intentional long cellular connects.
  // Heartbeat is emitted only while the connect attempt is active and still
  // within the selected budget, so genuine over-budget stalls still timeout.
  if (state == CONNECTING_STATE && connectRequested && !Particle.connected() && elapsedMs < budgetMs) {
    const unsigned long nowMs = millis();
    if (lastConnectHeartbeatMs == 0 || (nowMs - lastConnectHeartbeatMs) >= CONNECT_HEARTBEAT_MS) {
      thrashGuard.markProgress("CONNECT_WAIT");
      lastConnectHeartbeatMs = nowMs;
    }
    if (elapsedMs >= CONNECT_STATUS_LOG_MS &&
      (lastConnectStatusLogMs == 0 || (nowMs - lastConnectStatusLogMs) >= CONNECT_STATUS_LOG_MS)) {
      Log.info("CONNECTING: waiting for cloud (%lu/%lu ms)",
               (unsigned long)elapsedMs,
               (unsigned long)budgetMs);
      lastConnectStatusLogMs = nowMs;
    }
  }

  if (Particle.connected()) {
    if (!postConnectDone) {
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
        if (!Cloud::instance().publishDataToLedger()) {
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
      state = FIRMWARE_UPDATE_STATE;
    } else if (session.returnToSleepAfterReport) {
      // After reporting occupied state in low-power mode, return to sleep to check debounce timeout
      session.returnToSleepAfterReport = false;
      state = SLEEPING_STATE;
    } else {
      state = IDLE_STATE;
    }
    return;
  }

  if (elapsedMs > budgetMs) {
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
    state = SLEEPING_STATE;
  }
}

// FIRMWARE_UPDATE_STATE: Stay connected for firmware/config updates
void handleFirmwareUpdateState() {
  // Track how long we've been in update mode so we can mirror the
  // Particle Wake-Publish-Sleep example behaviour: bound the time
  // spent waiting for an update before going back to sleep.
  static unsigned long firmwareUpdateStartMs = 0;

  if (state != oldState) {
    publishStateTransition();
    Log.info("Entering FIRMWARE_UPDATE_STATE - keeping device connected for updates");

    firmwareUpdateStartMs = millis();

    // Ensure cloud connection is requested
    if (!Particle.connected()) {
      Particle.connect();
    }
  }

  // Once connected, ensure configuration is loaded at least once
  if (Particle.connected()) {
    static bool configLoadedInUpdateMode = false;
    if (!configLoadedInUpdateMode) {
      Log.info("Connected in FIRMWARE_UPDATE_STATE - loading configuration from cloud");
      Cloud::instance().loadConfigurationFromCloud();
      configLoadedInUpdateMode = true;
    }

    // If no updates are pending anymore and no OTA in progress, exit update mode
    if (!System.updatesPending()) {
      Log.info("No updates pending - leaving FIRMWARE_UPDATE_STATE to IDLE_STATE");
      configLoadedInUpdateMode = false;
      state = IDLE_STATE;
      return;
    }
  }

  // Optional escape hatch: user button can also exit update mode
  if (!digitalRead(BUTTON_PIN)) { // Active-low user button
    Log.info("User button pressed - exiting FIRMWARE_UPDATE_STATE to IDLE_STATE");
    state = IDLE_STATE;
    return;
  }

  // Firmware update timeout: if we've spent more than firmwareUpdateMaxMs
  // in this state, mirror the reference example and go to sleep so we can
  // try again in a future cycle instead of keeping the modem on
  // indefinitely.
  if (firmwareUpdateStartMs != 0 && (millis() - firmwareUpdateStartMs) > firmwareUpdateMaxMs) {
    Log.info("Firmware update timed out after %lu ms in FIRMWARE_UPDATE_STATE - transitioning to SLEEPING_STATE",
             (unsigned long)(millis() - firmwareUpdateStartMs));
    state = SLEEPING_STATE;
  }
}
