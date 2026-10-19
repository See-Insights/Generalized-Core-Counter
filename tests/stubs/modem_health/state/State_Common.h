#pragma once
// Host stub of the surface ModemHealth.cpp uses from State_Common.h: the four
// latch fields of `session` and a Log that records level and text.
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

struct SessionState {
  bool modemUnstable = false;
  bool modemStandbySuppressed = false;
  uint8_t modemUnstableReason = 0;
  uint8_t modemConnectTimeoutCount = 0;
};
static SessionState session;

static std::vector<std::string> logLines;
struct LogStub {
  void add(const char *level, const char *fmt, va_list ap) {
    char buf[256];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    logLines.push_back(std::string(level) + " " + buf);
  }
  void info(const char *fmt, ...) { va_list ap; va_start(ap, fmt); add("INFO", fmt, ap); va_end(ap); }
  void warn(const char *fmt, ...) { va_list ap; va_start(ap, fmt); add("WARN", fmt, ap); va_end(ap); }
};
static LogStub Log;
