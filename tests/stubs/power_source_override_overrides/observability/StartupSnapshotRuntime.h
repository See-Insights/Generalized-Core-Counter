#pragma once

#include <cstdint>
#include <ctime>

// Minimal host-side stand-in for observability/StartupSnapshotRuntime.h. Only
// the fields src/cloud/DeviceStatusPublisher.cpp reads from
// currentStartupSnapshot() are reproduced, with fixed test values.
namespace Observability {

struct StartupSnapshot {
  time_t epoch = 0;
  const char *reason = "unknown";
  const char *firmware = "test-fw";
  const char *deviceOS = "test-os";
  uint32_t resetCount = 0;
};

inline const StartupSnapshot &currentStartupSnapshot() {
  static StartupSnapshot snapshot;
  return snapshot;
}

} // namespace Observability
