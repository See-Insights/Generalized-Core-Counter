#pragma once

/**
 * @file CurrentReadings.h
 * @brief Facade: measured values, counts, and occupancy state
 *        (/usr/current.dat, currentStatusData / CurrentData).
 *
 * WO-2026-09-23-001 (Step 5). The alert fields that physically live in
 * CurrentData are deliberately NOT here - decision A puts alertCode /
 * lastAlertTime / raiseAlert() in RecoveryState.h, on concern rather than
 * on storage location.
 *
 * INCLUDES ARE THE POINT OF THIS HEADER. It must never include
 * StorageHelperRK.h, MyPersistentData.h, Particle.h, or a sibling facade.
 */

#include <stdint.h>
#include <time.h>

namespace CurrentReadings {

// *************** Store lifecycle (/usr/current.dat) ***************

/// Forwards to currentStatusData::setup().
void setup();

/// Forwards to currentStatusData::loop() - deferred flush servicing.
void loop();

/**
 * @brief Zero the counts for a new day.
 *
 * Finding 4: this operation is cross-store - it also clears the sysStatus
 * reset count exposed as RecoveryState::set_resetCount(0). That coupling
 * stays inside MyPersistentData.cpp and is unchanged by the split; it is
 * documented here so the behavior is not lost behind an accessor-shaped
 * facade.
 */
void resetEverything();

/**
 * @brief Re-run the occupancyStartTime future-date corruption clamp once a
 *        time source exists this boot.
 *
 * validate()'s own clamp runs from the .load() in setup(), long before
 * ab1805.setup() seeds Time, so it can never fire there (WO-2026-09-22-001).
 * Call this once, opportunistically, from global setup() as soon as
 * Clock::isTimeValid(). No-op if occupied is false or occupancyStartTime is
 * already plausible.
 */
void revalidateOccupancyStartTimeIfTimeAvailable();

// *************** Counting ***************

uint16_t get_hourlyCount();
void set_hourlyCount(uint16_t value);

uint16_t get_dailyCount();
void set_dailyCount(uint16_t value);

time_t get_lastCountTime();
void set_lastCountTime(time_t value);

// *************** Occupancy ***************

bool get_occupied();
void set_occupied(bool value);

/// millis() timestamp of the last occupancy detection.
uint32_t get_lastOccupancyEvent();
void set_lastOccupancyEvent(uint32_t value);

/// Epoch time the current occupancy session started.
time_t get_occupancyStartTime();
void set_occupancyStartTime(time_t value);

uint32_t get_totalOccupiedSeconds();
void set_totalOccupiedSeconds(uint32_t value);

// *************** Measured battery and temperature ***************

float get_stateOfCharge();
void set_stateOfCharge(float value);

uint8_t get_batteryState();
void set_batteryState(uint8_t value);

/// Enclosure temperature in degrees C - the single accepted temperature
/// reading every consumer reads from; never independently re-acquired.
float get_internalTempC();
void set_internalTempC(float value);

float get_externalTempC();
void set_externalTempC(float value);

}  // namespace CurrentReadings
