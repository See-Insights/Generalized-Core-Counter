# WO-2026-08-12-001: Network-keep-alive visibility and modem-teardown confirmation logging

## Context (revised — original premise was partially wrong, corrected here)

This week's NCP/connect-timeout investigation initially assumed
`HIBERNATE` and the alert-remediation reset paths never perform any
modem teardown before proceeding. Follow-up investigation found this is
only true at the narrow code-block level — `handleSleepingState()` has a
mandatory precondition gate (`sleepPreconditionsSatisfied()`,
`State_Sleep.cpp:862-950`) that runs *before* both the `HIBERNATE` branch
and both reset paths. For the non-standby case (overnight/closed-hours,
exactly when `HIBERNATE` applies), this gate requires
`!Particle.connected() && !Connectivity::isRadioPoweredOn()` before
execution can proceed at all — already calling
`Connectivity::requestRadioPowerOff()` (the same `Cellular.off()` TAN004
recommends) and polling to confirm completion within its own budget. If
the gate can't be satisfied, the function diverts to `ERROR_STATE`
instead of ever reaching `HIBERNATE` or either reset path.

So in the normal/successful control flow, radio is very likely already
off by the time these paths run — inherited from this earlier gate, not
because `HIBERNATE`/the reset paths do their own teardown. **This WO no
longer proposes adding a new teardown sequence to those paths** — doing
so would be redundant at best, and carries a real, separately-confirmed
risk: a naive `waitFor(Cellular.isOff, 30000)` placed before
`ab1805.stopWDT()`/`pauseAwakeWatchdogForSleep()` would not be seen by
`serviceAwakeWatchdog()` (confirmed: `waitFor()` has no knowledge of this
app's watchdog-refresh mechanism), risking exactly the watchdog-reset
failure mode this investigation has spent multiple WOs characterizing.
**Not fixing this before Friday given that risk** — this needs careful,
unhurried design, not a pre-vacation rush.

## Fix (narrowed scope — further narrowed, item 1 dropped)

1. ~~Add network-keep-alive state to logging.~~ **Dropped.** Verified
   directly against source: `Sleep: ULP standby=%d reason=%s dur=%ds
   occ=%d soc=%.1f` (`State_Sleep.cpp:1333`, unconditional `Log.info`)
   already logs exactly this, and an existing invariant check
   (`:1197-1206`) already warns if standby state looks inconsistent with
   actual connection state. `HIBERNATE`/`STOP` configs never call
   `.network(...)` (confirmed earlier), so standby is structurally always
   off for those paths — there is nothing missing to add. Chip confirmed:
   drop this item rather than dispatch a redundant/no-op change.

2. **Add confirmation logging, not new teardown behavior, to `HIBERNATE`,
   the two `State_Sleep.cpp` reset paths, and the `ERROR_STATE` soft-reset
   path (`State_Error.cpp:151-158`, Case 2).** The `ERROR_STATE` path was
   found separately (Chip, reviewing `State_Error.cpp`) and confirmed to
   have the same shape of gap: `Connectivity::requestFullDisconnectAndRadioOff()`
   already fires unconditionally on `ERROR_STATE` entry
   (`State_Error.cpp:117-122`, with its own explicit pre-existing safety
   comment), and Case 2's `System.reset()` only fires after
   `resetWait = 30000` ms (`Generalized-Core-Counter.cpp:695`) — generous
   margin, but still a fire-and-forget request with no confirmation poll
   before the reset actually fires, same as the other three points. Case 3
   (`ab1805.deepPowerDown()`, a full hardware power-cycle rather than a
   soft MCU-only reset) is likely a different situation - not included
   here, out of scope unless a specific reason emerges to revisit it. This
   is now the WO's entire scope (four points total). Log whether the radio
   was actually off (per `Connectivity::isRadioPoweredOn()`) at the point
   each path executes — this gives the visibility TAN004's spirit wants
   (confirming graceful shutdown happened) without touching working
   behavior or introducing new blocking calls near the watchdog. If this
   logging ever shows radio-on at one of these points, *that's* the
   signal a real gap exists and needs its own properly-scoped fix — not
   something to guess at and pre-emptively patch now.

## Explicitly deferred, not part of this WO

- **The `ERROR_STATE` edge case**: what happens if the precondition gate's
  own budget expires and diverts there — does that path eventually reach
  `HIBERNATE` or a reset *without* the radio-off guarantee? Not yet
  mapped. Worth its own investigation, not assumed either way.
- **Any actual new teardown sequence**, if item 2's logging ever reveals
  one is needed — must account for the watchdog-timing constraint
  explicitly (any blocking wait added near these paths must be placed
  correctly relative to `ab1805.stopWDT()`/`pauseAwakeWatchdogForSleep()`,
  or use this app's own polling pattern instead of `waitFor()`, which
  doesn't feed the watchdog). Fold into Phase 2 or its own WO once
  there's evidence it's actually needed.
- Incident 2 (full 660s timeout, unrecovered) — unchanged from original
  scope, still open, still not explained by this thread.

## Acceptance Criteria

- Radio-off confirmation logged at `HIBERNATE` entry, both
  `State_Sleep.cpp` reset paths, and the `State_Error.cpp` Case 2
  soft-reset path — four points total — without adding any new blocking
  call.
- No behavior change to existing `HIBERNATE`/reset/`ERROR_STATE` timing.

## Required Tests

- Confirm radio-off confirmation logging fires correctly at all four
  points and doesn't introduce any new delay (should be a read of
  existing state, not a new wait).
- Regression: no change to `HIBERNATE`/reset/`ERROR_STATE` control flow,
  including the existing `Connectivity::requestFullDisconnectAndRadioOff()`
  call and `resetWait` timing in `State_Error.cpp`.

## Permitted Files

`src/state/State_Sleep.cpp` and `src/state/State_Error.cpp` for the
production change. No `Connectivity.h`/`.cpp` changes expected — no new
teardown call is being added, and item 1 (which would have touched
`PowerDiag` logging) was dropped as already satisfied. A new host test
(`tests/modem_teardown_confirmation_logging_test.py`) and a required stub
addition to the pre-existing `tests/nightly_heap_guard_flush_test.sh`
(needed because that test extracts and compiles a real braced block from
`State_Sleep.cpp` containing the new `Connectivity::isRadioPoweredOn()`
reference) are expected companions per the Required Tests section above —
noted explicitly here after a Stage 7 pass flagged the omission as a
scope-documentation gap, not a scope violation.

## Status

Narrowed after investigation found the original premise was only true at
the code-block level, not the practical runtime level — and found a
separate, real risk in the originally-proposed fix (watchdog-timing
hazard from a naive `waitFor()` placement). This is a much smaller,
lower-risk WO than originally scoped: logging only, no behavior change.
Reasonable to include before Friday given the reduced scope, but the
deferred items above stay deferred — do not let this WO's simplicity
create pressure to also quickly resolve the `ERROR_STATE` edge case or a
real teardown fix before the deadline.

Dispatched to Copilot for implementation (not self-implemented — this
code region, despite being logging-only, is the same sleep-entry/
watchdog-adjacent path that WO-2026-08-10-001 and WO-2026-08-11-001 spent
multiple corrective rounds on; same governance standard applies).

Implemented in two Copilot rounds: the original three points in
`State_Sleep.cpp` (`hibernate`, `heap-guard-reset`,
`sleep-attempts-failed-reset`), then a follow-up round adding the fourth
point Chip found separately in `State_Error.cpp`
(`error-state-soft-reset`, Case 2) once the first round was independently
verified clean. Both rounds independently verified by Claude: diff
review, the new/extended test run directly plus mutation-tested against
two fresh mutations per round (all four caught correctly, reverted), the
full pre-existing suite (14 suites, including two Copilot itself couldn't
get running due to a missing-flag issue on its end — confirmed via direct
run with the correct flags from `tests/README.md` that this was a
tooling gap, not a real regression), and an independent compile matching
Copilot's reported numbers exactly both rounds (Flash 130642/RAM 3680,
then Flash 130714/RAM 3608).

