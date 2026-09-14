// Host test for src/time/ClockTrust.h (WO-2026-08-29-002 items 6/7/8).
//
// This header has zero Particle/AB1805 dependencies (no Particle.h,
// AB1805_RK.h, or other project headers - only <stdint.h>), so it compiles
// and runs directly on the host, the same pattern used by
// tests/sleep_prep_span_timing_test.cpp and
// src/power/ChargeInhibitPolicy.h's host tests.
//
// Covers the bench-test scenario from the Work Order's Verification
// section, to the extent it is host-testable:
//   1/2. A skewed RTC seeds Time.isValid()==true at boot, with no confirmed
//        sync yet (Particle.timeSyncedLast() == 0) - isTrusted() must be
//        false and shouldResync() must be true, even though "valid" is
//        true. This is the exact Finding-3 gap: Time.isValid() alone gave
//        no information about correctness.
//   3. After a confirmed sync, the gate is satisfied immediately, and it
//      recurs on a >=24h cadence rather than being a one-shot - proving
//      the design is not the AB1805::loop() per-boot `timeSet` latch this
//      Work Order replaces.
//   5. isTrusted() flips false -> true across the same before/after
//      confirmed-sync transition used for the resync gate.
//
// (Step 4 - surviving a real HIBERNATE wake - requires the actual AB1805
// RTC hardware and is the bench device's job per the Work Order; nothing
// in this pure header can exercise real hardware.)

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "time/ClockTrust.h"

