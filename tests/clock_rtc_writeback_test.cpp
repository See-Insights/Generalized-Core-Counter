// Host test for the RTC write-back state machine added to
// checkClockResync() (WO-2026-08-29-002, review fixes through Round 5).
//
// checkClockResync() itself lives in Generalized-Core-Counter.cpp and can't
// be compiled standalone on the host (heavy Particle/AB1805/PublishQueue
// dependencies - see tests/README.md's established precedent). Its
// write-back gate DECISION, however, is now a small, pure, shared function
// (Round 5 cleanup task 4, Stage 7 finding 7) that this test calls directly
// - not a hand-written mirror of it:
//
//     if (ClockTrust::shouldAttemptRtcWriteNow(lastSyncMs, lastRtcWriteSyncedLastMs,
//                                               nowMs, lastRtcWriteFailedAttemptMs,
//                                               lastRtcWriteFailedSyncMs)) {
//       const bool rtcUpdated = ab1805.setRtcFromSystem();
//       if (rtcUpdated) {
//         lastRtcWriteSyncedLastMs = lastSyncMs;
//         sysStatus.set_lastTimeSync(Time.now());
//       } else {
//         lastRtcWriteFailedAttemptMs = nowMs;
//         lastRtcWriteFailedSyncMs = lastSyncMs;
//       }
//     }
//
// `RtcWriteBackSimulator` below calls the REAL ClockTrust::shouldAttemptRtcWriteNow()
// for the gate condition, with only the real ab1805.setRtcFromSystem() I2C
// call replaced by an injectable outcome (rtcWriteSucceeds - this part is
// vendored hardware I/O and cannot be pure host code), so the gate decision
// itself is exercised with real production logic on real call sequences.
// tests/clock_resync_wiring_test.py separately parses the ACTUAL shipped
// source and asserts checkClockResync() calls this exact shared function
// (rather than re-deriving the condition inline) and in the correct order
// relative to the confirmed-write bookkeeping - so the two tests together
// prove both "the real code calls the real gate function, in the right
// place" and "that gate function behaves correctly", with no hand-written
// re-derivation of the condition left to drift from the real one.
//
// Covers the scenarios from the Chief Engineer's reviews:
//   1. A sync completing WITHOUT any app-initiated request (Device OS's own
//      cloud-handshake sync) still drives an RTC write - the gate reacts to
//      OBSERVED Particle.timeSyncedLast() state only, never to whether this
//      app called requestClockResync().
//   2. The step-2 race explicitly: syncTimeDone() (not modeled here, since
//      the gate no longer references it at all - that IS the fix) going
//      true before timeSyncedLast() advances, then timeSyncedLast()
//      advancing on a later check, still produces exactly one RTC write.
//   3. Exactly-once semantics for THIS function's own idempotency: a single
//      sync value must not produce repeated RTC writes on subsequent checks
//      while timeSyncedLast() is unchanged. (This is explicitly NOT a claim
//      about the total number of physical RTC writes across the whole
//      system - see Finding 8 below and checkClockResync()'s doc comment.)
//   4. No sync at all (timeSyncedLast() stays 0): no RTC write, no
//      lastTimeSync stamp ever occurs.
//   5. Finding 1 (Round 4 review, HIGH): a FAILED write must not be
//      permanently treated as done. The tracked value must only advance
//      after a CONFIRMED successful write; a failed write must retry on a
//      later check (past the retry floor) with the same lastSyncMs, and
//      only stop retrying once a write actually succeeds.
//   6. Finding 8 (Round 4 review, LOW, documented not "fixed"): the
//      vendored AB1805::loop() one-shot reacts to the same "a sync has
//      completed" condition as this gate and runs immediately before it in
//      loop() - so the first successful sync of each boot is written to the
//      RTC twice in practice (once by the vendored one-shot, once by this
//      gate). This test suite does NOT claim otherwise; it only proves this
//      gate's OWN calls to ab1805.setRtcFromSystem() are exactly-once per
//      observed sync advance.
//   7. Round 5 cleanup task 2 (Stage 7 finding 5, HIGH): a FAILED write must
//      be PACED, not retried on every single check. Without pacing, a
//      permanent AB1805/Wire fault would retry a locked I2C transaction on
//      every main-loop pass - potentially hundreds of times per second on a
//      bus shared with PMIC/battery work. This replaces the OLD test that
//      asserted "200 attempts in 200 checks", which had codified the unpaced
//      hot loop as a requirement rather than catching it as a defect.
//   8. Round 6 (second follow-up), Stage 7 pass 4 (HIGH): the pacing floor
//      from (7) must NEVER delay admission of a DISTINCT new sync value,
//      even if it arrives within kMinRtcWriteRetryIntervalMs of a prior
//      SUCCESSFUL write - only a repeat attempt of the exact value that
//      already FAILED is paced. The Round 5 implementation kept a single
//      "last attempt of any kind" timestamp and applied the floor to every
//      write attempt regardless of outcome, so a second distinct confirmed
//      sync arriving soon after a success was wrongly withheld from the
//      RTC - a narrower recurrence of the exact defect this Work Order
//      exists to fix, since a hibernate inside that window would persist
//      the OLDER, already-superseded sync value. Fixed by tracking
//      lastRtcWriteFailedAttemptMs/lastRtcWriteFailedSyncMs - set ONLY on
//      failure - so the floor only ever keys off a failed attempt of the
//      SAME value.

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "time/ClockTrust.h"

