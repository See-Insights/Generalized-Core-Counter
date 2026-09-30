/**
 * @file DailyBoundary.h
 * @brief Single owner of the "is the daily close due" test
 *        (WO-2026-09-30-001).
 *
 * @details The due-test used to live only inside `handleReportingState()`
 *          (`State_Report.cpp`), so the night-sleep commitment in
 *          `handleSleepingState()` could never ask it and hibernated with
 *          the close still pending. The logic is moved here unchanged; both
 *          the report and the night-sleep commitment now call it.
 */

#ifndef __DAILY_BOUNDARY_H
#define __DAILY_BOUNDARY_H

#include <cstdint>
#include <ctime>

namespace DailyBoundary {

/// `due` is false with `boundary`/`close` at 0 whenever the clock is not
/// trusted - the day-boundary logic must not run or stamp state on an
/// untrusted clock.
struct Result {
  bool due;
  time_t boundary;
  uint8_t close;
};

/// Evaluates whether the daily close is due as of `now` (UTC epoch).
Result check(time_t now);

} // namespace DailyBoundary

#endif /* __DAILY_BOUNDARY_H */
