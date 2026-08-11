#pragma once
#include <cstdint>

/**
 * @brief Starts a SLEEP_PREP span if one isn't already in progress.
 *        Zero-gated (not entry-flag-gated): correctly handles
 *        SLEEPING_STATE -> SLEEPING_STATE self-transitions, where a real
 *        cycle ends and a new one begins without the FSM state value
 *        changing.
 */
inline void maybeStartSleepPrepSpan(uint32_t &spanStartMillis, uint32_t nowMs) {
  if (spanStartMillis == 0) {
    spanStartMillis = nowMs;
  }
}

/**
 * @brief Closes a SLEEP_PREP span, returning its elapsed duration, and
 *        resets the timestamp so the next call to
 *        maybeStartSleepPrepSpan() starts a fresh span.
 */
inline uint32_t closeSleepPrepSpan(uint32_t &spanStartMillis, uint32_t nowMs) {
  const uint32_t elapsedMs = (spanStartMillis != 0) ? (nowMs - spanStartMillis) : 0U;
  spanStartMillis = 0;
  return elapsedMs;
}

/**
 * @brief Must be called unconditionally on every boot (setup()), before
 *        any SLEEPING_STATE dispatch. A SLEEP_PREP span cannot survive a
 *        reset of any kind (HIBERNATE success, an explicit
 *        System.reset(), or any other reset reason) - the call stack that
 *        would have closed it via closeSleepPrepSpan() is gone, so any
 *        retained nonzero value is always stale and must be discarded.
 */
inline void resetSleepPrepSpanOnBoot(uint32_t &spanStartMillis) {
  spanStartMillis = 0;
}
