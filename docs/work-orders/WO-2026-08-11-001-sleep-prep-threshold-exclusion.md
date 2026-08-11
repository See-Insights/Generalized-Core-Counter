# WO-2026-08-11-001: Exclude SLEEP_PREP from LoopStage threshold-based logging

## Context

Investigation (this session) confirmed `LOOP_STAGE_SLEEP_PREP`'s tracked
`elapsed` structurally spans three back-to-back things inside one call:
pre-sleep prep/gate-wait, the entire blocking `System.sleep()` (same
duration as `WakeReturn`'s `elapsed`), and the post-wake tail. Because the
stage tag is set once at entry and never re-tagged around the blocking
sleep call, `kLoopStageErrorThresholdMs` (10000ms) ends up gating on
"processing time plus the intentional multi-minute sleep itself" — not
processing time alone.

Measured effect: 21 of 23 sampled cycles from a single clean, fault-free
capture hit `WARN`/`ERROR` on this stage, virtually all of it routine
(any TIMER wake of 300s+ unconditionally logs at `ERROR`). The two
genuine anomalies in that sample (gate-wait outliers of 25,129ms and
7,815ms) are completely indistinguishable in the log stream from routine
300s sleeps without manual `WakeReturn`-pairing arithmetic. This
diagnostic currently provides no discriminating signal for this specific
stage — a real gap, given this is exactly the area WO-2026-08-10-001's
instrumentation work was meant to improve visibility into.

`WakeReturn`/`GateRelease` already carry the real signal for the
components this stage's elapsed time conflates (sleep duration and
gate-wait duration, respectively).

## Fix

Exclude `SLEEP_PREP` from `kLoopStageErrorThresholdMs`-based `WARN`/`ERROR`
escalation. Decided against giving the gate-wait its own stage tag
(rejected: adds new instrumentation surface for a case `GateRelease`
already covers, cuts against this project's standing "avoid unneeded
complexity" principle).

Keep logging at `INFO` for this stage (don't silently drop the line
entirely — still useful raw data if someone's specifically looking), just
remove it from whatever escalates stages past the threshold to
`WARN`/`ERROR`. Confirm the cleanest implementation point — likely a
stage-specific exclusion in `noteLoopStageDuration()` or wherever the
threshold comparison currently happens — rather than special-casing at
each of the (currently single) call site.

## Acceptance Criteria

- Routine `SLEEP_PREP` cycles (both `state=3` PIR-return-to-sleep and
  `state=5` Report transitions, any sleep duration) no longer log at
  `WARN`/`ERROR` — confirm against the same 23-cycle sample data if still
  available, or a fresh equivalent capture.
- `SLEEP_PREP` still logs at `INFO` with its `elapsed`/`state`/`q`/`connMs`
  fields intact — this is an escalation-level change, not a removal of the
  diagnostic.
- No change to any other loop stage's threshold behavior (`CONNECTIVITY`,
  `CLOUD_LOOP`, `PUBLISH_QUEUE`, `DIAGNOSTICS`) — confirm the exclusion is
  scoped to `SLEEP_PREP` specifically, not a global threshold change.
- A genuinely anomalous `SLEEP_PREP` duration (if one recurs) should still
  be identifiable via `WakeReturn`/`GateRelease`'s existing signal — this
  fix doesn't reduce real anomaly-detection capability, it removes false
  positives that were masking it.

## Required Tests

- Regression: a routine 300s+ TIMER wake no longer produces a `WARN`/
  `ERROR`-level `LoopStage` line.
- Regression: a routine short PIR-return-to-sleep (`state=3`) no longer
  produces a `WARN`/`ERROR`-level line.
- Confirm `INFO`-level logging for `SLEEP_PREP` still fires and carries
  the same fields as before.
- Confirm other loop stages' threshold behavior is unchanged (existing
  tests, if any, should already cover this — verify they still pass).

## Permitted Files

Likely `src/state/State_Sleep.cpp` and wherever `noteLoopStageDuration()`/
the threshold comparison lives (`Generalized-Core-Counter.cpp` per the
investigation). Codex to confirm exact scope.

## Status

