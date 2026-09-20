# WO-2026-09-18-001: `Clock` - single owner of "what time is it, and can it be trusted"

**Type:** Structural fix (Step 3a of the Structural Ownership Map roadmap).

**Status:** CLOSED 2026-09-20. Code complete, fully tested on the host and
via cloud compile (Boron, target 6.4.1), and bench-validated on Dev-14
across two nights - see Bench validation below. No behaviour change was
intended, and none was observed.

## The observation

Before this step, `isWithinOpenHours()`/`isWithinOpenHoursAt()`/
`secondsUntilNextOpen()` (open-hours evaluation) and
`checkClockResync()`/`requestClockResync()`/`isClockTrusted()`/
`observedTimeSyncedLastMs()`/`reportedSyncAgeMs()` (the resync/RTC-write-back
cycle and trust signal) were all defined directly inside
`Generalized-Core-Counter.cpp`, callable only because every consumer
happened to link into the same binary - no single file could answer "what
time is it after this wake" on its own. This is the same finding
`docs/architecture-review-2026-09-03.md` names for Step 2's `HibernateCycle`
work, applied to the clock-trust side of the same problem (Q1 of the agreed
Structural Ownership Map roadmap).

## The fix

New module, `time/Clock.{h,cpp}`, now owns open-hours evaluation, the
resync/RTC-write-back cycle, and the sync-recency trust signal:

- **Eight functions relocated verbatim.** `isWithinOpenHours()`,
  `isWithinOpenHoursAt()`, `secondsUntilNextOpen()`, `checkClockResync()`,
  `requestClockResync()`, `isClockTrusted()`, `observedTimeSyncedLastMs()`,
  and `reportedSyncAgeMs()` kept their existing global-scope names and
  bodies - only the defining file moved, confirmed byte-identical against
  the original source by direct diff. Every existing call site across the
  tree (`state/*.cpp`, `reporting/`, `cloud/`, `diagnostics/`) kept working
  unchanged; this step proves it is only a move.
- **`isRtcTimeValidForHibernate()` folded into `Clock::isPlausibleEpoch()`.**
  Absorbed from its former `State_Sleep.cpp`-local home - same bounds
  (2024-01-01T00:00:00Z inclusive to 2035-01-01T00:00:00Z exclusive), same
  one caller (`shouldUseBoronRtcAlarmHibernate()`), renamed to reflect that
  the check itself has nothing hibernate-specific about it.
- **`Clock::isTimeValid()` introduced** as a thin, semantics-preserving
  `return Time.isValid();` wrapper - the seam this step's own scope
  deliberately does nothing with yet (see "Not in scope," below). Roughly
  12 decision sites across `src/state/*.cpp` and 3 inside
  `Generalized-Core-Counter.cpp` itself now call this wrapper instead of the
  raw Particle API, each a mechanical, zero-logic-change swap required to
  satisfy this step's own "zero `Time.isValid()` under `src/state/`"
  structural test.
- **`MyPersistentData.cpp:43`'s forward declaration replaced with an
  `#include "time/Clock.h"`** - the correct-direction fix, mirroring the
  same fix already applied to `isClockTrusted()`'s declaration during Step 2.

### Why the relocated functions stayed un-namespaced

All eight pre-existing functions kept their global-scope names rather than
moving under a `Clock::` prefix. Namespacing them would have forced touching
every call site across `state/`, `reporting/`, `cloud/`, and `diagnostics/`
for a rename with no behavioral point - `Clock::isTimeValid()` and
`Clock::isPlausibleEpoch()` are the only two functions namespaced, marking
them as genuinely new additions rather than moves.

## Verification

- **Structural allowlist test.** `tests/clock_owner_structural_test.py`
  asserts all 8 relocated functions are defined exactly once in `Clock.cpp`
  and zero times in `Generalized-Core-Counter.cpp`; `isRtcTimeValidForHibernate`
  is gone tree-wide; `Clock::isPlausibleEpoch` has exactly one caller; zero
  `Time.isValid()` calls exist under `src/state/`; every surviving
  `Time.isValid()` call in `Generalized-Core-Counter.cpp` is on an explicit
  4-line formatting/telemetry allowlist (never a decision site); and
  `MyPersistentData.cpp` uses the include, not a forward declaration.
  Mutation-tested: reintroduced a stray `Time.isValid()` in `State_Idle.cpp`,
  confirmed the test failed for the right reason, restored byte-identical
  (sha256 match).
- **Existing tests re-pointed.** `tests/clock_resync_wiring_test.py` (the
  heaviest existing clock test) now extracts `checkClockResync()`/
  `requestClockResync()`/`isClockTrusted()` from `Clock.cpp` instead of the
  app file; `tests/rtc_skew_test.sh`'s A.6-gap-2 harness now extracts
  `isPlausibleEpoch()` from `Clock.cpp`. Both pass with zero loosened
  assertions. Three files got comment-only accuracy fixes
  (`clock_rtc_writeback_test.cpp`, a `CloudTestShim.cpp` stub comment,
  `tests/README.md`).
