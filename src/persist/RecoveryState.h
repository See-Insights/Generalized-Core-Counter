#pragma once

/**
 * @file RecoveryState.h
 * @brief Facade: recovery escalation, watchdog forensics, and alert state.
 *
 * WO-2026-09-23-001 (Step 5), decision A: the alert fields
 * (alertCode / lastAlertTime / raiseAlert) are exposed here even though they
 * physically live in CurrentData, because they are forensics/alert tracking -
 * the concern this facade is named for - not sensor readings. A facade is a
 * header-level grouping, not new storage: the declarations below forward into
 * the existing currentStatusData object, unchanged. Severity arbitration and
 * timestamp behavior in raiseAlert() are preserved exactly.
 *
 * INCLUDES ARE THE POINT OF THIS HEADER. It must never include
 * StorageHelperRK.h, MyPersistentData.h, Particle.h, or a sibling facade.
 */

#include <stdint.h>
#include <time.h>

namespace RecoveryState {

/**
 * @brief Identifies which mechanism was responsible for the most recently
 *        recorded watchdog reset. Relocated from MyPersistentData.h,
 *        values unchanged.
 *
 * DEVICE_OS:  Device OS itself reported RESET_REASON_WATCHDOG.
 * AB1805_PIN: Device OS reported RESET_REASON_PIN_RESET, but the AB1805
 *             RTC/watchdog chip's own wake-reason register confirmed it
 *             fired the reset pin externally.
 */
enum WatchdogSource : uint8_t {
    WATCHDOG_SOURCE_DEVICE_OS  = 0,
    WATCHDOG_SOURCE_AB1805_PIN = 1
};

// *************** Reset accounting ***************

/// Boot reset count (0-255). Cleared as part of CurrentReadings::resetEverything().
uint8_t get_resetCount();
void set_resetCount(uint8_t value);

// *************** Connectivity failsafe escalation ***************

/// 0 = none, 1 = radio reset, 2 = system reset, 3 = deep power-down.
uint8_t get_connectivityRecoveryStage();
void set_connectivityRecoveryStage(uint8_t value);

/// Unix time of the last recovery action or defer decision.
time_t get_lastConnectivityRecoveryAction();
void set_lastConnectivityRecoveryAction(time_t value);

/// Recovery actions taken since the last good cloud session.
uint8_t get_connectivityRecoveryCount();
void set_connectivityRecoveryCount(uint8_t value);

// *************** Watchdog forensics ***************

uint16_t get_watchdogResetCount();
void set_watchdogResetCount(uint16_t value);

uint8_t get_lastWatchdogBreadcrumb();
void set_lastWatchdogBreadcrumb(uint8_t value);

uint32_t get_lastWatchdogUptimeMs();
void set_lastWatchdogUptimeMs(uint32_t value);

uint32_t get_lastWatchdogResetReasonData();
void set_lastWatchdogResetReasonData(uint32_t value);

/// WatchdogSource enum value.
uint8_t get_lastWatchdogSource();
void set_lastWatchdogSource(uint8_t value);

// *************** Alert state (decision A - backed by CurrentData) ***************

int8_t get_alertCode();
void set_alertCode(int8_t value);

/**
 * @brief Raise an alert, keeping the highest severity code when multiple occur.
 *
 * Forwards to the existing currentStatusData::raiseAlert(). Severity
 * arbitration (a later, less serious warning must not mask a prior critical
 * condition) and the lastAlertTime stamping behavior are unchanged.
 */
void raiseAlert(int8_t value);

time_t get_lastAlertTime();
void set_lastAlertTime(time_t value);

}  // namespace RecoveryState
