#pragma once

#include <cstdint>

// Minimal host-side stand-in for observability/WakeCycleStats.h. Only the
// fields/functions src/cloud/DeviceStatusPublisher.cpp actually reads
// (cycleStats().connect_result / connect_duration_ms, and toString() for the
// connect result) are reproduced.
namespace Observability {

struct WakeCycleStats {
  enum class ConnectResult : int8_t {
    NOT_ATTEMPTED = -1,
    SUCCESS = 0,
    TIMEOUT = 1,
    ABORTED = 2,
  };

  ConnectResult connect_result = ConnectResult::NOT_ATTEMPTED;
  uint32_t connect_duration_ms = 0;
};

inline WakeCycleStats &cycleStats() {
  static WakeCycleStats stats;
  return stats;
}

inline const char *toString(WakeCycleStats::ConnectResult v) {
  switch (v) {
    case WakeCycleStats::ConnectResult::NOT_ATTEMPTED:
      return "na";
    case WakeCycleStats::ConnectResult::SUCCESS:
      return "ok";
    case WakeCycleStats::ConnectResult::TIMEOUT:
      return "timeout";
    case WakeCycleStats::ConnectResult::ABORTED:
      return "aborted";
    default:
      return "?";
  }
}

} // namespace Observability
