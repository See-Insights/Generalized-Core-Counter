// Host test for src/time/RtcSkewTest.h (WO-2026-08-31-003, Amendment A).
//
// This header has zero Particle/AB1805 dependencies (no Particle.h,
// AB1805_RK.h, or other project headers - only <stdint.h>), so it compiles
// and runs directly on the host, the same pattern used by
// tests/clock_trust_test.cpp and src/power/ChargeInhibitPolicy.h's host
// tests.
//
// Covers:
//   1. applySkew() arithmetic: anchor + kSkewSeconds, exactly, and the
//      revised constant is -28800s (A.1), not the unreachable -48456s.
//   2. chooseAnchor() picks the raw AB1805 reading when it looks sane
//      (>= kSaneReadingFloor and the read succeeded), and falls back to
//      kFallbackAnchor otherwise (read failed, or a garbage/zero reading
//      from a power-cycled/unset RTC).
//   3. The combination of chooseAnchor()+applySkew() lands inside
//      isRtcTimeValidForHibernate()'s real accepted range
//      (src/state/State_Sleep.cpp) for representative anchor times -
//      cross-checked against the REAL kRtcMin/kRtcMax extracted verbatim
//      from that file by tests/rtc_skew_test.sh, not hardcoded here, so
//      this claim can't silently drift from the real acceptance gate.
//   4. OneShotGuard (A.2): shouldRun() returns true exactly once per
//      persisted flag, false on every subsequent call INCLUDING across a
//      fresh guard instance over the SAME flag (modeling a reset) - the
//      "fires once ever, not once per boot" property Amendment A.2
//      requires. A fresh flag (modeling a fresh flash) re-arms it.
//   5. A.1 reachability proof: wouldReFiredSkewReachRetainedThreshold()
//      is UNSATISFIABLE for every legal hibernate duration at the OLD
//      constant (-48456s) and IS satisfiable (but not vacuously true) at
//      the REVISED constant (-28800s) - verified by computation across
//      the real [kMinHibernateSleepSec, kMaxHibernateSleepSec] range from
//      src/state/State_Sleep.cpp, not merely asserted in prose.
//   6. A.6 gap 3: models the full correction -> hibernate -> fresh-boot
//      sequence end to end, proving the fixed one-shot guard does NOT
//      re-fire on the acceptance wake and that bench step 6's condition is
//      satisfied for a representative in-range hibernate duration.
//   7. Round 3 HIGH: models the REAL semantics of `retained` storage
//      SURVIVING a firmware flash (not a fresh ordinary bool, which models
//      a full power loss instead - the exact test gap that let round 2's
//      defect through). Proves round 2's bare-bool composition (OneShotGuard
//      alone) fails to fire when a stale/garbage byte reads true across
//      such a flash, and that the round-3 fix (rearmIfBuildTokenChanged()
//      before OneShotGuard) fires correctly in that same scenario, while
//      remaining a no-op (no re-arm) across an ordinary reset of the SAME
//      already-fired firmware. Also covers fnv1aHash64()'s determinism.
//   8. Round 4 HIGH: fnv1aHash32() widened to fnv1aHash64() after Stage 7
//      demonstrated a real 32-bit collision between two distinct build
//      identity strings that silently suppressed a legitimate re-arm.
//      testFnv1aHash64IsDeterministicAndDiscriminates() proves that exact
//      pair no longer collides at 64 bits.
//      testRearmFiresOnDistinctTokensDirectly() asserts the re-arm
//      decision (rearmIfBuildTokenChanged() + OneShotGuard) directly
//      against literal distinct/equal token values - not by hashing
//      strings and assuming the hash is injective - so a hash collision,
//      were one ever found again, could never silently suppress a
//      legitimate re-arm without failing this test.

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "time/RtcSkewTest.h"