namespace {

void testNeverSyncedThisBootIsUntrustedAndNeedsResync() {
  // Mirrors ab1805.setup() seeding system time from a wrong RTC: the caller
  // would observe Time.isValid() == true, but Particle.timeSyncedLast() is
  // still 0 because no sync has completed yet this boot.
  const bool timeValid = true;
  const uint32_t nowMs = 5000; // a few seconds into boot
  const uint32_t timeSyncedLastMs = 0;

  assert(ClockTrust::shouldResync(nowMs, timeSyncedLastMs) == true);
  assert(ClockTrust::isTrusted(timeValid, nowMs, timeSyncedLastMs) == false);
}

void testConfirmedSyncIsTrustedAndGateClosesImmediately() {
  const bool timeValid = true;
  const uint32_t timeSyncedLastMs = 10000;
  const uint32_t nowMs = timeSyncedLastMs + 1; // sync just completed

  assert(ClockTrust::isTrusted(timeValid, nowMs, timeSyncedLastMs) == true);
  assert(ClockTrust::shouldResync(nowMs, timeSyncedLastMs) == false);
}

void testGateRecursAfterMaxSyncAgeRatherThanStayingSpent() {
  // This is the core item-6/7 regression proof: unlike AB1805::loop()'s
  // per-boot `timeSet` latch (which, once spent, blocks every later
  // correction for that boot), shouldResync() must fire again once
  // kMaxSyncAgeMs has elapsed, on a recurring basis.
  const uint32_t timeSyncedLastMs = 1000;

  // Just under the threshold: gate stays closed.
  const uint32_t justBefore = timeSyncedLastMs + ClockTrust::kMaxSyncAgeMs - 1;
  assert(ClockTrust::shouldResync(justBefore, timeSyncedLastMs) == false);

  // At/after the threshold: gate reopens.
  const uint32_t atThreshold = timeSyncedLastMs + ClockTrust::kMaxSyncAgeMs;
  assert(ClockTrust::shouldResync(atThreshold, timeSyncedLastMs) == true);

  const uint32_t wellAfter = atThreshold + 60000;
  assert(ClockTrust::shouldResync(wellAfter, timeSyncedLastMs) == true);
}

void testTrustDegradesAfterMaxSyncAge() {
  const bool timeValid = true;
  const uint32_t timeSyncedLastMs = 2000;

  const uint32_t justBefore = timeSyncedLastMs + ClockTrust::kMaxSyncAgeMs - 1;
  assert(ClockTrust::isTrusted(timeValid, justBefore, timeSyncedLastMs) == true);

  const uint32_t atThreshold = timeSyncedLastMs + ClockTrust::kMaxSyncAgeMs;
  assert(ClockTrust::isTrusted(timeValid, atThreshold, timeSyncedLastMs) == false);
}

void testTimeInvalidNeverTrustedEvenIfRecentlySynced() {
  // Defensive: a confirmed-recent sync alone shouldn't be trusted if
  // Time.isValid() somehow became false again (belt-and-suspenders; the two
  // signals are ANDed, matching item 8's "not Time.isValid() alone" -
  // meaning isValid() is necessary too, just not sufficient).
  const uint32_t nowMs = 5000;
  const uint32_t timeSyncedLastMs = 4999;
  assert(ClockTrust::isTrusted(false, nowMs, timeSyncedLastMs) == false);
}

void testElapsedMsWrapsCorrectlyAcrossMillisRollover() {
  // millis() rolls over roughly every 49.7 days. sinceMs just before the
  // rollover, nowMs just after - unsigned subtraction must still yield the
  // small, correct elapsed value instead of a huge one.
  const uint32_t sinceMs = 0xFFFFFFF0u; // 16 ms before rollover
  const uint32_t nowMs = 10u;           // 10 ms after rollover
  const uint32_t expectedElapsed = 26u; // 16 ms to rollover + 10 ms after
  assert(ClockTrust::elapsedMs(nowMs, sinceMs) == expectedElapsed);

  // shouldResync()/isTrusted() must not spuriously fire/fail across a
  // rollover for a sync that only just completed.
  assert(ClockTrust::shouldResync(nowMs, sinceMs) == false);
  assert(ClockTrust::isTrusted(true, nowMs, sinceMs) == true);
}

void testNeverSyncedAtBootZeroMillisIsStillUntrusted() {
  // Edge case: the very first check at nowMs == 0 (boot instant), with no
  // sync completed yet. timeSyncedLastMs == 0 must still mean "never
  // synced", not be confused with "synced at millis()==0".
  assert(ClockTrust::shouldResync(0, 0) == true);
  assert(ClockTrust::isTrusted(true, 0, 0) == false);
}

void testCanRetryResyncNowAllowsFirstAttemptImmediately() {
  // No attempt yet this boot (lastAttemptMs == 0) - an immediate first
  // attempt must always be allowed, regardless of nowMs.
  assert(ClockTrust::canRetryResyncNow(0, 0) == true);
  assert(ClockTrust::canRetryResyncNow(123456, 0) == true);
}

void testCanRetryResyncNowPacesRetriesAfterAFailedAttempt() {
  // Review fix (Change 1): Particle.syncTimeDone() going true does not mean
  // success - it also goes true on protocol timeout or mid-request
  // disconnect. Without a retry-pacing floor, a device that stays connected
  // but whose sync never completes would re-request on every single loop()
  // iteration. canRetryResyncNow() must block retries until
  // kMinResyncRetryIntervalMs has elapsed since the last attempt...
  const uint32_t lastAttemptMs = 10000;
  const uint32_t justBefore = lastAttemptMs + ClockTrust::kMinResyncRetryIntervalMs - 1;
  assert(ClockTrust::canRetryResyncNow(justBefore, lastAttemptMs) == false);

  // ...and must allow it again once that floor has elapsed (recurring, not
  // a one-shot block - matching the rest of this header's design).
  const uint32_t atThreshold = lastAttemptMs + ClockTrust::kMinResyncRetryIntervalMs;
  assert(ClockTrust::canRetryResyncNow(atThreshold, lastAttemptMs) == true);
}

void testCanRetryResyncNowWrapsCorrectlyAcrossMillisRollover() {
  const uint32_t lastAttemptMs = 0xFFFFFFF0u; // 16 ms before rollover
  const uint32_t nowMs = 10u;                 // 10 ms after rollover (26ms elapsed)
  assert(ClockTrust::canRetryResyncNow(nowMs, lastAttemptMs) == false);
}

// ===== Finding 4 (Round 4 review): millis() full-wrap safety =====

void testWrapAwareGatesBehaveLikePlainGatesWithinASingleWrapPeriod() {
  // No wrap has occurred (currentWrapGeneration == syncCaptureWrapGeneration)
  // - the wrap-aware variants must agree exactly with the plain variants
  // for ordinary spans (this is the "no regression for the common case"
  // proof).
  const bool timeValid = true;
  const uint32_t timeSyncedLastMs = 10000;
  const uint32_t nowMs = timeSyncedLastMs + 1;

  assert(ClockTrust::shouldResyncWrapAware(nowMs, timeSyncedLastMs, 0, 0) ==
         ClockTrust::shouldResync(nowMs, timeSyncedLastMs));
  assert(ClockTrust::isTrustedWrapAware(timeValid, nowMs, timeSyncedLastMs, 0, 0) ==
         ClockTrust::isTrusted(timeValid, nowMs, timeSyncedLastMs));

  const uint32_t atThreshold = timeSyncedLastMs + (uint32_t)ClockTrust::kMaxSyncAgeMs;
  assert(ClockTrust::shouldResyncWrapAware(atThreshold, timeSyncedLastMs, 5, 5) == true);
  assert(ClockTrust::isTrustedWrapAware(timeValid, atThreshold, timeSyncedLastMs, 5, 5) == false);
}

void testWrapAwareGatesTreatAFullWrapAsMaximallyStale() {
  // The core Finding-4 regression proof: a sync completed a long time ago
  // (captured at wrap generation 0), the device never hibernated and never
  // synced again, and millis() has since wrapped a full cycle (generation
  // now 1). Plain elapsedMs()-based shouldResync()/isTrusted() cannot see
  // this - feed them a small nowMs (just after the wrap) and a timeSyncedLastMs
  // just before the wrap, and they would misreport "just synced, still
  // fresh". The wrap-aware variants must instead treat this as needing a
  // resync / not trusted, regardless of what the raw millis() difference
  // says.
  const bool timeValid = true;
  const uint32_t timeSyncedLastMs = 0xFFFFFFF0u; // captured just before rollover
  const uint32_t nowMs = 10u;                    // just after rollover

  // Sanity: the PLAIN (non-wrap-aware) functions are indeed fooled here -
  // this is the exact defect being guarded against, not a strawman.
  assert(ClockTrust::shouldResync(nowMs, timeSyncedLastMs) == false);
  assert(ClockTrust::isTrusted(timeValid, nowMs, timeSyncedLastMs) == true);

  // The wrap-aware variants, told a wrap occurred since capture
  // (currentWrapGeneration=1 != syncCaptureWrapGeneration=0), must not be
  // fooled.
  assert(ClockTrust::shouldResyncWrapAware(nowMs, timeSyncedLastMs, /*currentWrapGeneration=*/1,
                                            /*syncCaptureWrapGeneration=*/0) == true);
  assert(ClockTrust::isTrustedWrapAware(timeValid, nowMs, timeSyncedLastMs, /*currentWrapGeneration=*/1,
                                          /*syncCaptureWrapGeneration=*/0) == false);
}

void testWrapAwareGatesStillHandleNeverSyncedAndTimeInvalid() {
  // timeSyncedLastMs == 0 (never synced) must still force a resync
  // regardless of wrap generations.
  assert(ClockTrust::shouldResyncWrapAware(5000, 0, 3, 3) == true);
  assert(ClockTrust::isTrustedWrapAware(true, 5000, 0, 3, 3) == false);

  // Time.isValid() == false must still deny trust even with matching
  // generations and a recent sync.
  assert(ClockTrust::isTrustedWrapAware(false, 5000, 4999, 7, 7) == false);
}

void testReportedSyncAgeIsUnavailableSentinelWhenNeverSyncedNotUptime() {
  // Round 6 (Stage 7 finding 1, defect 2): before any sync has completed
  // this boot (timeSyncedLastMs == 0), the REPORTED age must be the
  // "unavailable" sentinel - never elapsed-since-zero (uptime), which is a
  // real, misleading number that means nothing as a sync age and is
  // ambiguous with "synced 0 ms ago".
  const uint32_t nowMs = 999999u; // sizeable uptime, would look like a real age if misreported
  assert(ClockTrust::wrapAwareReportedSyncAgeMs(nowMs, /*timeSyncedLastMs=*/0,
                                                 /*currentWrapGeneration=*/0,
                                                 /*syncCaptureWrapGeneration=*/0)
         == ClockTrust::kReportedSyncAgeUnavailableMs);

  // Sanity: raw elapsedMs() (what the pre-fix code used) WOULD misreport
  // uptime here - proving this is a real defect being guarded against, not
  // a strawman.
  assert(ClockTrust::elapsedMs(nowMs, 0) == nowMs);
}

void testReportedSyncAgeIsUnavailableSentinelAfterFullWrapNotASmallStaleValue() {
  // Round 6 (Stage 7 finding 1, defect 1): a sync completed a long time ago
  // (captured at wrap generation 0), the device never synced again, and
  // millis() has since wrapped a full cycle (generation now 1). The trust
  // decision (isTrustedWrapAware()) correctly goes false in this exact
  // scenario (proven by testWrapAwareGatesTreatAFullWrapAsMaximallyStale()
  // above) - the REPORTED age must agree, not contradict it with a small,
  // deceptively fresh-looking number.
  const uint32_t timeSyncedLastMs = 0xFFFFFFF0u; // captured just before rollover
  const uint32_t nowMs = 10u;                    // just after rollover

  // Sanity: raw elapsedMs() WOULD misreport a small, fresh-looking age here
  // (this is the exact defect - proving it's real, not a strawman).
  assert(ClockTrust::elapsedMs(nowMs, timeSyncedLastMs) == 26u);

  // The wrap-aware reported age must not be that small value - it must be
  // the "unavailable" sentinel, consistent with trusted==false.
  assert(ClockTrust::wrapAwareReportedSyncAgeMs(nowMs, timeSyncedLastMs,
                                                 /*currentWrapGeneration=*/1,
                                                 /*syncCaptureWrapGeneration=*/0)
         == ClockTrust::kReportedSyncAgeUnavailableMs);
  assert(ClockTrust::isTrustedWrapAware(/*timeValid=*/true, nowMs, timeSyncedLastMs,
                                         /*currentWrapGeneration=*/1,
                                         /*syncCaptureWrapGeneration=*/0) == false);
}

void testReportedSyncAgeIsTheRealElapsedSpanWhenGenuinelyFresh() {
  // Negative control: within a single wrap generation, with a genuine
  // recent sync, the reported age must be the REAL elapsed span, not the
  // sentinel - the fix must not blanket-sentinel every case.
  const uint32_t nowMs = 50000u;
  const uint32_t timeSyncedLastMs = 40000u;
  const uint32_t reportedAgeMs = ClockTrust::wrapAwareReportedSyncAgeMs(
      nowMs, timeSyncedLastMs, /*currentWrapGeneration=*/2, /*syncCaptureWrapGeneration=*/2);
  assert(reportedAgeMs == 10000u);
  assert(reportedAgeMs != ClockTrust::kReportedSyncAgeUnavailableMs);
}

} // namespace

