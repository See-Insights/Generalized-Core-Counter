#pragma once

#include "Particle.h"

class ThrashGuard {
public:
  void markProgress(const char *tag);
  void recordStateTransition(int oldState, int newState);
  void loop(int currentState, uint32_t nowMs);
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
