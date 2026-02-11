#include "state/State_Common.h"
#include "Config.h"
#include "cloud/Cloud.h"
#include "LocalTimeRK.h"
#include "MyPersistentData.h"
#include "PublishQueuePosixRK.h"
#include "sensors/SensorManager.h"
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
    current.set_hourlyCount(current.get_hourlyCount() + 1);
    current.set_dailyCount(current.get_dailyCount() + 1);
    current.set_lastCountTime(Time.now());

    // Log the new count once per event
    Log.info("Count detected - Hourly: %d, Daily: %d",
             current.get_hourlyCount(), current.get_dailyCount());

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
  if (SensorManager::instance().loop()) {
    // Sensor detected presence
    if (!current.get_occupied()) {
      // Transition from unoccupied to occupied
      current.set_occupied(true);
      current.set_occupancyStartTime(Time.now());
      
      // V3.23: Set LED with debounce timeout from sensor.setting1
      uint32_t setting1Raw = sensorConfig.get_sensorSetting1();
      uint32_t debounceSeconds = setting1Raw / 1000;
      Log.info("OCCUPANCY detection: setting1=%lu ms => debounce=%lu sec",
               (unsigned long)setting1Raw, (unsigned long)debounceSeconds);
      if (debounceSeconds == 0) {
        Log.warn("Debounce timeout is 0 - using 60 sec default (check sensor.setting1=%lu)", 
                 (unsigned long)setting1Raw);
        debounceSeconds = 60;  // Minimum 60 second debounce for occupancy
      }
      signalLED(true, debounceSeconds * 1000UL);  // Turn on LED until debounce expires

      Log.info("Space now OCCUPIED at %s (LED timeout in %lu sec)", Time.timeStr().c_str(), debounceSeconds);
      
      // In INTERMITTENT_KEEP_ALIVE mode, report immediately on occupancy state changes
      // This allows dashboard to show real-time occupancy transitions
      if (sysStatus.get_connectionMode() == INTERMITTENT_KEEP_ALIVE) {
        Log.info("Occupancy change detected - triggering immediate report");
        session.occupancyChangeTriggered = true;
        state = REPORTING_STATE;
      }
    } else {
      // Already occupied - reset LED timeout on new motion
      uint32_t debounceSeconds = sensorConfig.get_sensorSetting1() / 1000;
      if (debounceSeconds == 0) {
        debounceSeconds = 60;  // Minimum 60 second debounce for occupancy
      }
      signalLED(true, debounceSeconds * 1000UL);  // Reset LED timeout
      if (sysStatus.get_verboseMode()) {
        Log.info("Motion detected - LED timeout reset to %lu sec", debounceSeconds);
      }
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
 *          Triggers immediate reporting on state change for DISCONNECTED_KEEP_ALIVE mode.
 */
void updateOccupancyState() {
  if (!current.get_occupied()) {
    return; // Nothing to do if not occupied
  }

  // V3.23: Occupancy debounce timeout comes from sensor.setting1
  uint32_t debounceMs = sensorConfig.get_sensorSetting1();
  if (debounceMs == 0) {
    debounceMs = 60000; // default 60s
  }

  uint32_t lastEvent = current.get_lastOccupancyEvent();
  if (lastEvent == 0) {
    // If this is 0 (for example, occupancy set from a PIR wake path that
    // didn't update lastOccupancyEvent), we must not immediately expire.
    lastEvent = millis();
    current.set_lastOccupancyEvent(lastEvent);
  }
  uint32_t timeSinceLastEvent = millis() - lastEvent;

  // Check if debounce timeout has expired
  if (timeSinceLastEvent > debounceMs) {
    Log.info("[OCC DBG] debounceMs=%lu nowMs=%lu lastEventMs=%lu sinceMs=%lu start=%lu now=%lu totalSec=%lu",
             (unsigned long)debounceMs,
             (unsigned long)millis(),
             (unsigned long)lastEvent,
             (unsigned long)timeSinceLastEvent,
             (unsigned long)current.get_occupancyStartTime(),
             (unsigned long)Time.now(),
             (unsigned long)current.get_totalOccupiedSeconds());
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

    signalLED(false);  // Turn off LED
    
    // In INTERMITTENT_KEEP_ALIVE mode, report immediately on occupancy state changes
    // This allows dashboard to show real-time occupancy transitions
    if (sysStatus.get_connectionMode() == INTERMITTENT_KEEP_ALIVE) {
      Log.info("Occupancy change detected - triggering immediate report");
      session.occupancyChangeTriggered = true;
      state = REPORTING_STATE;
    }
  }
}
