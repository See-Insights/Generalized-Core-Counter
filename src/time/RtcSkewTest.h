/**
 * @file RtcSkewTest.h
 * @brief Pure, dependency-free arithmetic and one-shot latch for the
 *        ENABLE_RTC_SKEW_TEST bench hook (WO-2026-08-31-003, Amendment A).
 *
 * @details Round 2 -> round 3: A.2's one-shot guard did not reliably
 *          re-arm on a fresh flash. See the "round 3" section far below,
 *          after the A.1-A.4 sections (kept verbatim from round 2, since
 *          they still hold), for the fix and why it was needed.
 *
 *          This header has zero Particle/AB1805 dependencies (no
 *          Particle.h, AB1805_RK.h, or other project headers - only
 *          <stdint.h>), so it compiles and runs directly on the host, the
 *          same pattern used by `time/ClockTrust.h` and
 *          `power/ChargeInhibitPolicy.h`.
 *
 *          ---- A.1: skew constant, and why it must be -28800s, not -48456s
 *
 *          `kSkewSeconds` is the compile-time skew magnitude applied by the
 *          bench hook (see `BuildProfile.h`'s `ENABLE_RTC_SKEW_TEST`).
 *
 *          The first implementation used -48456s (-13.46h), matching
 *          Dev-11's live *accumulated* drift at time of writing. That was
 *          the wrong reference point: it reflects THREE nights of
 *          compounding loss, not the loss from one hibernate cycle, and it
 *          is unreachable by construction against this codebase's bench
 *          step 6 ("confirm the corrected time survives the wake"):
 *
 *            `shouldUseBoronRtcAlarmHibernate()` (src/state/State_Sleep.cpp)
 *            caps any single requested hibernate at
 *            `kMaxHibernateSleepSec` = 36,000s. Bench step 6's condition,
 *            evaluated on the wake boot
 *            (`rtcTime >= retainedHibernateRtcBefore`,
 *            src/Generalized-Core-Counter.cpp), only fails if something
 *            drives the post-wake RTC reading backwards past the
 *            pre-sleep reading by MORE than the actual sleep duration
 *            added. The only way this bench hook could do that is if its
 *            one-shot guard incorrectly re-armed and fired again on the
 *            acceptance wake, reading the just-woken (already-correct) RTC
 *            as its anchor and re-applying the skew:
 *
 *              rtcTime_afterReFire = (retainedHibernateRtcBefore +
 *                                     actualSleepSec) - skewMagnitude
 *
 *            so `rtcTime_afterReFire >= retainedHibernateRtcBefore` reduces
 *            to `actualSleepSec >= skewMagnitude`. Since
 *            `actualSleepSec <= kMaxHibernateSleepSec` (36,000s) always,
 *            with `skewMagnitude = 48456` that inequality is
 *            **UNSATISFIABLE for any legal actualSleepSec** - step 6 was
 *            dead code by construction, not merely fragile. With
 *            `skewMagnitude = 28800` it IS satisfiable
 *            (e.g. `actualSleepSec = 30000 >= 28800`), and is also
 *            correctly UNsatisfiable for a short hibernate
 *            (`actualSleepSec = 900 < 28800`), so the check genuinely
 *            discriminates rather than being vacuously true.
 *            `tests/rtc_skew_test.cpp`'s
 *            `testStep6ReachabilityAgainstBothConstants()` asserts exactly
 *            this claim against both constants by computation, not
 *            assertion-by-prose - see `wouldReFiredSkewReachRetainedThreshold()`
 *            below.
 *
 *            NOTE: Amendment A.2 (below) separately fixes the one-shot
 *            guard so this re-fire scenario cannot occur in the shipped
 *            hook at all. This reachability analysis is retained here
 *            (and covered by a host test) because Stage 7 required the
 *            claim to be verified explicitly at both constants, not
 *            because the fixed hook still depends on it.
 *
 *          Revised constant: **-28,800s (8h)**, matching Dev-11's second
 *          observed SINGLE-CYCLE loss (8h29m), not its cumulative drift.
 *          Comfortably under the 36,000s cap, large enough to be
 *          unambiguous against ordinary RTC jitter, small enough that the
 *          hibernate-duration cap does not interfere with the very test it
 *          exists to exercise. It must also stay large enough in magnitude
 *          to be a meaningful reproduction of the field defect, while
 *          remaining small enough that `applySkew()`'s result still falls
 *          inside `isRtcTimeValidForHibernate()`'s accepted range
 *          (`src/state/State_Sleep.cpp`, kRtcMin=2024-01-01,
 *          kRtcMax=2035-01-01) for any plausible in-service anchor time -
 *          see `tests/rtc_skew_test.sh`, which extracts those two bounds
 *          verbatim from the real source and checks `applySkew()` against
 *          them, so this claim cannot silently drift out of sync with the
 *          real acceptance gate.
 *
 *          ---- A.2: one-shot MUST survive a reset - this is a design
 *          decision, not an accident of implementation
 *
 *          The first implementation's guard was a function-local static -
 *          reconstructed fresh on every boot, so it re-fired on every
 *          single wake. That was precisely the defect Amendment A.2 raised,
 *          not a fix for anything.
 *
 *          Required behaviour: the skew fires once on the first boot after
 *          flashing, the guard latches after that one firing, and it does
 *          not re-arm without a fresh flash or an explicit reset of the
 *          latch. `OneShotGuard` below is therefore stateless: it operates
 *          on a persisted bool supplied BY REFERENCE by the caller, rather
 *          than owning its own storage. In production
 *          (`Generalized-Core-Counter.cpp`), that persisted bool is a
 *          `retained bool` global, e.g.:
 *
 *              retained bool retainedRtcSkewTestFired = false;
 *              ...
 *              RtcSkewTest::OneShotGuard guard(retainedRtcSkewTestFired);
 *              if (guard.shouldRun()) { ... }
 *
 *          `retained` RAM survives an MCU reset - soft reset, AB1805
 *          watchdog-triggered reset, and HIBERNATE wake - as long as system
 *          power (LiPo and/or USB) is maintained throughout, which is
 *          exactly the "does not re-arm without a fresh flash or explicit
 *          reset" property A.2 requires across the bench procedure's
 *          steps 4-7. IMPORTANT: `retained` RAM does NOT survive a full
 *          power loss (LiPo AND USB both disconnected, as in bench step 1)
 *          - that clears the latch along with the AB1805 RTC. This is
 *          intentional and matches the bench procedure as written: step 1
 *          is a power-cycle that is expected to precede every fresh flash,
 *          not something performed mid-soak. If a bench operator power-
 *          cycles the device mid-soak without reflashing,
 *          `retainedRtcSkewTestFired` clears too and the hook WILL re-arm on
 *          the next boot - this is stated explicitly here so it is not
 *          silently assumed away.
 *
 *          A plain host-side `bool` (not `retained`) is used in
 *          `tests/rtc_skew_test.cpp` to exercise the same stateless-guard
 *          logic without any Particle/retained-RAM dependency; the
 *          "survives what looks like a reset, cleared by what looks like a
 *          fresh flash" behaviour is simulated by constructing a NEW guard
 *          over the SAME bool for "reset", and a fresh bool set back to
 *          false for "fresh flash".
 *
 *          ---- A.3 / A.4: RTC write only, and its return value is checked
 *
 *          Per Amendment A.3, the hook writes the AB1805 RTC only -
 *          `ab1805.setRtcFromTime()` - and does not also call
 *          `Time.setTime()`. Per A.4 (a standing rule for ANY RTC write in
 *          this codebase, not just this hook), the production call site
 *          checks `setRtcFromTime()`'s return value before treating the
 *          write as done, and logs distinctly on failure instead of
 *          silently proceeding as if the RTC now held the skewed value.
 *
 *          ---- Round 3 HIGH: A.2's guard did not reliably re-arm on a
 *          fresh flash - root cause and fix
 *
 *          Round 2's `retained bool retainedRtcSkewTestFired = false;`
 *          relies on that `= false` initializer running again on every
 *          fresh flash. It does not. Verified against Device OS 6.4.1
 *          (`wiring/src/user.cpp`):
 *
 *              backup_ram_was_valid_ = __backup_sram_signature == signature;
 *              if (!backup_ram_was_valid_) {
 *                  system_initialize_user_backup_ram();  // memcpy of
 *                                                         // initial values
 *              }
 *
 *          `signature` is a single fixed constant (0x9A271C1E), not a
 *          layout/size hash - so it is NOT invalidated by adding, removing,
 *          or resizing retained variables between builds. It is only
 *          invalidated by a FULL power loss (backup SRAM itself loses
 *          power - LiPo AND USB both disconnected, exactly bench step 1).
 *          A firmware flash performed with system power maintained leaves
 *          the signature valid, so `system_initialize_user_backup_ram()`
 *          is skipped and the retained region's PREVIOUS bytes survive
 *          untouched - including at a byte offset that previous firmware
 *          never used for this purpose, whose value is therefore whatever
 *          happened to be there (typically 0 from manufacture, but not
 *          guaranteed, and irrelevant either way - see below). If that
 *          byte reads non-zero, round 2's guard silently never fires:
 *          the bench device looks healthy, produces no skew, and the test
 *          it exists to enable cannot run - a failure invisible at the
 *          point it happens.
 *
 *          Fix: arm/disarm now depends on a second retained field - a
 *          build token - rather than solely on the fired-flag's own
 *          initializer. `armTokenIndicatesFreshFlash()` / `rearmIfBuildTokenChanged()`
 *          below compare a persisted token (originally `uint32_t`, widened
 *          to `uint64_t` by the round-4 fix described further below)
 *          against a token computed for the CURRENT build; on any
 *          mismatch, both the persisted token and the persisted fired-flag
 *          are forced back to "unfired" - unconditionally, regardless of
 *          what the fired-flag byte previously held. This makes the true
 *          fresh-flash case (persisted token proven to be that of a
 *          different build, or leftover unconstrained garbage from
 *          firmware that never wrote this field at all) self-correcting
 *          instead of dependent on an initializer that may not run.
 *
 *          In production (`Generalized-Core-Counter.cpp`), the token was
 *          originally computed (round 3) as
 *          `fnv1aHash32(__DATE__ " " __TIME__ " " __FILE__)` at the one
 *          call site, evaluated in that translation unit at COMPILE time;
 *          round 4 (below) widens this to `fnv1aHash64()` after a real
 *          32-bit collision was found. What round 3's 32-bit token
 *          guaranteed, and what it did NOT (kept here for the historical
 *          record - see "Round 4 HIGH" further below for the corrected,
 *          currently-accurate guarantee):
 *
 *            - GUARANTEES: two builds compiled at genuinely different
 *              times (i.e. any time this translation unit is recompiled -
 *              a source change to it or any header it includes, or a
 *              clean rebuild) get different tokens with overwhelming
 *              probability, so flashing a NEWLY COMPILED image - the
 *              bench procedure's actual step 2 - always re-arms,
 *              independent of whatever garbage byte round 2 depended on.
 *            - GUARANTEES: the very first flash of this feature onto a
 *              device that previously ran firmware without it also
 *              re-arms - the persisted token slot holds unconstrained
 *              prior bytes, which match the current build's token only by
 *              a 1-in-2^64 coincidence (see "Round 4 HIGH" below for why
 *              this is now 2^64, not 2^32).
 *            - DOES NOT GUARANTEE anything at compile-granularity finer
 *              than "this translation unit was recompiled": relinking an
 *              unchanged object (no recompile of this file) leaves the
 *              embedded token identical, so re-flashing the EXACT SAME
 *              compiled binary twice does not re-arm - this is correct
 *              (nothing about the firmware changed) and consistent with
 *              "does not re-arm without a fresh flash", since no new
 *              content was flashed.
 *            - DOES NOT distinguish a meaningful source change from an
 *              incidental recompile of this same file with no functional
 *              difference (e.g. `make clean && make` with no edits): both
 *              produce a new `__TIME__` and therefore re-arm. This is a
 *              conservative bias (re-arms slightly more often than
 *              strictly necessary), not a case where a real fresh flash
 *              fails to re-arm - and it is the failure direction A.2 cares
 *              about avoiding.
 *
 *          `__backup_ram_was_valid()` (Device OS) was considered as a
 *          simpler alternative and rejected: it answers "did backup RAM
 *          survive at all since the last full power loss", which is true
 *          across every reset/hibernate/reflash-with-power-held bench step
 *          2-7 use - i.e. it cannot distinguish "still the same firmware
 *          that already fired" from "freshly reflashed firmware that has
 *          never fired", which is exactly the distinction A.2 requires.
 *
 *          ---- Round 4 HIGH: 32-bit token collisions could silently
 *          suppress a legitimate re-arm - two DIFFERENT problems, only one
 *          of which is a hash-width problem
 *
 *          Stage 7 pass 3 exhibited two distinct, real
 *          `__DATE__ " " __TIME__ " " __FILE__` inputs
 *          (`"Nov 15 2030 19:22:54 ..."` and `"Feb  7 2027 09:02:16 ..."`)
 *          that both hashed to `0x45BC2355` under the round-3 32-bit
 *          `fnv1aHash32()`, and drove that pair through the real
 *          `rearmIfBuildTokenChanged()` + `OneShotGuard` to show the
 *          fired-flag staying set (`fired_after_rearm=1`) and the hook
 *          silently not re-arming (`shouldRun=0`) even though the two
 *          inputs represent a genuinely different completed compile. The
 *          round-3 header claimed both "different with overwhelming
 *          probability" AND "any recompile re-arms" - those two statements
 *          are inconsistent at 32 bits, where a 4-billion-token space makes
 *          a same-day, same-file collision between two arbitrary builds
 *          plausible over a project's lifetime (birthday-bound: ~50%
 *          chance of some collision after ~77,000 distinct compiles of
 *          this file - not an exotic count over years of iteration).
 *
 *          There are two DIFFERENT failure modes bundled in that
 *          demonstration, and only the first is fixable by widening the
 *          hash:
 *
 *            (1) DISTINCT inputs colliding in the token's bit width. This
 *                IS a hash-width problem: the token space was too small
 *                for the number of distinct inputs it needs to
 *                distinguish over the project's lifetime.
 *
 *                Fix: widen to 64-bit FNV-1a (`fnv1aHash64()` below;
 *                offset basis 14695981039346656037, prime
 *                1099511628211 - the standard FNV-1a 64-bit constants),
 *                stored in a `retained uint64_t`
 *                (`retainedRtcSkewTestArmToken` in
 *                `Generalized-Core-Counter.cpp`). This reduces the
 *                distinct-input collision probability to negligible: at
 *                64 bits, the birthday bound puts a ~50% chance of ANY
 *                collision at roughly 5 billion distinct compiles of this
 *                file, a count with no realistic path to occurring over
 *                this project's lifetime.
 *
 *            (2) TWO COMPILES WITHIN THE SAME SECOND. This is NOT a
 *                hash-width problem and widening the hash does NOT fix
 *                it: `__DATE__ " " __TIME__ " " __FILE__` is
 *                BYTE-IDENTICAL for two compiles of this file that
 *                complete within the same wall-clock second (`__TIME__`
 *                has 1-second resolution), so ANY pure, deterministic
 *                function of that byte-identical input - a 32-bit hash, a
 *                64-bit hash, or any wider one - returns the SAME token
 *                for both, by definition. No hash width closes this gap.
 *
 *                This is accepted as a documented residual limitation,
 *                not fixed by construction, for a concrete practical
 *                reason: a full Boron firmware build via
 *                `make -f .../Makefile compile-user` (the mandatory build
 *                method this WO's bench procedure and Stage 7 both use -
 *                see `AI_DEVELOPMENT_WORKFLOW.md`'s "Verifying
 *                compile-time flags") takes on the order of MINUTES, not
 *                sub-second, to complete. Two genuinely distinct, fully
 *                completed builds of this firmware landing within the
 *                same wall-clock second is not a scenario the actual bench
 *                workflow can produce; it would require two independent
 *                toolchain invocations racing to finish compiling this
 *                specific translation unit inside a 1-second window, which
 *                does not happen with this build system in practice.
 *
 *          Revised guarantee, matching the mechanism's actual behaviour
 *          (this replaces the "overwhelming probability" wording above,
 *          which was accurate in spirit but not precise about WHICH
 *          probability and WHICH failure mode it did not cover):
 *
 *            "Two builds of this translation unit that complete in
 *            DIFFERENT wall-clock seconds get different 64-bit tokens
 *            with overwhelming probability (collision chance negligible
 *            below billions of distinct compiles). Two builds that
 *            complete within the SAME wall-clock second are
 *            indistinguishable by this mechanism and will NOT re-arm each
 *            other - this is a residual limitation, not eliminated by the
 *            64-bit widening, and is accepted because a real Boron
 *            firmware build takes minutes, making same-second distinct
 *            completed builds unreachable in this project's actual build
 *            workflow."
 *
 *          `tests/rtc_skew_test.cpp`'s
 *          `testRearmFiresOnDistinctTokensDirectly()` asserts the re-arm
 *          decision (`rearmIfBuildTokenChanged()` + `OneShotGuard`) against
 *          literal, hand-chosen distinct 64-bit token values directly -
 *          not by hashing two strings and assuming the hash is injective,
 *          since the entire point of this fix is that the hash is NOT
 *          assumed injective; the mechanism must be correct for any two
 *          distinct token values, by construction of
 *          `armTokenIndicatesFreshFlash()`'s `!=` comparison, independent
 *          of how those tokens were produced.
 */

