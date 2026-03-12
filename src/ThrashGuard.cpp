#include "ThrashGuard.h"
#include "state/StateMachine.h"
#include "power/Connectivity.h"
#include "MyPersistentData.h"

/**
 * @file ThrashGuard.cpp
 * @brief State machine timeout protection and thrash detection
 * 
 * @details Monitors forward progress in each state and escalates corrective actions
 * when the state machine appears stuck:
 * 
 * - Tier 1: Local backoff (10s delay, no state change)
 * - Tier 2: Force disconnect + enter SLEEPING_STATE, raises alert 17
 * - Tier 3: System.reset() after recent repeated trips, raises alert 17
 * 
 * Prevents infinite loops from:
 * - Blocking operations without progress markers
 * - Failed cloud/modem operations that don't timeout properly
 * - Unexpected Device OS behavior during sleep/wake
 * 
 * Progress is tracked via markProgress() calls throughout the codebase.
 */

ThrashGuard thrashGuard;

retained uint16_t thrashTripCount = 0;
retained uint16_t thrashResetCount = 0;
retained uint8_t lastThrashReason = 0;

namespace {
constexpr uint32_t BACKOFF_MS = 10 * 1000UL;
constexpr uint32_t TIER2_WINDOW_MS = 10 * 60 * 1000UL;
constexpr uint32_t TIER3_WINDOW_MS = 60 * 60 * 1000UL;
constexpr uint8_t TIER3_TRIP_COUNT = 3;
constexpr uint8_t REASON_NOPROGRESS = 1;
} // namespace

void ThrashGuard::setLogCooldownMs(uint32_t cooldownMs) {
  logCooldownMs_ = cooldownMs;
}

void ThrashGuard::markProgress(const char *tag) {
  if (tag && tag[0] != '\0') {
    strncpy(lastTag_, tag, sizeof(lastTag_) - 1);
    lastTag_[sizeof(lastTag_) - 1] = '\0';
  }
  lastProgressMs_ = millis();
}

void ThrashGuard::recordStateTransition(int oldState, int newState) {
  (void)oldState;
  (void)newState;
  // State transitions represent forward progress in the state machine.
  lastProgressMs_ = millis();
}

uint32_t ThrashGuard::timeoutForStateSec(int currentState) const {
  switch (currentState) {
  case SLEEPING_STATE:
    return 60;  // Increased from 30s to allow for cloud operations sync (30s) + sleep prep
  case CONNECTING_STATE:
    return 120;
  case REPORTING_STATE:
    return 120;
  case FIRMWARE_UPDATE_STATE:
    return 180;
  case INITIALIZATION_STATE:
  case ERROR_STATE:
    return 60;
  case IDLE_STATE:
  default:
    return 0;
  }
}

void ThrashGuard::logTrip(int currentState, uint32_t noprogSec, const char *tag, int tier, const char *action) {
  if (lastLogMs_ != 0 && (millis() - lastLogMs_) < logCooldownMs_) {
    return;
  }
  lastLogMs_ = millis();

  Log.warn("THRASH st=%s noprog=%lus last=%s tier=%d action=%s",
           stateNames[currentState],
           (unsigned long)noprogSec,
           tag ? tag : "?",
           tier,
           action ? action : "-");
}

void ThrashGuard::loop(int currentState, uint32_t nowMs) {
  uint32_t timeoutSec = timeoutForStateSec(currentState);
  if (timeoutSec == 0) {
    return;
  }

  if (lastProgressMs_ == 0) {
    lastProgressMs_ = nowMs;
    return;
  }

  if (nowMs < backoffUntilMs_) {
    return;
  }

  uint32_t elapsedMs = nowMs - lastProgressMs_;
  if (elapsedMs < (timeoutSec * 1000UL)) {
    return;
  }

  thrashTripCount++;
  lastThrashReason = REASON_NOPROGRESS;

  if (windowStartMs_ == 0 || (nowMs - windowStartMs_) > TIER3_WINDOW_MS) {
    windowStartMs_ = nowMs;
    recentTripCount_ = 0;
  }
  recentTripCount_++;

  int tier = 1;
  if (lastTripMs_ != 0 && (nowMs - lastTripMs_) <= TIER2_WINDOW_MS) {
    tier = 2;
  }
  if ((nowMs - windowStartMs_) <= TIER3_WINDOW_MS && recentTripCount_ >= TIER3_TRIP_COUNT) {
    tier = 3;
  }

  lastTripMs_ = nowMs;
  backoffUntilMs_ = nowMs + BACKOFF_MS;

  const uint32_t noprogSec = elapsedMs / 1000UL;

  if (tier == 1) {
    logTrip(currentState, noprogSec, lastTag_, 1, "backoff");
    return;
  }

  if (tier == 2) {
    logTrip(currentState, noprogSec, lastTag_, 2, "disconnect+sleep");
    current.raiseAlert(17);  // Alert 17: Thrash detected (tier 2)
    Connectivity::requestFullDisconnectAndRadioOff();
    if (state != SLEEPING_STATE) {
      state = SLEEPING_STATE;
    }
    return;
  }

  logTrip(currentState, noprogSec, lastTag_, 3, "reset");
  current.raiseAlert(17);  // Alert 17: Severe thrash detected (tier 3 - reset)
  thrashResetCount++;
  System.reset();
}