Original fix merged (PR #15), then found defective on real hardware — see
Corrective Pass below.

## Corrective Pass (post-merge regression)

### What went wrong

The merged fix (unconditional `if (stage == LOOP_STAGE_SLEEP_PREP) { Log.info(...); return; }`
inside `noteLoopStageDuration()`) caused a severe log-volume regression on
real hardware: `LoopStage: stage=SLEEP_PREP` logging at `INFO` on
essentially every `loop()` iteration while `state == SLEEPING_STATE`
(observed at ~2-3ms intervals, ~1000 lines truncated between two samples
in one capture), not once per sleep cycle as intended.

Root cause, confirmed directly against source: `handleSleepingState()` is
not a single blocking call spanning gate-wait + sleep + wake, as the
original investigation modeled it. It is a re-entrant polling state
machine, called on every `loop()` iteration while `SLEEPING_STATE` is
active, with multiple early-return points during gate-wait/teardown
(`State_Sleep.cpp:574`, `:741`, `:764` — e.g. `return; // Stay in
SLEEPING_STATE until complete or timeout`). Only the specific call that
finally clears every gate proceeds to the real blocking `System.sleep()`;
every prior call that cycle returns early, back to `loop()`, which calls
`noteLoopStageDuration(false)` unconditionally every iteration
(`Generalized-Core-Counter.cpp:1190`).

Before this WO, `elapsed` (time since first entry into `SLEEP_PREP`,
which only resets on a genuine stage change) climbed slowly across these
fast poll iterations, so the vast majority never crossed
`kLoopStageWarnThresholdMs` (2000ms) - the threshold was accidentally
functioning as a rate-limiter, not just a severity filter. This WO's
original fix removed that gate entirely for `SLEEP_PREP`, exposing every
polling iteration. It also explains why the original 23-cycle sample
capture used to diagnose this WO never showed the problem: that capture
was itself implicitly filtered by the exact suppression mechanism being
removed, so nobody (investigation, Copilot, or Stage 7) had visibility
into true per-iteration call frequency until real hardware exposed it
post-merge.

### Why Stage 7 didn't catch it

`tests/loop_stage_sleep_prep_exclusion_test.py` performed static
structural parsing of the source (branch exists, unconditional, returns,
carries the right fields - all genuinely valid) plus a synthetic
`decide()` function exercising exactly one simulated call per scenario.
It never modeled repeated calls against unchanged stage state, so it
asserted branch correctness in isolation, never call frequency or log
volume under the real polling architecture. Named gap for future WOs
touching anything in `loop()`'s polling path: single-call correctness
tests are insufficient when the call site re-enters many times per
second.

### Corrected design (approved)

Move the `SLEEP_PREP` `INFO` log from "every call while stage is active"
(`noteLoopStageDuration()`) to "once, on genuine transition away from the
stage" (`setLoopStage()`). `setLoopStage()` already detects real
transitions via its `if (retainedLoopForensics.lastLoopStage ==
(uint8_t)stage) { return; }` guard, and has access to the *old* stage and
its `stageStartMillis` at the exact moment of a real change, before
overwriting them. Add a check there: when leaving
`LOOP_STAGE_SLEEP_PREP`, log one `INFO` line with the total elapsed time
for that span (gate-wait + sleep + wake, whatever the real duration was),
using the same `stage`/`elapsed`/`state`/`q`/`connMs` fields as before.
This fires exactly once per real `SLEEP_PREP` span regardless of how many
polling iterations occurred inside it (1 or 5,000), and reproduces what
the original `WARN`/`ERROR` design was actually trying to measure.

Remove the `SLEEP_PREP` branch from `noteLoopStageDuration()` entirely -
this redesign replaces it, not supplements it. Since both
`noteLoopStageDuration()` and the new `setLoopStage()` branch need the
same five-field `LoopStage:` line, factor it into a small shared helper
(e.g. `logLoopStageInfo(stage, elapsedMs)`) rather than duplicating the
format string.

### Corrective Acceptance Criteria (supersedes/extends the original)

- No `LoopStage: stage=SLEEP_PREP` line is logged more than once per real
  `SLEEP_PREP` span, no matter how many `loop()` iterations/polling calls
  occur while gate-wait/teardown is in progress.
- The single line logged on exit carries the correct total elapsed time
  for the whole span (gate-wait + sleep + wake), and the same
  `state`/`q`/`connMs` fields as before.
- `SLEEP_PREP` still never escalates to `WARN`/`ERROR` (the original WO's
  core fix is preserved - only the logging trigger point changes).
- No change to any other loop stage's threshold/logging behavior.
- **Non-negotiable test requirement**: the regression test must explicitly
  drive multiple simulated calls against unchanged stage state (i.e.
  simulate N `loop()`-style polling iterations while `SLEEP_PREP` stays
  active without a real transition) and assert bounded, single-line
  output - not just correct branch logic for one isolated call. This is
  the exact gap that let the previous round through and must be closed by
  name, not implied by "add more tests."

### Corrective dispatch (first attempt - superseded, see Second Corrective Pass)

Approved: dispatch to Copilot (not self-implemented - this is a real
architectural change touching two functions plus a new shared helper).
Independent verification (diff review, tests run directly, independent
compile) plus a fresh Codex Stage 7 pass required before this returns to
Chip's final gate - the previous "Verified with concerns" did not catch
this, so this pass needs genuine scrutiny, specifically including the
call-frequency test requirement above.

Implemented (moving the SLEEP_PREP exit-log into `setLoopStage()`, gated
on its own same-stage transition guard) and passed independent
verification, including a strengthened regression test closing a real
gap Codex's Stage 7 pass found in the first draft (structural check only
matched literal `Log.info(` calls, not calls through the new shared
`logLoopStageLine()` helper - broadened and reverified via mutation
testing). Sent to a fresh Stage 7 pass with explicit adversarial framing,
given the history on this WO.

## Second Corrective Pass (Stage 7 found the redesign itself was still broken)

### What Stage 7 found

