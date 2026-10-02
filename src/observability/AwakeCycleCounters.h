/**
 * @file AwakeCycleCounters.h
 * @brief RAM counters for awake periods and successful sleeps
 *        (WO-2026-10-02-001 item E).
 *
 * @details `cycles` starts at 1 at boot and increments after every return
 *          from `System.sleep()`, whether the call succeeded or failed.
 *          `sleeps` increments only for calls that succeeded. Neither is
 *          retained, so a reset (including the night HIBERNATE) restarts
 *          both. Therefore `cycles >= sleeps` always, `cycles - sleeps - 1`
 *          is the number of failed sleep calls, and the same `cyc` in two
 *          consecutive reports means the device never slept between them.
 */

#pragma once
#include <cstdint>
namespace AwakeCycles { inline uint32_t cycles = 1, sleeps = 0;
inline void recordSleepReturn(bool slept) { ++cycles; if (slept) ++sleeps; } }
