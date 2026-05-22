#pragma once

#include "Particle.h"
#include "MyPersistentData.h"
#include "state/StateHandlers.h"
#include "state/StateMachine.h"

/**
 * @file State_Common.h
 * @brief Shared helper declarations used by multiple state handlers.
 */

/**
 * @brief Reasons the long-duration connectivity failsafe intentionally deferred action.
 */
enum FailsafeDeferReason : uint8_t {
	FAILSAFE_DEFER_NONE = 0,
	FAILSAFE_DEFER_INVALID_TIME = 1,
	FAILSAFE_DEFER_NO_LAST_CONNECTION = 2,
	FAILSAFE_DEFER_DISCONNECTED_MODE = 3,
	FAILSAFE_DEFER_UPDATE_PENDING = 4,
	FAILSAFE_DEFER_LOW_BATTERY_HARD_STAGE_SUPPRESSED = 5,
	FAILSAFE_DEFER_CLOSED_HOURS_LONG_SLEEP = 6,
};

// NOTE:
// This file was split from StateHandlers.cpp as a mechanical refactor.
// No behavioral changes were made.

/**
 * @brief Enables or re-enables the active sensor when a state requires it.
 *
 * @param context Short logging label for the caller
 */
void ensureSensorEnabled(const char* context);

/**
 * @brief Publishes a diagnostic event only when the current state allows it safely.
 *
 * @param eventName Particle event name
 * @param data Event payload
 * @param flags Particle publish flags
 * @return true when the publish was accepted
 */
extern bool publishDiagnosticSafe(const char* eventName, const char* data, PublishFlags flags);

/**
 * @brief Runs once-per-day cleanup work.
 */
void dailyCleanup();

/**
 * @brief Publishes the current report payload.
 */
void publishData();

/**
 * @brief Applies battery-aware policy overrides to the current connection mode.
 *
 * @param currentSoC Latest battery state of charge
 * @return Resulting battery tier used for the policy decision
 */
BatteryTier applyBatteryAwareConnectionModePolicy(float currentSoC);

/**
 * @brief Stores an application breadcrumb for the next boot.
 *
 * @param code Retained breadcrumb code
 */
void setAppBreadcrumb(uint8_t code);

/**
 * @brief Clears persisted connectivity failsafe state after successful recovery.
 *
 * @param reason Short log label describing why recovery state was cleared
 */
void clearConnectivityFailsafeRecovery(const char *reason);

/**
 * @brief Returns true when CONNECTING_STATE still owns an in-budget connect attempt.
 *
 * @return true when long-duration failsafe actions should defer to CONNECTING_STATE
 */
bool activeConnectAttemptWithinBudget();

/**
 * @brief Claims a one-shot defer reason log slot for the current boot.
 *
 * @param reason Reason to record
 * @return true when the caller should emit the corresponding log line
 */
bool claimFailsafeDeferLog(FailsafeDeferReason reason);

/**
 * @brief Converts a defer reason enum to a stable log label.
 *
 * @param reason Failsafe defer reason to stringify
 * @return Stable C string describing the reason
 */
const char *failsafeDeferReasonName(FailsafeDeferReason reason);

/**
 * @brief Logs an occupied-state transition or refresh in a consistent format.
 *
 * @param reason Short reason label
 * @param ledSeconds Configured LED hold time in seconds
 * @param report true when the event triggered a report
 * @param reset true when the log reflects a reset or refresh of the occupied timer
 */
inline void logOccupiedEvent(const char *reason,
													 uint32_t ledSeconds,
													 bool report,
													 bool reset = false) {
	if (reset) {
		Log.info("Occ: state=1 reason=%s reset=1 led=%lus",
						 reason,
						 (unsigned long)ledSeconds);
		return;
	}

	Log.info("Occ: state=1 reason=%s led=%lus report=%d",
					 reason,
					 (unsigned long)ledSeconds,
					 report ? 1 : 0);
}

/**
 * @brief Logs an unoccupied-state transition in a consistent format.
 *
 * @param reason Short reason label
 * @param sessionSeconds Occupied duration for the just-ended session
 * @param totalSeconds Total occupied seconds accumulated for the day
 * @param report true when the transition triggered a report
 */
inline void logUnoccupiedEvent(const char *reason,
														 uint32_t sessionSeconds,
														 uint32_t totalSeconds,
														 bool report) {
	Log.info("Occ: state=0 reason=%s session=%lus total=%lus report=%d",
					 reason,
					 (unsigned long)sessionSeconds,
					 (unsigned long)totalSeconds,
					 report ? 1 : 0);
}