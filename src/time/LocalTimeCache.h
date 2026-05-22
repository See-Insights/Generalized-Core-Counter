#pragma once

#include "Particle.h"
#include "LocalTimeRK.h"

// LocalTime conversion is relatively expensive and may copy timezone config.
// This utility caches the derived local time fields for the current time
// at a 60-second granularity to reduce repeated conversions.

namespace LocalTimeCache {

struct LocalTimeSnapshot {
  bool timeValid = false;
  time_t utcNow = 0;
  uint32_t utcMinute = 0;
  uint32_t configSig = 0;
  uint32_t localSecondsOfDay = 0;
  LocalTimeYMD localYmd;
  uint8_t localHour = 0;
  bool isDst = false;
};

// Returns a cached snapshot of the current local time.
// Cache refreshes when:
// - UTC minute changes
// - Time.isValid() changes
// - timezone configuration signature changes
const LocalTimeSnapshot &getLocalTimeSnapshot();

} // namespace LocalTimeCache
