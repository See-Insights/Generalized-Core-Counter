#pragma once

#include <cmath>
#include <cstdint>

constexpr int SYSTEM_ERROR_NONE = 0;

struct TestLog {
  template <typename... Args>
  void info(const char *, Args...) {}

  template <typename... Args>
  void warn(const char *, Args...) {}

  // WO-2026-09-25-001: DeviceStatusPublisher.cpp's payload overflow guard logs
  // at error level.
  template <typename... Args>
  void error(const char *, Args...) {}
};

inline TestLog Log;