int main() {
  testNeverSyncedThisBootIsUntrustedAndNeedsResync();
  testConfirmedSyncIsTrustedAndGateClosesImmediately();
  testGateRecursAfterMaxSyncAgeRatherThanStayingSpent();
  testTrustDegradesAfterMaxSyncAge();
  testTimeInvalidNeverTrustedEvenIfRecentlySynced();
  testElapsedMsWrapsCorrectlyAcrossMillisRollover();
  testNeverSyncedAtBootZeroMillisIsStillUntrusted();
  testCanRetryResyncNowAllowsFirstAttemptImmediately();
  testCanRetryResyncNowPacesRetriesAfterAFailedAttempt();
  testCanRetryResyncNowWrapsCorrectlyAcrossMillisRollover();
  testWrapAwareGatesBehaveLikePlainGatesWithinASingleWrapPeriod();
  testWrapAwareGatesTreatAFullWrapAsMaximallyStale();
  testWrapAwareGatesStillHandleNeverSyncedAndTimeInvalid();
  testReportedSyncAgeIsUnavailableSentinelWhenNeverSyncedNotUptime();
  testReportedSyncAgeIsUnavailableSentinelAfterFullWrapNotASmallStaleValue();
  testReportedSyncAgeIsTheRealElapsedSpanWhenGenuinelyFresh();

  printf("clock_trust_test: all assertions passed\n");
  return 0;
}
