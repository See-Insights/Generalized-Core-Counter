/**
 * @file Clock.h
 * @brief Single owner of "what time is it, and can it be trusted" -
 *        open-hours evaluation, the resync/RTC-write-back cycle, and the
 *        sync-recency trust signal.
 *
 * @details WO-2026-09-18 Step 3a of the Structural Ownership Map roadmap
 *          (`docs/architecture-review-2026-09-03.md`'s "Next session -
 *          agreed direction" item 1: "Name the owner... a consolidated
 *          `time/` component"). Before this step, `isWithinOpenHours()`,
 *          `isWithinOpenHoursAt()`, `secondsUntilNextOpen()`,
 *          `checkClockResync()`/`requestClockResync()`, `isClockTrusted()`,
 *          and `reportedSyncAgeMs()` were all defined directly inside
 *          `Generalized-Core-Counter.cpp`, callable only because every
 *          consumer happened to link into the same binary - no single file
 *          could answer "what time is it after this wake" on its own.
 *
 *          This is a RELOCATION, not a redesign: every function below kept
 *          its existing global-scope name and signature so every existing
 *          call site across the tree (every `.cpp` file under `state/`, `reporting/`,
 *          `cloud/`, `diagnostics/`) keeps working unchanged - only the
 *          `.cpp` file that defines them moved. `time/ClockTrust.h` (the
 *          pure, dependency-free trust-gate math these functions call into)
 *          is unchanged and continues to be this module's own dependency,
 *          not something it re-implements.
 *
 *          Two functions are genuinely NEW, added by this step, and are
 *          namespaced under `Clock::` to mark that distinction at a glance:
 *          `Clock::isTimeValid()` (a thin, semantics-preserving wrapper
 *          around `Time.isValid()` that decision sites in `src/state/`'s `.cpp` files
 *          now call instead of reaching for the Particle API directly) and
 *          `Clock::isPlausibleEpoch()` (absorbing the former
 *          `State_Sleep.cpp`-local `isRtcTimeValidForHibernate()`, which
 *          checked exactly this same "is this epoch a real date, not a
 *          reset/underflow artifact" question for one caller only).
 *
 *          Everything else below stays deliberately un-namespaced,
 *          preserving its existing call sites verbatim - this step proves,
 *          via a structural test, that it is only a move. Restructuring the
 *          ~8 sites that gate sleep/connect/report decisions on clock state
 *          is explicitly Step 3b's job, not this one's.
 *
 *          WO-2026-09-19 Step 3b update: that restructuring landed here.
 *          `Clock::isTrusted()` (a thin wrapper around `isClockTrusted()`)
 *          and `Clock::openness()` (a NEW, trust-aware replacement for
 *          `isWithinOpenHours()`'s boolean, fail-open answer) are what the
 *          decision sites now consume - `Time.isValid()` alone was never a
 *          trust signal (see `ClockTrust.h`), so a decision resting on
 *          `Clock::isTimeValid()`/`isWithinOpenHours()` was resting on "an
 *          epoch exists," not "this epoch can be trusted." `isWithinOpenHours()`
 *          and `isClockTrusted()` themselves are UNCHANGED - they remain the
 *          fail-open/telemetry-only helpers for the callers Step 3b did not
 *          touch (see `docs/work-orders/` for the exact reviewed list).
 */

#ifndef __CLOCK_H
#define __CLOCK_H

#include <stdint.h>
#include <time.h>

// ===== Relocated verbatim - global scope, unchanged call sites =====

/**
 * @brief Returns true when the local-time snapshot is inside configured open hours.
 *
 * @return true when the device should behave as within operating hours
 */
bool isWithinOpenHours();

/**
 * @brief Tests whether an epoch falls within configured local open hours.
 *
 * @param epoch UTC epoch to evaluate
 * @return true when reporting is allowed at that local time
 */
bool isWithinOpenHoursAt(time_t epoch);

/**
 * @brief Returns the seconds until the next configured open window begins.
 *
 * @return Seconds until the next open period, or 0 if already open
 */
int secondsUntilNextOpen();

/**
 * @brief Requests a Device OS time sync, paced by the retry floor.
 */
void requestClockResync(const char *reason);

