#pragma once

#include <stdint.h>

/**
 * @file ResetCause.h
 * @brief Distinct, non-zero codes passed to System.reset(uint32_t data).
 *
 * Device OS 6.4.1 resets with RESET_REASON_USER (140) and hands `data` back to
 * the next boot as System.resetReasonData(), which the startup status already
 * publishes. So reason 140 with data 0 means the reset came from outside these
 * sites (the AB1805 library fallback, a cloud reset, or Device OS itself).
 * WO-2026-10-01-001 item C.
 */
enum ResetCause : uint32_t {
  RESET_CAUSE_APP_WATCHDOG = 1,            ///< appWatchdogHandler() (non-Wiring_Watchdog builds)
  RESET_CAUSE_CONNECTIVITY_FAILSAFE = 2,   ///< Connectivity failsafe stage 2
  RESET_CAUSE_THRASH_TIER3 = 3,            ///< ThrashGuard tier 3
  RESET_CAUSE_SLEEP_HEAP_GUARD = 4,        ///< Nightly heap guard before overnight sleep
  RESET_CAUSE_SLEEP_ATTEMPTS_EXHAUSTED = 5,///< All sleep attempts failed
  RESET_CAUSE_ERROR_STATE_SOFT = 6,        ///< ERROR_STATE soft reset
};
