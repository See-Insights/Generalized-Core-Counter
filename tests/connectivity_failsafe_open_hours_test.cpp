// WO-2026-10-02-001 item A: the connectivity failsafe's open-hours recovery.
//
// Part 1 of tests/connectivity_failsafe_open_hours_test.sh. This is a
// hand-written mirror of connectivityFailsafeSupervisor()'s gating, but every
// value that the work order fixes is injected from the REAL source by the
// shell driver:
//
//   REAL_STALE_SEC / REAL_COOLDOWN_SEC / REAL_JITTER_MAX_SEC
//       parsed from src/power/ConnectivityPolicy.h (production branch)
//   REAL_OPEN_GATE      1 when the supervisor checks Clock::openness()
//   REAL_USES_OPEN_BASE 1 when the supervisor bases the age on
//                       DailyBoundary::todayAt(SystemConfig::get_openTime())
//   REAL_FIRST_STAGE    the stage taken with no stage recorded
//
// So restoring the 12 h threshold, or removing the open-hours base, changes
// the behaviour this file asserts - it does not merely change a comment.

#include <cstdint>
#include <cstdio>
#include <ctime>

#ifndef REAL_STALE_SEC
#error "REAL_STALE_SEC must be injected from src/power/ConnectivityPolicy.h"
#endif
#ifndef REAL_COOLDOWN_SEC
#error "REAL_COOLDOWN_SEC must be injected from src/power/ConnectivityPolicy.h"
#endif
#ifndef REAL_JITTER_MAX_SEC
#error "REAL_JITTER_MAX_SEC must be injected from src/power/ConnectivityPolicy.h"
#endif
#ifndef REAL_OPEN_GATE
#error "REAL_OPEN_GATE must be injected from the real supervisor body"
#endif
#ifndef REAL_USES_OPEN_BASE
#error "REAL_USES_OPEN_BASE must be injected from the real supervisor body"
#endif
#ifndef REAL_FIRST_STAGE
#error "REAL_FIRST_STAGE must be injected from the real supervisor body"
#endif

namespace {

int failures = 0;

// 0 = no action; otherwise the failsafe stage that would act now.
// Mirrors connectivityFailsafeSupervisor() from the "lastConnection" read
// down to the stage dispatch, with the jitter at its maximum (the real
// jitter is a per-stage random value in [0, JITTER_MAX]).
uint8_t failsafeAction(bool open,
                       time_t now,
                       time_t lastConnection,
                       time_t openedAt,
                       uint8_t currentStage,
                       time_t lastAction) {
  if (lastConnection == 0) {
    return 0;
  }

  time_t ageBase = lastConnection;
#if REAL_USES_OPEN_BASE
  if (openedAt > ageBase) {
    ageBase = openedAt;
  }
#else
  (void)openedAt;
#endif

#if REAL_OPEN_GATE
  if (!open) {
    return 0;
  }
#else
  (void)open;
#endif

  if (now <= ageBase) {
    return 0;
  }

  const time_t connectionAgeSec = now - ageBase;
  if (connectionAgeSec < (time_t)REAL_STALE_SEC) {
    return 0;
  }

  if (currentStage > 3) {
    currentStage = 0;
  }
  if (currentStage >= 3) {
    return 0;
  }

  if (lastAction > now) {
    lastAction = 0;
  }

  const uint8_t nextStage =
      (currentStage <= 1) ? (uint8_t)REAL_FIRST_STAGE : (uint8_t)(currentStage + 1);

  time_t requiredDelay = (time_t)REAL_COOLDOWN_SEC;
  if (nextStage >= 2) {
    requiredDelay += (time_t)REAL_JITTER_MAX_SEC;
  }

  if (currentStage != 0 && lastAction != 0 && (now - lastAction) < requiredDelay) {
    return 0;
  }

  return nextStage;
}

void check(const char *name, uint8_t got, uint8_t want) {
  if (got == want) {
    printf("  ok   %-62s (stage %u)\n", name, (unsigned)got);
  } else {
    printf("  FAIL %-62s got stage %u, want %u\n", name, (unsigned)got, (unsigned)want);
    failures++;
  }
}

// A device opening at 06:00 local. Epochs are arbitrary but consistent.
constexpr time_t OPEN_TODAY = 1000000;            // today's 06:00
constexpr time_t CLOSE_YESTERDAY = OPEN_TODAY - 8 * 3600; // yesterday 22:00

} // namespace

