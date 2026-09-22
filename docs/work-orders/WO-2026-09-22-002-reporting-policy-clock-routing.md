# WO-2026-09-22-002: `RuntimeReportingPolicy` - route report-scheduling trust through `Clock`, not around it

**Type:** Code-defect fix (decision-site conversion, same family as Step 3b's
sweep).

**Status:** CLOSED 2026-09-22. Code complete, fully tested on the host
(mutation-tested structural test plus a truth-table unit test), clean local
ARM-toolchain and cloud builds, flashed clean to Dev-09/Dev-11/Dev-14 with no
regression observed. Acceptance standard for this WO is **host coverage plus
a clean flash** - see "Bench validation and its honest limit" below for why,
and what remains open.

## The observation

`RuntimeReportingPolicy.cpp:45` set `ReportingPolicyInputs.timeValid =
Time.isValid()`, which `ReportingPolicy.cpp:32` used to decide whether a
report-scheduling boundary could be computed at all. This is a real
decision that commits the device to behavior (whether/when to next connect
and report) resting on raw epoch-existence rather than `Clock::isTrusted()`'s
trust verdict - the same class of gap Step 3b's ~13-site decision-site sweep
closed everywhere else, missed here because this file was not in that
sweep's original list. Found during a follow-up investigation prompted by a
second review pass, alongside a sibling `Time.isValid()` site in
`MyPersistentData.cpp` (see WO-2026-09-22-001).

## Circularity check (done first, before any design work)

Per this project's own standing rule for any conversion in this family - a
decision routed through `Clock::isTrusted()` must not be able to disable the
one path that would let trust be restored (`connectivityFailsafeSupervisor()`'s
own deliberate exception exists for exactly this reason).

**No hard circularity found.** Two independent safeguards already exist,
neither of which depends on `ReportingPolicy`:

- The first sync of any boot is guaranteed by a dedicated `setup()`-time gate
  (`neverConfirmedSyncEver` / `!Clock::isTimeValid()`,
  `Generalized-Core-Counter.cpp`) that forces `CONNECTING_STATE`
  unconditionally on a never-synced device.
- A device that loses trust mid-operation is still backstopped by
  `connectivityFailsafeSupervisor()`, which forces reconnection on its own
  schedule, independent of reporting cadence, and is itself already
  deliberately exempted from `Clock::isTrusted()` for this same reason.

A concrete finding sharpened the case for fixing this anyway: today's
un-converted behavior is not a safe default, it is an accident.
`isBoundaryDue(0, 3600, 30)` (an invalid/zero epoch against the default
hourly interval) evaluates to `true` by construction - an invalid clock
already makes `cadenceDue` unconditionally true today, via modulo arithmetic
coincidence, not a deliberate choice.

## The fix

- `ReportingPolicyInputs.timeValid` renamed to `clockTrusted`
  (`reporting/ReportingPolicy.h`) - the field now documents that it carries
  a trust verdict, not raw epoch-existence.
- `RuntimeReportingPolicy.cpp` now sets it from `Clock::isTrusted()`
  directly - confirmed the existing `Clock::isTrusted()`/`Clock::openness()`
  surface already answers the question this caller needs; no new Clock
  surface was added.
- `ReportingPolicy::resolve()` (`reporting/ReportingPolicy.cpp`): an
  untrusted clock now resolves `cadenceDue` to **due-now** (still subject to
  the existing, untouched `windowOpen` gate) rather than deferring.
  Documented in-line as the reporting-domain analogue of Step 3b's "short
  nap, don't commit to overnight" design principle, applied with the
  opposite polarity from 3b's own fail-to-Unknown choice: in the
  sleep-duration domain, over-committing to a long sleep on a wrong clock is
  the costly mistake; in report cadence, under-connecting is the costly
  mistake, since skipping a connect also skips the one thing
  (`checkClockResync()`'s connected-side resync) that could restore trust.
  `nextReportEpoch` still correctly stays `0` when untrusted - an unconfirmed
  epoch cannot support computing a real future boundary either way.

## Folded in: `MyPersistentData.cpp`'s sibling site

Investigating this alongside the flagged sibling `Time.isValid()` site found
it was not a live decision at all: `currentStatusData::validate()` runs
exactly once per boot, from `.load()`, called over 150 lines before
`ab1805.setup()` ever seeds `Time` - so the branch it guarded could never be
true regardless of which signal gated it. Removed as dead code with an
explicit comment; not converted, since converting to `Clock::isTrusted()`
would have been equally always-false. Relocating the underlying
future-dated-timestamp safety check (if it's still wanted) is filed
separately as `WO-2026-09-22-001`, kept out of this WO's diff.

## Verification

- **Structural test**, mutation-tested (reintroduced `Time.isValid()`,
  confirmed the test failed for the right reason, restored byte-identical
  via sha256): `tests/reporting_policy_clock_owner_structural_test.py`.
- **Truth-table unit tests** (`tests/reporting_policy_test.cpp`,
  `tests/reporting_policy_adapter_test.cpp`, the latter through the real
  `resolveRuntime()` adapter, not `resolve()` in isolation): trusted clock
  keeps the existing boundary-alignment behavior unchanged; untrusted clock
  with the window open resolves due-now with `nextReportEpoch == 0`;
  untrusted clock with the window closed still correctly defers (untrusted
  does not override `windowOpen`).
- Full host suite: 36/36.
- Local ARM-toolchain build: clean, 149200/1086/2324.
- Cloud compile: clean, 150362/3418 (-144 bytes flash vs. the prior
  baseline, RAM unchanged - consistent with a dead-code removal plus a small
  logic addition).

## Bench validation and its honest limit

Flashed clean to Dev-09, Dev-11, and Dev-14. All three came up with
`clock.trusted: true`, low `syncAgeSec`, and a normally-aligned
`reporting.nextReportEpoch` - no regression in the trusted path, which is
the only path a routine flash-and-reboot actually exercises.

**Acceptance standard for this WO is host coverage plus a clean flash, not a
live on-device trigger of the untrusted branch - stated plainly, not
glossed over:** the untrusted-clock behavior this WO changes cannot be
reached by a normal boot. The pre-existing `neverConfirmedSyncEver` gate in
`setup()` already forces a connect and establishes trust within seconds on
any fresh flash, so the fallback path this WO adds was never actually
exercised on hardware during this validation. Reaching it for real requires
a device to go roughly 24 hours (`ClockTrust::kMaxSyncAgeMs`) without a
confirmed resync - not something a bench check can produce on demand
without a deliberate extended disconnection, which was not staged for this
WO's closure.

**The logic itself is pinned by host truth-table tests; on-device
confirmation of the untrusted branch specifically is pending a natural
extended-disconnection event, expected opportunistically during ongoing
soak, not yet observed.** If one of the three bench devices goes quiet for
an extended stretch during normal soak operation, that is the moment to
check this WO's actual field behavior - not before.

## Provenance

Implemented, tested, and bench-flashed 2026-09-22 on branch
`wo/2026-09-22-reporting-clock-routing`, commit `b6730ba`.

## Related work orders

- `WO-2026-09-22-001-occupancy-start-time-clamp-unreachable.md` - the
  sibling `Time.isValid()` finding folded in above, filed separately to keep
  this WO's diff scoped.
- `WO-2026-09-19-001-clock-trust-standard.md` (Step 3b) - the decision-site
  conversion sweep this WO extends to a site that sweep missed.
