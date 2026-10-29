#include "state/State_Common.h"
#include "../Config.h"
#include "cloud/Cloud.h"
#include "LocalTimeRK.h"
#include "persist/CurrentReadings.h"
#include "persist/PowerConfig.h"
#include "persist/SystemConfig.h"
#include "power/BatteryAuthority.h"
#include "power/PowerManager.h"
#include "PublishQueuePosixRK.h"
#include "sensors/SensorManager.h"
#include "ThrashGuard.h"
#include "device_pinout.h"
#include "sensors/SensorDefinitions.h"

// NOTE:
// This file was split from StateHandlers.cpp as a mechanical refactor.
// No behavioral changes were made.

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
    CurrentReadings::set_hourlyCount(CurrentReadings::get_hourlyCount() + 1);
    CurrentReadings::set_dailyCount(CurrentReadings::get_dailyCount() + 1);
    // WO-2026-08-29-002 item 8 (Finding 6, Round 4 review): lastCountTime has
    // no consumers anywhere in the codebase (see MyPersistentData.cpp and
    // State_Sleep.cpp's identical gating for the consumer analysis) - safe
    // to gate on isClockTrusted() so an untrusted clock records the 0
    // sentinel instead of a wrong timestamp, rather than Time.isValid()
    // alone (Finding 3).
    CurrentReadings::set_lastCountTime(isClockTrusted() ? Time.now() : 0);

    // Log the new count once per event
    Log.info("Count detected - Hourly: %d, Daily: %d",
             CurrentReadings::get_hourlyCount(), CurrentReadings::get_dailyCount());

    // Flash the on-module BLUE LED for ~1 second as a
    // visual count indicator (works across sleep cycles)
    signalLED(true, 1000);  // Turn on with 1-second timeout

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
  // loop() always runs first so the sensor flag is consumed; a detection after the close is ignored.
  if (SensorManager::instance().loop() && !occupancyIgnoredWhenClosed()) {
    // Sensor detected presence
    if (!CurrentReadings::get_occupied()) {
      // Transition from unoccupied to occupied
      CurrentReadings::set_occupied(true);
      CurrentReadings::set_occupancyStartTime(Time.now());
      
      // V3.23: Set LED with debounce timeout from sensor.setting1
      uint32_t setting1Raw = SystemConfig::SensorSettings::get_sensorSetting1();
      uint32_t debounceSeconds = setting1Raw / 1000;
      Log.info("OCCUPANCY detection: setting1=%lu ms => debounce=%lu sec",
               (unsigned long)setting1Raw, (unsigned long)debounceSeconds);
      if (debounceSeconds == 0) {
        Log.warn("Debounce timeout is 0 - using 60 sec default (check sensor.setting1=%lu)", 
                 (unsigned long)setting1Raw);
        debounceSeconds = 60;  // Minimum 60 second debounce for occupancy
      }
      signalLED(true, debounceSeconds * 1000UL);  // Turn on LED until debounce expires
      const bool reportNow = reportsOccupancyChangesNow();
      logOccupiedEvent("pir", debounceSeconds, reportNow);
      
      // In INTERMITTENT_KEEP_ALIVE and CONNECTED modes, report immediately on occupancy state changes
      // This allows dashboard to show real-time occupancy transitions
      if (reportNow) {
        session.occupancyChangeTriggered = true;
        if (state == IDLE_STATE) {
          transitionTo(REPORTING_STATE, "occupancy change");
        }
      }
    } else {
      // Already occupied - reset LED timeout on new motion
      uint32_t debounceSeconds = SystemConfig::SensorSettings::get_sensorSetting1() / 1000;
      if (debounceSeconds == 0) {
        debounceSeconds = 60;  // Minimum 60 second debounce for occupancy
      }
      signalLED(true, debounceSeconds * 1000UL);  // Reset LED timeout
      if (SystemConfig::get_verboseMode()) {
        logOccupiedEvent("pir", debounceSeconds, false, true);
      }
    }

    // Update last event time (resets debounce timer)
    CurrentReadings::set_lastOccupancyEvent(millis());

    if (SystemConfig::get_verboseMode()) {
      uint32_t occupiedDuration = Time.now() - CurrentReadings::get_occupancyStartTime();
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
 *          Triggers immediate reporting on state change for DISCONNECTED_KEEP_ALIVE mode.
 */
void updateOccupancyState() {
  if (!CurrentReadings::get_occupied()) {
    return; // Nothing to do if not occupied
  }

  // V3.23: Occupancy debounce timeout comes from sensor.setting1
  const uint32_t debounceMs = occupancyDebounceMs();

  uint32_t lastEvent = CurrentReadings::get_lastOccupancyEvent();
  if (lastEvent == 0) {
    // If this is 0 (for example, occupancy set from a PIR wake path that
    // didn't update lastOccupancyEvent), we must not immediately expire.
    lastEvent = millis();
    CurrentReadings::set_lastOccupancyEvent(lastEvent);
  }
  uint32_t timeSinceLastEvent = millis() - lastEvent;

  // Check if debounce timeout has expired
  if (timeSinceLastEvent > debounceMs) {
    const bool reportNow = reportsOccupancyChangesNow();
    const OccupancyCloseResult closeResult = closeOccupancySessionSafely("modes");
    if (closeResult.valid) {
      logUnoccupiedEvent("debounce", closeResult.sessionSeconds, closeResult.totalSeconds, reportNow);
    }

    signalLED(false);  // Turn off LED
    
    // In INTERMITTENT_KEEP_ALIVE and CONNECTED modes, report immediately on occupancy state changes
    // This allows dashboard to show real-time occupancy transitions
    if (reportNow && !closeResult.stillOpen) {
      session.occupancyChangeTriggered = true;
      if (state == IDLE_STATE) {
        transitionTo(REPORTING_STATE, "occupancy cleared");
      }
    }
  }
}

bool processSleepWakeEvent(bool pirWake) {
    // Check if LED timeout expired FIRST (before processing new PIR wake)
    // This ensures we don't immediately undo state changes from PIR processing
    if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY && signalLEDTimeRemaining() == 0 && signalLEDStatus()) {
      // LED timeout expired - debounce period elapsed without motion
      const bool reportNow = reportsOccupancyChangesNow();
      const OccupancyCloseResult closeResult = closeOccupancySessionSafely("sleep");
      signalLED(false);  // Turn off LED
      if (closeResult.valid) {
        logUnoccupiedEvent("debounce", closeResult.sessionSeconds, closeResult.totalSeconds, reportNow);
      }
      
      // Match the rest of the occupancy state machine: only KEEP_ALIVE and
      // CONNECTED force an immediate report/connect on occupancy transitions.
      if (reportNow && !closeResult.stillOpen) {
        session.occupancyChangeTriggered = true;
        transitionTo(REPORTING_STATE, "sleep-occupancy-debounce-report");
        return true;
      }
    }

    // Process PIR wake events (after checking LED timeout above)
    // If this wake was caused by the PIR interrupt, synthesize a single
    // detection event so that the motion that woke the device is counted
    // even if the ISR flag did not survive ULTRA_LOW_POWER sleep.
    if (pirWake) {
      if (SystemConfig::get_sensorMode() == SystemConfig::COUNTING) {
        CurrentReadings::set_hourlyCount(CurrentReadings::get_hourlyCount() + 1);
        CurrentReadings::set_dailyCount(CurrentReadings::get_dailyCount() + 1);
        // WO-2026-08-29-002 item 8: lastCountTime has no consumers anywhere
        // in this codebase (write-only telemetry field) - gating on
        // isClockTrusted() changes only the recorded value, never control
        // flow.
        CurrentReadings::set_lastCountTime(isClockTrusted() ? Time.now() : 0);
        signalLED(true, 1000);  // Flash for 1 second
        Log.info("Count detected from PIR wake - Hourly: %d, Daily: %d",
                 CurrentReadings::get_hourlyCount(), CurrentReadings::get_dailyCount());
        if (Serial && Serial.isConnected()) {
          Serial.printf("Count detected from PIR wake - Hourly: %d, Daily: %d\r\n",
                        CurrentReadings::get_hourlyCount(), CurrentReadings::get_dailyCount());
        }
      } else if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY) {
        // Occupancy mode: PIR wakes are expected behavior, not thrashing
        thrashGuard.markProgress("PIR_WAKE_OCCUPANCY");
        if (occupancyIgnoredWhenClosed()) {
          Log.info("OCCUPANCY pir-wake ignored: closed");
          return false;
        }

        // If KEEP_ALIVE was temporarily disabled for low battery, refresh the
        // battery policy on wake so recovered power can restore the intended
        // occupancy behavior before we decide whether to report or sleep again.
        // WO-2026-09-21 Step 4 (corrected same day): evaluate() then commit() -
        // one of the four deliberate policy-application sites.
        if (PowerConfig::get_lowBatteryMode()) {
          const float soc = PowerManager::instance().soc();
          float vcell = 0.0f;
          const SensorManager::VcellSampleState vcellState =
              SensorManager::instance().cachedBatteryVoltageState(vcell);
          const BatteryHealth::SocTrust trust = SensorManager::instance().cachedSocTrust();
          BatteryAuthority::commit(
              BatteryAuthority::evaluate(soc, vcellState, vcell, trust, BatteryAuthority::currentTier()),
              soc);
        }
        
        if (!CurrentReadings::get_occupied()) {
          CurrentReadings::set_occupied(true);
          // WO-2026-08-29-002 item 8: deliberately NOT gated on
          // isClockTrusted(). Unlike lastCountTime, occupancyStartTime's 0
          // value is a load-bearing sentinel for two consumers:
          // MyPersistentData.cpp's validate() treats occupied=true with
          // occupancyStartTime==0 as corrupt state and forces occupied=false
          // (a real state-machine side effect, not just a recorded value),
          // and State_Modes.cpp computes `Time.now() - occupancyStartTime`
          // for a logged duration, which would become a multi-decade garbage
          // span if this were 0 while occupied. Writing Time.now() here even
          // when untrusted preserves existing behavior; see the
          // Implementation Report for the Chief Engineer's review.
          CurrentReadings::set_occupancyStartTime(Time.now());
          // Treat the PIR wake as an occupancy event for debounce purposes.
          // If lastOccupancyEvent is left at 0, the debounce logic that uses
          // millis()-based timing will immediately expire and mark UNOCCUPIED.
          CurrentReadings::set_lastOccupancyEvent(millis());

          // Keep LED on for the debounce window (sensor.setting1 is milliseconds).
          uint32_t debounceMs = SystemConfig::SensorSettings::get_sensorSetting1();
          if (debounceMs == 0) {
            debounceMs = Config::occupancyDebounceMsForRuntime();
          }
          signalLED(true, debounceMs);
          const bool reportNow = reportsOccupancyChangesNow();
          logOccupiedEvent("pir-wake", debounceMs / 1000UL, reportNow);
          
          // Match the rest of the occupancy state machine: only KEEP_ALIVE and
          // CONNECTED force an immediate report/connect on occupancy transitions.
          if (reportNow) {
            session.occupancyChangeTriggered = true;
            setAppBreadcrumb(5);
            transitionTo(REPORTING_STATE, "sleep-pir-occupancy-report");
            return true;
          }
        } else {
          // Already occupied - motion detected during debounce, restart timer
          uint32_t debounceMs = SystemConfig::SensorSettings::get_sensorSetting1();
          if (debounceMs == 0) {
            debounceMs = Config::occupancyDebounceMsForRuntime();
          }
          signalLED(true, debounceMs);  // Restart LED timer
          logOccupiedEvent("pir-wake", debounceMs / 1000UL, false, true);
        }
        CurrentReadings::set_lastOccupancyEvent(millis());
      }
    }
  return false;
}
