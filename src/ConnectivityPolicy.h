#pragma once

#include "Particle.h"

/*
 * ConnectivityPolicy.h
 *
 * POLICY, NOT MECHANISM
 * --------------------
 * This file defines connectivity/power *policy* (budgets, thresholds, safety caps).
 * It does not implement how the device connects/disconnects/sleeps; the state
 * machine (and Device OS) provide the *mechanism*.
 *
 * SINGLE SOURCE OF TRUTH
 * ----------------------
 * All connectivity timing/power “budgets” that constrain battery impact and bound
 * blocking/waiting behavior must live here. If you find a new magic number related
 * to connect, cloud-service gates, disconnect, modem-off, or long-term escalation,
 * move it here and reference it by name.
 *
 * SAFETY / CHANGE CONTROL
 * -----------------------
 * Hardware testing is ongoing; this header is documentation + naming only.
 * DO NOT change any values without validating Particle modem behavior and field
 * power impact.
 *
 * Key idea: we operate in poor connectivity + solar/LiPo constraints. Budgets exist
 * to (1) allow the modem enough time to succeed when it can, while (2) preventing
 * pathological hangs from draining the battery or leaving the modem on indefinitely.
 */

// Centralized connectivity timing/power policy constants.
// Mechanical refactor invariant: values must remain identical to prior scattered literals.

