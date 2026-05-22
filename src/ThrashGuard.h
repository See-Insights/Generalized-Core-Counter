#pragma once

#include "Particle.h"

/**
 * @file ThrashGuard.h
 * @brief State machine watchdog - prevents infinite loops and thrashing
 * 
 * @details ThrashGuard provides timeout protection for each state in the
 * state machine. If a state doesn't show forward progress within its timeout
 * window, escalating corrective actions are taken:
 * 
 * Tier 1: Backoff only (transient issue)
 * Tier 2: Force sleep + disconnect (stuck in waiting state) + alert 18
 * Tier 3: System.reset() (repeated failures within 1 hour) + alert 18
 * 
 * State-specific timeouts (seconds):
 * - SLEEPING_STATE: 60s (cloud ops + disconnect wait)
 * - CONNECTING_STATE: 120s (cloud connect + config sync)
 * - REPORTING_STATE: 120s (webhook publish + queue)
 * - FIRMWARE_UPDATE_STATE: 180s (OTA download)
 * - INITIALIZATION/ERROR: 60s
 * - IDLE: No timeout (fast-cycling state)
 */

class ThrashGuard {
public:
  /**
   * @brief Marks forward progress within the current state.
   *
   * @param tag Short label describing the progress milestone
   */
  void markProgress(const char *tag);

  /**
   * @brief Records a state transition and resets internal progress tracking as needed.
   *
   * @param oldState Prior top-level state
   * @param newState New top-level state
   */
  void recordStateTransition(int oldState, int newState);

  /**
   * @brief Evaluates the current state for no-progress timeout escalation.
   *
   * @param currentState Active top-level state
   * @param nowMs Current `millis()` value
   */
  void loop(int currentState, uint32_t nowMs);

  /**
   * @brief Adjusts the cooldown between repeated thrash log messages.
   *
   * @param cooldownMs Minimum interval between repeated log lines
   */
  void setLogCooldownMs(uint32_t cooldownMs);

private:
  uint32_t timeoutForStateSec(int currentState) const;
  void logTrip(int currentState, uint32_t noprogSec, const char *tag, int tier, const char *action);

  uint32_t lastProgressMs_ = 0;
  uint32_t lastTripMs_ = 0;
  uint32_t backoffUntilMs_ = 0;
  uint32_t logCooldownMs_ = 5 * 60 * 1000UL;
  uint32_t lastLogMs_ = 0;
  uint32_t windowStartMs_ = 0;
  uint8_t recentTripCount_ = 0;

  char lastTag_[24] = "BOOT";
};

extern ThrashGuard thrashGuard;

// Retained counters for field diagnostics.
extern retained uint16_t thrashTripCount;
extern retained uint16_t thrashResetCount;
extern retained uint8_t lastThrashReason;
