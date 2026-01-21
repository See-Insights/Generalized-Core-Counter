#include "state/State_Common.h"
#include "Config.h"
#include "Cloud.h"
#include "DeviceInfoLedger.h"
#include "MyPersistentData.h"
#include "PublishQueuePosixRK.h"
#include "SensorManager.h"
#include "device_pinout.h"
#include "SensorDefinitions.h"

// NOTE:
// This file was split from StateHandlers.cpp as a mechanical refactor.
// No behavioral changes were made.

// Maximum amount of time to remain in FIRMWARE_UPDATE_STATE before
// giving up and returning to normal low-power operation. Mirrors the
// Wake-Publish-Sleep Cellular example, which uses a 5 minute budget
// for firmware updates before going back to sleep.
static const unsigned long firmwareUpdateMaxMs = 5UL * 60UL * 1000UL;

bool isRadioPoweredOn() {
#if Wiring_WiFi
  return WiFi.isOn();
#elif Wiring_Cellular
  return Cellular.isOn();
#else
  return false;
#endif
}

void requestRadioPowerOff() {
#if Wiring_Cellular
  Cellular.disconnect();
  Cellular.off();
#elif Wiring_WiFi
  WiFi.disconnect();
  WiFi.off();
#endif
}

void requestFullDisconnectAndRadioOff() {
  Particle.disconnect();
  requestRadioPowerOff();
}

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

  if (state != oldState) {
    publishStateTransition();
    lastEnteredFromReporting = (oldState == REPORTING_STATE);
    sysStatus.set_lastConnectionDuration(0);
    connectionStartTimeStamp = millis();
    connectRequested = false;
    postConnectDone = false;
  }

  unsigned long elapsedMs = millis() - connectionStartTimeStamp;
  sysStatus.set_lastConnectionDuration(int(elapsedMs / 1000));

  // Use a ledger-configured budget when available; otherwise fall back
  // to the compiled default maxConnectAttemptMs constant.
  unsigned long budgetMs = maxConnectAttemptMs;
  uint16_t budgetSec = sysStatus.get_connectAttemptBudgetSec();
  if (budgetSec >= 30 && budgetSec <= 900) {
    budgetMs = (unsigned long)budgetSec * 1000UL;
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
    Log.info("Starting connection attempt - Signal: S=%2.0f%% Q=%2.0f%%",
             (double)strengthPct, (double)qualityPct);
#endif
    Log.info("Requesting Particle cloud connection");
    Particle.connect();
    connectRequested = true;
  }

  if (Particle.connected()) {
    if (!postConnectDone) {
      connectedStartMs = millis();
      sysStatus.set_lastConnection(Time.now());
      if (current.get_alertCode() == 31) {
        Log.info("Connection successful - clearing alert 31");
        current.set_alertCode(0);
      }
      measure.getSignalStrength();
      measure.batteryState();
      Log.info("Enclosure temperature at connect: %4.2f C", (double)current.get_internalTempC());
      if (sysStatus.get_verboseMode()) {
        char data[64];
        snprintf(data, sizeof(data), "Connected in %i secs", sysStatus.get_lastConnectionDuration());
        publishDiagnosticSafe("Cellular", data, PRIVATE);
      }

      bool configOk = Cloud::instance().loadConfigurationFromCloud();
      if (!configOk) {
        Log.warn("Configuration apply failed (will raise alert 41)");
        current.raiseAlert(41);
      } else if (current.get_alertCode() == 41) {
        Log.info("Configuration apply succeeded - clearing stale alert 41");
        current.set_alertCode(0);
      }

      if (!lastEnteredFromReporting) {
        if (!Cloud::instance().publishDataToLedger()) {
          current.raiseAlert(42); // data ledger publish failure
        }
      }

      size_t pending = PublishQueuePosix::instance().getNumEvents();
      Log.info("Publish queue depth after connect: %u event(s)", (unsigned)pending);

      if (!firstConnectionObserved) {
        firstConnectionObserved = true;
        firstConnectionQueueDrainedLogged = false;
      }

      postConnectDone = true;
    }

    if (System.updatesPending()) {
      Log.info("Updates pending after connect - transitioning to FIRMWARE_UPDATE_STATE");
      state = FIRMWARE_UPDATE_STATE;
    } else {
      state = IDLE_STATE;
    }
    return;
  }

  if (elapsedMs > budgetMs) {
    Log.warn("Connection attempt exceeded budget (%lu ms > %lu ms) - raising alert 31",
             (unsigned long)elapsedMs, (unsigned long)budgetMs);
    current.raiseAlert(31);
    requestFullDisconnectAndRadioOff();
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
