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

Drafted from a fully diagnosed investigation, ready to dispatch to Copilot
for implementation (not self-implemented — genuine behavior change to
existing logging escalation, same governance standard applied throughout
this project). Given this is observability/diagnostic-only, not
safety-critical decision logic, a lighter but still genuine Codex
verification pass is appropriate before commit — confirm the exclusion
works and nothing else regressed, doesn't need the full WO-002/003-style
multi-round rigor given the narrow, precisely-diagnosed scope.
