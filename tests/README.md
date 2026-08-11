# Host regression tests

The reporting-policy calculation is isolated from Particle Device OS so its
battery tiers, effective intervals, boundary alignment, and open-window filter
can be checked with the host compiler:

```sh
c++ -std=c++11 -Wall -Wextra -Werror -Isrc \
  tests/reporting_policy_test.cpp src/reporting/ReportingPolicy.cpp \
  -o /tmp/reporting_policy_test
/tmp/reporting_policy_test
```

The runtime adapter is validated by the normal Particle firmware compile.

The boot-scoped startup snapshot has a separate host test:

```sh
c++ -std=c++11 -Wall -Wextra -Werror -Isrc \
  tests/startup_snapshot_test.cpp src/observability/StartupSnapshot.cpp \
  -o /tmp/startup_snapshot_test
/tmp/startup_snapshot_test
```

## WO-2026-08-11-001 (SLEEP_PREP threshold exclusion) host test

`noteLoopStageDuration()`/`setLoopStage()`/`transitionTo()`
(`src/Generalized-Core-Counter.cpp`) and `handleSleepingState()`
(`src/state/State_Sleep.cpp`) pull in heavy Particle/PublishQueuePosix
dependencies and can't be compiled standalone on the host. This test parses
the actual shipped source directly to confirm the corrected design:

- `LOOP_STAGE_SLEEP_PREP` is still excluded from the
  `kLoopStageWarnThresholdMs`/`kLoopStageErrorThresholdMs` WARN/ERROR
  escalation inside `noteLoopStageDuration()` (a bare early return, before
  the threshold comparisons, with no logging call of its own).
- `setLoopStage()` is fully reverted to its simple original form (same-stage
  early-return guard, then unconditional overwrite of
  `lastLoopStage`/`stageStartMillis`), with no SLEEP_PREP-specific logic of
  any kind. An earlier corrective pass tried putting the SLEEP_PREP exit-log
  here, gated on `setLoopStage()`'s own same-stage guard - broken, because
  `loop()` retags the loop stage through a fixed sequence every iteration
  regardless of what the dispatched state handler did, so the guard fired
  the exit-log on nearly every gate-wait polling iteration (a log-storm
  regression under a different call site than the original bug).
- **The `INFO`-level `LoopStage:` line for `SLEEP_PREP` is logged from a
  single universal choke point: `transitionTo()`
  (`src/Generalized-Core-Counter.cpp`).** The codebase's `state` FSM
  variable is written in exactly one place in the whole codebase -
  `transitionTo()` - so every transition, from any call site in any file,
  passes through it. Before overwriting `state`, `transitionTo()` checks
  whether the OLD state is `SLEEPING_STATE` and a SLEEP_PREP span is
  currently open (`retainedLoopForensics.sleepPrepSpanStartMillis != 0`); if
  so, it closes the span (`closeSleepPrepSpan()`) and logs the one INFO line
  (`logLoopStageLine()`) before proceeding with the state change and its own
  `"StateReq: ..."` log line.

  An earlier corrective pass instead used a local wrapper function,
  `exitSleepingState()`, that all 13 real exit points inside
  `handleSleepingState()` called instead of `transitionTo()` directly. That
  design rested on an unstated, false assumption: that every real exit from
  `SLEEPING_STATE` happens via a `transitionTo()` call from *within*
  `handleSleepingState()`. Stage 7 found three live counterexamples that
  bypass any such wrapper entirely: out-of-memory handling and
  user-switch-press handling in `loop()` (both call `transitionTo()`
  directly after the state-handler switch), and
  `connectivityFailsafeSupervisor()`'s failsafe-stage-1 action (called
  *before* the switch even runs). None of these route through a
  `State_Sleep.cpp`-local wrapper; if any fires while a SLEEP_PREP span is
  open, the span would never close under that design - poisoning the next
  real dwell's elapsed computation. Moving the check into `transitionTo()`
  itself closes this gap completely and by construction: it applies to
  every exit from `SLEEPING_STATE`, regardless of which file or function
  calls `transitionTo()`.

  `exitSleepingState()` has been removed entirely; all 13 call sites in
  `State_Sleep.cpp` are plain `transitionTo()` calls again. Two
  `System.reset()` paths (the nightly heap-guard reset and the
  all-sleep-attempts-failed reset) never routed through `transitionTo()` (or
  the old wrapper) in the first place, since `System.reset()` itself doesn't
  return - both already carry more precise purpose-built `Log.warn`/
  `Log.error` diagnostics, and the device reboots regardless. The
  dead/unreachable `transitionTo(ERROR_STATE, "sleep-all-attempts-failed")`
  line that follows the second reset is also left untouched, out of scope.

  A non-negotiable structural test confirms `State_Sleep.cpp` contains
  **zero** occurrences of the shared logging helper name anywhere in the
  file (not scoped to any function) and **zero** occurrences of the
  identifier `exitSleepingState` - this directly catches a rogue logging
  call planted anywhere in the file (e.g. directly before the cloud-gate
  polling `return;`), which a wrapper-scoped presence check could not.
