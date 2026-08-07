#pragma once

#include <cmath>
#include <cstdint>

constexpr int SYSTEM_ERROR_NONE = 0;

struct TestLog {
  template <typename... Args>
  void info(const char *, Args...) {}

  template <typename... Args>
  void warn(const char *, Args...) {}
};

inline TestLog Log;