int main() {
  printf("--- Part 1: connectivityFailsafeSupervisor() gating "
         "(stale=%ld cooldown=%ld jitter=%ld openGate=%d openBase=%d first=%d) ---\n",
         (long)REAL_STALE_SEC, (long)REAL_COOLDOWN_SEC, (long)REAL_JITTER_MAX_SEC,
         (int)REAL_OPEN_GATE, (int)REAL_USES_OPEN_BASE, (int)REAL_FIRST_STAGE);

  // The last successful connection was before yesterday's close: the device
  // has been silent across the whole night.
  const time_t silentSince = CLOSE_YESTERDAY - 3600;

  // 1. The 06:00 wake after a normal 22:00 close must NOT reset: 8 h of wall
  //    clock have passed, but zero open hours.
  check("06:00 wake after a 22:00 close does not reset",
        failsafeAction(true, OPEN_TODAY + 5, silentSince, OPEN_TODAY, 0, 0), 0);

  // 2. Not before 3 open hours.
  check("2 h 59 m of open hours does not act",
        failsafeAction(true, OPEN_TODAY + 3 * 3600 - 60, silentSince, OPEN_TODAY, 0, 0), 0);

  // 3. At 3 open hours the full reset (stage 2) fires.
  check("3 h of open hours fires the full reset",
        failsafeAction(true, OPEN_TODAY + 3 * 3600, silentSince, OPEN_TODAY, 0, 0), 2);
  check("a stage 1 persisted by older firmware still progresses to 2",
        failsafeAction(true, OPEN_TODAY + 3 * 3600, silentSince, OPEN_TODAY, 1, 0), 2);

  // 4. Closed hours never act, however old the connection is.
  check("closed hours never act (a day of silence)",
        failsafeAction(false, OPEN_TODAY + 20 * 3600, silentSince, OPEN_TODAY, 0, 0), 0);
  check("closed hours never act (overnight, stage 1 recorded)",
        failsafeAction(false, OPEN_TODAY - 3600, silentSince, OPEN_TODAY - 86400, 1, 0), 0);

  // 5. A successful connection clears it: lastConnection moves past the
  //    opening, so the age restarts from the connection.
  const time_t connectedAt = OPEN_TODAY + 2 * 3600;
  check("a successful connection clears the timer",
        failsafeAction(true, connectedAt + 60, connectedAt, OPEN_TODAY, 0, 0), 0);
  check("a successful connection restarts the full 3 h",
        failsafeAction(true, connectedAt + 3 * 3600 - 60, connectedAt, OPEN_TODAY, 0, 0), 0);
  check("3 h after a successful connection acts again",
        failsafeAction(true, connectedAt + 3 * 3600, connectedAt, OPEN_TODAY, 0, 0), 2);

  // 6. Stage 3 stays COOLDOWN + jitter after stage 2, and still needs 3 open
  //    hours of age - so a stage 2 late in the day pushes stage 3 into the
  //    next open period.
  const time_t stage2At = OPEN_TODAY + 3 * 3600;
  const time_t stage3Due = stage2At + REAL_COOLDOWN_SEC + REAL_JITTER_MAX_SEC;
  check("stage 3 waits the full cooldown + jitter after stage 2",
        failsafeAction(true, stage3Due - 1, silentSince, OPEN_TODAY, 2, stage2At), 0);
  check("stage 3 fires once cooldown + jitter has elapsed",
        failsafeAction(true, stage3Due, silentSince, OPEN_TODAY, 2, stage2At), 3);
  check("stage 3 does not fire during closed hours",
        failsafeAction(false, stage3Due, silentSince, OPEN_TODAY, 2, stage2At), 0);
  check("stage 3 is the last stage",
        failsafeAction(true, stage3Due + 86400, silentSince, OPEN_TODAY, 3, stage2At), 0);

  // 7. No last connection recorded: nothing to measure.
  check("no recorded connection defers",
        failsafeAction(true, OPEN_TODAY + 10 * 3600, 0, OPEN_TODAY, 0, 0), 0);

  if (failures != 0) {
    printf("\nFAILED: %d check(s)\n", failures);
    return 1;
  }
  printf("\nPart 1 passed\n");
  return 0;
}
