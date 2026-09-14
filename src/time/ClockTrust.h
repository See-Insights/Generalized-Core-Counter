/**
 * @file ClockTrust.h
 * @brief Pure, dependency-free clock-trust and resync-gate logic.
 *
 * @details WO-2026-08-29-002 ("Clock-trust mechanism - RTC write-back,
 *          non-blocking sync observation, and a trustworthy clock signal").
 *          That WO was originally titled "Guarantee time sync, make clock
 *          validity self-known, and stop a wrong RTC re-seeding itself";
 *          the "guarantee" half was split out to WO-2026-08-31-002 on
 *          2026-08-31 and the title corrected. The old title is recorded
 *          here only so the historical reference resolves - do not read it
 *          as a claim this header makes.
 *
 *          `Time.isValid()` is NOT a trust signal in this codebase - it only
 *          means "system time has been set from some source", and
 *          `ab1805.setup()` sets it from the RTC on every boot, even when the
 *          RTC itself is wrong (see the Work Order's Investigation Finding
 *          3). The actual trust signal is the *recency of a confirmed cloud
 *          time sync*, exposed by Device OS as `Particle.timeSyncedLast()` -
 *          a `system_tick_t` (`millis()`-based, monotonic) timestamp of the
 *          last completed sync, which is `0` if no sync has completed yet
 *          this boot.
 *
 *          Every function here is driven by `millis()`/uptime values passed
 *          in by the caller, never by wall clock (`Time.now()`), so a wrong
 *          wall clock cannot postpone its own repair (item 6). This header
 *          has no Particle/AB1805 dependency and compiles/tests on the host,
 *          the same pattern used by `state/SleepPrepSpanTiming.h` and
 *          `power/ChargeInhibitPolicy.h`.
 */

#ifndef __CLOCK_TRUST_H
#define __CLOCK_TRUST_H

#include <stdint.h>