Codex returned **Not verified**, with a concrete, source-verified
counterexample: `loop()`'s structure
(`Generalized-Core-Counter.cpp:1220-1258`) retags the loop stage through
a fixed sequence every single iteration -
`CONNECTIVITY -> STATE_HANDLER -> [handler] -> DIAGNOSTICS -> ...` -
regardless of what the dispatched handler actually did. When
`handleSleepingState()` sets `SLEEP_PREP` and then returns early (still
gate-waiting), `loop()` unconditionally calls
`setLoopStage(LOOP_STAGE_DIAGNOSTICS)` two lines later - a
`SLEEP_PREP -> DIAGNOSTICS` transition by `setLoopStage()`'s own
same-stage-guard definition, since the two tags differ. This fired the
new exit-log on essentially every polling iteration, reproducing almost
exactly the original regression, just relocated. Confirmed directly
against source (not taken on Codex's word alone): `setLoopStage()`'s
same-stage guard cannot distinguish "the real end of a `SLEEP_PREP` span"
from "just this poll ended," because something else always re-tags the
loop stage in between, every iteration, for unrelated bookkeeping.
Codex's own regression test gap: its call-frequency simulation modeled
`SLEEP_PREP` remaining the "current stage" across repeated calls, which
is not what production `loop()` actually does.

### The real signal (confirmed against source)

`state` (the app-level FSM variable, not the loop-stage tag) is written
in exactly one place in the entire codebase: `transitionTo()`
(`Generalized-Core-Counter.cpp:2124`), confirmed via a full-repo grep for
direct `state =` assignments. Every early-return gate-wait/teardown path
in `handleSleepingState()` (`State_Sleep.cpp:574`, `:741`, `:764`)
returns without calling `transitionTo()`; every genuine exit calls it
exactly once. Whether `transitionTo()` fired during a given dispatch is
the correct, already-existing signal for "did a `SLEEP_PREP` span
genuinely end" - not the loop-stage tag.

Two designs built on this signal were evaluated head-to-head (both were
worked up in full, not just one presented as a fait accompli):

- **Option B (transition-counter in `loop()`)**: a global counter
  incremented inside `transitionTo()`, sampled before/after the
  state-handler switch in `loop()`. Smaller diff, but touches
  `transitionTo()` (shared by every state handler in the codebase) and
  depends on an unenforced invariant - nothing else may run between the
  switch and the check that itself calls `transitionTo()`. True today,
  not guaranteed to stay true.
- **Option A (local wrapper at `handleSleepingState()`'s own exit
  points) - approved**: inventoried `transitionTo(...)` call sites inside
  `handleSleepingState()` (verified via `awk`/`grep` against the actual
  function body, `State_Sleep.cpp:316-1679`) plus 2 `System.reset()`
  paths that reboot without transitioning. Original count stated here was
  15 - a counting error caught during implementation; Copilot's own
  direct grep found 14 total `transitionTo(` calls (13 real/reachable + 1
  dead line immediately after the second `System.reset()`), correctly
  reported rather than silently reconciled. Introduce a local wrapper
  `exitSleepingState(State newState, const char *reason)` that logs the
  SLEEP_PREP exit line then calls the real `transitionTo()`; replace all
  13 real direct calls with it (the 1 dead line stays untouched, grouped
  with the reset-path exclusions below). Confined entirely to
  `State_Sleep.cpp`; the "did someone forget a call site" risk is closed
  by a structural test asserting zero raw `transitionTo(` calls remain
  inside `handleSleepingState()`'s body. Chosen over Option B specifically
  because this WO has now missed twice from designs that looked correct
  but had a non-obvious cross-cutting dependency - Option A trades a
  larger, mechanical diff for zero reliance on `loop()`'s surrounding
  structure staying the same.

A real correctness bug was found and fixed during this evaluation, before
implementation: gating the span-start timestamp reset on `enteredState`
(`state != oldState`) is wrong, because two of the 15 exit paths
transition `SLEEPING_STATE -> SLEEPING_STATE`
(`sleep-timer-occupied-suppress-report`, `sleep-pir-return-to-sleep`) - a
real cycle ends and a new one begins, but the FSM `state` value doesn't
change, so `enteredState` stays false and the timestamp would never
reset, silently reporting `elapsed=0` for the next cycle. Fixed by
gating the SET on the timestamp's own value being `0` (self-initializing,
mirroring the existing `cloudSyncStartMs` pattern at
`State_Sleep.cpp:462-463`), independent of whether the FSM state value
itself changes.

### Deliberate scope decision: the two `System.reset()` paths are excluded

Both already carry specific, purpose-built diagnostics immediately before
the reset that explain the reset cause with more precision than a generic
span-duration line would: the nightly heap-guard reset
(`State_Sleep.cpp:1123`) logs
`Log.warn("Nightly heap guard: freeHeap=%lu <= %lu ... resetting for
clean next-day start", ...)`; the all-sleep-attempts-failed reset
(`State_Sleep.cpp:1414`) logs `Log.error("All sleep attempts failed
err=%d - immediate reset required", ...)` plus a follow-up `Log.info`,
and already raises alert 16. The device reboots regardless in both cases,
so the span-tracking timestamp becomes moot the instant `System.reset()`
executes, and self-initializes cleanly post-boot either way. These two
paths stay as direct, unwrapped `transitionTo()`/`System.reset()` calls,
explicitly outside `exitSleepingState()`'s scope.

### Hand-traced dry-run against both pieces of real log evidence (done before dispatch, per Chip's explicit request)

**~1000-line polling capture**: none of the lines coincide with any of
the 15 real exit points - they are all early `return`s from the
gate-wait section, before any exit point is reached. Zero lines logged
for the entire span under the new design; exactly one line at the actual
exit, elapsed spanning the true full dwell.