namespace {

// Line-for-line mirror of checkClockResync()'s write-back gate, with the
// real ab1805.setRtcFromSystem() call replaced by an injectable outcome
// (rtcWriteSucceeds) so Finding 1's failure/retry path can be exercised
// without real hardware.
//
// Round 5 cleanup task 4 (Stage 7 finding 7): the GATE CONDITION itself is
// no longer re-derived here - it calls ClockTrust::shouldAttemptRtcWriteNow(),
// the exact same pure function checkClockResync() calls in production (see
// tests/clock_resync_wiring_test.py's write_back_gate check, which parses
// the real source and asserts it calls this same function). Only the parts
// that genuinely cannot be pure host code remain simulated here: the
// ab1805.setRtcFromSystem() I2C call's outcome (rtcWriteSucceeds) and the
// bookkeeping updates (lastRtcWriteSyncedLastMs,
// lastRtcWriteFailedAttemptMs/lastRtcWriteFailedSyncMs,
// sysStatus.set_lastTimeSync()) that checkClockResync() performs around
// that real call. This closes the "tests a hand-written simulator, not the
// production function" gap for the gate DECISION, while the I2C outcome
// remains necessarily mocked (it is vendored hardware I/O).
struct RtcWriteBackSimulator {
  uint32_t lastRtcWriteSyncedLastMs = 0;
  // Round 6 (second follow-up): failure-only pair, mirroring production's
  // fix - never touched on a success, so the pacing floor can never delay
  // a distinct new sync value.
  uint32_t lastRtcWriteFailedAttemptMs = 0;
  uint32_t lastRtcWriteFailedSyncMs = 0;
  bool rtcWriteSucceeds = true; // stands in for ab1805.setRtcFromSystem()'s return value
  int rtcWriteAttemptCount = 0;  // every call to ab1805.setRtcFromSystem(), success or failure
  int rtcWriteConfirmedCount = 0; // only calls that returned true
  int lastTimeSyncStampCount = 0;