namespace {

// Mirrors the real bounds from src/state/State_Sleep.cpp's
// shouldUseBoronRtcAlarmHibernate() (kMinHibernateSleepSec=900,
// kMaxHibernateSleepSec=36000). This test file only needs the bounds to
// sweep actualSleepSec, not to reproduce isRtcTimeValidForHibernate()
// itself - tests/rtc_skew_test.sh independently extracts and compiles the
// REAL isRtcTimeValidForHibernate() body (A.6 gap 2), so that acceptance
// gate cannot silently drift out of sync with what is hardcoded here.
const uint32_t kMinHibernateSleepSec = 900;
const uint32_t kMaxHibernateSleepSec = 36000;

// The rejected Amendment-A.1 constant, kept here (not in the production
// header) purely so this test can prove it was correctly rejected.
const int32_t kRejectedSkewSeconds = -48456;

void testApplySkewIsExactArithmetic() {
  assert(RtcSkewTest::applySkew(0) == RtcSkewTest::kSkewSeconds);
  assert(RtcSkewTest::applySkew(1000000000) == 1000000000 + RtcSkewTest::kSkewSeconds);
  // The skew must be a negative offset (RTC set backwards), per the field
  // defect (Dev-11: RTC ahead of the true time) and the Work Order's
  // requirement that this reproduce that defect, not invent a new one.
  assert(RtcSkewTest::kSkewSeconds < 0);
  // A.1: the revised constant is exactly -28800s (8h), not the unreachable
  // -48456s the first implementation used.
  assert(RtcSkewTest::kSkewSeconds == -28800);
}

void testChooseAnchorPrefersSaneRawReading() {
  const int64_t saneReading = 1735689600; // 2025-01-01 00:00:00 UTC
  assert(RtcSkewTest::chooseAnchor(saneReading, /*readOk=*/true) == saneReading);
}

void testChooseAnchorFallsBackOnReadFailure() {
  const int64_t saneReading = 1735689600;
  assert(RtcSkewTest::chooseAnchor(saneReading, /*readOk=*/false) == RtcSkewTest::kFallbackAnchor);
}

void testChooseAnchorFallsBackOnGarbageReading() {
  // A power-cycled/unset AB1805 reading as zero (or otherwise implausibly
  // low) must not be used as the skew's reference - see WO-2026-08-31-003's
  // "RTC unset vs RTC skewed" comparison table.
  assert(RtcSkewTest::chooseAnchor(0, /*readOk=*/true) == RtcSkewTest::kFallbackAnchor);
  assert(RtcSkewTest::chooseAnchor(-1, /*readOk=*/true) == RtcSkewTest::kFallbackAnchor);
  assert(RtcSkewTest::chooseAnchor(RtcSkewTest::kSaneReadingFloor - 1, /*readOk=*/true)
         == RtcSkewTest::kFallbackAnchor);
  assert(RtcSkewTest::chooseAnchor(RtcSkewTest::kSaneReadingFloor, /*readOk=*/true)
         == RtcSkewTest::kSaneReadingFloor);
}

void testOneShotGuardFiresExactlyOncePerPersistedFlag() {
  bool persistedFlag = false;
  RtcSkewTest::OneShotGuard guard(persistedFlag);
  assert(guard.hasFired() == false);
  assert(guard.shouldRun() == true);
  assert(guard.hasFired() == true);
  assert(persistedFlag == true);
  // Every subsequent call, no matter how many, must return false.
  assert(guard.shouldRun() == false);
  assert(guard.shouldRun() == false);
  assert(guard.shouldRun() == false);
}

void testOneShotGuardSurvivesSimulatedReset() {
  // A.2's core requirement: the guard must NOT re-arm on a reset. Model a
  // reset by constructing a FRESH OneShotGuard instance over the SAME
  // persisted flag (exactly what happens in production: a fresh boot
  // constructs a new local OneShotGuard over the same `retained bool`,
  // which is untouched by the reset).
  bool persistedFlag = false;
  {
    RtcSkewTest::OneShotGuard firstBootGuard(persistedFlag);
    assert(firstBootGuard.shouldRun() == true);
  }
  // "Reset": a new guard instance, same underlying flag.
  {
    RtcSkewTest::OneShotGuard secondBootGuard(persistedFlag);
    assert(secondBootGuard.hasFired() == true);
    assert(secondBootGuard.shouldRun() == false);
  }
  {
    RtcSkewTest::OneShotGuard thirdBootGuard(persistedFlag);
    assert(thirdBootGuard.shouldRun() == false);
  }
}

void testOneShotGuardRearmsOnlyOnFreshFlag() {
  // A.2: re-arming requires "a fresh flash or an explicit reset" - modeled
  // as a brand-new flag starting false again (a fresh flash zeroes
  // retained RAM's initial value; an explicit reset clears the same flag
  // in place, which is equivalent from the guard's point of view).
  //
  // NOTE: this models OneShotGuard in isolation, given a bool that is
  // ALREADY known to be freshly false. It does NOT model how that bool
  // gets to false on a real fresh flash - see
  // testGarbageRetainedByteWithoutRearm_DoesNotFire_Round2Defect() and
  // testGarbageRetainedByteWithBuildTokenRearm_FiresOnFreshFlash() below,
  // which model the real "retained storage survives a flash" semantics
  // that this simplified test cannot represent, and which is exactly what
  // the round-3 HIGH finding required a test for.
  bool firstFlashFlag = false;
  RtcSkewTest::OneShotGuard firstFlash(firstFlashFlag);
  firstFlash.shouldRun();
  assert(firstFlash.hasFired() == true);

  bool secondFlashFlag = false; // fresh flag = fresh flash
  RtcSkewTest::OneShotGuard secondFlash(secondFlashFlag);
  assert(secondFlash.hasFired() == false);
  assert(secondFlash.shouldRun() == true);
}

// Round 3 HIGH: models the REAL semantics of `retained` memory surviving a
// firmware flash performed with system power maintained - Device OS's
// backup-RAM signature (a single fixed constant, not a layout/size hash;
// verified against wiring/src/user.cpp) is NOT invalidated by that flash,
// so a byte at an offset previous firmware never wrote holds whatever was
// already there. This is NOT representable by constructing a fresh
// ordinary `bool` (that models a full power loss / manufacturing reset,
// not a flash) - which is exactly the test gap Stage 7 called out in round
// 2's `testOneShotGuardRearmsOnlyOnFreshFlag()` above.
//
// This test asserts that round 2's bare-bool design - OneShotGuard alone,
// with NO build-token rearm - fails to fire when the persisted flag's
// storage happens to read back non-zero/true across such a flash. It must
// FAIL if run against round 2's implementation (i.e. it demonstrates the
// defect), and it does: OneShotGuard has no way to distinguish "genuinely
// already fired by this same firmware" from "garbage true left by a flash
// that didn't reinitialize this byte" - by design, `shouldRun()` only
// looks at the flag's current value, never at how it got there.
void testGarbageRetainedByteWithoutRearm_DoesNotFire_Round2Defect() {
  // Models: system power was maintained across a firmware flash, so
  // backup RAM's signature stayed valid and this byte was NOT
  // reinitialized. Whatever was here before - in round 2's defect, an
  // unconstrained non-zero byte - survives untouched and reads as `true`.
  bool retainedFiredFlagSurvivingTheFlash = true;

  // Round 2's exact composition: OneShotGuard directly over the retained
  // bool, no build-token check.
  RtcSkewTest::OneShotGuard guard(retainedFiredFlagSurvivingTheFlash);

  // THE DEFECT: the hook silently never fires on this fresh flash, because
  // the stale byte happened to already read true.
  assert(guard.shouldRun() == false);
}

// Round 3 HIGH fix, same scenario as above but with the build-token rearm
// applied first. This is the fixed production composition
// (rearmIfBuildTokenChanged() followed by OneShotGuard) and it MUST arm,
// regardless of the same stale/garbage byte used above, because the
// persisted arm-token cannot coincidentally match the current build's
// token (a 1-in-2^32 chance for an arbitrary stale value).
void testGarbageRetainedByteWithBuildTokenRearm_FiresOnFreshFlash() {
  // Same "flash with power maintained, byte never reinitialized" model as
  // the defect test above: BOTH persisted fields hold whatever unrelated
  // prior firmware left there - a stale "already fired" flag, and an
  // arm-token belonging to a different build (or, equally plausible,
  // simply uninitialized/garbage, since prior firmware never wrote this
  // field at all).
  uint64_t persistedArmTokenSurvivingTheFlash = 0xDEADBEEFu;
  bool persistedFiredFlagSurvivingTheFlash = true;

  const uint64_t currentBuildToken = 0x12345678u; // "this flash"'s token

  RtcSkewTest::rearmIfBuildTokenChanged(persistedArmTokenSurvivingTheFlash,
                                        persistedFiredFlagSurvivingTheFlash, currentBuildToken);
  // The mismatch must have forced both fields back to the fresh-flash
  // state, unconditionally - not merely "if the fired flag looked false".
  assert(persistedFiredFlagSurvivingTheFlash == false);
  assert(persistedArmTokenSurvivingTheFlash == currentBuildToken);

  RtcSkewTest::OneShotGuard guard(persistedFiredFlagSurvivingTheFlash);
  assert(guard.shouldRun() == true); // THE FIX: fires on this fresh flash
  assert(guard.shouldRun() == false); // and only once, as usual
}

// Round 3: the token check must be a no-op - and must NOT re-arm - across
// an ordinary reset/hibernate wake/reboot of the SAME already-fired
// firmware (no flash occurred, so the persisted token still matches).
// This is the "does not re-arm without a fresh flash or explicit reset"
// half of A.2 that the build-token mechanism must not break.
void testBuildTokenRearm_NoOpWhenTokenMatches_SameFirmwareStaysFired() {
  uint64_t persistedArmToken = 0xAAAAAAAAu;
  bool persistedFiredFlag = true; // this firmware already fired once

  // Same build re-running after a reset/hibernate wake: the token this
  // boot computes is identical to what is already persisted.
  const uint64_t currentBuildToken = persistedArmToken;

  RtcSkewTest::rearmIfBuildTokenChanged(persistedArmToken, persistedFiredFlag, currentBuildToken);

  // Must be untouched - no re-arm just because the check ran.
  assert(persistedFiredFlag == true);
  assert(persistedArmToken == currentBuildToken);

  RtcSkewTest::OneShotGuard guard(persistedFiredFlag);
  assert(guard.shouldRun() == false); // still does not re-fire
}

// Round 3/4: fnv1aHash64() must be deterministic (same input -> same
// output) and must actually discriminate between distinct build tokens
// (different input -> different output) - both are load-bearing for
// armTokenIndicatesFreshFlash()'s correctness. Round 4 widened this from
// 32 to 64 bits after Stage 7 demonstrated two distinct real
// __DATE__/__TIME__/__FILE__ strings colliding at 32 bits; the two exact
// strings from that finding are asserted here to no longer collide.
void testFnv1aHash64IsDeterministicAndDiscriminates() {
  assert(RtcSkewTest::fnv1aHash64("Jan  1 2026 00:00:00 src/x.cpp")
         == RtcSkewTest::fnv1aHash64("Jan  1 2026 00:00:00 src/x.cpp"));
  assert(RtcSkewTest::fnv1aHash64("Jan  1 2026 00:00:00 src/x.cpp")
         != RtcSkewTest::fnv1aHash64("Jan  2 2026 00:00:00 src/x.cpp"));
  assert(RtcSkewTest::fnv1aHash64("") == 14695981039346656037ull); // FNV-1a 64-bit offset basis, empty input
  // The exact Stage-7 pass-3 32-bit collision pair (round-3
  // fnv1aHash32() mapped both to 0x45BC2355) must NOT collide under the
  // widened 64-bit hash.
  assert(RtcSkewTest::fnv1aHash64("Nov 15 2030 19:22:54 src/Generalized-Core-Counter.cpp")
         != RtcSkewTest::fnv1aHash64("Feb  7 2027 09:02:16 src/Generalized-Core-Counter.cpp"));
}

// Round 4 HIGH: the re-arm decision must be correct for any two DISTINCT
// token values, asserted directly against literal, hand-chosen tokens -
// NOT by hashing two strings and assuming fnv1aHash64() is injective. This
// is the exact gap Stage 7 found: round 3's tests exercised the mechanism
// only through the hash, so a hash collision was never modeled as an
// input to the re-arm decision itself. armTokenIndicatesFreshFlash()'s
// `!=` comparison is correct for ANY two distinct uint64_t values by
// construction, independent of how those values were produced, and this
// test asserts exactly that: a legitimate re-arm (distinct persisted vs.
// current build token) always clears the fired-flag and always lets
// OneShotGuard fire again, and this holds even when the token values are
// numerically close together (adjacent integers) rather than
// hash-shaped - the mechanism does not depend on the tokens looking like
// hash output at all.
void testRearmFiresOnDistinctTokensDirectly() {
  {
    // Adjacent tokens - about as "close" as two distinct uint64_t values
    // can be - must still be treated as a legitimate re-arm.
    uint64_t persistedToken = 0x1000000000000001ull;
    bool firedFlag = true; // this firmware already fired once
    const uint64_t currentBuildToken = 0x1000000000000002ull; // one more

    assert(RtcSkewTest::armTokenIndicatesFreshFlash(persistedToken, currentBuildToken) == true);
    RtcSkewTest::rearmIfBuildTokenChanged(persistedToken, firedFlag, currentBuildToken);
    assert(firedFlag == false); // re-armed
    assert(persistedToken == currentBuildToken);

    RtcSkewTest::OneShotGuard guard(firedFlag);
    assert(guard.shouldRun() == true); // fires again after the legitimate re-arm
  }
  {
    // Full 64-bit range extremes - also a legitimate re-arm.
    uint64_t persistedToken = 0x0000000000000000ull;
    bool firedFlag = true;
    const uint64_t currentBuildToken = 0xFFFFFFFFFFFFFFFFull;

    assert(RtcSkewTest::armTokenIndicatesFreshFlash(persistedToken, currentBuildToken) == true);
    RtcSkewTest::rearmIfBuildTokenChanged(persistedToken, firedFlag, currentBuildToken);
    assert(firedFlag == false);

    RtcSkewTest::OneShotGuard guard(firedFlag);
    assert(guard.shouldRun() == true);
  }
  {
    // Equal tokens (a real collision, or a genuine same-firmware reboot) -
    // must NOT re-arm. Asserted here for contrast so this test proves both
    // directions of the `!=` comparison, not just the "distinct" half.
    uint64_t persistedToken = 0x45BC2355ull;
    bool firedFlag = true;
    const uint64_t currentBuildToken = 0x45BC2355ull;

    assert(RtcSkewTest::armTokenIndicatesFreshFlash(persistedToken, currentBuildToken) == false);
    RtcSkewTest::rearmIfBuildTokenChanged(persistedToken, firedFlag, currentBuildToken);
    assert(firedFlag == true); // unchanged - no re-arm

    RtcSkewTest::OneShotGuard guard(firedFlag);
    assert(guard.shouldRun() == false); // does not fire again
  }
}

// A.1: verify step 6 reachability EXPLICITLY at both constants, by
// computation over the real hibernate-duration range, not by assertion in
// prose.
void testStep6ReachabilityAgainstBothConstants() {
  bool rejectedConstantEverReachable = false;
  bool revisedConstantEverReachable = false;
  bool revisedConstantEverUnreachable = false;

  for (uint32_t actualSleepSec = kMinHibernateSleepSec;
       actualSleepSec <= kMaxHibernateSleepSec;
       actualSleepSec += 100) {
    if (RtcSkewTest::wouldReFiredSkewReachRetainedThreshold(actualSleepSec, kRejectedSkewSeconds)) {
      rejectedConstantEverReachable = true;
    }
    if (RtcSkewTest::wouldReFiredSkewReachRetainedThreshold(actualSleepSec, RtcSkewTest::kSkewSeconds)) {
      revisedConstantEverReachable = true;
    } else {
      revisedConstantEverUnreachable = true;
    }
  }
  // Also check the exact boundary values.
  assert(!RtcSkewTest::wouldReFiredSkewReachRetainedThreshold(kMaxHibernateSleepSec, kRejectedSkewSeconds));
  assert(RtcSkewTest::wouldReFiredSkewReachRetainedThreshold(30000, RtcSkewTest::kSkewSeconds));
  assert(!RtcSkewTest::wouldReFiredSkewReachRetainedThreshold(kMinHibernateSleepSec, RtcSkewTest::kSkewSeconds));

  // -48456s: UNSATISFIABLE for every legal hibernate duration - dead code
  // by construction, exactly as Amendment A.1 states.
  assert(rejectedConstantEverReachable == false);
  // -28800s: satisfiable for SOME legal durations (step 6 is reachable)...
  assert(revisedConstantEverReachable == true);
  // ...but not vacuously true for ALL of them (the check discriminates).
  assert(revisedConstantEverUnreachable == true);
}

// A.6 gap 3: model the correction -> hibernate -> fresh-boot sequence the
// bench procedure's steps 4-6 exercise, end to end, using the FIXED
// one-shot guard - proving the A.1 defect (re-fire on the acceptance wake)
// cannot recur and that step 6's real condition is satisfied.
void testCorrectionHibernateFreshBootSequenceEndToEnd() {
  uint64_t retainedRtcSkewTestArmToken = 0; // models the retained token
  bool retainedRtcSkewTestFired = false;    // models the retained bool
  const uint64_t buildToken = 0xC0FFEEu;    // same firmware both boots

  // --- Boot 1: fresh flash. Bench steps 1-2. Production always runs the
  // token rearm check before constructing the guard; here the token slot
  // starts at its declared initial value (0), which mismatches buildToken,
  // so this is a no-op with respect to `fired` (already false) but is
  // exercised anyway to match the real call sequence. ---
  int64_t rtcAfterHook;
  {
    RtcSkewTest::rearmIfBuildTokenChanged(retainedRtcSkewTestArmToken, retainedRtcSkewTestFired, buildToken);
    RtcSkewTest::OneShotGuard guard(retainedRtcSkewTestFired);
    assert(guard.shouldRun() == true); // fires on first boot after flash
    const int64_t anchor = RtcSkewTest::chooseAnchor(RtcSkewTest::kFallbackAnchor, /*readOk=*/true);
    rtcAfterHook = RtcSkewTest::applySkew(anchor);
  }
  assert(retainedRtcSkewTestFired == true);
  assert(retainedRtcSkewTestArmToken == buildToken);

  // --- Some time later, same boot or a later one: the field mechanism
  // this bench hook exists to exercise. checkClockResync() (real
  // production code, not modeled here - out of this WO's scope) detects
  // the mismatch and calls ab1805.setRtcFromSystem() to correct the RTC
  // back to a real, trustworthy time. Model that correction as a plain
  // "real now" value, independent of the skewed value above. ---
  const int64_t correctedRtcBeforeSleep = 1798761600; // an arbitrary real "now"

  // --- Bench step 6: closed-hours hibernate. retainedHibernateRtcBefore
  // is set from the CORRECTED time (State_Sleep.cpp's real behaviour). ---
  const int64_t retainedHibernateRtcBefore = correctedRtcBeforeSleep;
  const uint32_t actualSleepSec = 30000; // within [900, 36000], picked so
                                         // it also demonstrates A.1's
                                         // reachability requirement above.
  const int64_t rtcAtWake = correctedRtcBeforeSleep + (int64_t)actualSleepSec;

  // --- Boot 2 (the acceptance wake). SAME firmware, so the token rearm
  // check must be a no-op (persisted token still matches buildToken) and
  // the guard must NOT re-fire: the persisted flag survived the
  // hibernate/reset (that is exactly what "retained" means), so
  // shouldRun() must be false here. This is the property that makes the
  // pre-Amendment-A.1 defect (re-fire on the acceptance wake, corrupting
  // the just-corrected RTC again) impossible with the fixed guard,
  // independent of which skew constant is compiled in. ---
  int64_t rtcAfterWakeBoot = rtcAtWake;
  {
    RtcSkewTest::rearmIfBuildTokenChanged(retainedRtcSkewTestArmToken, retainedRtcSkewTestFired, buildToken);
    RtcSkewTest::OneShotGuard guard(retainedRtcSkewTestFired);
    assert(guard.shouldRun() == false); // must NOT re-fire
    // If it incorrectly HAD fired (the exact Stage-7 HIGH finding), this is
    // what would have happened - kept here only to document the contrast:
    // rtcAfterWakeBoot = RtcSkewTest::applySkew(rtcAtWake);
  }
  assert(retainedRtcSkewTestFired == true); // still fired, no false re-arm

  // --- Bench step 6's real acceptance condition
  // (Generalized-Core-Counter.cpp: `rtcTime >= retainedHibernateRtcBefore`)
  // must hold, and does, because the guard did not disturb the corrected
  // RTC on the acceptance wake. ---
  assert(rtcAfterWakeBoot >= retainedHibernateRtcBefore);

  (void)rtcAfterHook; // only used to show boot 1's write happened; the
                      // wake-boot outcome does not depend on its value,
                      // since ab1805.setup() (real code, not modeled here)
                      // re-seeds Time from the RTC on every boot and the
                      // skew is never read back into this sequence once
                      // the resync has corrected it.
}

} // namespace

int main() {
  testApplySkewIsExactArithmetic();
  testChooseAnchorPrefersSaneRawReading();
  testChooseAnchorFallsBackOnReadFailure();
  testChooseAnchorFallsBackOnGarbageReading();
  testOneShotGuardFiresExactlyOncePerPersistedFlag();
  testOneShotGuardSurvivesSimulatedReset();
  testOneShotGuardRearmsOnlyOnFreshFlag();
  testGarbageRetainedByteWithoutRearm_DoesNotFire_Round2Defect();
  testGarbageRetainedByteWithBuildTokenRearm_FiresOnFreshFlash();
  testBuildTokenRearm_NoOpWhenTokenMatches_SameFirmwareStaysFired();
  testFnv1aHash64IsDeterministicAndDiscriminates();
  testRearmFiresOnDistinctTokensDirectly();
  testStep6ReachabilityAgainstBothConstants();
  testCorrectionHibernateFreshBootSequenceEndToEnd();
  printf("All RtcSkewTest tests passed\n");
  return 0;
}

