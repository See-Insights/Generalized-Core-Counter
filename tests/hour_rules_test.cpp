// WO-2026-09-24-004 (v36-HourRules): the three open/close hour rules.
//
// Part 1 of tests/hour_rules_test.sh.
//
// Two things are exercised here, and both are PRODUCTION code:
//
//   * the REAL shared check, src/Config.h's Config::hoursRuleFailure() /
//     Config::hoursFollowRules() - included directly, not mirrored. The shell
//     driver re-runs this file against deliberately broken COPIES of Config.h
//     (production sources are never modified) and requires a failure for each
//     rule, so a weakened rule cannot pass unnoticed;
//
//   * the REAL window functions from src/time/Clock.cpp -
//     isWithinOpenHoursForHour() and secondsUntilNextOpenForSeconds() - which
//     are file-static, so the shell driver extracts them verbatim into
//     real_clock_fns.inc and compiles them into this translation unit. They
//     are compared against hand-written copies of the v35 versions (the ones
//     with the overnight and open == close branches) for EVERY pair the rules
//     allow: the removals must not change behaviour for a valid pair.
//
// The expected rules are written out here independently of src/, which is what
// makes a mutation in src/ observable rather than self-consistent.

#include "Config.h"

#include <cstdint>
#include <cstdio>

#include "real_clock_fns.inc"