/**
 * @brief Per-loop monotonic-gated resync request and recurring, observed-
 *        state-driven RTC write-back.
 */
void checkClockResync();

/**
 * @brief WO-2026-08-29-002 item 8: is Time.now() currently trustworthy?
 *
 * @return true when Time.now() should be treated as trustworthy.
 */
bool isClockTrusted();

/**
 * @brief Non-blocking substitute for Particle.timeSyncedLast().
 *
 * @return The last real Particle.timeSyncedLast() value observed while
 *         connected, or 0 if this boot has never yet been connected.
 */
uint32_t observedTimeSyncedLastMs();

/**
 * @brief Wrap-aware, sentinel-bearing sync age for TELEMETRY ONLY - not a
 *        control-path signal.
 */
uint32_t reportedSyncAgeMs();

// ===== New in Step 3a - namespaced to mark them as additions, not moves =====

namespace Clock {

/**
 * @brief Thin, semantics-preserving wrapper around `Time.isValid()`.
 *
 * @details Introduced so decision sites in `src/state/`'s `.cpp` files go through this
 *          module instead of calling the Particle API directly - the same
 *          "single owner" reasoning as every other function in this file.
 *          Deliberately just `return Time.isValid();` - Step 3a moves code,
 *          it does not change what any decision site decides. See this
 *          file's own top comment for why `Time.isValid()` alone is not a
 *          trust signal (`ClockTrust.h` has the full writeup); this wrapper
 *          intentionally preserves that exact (limited) meaning rather than
 *          quietly upgrading callers to `isClockTrusted()`, which would be
 *          a real decision-site behavior change reserved for Step 3b.
 */
bool isTimeValid();

/**
 * @brief True when an epoch is a plausible real-world date, not a reset/
 *        underflow artifact.
 *
 * @details Absorbs the former `State_Sleep.cpp`-local
 *          `isRtcTimeValidForHibernate()` - same bounds
 *          (2024-01-01T00:00:00Z inclusive to 2035-01-01T00:00:00Z
 *          exclusive), same one caller
 *          (`shouldUseBoronRtcAlarmHibernate()`), renamed to reflect that
 *          the check itself has nothing hibernate-specific about it: it is
 *          a general epoch-plausibility test that happened to have only one
 *          consumer before this move.
 */
bool isPlausibleEpoch(time_t epoch);

/**
 * @brief WO-2026-09-19 Step 3b: thin wrapper around `isClockTrusted()`.
 *
 * @details Deliberately just `return isClockTrusted();` - `isClockTrusted()`
 *          is already correct (WO-2026-08-29-002 item 8) and keeps its own
 *          six existing telemetry-only callers unchanged. This wrapper exists
 *          so the decision sites this step converts go through `Clock::`
 *          rather than the bare global name, the same "new seam gets a
 *          namespaced name" idiom `Clock::isTimeValid()` established in
 *          Step 3a - not a rename or a reimplementation.
 */
bool isTrusted();

/**
 * @brief Whether the park is Open, Closed, or Unknown right now.
 *
 * @details The trust-aware replacement for `isWithinOpenHours()`'s boolean
 *          answer at the decision sites Step 3b converts. `isWithinOpenHours()`
 *          fails OPEN (returns true) when `Time.isValid()` is false OR the
 *          open/close-hour config isn't loaded yet - a deliberate choice for
 *          its own callers (mostly telemetry/diagnostics, or "let the device
 *          start sensing before it has configuration"), but wrong for a
 *          decision that commits the device to state (an overnight hibernate,
 *          a sleep-ceiling exemption): failing open there means a clock that
 *          is merely RTC-seeded - not confirmed by a recent cloud sync, and
 *          therefore possibly hours wrong (see this file's Step 3a note on
 *          why `Time.isValid()` is not a trust signal) - can commit the
 *          device to a decision based on a wrong belief about what time it
 *          is. `openness()` never fails open: an untrusted clock or missing
 *          config both return `Unknown`, explicitly, so every caller must
 *          decide what `Unknown` means for its own decision rather than
 *          silently inheriting `true`.
 */
enum class Openness : uint8_t {
  Open,
  Closed,
  Unknown,
};

Openness openness();

} // namespace Clock

#endif
