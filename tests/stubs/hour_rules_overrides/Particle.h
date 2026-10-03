#pragma once

// WO-2026-09-24-004: minimal Device OS stub so the REAL src/Config.h (and the
// headers it pulls in) compile on the host. Only what those headers need.

#include <cstdint>
#include <ctime>

struct TestLog {
  template <typename... Args>
  void info(const char *, Args...) {}

  template <typename... Args>
  void warn(const char *, Args...) {}
};

inline TestLog Log;
