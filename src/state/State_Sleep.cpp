// Enable debug serial wait for sleep debugging
// This must be defined before any includes to ensure it's available
#define DEBUG_SERIAL

#include "state/State_Common.h"
#include "Config.h"
#include "Cloud.h"
#include "Connectivity.h"
#include "LocalTimeCache.h"
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
  static bool operationsCompleteLogged = false;  // Track if we've logged completion message

  if (enteredState) {
    publishStateTransition();
    operationsCompleteLogged = false;  // Reset flag on state entry
    // One-time diagnostic on entry so logs clearly show the device's view of park hours.
    if (Time.isValid()) {
      const LocalTimeCache::LocalTimeSnapshot &snapshot = LocalTimeCache::getLocalTimeSnapshot();
      uint8_t hour = snapshot.localHour;
      Log.info("SLEEP entry: parkHours %02u-%02u localHour=%02u => %s",
               sysStatus.get_openTime(), sysStatus.get_closeTime(), hour,
               isWithinOpenHours() ? "OPEN" : "CLOSED");
    } else {
      Log.info("SLEEP entry: Time invalid => treating as OPEN (per policy)");
    }
    Log.info("SLEEP entry: sensorReady=%s", SensorManager::instance().isSensorReady() ? "true" : "false");
  }

  // Determine if we will use *effective* network standby for the upcoming sleep.
  // Only cellular platforms actually apply standby; on WiFi, treat as disabled
  // so disconnect logic doesn't wait for a radio state that won't change.
  bool useNetworkStandbyEffective = (sysStatus.get_connectionMode() == INTERMITTENT_KEEP_ALIVE) &&
                                   isWithinOpenHours();
#if HAL_PLATFORM_CELLULAR
  useNetworkStandbyEffective = useNetworkStandbyEffective && !Cellular.isOff();
#else
  useNetworkStandbyEffective = false;
