#include "Particle.h"
#include "Cloud.h"
#include "LocalTimeRK.h"
#include "MyPersistentData.h"
#include "PublishQueuePosixRK.h"
#include "SensorManager.h"
#include "device_pinout.h"
#include "SensorDefinitions.h"
#include "StateHandlers.h"

// Maximum amount of time to remain in FIRMWARE_UPDATE_STATE before
// giving up and returning to normal low-power operation. Mirrors the
// Wake-Publish-Sleep Cellular example, which uses a 5 minute budget
// for firmware updates before going back to sleep.
static const unsigned long firmwareUpdateMaxMs = 5UL * 60UL * 1000UL;

// Forward declaration for internal error supervisor helper
static int resolveErrorAction();

// IDLE_STATE: Awake, monitoring sensor and deciding what to do next
void handleIdleState() {
  if (state != oldState) {
    publishStateTransition();
  }

  // ********** Counting Mode / Trigger Behaviour **********
  // COUNTING and OCCUPANCY use interrupt-driven handlers.
  // SCHEDULED uses time-based sampling (non-interrupt).
  uint8_t countingMode = sysStatus.get_countingMode();

  if (countingMode == SCHEDULED) {
    if (Time.isValid()) {
      static time_t lastScheduledSample = 0;
      uint16_t intervalSec = sysStatus.get_reportingInterval();
      if (intervalSec == 0) {
        intervalSec = 3600; // Fallback to 1 hour
      }

      time_t now = Time.now();
      if (lastScheduledSample == 0 || (now - lastScheduledSample) >= intervalSec) {
        measure.batteryState();
        Log.info("Scheduled trigger sample - battery SoC: %4.2f%%", (double)current.get_stateOfCharge());
        lastScheduledSample = now;
      }
    }
  } else {
    // ********** Mode-Specific Sensor Handling (Interrupt-driven) **********
    // Call the appropriate handler based on counting mode configuration
    if (countingMode == COUNTING) {
      handleCountingMode();  // Count each sensor event
    } else if (countingMode == OCCUPANCY) {
      handleOccupancyMode(); // Track occupied/unoccupied state
    }
  }

  // ********** First-connection queue drain visibility **********
  // After the first successful cloud connection, log once when the
  // publish queue has fully drained so we can confirm that any
  // pending offline events (for example, from before boot) have
  // been flushed.
  if (firstConnectionObserved && !firstConnectionQueueDrainedLogged && Particle.connected()) {
    if (PublishQueuePosix::instance().getCanSleep() &&
        PublishQueuePosix::instance().getNumEvents() == 0) {
      Log.info("First connection queue drained - all pending events flushed");
      firstConnectionQueueDrainedLogged = true;
    }
  }

  // ********** Scheduled Reporting **********
  // Use the configured reportingIntervalSec to determine when to
  // generate a periodic report, regardless of trigger mode.
  if (Time.isValid()) {
    uint16_t intervalSec = sysStatus.get_reportingInterval();
    if (intervalSec == 0) {
      intervalSec = 3600; // Fallback to 1 hour
    }

    time_t now = Time.now();
    time_t lastReport = sysStatus.get_lastReport();
    if (lastReport == 0 || (now - lastReport) >= intervalSec) {
      state = REPORTING_STATE;
      return;
    }
  }

  // ********** Power Management **********
  if (sysStatus.get_lowPowerMode()) {
    // Honour a post-wake or post-connect stay-awake window so that
    // technicians have a brief period to interact with the user
    // button or observe behaviour before the device immediately
    // returns to low-power sleep.
    if (stayAwake > 0 && (millis() - stayAwakeTimeStamp) < stayAwake) {
      // Skip low-power sleep gating during this window.
      return;
    }
    bool updatesPending = sysStatus.get_updatesPending() || System.updatesPending();

    // If we've been online longer than the allowed work window, treat this
    // connection as best-effort complete and force a sleep so we don't
    // remain connected indefinitely during backend outages.
    if (onlineWorkStartMs != 0 && Particle.connected() &&
        (millis() - onlineWorkStartMs) > maxOnlineWorkMs) {
      size_t pending = PublishQueuePosix::instance().getNumEvents();
      Log.warn("Online work window exceeded (%lu ms) with %u pending event(s) - forcing sleep",
               (unsigned long)(millis() - onlineWorkStartMs),
               (unsigned)pending);
      forceSleepThisCycle = true;
      current.raiseAlert(43); // publish queue not drained before forced sleep
      state = SLEEPING_STATE;
      return;
    }

    // In low-power mode, once all work for this connection cycle is
    // complete (no updates pending), we can safely enter SLEEPING_STATE
    // to turn off the radio and save power. We only require the publish
    // queue to be fully drained when we are actually connected; when
    // offline, it's expected to have a non-zero queue and we still want
    // to sleep, flushing the queue on the next connection.
    bool canSleepGate = true;
    if (Particle.connected()) {
      canSleepGate = PublishQueuePosix::instance().getCanSleep();
    }

    if (!updatesPending && canSleepGate) {
      // If a sensor event is still pending or the BLUE LED timer is
      // active from a recent count, defer transitioning into the
      // SLEEPING_STATE. This avoids rapid Idle<->Sleeping ping-pong
      // and the associated extra logging while still honouring the
      // low-power policy once the indication has finished.
      if (sensorDetect || countSignalTimer.isActive()) {
        return;
      }

      size_t pending = PublishQueuePosix::instance().getNumEvents();
      if (!Particle.connected() && pending > 0) {
        Log.info("Low-power idle: offline with %u queued event(s) - sleeping and will flush on next connect",
                 (unsigned)pending);
      } else {
        Log.info("Low-power idle: queue drained and no updates pending - entering SLEEPING_STATE");
      }
      state = SLEEPING_STATE;
      return; // Go back to sleep when there's no work this hour
    }
  }
}

