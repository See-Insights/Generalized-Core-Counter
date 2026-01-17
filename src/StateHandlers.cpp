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

// Forward declarations
static int resolveErrorAction();
extern bool publishDiagnosticSafe(const char* eventName, const char* data, PublishFlags flags);

static bool isRadioPoweredOn() {
#if Wiring_WiFi
  return WiFi.isOn();
#elif Wiring_Cellular
  return Cellular.isOn();
#else
  return false;
#endif
}

static void requestRadioPowerOff() {
#if Wiring_Cellular
  Cellular.disconnect();
  Cellular.off();
#elif Wiring_WiFi
  WiFi.disconnect();
  WiFi.off();
#endif
}

static void requestFullDisconnectAndRadioOff() {
  Particle.disconnect();
  requestRadioPowerOff();
}

static void ensureSensorEnabled(const char* context) {
  if (SensorManager::instance().isSensorReady()) {
    return;
  }

  Log.info("%s - enabling sensor", context);
  SensorManager::instance().onExitSleep();
  if (!SensorManager::instance().isSensorReady()) {
    SensorManager::instance().initializeFromConfig();
  }
  Log.info("%s - sensorReady=%s", context, SensorManager::instance().isSensorReady() ? "true" : "false");
}

// IDLE_STATE: Awake, monitoring sensor and deciding what to do next
void handleIdleState() {
  if (state != oldState) {
    publishStateTransition();
  }

  // If configuration changes (for example, device-settings ledger updates)
  // move the park from CLOSED->OPEN while the device is already awake,
  // ensure the sensor stack is enabled. Previously this only happened on
  // wake from sleep, which caused "awake but not counting".
  {
    static bool enableAttemptedThisOpenWindow = false;
    bool openNow = isWithinOpenHours();

    if (!openNow) {
      enableAttemptedThisOpenWindow = false;
    } else if (!SensorManager::instance().isSensorReady() && !enableAttemptedThisOpenWindow) {
      enableAttemptedThisOpenWindow = true;
      ensureSensorEnabled("IDLE: park OPEN but sensorReady=false");
    }
  }

  // ********** CONNECTED Mode Park-Hours Policy **********
  // In CONNECTED operating mode, the device stays awake during park open hours.
  // When the park is closed, it should disconnect, power down the sensor, and
  // deep-sleep until the next opening time.
  if (Time.isValid() && sysStatus.get_operatingMode() == CONNECTED) {
    if (!isWithinOpenHours()) {
      Log.info("CONNECTED mode: park CLOSED - transitioning to SLEEPING_STATE for overnight sleep");
      state = SLEEPING_STATE;
      return;
    }
    // Park is open: remain awake in CONNECTED mode.

  }

  // ********** Scheduled Mode Sampling **********
  // SCHEDULED mode uses time-based sampling (non-interrupt).
  // Interrupt-driven modes (COUNTING/OCCUPANCY) are handled centrally in main loop().
  if (sysStatus.get_countingMode() == SCHEDULED) {
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
  if (Time.isValid() && isWithinOpenHours()) {
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
  // In LOW_POWER (1) or DISCONNECTED (2) modes, manage connection lifecycle.
  if (sysStatus.get_operatingMode() != CONNECTED) {
    // In LOW_POWER or DISCONNECTED modes, enforce maximum connected time.
    // Use connectAttemptBudgetSec as the max connected duration.
    if (Particle.connected() && connectedStartMs != 0) {
      uint16_t budgetSec = sysStatus.get_connectAttemptBudgetSec();
      if (budgetSec >= 30 && budgetSec <= 900) {
        unsigned long connectedMs = millis() - connectedStartMs;
        unsigned long budgetMs = (unsigned long)budgetSec * 1000UL;
        if (connectedMs > budgetMs) {
          Log.info("Connection timeout (%lu ms > %lu ms) - returning to sleep",
                   (unsigned long)connectedMs, (unsigned long)budgetMs);
          connectedStartMs = 0;
          state = SLEEPING_STATE;
          return;
        }
      }
    }

    // In CONNECTED mode during open hours, never auto-sleep.
    if (Time.isValid() && sysStatus.get_operatingMode() == CONNECTED && isWithinOpenHours()) {
      return;
    }

    bool updatesPending = System.updatesPending();

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
 *          sleep-safe state. The
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
  bool enteredState = (state != oldState);
  bool ignoreDisconnectFailure = false;

  if (enteredState) {
    publishStateTransition();
    // One-time diagnostic on entry so logs clearly show the device's view of park hours.
    if (Time.isValid()) {
      LocalTimeConvert conv;
      conv.withConfig(LocalTime::instance().getConfig()).withCurrentTime().convert();
      uint8_t hour = (uint8_t)(conv.getLocalTimeHMS().toSeconds() / 3600);
      Log.info("SLEEP entry: parkHours %02u-%02u localHour=%02u => %s",
               sysStatus.get_openTime(), sysStatus.get_closeTime(), hour,
               isWithinOpenHours() ? "OPEN" : "CLOSED");
    } else {
      Log.info("SLEEP entry: Time invalid => treating as OPEN (per policy)");
    }
    Log.info("SLEEP entry: sensorReady=%s", SensorManager::instance().isSensorReady() ? "true" : "false");
  }

  // If a ledger update (or time progression) moves the park into OPEN hours
  // while we are in SLEEPING_STATE, abort sleeping immediately in CONNECTED
  // mode so we stay awake/connected and resume counting.
  if (Time.isValid() && sysStatus.get_operatingMode() == CONNECTED && isWithinOpenHours()) {
    ensureSensorEnabled("SLEEP abort: CONNECTED+OPEN");
    state = IDLE_STATE;
    return;
  }

  // If we are connected and the publish queue is not yet in a sleep-safe
  // state (events queued or a publish in progress), defer sleeping so we
  // can finish delivering data. When offline, allow sleep immediately;
  // queued events will be flushed on the next connection.
  if (Particle.connected() && !PublishQueuePosix::instance().getCanSleep()) {
    static size_t lastPendingLogged = (size_t)-1;
    static unsigned long lastDeferralLogMs = 0;

    size_t pending = PublishQueuePosix::instance().getNumEvents();
    unsigned long nowMs = millis();
    bool shouldLog = (pending != lastPendingLogged) || (nowMs - lastDeferralLogMs) > 5000UL;
    if (shouldLog) {
      Log.info("Deferring sleep - publish queue has %u pending event(s) or publish in progress",
               (unsigned)pending);
      lastPendingLogged = pending;
      lastDeferralLogMs = nowMs;
    }
    // Stay in SLEEPING_STATE until the queue is sleep-safe.
    return;
  }

  // ********** Non-blocking disconnect + modem power-down **********
  // Device OS already manages the asynchronous cloud session teardown once
  // Particle.disconnect() has been requested. Here, we keep application
  // logic minimal: request cloud disconnect and radio-off once, then wait
  // (bounded) until both cloud and modem are actually off before sleeping.
  static bool disconnectRequested = false;
  static unsigned long disconnectRequestStartMs = 0;

  if (enteredState) {
    disconnectRequested = false;
    disconnectRequestStartMs = 0;
  }

  bool needDisconnect = Particle.connected() || isRadioPoweredOn();
  if (needDisconnect) {
    // Use ledger-configured budgets when available, with conservative defaults.
    uint16_t cloudBudgetSec = sysStatus.get_cloudDisconnectBudgetSec();
    if (cloudBudgetSec < 5 || cloudBudgetSec > 120) {
      cloudBudgetSec = 15;
    }

    uint16_t modemBudgetSec = sysStatus.get_modemOffBudgetSec();
    if (modemBudgetSec < 5 || modemBudgetSec > 120) {
      modemBudgetSec = 30;
    }

    unsigned long budgetMs = (unsigned long)((modemBudgetSec > cloudBudgetSec) ? modemBudgetSec : cloudBudgetSec) * 1000UL;

    if (!disconnectRequested) {
      Log.info("SLEEP: requesting cloud disconnect + modem off");
      requestFullDisconnectAndRadioOff();
      disconnectRequested = true;
      disconnectRequestStartMs = millis();
      return;
    }

    bool stillOn = Particle.connected() || isRadioPoweredOn();
    if (stillOn) {
      if (disconnectRequestStartMs != 0 && (millis() - disconnectRequestStartMs) > budgetMs) {
        if (sysStatus.get_operatingMode() != CONNECTED) {
          Log.warn("SLEEP: disconnect/modem-off exceeded budget (%lu ms) - continuing to sleep",
                   (unsigned long)(millis() - disconnectRequestStartMs));
          ignoreDisconnectFailure = true;
          disconnectRequested = false;
          disconnectRequestStartMs = 0;
        } else {
          Log.warn("SLEEP: disconnect/modem-off exceeded budget (%lu ms) - raising alert 15",
                   (unsigned long)(millis() - disconnectRequestStartMs));
          current.raiseAlert(15);
          state = ERROR_STATE;
          disconnectRequested = false;
          disconnectRequestStartMs = 0;
          return;
        }
      }
      if (!ignoreDisconnectFailure) {
        return;
      }
    }
  }

  int nightSleepSec = -1;
  if (!isWithinOpenHours()) {
    // Notify sensor layer we are entering full night sleep so sensors and
    // indicator LEDs can be powered down. During daytime naps we keep
    // interrupt-driven sensors (like PIR) powered so they can wake the
    // device from ULTRA_LOW_POWER sleep.
    Log.info("CLOSED-hours deep sleep: disabling sensor (onEnterSleep)");
    SensorManager::instance().onEnterSleep();
    Log.info("CLOSED-hours deep sleep: sensorReady after disable=%s", SensorManager::instance().isSensorReady() ? "true" : "false");

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
      // Reset sleep configuration so prior ULTRA_LOW_POWER GPIOs do not
      // accidentally carry into HIBERNATE configuration.
      config = SystemSleepConfiguration();
      config.mode(SystemSleepMode::HIBERNATE)
        .gpio(BUTTON_PIN, FALLING)
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
    // Within opening hours, align wake to the hour (or boundary) when possible.
    if (Time.isValid() && wakeBoundary > 0) {
      int boundary = wakeBoundary;
      int aligned = (int)(boundary - (Time.now() % boundary));
      if (aligned < 1) {
        aligned = 1;
      } else if (aligned > boundary) {
        aligned = boundary;
      }
      wakeInSeconds = aligned;
    } else {
      wakeInSeconds = (int)intervalSec;
    }
  }

  // If a sensor event is pending or the BLUE LED timer is still
  // active from a recent count, defer entering deep sleep so we
  // don't cut off in-progress events or visible indications.
  if (sensorDetect || countSignalTimer.isActive()) {
    Log.info("Deferring sleep - sensor event or LED timer active");
    state = IDLE_STATE;
    return;
  }

  if (digitalRead(BLUE_LED) == HIGH) {
    digitalWrite(BLUE_LED, LOW);
  }

  // Reset sleep configuration on each sleep so GPIO selections do not
  // accumulate across calls.
  config = SystemSleepConfiguration();

  Log.info("SLEEP duration: %d seconds", wakeInSeconds);

  config.mode(SystemSleepMode::ULTRA_LOW_POWER)
    .duration(wakeInSeconds * 1000L);

  // Configure wake pins. Button wake is always enabled. PIR wake is only
  // enabled during park open hours to avoid waking all night from floating
  // pins or motion we intentionally ignore.
  config.gpio(BUTTON_PIN, FALLING);
  if (isWithinOpenHours()) {
    Log.info("SLEEP config: arming button + PIR wake pins (open hours)");
    config.gpio(intPin, RISING);
  } else {
    Log.info("SLEEP config: arming button wake pin only (closed hours)");
  }

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

  Log.info("Woke from ULTRA_LOW_POWER sleep: reason=%d, pin=%d",
           (int)result.wakeupReason(), (int)result.wakeupPin());

  // Diagnostic: confirm open/closed decision at wake.
  if (Time.isValid()) {
    LocalTimeConvert convWake;
    convWake.withConfig(LocalTime::instance().getConfig()).withCurrentTime().convert();
    uint8_t hour = (uint8_t)(convWake.getLocalTimeHMS().toSeconds() / 3600);
    Log.info("Wake eval: parkHours %02u-%02u localHour=%02u => %s",
             sysStatus.get_openTime(), sysStatus.get_closeTime(), hour,
             isWithinOpenHours() ? "OPEN" : "CLOSED");
  } else {
    Log.info("Wake eval: Time invalid => treating as OPEN (per policy)");
  }

  if (result.wakeupPin() == BUTTON_PIN) {
    // User button wake: go directly to CONNECTING_STATE.
    SensorManager::instance().onExitSleep();
    Log.info("Button wake - connecting to sync and drain queue");
    userSwitchDetected = false;
    state = CONNECTING_STATE;
    return;
  } else {
    // In this state the device was awoken for hourly reporting or PIR
    // Re-enable sensors only if within opening hours; otherwise they
    // remain powered down to minimize sleep current.
    if (isWithinOpenHours()) {
      // If the sensor stack was never initialized (for example, device booted
      // while closed and remained asleep), onExitSleep() will be a no-op.
      // Ensure we (re)initialize the active sensor when entering open hours.
      Log.info("Wake: OPEN hours - enabling sensor (onExitSleep)");
      SensorManager::instance().onExitSleep();
      if (!SensorManager::instance().isSensorReady()) {
        Log.info("Wake: sensorReady=false - initializing from config");
        SensorManager::instance().initializeFromConfig();
      }
      Log.info("Wake: sensorReady=%s", SensorManager::instance().isSensorReady() ? "true" : "false");

      // In CONNECTED operating mode, the device should reconnect at the
      // start of open hours so it can resume normal connected behavior.
      if (sysStatus.get_operatingMode() == CONNECTED && !Particle.connected()) {
        Log.info("Wake: CONNECTED mode - reconnecting to Particle (park OPEN)");
        state = CONNECTING_STATE;
        return;
      }
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
      if (countSignalTimer.isActive()) {
        countSignalTimer.reset();
      } else {
        countSignalTimer.start();
      }
    }

    // If PIR woke us in LOW_POWER or DISCONNECTED mode, return immediately to sleep.
    if (pirWake && sysStatus.get_operatingMode() != CONNECTED) {
      state = SLEEPING_STATE;
      return;
    }

    // Check if this wake is due to scheduled reporting interval.
    // This ensures hourly (or configured-interval) reporting works even
    // when no other wake conditions are present.
    if (Time.isValid() && isWithinOpenHours()) {
      uint16_t intervalSec = sysStatus.get_reportingInterval();
      if (intervalSec == 0) {
        intervalSec = 3600; // Fallback to 1 hour
      }

      time_t now = Time.now();
      time_t lastReport = sysStatus.get_lastReport();
      if (lastReport == 0 || (now - lastReport) >= intervalSec) {
        Log.info("Wake: scheduled report due (now=%ld lastReport=%ld interval=%u)",
                 (long)now, (long)lastReport, (unsigned)intervalSec);
        state = REPORTING_STATE;
        return;
      }
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

  // ********** Connectivity Decision **********
  // In low-power mode, connect on every scheduled report to drain queue.
  // User can control report frequency via reportingIntervalSec.
  if (!Particle.connected()) {
    Log.info("Transitioning to CONNECTING_STATE after report");
    state = CONNECTING_STATE;
  } else {
    state = IDLE_STATE;
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
  case 44: // prolonged offline (>3 hours during open hours)
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

// ERROR_STATE: Error supervisor: decide recovery action
void handleErrorState() {
  static unsigned long resetTimer = 0;
  static int resolution = 0;

  if (state != oldState) {
    publishStateTransition();

    // Safety: regardless of recovery choice, do not leave radio/modem powered
    // while we sit in ERROR_STATE waiting for reset.
    requestFullDisconnectAndRadioOff();

    // In LOW_POWER or DISCONNECTED modes, avoid reset loops for connectivity/sleep alerts.
    if (sysStatus.get_operatingMode() != CONNECTED) {
      int8_t alert = current.get_alertCode();
      if (alert == 15 || alert == 16 || alert == 31) {
        Log.warn("Low-power mode: clearing alert %d to avoid reset loop", alert);
        current.set_alertCode(0);
        current.set_lastAlertTime(0);
        resolution = 0;
      } else {
        resolution = resolveErrorAction();
      }
    } else {
      resolution = resolveErrorAction();
    }
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
    // avoid thrashing.
    if (millis() - resetTimer > resetWait) {
      Log.info("Executing deep power down from ERROR_STATE (alert=%d)", current.get_alertCode());
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
    if (countSignalTimer.isActive()) {
      countSignalTimer.reset();
    } else {
      countSignalTimer.start();
    }

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
