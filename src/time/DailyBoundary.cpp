#include "time/DailyBoundary.h"

#include "LocalTimeRK.h"
#include "persist/SystemConfig.h"
#include "time/Clock.h"

namespace DailyBoundary {

time_t todayAt(uint8_t hour) {
  LocalTimeConvert converter;
  converter.withConfig(LocalTime::instance().getConfig()).withCurrentTime().convert();

  if (hour == 24) {
    converter.nextDay(LocalTimeHMS("00:00:00"));
  } else {
    LocalTimeHMS hms;
    hms.hour = (int8_t)hour;
    hms.minute = 0;
    hms.second = 0;
    converter.atLocalTime(hms);
  }

  return converter.time;
}

Result check(time_t now) {
  bool due = false;
  time_t boundary = 0;
  uint8_t close = 0;

  // WO-2026-09-19 Step 3b: Clock::isTrusted(), not isTimeValid() - the day-
  // boundary logic must not run or stamp state on an untrusted clock, which
  // could permanently consume a missed boundary.
  if (Clock::isTrusted()) {
    close = SystemConfig::get_closeTime();
    boundary = todayAt(close);
    if (now < boundary) {
      boundary -= 86400;
    }

    const time_t lastDailyCleanup = SystemConfig::get_lastDailyCleanup();
    due = (lastDailyCleanup < boundary || lastDailyCleanup > now);
  }

  return {due, boundary, close};
}

} // namespace DailyBoundary