/**
 * @brief SLEEPING_STATE: deep sleep between reporting intervals.
 *
 * @details Performs a non-blocking, phased disconnect before entering
 *          Device OS sleep modes. While connected (or with the radio
 *          still on) it gates sleep on the publish queue being in a
 *          sleep-safe state unless forceSleepThisCycle is set. The
 *          disconnect sequence uses an internal DisconnectPhase state
 *          machine to:
 *            - Request cloud disconnect and wait up to
 *              cloudDisconnectBudgetSec (from sysStatus / Ledger).
 *            - Power down the modem (Cellular/WiFi) and wait up to
 *              modemOffBudgetSec.
 *          If either budget is exceeded, alert 15 is raised and
 *          control transfers to ERROR_STATE so the error supervisor
 *          can decide on recovery. Once offline, the handler selects
 *          HIBERNATE or ULTRA_LOW_POWER based on opening hours and
 *          validates that long night sleeps are honoured.
 */
void handleSleepingState() {
  bool radioOn = false; // Flag to indicate if the radio is on
#if Wiring_WiFi
  radioOn = WiFi.ready(); // Check if the WiFi is ready
#elif Wiring_Cellular
  radioOn = Cellular.ready(); // Check if the Cellular is ready
#endif

  if (state != oldState) {
    publishStateTransition();
  }

  // If we are connected (or the radio is still on) and the publish
  // queue is not yet in a sleep-safe state (events queued or a publish
  // in progress), defer sleeping so we can finish delivering data while
  // the radio is available. When forceSleepThisCycle is set, we bypass
  // this gate to avoid staying online indefinitely during backend
  // outages. When offline with the radio off, we allow sleep even with
  // a non-zero queue; events will be flushed on the next connection.
  bool gateOnQueue = !forceSleepThisCycle && (Particle.connected() || radioOn);
  if (gateOnQueue && !PublishQueuePosix::instance().getCanSleep()) {
    size_t pending = PublishQueuePosix::instance().getNumEvents();
    Log.info("Deferring sleep - publish queue has %u pending event(s) or publish in progress",
             (unsigned)pending);
    state = IDLE_STATE;
    return;
  }
  
  // ********** Non-blocking disconnect sequence **********
  // If we are connected to the cloud or the radio is still on, perform a
  // phased disconnect with explicit timeouts so we never block the
  // application thread. If already offline with radio off, skip straight
  // to sleep configuration.
  enum DisconnectPhase {
    DISC_PHASE_IDLE,
    DISC_PHASE_REQUEST_DISCONNECT,
    DISC_PHASE_WAIT_CLOUD_OFF,
    DISC_PHASE_REQUEST_MODEM_OFF,
    DISC_PHASE_WAIT_MODEM_OFF,
    DISC_PHASE_DONE
  };

  static DisconnectPhase disconnectPhase = DISC_PHASE_IDLE;
  static unsigned long discPhaseStartMs = 0;

  // Use ledger-configured budgets for disconnect phases when available,
  // falling back to conservative defaults if values are out of range.
  uint16_t cloudBudgetSec = sysStatus.get_cloudDisconnectBudgetSec();
  if (cloudBudgetSec < 5 || cloudBudgetSec > 120) {
    cloudBudgetSec = 15; // default 15 seconds
  }
  const unsigned long cloudDisconnectBudgetMs = (unsigned long)cloudBudgetSec * 1000UL;

  uint16_t modemBudgetSec = sysStatus.get_modemOffBudgetSec();
  if (modemBudgetSec < 5 || modemBudgetSec > 120) {
    modemBudgetSec = 30; // default 30 seconds
  }
  const unsigned long modemOffBudgetMs = (unsigned long)modemBudgetSec * 1000UL;

  bool needDisconnect = Particle.connected() || radioOn;

  if (state != oldState) {
    // Reset phase machine on first entry to SLEEPING_STATE
    disconnectPhase = DISC_PHASE_IDLE;
  }

  if (needDisconnect) {
    switch (disconnectPhase) {
    case DISC_PHASE_IDLE:
      // Start by requesting a cloud disconnect if currently connected
      if (Particle.connected()) {
        Log.info("Requesting Particle cloud disconnect before sleep");
        Particle.disconnect();
      }
      discPhaseStartMs = millis();
      disconnectPhase = DISC_PHASE_WAIT_CLOUD_OFF;
      return; // Allow state machine to advance on subsequent loop iterations

    case DISC_PHASE_WAIT_CLOUD_OFF:
      if (!Particle.connected()) {
        // Cloud is disconnected; proceed to modem power down if radio is on
#if Wiring_Cellular || Wiring_WiFi
        if (radioOn) {
          Log.info("Cloud disconnected - powering down modem before sleep");
          discPhaseStartMs = millis();
          disconnectPhase = DISC_PHASE_REQUEST_MODEM_OFF;
          return;
        }
#endif
        // No radio to power down; disconnect complete
        disconnectPhase = DISC_PHASE_DONE;
        break;
      }

      if (millis() - discPhaseStartMs > cloudDisconnectBudgetMs) {
        Log.warn("Cloud disconnect exceeded budget (%lu ms) - raising alert 15",
                 (unsigned long)(millis() - discPhaseStartMs));
        current.raiseAlert(15);
        state = ERROR_STATE;
        disconnectPhase = DISC_PHASE_IDLE;
        return;
      }
      return; // Still waiting for cloud to disconnect

    case DISC_PHASE_REQUEST_MODEM_OFF:
#if Wiring_Cellular
      Log.info("Disconnecting from cellular network and powering down modem");
      Cellular.disconnect();
      Cellular.off();
#elif Wiring_WiFi
      Log.info("Disconnecting from WiFi network and powering down modem");
      WiFi.disconnect();
      WiFi.off();
#endif
      discPhaseStartMs = millis();
      disconnectPhase = DISC_PHASE_WAIT_MODEM_OFF;
      return;

    case DISC_PHASE_WAIT_MODEM_OFF: {
      bool modemOff = true;
#if Wiring_Cellular
      modemOff = !Cellular.isOn();
#elif Wiring_WiFi
      modemOff = !WiFi.isOn();
#endif
      if (modemOff) {
        Log.info("Modem powered down successfully before sleep");
        disconnectPhase = DISC_PHASE_DONE;
        break;
      }

      if (millis() - discPhaseStartMs > modemOffBudgetMs) {
        Log.warn("Modem power-down exceeded budget (%lu ms) - raising alert 15",
                 (unsigned long)(millis() - discPhaseStartMs));
        current.raiseAlert(15);
        state = ERROR_STATE;
        disconnectPhase = DISC_PHASE_IDLE;
        return;
      }
      return; // Still waiting for modem to power down
    }

    case DISC_PHASE_DONE:
      // Disconnect completed; fall through to sleep configuration below.
      break;
    }
  }

  int nightSleepSec = -1;
  if (!isWithinOpenHours()) {
    // Notify sensor layer we are entering full night sleep so sensors and
    // indicator LEDs can be powered down. During daytime naps we keep
    // interrupt-driven sensors (like PIR) powered so they can wake the
    // device from ULTRA_LOW_POWER sleep.
    SensorManager::instance().onEnterSleep();

    // ********** Night sleep (outside opening hours) **********
    nightSleepSec = secondsUntilNextOpen();
    if (nightSleepSec <= 0) {
      nightSleepSec = 3600; // Fallback
    }

    // Device OS maximum sleep duration is 546 minutes (~9.1 hours).
    // Clamp our requested night sleep to this limit so the
    // underlying platform will reliably honour it.
    const int MAX_SLEEP_SEC = 546 * 60;
    if (nightSleepSec > MAX_SLEEP_SEC) {
      Log.info("Clamping night sleep duration to max supported %d seconds (requested=%d)", MAX_SLEEP_SEC, nightSleepSec);
      nightSleepSec = MAX_SLEEP_SEC;
    }

    // First attempt a true HIBERNATE so platforms that support it
    // still get a cold boot at next opening time.
    if (!hibernateDisabledForSession) {
      Log.info("Outside opening hours - entering NIGHT HIBERNATE sleep for %d seconds", nightSleepSec);

      ab1805.stopWDT();
      config.mode(SystemSleepMode::HIBERNATE)
        .gpio(BUTTON_PIN, CHANGE)
        .duration((uint32_t)nightSleepSec * 1000UL);

      // HIBERNATE should reset the device on wake, so execution should
      // not resume here under normal conditions.
      System.sleep(config);

      // If we reach this point, HIBERNATE did not reset as expected on
      // this hardware/OS combination. Log once, raise an alert, and
      // permanently disable HIBERNATE for the remainder of this boot so
      // we can fall back to ULTRA_LOW_POWER instead of thrashing.
      ab1805.resumeWDT();
      Log.error("HIBERNATE sleep returned unexpectedly - disabling HIBERNATE for this session");
      current.raiseAlert(16); // Alert: unexpected return from HIBERNATE
      hibernateDisabledForSession = true;
      // Fall through to ULTRA_LOW_POWER fallback below.
    }
  }

  // ********** ULTRA_LOW_POWER sleep (daytime or night fallback) **********
  // During opening hours we use the reportingIntervalSec as before.
  // Outside opening hours, if HIBERNATE is disabled or unsupported,
  // fall back to ULTRA_LOW_POWER with a long sleep equal to the
  // time until next open to avoid rapid wake/sleep thrashing.
  uint16_t intervalSec = sysStatus.get_reportingInterval();
  if (intervalSec == 0) {
    intervalSec = 1 * 3600; // Preserve 1 hour default if unset
  }

  int wakeInSeconds;
  if (!isWithinOpenHours() && nightSleepSec > 0) {
    wakeInSeconds = nightSleepSec;
    Log.info("Outside opening hours - using ULTRA_LOW_POWER fallback sleep for %d seconds", wakeInSeconds);
  } else {
    // Within opening hours, or we don't have a night window computed.
    wakeInSeconds = (int)intervalSec;
  }

  // If a sensor event is pending or the BLUE LED timer is still
  // active from a recent count, defer entering deep sleep so we
  // don't cut off in-progress events or visible indications.
  if (sensorDetect || countSignalTimer.isActive()) {
    Log.info("Deferring sleep - sensor event or LED timer active");
    state = IDLE_STATE;
    return;
  }

  config.mode(SystemSleepMode::ULTRA_LOW_POWER)
    .gpio(BUTTON_PIN, CHANGE)
    .gpio(intPin, RISING)
    .duration(wakeInSeconds * 1000L);

  // Capture wall-clock time around sleep so we can detect ULTRA_LOW_POWER
  // calls that return immediately during the long night window.
  time_t sleepStart = Time.isValid() ? Time.now() : 0;

  ab1805.stopWDT(); // No watchdogs interrupting our slumber
  SystemSleepResult result = System.sleep(config); // Put the device to sleep
  ab1805.resumeWDT();       // Wakey Wakey - WDT can resume

  // If this wake was caused by the PIR interrupt pin, turn on the
  // BLUE LED immediately so motion-triggered wakes are visible with
  // minimal latency, even before serial logging resumes.
  bool pirWake = (result.wakeupPin() == intPin);
  if (pirWake) {
    digitalWrite(BLUE_LED, HIGH);
  }

  // When debugging over USB, the host often disconnects/reconnects the
  // serial port across ULTRA_LOW_POWER sleep. Give it a brief window to
  // reattach before emitting logs so wake events are visible.
  if (sysStatus.get_serialConnected()) {
    waitFor(Serial.isConnected, 5000);
  }

  time_t sleepEnd = Time.isValid() ? Time.now() : 0;

  Log.info("Woke from ULTRA_LOW_POWER sleep: reason=%d, pin=%d",
           (int)result.wakeupReason(), (int)result.wakeupPin());

  // Detect clearly abnormal ULTRA_LOW_POWER wakes during the long
  // night-sleep window: we requested a long nightSleepSec duration
  // but the wall-clock time advanced by less than a minute. This
  // indicates that deep sleep is not being honoured and we should
  // escalate to the error supervisor instead of looping.
  if (!isWithinOpenHours() && nightSleepSec > 0 &&
      wakeInSeconds == nightSleepSec &&
      sleepStart != 0 && sleepEnd != 0) {
    time_t slept = sleepEnd - sleepStart;
    if (slept < 60) {
      Log.error("Unexpected short ULTRA_LOW_POWER sleep during night window (slept=%ld s, expected=%d s) - raising alert 16",
                (long)slept, wakeInSeconds);
      current.raiseAlert(16);
      onlineWorkStartMs = 0;
      forceSleepThisCycle = false;
      state = ERROR_STATE;
      return;
    }
  }

  // Reset per-connection online window and forced-sleep flag after waking
  onlineWorkStartMs = 0;
  forceSleepThisCycle = false;

  if (result.wakeupPin() == BUTTON_PIN) {
    // If the user woke the device we need to get up - device
    // was sleeping so we need to reset opening hours
    // User button always wakes sensor stack regardless of hours.
    SensorManager::instance().onExitSleep();
    Log.info("Woke with user button - Resetting hours and going to connect");
    sysStatus.set_lowPowerMode(false);
    sysStatus.set_operatingMode(CONNECTED); // Treat button wake as service/connected mode
    userSwitchDetected = false;
    stayAwake = stayAwakeLong;
    stayAwakeTimeStamp = millis();
    state = REPORTING_STATE;
  } else {
    // In this state the device was awoken for hourly reporting or PIR
    // Re-enable sensors only if within opening hours; otherwise they
    // remain powered down to minimize sleep current.
    if (isWithinOpenHours()) {
      SensorManager::instance().onExitSleep();
    } else {
      Log.info("Woke outside opening hours; keeping sensors powered down");
    }

    // If this wake was caused by the PIR interrupt, synthesize a single
    // detection event so that the motion that woke the device is counted
    // even if the ISR flag did not survive ULTRA_LOW_POWER sleep.
    if (pirWake) {
      if (sysStatus.get_countingMode() == COUNTING) {
        current.set_hourlyCount(current.get_hourlyCount() + 1);
        current.set_dailyCount(current.get_dailyCount() + 1);
        current.set_lastCountTime(Time.now());
        Log.info("Count detected from PIR wake - Hourly: %d, Daily: %d",
                 current.get_hourlyCount(), current.get_dailyCount());
      } else if (sysStatus.get_countingMode() == OCCUPANCY) {
        if (!current.get_occupied()) {
          current.set_occupied(true);
          current.set_occupancyStartTime(Time.now());
          Log.info("Space now OCCUPIED from PIR wake at %s", Time.timeStr().c_str());
        }
        current.set_lastOccupancyEvent(millis());
      }
    }

    // If this wake was from PIR, keep the BLUE LED on using the
    // same software timer used for normal count events so the
    // visible behaviour is consistent.
    if (pirWake) {
      digitalWrite(BLUE_LED, HIGH);
      countSignalTimer.reset();
    }
    state = IDLE_STATE;
  }
}