  // Mirrors:
  //   if (ClockTrust::shouldAttemptRtcWriteNow(lastSyncMs, lastRtcWriteSyncedLastMs,
  //                                             nowMs, lastRtcWriteFailedAttemptMs,
  //                                             lastRtcWriteFailedSyncMs)) {
  //     const bool rtcUpdated = ab1805.setRtcFromSystem();
  //     if (rtcUpdated) {
  //       lastRtcWriteSyncedLastMs = lastSyncMs;
  //       sysStatus.set_lastTimeSync(Time.now());
  //     } else {
  //       lastRtcWriteFailedAttemptMs = nowMs;
  //       lastRtcWriteFailedSyncMs = lastSyncMs;
  //     }
  //   }
  // The condition itself (the `if`) is the REAL production function, not a
  // re-derivation of it - only the body's I2C call is simulated.
  void check(uint32_t nowMs, uint32_t lastSyncMs) {
    if (ClockTrust::shouldAttemptRtcWriteNow(lastSyncMs, lastRtcWriteSyncedLastMs,
                                              nowMs, lastRtcWriteFailedAttemptMs,
                                              lastRtcWriteFailedSyncMs)) {
      rtcWriteAttemptCount++;              // mirrors calling ab1805.setRtcFromSystem()
      const bool rtcUpdated = rtcWriteSucceeds;
      if (rtcUpdated) {
        lastRtcWriteSyncedLastMs = lastSyncMs; // Finding 1: only on confirmed success
        rtcWriteConfirmedCount++;
        lastTimeSyncStampCount++;              // mirrors sysStatus.set_lastTimeSync(Time.now())
      } else {
        lastRtcWriteFailedAttemptMs = nowMs;    // Round 6: failure-only, never set on success
        lastRtcWriteFailedSyncMs = lastSyncMs;
      }
    }
  }
};

void testHandshakeInitiatedSyncDrivesRtcWriteWithNoAppRequest() {
  // Device OS performs its own time sync at cloud handshake, entirely
  // independent of requestClockResync(). The gate must react to this the
  // same as an app-requested sync, since it only observes
  // Particle.timeSyncedLast() - there is no "who asked for this" input to
  // the gate at all.
  RtcWriteBackSimulator sim;
  assert(sim.rtcWriteConfirmedCount == 0);

  sim.check(1000, 0); // fresh boot, no sync yet - no write
  assert(sim.rtcWriteConfirmedCount == 0);
  assert(sim.lastTimeSyncStampCount == 0);

  // Device OS's own handshake sync completes; timeSyncedLast() advances to
  // a nonzero value with no preceding requestClockResync() call modeled at
  // all (this simulator has no request-side state to even manipulate).
  sim.check(2000, 15000);
  assert(sim.rtcWriteConfirmedCount == 1);
  assert(sim.lastTimeSyncStampCount == 1);
}

void testFastCompletionRaceStillProducesExactlyOneWrite() {
  // The exact failure trace from the 2nd-round review: a request is issued,
  // and the system thread completes it before the very next loop()
  // iteration observes it. Modeled here as: the observed timeSyncedLast()
  // simply jumps straight from 0 to a nonzero value on the first
  // post-request check - there is no separate "syncTimeDone() went true
  // early" state for this gate to get confused by, because the gate no
  // longer looks at syncTimeDone() at all. This IS the fix: removing that
  // dependency removes the race entirely rather than papering over it.
  RtcWriteBackSimulator sim;

  sim.check(1000, 0); // pre-request state
  assert(sim.rtcWriteConfirmedCount == 0);

  // "Request issued, races ahead, completes before this very next check."
  sim.check(2000, 20000);
  assert(sim.rtcWriteConfirmedCount == 1);
  assert(sim.lastTimeSyncStampCount == 1);

  // A subsequent check before anything else changes must not double-write.
  sim.check(3000, 20000);
  assert(sim.rtcWriteConfirmedCount == 1);
  assert(sim.lastTimeSyncStampCount == 1);
}

void testExactlyOnceWritePerAdvanceNotPerCheck() {
  // A single sync (timeSyncedLast() constant across many checks - e.g. many
  // loop() iterations between resyncs) must produce exactly one RTC write,
  // not one per check.
  RtcWriteBackSimulator sim;

  sim.check(1000, 5000);
  assert(sim.rtcWriteConfirmedCount == 1);

  uint32_t nowMs = 1000;
  for (int i = 0; i < 50; i++) {
    nowMs += 1000;
    sim.check(nowMs, 5000); // simulates 50 more loop() iterations, no new sync
  }
  assert(sim.rtcWriteConfirmedCount == 1);
  assert(sim.lastTimeSyncStampCount == 1);

  // A genuinely new sync (24h later, say) must produce exactly one more
  // write - recurring, not a one-shot latch.
  nowMs += (uint32_t)ClockTrust::kMaxSyncAgeMs + 1;
  sim.check(nowMs, 5000 + (uint32_t)ClockTrust::kMaxSyncAgeMs + 1);
  assert(sim.rtcWriteConfirmedCount == 2);
  assert(sim.lastTimeSyncStampCount == 2);
}

void testNoSyncEverMeansNoWriteAndClockStaysUntrusted() {
  // timeSyncedLast() staying 0 across many checks (device never
  // successfully syncs - e.g. never connects) must never write the RTC or
  // stamp lastTimeSync, and (via ClockTrust::isTrusted(), tested directly
  // here rather than mirrored) the clock must never be reported trusted.
  RtcWriteBackSimulator sim;
  uint32_t nowMs = 1000;
  for (int i = 0; i < 100; i++) {
    nowMs += 1000;
    sim.check(nowMs, 0);
  }
  assert(sim.rtcWriteConfirmedCount == 0);
  assert(sim.lastTimeSyncStampCount == 0);

  const bool timeValid = true; // ab1805.setup() still seeds a wrong RTC
  const uint32_t nowMsForTrust = 999999;
  assert(ClockTrust::isTrusted(timeValid, nowMsForTrust, /*timeSyncedLastMs=*/0) == false);
}

void testRetryPacingStillHonouredOnRequestSide() {
  // The request side (kept, unchanged by the 2nd review fix) must still
  // pace automatic retries via ClockTrust::canRetryResyncNow(), independent
  // of the write-back gate above - the two are separate concerns by design.
  const uint32_t lastAttemptMs = 30000;
  const uint32_t tooSoon = lastAttemptMs + ClockTrust::kMinResyncRetryIntervalMs - 1;
  const uint32_t longEnough = lastAttemptMs + ClockTrust::kMinResyncRetryIntervalMs;

  assert(ClockTrust::canRetryResyncNow(tooSoon, lastAttemptMs) == false);
  assert(ClockTrust::canRetryResyncNow(longEnough, lastAttemptMs) == true);

  // And the request-side gate itself (shouldResync()) is unaffected by the
  // write-back gate's internal lastRtcWriteSyncedLastMs bookkeeping - it
  // only ever looks at timeSyncedLast() and millis(), per ClockTrust.h.
  assert(ClockTrust::shouldResync(tooSoon, /*timeSyncedLastMs=*/0) == true);
}

void testFinding1FailedWriteIsRetriedNotPermanentlyConsumed() {
  // Finding 1 (Round 4 review, HIGH): if ab1805.setRtcFromSystem() fails
  // (returns false - e.g. a transient I2C error), the tracked value must
  // NOT advance, so a LATER check (still observing the same unwritten
  // lastSyncMs, once the retry floor has elapsed) retries the write,
  // rather than silently treating a failed write as done and blocking any
  // correction for up to 24h. Checks here are spaced past
  // kMinRtcWriteRetryIntervalMs so each one is actually eligible to retry
  // (Round 5 cleanup task 2's pacing is exercised separately, and more
  // aggressively, by testFinding1FailedWriteRetryIsPacedNotHotLooped()
  // below - this test is about "does it retry at all", not "how fast").
  RtcWriteBackSimulator sim;
  sim.rtcWriteSucceeds = false;

  uint32_t nowMs = 8000;
  sim.check(nowMs, 8000);
  assert(sim.rtcWriteAttemptCount == 1); // it DID try
  assert(sim.rtcWriteConfirmedCount == 0); // but did not confirm success
  assert(sim.lastTimeSyncStampCount == 0); // so lastTimeSync must NOT be stamped
  assert(sim.lastRtcWriteSyncedLastMs == 0); // and the tracked value must NOT advance

  // Next check, same lastSyncMs, write still failing, past the retry
  // floor - must retry (attempt again), not silently give up because
  // "this value was already handled".
  nowMs += (uint32_t)ClockTrust::kMinRtcWriteRetryIntervalMs;
  sim.check(nowMs, 8000);
  assert(sim.rtcWriteAttemptCount == 2);
  assert(sim.rtcWriteConfirmedCount == 0);
  assert(sim.lastTimeSyncStampCount == 0);

  // The write starts succeeding (e.g. I2C recovers) - the next eligible
  // check with the SAME lastSyncMs must now confirm and stamp exactly once.
  sim.rtcWriteSucceeds = true;
  nowMs += (uint32_t)ClockTrust::kMinRtcWriteRetryIntervalMs;
  sim.check(nowMs, 8000);
  assert(sim.rtcWriteAttemptCount == 3);
  assert(sim.rtcWriteConfirmedCount == 1);
  assert(sim.lastTimeSyncStampCount == 1);
  assert(sim.lastRtcWriteSyncedLastMs == 8000);

  // And now that it's confirmed, further checks with the same value must
  // not re-attempt at all (this function's own idempotency, per Finding 8's
  // note above) - even well past the retry floor.
  nowMs += (uint32_t)ClockTrust::kMinRtcWriteRetryIntervalMs;
  sim.check(nowMs, 8000);
  assert(sim.rtcWriteAttemptCount == 3);
  assert(sim.rtcWriteConfirmedCount == 1);
  assert(sim.lastTimeSyncStampCount == 1);
}

void testFinding1FailedWriteRetryIsPacedNotHotLooped() {
  // Round 5 cleanup task 2 (Stage 7 finding 5): a PERMANENT AB1805/Wire
  // fault must NOT retry ab1805.setRtcFromSystem() (a locked I2C
  // transaction) on every single check - in the real system, every
  // main-loop pass. This replaces the old "200 attempts in 200 checks"
  // assertion, which codified the unpaced hot loop as a requirement rather
  // than catching it as a defect.
  //
  // 200 checks spaced 10ms apart simulate a hot main loop across 2000ms of
  // total elapsed time - far less than kMinRtcWriteRetryIntervalMs (60s) -
  // so only the FIRST check should actually attempt a write; the other 199
  // must be paced out.
  RtcWriteBackSimulator sim;
  sim.rtcWriteSucceeds = false;

  uint32_t nowMs = 1000;
  for (int i = 0; i < 200; i++) {
    sim.check(nowMs, 42000);
    nowMs += 10;
  }
  assert(sim.rtcWriteAttemptCount == 1);
  assert(sim.rtcWriteConfirmedCount == 0);
  assert(sim.lastTimeSyncStampCount == 0);
  assert(sim.lastRtcWriteSyncedLastMs == 0);

  // Once the retry floor has elapsed since the LAST attempt (not since
  // boot), the next check must attempt again - paced, not permanently
  // blocked.
  nowMs = 1000 + (uint32_t)ClockTrust::kMinRtcWriteRetryIntervalMs;
  sim.check(nowMs, 42000);
  assert(sim.rtcWriteAttemptCount == 2);
  assert(sim.rtcWriteConfirmedCount == 0);
}

void testDistinctSyncAfterSuccessIsAdmittedWithinRetryFloor() {
  // Round 6 (second follow-up), Stage 7 pass 4 (HIGH), part (a): a
  // DISTINCT new confirmed sync value arriving well within
  // kMinRtcWriteRetryIntervalMs (60s) of a PRIOR SUCCESSFUL write must be
  // admitted IMMEDIATELY - the retry floor exists only to bound repeated
  // failures of the SAME value, never to delay a new one. This is the
  // exact scenario Stage 7 demonstrated broken: "second distinct sync at
  // +1s: 0 <- blocked, wrong; second distinct sync at +60s: 1".
  RtcWriteBackSimulator sim;

  // First sync, succeeds.
  sim.check(1000, 5000);
  assert(sim.rtcWriteAttemptCount == 1);
  assert(sim.rtcWriteConfirmedCount == 1);
  assert(sim.lastRtcWriteSyncedLastMs == 5000);

  // A SECOND, DISTINCT sync value arrives only 1 second later - well
  // inside the 60s floor - and must be attempted (and confirmed) on this
  // very check, not withheld.
  sim.check(1001, 6000);
  assert(sim.rtcWriteAttemptCount == 2); // <- was wrongly 1 (blocked) before this fix
  assert(sim.rtcWriteConfirmedCount == 2);
  assert(sim.lastRtcWriteSyncedLastMs == 6000);
  assert(sim.lastTimeSyncStampCount == 2);
}

void testRetryOfSameFailedValueStaysPacedWithinFloor() {
  // Round 6 (second follow-up), Stage 7 pass 4 (HIGH), part (b): the
  // inverse case must NOT regress - a retry of the SAME sync value that
  // already FAILED to write must still be paced (Stage 7 finding 5,
  // preserved). Only admission of a genuinely NEW value bypasses the
  // floor; repeating the same unwritten value does not.
  RtcWriteBackSimulator sim;
  sim.rtcWriteSucceeds = false;

  sim.check(1000, 7000);
  assert(sim.rtcWriteAttemptCount == 1); // first attempt for this value, always allowed
  assert(sim.rtcWriteConfirmedCount == 0);
  assert(sim.lastRtcWriteSyncedLastMs == 0);

  // Same value, 1 second later - still well within the 60s floor since the
  // FAILED attempt above - must NOT retry yet.
  sim.check(1001, 7000);
  assert(sim.rtcWriteAttemptCount == 1); // unchanged - paced
  assert(sim.rtcWriteConfirmedCount == 0);

  // Past the floor, same still-unwritten value - must retry.
  sim.check(1000 + (uint32_t)ClockTrust::kMinRtcWriteRetryIntervalMs, 7000);
  assert(sim.rtcWriteAttemptCount == 2);
  assert(sim.rtcWriteConfirmedCount == 0);
}

} // namespace

int main() {
  testHandshakeInitiatedSyncDrivesRtcWriteWithNoAppRequest();
  testFastCompletionRaceStillProducesExactlyOneWrite();
  testExactlyOnceWritePerAdvanceNotPerCheck();
  testNoSyncEverMeansNoWriteAndClockStaysUntrusted();
  testRetryPacingStillHonouredOnRequestSide();
  testFinding1FailedWriteIsRetriedNotPermanentlyConsumed();
  testFinding1FailedWriteRetryIsPacedNotHotLooped();
  testDistinctSyncAfterSuccessIsAdmittedWithinRetryFloor();
  testRetryOfSameFailedValueStaysPacedWithinFloor();

  printf("clock_rtc_writeback_test: all assertions passed\n");
  return 0;
}