**Original 23-cycle sample**: 21 "boring" cycles (already disconnected on
entry, single call, no self-transition) reproduce the original numbers
exactly (`WakeReturn_elapsed + ~1.6-1.9s`). The two historical outliers
(25,129ms and 7,815ms excess): re-deriving this carefully surfaced that
the OLD measurement mechanism (`retainedLoopForensics.stageStartMillis`)
was subject to the same per-iteration reset contamination as the design
that just failed Stage 7 - in the original, pre-WO code too, not only in
the redesign - so the original causal explanation for those two specific
numbers (gate-wait time accumulating into one call's elapsed) is likely
incomplete, and confirming the true historical explanation would need the
raw surrounding log lines for those two cycles, not available this
session. Explicitly flagged as an open question, not resolved, and
explicitly not a blocker: the new design's timestamp is structurally
independent of loop-stage tag churn, so it is provably correct for any
future cycle of this shape regardless of how the old, now-abandoned
mechanism behaved historically.

### Second corrective dispatch

Approved: dispatch to Copilot. Full revert of the `setLoopStage()`-based
exit-log from the first corrective attempt, replaced by
`exitSleepingState()` wrapping all 15 real exit points in
`State_Sleep.cpp`, zero-gated timestamp reset, and a structural test
asserting no raw `transitionTo(` calls remain inside
`handleSleepingState()`'s body. Independent verification plus a fresh,
explicitly adversarial Codex Stage 7 pass required again before this
returns to Chip's final gate.

Implemented (Copilot correctly executed the approved design exactly as
specified - both findings below are gaps in the design/verification
process, not implementation defects). Independently verified: diff
review, 3 self-run mutation tests (enteredState-gate reversion, one
call-site left unconverted, missing timestamp reset - all caught
correctly), full pre-existing suite, and an independent compile matching
Copilot's reported numbers exactly. Sent to a third, explicitly
adversarial Stage 7 pass given the WO's history.

## Third Corrective Pass (Stage 7 found a real design gap plus a recurring test-quality pattern)

### Finding 1: HIBERNATE resets poison the next span (real bug, design gap - not a Copilot execution error)

`State_Sleep.cpp:1097-1131`: the Boron overnight closed-hours sleep path
uses `SystemSleepMode::HIBERNATE`, which the code's own comments confirm
resets the MCU without returning on its normal success path (`:1119`,
`:1126`) - not a rare edge case, this is the ordinary nightly path for
this product. `sleepPrepSpanStartMillis` is part of the `retained` struct
and survives any reset, not just sleep; the version-mismatch reinit
(`ensureRetainedLoopForensicsInitialized()`) only fires on a genuine
struct-layout change, not an ordinary same-firmware reboot. So a
successful HIBERNATE cycle leaves the timestamp stuck nonzero, and the
next `SLEEPING_STATE` dwell's zero-gated init is skipped, computing
elapsed against a pre-reset `millis()` value - `millis()` resets to 0 on
a real reboot (unlike STOP/ULP sleep), so this produces a stale or
unsigned-wrapped number, corrupting one `LoopStage: SLEEP_PREP` line
every morning after a normal overnight cycle.

Root cause of the miss: the "two `System.reset()` paths" inventory from
the Second Corrective Pass was built by grepping for the literal string
`System.reset()`. HIBERNATE's reset is implicit, inside
`System.sleep(config)` with `SystemSleepMode::HIBERNATE` - not a
syntactically visible reset call - so a text-based inventory could not
have found it. A genuine gap in the design/verification method, not
something Copilot should have caught independently either, since it
correctly implemented exactly the design it was given.

**Fix**: `retainedLoopForensics.sleepPrepSpanStartMillis` must be cleared
unconditionally on every boot, in `setup()`, regardless of magic/version
match - any real boot invalidates any in-flight span by definition,
since the call stack that would have closed it is gone. This single fix
covers all three reset paths (HIBERNATE, and both explicit
`System.reset()` calls) uniformly.

### Finding 2: the regression test accepted both duplicate and zero logging

Codex substituted `exitSleepingState()`'s body with two mutations - log
the SLEEP_PREP line twice per call, and never log it (wrapped in
`if (false)`) - and the full test suite passed both. Root cause: the
structural check only verified the logging helper's name appears
*somewhere* in the wrapper's text (a presence check), and the
call-frequency simulation is a hand-written Python model that
unconditionally logs exactly once per simulated call, never actually
derived from or tied to the real C++ call count. This is the same
*class* of gap as the one fixed in the First Corrective Pass (a
presence/substring check standing in for a reachability/count
guarantee) - the third variation of "the Python model doesn't verify the
C++" across this WO's three rounds, worth naming as a recurring pattern,
not just patching quietly again.

### Testing design change: extract the drifted logic into a genuinely compiled test, not a fourth simulation

Given the pattern above, the span-timing arithmetic (start/close/
boot-reset - the exact logic all three of this WO's regressions have
lived in) is extracted into a new, dependency-free header,
`src/state/SleepPrepSpanTiming.h` (`<cstdint>` only - no `Particle.h`,
no `AB1805_RK.h`), defining three pure inline functions:

- `maybeStartSleepPrepSpan(uint32_t &spanStartMillis, uint32_t nowMs)` -
  the zero-gated start, used by `handleSleepingState()`.
- `closeSleepPrepSpan(uint32_t &spanStartMillis, uint32_t nowMs) ->
  uint32_t` - computes elapsed, resets to 0, used by
  `exitSleepingState()`.
- `resetSleepPrepSpanOnBoot(uint32_t &spanStartMillis)` - the new boot-time
  fix, called from `setup()`.

A new test, `tests/sleep_prep_span_timing_test.cpp`, `#include`s this
header directly and compiles/runs on the host with zero stubbing (no
`tests/stubs/` dependency at all, since the header has no Particle
dependencies) - this exercises the *real* production functions, not a
reimplementation, closing the class of gap that recurred three times for
this specific logic. It must include, at minimum: a normal span; the
`SLEEPING_STATE -> SLEEPING_STATE` self-transition case (two spans back
to back via the same zero-gate); and the HIBERNATE-poisoning scenario as
a genuine before/after regression proof - a stale nonzero value survives
without calling `resetSleepPrepSpanOnBoot()` (proving the bug would occur
without the fix) and is correctly cleared when it is called, letting the
next span start fresh.

`State_Common.h` was considered as the location for these functions
instead of a new header, but it transitively includes `AB1805_RK.h` via
`StateMachine.h`, which isn't stubbed for host compilation - confirmed by
checking `tests/stubs/`'s actual contents - so it wasn't viable without a
much larger stub investment. This is a deliberate, stated scope boundary,
not a default: the wrapper glue around these functions
(`exitSleepingState()`'s logging call, `handleSleepingState()`'s call-site
wiring) remains structurally verified via source parsing rather than
compiled, since making that fully host-compilable would require stubbing
`AB1805_RK`, `Cloud`, `ConnectivityPolicy`, `ThrashGuard`, `Observability`,
and `SensorManager` - disproportionate to this WO, and not where any of
the three actual regressions have lived.

The existing `tests/loop_stage_sleep_prep_exclusion_test.py` is kept and
updated for what it's already suited to (wrapper call-site count,
ordering, no-raw-`transitionTo`, no-direct-`Log.x` checks), with Finding
2's specific gap closed: the check for `exitSleepingState()`'s logging
call must assert it occurs exactly once, at the function's own top-level
brace depth (not nested inside any conditional/loop) - catching both of
Codex's example mutations directly.

### Third corrective dispatch

Approved: dispatch to Copilot. Scope: new `src/state/SleepPrepSpanTiming.h`
header and its host test; `State_Sleep.cpp` and `Generalized-Core-Counter.cpp`
updated to call the three extracted functions instead of their inline
equivalents, plus the new unconditional boot-time clear in `setup()`;
`tests/loop_stage_sleep_prep_exclusion_test.py` strengthened per Finding 2;
`tests/README.md` updated. Independent verification plus a fresh,
explicitly adversarial Codex Stage 7 pass required again before this
returns to Chip's final gate.