#ifndef __RTC_SKEW_TEST_H
#define __RTC_SKEW_TEST_H

#include <stdint.h>

namespace RtcSkewTest {

// Deliberate skew applied to the RTC by the bench hook, in seconds.
// Negative = RTC set backwards in time. -28800s = -8h, matching Dev-11's
// second observed SINGLE-CYCLE field loss (8h29m) - see the file-level
// doc comment above (A.1) for why this replaced -48456s and the
// reachability proof for both constants.
const int32_t kSkewSeconds = -28800;

// A safe, in-range anchor to skew from when the AB1805's own pre-hook
// reading is not itself a sane basis (e.g. a power-cycled/unset RTC
// returning zero or otherwise implausible garbage). 2026-01-01 00:00:00
// UTC - comfortably inside isRtcTimeValidForHibernate()'s accepted range
// both before and after kSkewSeconds is applied.
const int64_t kFallbackAnchor = 1767225600;

// Below this, a raw AB1805 reading is treated as not sane enough to anchor
// the skew from (2020-01-01 00:00:00 UTC - well before this device's
// earliest possible valid deployment, but well above zero/garbage).
const int64_t kSaneReadingFloor = 1577836800;

// Applies the compile-time skew to an RTC-style time_t reading. Pure
// arithmetic; the caller is responsible for picking a sane `anchor` (see
// kFallbackAnchor/kSaneReadingFloor above).
inline int64_t applySkew(int64_t anchor) {
  return anchor + kSkewSeconds;
}

// Picks the anchor to skew from: the raw pre-hook AB1805 reading if it
// looks sane, otherwise the fixed fallback. Kept separate from applySkew()
// so both halves are independently host-testable.
inline int64_t chooseAnchor(int64_t rawReading, bool readOk) {
  if (readOk && rawReading >= kSaneReadingFloor) {
    return rawReading;
  }
  return kFallbackAnchor;
}

// A.1 verification helper (NOT used by production code - the fixed
// OneShotGuard below does not re-fire, so this scenario cannot occur in
// the shipped hook). Models what bench step 6's acceptance condition
// (`rtcTime >= retainedHibernateRtcBefore`, evaluated on the wake boot in
// Generalized-Core-Counter.cpp) would evaluate to IF the one-shot guard
// incorrectly re-armed and re-fired on the acceptance wake, reading the
// just-woken (already-correct) RTC as its anchor and re-applying the skew.
// See the file-level doc comment's A.1 section for the full derivation:
// this reduces to `actualSleepSec >= -skewSeconds`.
//
// Exists purely so the Amendment-A.1-required "verify this explicitly
// against both constants; do not assume it" claim is backed by an
// assertion instead of prose - see tests/rtc_skew_test.cpp.
inline bool wouldReFiredSkewReachRetainedThreshold(uint32_t actualSleepSec, int32_t skewSeconds) {
  return (int64_t)actualSleepSec + (int64_t)skewSeconds >= 0;
}

// Stateless one-shot latch (A.2). This class owns NO storage of its own -
// it operates on a persisted bool supplied by reference, so "survives a
// reset" is a property of whatever storage the CALLER provides (a
// `retained bool` in production; see the file-level doc comment's A.2
// section for why that satisfies "does not re-arm without a fresh flash or
// an explicit reset", and what it does NOT survive).
class OneShotGuard {
public:
  explicit OneShotGuard(bool &persistedHasFiredFlag) : hasFiredFlag_(persistedHasFiredFlag) {}