#endif

  // If a ledger update (or time progression) moves the park into OPEN hours
  // while we are in SLEEPING_STATE, abort sleeping immediately in CONNECTED
  // mode so we stay awake/connected and resume counting.
  if (Time.isValid() && sysStatus.get_connectionMode() == CONNECTED && isWithinOpenHours()) {
    ensureSensorEnabled("SLEEP abort: CONNECTED+OPEN");
    state = IDLE_STATE;
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
  const unsigned long CLOUD_SYNC_TIMEOUT_MS = 30000; // 30 second budget

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

  // Safety: if disconnectRequested was latched but the timestamp got lost
  // (for example, due to an unexpected state-machine re-entry), reset so we
  // can actually issue the request instead of waiting forever.
  if (disconnectRequested && disconnectRequestStartMs == 0) {
    Log.warn("SLEEP: disconnectRequested latched without timestamp - resetting");
    disconnectRequested = false;
  }

  // Only wait for cloud operations *before* requesting disconnect.
  if (Particle.connected() && !disconnectRequested) {
    // Initialize timer on first check
    if (cloudSyncStartMs == 0) {
      cloudSyncStartMs = millis();
    }
    // Check prerequisite completion
    bool queueEmpty = PublishQueuePosix::instance().getCanSleep();
    bool ledgersSynced = Cloud::instance().areLedgersSynced();
    bool updatesChecked = !System.updatesPending();
    bool webhookConfirmed = !session.awaitingWebhookResponse;

    bool allComplete = queueEmpty && ledgersSynced && updatesChecked && webhookConfirmed;

    if (!allComplete) {
      unsigned long elapsedMs = millis() - cloudSyncStartMs;

      if (elapsedMs < CLOUD_SYNC_TIMEOUT_MS) {
        // Still within budget - log status every 5 seconds
        static unsigned long lastStatusLogMs = 0;
        if ((millis() - lastStatusLogMs) > 5000UL) {
          Log.info("SLEEP: Waiting for cloud operations - queue:%s ledgers:%s updates:%s webhook:%s (%lu/%lu ms)",
                   queueEmpty ? "Y" : "N",
                   ledgersSynced ? "Y" : "N",
                   updatesChecked ? "Y" : "N",
                   webhookConfirmed ? "Y" : "N",
                   elapsedMs, CLOUD_SYNC_TIMEOUT_MS);
          lastStatusLogMs = millis();
        }
        Particle.process(); // Keep connection alive
        return; // Stay in SLEEPING_STATE until complete or timeout
      }

      // Budget exceeded - log incomplete operations and raise ONE alert (priority order)
      Log.warn("SLEEP: Cloud sync timeout after %lu ms", elapsedMs);
      Log.warn("SLEEP: State at timeout - queue=%d ledgers=%d updates=%d webhook=%d",
               queueEmpty, ledgersSynced, updatesChecked, webhookConfirmed);

      // Only raise one alert - check in priority order (queue > ledger > updates > webhook)
      if (!queueEmpty) {
        Log.warn("SLEEP: Publish queue not empty - raising alert 43");
        current.raiseAlert(43); // Queue drainage failure (highest priority)
      } else if (!ledgersSynced) {
        Log.warn("SLEEP: Ledger sync incomplete - raising alert 41");
        current.raiseAlert(41); // Configuration sync failure
      } else if (!updatesChecked) {
        Log.warn("SLEEP: OTA updates pending - raising alert 42");
        current.raiseAlert(42); // OTA updates pending
      } else if (!webhookConfirmed) {
        Log.warn("SLEEP: Webhook response not received - raising alert 40");
        current.raiseAlert(40); // Webhook response timeout
      }

      // Proceed with disconnect despite incomplete operations
      cloudSyncStartMs = 0;
    } else {
      // All operations complete - reset timer and proceed
      cloudSyncStartMs = 0;
    }

    if (!operationsCompleteLogged) {
      Log.info("SLEEP: Cloud operations gate passed - ready to disconnect");
      // One-time diagnostic to explain disconnect decisions; helps catch
      // cases where disconnect is skipped due to connectivity state.
      Log.info("SLEEP: disconnect context connected=%d radioOn=%d standbyEffective=%d occupied=%d",
               Particle.connected(), isRadioPoweredOn(), useNetworkStandbyEffective, current.get_occupied());
      operationsCompleteLogged = true;
    }
  } else {
    // Not connected - reset timer
    cloudSyncStartMs = 0;
  }

  // If we're going to sleep with cellular network standby, do NOT wait for radio-off.
  // We only need to tear down the cloud session (quickly) before sleeping.
  bool stillOn;
#if Wiring_Cellular
  stillOn = useNetworkStandbyEffective ? Particle.connected() : (Particle.connected() || isRadioPoweredOn());
#else
  // On WiFi platforms, waiting for WiFi.isOn() to flip false can take a long
  // time and is not required for safe low-power sleep. Only gate on cloud.
  stillOn = Particle.connected();
#endif

  // If cloud is already offline but the radio is still on (common after a transient
  // cloud drop), power down the radio before sleeping (unless using standby).
#if Wiring_Cellular
  if (!disconnectRequested && !useNetworkStandbyEffective && !Particle.connected() && isRadioPoweredOn()) {
    Log.info("SLEEP: cloud offline but radio still on - powering down radio");
    requestRadioPowerOff();
    disconnectRequested = true;
    disconnectRequestStartMs = millis();
    return;
  }
#endif

  // Compute disconnect budget once per loop to avoid duplicated logic.
  auto computeDisconnectBudgetMs = [&]() -> unsigned long {
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

    // When using network standby, shorten the disconnect grace period: we don't
    // want to burn connection budget waiting for a perfect teardown.
    if (useNetworkStandbyEffective) {
      budgetMs = (unsigned long)cloudBudgetSec * 1000UL;
      if (budgetMs > 5000UL) {
        budgetMs = 5000UL;
      }
    }
    return budgetMs;
  };

  unsigned long disconnectBudgetMs = computeDisconnectBudgetMs();

  // Once cloud operations are satisfied (or timed out), initiate disconnect exactly once.
  if (Particle.connected() && !disconnectRequested) {
    if (useNetworkStandbyEffective) {
      Log.info("SLEEP: requesting cloud disconnect (network standby enabled)");
      Connectivity::requestCloudDisconnectOnly();
    } else {
      Log.info("SLEEP: requesting cloud disconnect + modem off");
      requestFullDisconnectAndRadioOff();
    }
    disconnectRequested = true;
    disconnectRequestStartMs = millis();
    return;
  }

  // If disconnect was requested, wait (bounded) for it to take effect before sleeping.
  if (disconnectRequested && stillOn) {
      if (disconnectRequestStartMs != 0 && (millis() - disconnectRequestStartMs) > disconnectBudgetMs) {
        if (sysStatus.get_connectionMode() != CONNECTED) {
          Log.warn("SLEEP: disconnect/modem-off exceeded budget (%lu ms) - continuing to sleep",
                   (unsigned long)(millis() - disconnectRequestStartMs));

          // Best-effort: if cloud/radio teardown is stalling, force the radio off
          // so we don't repeatedly wake with the Wi-Fi stack trying to reconnect.
#if Wiring_WiFi
          requestRadioPowerOff();
#elif Wiring_Cellular
          if (!useNetworkStandbyEffective) {
            requestRadioPowerOff();
          }
#endif

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
        // Help DeviceOS make progress on the disconnect.
        Particle.process();
        return;
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
    if (!session.hibernateDisabledForSession) {
      // Diagnostic logging to help debug alert 16 issues
      const LocalTimeCache::LocalTimeSnapshot &snapshot = LocalTimeCache::getLocalTimeSnapshot();
      uint8_t currentHour = snapshot.localHour;
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
      session.hibernateDisabledForSession = true;
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
      Log.info("Sleep alignment: now=%lu boundary=%d offset=%d aligned=%d (+1 for margin)",
               (unsigned long)now, boundary, offset, aligned);
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
      state = IDLE_STATE;
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
      Log.info("OCCUPANCY: Sleeping for debounce check: %d sec (occupied)", wakeInSeconds);
    } else {
      // Unoccupied - use scheduled wake time
      Log.info("OCCUPANCY: Sleeping for scheduled wake: %d sec (unoccupied)", wakeInSeconds);
    }
  }

  // Reset sleep configuration on each sleep so GPIO selections do not
  // accumulate across calls.
  config = SystemSleepConfiguration();

  // ********** WORKING SLEEP CONFIGURATION **********
  // Based on Connected-Counter-Next which uses system timer + GPIO wake.
  // NO AB1805 alarms - AB1805 is only used for watchdog + RTC time sync.
  // This approach works reliably on Photon2!
  
  // In INTERMITTENT_KEEP_ALIVE mode during open hours on *cellular* devices,
  // use network standby to avoid rapid reconnects that could trigger carrier
  // blacklisting. On WiFi devices, standby is not applied.
  bool useNetworkStandby = useNetworkStandbyEffective;
  
  if (useNetworkStandby) {
    Log.info("Entering ULTRA_LOW_POWER sleep with NETWORK STANDBY for %d seconds (occupancy mode)", wakeInSeconds);
  } else {
    Log.info("Entering ULTRA_LOW_POWER sleep for %d seconds (wakes at boundary or on GPIO)", wakeInSeconds);
  }

  // On WiFi platforms, ensure the radio is actually OFF before entering
  // ULTRA_LOW_POWER. If the cloud/radio was left on, the device can flash
  // green briefly on wake as WiFi attempts to reconnect.
#if Wiring_WiFi
  if (!useNetworkStandby) {
    if (Particle.connected()) {
      Particle.disconnect();
    }
    if (WiFi.isOn()) {
      WiFi.disconnect();
      WiFi.off();
    }
  }
#endif
  
  ab1805.stopWDT();
  
  config.mode(SystemSleepMode::ULTRA_LOW_POWER)
    .gpio(BUTTON_PIN, CHANGE)    // Service button wake
    .gpio(intPin, RISING);        // PIR sensor wake (active HIGH on detect)
    
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
  
  config.duration(wakeInSeconds * 1000L);  // Timer-based wake at reporting boundary
  
  SystemSleepResult result = System.sleep(config);
  
  // Clear any pending interrupts on wake pins after sleep to ensure clean state
  // This is critical for proper interrupt handling after wake
  pinMode(BUTTON_PIN, INPUT);    // Reset pin mode
  pinMode(intPin, INPUT);         // Reset pin mode
  
  // Resume hardware watchdog immediately after wake
  ab1805.resumeWDT();

#ifdef DEBUG_SERIAL
  // Photon2 USB serial needs time to re-enumerate after sleep.
  // Must re-initialize before any Log.info() calls to avoid corrupted output.
  Serial.begin();  // Re-initialize serial port
  delay(500);     // Give USB time to re-enumerate
  
  // Use blocking wait for serial connection - waitFor() is non-blocking
  // and the device would go back to sleep before connection established
  unsigned long serialWaitStart = millis();
  while (!Serial.isConnected() && (millis() - serialWaitStart) < 30000) {
    // Feed application watchdog during the wait to prevent reset
    Particle.process();
    delay(100);  // Blocking wait for serial connection
  }
  
  if (Serial.isConnected()) {
    delay(500);  // Extra settling time after connection
    Log.info("Serial reconnected after %lu ms", (millis() - serialWaitStart));
  } else {
    Log.warn("Serial did not reconnect within 30s - continuing without serial");
  }
#endif // DEBUG_SERIAL

  // Re-attach user button interrupt after sleep (sleep may have detached it)
  attachInterrupt(BUTTON_PIN, userSwitchISR, FALLING);
  userSwitchDetected = false;  // Clear any spurious flag

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
  
  // LED will be turned on with proper timeout by PIR wake processing below
  // Don't turn it on here without timeout as it causes false LED timeout detection

  // Diagnostic: confirm open/closed decision at wake.
  if (Time.isValid()) {
    const LocalTimeCache::LocalTimeSnapshot &snapshot = LocalTimeCache::getLocalTimeSnapshot();
    uint8_t hour = snapshot.localHour;
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
      if (sysStatus.get_connectionMode() == CONNECTED && !Particle.connected()) {
        Log.info("WAKE: CONNECTED mode + OPEN hours - reason=MAINTAIN_CONNECTION transitioning to CONNECTING_STATE");
        state = CONNECTING_STATE;
        return;
      }
    } else {
      Log.info("Woke outside opening hours; keeping sensors powered down");
    }

    // Check if LED timeout expired FIRST (before processing new PIR wake)
    // This ensures we don't immediately undo state changes from PIR processing
    Log.info("[WAKE CHECK] Mode=%d, LED status=%s, LED time remaining=%lu sec, occupied=%s",
             sysStatus.get_sensorMode(),
             signalLEDStatus() ? "ON" : "OFF",
             (unsigned long)signalLEDTimeRemaining(),
             current.get_occupied() ? "true" : "false");
    
    if (sysStatus.get_sensorMode() == OCCUPANCY && signalLEDTimeRemaining() == 0 && signalLEDStatus()) {
      // LED timeout expired - debounce period elapsed without motion
      Log.info("[LED TIMEOUT] Detected on wake - processing unoccupied transition");
      uint32_t sessionDuration = Time.now() - current.get_occupancyStartTime();
      uint32_t totalOccupied = current.get_totalOccupiedSeconds() + sessionDuration;
      current.set_totalOccupiedSeconds(totalOccupied);
      current.set_occupied(false);
      current.set_occupancyStartTime(0);
      signalLED(false);  // Turn off LED

      Log.info("Space now UNOCCUPIED (LED timeout on wake) - Session: %lu sec, Total today: %lu sec",
               sessionDuration, totalOccupied);
      
      // Report immediately on occupancy state changes
      Log.info("Occupancy change detected (occupied->unoccupied) - triggering immediate report");
      session.occupancyChangeTriggered = true;
      state = REPORTING_STATE;
      return;
    }

    // Process PIR wake events (after checking LED timeout above)
    // If this wake was caused by the PIR interrupt, synthesize a single
    // detection event so that the motion that woke the device is counted
    // even if the ISR flag did not survive ULTRA_LOW_POWER sleep.
    if (pirWake) {
      Log.info("[PIR WAKE] Processing PIR wake event (mode=%d)", sysStatus.get_sensorMode());
      if (sysStatus.get_sensorMode() == COUNTING) {
        current.set_hourlyCount(current.get_hourlyCount() + 1);
        current.set_dailyCount(current.get_dailyCount() + 1);
        current.set_lastCountTime(Time.now());
        signalLED(true, 1000);  // Flash for 1 second
        Log.info("Count detected from PIR wake - Hourly: %d, Daily: %d",
                 current.get_hourlyCount(), current.get_dailyCount());
      } else if (sysStatus.get_sensorMode() == OCCUPANCY) {
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
            debounceMs = 60000; // default 60s
          }
          signalLED(true, debounceMs);
          Log.info("Space now OCCUPIED from PIR wake at %s",
                   Time.timeStr().c_str());
          
          // Report immediately on occupancy state changes
          Log.info("Occupancy change detected (unoccupied->occupied) - triggering immediate report");
          session.occupancyChangeTriggered = true;
          state = REPORTING_STATE;
          return;
        } else {
          // Already occupied - motion detected, LED stays on
          Log.info("Motion from PIR wake - space remains occupied");
        }
        current.set_lastOccupancyEvent(millis());
      }
    }

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
        Log.info("WAKE: Timer wake while OCCUPIED (KEEP_ALIVE) - suppressing scheduled report");
        state = SLEEPING_STATE;
        return;
      }

      Log.info("WAKE: Timer wake - reason=SCHEDULED_REPORT transitioning to REPORTING_STATE");
      state = REPORTING_STATE;
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
    }

    // If PIR woke us in INTERMITTENT or DISCONNECTED mode and no report is needed,
    // return immediately to sleep. This check comes AFTER opportunistic reporting
    // so overdue reports are not missed.
    if (pirWake && sysStatus.get_connectionMode() != CONNECTED) {
      state = SLEEPING_STATE;
      return;
    }

    Log.info("WAKE: No immediate action needed - transitioning to IDLE_STATE");
    state = IDLE_STATE;
  }
}