A genuine Codex Stage 7 pass over the full combined diff (both files, all
four points) is running next before this returns to Chip's final gate,
matching the standard applied to this code region throughout the week.

## Stage 7 findings and fixes (test-only, no production behavior change)

Codex returned **Verified with concerns** — the production implementation
itself was confirmed correct on all four points (pure reads, zero control-
flow/timing/breadcrumb changes, `case 0`/`case 3`/`default` in
`State_Error.cpp` untouched, `Connectivity.h`/`.cpp` untouched). Two real
design observations, both already correctly scoped, not fixes needed:

- The `sleep-attempts-failed-reset` point (`State_Sleep.cpp:1421`) is
  reachable during open-hours standby, where the precondition gate only
  requires cloud-disconnect, not radio-off — so `radioOn=1` is genuinely
  possible there. This doesn't invalidate the logging; it's exactly the
  scenario this diagnostic exists to make visible, not a bug in the fix.
- `ERROR_STATE` Case 2's 30-second delay is a reasonable nominal
  allowance (matches TAN004's own example) but not a hard guarantee -
  Particle's own docs describe waiting up to 60s for `Cellular.isOff()`,
  and this app's own modem-off budget allows up to 120s
  (`ConnectivityPolicy.h:192`). Real, worth being aware of, correctly
  left deferred per this WO's explicit scope (diagnostic only, no new
  teardown/wait behavior).

One real regression-test gap, fixed directly by Claude (test-only, same
governance pattern as the rest of this WO's self-implemented test
hardening): the original test's checks were presence/adjacency-only and
missed three mutations Codex demonstrated in memory - wrapping a log call
in `if (!Connectivity::isRadioPoweredOn())` (which would silently
suppress the diagnostic exactly when the radio is on - the one case it
exists to catch), commenting out a log line, and a rogue blocking call
inserted after (not before) a log line. Fixed by: stripping `//` line
comments before any matching (mirroring the same technique used
repeatedly in WO-2026-08-11-001's test suite), adding an
indentation-depth-equality check between each new log line and its
anchor line (proving the log call isn't wrapped in its own new
conditional), and widening the blocking-call-token window to check after
each log line, not just before. All three of Codex's exact mutations
independently reproduced against the real source and confirmed now
caught, then reverted; full 14-suite pre-existing test run and an
independent compile re-confirmed clean afterward.

A documentation-only gap was also fixed: the Permitted Files section
didn't explicitly mention the new/extended test files as expected
companions, even though the Required Tests section clearly implies them
- updated above.

Given the concerns raised were fully addressed (one real test gap fixed
and reverified, two design observations confirmed correctly scoped as
already-deferred, one doc gap closed) and the production implementation
itself required zero changes, this is ready for Chip's final gate without
requiring a further Stage 7 round - but one is available if Chip wants
final confirmation given this code region's history this week.
