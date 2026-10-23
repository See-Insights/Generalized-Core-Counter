#pragma once
#include "Particle.h"

namespace Observability {
struct WakeCycleStats {
  void markTeardownStart(uint32_t startMs, bool useNetworkStandby) {
    sim::emit("CYCLE teardownStart ms=%u standby=%d", (unsigned)startMs, useNetworkStandby ? 1 : 0);
  }
  void markTeardownEnd(uint32_t endMs) { sim::emit("CYCLE teardownEnd ms=%u", (unsigned)endMs); }
};
inline WakeCycleStats &cycleStats() {
  static WakeCycleStats s;
  return s;
}
}  // namespace Observability
