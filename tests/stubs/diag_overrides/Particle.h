#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>

constexpr int SYSTEM_ERROR_NONE = 0;

using byte = uint8_t;

struct TestLog {
  // WO-2026-09-15-001 Amendment C: real call counter, additive to the
  // existing no-op behavior - power_diagnostics_batch_test.cpp never calls
  // logPowerState() (see its own header comment), so this counter is
  // inert for that test. Used by power_diagnostics_log_guard_test.cpp to
  // confirm logPowerState()'s change-detection guard actually suppresses/
  // emits, not just that a call was made.
  int infoCallCount = 0;

  template <typename... Args>
  void info(const char *, Args...) {
    infoCallCount++;
  }

  template <typename... Args>
  void warn(const char *, Args...) {}
};

inline TestLog Log;