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

enum LoopStage : uint8_t {
	LOOP_STAGE_NONE = 0,
	LOOP_STAGE_STATE_HANDLER = 1,
	LOOP_STAGE_CLOUD_LOOP = 2,
	LOOP_STAGE_PUBLISH_QUEUE = 3,
	LOOP_STAGE_DIAGNOSTICS = 4,
	LOOP_STAGE_IDLE_PROCESSING = 5,
	LOOP_STAGE_SLEEP_PREP = 6,
	LOOP_STAGE_CONNECTIVITY = 7,
};

/**
 * @brief Retained (RAM-persisted across sleep/reset) loop-stage forensics.
 *
 * sleepPrepSpanStartMillis is a dedicated elapsed-time reference for the
 * SLEEP_PREP exit-log design (see WO-2026-08-11-001 Fourth Corrective
 * Pass). It is NOT the same as stageStartMillis - that field is
 * contaminated by loop()'s per-iteration stage-tag churn. It is set exactly
 * once per genuine SLEEPING_STATE dwell (zero-gated, in
 * handleSleepingState()) and reset to 0 by transitionTo()'s choke-point
 * guard (Generalized-Core-Counter.cpp) on genuine exit, from any call site
 * in any file.
 */
struct RetainedLoopForensics {
	uint32_t magic;
	uint8_t version;
	uint8_t lastBreadcrumb;
	uint8_t lastLoopStage;
	uint8_t currentState;
	uint16_t publishQueueDepth;
	uint32_t stageStartMillis;
	uint32_t lastLoopStageElapsed;
	uint32_t millisSinceLastCloudConnect;
	uint32_t sleepPrepSpanStartMillis;
};

extern retained RetainedLoopForensics retainedLoopForensics;

/**
 * @brief Logs the shared five-field "LoopStage:" diagnostic line.
 *
 * Used both by noteLoopStageDuration()'s WARN/ERROR threshold escalation
 * and by transitionTo()'s once-per-real-span SLEEP_PREP exit log (see
 * WO-2026-08-11-001 Fourth Corrective Pass).
 *
 * @param level LOG_LEVEL_INFO/WARN/ERROR
 * @param stage Loop stage the line describes
 * @param elapsedMs Elapsed time for the stage/span
 * @param queueDepth Publish queue depth at log time
 * @param millisSinceLastCloudConnect Time since the last successful cloud connect
 */
void logLoopStageLine(LogLevel level, LoopStage stage, unsigned long elapsedMs,
                       uint16_t queueDepth, uint32_t millisSinceLastCloudConnect);

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
 * @param resolvedTier Tier already selected by the shared reporting resolver
 */
void applyBatteryAwareConnectionModePolicy(float currentSoC, BatteryTier resolvedTier);
void applyBatteryAwareConnectionModePolicy(float currentSoC);

/**
 * @brief Tests whether an epoch falls within configured local open hours.
 *
 * @param epoch UTC epoch to evaluate
 * @return true when reporting is allowed at that local time
 */
bool isWithinOpenHoursAt(time_t epoch);

/**
 * @brief Stores an application breadcrumb for the next boot.
 *
 * @param code Retained breadcrumb code
 */
void setAppBreadcrumb(uint8_t code);

/**
 * @brief Stores the active loop stage for watchdog forensics.
 *
 * @param stage Current high-level loop stage
 */
void setLoopStage(LoopStage stage);

// Retained hibernate diagnostics (Boron RTC-alarm hibernate path)
extern retained time_t retainedHibernateRtcBefore;
extern retained time_t retainedHibernateWakeTime;
extern retained uint32_t retainedHibernateRequestedSleep;
extern retained uint32_t retainedHibernateCount;
extern retained bool retainedHibernatePending;

/**
 * @brief Clears persisted connectivity failsafe state after successful recovery.
 *
 * @param reason Short log label describing why recovery state was cleared
 */
void clearConnectivityFailsafeRecovery(const char *reason);

/**
 * @brief Logs a compact time/park-hours diagnostic snapshot.
 *
 * @param isOpen Current open-hours decision
 */
void logTimeDiag(bool isOpen);

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

/**
 * @brief Result of safely closing an occupancy session.
 */
struct OccupancyCloseResult {
	bool valid = false;
	uint32_t sessionSeconds = 0;
	uint32_t totalSeconds = 0;
};

/**
 * @brief Safely closes the current occupancy session and guards wrapped time math.
 *
 * Unsigned subtraction of Time.now() - occupancyStartTime can wrap into a huge
 * bogus duration if the stored start time is zero or in the future.
 *
 * @param path Short caller label for anomaly logs
 * @return Close result including the session duration and new total on valid close
 */
inline OccupancyCloseResult closeOccupancySessionSafely(const char *path) {
	OccupancyCloseResult result;
	const bool occupied = current.get_occupied();
	const int8_t alertCode = current.get_alertCode();
	const bool timeValid = Time.isValid();
	const time_t now = Time.now();
	const time_t start = current.get_occupancyStartTime();
	const uint32_t previousTotal = current.get_totalOccupiedSeconds();
	result.totalSeconds = previousTotal;

	bool durationComputable = false;
	long long rawSessionSeconds = 0;
	uint32_t sessionSeconds = 0;
	bool invalid = !timeValid || start == 0;

	if (timeValid && start != 0) {
		time_t effectiveStart = start;
		if (start > now && start <= now + 5) {
			effectiveStart = now;
		}
		rawSessionSeconds = (long long)now - (long long)effectiveStart;
		durationComputable = true;
	}

	if (timeValid && start > now + 5) {
		invalid = true;
	}

	if (!invalid && durationComputable) {
		sessionSeconds = (uint32_t)rawSessionSeconds;
		const uint32_t newTotal = previousTotal + sessionSeconds;
		if (sessionSeconds > 86400UL || newTotal > 86400UL) {
			invalid = true;
		} else {
			current.set_totalOccupiedSeconds(newTotal);
			result.valid = true;
			result.sessionSeconds = sessionSeconds;
			result.totalSeconds = newTotal;
		}
	} else if (!invalid) {
		invalid = true;
	}

	if (invalid) {
		char sessionBuf[24];
		if (durationComputable) {
			snprintf(sessionBuf, sizeof(sessionBuf), "%lld", rawSessionSeconds);
		} else {
			strncpy(sessionBuf, "na", sizeof(sessionBuf));
			sessionBuf[sizeof(sessionBuf) - 1] = '\0';
		}
		Log.warn("OccAnom: path=%s now=%lu start=%lu prev=%lu dur=%s occ=%d alert=%d",
					 path ? path : "?",
					 (unsigned long)now,
					 (unsigned long)start,
					 (unsigned long)previousTotal,
					 sessionBuf,
					 occupied ? 1 : 0,
					 (int)alertCode);
	}

	current.set_occupied(false);
	current.set_occupancyStartTime(0);
	return result;
}
