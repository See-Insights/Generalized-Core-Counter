#include "LocalTimeCache.h"

namespace {

static uint32_t fnv1aMix(uint32_t hash, uint32_t value) {
  hash ^= value;
  hash *= 16777619u;
  return hash;
}

static uint32_t configSignature(const LocalTimePosixTimezone &config) {
  uint32_t hash = 2166136261u;

  hash = fnv1aMix(hash, (uint32_t)config.valid);
  hash = fnv1aMix(hash, (uint32_t)config.standardHMS.toSeconds());
  hash = fnv1aMix(hash, (uint32_t)config.dstHMS.toSeconds());

  const LocalTimeChange &dstStart = config.dstStart;
  hash = fnv1aMix(hash, (uint32_t)(uint8_t)dstStart.month);
  hash = fnv1aMix(hash, (uint32_t)(uint8_t)dstStart.week);
  hash = fnv1aMix(hash, (uint32_t)(uint8_t)dstStart.dayOfWeek);
  hash = fnv1aMix(hash, (uint32_t)(uint8_t)dstStart.valid);
  hash = fnv1aMix(hash, (uint32_t)dstStart.hms.toSeconds());

  const LocalTimeChange &standardStart = config.standardStart;
  hash = fnv1aMix(hash, (uint32_t)(uint8_t)standardStart.month);
  hash = fnv1aMix(hash, (uint32_t)(uint8_t)standardStart.week);
  hash = fnv1aMix(hash, (uint32_t)(uint8_t)standardStart.dayOfWeek);
  hash = fnv1aMix(hash, (uint32_t)(uint8_t)standardStart.valid);
  hash = fnv1aMix(hash, (uint32_t)standardStart.hms.toSeconds());

  return hash;
}

} // namespace

namespace LocalTimeCache {

const LocalTimeSnapshot &getLocalTimeSnapshot() {
  static LocalTimeSnapshot cache;
  static bool initialized = false;
  static LocalTimeConvert converter;

  const bool timeValidNow = Time.isValid();
  const time_t utcNow = Time.now();
  const uint32_t utcMinute = (uint32_t)(utcNow / 60);

  const LocalTimePosixTimezone &config = LocalTime::instance().getConfig();
  const uint32_t sig = configSignature(config);

  const bool needRefresh = !initialized ||
                           cache.utcMinute != utcMinute ||
                           cache.timeValid != timeValidNow ||
                           cache.configSig != sig;

  if (needRefresh) {
    // Only update the converter's config when it changes to avoid repeated
    // String copies/allocations in steady state.
    if (!initialized || cache.configSig != sig) {
      converter.withConfig(config);
    }

    converter.withTime(utcNow).convert();
    const uint32_t secondsOfDay = (uint32_t)converter.getLocalTimeHMS().toSeconds();

    cache.timeValid = timeValidNow;
    cache.utcNow = utcNow;
    cache.utcMinute = utcMinute;
    cache.configSig = sig;
    cache.localSecondsOfDay = secondsOfDay;
    cache.localYmd = converter.getLocalTimeYMD();
    cache.localHour = (uint8_t)(secondsOfDay / 3600u);
    cache.isDst = converter.isDST();

    initialized = true;
  }

  return cache;
}

} // namespace LocalTimeCache
