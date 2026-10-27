#pragma once
#include <ctime>
#include "Particle.h"

namespace Observability {
struct WakeCycleStats {
  enum class ConnectResult { NONE, SUCCESS, FAILED, TIMEOUT, ABORTED };
  enum class ConnectAttemptType : uint8_t { NONE = 0, NORMAL = 1, DEEP = 2 };
  ConnectResult connect_result = ConnectResult::NONE;
  // The fields endOfCycleSummary() reads (WO-2026-10-21-004); finalizeBeforeSleep() sets what the real one sets.
  ConnectAttemptType connect_attempt_type = ConnectAttemptType::NONE;
  uint32_t wake_start_ms = 0, connect_budget_ms = 0, connect_duration_ms = 0, service_duration_ms = 0,
           teardown_duration_ms = 0, total_awake_ms = 0;
  uint16_t publish_queue_depth_before_connect = 0xFFFF, publish_queue_depth_after_connect = 0xFFFF,
           publish_queue_depth_before_sleep = 0xFFFF, battery_soc_tenths = 0xFFFF;
  uint8_t is_charging = 0xFF;
  time_t last_success_epoch = 0;
  void finalizeBeforeSleep(uint32_t nowMs, bool cloudConnected, bool radioOn, uint16_t queueDepth, uint16_t socTenths,
                           uint8_t charging, time_t lastSuccessEpoch) {
    sim::emit("FINALIZE now=%u cloud=%d radio=%d q=%u soc=%u chg=%u last=%ld", (unsigned)nowMs, (int)cloudConnected,
              (int)radioOn, (unsigned)queueDepth, (unsigned)socTenths, (unsigned)charging, (long)lastSuccessEpoch);
    publish_queue_depth_before_sleep = queueDepth;
    battery_soc_tenths = socTenths;
    is_charging = charging;
    if (lastSuccessEpoch != 0) last_success_epoch = lastSuccessEpoch;
    if (wake_start_ms != 0) total_awake_ms = (nowMs >= wake_start_ms) ? (nowMs - wake_start_ms) : 0;
  }
  void resetOnWake(uint32_t nowMs) { sim::emit("CYCLE resetOnWake ms=%u", (unsigned)nowMs); }
  void markTeardownStart(uint32_t startMs, bool useNetworkStandby) {
    sim::emit("CYCLE teardownStart ms=%u standby=%d", (unsigned)startMs, useNetworkStandby ? 1 : 0);
  }
  void markServiceEnd(uint32_t endMs) { sim::emit("CYCLE markServiceEnd ms=%u", (unsigned)endMs); }
  void markTeardownEnd(uint32_t endMs) { sim::emit("CYCLE teardownEnd ms=%u", (unsigned)endMs); }
};
inline const char *toString(WakeCycleStats::ConnectAttemptType v) {
  return v == WakeCycleStats::ConnectAttemptType::NONE ? "none" : v == WakeCycleStats::ConnectAttemptType::NORMAL ? "normal"
       : v == WakeCycleStats::ConnectAttemptType::DEEP ? "deep" : "?";
}
inline const char *toString(WakeCycleStats::ConnectResult v) {
  switch (v) {
    case WakeCycleStats::ConnectResult::SUCCESS: return "ok";
    case WakeCycleStats::ConnectResult::TIMEOUT: return "timeout";
    case WakeCycleStats::ConnectResult::ABORTED: return "aborted";
    case WakeCycleStats::ConnectResult::NONE: return "na";
    default: return "?";
  }
}
inline WakeCycleStats &cycleStats() {
  static WakeCycleStats s;
  return s;
}
}  // namespace Observability
