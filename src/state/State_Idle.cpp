#include "state/State_Common.h"
#include "../Config.h"
#include "cloud/Cloud.h"
#include "LocalTimeRK.h"
#include "MyPersistentData.h"
#include "PublishQueuePosixRK.h"
#include "sensors/SensorManager.h"
#include "device_pinout.h"
#include "sensors/SensorDefinitions.h"
#include "power/Connectivity.h"
#include "ThrashGuard.h"

// NOTE:
// This file was split from StateHandlers.cpp as a mechanical refactor.
// No behavioral changes were made.

void ensureSensorEnabled(const char* context) {
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
  setLoopStage(LOOP_STAGE_IDLE_PROCESSING);

  if (state != oldState) {
    publishStateTransition();
  }

  // Process LED timers for timed flashes (COUNTING mode)
  signalLEDUpdate();

  // Maintain LED state for OCCUPANCY mode and check debounce timeout
  if (sysStatus.get_sensorMode() == OCCUPANCY) {
    // Check if debounce timeout expired (no motion for debounce period)
    if (current.get_occupied()) {
      uint32_t debounceMs = sensorConfig.get_sensorSetting1();
      if (debounceMs == 0) {
        debounceMs = Config::occupancyDebounceMsForRuntime();
      }

      uint32_t lastEvent = current.get_lastOccupancyEvent();
      if (lastEvent == 0) {
        lastEvent = millis();
        current.set_lastOccupancyEvent(lastEvent);
      }
      uint32_t timeSinceLastEvent = millis() - lastEvent;

      if (timeSinceLastEvent >= debounceMs) {
        // Debounce timeout expired - space is now unoccupied
        const bool reportNow = (sysStatus.get_connectionMode() == INTERMITTENT_KEEP_ALIVE);
        const OccupancyCloseResult closeResult = closeOccupancySessionSafely("idle");
        signalLED(false);  // Turn off LED
        if (closeResult.valid) {
          logUnoccupiedEvent("debounce", closeResult.sessionSeconds, closeResult.totalSeconds, reportNow);
        }
        
        // In INTERMITTENT_KEEP_ALIVE mode (connectionMode 3), report immediately
        // on occupancy transitions. In other modes, occupancy transitions are
        // still tracked, but do not force an immediate report/connect.
        if (reportNow) {
          session.occupancyChangeTriggered = true;
          state = REPORTING_STATE;
          return;
        }
      }
    }
    
    // Maintain LED state based on occupancy
    if (current.get_occupied() && !signalLEDStatus()) {
      signalLED(true);   // Keep LED on while occupied
    } else if (!current.get_occupied() && signalLEDStatus()) {
      signalLED(false);  // Ensure LED off when unoccupied
    }
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
  if (Time.isValid() && sysStatus.get_connectionMode() == CONNECTED) {
    const bool openNow = isWithinOpenHours();
    logTimeDiag(openNow);
    if (!openNow) {
      Log.info("CONNECTED mode: park CLOSED - transitioning to SLEEPING_STATE for overnight sleep");
      state = SLEEPING_STATE;
      return;
    }
    // Park is open: remain awake in CONNECTED mode.

  }

  // ********** Scheduled Mode Sampling **********
  // SCHEDULED mode uses time-based sampling (non-interrupt).
  // Interrupt-driven modes (COUNTING/OCCUPANCY) are handled centrally in main loop().
  if (sysStatus.get_sensorMode() == MEASUREMENT) {
    if (Time.isValid()) {
      static time_t lastScheduledSample = 0;
      uint16_t intervalSec = Config::reportingIntervalSecForRuntime();

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
  {
    static uint16_t lastQueueDepth = 0xFFFF;
    if (Particle.connected()) {
      uint16_t depth = (uint16_t)PublishQueuePosix::instance().getNumEvents();
      if (lastQueueDepth != 0xFFFF && depth < lastQueueDepth) {
        thrashGuard.markProgress("QUEUE_DRAIN");
      }
      lastQueueDepth = depth;
    } else {
      lastQueueDepth = 0xFFFF;
    }
  }
  if (session.firstConnectionObserved && !session.firstConnectionQueueDrainedLogged && Particle.connected()) {
    if (PublishQueuePosix::instance().getCanSleep() &&
        PublishQueuePosix::instance().getNumEvents() == 0) {
      session.firstConnectionQueueDrainedLogged = true;
    }
  }

  // ********** Scheduled Reporting **********
  // Use the configured reportingIntervalSec to determine when to
  // generate a periodic report, regardless of trigger mode.
  if (Time.isValid() && isWithinOpenHours()) {
    // In OCCUPANCY + INTERMITTENT_KEEP_ALIVE mode, do not generate periodic
    // reports while occupied. Occupancy=1 should only be reported on the
    // transition 0->1 (and 1->0 when it clears).
    if (sysStatus.get_sensorMode() == OCCUPANCY && current.get_occupied() &&
        sysStatus.get_connectionMode() == INTERMITTENT_KEEP_ALIVE) {
      // Skip periodic reporting while occupied in this mode.
    } else {
    uint16_t intervalSec = Config::reportingIntervalSecForRuntime();

    time_t now = Time.now();
    time_t lastReport = sysStatus.get_lastReport();
    if (lastReport == 0 || (now - lastReport) >= intervalSec) {
      int secondsOverdue = (lastReport == 0) ? 0 : (int)(now - lastReport - intervalSec);
      if (secondsOverdue > 0) {
        Log.info("IDLE: Report overdue by %d seconds - transitioning to REPORTING_STATE", secondsOverdue);
      } else {
        Log.info("IDLE: Scheduled report interval reached - transitioning to REPORTING_STATE");
      }
      state = REPORTING_STATE;
      return;
    }
    }
  }

  

  // ********** Power Management **********
  // In INTERMITTENT (1) or DISCONNECTED (2) modes, manage connection lifecycle.
  if (sysStatus.get_connectionMode() != CONNECTED) {
    // In CONNECTED mode during open hours, never auto-sleep.
    if (Time.isValid() && sysStatus.get_connectionMode() == CONNECTED && isWithinOpenHours()) {
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
      // If a sensor event is still pending or the BLUE LED is still on
      // from a recent count, defer transitioning into the SLEEPING_STATE.
      // This avoids rapid Idle<->Sleeping ping-pong and the associated
      // extra logging while still honouring the low-power policy once
      // the indication has finished.
      // NOTE: In OCCUPANCY mode, we intentionally allow sleep even while the
      // debounce/LED timer is running. The PIR interrupt will wake us early
      // for new motion, and SLEEPING_STATE already wakes on timer to evaluate
      // the debounce timeout. Keeping the device awake here causes it to
      // remain connected and burn power during continued motion.
      if (sensorDetect || (sysStatus.get_sensorMode() == COUNTING && signalLEDTimeRemaining() > 0)) {
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

  // ********** IDLE Connectivity Ceiling Safety Net **********
  // ThrashGuard intentionally does not supervise IDLE_STATE, so enforce a
  // conservative max awake ceiling when cloud/modem remains powered without
  // meaningful work. This prevents modem-on battery drain wedges.
  {
    static unsigned long idleCeilingStartMs = 0;

    const unsigned long nowMs = millis();
    const bool cloudConnected = Particle.connected();
    const bool radioOn = Connectivity::isRadioPoweredOn();
    const bool connectivityPowered = cloudConnected || radioOn;
    const bool updatesPending = System.updatesPending();

    bool queueCanSleep = true;
    if (cloudConnected) {
      queueCanSleep = PublishQueuePosix::instance().getCanSleep();
    }

    const bool openHoursKeepAwakeValid = Time.isValid() && isWithinOpenHours();
    const bool healthyConnectedAwakePath =
      (sysStatus.get_connectionMode() == CONNECTED) &&
      openHoursKeepAwakeValid &&
      cloudConnected;

    const bool noMeaningfulWorkRemains = !updatesPending && queueCanSleep;
    const bool shouldApplyIdleCeiling =
      connectivityPowered &&
      noMeaningfulWorkRemains &&
      !healthyConnectedAwakePath;

    if (!shouldApplyIdleCeiling) {
      idleCeilingStartMs = 0;
    } else {
      // Reuse existing connectAttemptBudgetSec behavior with conservative fallback.
      uint16_t budgetSec = sysStatus.get_connectAttemptBudgetSec();
      if (budgetSec < 30 || budgetSec > 900) {
        budgetSec = 300;
      }
      const unsigned long budgetMs = (unsigned long)budgetSec * 1000UL;

      // Use connectedStartMs when available; otherwise track this IDLE-powered
      // dwell with a local timer so radio-on/cloud-off wedges are also bounded.
      const unsigned long startMs = (connectedStartMs != 0) ? connectedStartMs : idleCeilingStartMs;
      if (startMs == 0) {
        idleCeilingStartMs = nowMs;
      } else {
        const unsigned long elapsedMs = nowMs - startMs;
        if (elapsedMs > budgetMs) {
          Log.warn("IDLE ceiling trip: mode=%d cloud=%d radioOn=%d elapsedMs=%lu connectedStartMs=%lu -> forcing teardown and sleep",
                   (int)sysStatus.get_connectionMode(),
                   (int)cloudConnected,
                   (int)radioOn,
                   elapsedMs,
                   connectedStartMs);

          Connectivity::requestFullDisconnectAndRadioOff();
          connectedStartMs = 0;
          idleCeilingStartMs = 0;
          state = SLEEPING_STATE;
          return;
        }
      }
    }
  }
}