  // Returns true exactly once for a given persisted flag's lifetime - i.e.
  // from whenever that flag was last false (a fresh flash, or an explicit
  // reset of the flag) until it is next cleared. Every other call, on this
  // guard instance OR a new instance over the same flag (e.g. a fresh
  // OneShotGuard constructed on the very next boot, over the same
  // `retained bool`), returns false.
  bool shouldRun() {
    if (hasFiredFlag_) {
      return false;
    }
    hasFiredFlag_ = true;
    return true;
  }

  bool hasFired() const {
    return hasFiredFlag_;
  }

private:
  bool &hasFiredFlag_;
};

// ---- Round 3 HIGH fix: build-token arming, so a fresh flash re-arms the
// guard above even when the persisted fired-flag's OWN storage did not get
// re-initialized. See the file-level doc comment's "Round 3 HIGH" section
// for the full root-cause analysis and what this construction does and
// does not guarantee.

// Deterministic, pure FNV-1a 64-bit hash. Used (only at the one call site
// in Generalized-Core-Counter.cpp) to turn `__DATE__ " " __TIME__ " "
// __FILE__` into a compact build token; kept here, not there, purely so it
// is host-testable without any Particle dependency, and so its
// determinism (same input -> same output, different input -> different
// output with overwhelming probability - see the file-level doc comment's
// "Round 4 HIGH" section for exactly what that probability claim does and
// does not cover) can be asserted directly.
//
// Round 4 HIGH fix: widened from 32 to 64 bits after Stage 7 demonstrated
// two distinct real build-identity strings colliding at 32 bits and
// silently suppressing a legitimate re-arm. 64 bits reduces DISTINCT-INPUT
// collisions to negligible probability; it does NOT and CANNOT make two
// compiles completed within the same wall-clock second produce different
// tokens, since the input itself is byte-identical in that case - see the
// file-level doc comment for why that is accepted as a residual
// limitation rather than fixed.
inline uint64_t fnv1aHash64(const char *str) {
  uint64_t hash = 14695981039346656037ull; // FNV-1a 64-bit offset basis
  while (*str != '\0') {
    hash ^= (uint64_t)(unsigned char)(*str);
    hash *= 1099511628211ull; // FNV-1a 64-bit prime
    ++str;
  }
  return hash;
}

// True when the persisted arm-token does not match the CURRENT build's
// token - i.e. this is either firmware freshly flashed with different
// content from whatever last wrote that persisted token, or a persisted
// slot that some earlier, token-unaware firmware left in an unconstrained
// state. Pure predicate, no I/O, no mutation. Correct for ANY two distinct
// uint64_t values regardless of how they were produced - it does not rely
// on fnv1aHash64() being injective, only on `!=` between the two stored
// values (see testRearmFiresOnDistinctTokensDirectly() in
// tests/rtc_skew_test.cpp, which asserts this directly against literal
// distinct tokens rather than assuming the hash cannot collide).
inline bool armTokenIndicatesFreshFlash(uint64_t persistedToken, uint64_t currentBuildToken) {
  return persistedToken != currentBuildToken;
}

// Call once, early, before constructing OneShotGuard over the same
// persisted fired-flag. On a build-token mismatch (see
// armTokenIndicatesFreshFlash() above), UNCONDITIONALLY forces the
// persisted fired-flag back to "not fired" and stores the current build's
// token - regardless of whatever garbage value the fired-flag previously
// held. This is what makes arming self-correcting instead of dependent on
// the fired-flag's own `retained` initializer running (round 2's defect -
// see the file-level doc comment). A no-op when the token already
// matches (same firmware, an ordinary reset/hibernate wake/reboot).
inline void rearmIfBuildTokenChanged(uint64_t &persistedToken, bool &persistedHasFiredFlag,
                                      uint64_t currentBuildToken) {
  if (armTokenIndicatesFreshFlash(persistedToken, currentBuildToken)) {
    persistedToken = currentBuildToken;
    persistedHasFiredFlag = false;
  }
}

} // namespace RtcSkewTest

#endif
