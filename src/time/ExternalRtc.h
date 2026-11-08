#pragma once
// v50 S1: Device OS owns the carrier AB1805. Every app read/write of the chip goes through hal_rtc_* (external source).
#include "Particle.h"

namespace ExternalRtc {
constexpr time_t kWriteVerifyToleranceSec = 2;  // read-back must land within this of the written time
inline bool get(time_t &t) {
  struct timeval tv = {};
  hal_rtc_option_t opt = {};
  opt.size = sizeof(opt);
  opt.source = HAL_RTC_SOURCE_EXTERNAL;
  if (hal_rtc_get_time(&tv, &opt) != 0) return false;
  t = tv.tv_sec;
  return true;
}
inline bool set(time_t t) {
  struct timeval tv = {};
  tv.tv_sec = t;
  hal_rtc_option_t opt = {};
  opt.size = sizeof(opt);
  opt.source = HAL_RTC_SOURCE_EXTERNAL;
  if (hal_rtc_set_time(&tv, &opt) != 0) return false;
  time_t back = 0;
  return get(back) && (back > t ? back - t : t - back) <= kWriteVerifyToleranceSec;  // a silent no-op write fails
}
inline bool setFromSystem() { return Time.isValid() && set(Time.now()); }
inline bool isSet() {
  hal_rtc_option_t opt = {};
  opt.size = sizeof(opt);
  opt.source = HAL_RTC_SOURCE_EXTERNAL;
  return hal_rtc_time_is_valid(&opt);
}
// v50 S4: process-lifetime latch (RAM only, never cleared, never persisted); a reboot re-derives it from the clock source.
inline bool &oscillatorFaultSeenSinceBind() { static bool seen = false; return seen; }
inline void onOscillatorEvent(particle::RtcEvents e, void *) {
  if (e.isSet(particle::RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE)) oscillatorFaultSeenSinceBind() = true;
}
// The fields the fleet configuration fixes (including the clock source: Device OS reports the actual one).
template <typename C> bool sameFleetConfig(C &a, C &b) {
  return a.valid() && a.address() == b.address() && a.interruptPin() == b.interruptPin() &&
         a.watchdogPin() == b.watchdogPin() && a.capabilities().value() == b.capabilities().value() &&
         a.defaultTimeSource() == b.defaultTimeSource() && a.clockSource() == b.clockSource() && a.halDevice()->i2c.interface == b.halDevice()->i2c.interface;
}
// Bind once with the fleet configuration; a failure is logged and the boot continues on the internal clock.
template <typename R, typename C> int ensureBound(R &rtc, const C &fleet) {
  int r = 0;
  if (!rtc.status().bound()) {
    r = rtc.enable(fleet);
    Log.info("AB1805: enable r=%d", r);
  } else {
    C stored = rtc.getConfig();
    C want = fleet;
    if (!sameFleetConfig(stored, want)) {
      r = rtc.enable(fleet);
      Log.warn("AB1805: stored config differs, re-bound r=%d", r);
    }
  }
  return r;
}
}  // namespace ExternalRtc