- A dedicated `RetainedLoopForensics.sleepPrepSpanStartMillis` field (new;
  `kLoopForensicsVersion` bumped 1 -> 2 since this is `retained` RAM) tracks
  the elapsed-time reference for the whole span - NOT the pre-existing
  `stageStartMillis`, which stays contaminated by loop-stage tag churn. It
  is set with a **zero-gated** check via `maybeStartSleepPrepSpan()`, not an
  `enteredState`-gated one: two of the 13 real exit paths transition
  `SLEEPING_STATE -> SLEEPING_STATE`
  (`sleep-timer-occupied-suppress-report`, `sleep-pir-return-to-sleep`) - a
  real cycle ends and a new one begins, but the FSM `state` value doesn't
  change, so an `enteredState`-gated reset would never fire for the new
  cycle and would silently report `elapsed=0`. `transitionTo()`'s
  choke-point guard reads the OLD `state` value (before the assignment), so
  it still correctly fires on a `SLEEPING_STATE -> SLEEPING_STATE`
  self-transition.
- A shared `logLoopStageLine()` helper still backs both
  `noteLoopStageDuration()`'s WARN/ERROR lines and `transitionTo()`'s
  SLEEP_PREP `INFO` line, so the five-field format string
  (`stage`/`elapsed`/`state`/`q`/`connMs`) can't drift between the two.
- Other stages' WARN/ERROR threshold behavior is unchanged.

