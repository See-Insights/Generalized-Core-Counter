#pragma once
// Host fake of the Device OS surface src/time/ExternalRtc.h uses.
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <sys/time.h>

enum hal_rtc_source_t { HAL_RTC_SOURCE_INTERNAL = 0, HAL_RTC_SOURCE_EXTERNAL = 1 };
struct hal_rtc_option_t {
  uint16_t size;
  int source;
};

struct FakeHal {
  int getResult = 0, setResult = 0;
  time_t chipTime = 0;
  int lastSource = -1;
  int lastUsec = -1;
  int sets = 0;
  bool valid = false;
};
inline FakeHal &fakeHal() { static FakeHal h; return h; }

inline int hal_rtc_get_time(struct timeval *tv, const hal_rtc_option_t *o) {
  fakeHal().lastSource = o ? o->source : -1;
  if (fakeHal().getResult) return fakeHal().getResult;
  tv->tv_sec = fakeHal().chipTime;
  tv->tv_usec = 0;
  return 0;
}
inline int hal_rtc_set_time(const struct timeval *tv, const hal_rtc_option_t *o) {
  fakeHal().lastSource = o ? o->source : -1;
  fakeHal().lastUsec = (int)tv->tv_usec;
  fakeHal().sets++;
  if (fakeHal().setResult) return fakeHal().setResult;
  fakeHal().chipTime = tv->tv_sec;
  return 0;
}
inline bool hal_rtc_time_is_valid(const hal_rtc_option_t *o) {
  fakeHal().lastSource = o ? o->source : -1;
  return fakeHal().valid;
}

struct FakeTime {
  bool valid = true;
  time_t now_ = 1800000000;
  bool isValid() { return valid; }
  time_t now() { return now_; }
};
inline FakeTime &fakeTime() { static FakeTime t; return t; }
#define Time fakeTime()

struct FakeLog {
  int infos = 0, warns = 0;
  template <typename... A> void info(const char *, A...) { infos++; }
  template <typename... A> void warn(const char *, A...) { warns++; }
};
inline FakeLog &fakeLog() { static FakeLog l; return l; }
#define Log fakeLog()

namespace particle {
struct HalDevice { struct { int interface; } i2c; };
struct Caps { uint32_t v = 0; uint32_t value() const { return v; } };
struct Am18x5Configuration {
  bool valid_ = false;
  uint8_t addr = 0x69;
  int intPin = -1, wdtPin = -1, wire = 0, tsrc = 0;
  uint32_t caps = 0;
  HalDevice dev{};
  Am18x5Configuration &i2c(int w) { wire = w; dev.i2c.interface = w; valid_ = true; return *this; }
  Am18x5Configuration &interruptPin(int p) { intPin = p; return *this; }
  Am18x5Configuration &watchdogPin(int p) { wdtPin = p; return *this; }
  Am18x5Configuration &capabilities(uint32_t c) { caps = c; return *this; }
  bool valid() const { return valid_; }
  uint8_t address() const { return addr; }
  int interruptPin() const { return intPin; }
  int watchdogPin() const { return wdtPin; }
  Caps capabilities() const { Caps c; c.v = caps; return c; }
  int defaultTimeSource() const { return tsrc; }
  const HalDevice *halDevice() const { return &dev; }
};
struct Status { bool b; bool bound() const { return b; } };
}  // namespace particle