Implemented and independently verified (diff review, 3 self-run mutation
tests including compiling and running the new host test to confirm it
genuinely crashes on the HIBERNATE-fix regression, full pre-existing
suite, independent compile matching Copilot's reported numbers exactly).
Sent to a fourth, adversarial Stage 7 pass, specifically asked to hunt
for a fourth reset-causing path beyond HIBERNATE.

## Fourth Corrective Pass (Stage 7 found the exit-point enumeration itself was incomplete)

### Finding 1: `SLEEPING_STATE` can be exited from outside `handleSleepingState()` entirely (real design defect, not a test gap)

Codex found, and I independently confirmed directly against source, that
the entire `exitSleepingState()` wrapper design rested on an unstated,
false assumption: that every real exit from `SLEEPING_STATE` happens via
a `transitionTo()` call from *within* `handleSleepingState()`. Three
counterexamples, all confirmed live:

- `Generalized-Core-Counter.cpp:1290` - out-of-memory handling calls
  `transitionTo(ERROR_STATE, "out of memory")` unconditionally, in
  `loop()`, after the state-handler switch.
- `Generalized-Core-Counter.cpp:1298` - user-switch-press handling calls
  `transitionTo(REPORTING_STATE, "user switch")`, same location.
- `Generalized-Core-Counter.cpp:2242` - `connectivityFailsafeSupervisor()`
  (called *before* the state-handler switch even runs) calls
  `transitionTo(CONNECTING_STATE, "failsafe stage 1")`.

None of these route through `exitSleepingState()`. If any fires while a
`SLEEP_PREP` span is open (e.g. mid gate-wait poll), the span never
closes: no log line, and the timestamp stays stuck non-zero, poisoning
the next real dwell exactly like the HIBERNATE bug this same pass
already fixed - just via a path outside `State_Sleep.cpp` entirely, so
the earlier reset-path audit (which was scoped to reset-causing code,
correctly) never looked for it.

### Finding 2: a rogue extra logging call, planted directly in the gate-wait polling section, passes both test suites

Codex mutation-tested by inserting an unconditional
`logLoopStageLine(LOG_LEVEL_INFO, LOOP_STAGE_SLEEP_PREP, 0UL, 0, 0UL);`
immediately before the existing cloud-gate polling `return;`
(`State_Sleep.cpp:610`) - reproducing the original per-iteration
log-storm regression. Both suites passed. Root cause: the compiled test
only includes `SleepPrepSpanTiming.h` (untouched by this mutation), and
the Python test's helper-call-count check is scoped specifically to
`exitSleepingState()`'s body - it has no mechanism to detect an
additional, illegitimate call planted anywhere else in the file.

### Corrected design: move the exit-log to the one universal choke point

Enumerating call sites was always going to be incomplete by
construction, proven twice now (HIBERNATE, then this). `state` is
written in exactly one place in the entire codebase: `transitionTo()`
(`Generalized-Core-Counter.cpp:2098`) - every transition, from any file,
any call site, passes through it. The corrected fix moves the check
there:

```cpp
void transitionTo(State newState, const char *reason) {
  if (state == SLEEPING_STATE && retainedLoopForensics.sleepPrepSpanStartMillis != 0) {
    const unsigned long nowMs = millis();
    const unsigned long elapsedMs =
        closeSleepPrepSpan(retainedLoopForensics.sleepPrepSpanStartMillis, nowMs);
    const uint16_t queueDepth = (uint16_t)PublishQueuePosix::instance().getNumEvents();
    const uint32_t millisSinceLastCloudConnect = (connectedStartMs != 0 && nowMs >= connectedStartMs)
        ? (uint32_t)(nowMs - connectedStartMs) : 0UL;
    logLoopStageLine(LOG_LEVEL_INFO, LOOP_STAGE_SLEEP_PREP, elapsedMs, queueDepth, millisSinceLastCloudConnect);
  }
  Log.info("StateReq: %s->%s reason=%s", stateShortName(state), stateShortName(newState),
           (reason != nullptr) ? reason : "unspecified");
  state = newState;
}
```

Confirmed no false-positive risk: grepped every `transitionTo(SLEEPING_STATE, ...)`
call site across the codebase (`ThrashGuard.cpp`, `State_Connect.cpp`,
`State_Idle.cpp`) - all are entries *into* `SLEEPING_STATE` from a
different prior state, none are self-transitions while already in
`SLEEPING_STATE`, so the `state == SLEEPING_STATE` guard (checking the
*old* state, before the assignment) never misfires on entry.

**`exitSleepingState()` is removed entirely.** All 13 call sites in
`State_Sleep.cpp` revert to plain `transitionTo()` - the wrapper's job is
now done automatically and completely by the choke-point check, with no
enumeration and no way to bypass it. `maybeStartSleepPrepSpan()` in
`handleSleepingState()` and `resetSleepPrepSpanOnBoot()` in `setup()` are
both independently confirmed correct this pass and are unchanged.

This resolves Finding 2 as a direct side effect: with the only real
logging call site now inside `transitionTo()`, a structural test can
assert `State_Sleep.cpp` contains **zero** occurrences of the logging
helper anywhere - a complete, simple invariant (not a "is it inside this
one wrapper function" check) that directly catches Codex's exact
rogue-call mutation class.

### Fourth corrective dispatch

Approved: dispatch to Copilot. Scope: `Generalized-Core-Counter.cpp`'s
`transitionTo()` gets the new guard; `State_Sleep.cpp` reverts all 13
`exitSleepingState()` call sites to plain `transitionTo()` and removes
the wrapper function itself;
`tests/loop_stage_sleep_prep_exclusion_test.py` rewritten for the new
choke-point design (including the "zero occurrences in `State_Sleep.cpp`"
check); `tests/README.md` updated.
`tests/sleep_prep_span_timing_test.cpp`/`src/state/SleepPrepSpanTiming.h`
unchanged (already confirmed correct). Independent verification plus a
fresh, explicitly adversarial Codex Stage 7 pass required again before
this returns to Chip's final gate.

Once this pass verifies clean, this WO's Status section will include a
retrospective on the full four-round arc - three distinct failure-class
lessons (an accidental rate-limiter masking the original per-iteration
bug's true frequency; per-iteration loop-stage-tag churn defeating a
same-stage-transition guard; and enumeration of call sites losing to a
single universal choke point) - as a case study for this project's
future work generally, not just this fix.

Implemented and independently verified (diff review, 2 self-run mutation
tests, full pre-existing suite, independent compile matching Copilot's
numbers exactly). Sent to a fifth, adversarial Stage 7 pass.

## Fifth Corrective Pass (test-only - Stage 7 confirmed production code correct, found 3 remaining test-coverage gaps)

Codex's fifth pass gave the production design a clean bill of health:
"the choke-point correction is sound in the current source... I found no
bypass exists currently... The three prior counterexamples are fixed."
The `transitionTo()` choke point, `maybeStartSleepPrepSpan()`,
`closeSleepPrepSpan()`, and `resetSleepPrepSpanOnBoot()` are all
independently confirmed correct - no production code changed this pass.

Three test-coverage gaps remained, all mutation-tested and confirmed:

1. **The choke-point design's load-bearing claim - "`state` is written
   nowhere except `transitionTo()`" - was never actually enforced by a
   test**, only verified by hand (grep) at each dispatch. A direct
   `state = ERROR_STATE;` dropped in place of the OOM `transitionTo()`
   call bypasses the choke point entirely and is invisible to every
   other check.
2. The `State_Sleep.cpp` "zero occurrences" check only matched the
   helper's *name* - a hand-written
   `Log.info("LoopStage: stage=%s ...", ...)` call bypassing the helper
   entirely, planted directly in the gate-wait polling section,
   recreates the original log storm undetected.
3. The guard condition in `transitionTo()` could be silently narrowed
   (e.g. `&& newState != ERROR_STATE`), skipping the span-close for one
   specific target state, with nothing checking that the guard depends
   *only* on the old state and span-openness, never on the transition
   target.

Fixed directly (self-implemented, not dispatched to Copilot - test-only,
no production behavior change, same governance standard as the earlier
lighter test-hardening fixes in this WO's history):

- A new whole-codebase scan (`src/**/*.cpp`) asserting `state` is
  assigned nowhere except `transitionTo()`'s own `state = newState;`
  line - the choke-point claim is now mechanically enforced, not just
  verified by hand.
- The `State_Sleep.cpp` check broadened to also reject any raw
  `Log.info/warn/error("LoopStage: ...")` call, not just occurrences of
  the helper name.
- A new check asserting the guard condition never references `newState`.

All three verified via mutation testing against Codex's exact examples
(direct `state =` bypass at the OOM site; raw `Log.info` planted before
the cloud-gate polling return; `&& newState != ERROR_STATE` narrowing) -
all three now fail correctly, then reverted and re-confirmed clean. Full
pre-existing suite and both SLEEP_PREP test files re-run and passing; no
production file changed, so no recompile was required for correctness,
though the working tree remains identical to the already-compiled fourth
corrective pass state.

### Fifth corrective dispatch

A fresh, explicitly adversarial Codex Stage 7 pass is required regardless
of the "test-only" framing, per this WO's standing practice - the
production code was already confirmed correct this round, so this pass
is specifically to confirm the three new checks are sound and to make one
more genuine attempt at finding anything remaining before this returns to
Chip's final gate.

### Sixth Stage 7 pass: production code confirmed correct again; two more realistic test gaps fixed; two adversarial gaps deliberately left unaddressed (stated boundary, not a default)

Codex's sixth pass again found zero production-code defects ("I found no
current production-code defect... no alias, pointer, `memcpy`, `swap`,
compound-assignment, or other current bypass exists. The choke-point
guard, span start/close/reset functions, and logging placement remain
correct") - the third consecutive pass to confirm this. All findings were
against the three new tests added in response to the fifth pass.

Two are realistic and were fixed directly (both are things a normal
`clang-format` pass could produce, not contrived): a multi-line
`state =\n    ERROR_STATE;` assignment defeated the single-line
whole-codebase scan, and a multi-line-wrapped guard condition
(`if (state == SLEEPING_STATE &&\n    ...\n    && newState != ERROR_STATE) {`)
defeated the single-line `newState`-absence check. Both checks were
rewritten to operate on the full logical span (paren-matched for the
guard; comment/string-stripped and whitespace-flattened for the
whole-codebase scan) rather than one physical line, and re-verified via
mutation testing against exactly these two multi-line forms - both now
caught correctly.

Two are deliberately adversarial constructions, not realistic
organic mistakes, and are being left unaddressed as a stated decision:

1. **Pointer/reference aliasing** (`State &stateAlias = state; stateAlias
   = ERROR_STATE;`) bypasses the whole-codebase `state =` scan. Closing
   this fully would require tracking every possible alias/pointer to
   `state` through arbitrary indirection - equivalent to real alias
   analysis, not a text-level check.
2. **Adjacent string-literal concatenation** (`Log.info("Loop" "Stage: "
   "...", ...)`, a real, legal C++ feature) bypasses the raw-log-call
   scan in `State_Sleep.cpp`, which only matches a literal starting
   exactly with `"LoopStage:`.

Both would require genuine AST-level parsing (e.g. `libclang`) to close
completely - a regex/text-level test can always be evaded by a
sufficiently motivated author restructuring the same behavior into
different surface syntax. Building that level of tooling is
disproportionate to this WO: none of the five real regressions found
across this WO's history (the rate-limiter-masking bug, the
per-iteration tag-churn bug, the enumeration-vs-choke-point gap, or
either of the two realistic multi-line gaps just fixed) involved
anything resembling deliberate obfuscation - they were all structural
misunderstandings of the codebase's actual control flow, not adversarial
code. The two remaining gaps are qualitatively different: nobody
introduces a pointer alias or splits a log format string across two
literals by accident. Chip should be aware these two specific,
narrow bypasses exist and are not mechanically enforced, but closing
them is not proportionate given everything else this WO has verified.

All fixes (both realistic gaps, plus two trivial hygiene items Codex also
flagged - a stale "Fourth Corrective Pass" label in the test's final
print statement, and a trailing blank line at EOF in `tests/README.md`)
were made directly by Claude, mutation-tested against Codex's exact
examples, and re-verified against the full pre-existing suite. A fresh
compile was also run per Codex's provenance recommendation (Git alone
cannot prove no production file changed between passes, since all
corrective-pass work remains stacked in one uncommitted diff) - confirmed
byte-identical memory footprint to the fourth corrective pass (Flash
129786 / RAM 4528), independently confirming no production file actually
changed this round.

### Sixth corrective dispatch

A final Codex Stage 7 pass is requested to confirm the two realistic
fixes are sound and to make one more genuine attempt at finding anything
remaining, with the two adversarial gaps above disclosed up front as a
known, deliberate boundary rather than something to flag as newly
discovered.

### Seventh Stage 7 pass: production code confirmed correct a third consecutive time; two more realistic, cheaply-fixable test gaps closed; agreed the adversarial boundary is reasonable

Codex's seventh pass again found zero production-code defects and
explicitly endorsed the adversarial-boundary decision from the prior
round ("I agree that pointer/reference aliasing and adjacent
string-literal concatenation are reasonable disclosed boundaries...
neither should independently block the gate"). Two new, genuinely
realistic gaps were found in the two checks added last round - both
ordinary ways to write real C++, not contrived:

1. The whole-codebase `state =` scan only matched a bare
   identifier/dotted RHS and only scanned `*.cpp` files. Missed:
   `state = chooseErrorState();` (function-call RHS),
   `state = (ERROR_STATE);` (parenthesized RHS), and any direct
   assignment placed in a `.h` file (not scanned at all).
2. The choke-point logging-call check found only the first matching call
   and verified ordering/depth, but never cardinality or arguments -
   missed a duplicated call, `LOG_LEVEL_INFO` silently changed to
   `LOG_LEVEL_WARN`, the stage changed away from `LOOP_STAGE_SLEEP_PREP`,
   and the real `elapsedMs`/`queueDepth`/`millisSinceLastCloudConnect`
   fields replaced with literal zeros.

Both fixed directly by Claude (test-only again): the assignment regex
broadened to match any RHS terminated by `;` (excluding `==` via a
negative lookahead), and the scan extended to `src/**/*.h` as well as
`*.cpp` (68 files scanned, confirmed no false positives against real
source); the logging-call check extended to assert exactly one
occurrence of the helper call in `transitionTo()`, with an exact-argument
match against the real computed field names, not just presence of the
helper name.

All six of Codex's exact new counterexamples (the two assignment-RHS
forms, the header-file case, the duplicated call, the wrong log level,
the zeroed-out fields) independently mutation-tested and confirmed
caught, then reverted; a header-planted-assignment scenario mirroring
Codex's "State_Common.h" example was also independently constructed and
confirmed caught. Full pre-existing suite and both SLEEP_PREP test files
re-run and passing. A fresh compile again reproduced the identical Flash
129786 / RAM 4528 footprint - the third consecutive confirmation that no
production file has changed since the fourth corrective pass.

### Seventh corrective dispatch

A further Codex Stage 7 pass is requested - the disclosed
aliasing/string-concatenation boundary is now independently endorsed by
Codex and does not need to be revisited unless a fresh angle is found;
this pass is to confirm the two new fixes are sound and make one more
genuine attempt at anything remaining before this returns to Chip's
final gate.

### Eighth Stage 7 pass: first "Verified with concerns" - production confirmed correct a fourth consecutive time, one realistic gap left, closed

Codex's eighth pass again found zero production-code defects and, for
the first time, returned **Verified with concerns** rather than **Not
verified**. All seven of the specific mutations requested for
re-confirmation were caught correctly. One new, genuinely realistic
concern: the cardinality/ordering/argument checks added last round
operate on raw source text without stripping comments, so a
commented-out `logLoopStageLine(...)` call (Codex's framing: "commenting
out a call during debugging is ordinary") still satisfies every check -
removing the real log line from production while the test suite stays
green.

Codex also surfaced two much more marginal items, explicitly not treated
as blocking: a parenthesized-LHS assignment (`(state) = ERROR_STATE;`,
legal but unusually styled) would also miss the whole-codebase scan, and
a hypothetical unrelated `state` struct member elsewhere would falsely
trip it - confirmed not to currently exist anywhere in the 68 scanned
files. Both left unaddressed as further, smaller instances of the same
disclosed text-vs-AST boundary already accepted for the aliasing/
string-concatenation gaps - proportionality applies here too, not just
to the two originally-disclosed cases.

Fixed directly by Claude: `//` line comments are now stripped from the
`transitionTo()` body before any of Check 2/2b's pattern matching runs
(comment-stripping was already used elsewhere in this file for the
whole-codebase scan; this extends the same technique to this check).
Independently mutation-tested against Codex's exact commented-out-call
example - now correctly caught ("does not call the shared logging
helper"). Full pre-existing suite and both SLEEP_PREP test files re-run
and passing. A fresh compile again reproduced the identical Flash
129786 / RAM 4528 footprint - the fourth consecutive confirmation that no
production file has changed since the fourth corrective pass.

### Status: closed - ready for Chip's final gate

Eight consecutive Stage 7 passes, four of them confirming the production
implementation has zero defects (rounds five through eight); the other
four found and closed four real production bugs across the first four
rounds (the original log-volume regression, the loop-stage-tag-churn
regression, the HIBERNATE/reset-poisoning bug, and the
enumeration-vs-choke-point gap). Every realistic test-coverage gap found
along the way has been closed and mutation-tested against the exact
counterexample that found it. The two remaining known gaps (pointer/
reference aliasing, adjacent string-literal concatenation) are
deliberately, explicitly out of scope - independently agreed reasonable
by Codex - because closing them requires real AST-level parsing, and
neither resembles any of this WO's actual regressions.

Chip reviewed the "Verified with concerns" finding (round 8) directly,
confirmed the concerns are limited to the two already-disclosed,
Codex-endorsed adversarial test gaps above (not the production
implementation), and closed this WO without requesting a further Stage 7
pass. Ready for Chip's review of the complete uncommitted diff and
commit at his discretion - no further AI-driven changes pending.

## Retrospective

What looked like a one-line logging fix turned into eight independent
verification rounds and four real production bugs, each fixed by a
different-shaped design, before the fix actually worked. Recorded here
as a case study for this project's future work in general, not just this
one diagnostic line.

**1. An accidental rate-limiter had been masking the real bug's frequency
the whole time.** The original WO was scoped from a 23-cycle log sample
in which `LOG_STAGE_SLEEP_PREP`'s WARN/ERROR threshold happened to
suppress almost every intermediate log line, making the diagnostic look
like it fired once per sleep cycle. It didn't - it fired on every
`loop()` iteration during gate-wait polling; the threshold just happened
to keep `elapsed` too small to cross 2000ms most of the time. Removing
the threshold (the original fix's whole purpose) removed the accidental
suppression along with it, and the true per-iteration call frequency
only became visible on real hardware, post-merge. **Lesson:** when a
fix's evidence is a log sample gathered under the mechanism the fix is
about to remove, verify what that mechanism was *also* incidentally
doing before trusting the sample's shape.

**2. A same-stage-transition guard looked correct in isolation but broke
against the surrounding loop's own bookkeeping.** The first corrective
design (`setLoopStage()`'s same-stage guard) is a real, working pattern
- it's just that `loop()` retags the stage through several unrelated
values every single iteration (`CONNECTIVITY -> STATE_HANDLER -> ... ->
DIAGNOSTICS -> ...`) regardless of what the dispatched handler did, so
"the stage changed" and "the handler is really done" turned out not to
be the same event. **Lesson:** a guard that's provably correct about the
narrow mechanism it inspects can still be wrong about what that
mechanism is coupled to elsewhere in the same call graph - trace the
*caller's* surrounding behavior, not just the callee's own logic, before
trusting a "this only fires on a real transition" claim.

**3. Enumerating call sites is never actually complete; a single choke
point is.** The second corrective design (`exitSleepingState()` wrapping
13 known exit points) was defeated by three `transitionTo()` calls
living entirely outside the file being edited - out-of-memory handling,
user-switch handling, and the connectivity failsafe supervisor, all in
`Generalized-Core-Counter.cpp`, none related to sleep management on the
surface. The fix that actually held was moving the check into
`transitionTo()` itself, the one place `state` is ever written -
provably complete because it's structural (nothing can write `state`
without passing through it), not because every call site was found by
searching. **Lesson:** when "did X happen" can be asked from multiple
call sites across a codebase, look for the one place that's guaranteed
to see every occurrence before building a solution around a list of the
occurrences you found by searching.

**4. HIBERNATE-triggered resets don't `return` - and a text-search-based
inventory of reset paths can't see semantically implicit ones.** The
third corrective pass's HIBERNATE bug came from the same enumeration
weakness as #3, in miniature: an inventory built by grepping for the
literal string `System.reset()` correctly found two explicit calls, but
missed a third, more common one - `System.sleep()` with
`SystemSleepMode::HIBERNATE`, which resets the MCU as its *normal
success path* without any literal reset call visible at the call site.
**Lesson:** "does X happen" and "does the text 'X' appear" are different
questions; a grep-based inventory of a category of events (resets,
mutations, side effects) needs to be checked against what the platform
*does*, not just what the source *says*.

**5. Presence checks are not the same as reachability, cardinality, or
argument-correctness checks - and this exact gap recurred three separate
times.** Across the fifth through eighth Stage 7 passes, three
structurally identical mistakes were each found once, fixed, and then a
slightly different variant of the same category was found again: a check
that a logging helper's *name* appeared somewhere in a function's text
(not that it was called exactly once, unconditionally reachable, with
the right arguments, or was even live code rather than a comment);
a state-assignment scanner that matched a bare identifier but not a
function call, a parenthesized expression, or a header file; a
multi-line-formatted guard condition that a single-physical-line check
couldn't see. **Lesson:** a regression test's job is to encode an
*invariant* ("this can only happen once," "this can only be written
here," "this condition can't depend on X") - stating the invariant in
English is not the same as writing a check that actually enforces it
under reformatting, and it's worth explicitly asking "what's the
invariant, not just the example" before considering a regression test
done.

**6. Text-level verification of C++ has a real, principled stopping
point - name it, don't discover it by accident.** The final two rounds
surfaced bypasses (pointer/reference aliasing, adjacent string-literal
concatenation) that no reasonable amount of regex tuning can fully
close - only genuine AST-level parsing can. Rather than chase an
unbounded sequence of ever-more-exotic counterexamples, the boundary was
made explicit and disclosed in this document, and independently reviewed
and endorsed by Codex as reasonable given none of this WO's actual
regressions resembled deliberate obfuscation. **Lesson:** proportionality
is a real engineering judgment, not just an excuse to stop early - but
it needs to be *stated*, with reasoning, and ideally checked against
someone independent, rather than silently limiting scope and hoping it
doesn't come up.

**On the verification process itself:** every one of the first four
findings was caught by Codex, not by the design/implementation process
that produced the bug - Stage 7 did exactly the job it exists to do.
Independent verification (Claude's own diff review, direct mutation
testing of every fix before trusting it, and an independent compile
matching the implementer's reported numbers every round) caught nothing
Stage 7 had not already found, but did catch one arithmetic error in the
WO's own call-site count (15 claimed, 14 real) before it could compound
into a wrong test assertion - a reminder that even the investigation
documents driving a fix need the same scrutiny as the fix itself.
