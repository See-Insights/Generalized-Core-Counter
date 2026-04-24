#include "BuildProfile.h"
#include "state/State_Common.h"
#include "Config.h"
#include "cloud/Cloud.h"
#include "power/Connectivity.h"
#include "power/ConnectivityPolicy.h"
#include "time/LocalTimeCache.h"
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

} // namespace

// NOTE:
// This file was split from StateHandlers.cpp as a mechanical refactor.
// No behavioral changes were made.

/**
 * @brief SLEEPING_STATE: deep sleep between reporting intervals.
 * ...
 */
void handleSleepingState() {
  bool enteredState = (state != oldState);
  static bool operationsCompleteLogged = false;  // Track if we've logged completion message

  if (enteredState) {
    setAppBreadcrumb(3);
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
  // Standby is only useful after a successful cloud session in the current
  // wake cycle; if connect never succeeded, preserving the modem state buys
  // nothing and can carry a bad NCP state into the next wake.
  bool useNetworkStandbyEffective = (sysStatus.get_connectionMode() == INTERMITTENT_KEEP_ALIVE) &&
                                   isWithinOpenHours() &&
                                   Observability::cycleStats().connect_result == Observability::WakeCycleStats::ConnectResult::SUCCESS;
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
  static unsigned long cloudSyncBudgetMs = 0;
  static uint16_t cloudSyncMaxQueueDepth = 0;

  // ********** Non-blocking disconnect + modem power-down **********
  // Device OS already manages the asynchronous cloud session teardown once
  // Particle.disconnect() has been requested. Here, we keep application
  // logic minimal: request cloud disconnect and radio-off once, then wait
  // (bounded) until both cloud and modem are actually off before sleeping.
  static bool disconnectRequested = false;
  static unsigned long disconnectRequestStartMs = 0;
#if Wiring_WiFi
  static bool wifiOffGuardActive = false;
  static unsigned long wifiOffGuardStartMs = 0;
  static unsigned long wifiOffGuardLastRetryMs = 0;
  static uint8_t wifiOffGuardRetries = 0;
#endif

  if (enteredState) {
    disconnectRequested = false;
    disconnectRequestStartMs = 0;
    cloudSyncBudgetMs = 0;
    cloudSyncMaxQueueDepth = 0;
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
    uint16_t queueDepth = (uint16_t)PublishQueuePosix::instance().getNumEvents();
    if (queueDepth > cloudSyncMaxQueueDepth) {
      cloudSyncMaxQueueDepth = queueDepth;
    }
    unsigned long queueAwareBudgetMs = computeCloudSyncTimeoutMs(cloudSyncMaxQueueDepth);
    if (queueAwareBudgetMs > cloudSyncBudgetMs) {
      cloudSyncBudgetMs = queueAwareBudgetMs;
    }

    {
      static uint16_t lastQueueDepth = 0xFFFF;
      if (lastQueueDepth != 0xFFFF && queueDepth < lastQueueDepth) {
        thrashGuard.markProgress("QUEUE_DRAIN");
      }
      lastQueueDepth = queueDepth;
    }

    bool allComplete = queueEmpty && ledgersSynced && updatesChecked && webhookConfirmed;

    if (!allComplete) {
      unsigned long elapsedMs = millis() - cloudSyncStartMs;

      if (elapsedMs < cloudSyncBudgetMs) {
        // Still within budget - log status every 5 seconds
        static unsigned long lastStatusLogMs = 0;
        if ((millis() - lastStatusLogMs) > ConnectivityPolicy::CLOUD_OPS_STATUS_LOG_INTERVAL_MS) {
          Log.info("SLEEP: Waiting for cloud operations - queue:%s(%u) ledgers:%s updates:%s webhook:%s (%lu/%lu ms)",
                   queueEmpty ? "Y" : "N",
                   (unsigned)queueDepth,
                   ledgersSynced ? "Y" : "N",
                   updatesChecked ? "Y" : "N",
                   webhookConfirmed ? "Y" : "N",
                   elapsedMs, cloudSyncBudgetMs);
          lastStatusLogMs = millis();
        }
        // Mark progress to prevent ThrashGuard timeout during legitimate cloud operations wait
        thrashGuard.markProgress("CLOUD_OPS_WAIT");
        Particle.process(); // Keep connection alive
        return; // Stay in SLEEPING_STATE until complete or timeout
      }

      // Budget exceeded - log incomplete operations and raise ONE alert (priority order)
      Log.warn("SLEEP: Cloud sync timeout after %lu ms (budget=%lu ms, queueDepth=%u maxQueueDepth=%u)",
           elapsedMs,
           cloudSyncBudgetMs,
           (unsigned)queueDepth,
           (unsigned)cloudSyncMaxQueueDepth);
      Log.warn("SLEEP: State at timeout - queueEmpty=%d queueDepth=%u ledgers=%d updates=%d webhook=%d",
           queueEmpty, (unsigned)queueDepth, ledgersSynced, updatesChecked, webhookConfirmed);

      // Only raise one alert - check in priority order (queue > ledger > updates > webhook)
      if (!queueEmpty) {
        Log.warn("SLEEP: Publish queue not empty - raising alert 43");
        current.raiseAlert(43); // Queue drainage failure (highest priority)
      } else if (!ledgersSynced) {
        // Enhanced diagnostics for Alert 44 troubleshooting
        Log.warn("SLEEP: Ledger sync incomplete - raising alert 44");
        Log.warn("Alert 44 context: timeout after %lu ms (budget=%lu ms)",
                 elapsedMs, cloudSyncBudgetMs);
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
      // All operations complete - reset timer and proceed
      cloudSyncStartMs = 0;
      cloudSyncBudgetMs = 0;
      cloudSyncMaxQueueDepth = 0;
    }

    if (!operationsCompleteLogged) {
      Log.info("SLEEP: Cloud operations gate passed - ready to disconnect");
      // One-time diagnostic to explain disconnect decisions; helps catch
      // cases where disconnect is skipped due to connectivity state.
      Log.info("SLEEP: disconnect context connected=%d radioOn=%d standbyEffective=%d occupied=%d",
               Particle.connected(), Connectivity::isRadioPoweredOn(), useNetworkStandbyEffective, current.get_occupied());

      // Observability: end of service window (ledger sync + queue drain + OTA check + webhook).
      Observability::cycleStats().markServiceEnd(millis());

      operationsCompleteLogged = true;
    }
  } else {
    // Not connected - reset timer
    cloudSyncStartMs = 0;
    cloudSyncBudgetMs = 0;
    cloudSyncMaxQueueDepth = 0;
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

  // If cloud is already offline but the radio is still on (common after a transient
  // cloud drop), power down the radio before sleeping (unless using standby).
#if Wiring_Cellular
  if (!disconnectRequested && !useNetworkStandbyEffective && !Particle.connected() && Connectivity::isRadioPoweredOn()) {
    Log.info("SLEEP: cloud offline but radio still on - powering down radio");
    Connectivity::requestRadioPowerOff();
    disconnectRequested = true;
    disconnectRequestStartMs = millis();
    return;
  }
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

  // Once cloud operations are satisfied (or timed out), initiate disconnect exactly once.
  if (Particle.connected() && !disconnectRequested) {
    if (useNetworkStandbyEffective) {
      Log.info("SLEEP: requesting cloud disconnect (network standby enabled)");
      Connectivity::requestCloudDisconnectOnly();
    } else {
      Log.info("SLEEP: requesting cloud disconnect + modem off");
      Connectivity::requestFullDisconnectAndRadioOff();
    }
    disconnectRequested = true;
    disconnectRequestStartMs = millis();

    // Observability: teardown window start.
    Observability::cycleStats().markTeardownStart((uint32_t)disconnectRequestStartMs, useNetworkStandbyEffective);

    return;
  }

  // If disconnect was requested, wait (bounded) for it to take effect before sleeping.
  if (disconnectRequested && stillOn) {
    if (disconnectRequestStartMs != 0 && (millis() - disconnectRequestStartMs) > disconnectBudgetMs) {
      Log.warn("SLEEP: disconnect/modem-off exceeded budget (%lu ms) - raising alert 15",
               (unsigned long)(millis() - disconnectRequestStartMs));
      current.raiseAlert(15);
      state = ERROR_STATE;
      disconnectRequested = false;
      disconnectRequestStartMs = 0;
      return;
    }

    // Help DeviceOS make progress on the disconnect.
    // Throttle progress markers to avoid noisy per-loop updates.
    static unsigned long lastDisconnectProgressMs = 0;
    unsigned long nowMs = millis();
    if ((nowMs - lastDisconnectProgressMs) > 1000UL) {
      thrashGuard.markProgress("DISCONNECT_WAIT");
      lastDisconnectProgressMs = nowMs;
    }
    Particle.process();
    return;
  }

  // Observability: teardown completed (bounded wait finished and we are proceeding to sleep).
  if (disconnectRequested && !stillOn) {
    Observability::cycleStats().markTeardownEnd(millis());
  }

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
        Log.warn("SLEEP: pre-sleep gate blocked (cloud=%d standby=1) - requesting cloud disconnect",
                 (int)cloudConnected);
        Connectivity::requestCloudDisconnectOnly();
      } else if (cloudConnected) {
        Log.warn("SLEEP: pre-sleep gate blocked (cloud=1 standby=0) - requesting cloud disconnect + modem off");
        Connectivity::requestFullDisconnectAndRadioOff();
      } else {
        Log.warn("SLEEP: pre-sleep gate blocked (cloud=0 radioOn=1 standby=0) - requesting modem off");
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
      Log.warn("SLEEP: pre-sleep teardown exceeded budget (%lu ms) - raising alert 15", elapsedMs);
      current.raiseAlert(15);
      state = ERROR_STATE;
      disconnectRequested = false;
      disconnectRequestStartMs = 0;
      return;
    }

    static unsigned long lastGateProgressMs = 0;
    unsigned long nowMs = millis();
    if ((nowMs - lastGateProgressMs) > 1000UL) {
      thrashGuard.markProgress("DISCONNECT_WAIT");
      lastGateProgressMs = nowMs;
    }
    Particle.process();
    return;
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

  // -----------------------------------------------------------------------
  // Minimal end-of-cycle observability (one compact line per wake cycle)
  // -----------------------------------------------------------------------
  {
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
        (unsigned long)ConnectivityPolicy::CLOUD_OPS_GATE_TIMEOUT_MS +
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
      Log.info("SLEEP: WiFi radio powered off before sleep (%lu ms)", guardElapsedMs);
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
  ab1805.stopWDT();

  thrashGuard.markProgress("SLEEP_ATTEMPT");
  SystemSleepResult result = System.sleep(config);

  // If Device OS rejects the sleep configuration, System.sleep() returns
  // immediately and the wake pin will be invalid (65535). Without handling
  // that error, the state machine will interpret it as a timer wake and can
  // thrash in a tight loop.
  if (result.error() != SYSTEM_ERROR_NONE) {
    thrashGuard.markProgress("SLEEP_RETURNED");
    Log.error("ULTRA_LOW_POWER sleep failed err=%d (wakeIn=%d sec, button=%d pir=%d) - falling back to STOP", (int)result.error(), wakeInSeconds, (int)BUTTON_PIN, (int)intPin);
    current.raiseAlert(16);

   // STOP generally supports a wider set of wake pins on some platforms.
    config = SystemSleepConfiguration();
    config.mode(SystemSleepMode::STOP)
      .gpio(BUTTON_PIN, FALLING)
      .gpio(intPin, RISING)
      .duration((uint32_t)wakeInSeconds * 1000UL);
    result = System.sleep(config);

    if (result.error() != SYSTEM_ERROR_NONE) {
      Log.error("STOP sleep fallback failed err=%d (wakeIn=%d sec) - using timer-only STOP sleep", (int)result.error(), wakeInSeconds);
      config = SystemSleepConfiguration();
      config.mode(SystemSleepMode::STOP)
        .duration((uint32_t)wakeInSeconds * 1000UL);
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
        state = ERROR_STATE;
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
      Particle.process();
      delay(ConnectivityPolicy::DEBUG_SERIAL_WAIT_POLL_DELAY_MS);
    }

    if (Serial.isConnected()) {
      delay(ConnectivityPolicy::DEBUG_SERIAL_POST_CONNECT_DELAY_MS);
      Log.info("Serial reconnected after %lu ms", (millis() - serialWaitStart));
    } else {
      Log.warn("Serial did not reconnect within 30s - continuing without serial");
    }
  }

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

  // Observability: start of a new wake cycle (ULP return path).
  Observability::cycleStats().resetOnWake(millis());
  
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
    // User button wake: queue a fresh service report, then force immediate connect.
    SensorManager::instance().onExitSleep();
    session.serviceRequestTriggered = true;
    Log.info("WAKE: Button pressed - reason=SERVICE_REQUEST transitioning to REPORTING_STATE");
    state = REPORTING_STATE;
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
      
      // Match the rest of the occupancy state machine: only KEEP_ALIVE mode
      // forces an immediate report/connect on occupancy transitions.
      if (sysStatus.get_connectionMode() == INTERMITTENT_KEEP_ALIVE) {
        Log.info("Occupancy change detected (occupied->unoccupied) - triggering immediate report");
        session.occupancyChangeTriggered = true;
        state = REPORTING_STATE;
        return;
      }

      Log.info("Occupancy change detected (occupied->unoccupied) - no immediate report (connectionMode=%d)",
               (int)sysStatus.get_connectionMode());
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
        // Occupancy mode: PIR wakes are expected behavior, not thrashing
        thrashGuard.markProgress("PIR_WAKE_OCCUPANCY");

        // If KEEP_ALIVE was temporarily disabled for low battery, refresh the
        // battery policy on wake so recovered power can restore the intended
        // occupancy behavior before we decide whether to report or sleep again.
        if (sysStatus.get_lowBatteryMode()) {
          measure.batteryState();
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
            debounceMs = 60000; // default 60s
          }
          signalLED(true, debounceMs);
          Log.info("Space now OCCUPIED from PIR wake at %s",
                   Time.timeStr().c_str());
          
          // Match the rest of the occupancy state machine: only KEEP_ALIVE mode
          // forces an immediate report/connect on occupancy transitions.
          if (sysStatus.get_connectionMode() == INTERMITTENT_KEEP_ALIVE) {
            Log.info("Occupancy change detected (unoccupied->occupied) - triggering immediate report");
            session.occupancyChangeTriggered = true;
            setAppBreadcrumb(5);
            setAppBreadcrumb(5);
            state = REPORTING_STATE;
            return;
          }

          Log.info("Occupancy change detected (unoccupied->occupied) - no immediate report (connectionMode=%d)",
                   (int)sysStatus.get_connectionMode());
        } else {
          // Already occupied - motion detected during debounce, restart timer
          uint32_t debounceMs = sensorConfig.get_sensorSetting1();
          if (debounceMs == 0) {
            debounceMs = 60000; // default 60s
          }
          signalLED(true, debounceMs);  // Restart LED timer
          Log.info("Motion from PIR wake - space remains occupied (timer reset to %lu sec)", debounceMs/1000);
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
