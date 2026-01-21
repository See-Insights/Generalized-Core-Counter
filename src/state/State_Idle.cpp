#include "state/State_Common.h"
#include "Config.h"
#include "Cloud.h"
#include "LocalTimeRK.h"
#include "MyPersistentData.h"
#include "PublishQueuePosixRK.h"
#include "SensorManager.h"
#include "device_pinout.h"
#include "SensorDefinitions.h"

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
