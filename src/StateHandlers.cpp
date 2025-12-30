#include "Particle.h"
#include "Cloud.h"
#include "LocalTimeRK.h"
#include "MyPersistentData.h"
#include "Particle_Functions.h"
#include "PublishQueuePosixRK.h"
#include "SensorManager.h"
#include "device_pinout.h"
#include "SensorDefinitions.h"
#include "StateHandlers.h"

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

// SLEEPING_STATE: Deep sleep between reporting intervals
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

  if (Particle.connected() || radioOn) {
    // If we are connected to the cloud or the radio is on, we will disconnect
    if (!Particle_Functions::instance().disconnectFromParticle()) {
      // Modem or disconnect failure; raise an alert and transition to error
      // handling. If another alert is already active, a higher-severity
      // code will win.
      current.raiseAlert(15);
      state = ERROR_STATE;
      return;
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

    // If this wake was from PIR, turn off the BLUE LED now that we've
    // completed post-wake housekeeping.
    if (pirWake) {
      digitalWrite(BLUE_LED, LOW);
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
  // If this is the first report after a calendar day boundary, run the
  // daily cleanup once to reset daily counters and housekeeping.
  if (Time.isValid()) {
    time_t lastReport = sysStatus.get_lastReport();
    if (lastReport != 0 && Time.day(now) != Time.day(lastReport)) {
      Log.info("New day detected (last report day=%d, current day=%d) - running dailyCleanup",
               Time.day(lastReport), Time.day(now));
      dailyCleanup();
      sysStatus.set_lastDailyCleanup(now);
    }
  }

  sysStatus.set_lastReport(now);
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
  // response in more than 3 hours, raise alert 40 so the error
  // supervisor can evaluate and potentially reset the device.
  if (Time.isValid()) {
    time_t lastHook = sysStatus.get_lastHookResponse();
    if (lastHook != 0 && (now - lastHook) > (3 * 3600L)) {
      Log.info("No successful webhook response for >3 hours (last=%ld, now=%ld) - raising alert 40",
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

// CONNECTING_STATE: Establish cloud connection, then load config
void handleConnectingState() {
  static unsigned long connectionStartTimeStamp; // Time helps us know how long it took to connect
  char data[64];                                  // Holder for message strings

  if (state != oldState) { // One-time actions when entering state
    // Reset connection duration (will be updated once connected)
    sysStatus.set_lastConnectionDuration(0);
    publishStateTransition();
    connectionStartTimeStamp = millis(); // Use millis as the clock may get reset on connect
    Particle.connect();                  // Ask Device OS to connect; we'll poll status
  }

  sysStatus.set_lastConnectionDuration(int((millis() - connectionStartTimeStamp) / 1000));

  if (Particle.connected()) {
    sysStatus.set_lastConnection(Time.now()); // This is the last time we last connected
    // Keep device awake for a while after connection for troubleshooting
    stayAwake = stayAwakeLong;
    stayAwakeTimeStamp = millis(); // Start the stay awake timer now
    measure.getSignalStrength();   // Test signal strength while radio is on
    measure.batteryState();        // Refresh SoC and enclosure temperature on connect
    Log.info("Enclosure temperature at connect: %4.2f C", (double)current.get_internalTempC());
    snprintf(data, sizeof(data), "Connected in %i secs", sysStatus.get_lastConnectionDuration());
    Log.info(data);
    if (sysStatus.get_verboseMode()) {
      Particle.publish("Cellular", data, PRIVATE);
    }

    // Track when we first became cloud-connected for this connection cycle
    onlineWorkStartMs = millis();
    forceSleepThisCycle = false;

    // Load configuration from cloud ledgers after connecting
    bool configOk = Cloud::instance().loadConfigurationFromCloud();
    if (!configOk) {
      // Configuration or status ledger apply failure; record as an alert so
      // the error supervisor can evaluate and potentially reset.
      current.raiseAlert(41);
    }

    // After a successful connect, publish the latest counters and
    // temperature to the device-data ledger so the cloud view reflects
    // current state even if a regular hourly report has not yet run.
    if (!Cloud::instance().publishDataToLedger()) {
      current.raiseAlert(42); // data ledger publish failure
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
  } else if (sysStatus.get_lastConnectionDuration() > 600) {
    // What happens if we do not connect
    // Connection timeout; record a connectivity alert and let the error
    // handler decide what to do next. If multiple alerts are present, the
    // highest-severity one is preserved.
    current.raiseAlert(31);
    state = ERROR_STATE; // Note - not setting the ERROR timestamp to make this go quickly
  } else {
    // Remain in CONNECTING_STATE and loop
  }
}

// FIRMWARE_UPDATE_STATE: Stay connected for firmware/config updates
void handleFirmwareUpdateState() {
  if (state != oldState) {
    publishStateTransition();
    Log.info("Entering FIRMWARE_UPDATE_STATE - keeping device connected for updates");

    // In update mode, force connected operating mode and disable low power
    sysStatus.set_lowPowerMode(false);
    sysStatus.set_operatingMode(CONNECTED);

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
    }
  }

  // Optional escape hatch: user button can also exit update mode
  if (!digitalRead(BUTTON_PIN)) { // Active-low user button
    Log.info("User button pressed - exiting FIRMWARE_UPDATE_STATE to IDLE_STATE");
    state = IDLE_STATE;
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

    // Flash the on-module BLUE LED for 1 second as a
    // visual count indicator (more visible in daylight).
    digitalWrite(BLUE_LED, HIGH);
    softDelay(1000);
    digitalWrite(BLUE_LED, LOW);

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
