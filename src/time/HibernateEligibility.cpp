#include "time/HibernateEligibility.h"
#include "state/StateMachine.h"
#include "time/Clock.h"
#include "time/ExternalRtc.h"
#include "persist/SystemConfig.h"
#if PLATFORM_ID == PLATFORM_BORON
bool isDeviceOsAtLeast6400() {
  int major = 0;
  int minor = 0;
  int patch = 0;
  const String version = System.version();
  if (sscanf(version.c_str(), "%d.%d.%d", &major, &minor, &patch) < 2) {
    return false;
  }

  if (major > 6) {
    return true;
  }
  if (major < 6) {
    return false;
  }
  return minor >= 4;
}

bool shouldUseBoronRtcAlarmHibernate(uint32_t requestedSleepSec, time_t &rtcNow) {
  Log.info("HibernateDiag: check enabled=%d requested=%lu",
           SystemConfig::get_enableHibernateSleep() ? 1 : 0,
           (unsigned long)requestedSleepSec);
  if (!SystemConfig::get_enableHibernateSleep()) {
    Log.info("HibernateDiag: fail=disabled");
    return false;
  }

  if (!isDeviceOsAtLeast6400()) {
    Log.info("HibernateDiag: fail=device_os");
    return false;
  }

  static const uint32_t kMinHibernateSleepSec = 900;
  static const uint32_t kMaxHibernateSleepSec = 36000;
  if (requestedSleepSec < kMinHibernateSleepSec || requestedSleepSec > kMaxHibernateSleepSec) {
    Log.info("HibernateDiag: fail=duration requested=%lu", (unsigned long)requestedSleepSec);
    return false;
  }

  if (!ExternalRtc::isSet()) {
    Log.info("HibernateDiag: fail=rtc_not_set");
    return false;
  }

  if (!ExternalRtc::get(rtcNow)) {
    Log.info("HibernateDiag: fail=rtc_read");
    return false;
  }

  if (!Clock::isPlausibleEpoch(rtcNow)) {
    Log.info("HibernateDiag: fail=rtc_invalid epoch=%ld", (long)rtcNow);
    return false;
  }

  Log.info("HibernateDiag: pass epoch=%ld", (long)rtcNow);
  return true;
}
#endif
