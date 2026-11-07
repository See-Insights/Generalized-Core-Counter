// v50 S2 (WO-2026-11-05-004): the reason-20 (PIN_RESET) boot is classified from persisted evidence only.
// This runs the real HibernateWakeDiagnostics::isInferredWatchdog() (header-only, host-compilable).
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include "time/HibernateWakeDiagnostics.h"

using HibernateWakeDiagnostics::isInferredWatchdog;
using HibernateWakeDiagnostics::kPowerOffEvidenceMaxAgeSec;

static const int64_t kNow = 1800000000;

int main() {
  // No record at all: nothing says the firmware powered the board off, so it is an inferred watchdog.
  assert(isInferredWatchdog(false, 0, 0, true, kNow));
  // The Error-path byte was persisted before deepPowerDown(): a power-up, not a watchdog.
  assert(!isInferredWatchdog(true, 0, 0, true, kNow));
  // Failsafe stage 3 acted recently: a power-up.
  assert(!isInferredWatchdog(false, 3, kNow - 5, true, kNow));
  assert(!isInferredWatchdog(false, 3, kNow - kPowerOffEvidenceMaxAgeSec, true, kNow));
  // Stage 3 but stale beyond the window: inferred watchdog.
  assert(isInferredWatchdog(false, 3, kNow - kPowerOffEvidenceMaxAgeSec - 1, true, kNow));
  // A future timestamp is not evidence.
  assert(isInferredWatchdog(false, 3, kNow + 1, true, kNow));
  // Other stages never count, however recent.
  for (uint8_t stage : {(uint8_t)0, (uint8_t)1, (uint8_t)2, (uint8_t)4}) {
    assert(isInferredWatchdog(false, stage, kNow - 1, true, kNow));
  }
  // Time invalid: recency cannot be judged, stage 3 is accepted as evidence.
  assert(!isInferredWatchdog(false, 3, 12345, false, 0));
  assert(isInferredWatchdog(false, 2, 12345, false, 0));
  // The byte wins over everything else.
  assert(!isInferredWatchdog(true, 3, 0, true, kNow));
  puts("watchdog_inferred_classification_test: ok");
  return 0;
}
