#pragma once
#include "Particle.h"

namespace Observability {
struct WakeCycleStats {
  enum class ConnectResult { NONE, SUCCESS, FAILED };
  ConnectResult connect_result = ConnectResult::NONE;
  void resetOnWake(uint32_t nowMs) { sim::emit("CYCLE resetOnWake ms=%u", (unsigned)nowMs); }
  void markTeardownStart(uint32_t startMs, bool useNetworkStandby) {
    sim::emit("CYCLE teardownStart ms=%u standby=%d", (unsigned)startMs, useNetworkStandby ? 1 : 0);
  }
  void markServiceEnd(uint32_t endMs) { sim::emit("CYCLE markServiceEnd ms=%u", (unsigned)endMs); }
  void markTeardownEnd(uint32_t endMs) { sim::emit("CYCLE teardownEnd ms=%u", (unsigned)endMs); }
};
inline WakeCycleStats &cycleStats() {
  static WakeCycleStats s;
  return s;
}
}  // namespace Observability