namespace {

int failures = 0;

void check(bool ok, const char *what) {
  if (!ok) {
    ++failures;
    std::printf("FAIL: %s\n", what);
  }
}

// The rules, stated independently of src/Config.h:
//   1. always-open is exactly 0/24, so 24 is the highest close hour;
//   2. closeHour > openHour;
//   3. 0 <= openHour <= 12.
bool expectedFollowsRules(int openHour, int closeHour) {
  return openHour >= 0 && openHour <= 12 && closeHour > openHour && closeHour <= 24;
}

// --- v35 behaviour, copied verbatim from src/time/Clock.cpp at 6d8aaf9 ------

bool legacyIsWithinOpenHoursForHour(uint8_t hour, uint8_t openHour, uint8_t closeHour) {
  if (openHour < closeHour) {
    return (hour >= openHour) && (hour < closeHour);
  } else if (openHour > closeHour) {
    return (hour >= openHour) || (hour < closeHour);
  } else {
    return true;
  }
}

int legacySecondsUntilNextOpenForSeconds(uint32_t secondsOfDay,
                                         uint8_t openHour,
                                         uint8_t closeHour,
                                         bool openNow) {
  uint32_t openSec = (uint8_t)openHour * 3600;
  uint32_t closeSec = (uint8_t)closeHour * 3600;

  if (openNow) {
    return (int)((24 * 3600UL - secondsOfDay) + openSec);
  }

  if (openHour < closeHour) {
    if (secondsOfDay < openSec) {
      return (int)(openSec - secondsOfDay);
    } else {
      return (int)((24 * 3600UL - secondsOfDay) + openSec);
    }
  } else if (openHour > closeHour) {
    if (secondsOfDay < openSec && secondsOfDay >= closeSec) {
      return (int)(openSec - secondsOfDay);
    } else {
      return 3600;
    }
  } else {
    return 3600;
  }
}

// v35 DailyBoundary.cpp:37 - the sentinel normalization this WO removes.
uint8_t legacyDailyClose(uint8_t openHour, uint8_t closeHour) {
  return (openHour == closeHour) ? 24 : closeHour;
}

// v36 DailyBoundary.cpp - the close hour is used as configured.
uint8_t dailyClose(uint8_t /*openHour*/, uint8_t closeHour) { return closeHour; }

// --- 1. The rules table ----------------------------------------------------

void testRulesTable() {
  struct Pair {
    int open;
    int close;
  };

  const Pair accepted[] = {{6, 22}, {6, 23}, {0, 24}, {12, 13}};
  for (const Pair &p : accepted) {
    char what[96];
    std::snprintf(what, sizeof(what), "%d/%d must be accepted", p.open, p.close);
    check(Config::hoursFollowRules(p.open, p.close), what);
    std::snprintf(what, sizeof(what), "%d/%d must report no broken rule", p.open, p.close);
    check(Config::hoursRuleFailure(p.open, p.close) == nullptr, what);
  }

  // 6/6, 22/22 and 7/7 are the retired always-open sentinel; 20/6 is an
  // overnight window; 13/22 and 13/24 break rule 3; 0/25 breaks rule 1.
  const Pair rejected[] = {{6, 6}, {22, 22}, {20, 6}, {13, 22}, {0, 25}, {13, 24}, {7, 7}};
  for (const Pair &p : rejected) {
    char what[96];
    std::snprintf(what, sizeof(what), "%d/%d must be rejected", p.open, p.close);
    check(!Config::hoursFollowRules(p.open, p.close), what);
    std::snprintf(what, sizeof(what), "%d/%d must name the rule it breaks", p.open, p.close);
    const char *reason = Config::hoursRuleFailure(p.open, p.close);
    check(reason != nullptr && reason[0] != '\0', what);
  }

  // Exhaustive sweep, including every close <= open pair.
  for (int open = -1; open <= 30; ++open) {
    for (int close = -1; close <= 30; ++close) {
      const bool expected = expectedFollowsRules(open, close);
      if (Config::hoursFollowRules(open, close) != expected) {
        char what[96];
        std::snprintf(what, sizeof(what), "%d/%d must be %s", open, close,
                      expected ? "accepted" : "rejected");
        check(false, what);
      }
      if (close <= open && Config::hoursFollowRules(open, close)) {
        char what[96];
        std::snprintf(what, sizeof(what), "close <= open must always be rejected (%d/%d)", open, close);
        check(false, what);
      }
    }
  }

  std::printf("OK: the shared check accepts and rejects exactly the pairs the three rules allow\n");
}

// --- 2. Behaviour is unchanged for every valid pair -------------------------

void testValidPairsUnchanged() {
  for (int open = 0; open <= 12; ++open) {
    for (int close = open + 1; close <= 24; ++close) {
      if (!Config::hoursFollowRules(open, close)) {
        check(false, "sweep generated a pair the shared check rejects");
        continue;
      }

      for (int hour = 0; hour < 24; ++hour) {
        const bool now = isWithinOpenHoursForHour((uint8_t)hour, (uint8_t)open, (uint8_t)close);
        const bool was = legacyIsWithinOpenHoursForHour((uint8_t)hour, (uint8_t)open, (uint8_t)close);
        if (now != was) {
          char what[128];
          std::snprintf(what, sizeof(what), "isWithinOpenHoursForHour(%d, %d, %d) changed: %d -> %d",
                        hour, open, close, was ? 1 : 0, now ? 1 : 0);
          check(false, what);
        }
      }

      for (uint32_t sod = 0; sod < 86400UL; sod += 900) {
        const bool openNow = isWithinOpenHoursForHour((uint8_t)(sod / 3600), (uint8_t)open, (uint8_t)close);
        const int now = secondsUntilNextOpenForSeconds(sod, (uint8_t)open, (uint8_t)close, openNow);
        const int was = legacySecondsUntilNextOpenForSeconds(sod, (uint8_t)open, (uint8_t)close, openNow);
        if (now != was) {
          char what[128];
          std::snprintf(what, sizeof(what),
                        "secondsUntilNextOpenForSeconds(%u, %d, %d, %d) changed: %d -> %d",
                        (unsigned)sod, open, close, openNow ? 1 : 0, was, now);
          check(false, what);
        }
      }

      if (dailyClose((uint8_t)open, (uint8_t)close) != legacyDailyClose((uint8_t)open, (uint8_t)close)) {
        char what[96];
        std::snprintf(what, sizeof(what), "daily close hour changed for %d/%d", open, close);
        check(false, what);
      }
    }
  }

  std::printf("OK: the hour window, the next-open countdown and the daily close agree with v35 for every valid pair\n");
}

// --- 3. 0/24 is always-open, closing at midnight ---------------------------

void testAlwaysOpen() {
  for (int hour = 0; hour < 24; ++hour) {
    check(isWithinOpenHoursForHour((uint8_t)hour, 0, 24), "0/24 must be open at every hour");
    // The retired sentinel (any x/x) was always open too: 0/24 replaces it.
    check(legacyIsWithinOpenHoursForHour((uint8_t)hour, 6, 6), "sanity: the v35 sentinel was always open");
  }

  check(dailyClose(0, 24) == 24, "0/24's daily close must be hour 24 (midnight)");
  check(dailyClose(0, 24) == legacyDailyClose(6, 6), "0/24 closes where the v35 sentinel closed");

  // The one accepted value difference (WO fact check): with openNow true the
  // countdown now runs to tomorrow's opening, which at 0/24 is midnight.
  check(secondsUntilNextOpenForSeconds(12 * 3600, 0, 24, true) == 12 * 3600,
        "0/24 at noon counts down to the next midnight");

  std::printf("OK: 0/24 is open at every hour and closes at midnight\n");
}

} // namespace

int main() {
  testRulesTable();
  testValidPairsUnchanged();
  testAlwaysOpen();

  if (failures != 0) {
    std::printf("hour_rules_test: %d check(s) failed\n", failures);
    return 1;
  }

  std::printf("hour_rules_test (Part 1): all checks passed\n");
  return 0;
}
