#pragma once

// Minimal host-side stand-in for Particle.h, scoped to exactly what
// src/reporting/RuntimeReportingPolicy.cpp (the production adapter under
// test - WO-2026-08-25-001 Amendment C, AC-C6) needs: a `time_t`-compatible
// clock validity flag and a no-op logger. Real Device OS APIs are far larger;
// this intentionally stubs only the surface actually exercised.

#include <cmath>
#include <cstdint>
#include <ctime>

constexpr int SYSTEM_ERROR_NONE = 0;

struct TestLog {
  template <typename... Args>
  void info(const char *, Args...) {}

  template <typename... Args>
  void warn(const char *, Args...) {}
};

inline TestLog Log;

struct TestTime {
  bool valid = true;
  bool isValid() const { return valid; }
};

inline TestTime Time;
