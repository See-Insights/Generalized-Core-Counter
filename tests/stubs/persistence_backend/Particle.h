#pragma once

// Host stand-in for Particle.h, scoped to what src/MyPersistentData.cpp
// actually uses (WO-2026-09-23-001, Step 5 backend/facade behavior test).

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

using String = std::string;

enum PublishFlags { PRIVATE = 0, PUBLIC = 1 };

struct TestLog {
  template <typename... Args> void info(const char *, Args...) {}
  template <typename... Args> void warn(const char *, Args...) {}
  template <typename... Args> void error(const char *, Args...) {}
  template <typename... Args> void trace(const char *, Args...) {}
};
inline TestLog Log;

struct TestTime {
  time_t nowValue = 1750000000;  // fixed, so timestamp assertions are exact
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