namespace ConnectivityPolicy {

// ===== Connection attempt budgets =====
// Purpose:
// - Bound how long we will wait for a single connection attempt before declaring
//   failure and returning to low-power operation.
// - Provide two budgets: a normal budget (IMSI cycling) and a periodic “deep”
//   budget that is long enough to allow a full modem reset path.
//
// Rationale / tradeoffs:
// - Too short: modem may never reach the reset/registration behaviors needed for
//   recovery in marginal RF environments.
// - Too long: repeated long attempts can burn the daily energy budget on solar.
//
// Invariants / safety notes (DANGEROUS to change):
// - CONNECT_BUDGET_DEFAULT_MS MUST remain >= Particle cellular guidance for IMSI
//   cycling (~5 minutes). Reducing below this can permanently increase alert 31
//   rates in weak coverage.
// - CONNECT_BUDGET_DEEP_MS MUST remain >= the window that allows the modem “full
//   reset” behavior (observed around the 10-minute mark). Reducing can eliminate
//   the only recovery path for some SIM/tower edge cases.
//
// Relationship notes:
// - Connect budgets >> service gate and disconnect budgets. We accept waiting
//   longer to connect (when there is a chance), but we do NOT wait long to tear
//   down or to service stuck cloud operations.
constexpr unsigned long CONNECT_BUDGET_DEFAULT_MS = 5UL * 60UL * 1000UL; // 5 minutes
constexpr unsigned long CONNECT_BUDGET_DEEP_MS = 11UL * 60UL * 1000UL;   // 11 minutes
constexpr unsigned int CONNECT_BUDGET_DEFAULT_SEC = 300U;                // 5 minutes
constexpr uint16_t CONNECT_BUDGET_CONFIG_MIN_SEC = 120;                  // Particle docs minimum
constexpr uint16_t CONNECT_BUDGET_CONFIG_MAX_SEC = 900;

// Periodic deep attempts (full modem reset around 10 minute mark)
// Purpose:
// - Decide when we temporarily allow the longer deep budget.
//
// Rationale / tradeoffs:
// - Deep attempts are more energy-expensive. The counter-based threshold prevents
//   doing them every time, while the SoC threshold opportunistically permits them
//   when battery is healthy.
//
// Safety notes:
// - These thresholds are usually safer to tune than the connect budgets themselves,
//   but can still increase energy consumption and carrier-network load if too
//   aggressive.
constexpr uint8_t DEEP_ATTEMPT_COUNTER_THRESHOLD = 3; // deep when attemptCounter >= 3
constexpr float DEEP_ATTEMPT_SOC_THRESHOLD = 50.0f;   // deep when SoC > 50%

// ===== Firmware update (stay-connected) budget =====
// Purpose:
// - Upper-bound how long we remain in a firmware-update dwell state before
//   abandoning and returning to normal low-power behavior.
//
// Rationale / tradeoffs:
// - Keeps OTA from monopolizing the power budget in poor connectivity.
//
// Safety notes:
// - Generally “medium risk” to tune: making it longer increases energy burn;
//   making it shorter can cause updates to fail more often.
constexpr unsigned long FIRMWARE_UPDATE_MAX_MS = 5UL * 60UL * 1000UL;

// ===== Service gate (queue + ledgers + OTA + webhook) before disconnect =====
// Purpose:
// - Before tearing down the cloud session for sleep, give critical cloud-related
//   operations a brief chance to complete (publish queue drain, ledger sync, OTA
//   status, webhook response).
//
// Rationale / tradeoffs:
// - Prevents sleeping while important work is still in flight.
// - Timeout prevents a stuck publish/handshake from keeping the modem on.
//
// Invariants / safety notes:
// - This gate MUST remain much smaller than connect budgets. If this grows too
//   large, we risk “connected but doing nothing” battery drain.
// - This gate directly protects against “modem left on” failures caused by an
//   application-level wait that never completes.
constexpr unsigned long CLOUD_OPS_GATE_TIMEOUT_MS = 30000UL;
constexpr unsigned long CLOUD_OPS_STATUS_LOG_INTERVAL_MS = 5000UL;

// ===== Teardown/disconnect budgets =====
// Purpose:
// - Bound how long we will wait for cloud disconnect and modem power-off to take
//   effect after requesting it.
// - Provide conservative defaults that can be overridden by ledger configuration
//   within a safe min/max range.
//
// Rationale / tradeoffs:
// - Too short: we may declare failure while Device OS is still making progress.
// - Too long: we can sit awake burning energy while the radio remains on.
//
// Safety notes (DANGEROUS):
// - DISCONNECT_BUDGET_MIN_SEC/MAX_SEC protect against accidental “infinite wait”
//   from misconfiguration. Keep these tight enough to avoid runaway battery drain.
// - DISCONNECT_STANDBY_MAX_MS is a hard cap used when network standby is enabled;
//   it prevents spending meaningful connection budget on a perfect teardown.
//
// Relationship notes:
// - Disconnect budgets should remain < any “awake ceiling” implied by your sleep
//   interval strategy; otherwise we can burn most of the cycle just powering down.
constexpr uint16_t DISCONNECT_BUDGET_MIN_SEC = 5;
constexpr uint16_t DISCONNECT_BUDGET_MAX_SEC = 120;
constexpr uint16_t DISCONNECT_CLOUD_DEFAULT_SEC = 15;
constexpr uint16_t DISCONNECT_MODEM_DEFAULT_SEC = 30;
constexpr unsigned long DISCONNECT_STANDBY_MAX_MS = 5000UL;

// ===== Report/connectivity supervision thresholds =====
// Purpose:
// - Long-term health monitoring for webhook delivery/response (alerting and
//   escalation) during OPEN hours, with backoffs to prevent thrash.
//
// Rationale / tradeoffs:
// - These timers are about operational correctness and rate-limiting recovery
//   actions, not modem protocol constraints.
//
// Safety notes:
// - Generally safer to experiment with than modem/connect budgets, but still
//   affects power because it can force extra connections and resets.
constexpr long WEBHOOK_LONGTERM_ALERT40_SEC = 3L * 3600L;
constexpr long WEBHOOK_LONGTERM_FORCE_CONNECT_MIN_INTERVAL_SEC = 30L * 60L;
constexpr long WEBHOOK_LONGTERM_ESCALATE_TO_ERROR_SEC = 6L * 3600L;
constexpr long WEBHOOK_LONGTERM_CONNECTED_RECENTLY_SEC = 2L * 3600L;
constexpr long WEBHOOK_LONGTERM_ESCALATION_COOLDOWN_SEC = 3L * 3600L;

// Boundary alignment tolerance for tier-multiplied intervals
// Purpose:
// - Allow small jitter around computed boundaries so we don’t miss an intended
//   connect window due to processing time or timer drift.
//
// Safety notes:
// - Typically safe to tune; keep small relative to shortest effective interval.
constexpr time_t CONNECT_ALIGNMENT_TOLERANCE_SEC = 30;

// ===== Debug serial (post-sleep USB re-enumeration) =====
// Used only under DEBUG_SERIAL in State_Sleep.
// Purpose:
// - Make post-sleep logging reliable on platforms where USB serial needs time to
//   re-enumerate after ULTRA_LOW_POWER.
//
// Rationale / tradeoffs:
// - Delays are developer-experience oriented; they should not be enabled in
//   production power-critical builds.
//
// Safety notes:
// - Safe to tune for development ergonomics; impacts wake-time energy only when
//   DEBUG_SERIAL is enabled.
constexpr unsigned long DEBUG_SERIAL_REENUM_DELAY_MS = 500UL;
constexpr unsigned long DEBUG_SERIAL_WAIT_TIMEOUT_MS = 30000UL;
constexpr unsigned long DEBUG_SERIAL_WAIT_POLL_DELAY_MS = 100UL;
constexpr unsigned long DEBUG_SERIAL_POST_CONNECT_DELAY_MS = 500UL;

} // namespace ConnectivityPolicy
