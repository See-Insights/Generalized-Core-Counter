#pragma once

// Minimal host-side stand-in for time/Clock.h, scoped to exactly what
// src/reporting/RuntimeReportingPolicy.cpp calls (WO-2026-09-22:
// RuntimeReportingPolicy routes through Clock::isTrusted() instead of raw
// Time.isValid()). The real definition lives in time/Clock.cpp, which pulls
// in Particle/AB1805/LocalTimeRK/MyPersistentData - unrelated to what this
// adapter test targets - so it is faked here, test-controllable via the
// same "public settable global" pattern this override directory already
// uses for Config::testReportingIntervalSec/testWindowOpen/etc.

namespace Clock {

inline bool testClockTrusted = true;

inline bool isTrusted() {
  return testClockTrusted;
}

} // namespace Clock