It then runs a call-frequency/boundedness simulation matching the real
control flow: repeated polling calls with OTHER loop-stage tags retagged in
between each one (mirroring `loop()`'s actual per-iteration behavior),
proving the design is unaffected by that churn because it no longer depends
on loop-stage tags, or on which function/file calls `transitionTo()`, at
all - only on whether `state == SLEEPING_STATE` and a span is open at the
moment ANY `transitionTo()` call fires. A dedicated scenario models an
"external" transition (representing the OOM/user-switch/failsafe-stage-1
case from outside `handleSleepingState()` entirely) and proves it still
closes an open span correctly - this is the specific regression the
wrapper-based design could not catch.

A dedicated scenario also proves the zero-gated reset correctly starts a
fresh span for a second real cycle immediately following a
`sleep-pir-return-to-sleep`-style `SLEEPING_STATE -> SLEEPING_STATE`
self-transition - elapsed for the second cycle is neither `0` nor inclusive
of the first cycle's duration.

The test was mutation-tested against both of Stage 7's exact findings:
(a) temporarily removing the guard from `transitionTo()` (models an
external transition bypassing the choke point) makes the test fail; (b)
temporarily inserting a rogue `logLoopStageLine(...)` call directly in
`State_Sleep.cpp`'s cloud-gate polling section (before its `return;`) makes
the "zero occurrences in `State_Sleep.cpp`" check fail. Both mutations were
reverted after confirming.

```sh
python3 tests/loop_stage_sleep_prep_exclusion_test.py
```

## WO-2026-08-11-001 (SLEEP_PREP threshold exclusion, Third Corrective Pass) host tests

Stage 7 found two more issues on this WO after the Second Corrective Pass:
(1) `retainedLoopForensics.sleepPrepSpanStartMillis` survives a HIBERNATE
success reset (an implicit reset inside `System.sleep(config)` with
`SystemSleepMode::HIBERNATE`, not a syntactically visible `System.reset()`
call) and the two explicit `System.reset()` paths, poisoning the elapsed
computation for the next real `SLEEPING_STATE` dwell after a normal
overnight reboot; (2) the Second Corrective Pass's own regression test for
`exitSleepingState()`'s logging call was a presence check (does the helper
name appear anywhere in the wrapper's text), not a count/reachability
check, so it passed unchanged when Codex's Stage 7 mutated the wrapper to
log the SLEEP_PREP line twice, and also when it wrapped the logging call
in `if (false) { ... }` so it never logs at all.

Fix 1: `setup()` (`src/Generalized-Core-Counter.cpp`) now calls
`resetSleepPrepSpanOnBoot(retainedLoopForensics.sleepPrepSpanStartMillis)`
unconditionally on every boot, regardless of `System.resetReason()` -  any
real reboot invalidates any in-flight span by definition, since the call
stack that would have closed it via `closeSleepPrepSpan()` is gone.

Fix 2: the span start/close/boot-reset arithmetic - the exact logic all
three of this WO's regressions have lived in - is extracted into a new,
dependency-free header, `src/state/SleepPrepSpanTiming.h` (`<cstdint>`
only, no `Particle.h`/`AB1805_RK.h`/other project headers), defining three
pure inline functions: `maybeStartSleepPrepSpan()` (the zero-gated start,
used by `handleSleepingState()`), `closeSleepPrepSpan()` (computes
elapsed, resets to 0, used by `exitSleepingState()`), and
`resetSleepPrepSpanOnBoot()` (the new boot-time fix, called from
`setup()`). A new test, `tests/sleep_prep_span_timing_test.cpp`,
`#include`s this header directly and compiles/runs on the host with zero
stubbing, exercising the *real* production functions instead of a
hand-written simulation - closing the class of gap that recurred three
times for this specific logic. It covers a normal span; the
`SLEEPING_STATE -> SLEEPING_STATE` self-transition case (two spans back to
back through the same zero-gate); and the HIBERNATE-poisoning scenario as
a genuine before/after regression proof (a stale nonzero value survives a
`maybeStartSleepPrepSpan()` call made without first calling
`resetSleepPrepSpanOnBoot()`, proving the bug would occur, then is
correctly cleared once `resetSleepPrepSpanOnBoot()` is called, letting the
next span start fresh).

```sh
c++ -std=c++11 -Wall -Wextra -Werror -Isrc \
  tests/sleep_prep_span_timing_test.cpp \
  -o /tmp/sleep_prep_span_timing_test
/tmp/sleep_prep_span_timing_test
```

`tests/loop_stage_sleep_prep_exclusion_test.py` was subsequently rewritten
again in the Fourth Corrective Pass (see the section above) once Stage 7
found the wrapper-based design (`exitSleepingState()`, enumerating 13 call
sites in `State_Sleep.cpp`) itself missed real `SLEEPING_STATE` exits from
outside `handleSleepingState()` entirely. The current test asserts the
choke point lives in `transitionTo()` instead, and that `State_Sleep.cpp`
contains zero occurrences of the logging helper or `exitSleepingState`
identifier anywhere in the file - see above for the current, up-to-date
description.

```sh
python3 tests/loop_stage_sleep_prep_exclusion_test.py
```