// REPORTING_STATE: Build and send periodic report
void handleReportingState() {
  if (state != oldState) {
    publishStateTransition();
  }

  time_t now = Time.now();
  // If this is the first report after a calendar *local* day boundary,
  // run the daily cleanup once to reset daily counters and housekeeping.
  if (Time.isValid()) {
    time_t lastReport = sysStatus.get_lastReport();
    if (lastReport != 0) {
      LocalTimeConvert convNow;
      convNow.withConfig(LocalTime::instance().getConfig()).withTime(now).convert();
      LocalTimeConvert convLast;
      convLast.withConfig(LocalTime::instance().getConfig()).withTime(lastReport).convert();

      LocalTimeYMD ymdNow = convNow.getLocalTimeYMD();
      LocalTimeYMD ymdLast = convLast.getLocalTimeYMD();

      if (ymdNow.getYear() != ymdLast.getYear() ||
          ymdNow.getMonth() != ymdLast.getMonth() ||
          ymdNow.getDay() != ymdLast.getDay()) {
        Log.info("New local day detected (last=%04d-%02d-%02d, current=%04d-%02d-%02d) - running dailyCleanup",
                 ymdLast.getYear(), ymdLast.getMonth(), ymdLast.getDay(),
                 ymdNow.getYear(), ymdNow.getMonth(), ymdNow.getDay());
        dailyCleanup();
        sysStatus.set_lastDailyCleanup(now);
      }
    }
  }

  sysStatus.set_lastReport(now);

  // Read battery state BEFORE connectivity decision so SoC-tiered
  // logic below uses fresh data, not stale values from a previous
  // cycle or from during an active radio session.
  measure.loop();         // Take sensor measurements for reporting
  measure.batteryState(); // Update battery SoC/state and enclosure temperature

  Log.info("Enclosure temperature at report: %4.2f C", (double)current.get_internalTempC());
  publishData(); // Queue hourly report; actual send depends on connectivity policy

  // After each hourly report, reset the hourly counter so
  // the next report contains only the counts for that hour.
  if (sysStatus.get_countingMode() == COUNTING) {
    Log.info("Resetting hourlyCount after report (was %d)", current.get_hourlyCount());
    current.set_hourlyCount(0);
  }

  // Webhook supervision: if we have not seen a successful webhook
  // response in more than 6 hours, raise alert 40 so the error
  // supervisor can evaluate and potentially reset. Threshold is set
  // higher for remote/solar units in poor reception where intermittent
  // connectivity is expected.
  if (Time.isValid()) {
    time_t lastHook = sysStatus.get_lastHookResponse();
    if (lastHook != 0 && (now - lastHook) > (6 * 3600L)) {
      Log.info("No successful webhook response for >6 hours (last=%ld, now=%ld) - raising alert 40",
               (long)lastHook, (long)now);
      current.raiseAlert(40);
    }
  }

  // ********** Connectivity Decision (SoC-tiered) **********
  bool shouldConnect = true;

  // Explicit disconnected mode: never auto-connect
  if (sysStatus.get_disconnectedMode()) {
    shouldConnect = false;
    Log.info("Disconnected mode enabled - not connecting after report");
  } else {
    // Apply SoC-based tiers when in low-power mode
    if (sysStatus.get_lowPowerMode()) {
      int soc = current.get_stateOfCharge();
      int hour = Time.isValid() ? Time.hour() : -1;

      shouldConnect = false; // Start from conservative default

      if (soc > 65) {
        Log.info("SoC %d%% > 65 - connecting this hour", soc);
        shouldConnect = true; // Normal behaviour: connect every hour
      } else if (soc > 50) {
        if (hour < 0 || (hour % 2) == 0) {
          Log.info("SoC %d%% in 50-65%% tier - connecting on 2-hour cadence", soc);
          shouldConnect = true;
        } else {
          Log.info("SoC %d%% in 50-65%% tier - skipping this hour", soc);
        }
      } else if (soc > 35) {
        if (hour < 0 || (hour % 4) == 0) {
          Log.info("SoC %d%% in 35-50%% tier - connecting on 4-hour cadence", soc);
          shouldConnect = true;
        } else {
          Log.info("SoC %d%% in 35-50%% tier - skipping this hour", soc);
        }
      } else if (soc > 20) {
        Log.info("SoC %d%% in 20-35%% tier - store-only, no auto-connect", soc);
      } else {
        Log.info("SoC %d%% <= 20%% - emergency-only, no auto-connect", soc);
      }
    }

    // User button override (active-low): always allow service connection
    if (!digitalRead(BUTTON_PIN)) {
      Log.info("User button override - forcing connection");
      shouldConnect = true;
    }
  }

  if (shouldConnect && !Particle.connected()) {
    Log.info("Transitioning to CONNECTING_STATE after report");
    state = CONNECTING_STATE;
  } else {
    state = IDLE_STATE; // Either already connected or intentionally staying offline
  }

  // If a webhook supervision alert (40) has been raised, we leave the
  // state machine to proceed with its normal connection behaviour. The
  // error supervisor can still evaluate alert 40 via resolveErrorAction,
  // but we no longer override the connection decision here, so the
  // device can continue to attempt hourly connections.
  if (current.get_alertCode() == 40) {
    Log.info("Alert 40 active after report - continuing normal state flow (no immediate ERROR_STATE)");
  }
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

  enum ConnectPhase {
    CONN_PHASE_IDLE,
    CONN_PHASE_START,
    CONN_PHASE_WAIT_CLOUD,
    CONN_PHASE_LOAD_CONFIG,
    CONN_PHASE_PUBLISH_LEDGER,
    CONN_PHASE_COMPLETE
  };

  static ConnectPhase connectPhase = CONN_PHASE_IDLE;
  char data[64]; // Holder for message strings

  if (state != oldState) {
    // One-time actions when entering CONNECTING_STATE
    publishStateTransition();
    lastEnteredFromReporting = (oldState == REPORTING_STATE);
    sysStatus.set_lastConnectionDuration(0);
    connectionStartTimeStamp = millis();
    connectPhase = CONN_PHASE_START;
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

  switch (connectPhase) {
  case CONN_PHASE_START: {
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
    Particle.connect(); // Ask Device OS to connect; we'll poll status
    connectPhase = CONN_PHASE_WAIT_CLOUD;
    break;
  }

  case CONN_PHASE_WAIT_CLOUD:
    if (Particle.connected()) {
      // Connection established within budget
      sysStatus.set_lastConnection(Time.now());
      stayAwake = stayAwakeLong;
      stayAwakeTimeStamp = millis();
      measure.getSignalStrength();
      measure.batteryState();
      Log.info("Enclosure temperature at connect: %4.2f C", (double)current.get_internalTempC());
      snprintf(data, sizeof(data), "Connected in %i secs", sysStatus.get_lastConnectionDuration());
      Log.info(data);
      if (sysStatus.get_verboseMode()) {
        Particle.publish("Cellular", data, PRIVATE);
      }

      // Track when we first became cloud-connected for this connection cycle
      onlineWorkStartMs = millis();
      forceSleepThisCycle = false;

      connectPhase = CONN_PHASE_LOAD_CONFIG;
    } else if (elapsedMs > budgetMs) {
      // Connection timeout based on per-wake budget; record a connectivity
      // alert and let the error handler decide what to do next. If multiple
      // alerts are present, the highest-severity one is preserved.
      Log.warn("Connection attempt exceeded budget (%lu ms > %lu ms) - raising alert 31",
               (unsigned long)elapsedMs, (unsigned long)budgetMs);
      current.raiseAlert(31);
      state = ERROR_STATE;
      connectPhase = CONN_PHASE_IDLE;
    }
    break;

  case CONN_PHASE_LOAD_CONFIG: {
    // Load configuration from cloud ledgers after connecting
    bool configOk = Cloud::instance().loadConfigurationFromCloud();
    if (!configOk) {
      // Configuration or status ledger apply failure; record as an alert so
      // the error supervisor can evaluate and potentially reset.
      current.raiseAlert(41);
    }
    connectPhase = CONN_PHASE_PUBLISH_LEDGER;
    break;
  }

  case CONN_PHASE_PUBLISH_LEDGER: {
    // After a successful connect, publish the latest counters and
    // temperature to the device-data ledger so the cloud view reflects
    // current state even if a regular hourly report has not yet run.
    //
    // However, if we *just* came from REPORTING_STATE, publishData()
    // has already updated the device-data ledger with the correct
    // hourlyCount and then reset the in-memory hourly counter to 0.
    // Calling publishDataToLedger() again here would overwrite the
    // freshly published hourlyCount with 0, which is not desired.
    if (!lastEnteredFromReporting) {
      if (!Cloud::instance().publishDataToLedger()) {
        current.raiseAlert(42); // data ledger publish failure
      }
    }

    // Log the current publish queue depth so we can observe whether
    // events are being drained while connected.
    size_t pending = PublishQueuePosix::instance().getNumEvents();
    Log.info("Publish queue depth after connect: %u event(s)", (unsigned)pending);

    // Mark that we've observed the first successful connection this
    // boot so later we can log when any pending events have been
    // fully flushed.
    if (!firstConnectionObserved) {
      firstConnectionObserved = true;
      firstConnectionQueueDrainedLogged = false;
    }

    // If updates are pending for this device or an OTA is queued, enter
    // FIRMWARE_UPDATE_STATE instead of going idle so we stay online until
    // updates are handled.
    if (System.updatesPending() || sysStatus.get_updatesPending()) {
      Log.info("Updates pending after connect - transitioning to FIRMWARE_UPDATE_STATE");
      state = FIRMWARE_UPDATE_STATE;
    } else {
      state = IDLE_STATE; // Return to idle once connected and configured
    }

    connectPhase = CONN_PHASE_COMPLETE;
    break;
  }

  case CONN_PHASE_COMPLETE:
  case CONN_PHASE_IDLE:
  default:
    // Nothing to do; state transition will move us out of CONNECTING_STATE.
    break;
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

    // In update mode, force connected operating mode and disable low power
    sysStatus.set_lowPowerMode(false);
    sysStatus.set_operatingMode(CONNECTED);

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
    if (!sysStatus.get_updatesPending() && !System.updatesPending()) {
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

// Decide what corrective action to take for the current alert.
//
// Uses the current alert code and resetCount to choose between:
//  - 0: No action (return to IDLE and try again later)
//  - 2: Soft reset via System.reset()
//  - 3: Hard recovery using AB1805 deep power down
//
// The mapping is intentionally conservative to avoid thrashing:
//  - Out-of-memory (14): up to 3 soft resets, then stop resetting.
//  - Modem/disconnect failure (15) and connect timeout (31):
//    a couple of soft resets, then a hard power-cycle, then stop.
//  - Sleep failures (16): soft reset, then hard power-cycle, then stop.
static int resolveErrorAction() {
  int8_t alert   = current.get_alertCode();
  uint8_t resets = sysStatus.get_resetCount();

  if (alert <= 0) {
    return 0;
  }

  switch (alert) {
  case 14: // out-of-memory
    if (resets >= 3) {
      Log.info("OOM alert but reset count=%u; suppressing further resets", resets);
      return 0;
    }
    return 2; // soft reset

  case 15: // modem or disconnect failure
  case 31: // failed to connect to cloud
    if (resets >= 4) {
      Log.info("Connectivity alert %d with reset count=%u; suppressing further resets", alert, resets);
      return 0;
    }
    if (resets >= 2) {
      return 3; // escalate to hard power-cycle after a few soft resets
    }
    return 2;   // start with soft reset

  case 40: { // repeated webhook failures
    if (!Time.isValid()) {
      Log.info("Alert 40 set but time is invalid - deferring corrective action");
      return 0;
    }

    time_t lastHook = sysStatus.get_lastHookResponse();
    if (lastHook == 0) {
      Log.info("Alert 40 set but no recorded lastHookResponse - deferring corrective action");
      return 0;
    }

    time_t now = Time.now();
    if ((now - lastHook) > (3 * 3600L)) {
      Log.info("Alert 40 - no successful webhook response for >3 hours, scheduling soft reset");
      return 2; // soft reset to try to recover integration path
    }

    Log.info("Alert 40 active but webhook response is recent - no reset needed");
    return 0;
  }

  case 16: { // repeated sleep failures (HIBERNATE / ULTRA_LOW_POWER)
    // If both HIBERNATE and ULTRA_LOW_POWER are failing to honour
    // long sleep requests, treat this as a platform-level fault.
    // Start with a soft reset, then escalate to an AB1805 deep
    // power-down, and finally stop resetting to avoid thrash.
    if (resets >= 4) {
      Log.info("Alert 16 with reset count=%u; suppressing further resets", resets);
      return 0;
    }
    if (resets >= 1) {
      return 3; // after first reset, try a hard power-cycle
    }
    return 2;   // start with a soft reset
  }

  default:
    // Unknown or less critical alert: don't take drastic action here.
    return 0;
  }
}

// ERROR_STATE: Error supervisor: decide recovery action with backoff
void handleErrorState() {
  static unsigned long resetTimer = 0;
  static int resolution = 0;

  if (state != oldState) {
    publishStateTransition();
    resolution = resolveErrorAction();
    Log.info("Entering ERROR_STATE with alert=%d, resetCount=%u, resolution=%d",
             current.get_alertCode(), sysStatus.get_resetCount(), resolution);
    resetTimer = millis();
  }

  switch (resolution) {
  case 0:
    // No automatic recovery; return to IDLE so the normal state machine
    // can continue and we rely on future hourly reports to surface the
    // issue.
    state = IDLE_STATE;
    break;

  case 2:
    // Soft reset after a short delay to allow any queued publishes to
    // flush.
    if (millis() - resetTimer > resetWait) {
      Log.info("Executing soft reset from ERROR_STATE");
      System.reset();
    }
    break;

  case 3:
    // Hard recovery using AB1805 deep power down after delay. This fully
    // power-cycles the device and modem but is limited by resolveErrorAction
    // backoff to avoid thrashing.
    if (millis() - resetTimer > resetWait) {
      Log.info("Executing deep power down from ERROR_STATE");
      ab1805.deepPowerDown();
    }
    break;

  default:
    // Should not happen, but don't get stuck here.
    state = IDLE_STATE;
    break;
  }
}

// *************** Mode-Specific Handler Functions ***************

/**
 * @brief Handle sensor events in COUNTING mode
 *
 * @details In counting mode, each sensor detection increments counters.
 *          Counts are tracked hourly and daily.
 *          Used for: traffic counting, people counting, event tracking
 */
void handleCountingMode() {
  // Check if sensor has new data
  if (SensorManager::instance().loop()) {
    // Increment counters
    current.set_hourlyCount(current.get_hourlyCount() + 1);
    current.set_dailyCount(current.get_dailyCount() + 1);
    current.set_lastCountTime(Time.now());

    // Log the new count once per event
    Log.info("Count detected - Hourly: %d, Daily: %d",
             current.get_hourlyCount(), current.get_dailyCount());

    // Flash the on-module BLUE LED for ~1 second as a
    // visual count indicator using a software timer so we
    // don't block the main loop.
    digitalWrite(BLUE_LED, HIGH);
    countSignalTimer.reset();

    // Stay in IDLE_STATE; hourly reporting will publish aggregated counts.
  }
}

/**
 * @brief Handle sensor events in OCCUPANCY mode
 *
 * @details In occupancy mode, first detection marks space as "occupied".
 *          Space remains occupied until debounce timeout expires without new detections.
 *          Tracks total occupied time for reporting.
 *          Used for: room occupancy, parking space detection, resource availability
 */
void handleOccupancyMode() {
  // Check if sensor has new data
  if (SensorManager::instance().loop()) {
    // Sensor detected presence
    if (!current.get_occupied()) {
      // Transition from unoccupied to occupied
      current.set_occupied(true);
      current.set_occupancyStartTime(Time.now());

      Log.info("Space now OCCUPIED at %s", Time.timeStr().c_str());
      digitalWrite(BLUE_LED, HIGH); // Visual indicator
    }

    // Update last event time (resets debounce timer)
    current.set_lastOccupancyEvent(millis());

    if (sysStatus.get_verboseMode()) {
      uint32_t occupiedDuration = Time.now() - current.get_occupancyStartTime();
      Log.info("Occupancy event - Duration: %lu seconds", occupiedDuration);
    }
  }

  // Check if we need to update occupancy state (timeout check)
  updateOccupancyState();
}

/**
 * @brief Update occupancy state based on debounce timeout
 *
 * @details If space is occupied and debounce timeout has expired without
 *          new sensor events, mark space as unoccupied.
 *          Accumulates total occupied time for daily reporting.
 */
void updateOccupancyState() {
  if (!current.get_occupied()) {
    return; // Nothing to do if not occupied
  }

  uint32_t debounceMs = sysStatus.get_occupancyDebounceMs();
  uint32_t timeSinceLastEvent = millis() - current.get_lastOccupancyEvent();

  // Check if debounce timeout has expired
  if (timeSinceLastEvent > debounceMs) {
    // Calculate this occupancy session duration
    uint32_t sessionDuration = Time.now() - current.get_occupancyStartTime();

    // Add to total occupied seconds for the day
    uint32_t totalOccupied = current.get_totalOccupiedSeconds() + sessionDuration;
    current.set_totalOccupiedSeconds(totalOccupied);

    // Mark as unoccupied
    current.set_occupied(false);
    current.set_occupancyStartTime(0);

    Log.info("Space now UNOCCUPIED - Session duration: %lu seconds, Total today: %lu seconds",
             sessionDuration, totalOccupied);

    digitalWrite(BLUE_LED, LOW); // Turn off visual indicator
  }
}
