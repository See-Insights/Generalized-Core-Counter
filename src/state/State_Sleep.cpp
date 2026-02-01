#include "state/State_Common.h"
#include "Config.h"
#include "Cloud.h"
#include "LocalTimeRK.h"
#include "MyPersistentData.h"
#include "PublishQueuePosixRK.h"
#include "SensorManager.h"
#include "device_pinout.h"
#include "SensorDefinitions.h"
#include "AB1805_RK.h"

// NOTE:
// This file was split from StateHandlers.cpp as a mechanical refactor.
// No behavioral changes were made.

/**
 * @brief SLEEPING_STATE: deep sleep between reporting intervals.
 * ...
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
      // Diagnostic logging to help debug alert 16 issues
      LocalTimeConvert diagConv;
      diagConv.withConfig(LocalTime::instance().getConfig()).withCurrentTime().convert();
      uint8_t currentHour = (uint8_t)(diagConv.getLocalTimeHMS().toSeconds() / 3600);
      Log.info("Entering HIBERNATE: Time.isValid=%d localHour=%d openTime=%d closeTime=%d",
               Time.isValid(), currentHour, sysStatus.get_openTime(), sysStatus.get_closeTime());
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
      Log.error("HIBERNATE sleep returned unexpectedly (platform does not support or HIBERNATE woke early)");
      Log.error("Park hours context: Time.isValid=%d localHour=%d openTime=%d closeTime=%d",
                Time.isValid(), currentHour, sysStatus.get_openTime(), sysStatus.get_closeTime());
      current.raiseAlert(16); // Alert: unexpected return from HIBERNATE
      hibernateDisabledForSession = true;
      // Clear alert immediately since we've handled the failure by disabling HIBERNATE
      // Without a reset, the setup() alert clearing code won't run
      current.set_alertCode(0);
      current.set_lastAlertTime(0);
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
    // Within opening hours, align wake to the reporting boundary.
    // Add 1 second margin to ensure we wake slightly after the boundary.
    if (Time.isValid() && wakeBoundary > 0) {
      int boundary = wakeBoundary;
      time_t now = Time.now();
      int offset = (int)(now % boundary);
      int aligned = boundary - offset;
      if (aligned < 1) {
        aligned = 1;
      } else if (aligned > boundary) {
        aligned = boundary;
      }
      wakeInSeconds = aligned + 1;
      Log.info("Sleep alignment: now=%lu boundary=%d offset=%d aligned=%d (+1 for margin)",
               (unsigned long)now, boundary, offset, aligned);
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

  // ********** WORKING SLEEP CONFIGURATION **********
  // Based on Connected-Counter-Next which uses system timer + GPIO wake.
  // NO AB1805 alarms - AB1805 is only used for watchdog + RTC time sync.
  // This approach works reliably on Photon2!
  
  Log.info("Entering ULTRA_LOW_POWER sleep for %d seconds (wakes at boundary or on GPIO)", wakeInSeconds);
  
  ab1805.stopWDT();
  
  config.mode(SystemSleepMode::ULTRA_LOW_POWER)
    .gpio(BUTTON_PIN, CHANGE)    // Service button wake
    .gpio(intPin, RISING)         // PIR sensor wake (active HIGH on detect)
    .duration(wakeInSeconds * 1000L);  // Timer-based wake at reporting boundary
  
  SystemSleepResult result = System.sleep(config);
  
#ifdef DEBUG_SERIAL
  waitFor(Serial.isConnected, 30000);  // Wait for Serial after wake so logs are visible
#endif
  
  ab1805.resumeWDT();
  
  // Determine wake source
  // When both GPIO and timer wake are configured, the wake source detection can be
  // ambiguous. The explicit approach: if neither GPIO pin woke us, it's the timer.
  pin_t wakePin = result.wakeupPin();
  bool pirWake = (wakePin == intPin);
  bool buttonWake = (wakePin == BUTTON_PIN);
  bool timerWake = !pirWake && !buttonWake;  // If no GPIO woke us, it's the timer
  
  // Wake diagnostics - include raw wakeupReason() for debugging
  SystemSleepWakeupReason reason = result.wakeupReason();
  Log.info("Woke from ULTRA_LOW_POWER: wakeupReason=%d pin=%d (pir=%d button=%d timer=%d)",
           (int)reason, (int)wakePin, pirWake, buttonWake, timerWake);
  
  if (pirWake) {
    digitalWrite(BLUE_LED, HIGH);  // Immediate visual feedback for motion
  }

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

  if (buttonWake) {
    // User button wake: go directly to CONNECTING_STATE.
    SensorManager::instance().onExitSleep();
    Log.info("WAKE: Button pressed - reason=SERVICE_REQUEST transitioning to CONNECTING_STATE");
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
        Log.info("WAKE: CONNECTED mode + OPEN hours - reason=MAINTAIN_CONNECTION transitioning to CONNECTING_STATE");
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

    // Timer wake = scheduled report. No checks, no gates.
    // We trust that the system timer woke us at the correct boundary.
    if (timerWake) {
      Log.info("WAKE: Timer wake - reason=SCHEDULED_REPORT transitioning to REPORTING_STATE");
      state = REPORTING_STATE;
      return;
    }

    // For PIR wakes, check if reporting is also due (opportunistic reporting)
    if (pirWake && Time.isValid() && isWithinOpenHours()) {
      uint16_t intervalSec = sysStatus.get_reportingInterval();
      if (intervalSec == 0) intervalSec = 3600;

      time_t now = Time.now();
      time_t lastReport = sysStatus.get_lastReport();
      if (lastReport > 0 && (now - lastReport) >= intervalSec) {
        int overdue = (int)(now - lastReport - intervalSec);
        Log.info("WAKE: PIR + report overdue (%d sec) - transitioning to REPORTING_STATE", overdue);
        state = REPORTING_STATE;
        return;
      }
    }

    // If PIR woke us in LOW_POWER or DISCONNECTED mode and no report is needed,
    // return immediately to sleep. This check comes AFTER opportunistic reporting
    // so overdue reports are not missed.
    if (pirWake && sysStatus.get_operatingMode() != CONNECTED) {
      state = SLEEPING_STATE;
      return;
    }

    Log.info("WAKE: No immediate action needed - transitioning to IDLE_STATE");
    state = IDLE_STATE;
  }
}