- **Full suite.** 30/30 (29 pre-existing + the new structural test), plus
  the standalone host `.cpp` tests without a `.sh` wrapper - all pass, none
  needed changes beyond the one comment fix.
- **Cloud compile.** `particle compile boron . --target 6.4.1` succeeds.
  Flash 149430 / RAM 3406 (Step 2 baseline: 149614 / 3406 - 184 bytes
  smaller, RAM unchanged; consistent with a pure relocation).
- **Non-regression confirmed directly, not just inferred from green tests.**
  Diffed the relocated region of `Clock.cpp` against the original extracted
  source - byte-identical. Diffed every touched file's added lines
  individually - each is either an `#include`, a moved-declaration comment,
  or a `Time.isValid()` -> `Clock::isTimeValid()` token swap on an otherwise
  untouched line.

## Bench validation

Two nights, Dev-14 only (per the roadmap - Dev-11 deliberately held out,
reserved as Step 3b's acceptance device), commit `b517cbf`, binary
`wo-2026-09-18-step3a-boron-b517cbf.bin`
(sha256 `5c9136fda583dd3e9a0e0647233b0cad67f7421b46afc31934fd62c4952fca3f`),
flashed 2026-09-18 13:10 SGT (05:10 UTC).

| | Night 1 (Sept 18-19) | Night 2 (Sept 19-20) |
|---|---|---|
| `hibernate_wake` | result=ok, gateArm=none, wakeReason=ALARM, count=3 | result=ok, gateArm=none, wakeReason=ALARM, count=4 |
| req / actual sleep sec | 28797 / 28798 (err=1) | 28798 / 28799 (err=1) |
| First confirmed sync after wake | 9s | ~2s |
| `trusted` / `syncAgeSec` at later check | true, reconstructs within 5s of fetch time | true, reconstructs within 6s of fetch time |
| Watchdog resets | 2 (see below) | 0 |

**Clock module behaved correctly across both a planned hibernate wake and an
unplanned reset - a stronger result than a quiet night would have given.**
Every fresh boot this soak hit (both hibernate wakes, and the two watchdog
resets on night 1) is exactly the "fresh boot, `timeSyncedLast()==0`" case
`checkClockResync()` is designed to handle, and in every case a confirmed
resync landed within seconds of reconnecting. `reportedSyncAgeMs()` stayed
internally consistent for hours after each sync (reconstructed
`lastSyncEpoch + syncAgeSec` matched the actual ledger-fetch wall-clock time
to within single-digit seconds).

**Night 1's two watchdog resets are a real, pre-existing condition, not a
Step 3a regression.** Confirmed via the `watchdog` event forensics: one hung
in the `connectivity` loop stage (bc=18), the other in the `sleep` stage
(bc=28, elapsed 55s) - the second matches the shape of the already-filed
`WO-2026-09-03-004` ("MAFC-1 hangs entering sleep"). This step's diff never
touched `State_Connect.cpp`, loop-stage watchdog timing, or the sleep-entry
breadcrumb sequence. Night 2 had zero watchdog events - whatever triggered
the NCP/modem hiccup (`Failed to power off`, `NCP client is not ready`) on
night 1 did not recur.

**`HibernateCycle`'s own gate (Step 2) is confirmed unaffected**: `gateArm`
and `count` both progressed correctly (3 -> 4) across this soak, consistent
with Step 2's own bench results and untouched by anything in this step's diff.

## Not in scope for this step

Named per the roadmap and not touched here: restructuring the ~13 decision
sites that now call `Clock::isTimeValid()` to actually consume trust
(`Clock::isTrusted()`/`Clock::openness()`) instead of mere validity - that
is Step 3b, the roadmap's only behaviour-changing step, and is a separate
branch/WO stacked on this one. `BatteryAuthority` (Step 4), the
persistence-header split (Step 5), `sensors/SensorManager.cpp:490` (explicit
decision, same reasoning as Step 2), and the `AppBreadcrumb` enum
cross-TU-visibility fix (`WO-2026-09-16-001`, filed separately) are all
untouched.

## Provenance

Scoped by `docs/architecture-review-2026-09-03.md` and the Structural
Ownership Map roadmap agreed 2026-09-03 (Step 3a of that roadmap).
Implemented and bench-validated 2026-09-18 through 2026-09-20 on branch
`wo/2026-09-18-step3a-clock-owner`, commit `b517cbf`.

## Related work orders

- `docs/architecture-review-2026-09-03.md` - source of the roadmap this step
  is item 3a of.
- `WO-2026-09-16-002` - Step 2, established the "relocate verbatim, prove it
  with a structural test" pattern this step follows for a second module.
- Step 3b (not yet closed with its own WO doc as of this writing, branch
  `wo/2026-09-19-step3b-trust-standard`) - the behaviour change this step's
  `Clock::isTimeValid()` seam exists for.
- `WO-2026-09-03-004` - the MAFC-1 sleep-path watchdog-stall investigation;
  night 1's two watchdog resets match its signature and are explicitly not
  attributed to this step.
