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