namespace ClockTrust {

// Longest interval allowed between confirmed time syncs before another is
// requested, and the longest a confirmed sync is still considered "fresh"
// for trust purposes. 24 hours, per Work Order items 6 and 8.
constexpr uint32_t kMaxSyncAgeMs = 24UL * 60UL * 60UL * 1000UL;

// Shortest interval allowed between resync attempts that did not result in
// a confirmed sync (timeout or mid-request disconnect). Without this floor,
// a device that is connected but whose sync requests never complete (the
// observed session-resume-reconnect failure mode this Work Order
// investigates) would re-request on every single loop() iteration -
// potentially hundreds of times per second - since `shouldResync()` alone
// re-arms immediately once `timeSyncedLast()` is still unchanged. This is a
// retry-pacing floor, not a trust or gate boundary, so it is intentionally
// much shorter than `kMaxSyncAgeMs`. Driven by millis()/uptime only, per the
// same "never wall clock" requirement as the rest of this header.
constexpr uint32_t kMinResyncRetryIntervalMs = 60UL * 1000UL;

/**
 * @brief millis()-rollover-safe elapsed time.
 *
 * @details Unsigned subtraction wraps correctly across the ~49.7 day
 *          `millis()` rollover, matching the idiom Device OS itself uses
 *          (e.g. `AB1805::loop()`'s `millis() - lastWatchdogMillis`).
 */
inline uint32_t elapsedMs(uint32_t nowMs, uint32_t sinceMs) {
    return nowMs - sinceMs;
}

/**
 * @brief True when a time (re)sync should be requested now.
 *
 * @param nowMs Current `millis()`.
 * @param timeSyncedLastMs `Particle.timeSyncedLast()` - `0` if no sync has
 *        completed yet this boot.
 *
 * @details `timeSyncedLastMs == 0` covers every fresh boot, including every
 *          HIBERNATE wake (HIBERNATE wake *is* a fresh boot, so
 *          `Particle.timeSyncedLast()` resets to 0 - there is nothing
 *          hibernate-specific to detect separately here). NOTE: this makes
 *          the gate *eligible* after a hibernate wake; it does NOT by itself
 *          guarantee a resync actually happens, because the caller only
 *          issues a request when `Particle.connected()`. A device that wakes
 *          with a wrong clock, reads the wrong local time as "closed" and
 *          sleeps again never becomes connected, so no request is made. That
 *          guarantee was REMOVED from this Work Order on 2026-08-31 and is
 *          now WO-2026-08-31-002's responsibility (it is permitted to touch
 *          the sleep gate, which is what closing the loop requires). Once a
 *          sync has completed this boot, the gate recurs every
 *          `kMaxSyncAgeMs` for as long as the boot stays up without
 *          hibernating.
 */
inline bool shouldResync(uint32_t nowMs, uint32_t timeSyncedLastMs) {
    if (timeSyncedLastMs == 0) {
        return true;
    }
    return elapsedMs(nowMs, timeSyncedLastMs) >= kMaxSyncAgeMs;
}

/**
 * @brief True when `Time.now()` is currently trustworthy enough to persist
 *        or report.
 *
 * @param timeValid `Time.isValid()`.
 * @param nowMs Current `millis()`.
 * @param timeSyncedLastMs `Particle.timeSyncedLast()`.
 *
 * @details Requires BOTH `Time.isValid()` and a confirmed sync within
 *          `kMaxSyncAgeMs`. Finding 3: `Time.isValid()` alone was true
 *          throughout the ~5h09m-wrong-clock incident because
 *          `ab1805.setup()` had seeded system time from the RTC - it carries
 *          no information about correctness.
 */
inline bool isTrusted(bool timeValid, uint32_t nowMs, uint32_t timeSyncedLastMs) {
    return timeValid && timeSyncedLastMs != 0 && elapsedMs(nowMs, timeSyncedLastMs) < kMaxSyncAgeMs;
}

/**
 * @brief True when enough time has passed since the last resync *attempt*
 *        to issue another one.
 *
 * @param nowMs Current `millis()`.
 * @param lastAttemptMs `millis()` at the time the last resync request was
 *        actually issued, or `0` if none has been issued yet this boot.
 *
 * @details This paces retries after a request that did not result in a
 *          confirmed sync (see `kMinResyncRetryIntervalMs`); it is separate
 *          from `shouldResync()`, which decides *whether* a resync is
 *          needed at all. `lastAttemptMs == 0` (no attempt yet this boot)
 *          always allows an immediate first attempt.
 */
inline bool canRetryResyncNow(uint32_t nowMs, uint32_t lastAttemptMs) {
    if (lastAttemptMs == 0) {
        return true;
    }
    return elapsedMs(nowMs, lastAttemptMs) >= kMinResyncRetryIntervalMs;
}

// WO-2026-08-29-002, SCOPE NARROWED 2026-08-31 cleanup task 2 (Stage 7
// finding 5): the RTC write-back retry, on a FAILED write, must not
// re-attempt ab1805.setRtcFromSystem() on every single main-loop pass. A
// permanent AB1805/Wire fault would otherwise retry a locked I2C
// transaction hundreds of times per second on a bus shared with PMIC/
// battery work, with matching log volume and watchdog pressure. Reuses the
// same monotonic (millis()-based, never wall-clock) floor value as the
// request-side retry pacing above, since both exist for the identical
// reason (bound the retry rate of something that can fail repeatedly), but
// is kept as a DISTINCT constant/function from `kMinResyncRetryIntervalMs`/
// `canRetryResyncNow()` because the two gate unrelated operations (asking
// Device OS for a sync vs. writing a value already in hand to the RTC) and
// tying their pacing together would make future evolution of either one
// (e.g. a longer I2C backoff after repeated hardware faults) undesirably
// entangle the other.
constexpr uint32_t kMinRtcWriteRetryIntervalMs = 60UL * 1000UL;

/**
 * @brief True when enough time has passed since the last FAILED RTC write
 *        attempt to retry it.
 *
 * @param nowMs Current `millis()`.
 * @param lastAttemptMs `millis()` at the time `ab1805.setRtcFromSystem()`
 *        last FAILED, or `0` if it has not failed (or has not been
 *        attempted) yet this boot for the sync value currently in hand.
 *
 * @details Only ever called by `shouldAttemptRtcWriteNow()` with a FAILED
 *          attempt's timestamp (see that function and
 *          `checkClockResync()`'s `else` branch) - never with a
 *          successful attempt's timestamp. A confirmed successful write
 *          advances `lastRtcWriteSyncedLastMs` to the just-written sync
 *          value, so the write-back gate's own
 *          `lastSyncMs != lastRtcWriteSyncedLastMs` condition already
 *          prevents any further attempt for that same sync value - the
 *          success path's cadence is unaffected by this pacing floor, and
 *          a DISTINCT new sync value is never subject to it at all,
 *          regardless of how recently a write (of any kind, for any
 *          value) was last attempted.
 */
inline bool canRetryRtcWriteNow(uint32_t nowMs, uint32_t lastAttemptMs) {
    if (lastAttemptMs == 0) {
        return true;
    }
    return elapsedMs(nowMs, lastAttemptMs) >= kMinRtcWriteRetryIntervalMs;
}

/**
 * @brief True when the observed-state RTC write-back should be attempted
 *        right now (Round 5 cleanup task 4 / Stage 7 finding 7).
 *
 * @details Extracted so `checkClockResync()`'s production write-back gate
 *          and its host tests share the exact same decision logic instead
 *          of the test re-deriving it in a hand-written mirror that can
 *          silently drift from the real code. Combines both conditions
 *          the real gate applies:
 *            - `lastSyncMs` is non-zero and has advanced past the value
 *              the RTC was last successfully written for (item 7's
 *              observed-state trigger, decoupled from request/pending
 *              bookkeeping - see `Generalized-Core-Counter.cpp`'s
 *              `checkClockResync()` design note), and
 *            - IF `lastSyncMs` is the SAME value that most recently failed
 *              to write, the failed-write retry floor
 *              (`canRetryRtcWriteNow()`) must additionally permit an
 *              attempt now.
 *
 *          Round 6 (second follow-up) fix: `lastFailedAttemptMs`/
 *          `lastFailedSyncMs` are captured ONLY on a failed write (see
 *          `checkClockResync()`'s `else` branch) - never on a success, and
 *          never unconditionally before the attempt. Consequently the
 *          60-second floor applies ONLY to a repeat attempt of the exact
 *          sync value that already failed; a distinct new confirmed sync
 *          value - whether arriving after a success or after an unrelated
 *          prior failure - is admitted immediately, because
 *          `lastSyncMs != lastFailedSyncMs` in that case. Before this fix,
 *          the floor was (incorrectly) keyed to the timestamp of the last
 *          write ATTEMPT of any kind, including successes, so a second
 *          distinct sync arriving within 60 seconds of a successful write
 *          was wrongly withheld from the RTC - a narrower recurrence of the
 *          exact defect this Work Order exists to fix, since a hibernate
 *          inside that window would re-seed system time from the older,
 *          already-superseded value.
 *
 *          Deliberately does NOT decide whether the write *succeeds* -
 *          that is `AB1805::setRtcFromSystem()`'s I2C result, which is
 *          vendored hardware I/O and cannot be part of a pure host-testable
 *          function. Callers must still branch on that call's return value
 *          to update `lastRtcWriteSyncedLastMs` on success, or
 *          `lastRtcWriteFailedAttemptMs`/`lastRtcWriteFailedSyncMs` on
 *          failure, themselves.
 *
 * @param lastSyncMs Current `observedTimeSyncedLastMs()` value.
 * @param lastWrittenSyncedLastMs `lastRtcWriteSyncedLastMs` - the sync
 *        value the RTC was last CONFIRMED written for (0 if never).
 * @param nowMs Current `millis()`.
 * @param lastFailedAttemptMs `lastRtcWriteFailedAttemptMs` - `millis()` at
 *        the last FAILED write attempt, or 0 if the write has never failed
 *        (or has never been attempted) this boot.
 * @param lastFailedSyncMs `lastRtcWriteFailedSyncMs` - the sync value that
 *        `lastFailedAttemptMs` was recorded for, or 0 if none.
 */
inline bool shouldAttemptRtcWriteNow(uint32_t lastSyncMs, uint32_t lastWrittenSyncedLastMs,
                                      uint32_t nowMs, uint32_t lastFailedAttemptMs,
                                      uint32_t lastFailedSyncMs) {
    if (lastSyncMs == 0 || lastSyncMs == lastWrittenSyncedLastMs) {
        return false;
    }
    if (lastSyncMs == lastFailedSyncMs) {
        // Retrying the exact value that already failed to write - pace it
        // so a permanent I2C/Wire fault cannot spin every main-loop pass.
        return canRetryRtcWriteNow(nowMs, lastFailedAttemptMs);
    }
    // A distinct sync value (never attempted yet, or different from the one
    // that most recently failed) must never be delayed by the retry floor -
    // that floor exists only to bound repeated failures of the SAME value.
    return true;
}

// ===== Finding 4 (Round 4 review): millis() full-wrap safety =====
//
// `elapsedMs()`'s unsigned subtraction is correct for any true elapsed span
// up to (2^32 - 1) ms (~49.7 days) - that is what
// `testElapsedMsWrapsCorrectlyAcrossMillisRollover()` proves. It cannot,
// however, distinguish "just synced" from "synced, then the device stayed
// up so long without another confirmed sync that millis() wrapped all the
// way back around past that timestamp again" - both produce the same small
// `nowMs - sinceMs` result. A device that syncs once and then stays booted
// (never hibernating) for more than one full wrap without another
// confirmed sync would have an ~49.7-day-stale sync misread as fresh for
// up to another `kMaxSyncAgeMs`, suppressing a resync it genuinely needs.
//
// `Particle.timeSyncedLast()` itself is only a 32-bit `millis()` snapshot -
// Device OS gives no wrap information. So the caller (checkClockResync())
// must track, itself, how many times `millis()` has wrapped since the
// currently-observed sync value was captured, and pass that in here as two
// generation counters: `currentWrapGeneration` (the live count of wraps
// observed so far this boot) and `syncCaptureWrapGeneration` (the value of
// that same counter at the moment the currently-observed
// `Particle.timeSyncedLast()` value was captured). If they differ, at
// least one full wrap has happened since - treat the sync as maximally
// stale rather than trust whatever (possibly small, wrapped) value
// `elapsedMs()` computes.
inline bool shouldResyncWrapAware(uint32_t nowMs, uint32_t timeSyncedLastMs,
                                   uint32_t currentWrapGeneration,
                                   uint32_t syncCaptureWrapGeneration) {
    if (timeSyncedLastMs == 0) {
        return true;
    }
    if (currentWrapGeneration != syncCaptureWrapGeneration) {
        return true;
    }
    return elapsedMs(nowMs, timeSyncedLastMs) >= kMaxSyncAgeMs;
}

inline bool isTrustedWrapAware(bool timeValid, uint32_t nowMs, uint32_t timeSyncedLastMs,
                                uint32_t currentWrapGeneration,
                                uint32_t syncCaptureWrapGeneration) {
    if (!timeValid || timeSyncedLastMs == 0) {
        return false;
    }
    if (currentWrapGeneration != syncCaptureWrapGeneration) {
        return false;
    }
    return elapsedMs(nowMs, timeSyncedLastMs) < kMaxSyncAgeMs;
}

// ===== Round 6 (Stage 7 finding 1): wrap-aware, sentinel-bearing =====
// ===== REPORTED sync age (telemetry only, not a control signal) =====
//
// `elapsedMs()` is correct for the trust DECISION (isTrustedWrapAware()/
// shouldResyncWrapAware()) because that decision is always re-evaluated
// against a bounded ceiling (kMaxSyncAgeMs) - it never needs to represent
// an age beyond that ceiling. REPORTING is different: item 9's telemetry
// wants to show a human/consumer how old the sync actually is, including
// values that make the trust verdict legible - `elapsedMs()`'s raw
// unsigned subtraction, on its own, produces two misleading results for
// that purpose: (1) after a full millis() wrap past the captured sync
// timestamp with no further confirmed sync, it wraps back around to a
// small, deceptively fresh-looking value even though isTrustedWrapAware()
// has (correctly) already gone false; (2) before any sync has completed
// this boot (timeSyncedLastMs == 0), naively computing "elapsed since 0"
// reports uptime, not a sync age.
//
// `kReportedSyncAgeUnavailableMs` covers BOTH cases with a single sentinel
// meaning "no reliable sync age is available to report right now" -
// deliberately not trying to distinguish "never synced" from "stale beyond
// a full wrap" at this layer, since callers already have `isTrustedWrapAware()`
// (false in both cases) and `timeSyncedLastMs == 0` on hand if they need to
// tell those two apart for other purposes; this function only prevents a
// misleading NUMBER from being reported for either. Chosen as UINT32_MAX
// because no genuine elapsed span this codebase ever measures can reach it
// (the trust ceiling itself, kMaxSyncAgeMs, is ~24h - many orders of
// magnitude below UINT32_MAX ms, i.e. ~49.7 days), so a log reader or
// ledger consumer can never mistake it for a real duration.
constexpr uint32_t kReportedSyncAgeUnavailableMs = 0xFFFFFFFFUL;

/**
 * @brief Sync age (in `millis()`, i.e. same units as `elapsedMs()`) safe to
 *        REPORT in telemetry - not a control-path value.
 *
 * @param nowMs Current `millis()`.
 * @param timeSyncedLastMs `Particle.timeSyncedLast()` (via
 *        `observedTimeSyncedLastMs()`), `0` if no sync has completed yet
 *        this boot.
 * @param currentWrapGeneration The live wrap-generation counter (see
 *        `isTrustedWrapAware()`'s doc comment for the full design).
 * @param syncCaptureWrapGeneration The wrap-generation counter's value at
 *        the moment `timeSyncedLastMs` was captured.
 *
 * @details Reuses the EXACT SAME wrap-generation tracking the trust
 *          decision already uses - no parallel mechanism. Returns
 *          `kReportedSyncAgeUnavailableMs` for both "never synced this
 *          boot" and "at least one full wrap has occurred since the
 *          currently-observed sync was captured" (both cases where a raw
 *          `elapsedMs()` result would be misleading), and the true elapsed
 *          span otherwise.
 */
inline uint32_t wrapAwareReportedSyncAgeMs(uint32_t nowMs, uint32_t timeSyncedLastMs,
                                            uint32_t currentWrapGeneration,
                                            uint32_t syncCaptureWrapGeneration) {
    if (timeSyncedLastMs == 0) {
        return kReportedSyncAgeUnavailableMs;
    }
    if (currentWrapGeneration != syncCaptureWrapGeneration) {
        return kReportedSyncAgeUnavailableMs;
    }
    return elapsedMs(nowMs, timeSyncedLastMs);
}

} // namespace ClockTrust

#endif
