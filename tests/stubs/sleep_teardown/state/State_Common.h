#pragma once
// Host stub of the State_Common.h surface the sleep teardown uses: the session
// timings, transitionTo, breadcrumbs, the awake watchdog and PowerManager's
// connection mode. The config, readings and alert headers are the real ones;
// the harness defines their functions.
#include <cstdint>
#include "Particle.h"
#include "power/ConnectivityPolicy.h"
#include "persist/SystemConfig.h"
#include "persist/CurrentReadings.h"
#include "persist/RecoveryState.h"

enum State { SLEEPING_STATE, ERROR_STATE };

namespace sim {
inline int transitions = 0;
inline uint8_t connectionMode = 0;
}  // namespace sim

inline void transitionTo(State newState, const char *reason) {
  sim::transitions++;
  sim::emit("TRANSITION %d %s", (int)newState, reason);
}
inline void setAppBreadcrumb(uint8_t code) { sim::emit("CRUMB %u", (unsigned)code); }
inline void serviceAwakeWatchdog() { sim::reads.watchdog++; }

struct SessionState {
  unsigned long lastTeardownEndMs = 0;
  uint32_t lastCloudDisconnectElapsedMs = 0;
  uint32_t lastModemOffElapsedMs = 0;
  uint32_t lastTotalTeardownElapsedMs = 0;
  bool modemUnstable = false;
};
inline SessionState session;

class PowerManager {
 public:
  static PowerManager &instance() {
    static PowerManager p;
    return p;
  }
  uint8_t effectiveConnectionMode() const { return sim::connectionMode; }
  float soc() const { return 87.5f; }
};
