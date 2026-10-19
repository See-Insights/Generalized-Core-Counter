#include "time/ClockDiagnostics.h"
#include "state/State_Common.h"
#include "LocalTimeRK.h"
namespace {

// Not Boron-specific: the AB1805 RTC/watchdog chip is used on every supported
// platform (ab1805.setup()/setWDT() run unconditionally below), so this name
// lookup must be available generally, not just under the Boron-only hibernate
// wake-reason path. Named "...Local" because this definition sits inside
// this file's anonymous namespace (internal linkage) - HibernateCycle.cpp
// needs it too (WO-2026-09-16 Step 2), so a thin external-linkage wrapper
// is defined below the namespace, the same pattern already used here for
// failsafeDeferReasonName()/failsafeDeferReasonNameLocal().
const char *ab1805WakeReasonNameLocal(AB1805::WakeReason reason) {
  switch (reason) {
  case AB1805::WakeReason::WATCHDOG:
    return "WATCHDOG";
  case AB1805::WakeReason::DEEP_POWER_DOWN:
    return "DEEP_POWER_DOWN";
  case AB1805::WakeReason::COUNTDOWN_TIMER:
    return "COUNTDOWN_TIMER";
  case AB1805::WakeReason::ALARM:
    return "ALARM";
  case AB1805::WakeReason::UNKNOWN:
  default:
    return "UNKNOWN";
  }
}
} // namespace

const char *ab1805WakeReasonName(AB1805::WakeReason reason) {
  return ab1805WakeReasonNameLocal(reason);
}

void logTimeDiag(bool isOpen) {
  const char *tz = SystemConfig::get_timeZoneStrCStr();
  if (!tz) {
    tz = "";
  }

  const bool timeValid = Time.isValid();
  const time_t epoch = Time.now();
  struct tm utcTm = {};
  if (timeValid) {
    gmtime_r(&epoch, &utcTm);
  }

  LocalTimeConvert localConverter;
  localConverter.withConfig(LocalTime::instance().getConfig()).withCurrentTime().convert();
  const LocalTimeYMD localDate = localConverter.getLocalTimeYMD();
  const uint32_t localSecondsOfDay = (uint32_t)localConverter.getLocalTimeHMS().toSeconds();
  const uint8_t localHour = (uint8_t)localConverter.getLocalTimeHMS().hour;
  const uint8_t localMinute = (uint8_t)((localSecondsOfDay / 60UL) % 60UL);
  const uint8_t localSecond = (uint8_t)(localSecondsOfDay % 60UL);

  // Item 9 (WO-2026-08-29-002): sync recency and the trust verdict, using
  // the same monotonic Particle.timeSyncedLast() signal as the resync gate
  // and trust check above - not Time.isValid() alone (Finding 3).
  // lastTimeSyncEpoch is the persisted, wall-clock completion stamp from
  // checkClockResync() (item 6's fix to SystemConfig::get_lastTimeSync(), which
  // previously had zero callers).
  //
  // Round 6 (Stage 7 finding 1): syncAgeMs is now reportedSyncAgeMs(), the
  // wrap-aware, sentinel-bearing telemetry value - not raw elapsedMs().
  // Raw elapsedMs() would (a) misreport uptime as sync age before any sync
  // has completed this boot (ambiguous with "synced 0 ms ago"), and (b)
  // wrap back around to a small, deceptively fresh-looking value after a
  // full millis() rollover with no further confirmed sync, even though
  // `trusted` has correctly already gone false - telemetry would then
  // contradict the trust verdict. reportedSyncAgeMs() reports
  // ClockTrust::kReportedSyncAgeUnavailableMs (UINT32_MAX) for both cases
  // instead - a value no genuine sync age can ever reach, so a log reader
  // cannot mistake it for a real duration.
  const bool clockTrusted = isClockTrusted();
  const time_t lastTimeSyncEpoch = SystemConfig::get_lastTimeSync();

  // WO-2026-09-19 Step 3b: openness=0/1/2 (Open/Closed/Unknown) surfaces
  // Clock::openness()'s verdict directly, alongside the pre-existing isOpen=
  // (the caller's own boolean, usually from isWithinOpenHours() - fail-open,
  // and NOT necessarily the same source as openness()). The two are expected
  // to diverge exactly when the clock is untrusted: isOpen=1/openness=2 is
  // the signature of "fail-open would have said open, openness() correctly
  // says Unknown" - the acceptance evidence Step 3b's bench run looks for.
  const int opennessCode = static_cast<int>(Clock::openness());

  Log.info("TimeDiag: tz=%s valid=%d epoch=%lu utc=%04d-%02d-%02d %02d:%02d:%02d local=%04d-%02d-%02d %02d:%02d:%02d open=%d close=%d isOpen=%d trusted=%d openness=%d syncAgeMs=%lu lastSyncEpoch=%ld",
           tz,
           timeValid ? 1 : 0,
           (unsigned long)epoch,
           utcTm.tm_year + 1900,
           utcTm.tm_mon + 1,
           utcTm.tm_mday,
           utcTm.tm_hour,
           utcTm.tm_min,
           utcTm.tm_sec,
           localDate.getYear(),
           localDate.getMonth(),
           localDate.getDay(),
           localHour,
           localMinute,
           localSecond,
           (int)SystemConfig::get_openTime(),
           (int)SystemConfig::get_closeTime(),
           isOpen ? 1 : 0,
           clockTrusted ? 1 : 0,
           opennessCode,
           (unsigned long)reportedSyncAgeMs(),
           (long)lastTimeSyncEpoch);
}
