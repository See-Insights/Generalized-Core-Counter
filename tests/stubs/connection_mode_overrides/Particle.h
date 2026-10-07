#pragma once

// Host stand-in for Particle.h for tests/connection_mode_downgrade_test.sh
// (WO-2026-10-07-002). Same surface as tests/stubs/persistence_backend/Particle.h,
// except Log records every formatted line so the test can assert that a
// downgrade logs once and a re-apply logs nothing.

#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

using String = std::string;

enum PublishFlags { PRIVATE = 0, PUBLIC = 1 };

struct RecordingLog {
  std::vector<std::string> lines;

  void record(const char *fmt, va_list args) {
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, args);
    lines.push_back(buf);
  }
  void info(const char *fmt, ...) __attribute__((format(printf, 2, 3))) {
    va_list args;
    va_start(args, fmt);
    record(fmt, args);
    va_end(args);
  }
  void warn(const char *, ...) {}
  void error(const char *, ...) {}
  void trace(const char *, ...) {}

  int count(const char *needle) const {
    int n = 0;
    for (const std::string &l : lines) {
      if (l.find(needle) != std::string::npos) ++n;
    }
    return n;
  }
};
inline RecordingLog Log;

struct TestTime {
  time_t nowValue = 1750000000;
  time_t now() const { return nowValue; }
  bool isValid() const { return true; }
};
inline TestTime Time;

struct TestParticle {
  bool connectedValue = false;
  bool connected() const { return connectedValue; }
};
inline TestParticle Particle;

inline uint32_t millisValue = 0;
inline uint32_t millis() { return millisValue; }
