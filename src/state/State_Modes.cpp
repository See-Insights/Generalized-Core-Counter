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
