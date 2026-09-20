# WO-2026-09-19-001: One trust standard for sleep/connect/report decisions

**Type:** Behaviour-changing fix (Step 3b of the Structural Ownership Map
roadmap - the roadmap's only behaviour-changing step).

**Status:** CLOSED 2026-09-20. Code complete, fully tested on the host and
via cloud compile (Boron, target 6.4.1), and bench-accepted on Dev-14 - see
Bench acceptance below, including an explicit statement of what was and was
not field-proven.

## The observation

`Time.isValid()` is seeded true by `ab1805.setup()` from whatever the RTC
holds - including a clock that is hours wrong - so a decision resting on it
rests on "an epoch exists," not "this epoch can be trusted." Step 3a
introduced `Clock::isTimeValid()` as a deliberate, semantics-preserving
wrapper around exactly that (no behaviour change), re-pointing ~15 raw call
sites at it. `isClockTrusted()` already existed, was already correct, and
had six call sites - all telemetry-only. Nothing that decided anything
consumed it. `isWithinOpenHours()`'s fail-open behaviour (returns `true` on
invalid time or unloaded config) meant a plausible-but-wrong RTC-seeded
clock could compute a wrong CLOSED verdict and commit the device to an
overnight hibernate based on a wrong belief about what time it is - the
"clock correct in, overslept anyway" class of finding this roadmap exists to
close out.

## The fix

`Clock::isTrusted()` (a thin wrapper around the already-correct
`isClockTrusted()`) and `Clock::openness()` (a new `Open`/`Closed`/`Unknown`
tri-state, never fails open) replace `Clock::isTimeValid()`/
`isWithinOpenHours()` at every decision site that commits the device to
state:

- **14 decision sites converted**, across `Generalized-Core-Counter.cpp`,
  `state/State_Common.h`, `state/State_Error.cpp`, `state/State_Idle.cpp`,
  `state/State_Report.cpp`, `state/State_Sleep.cpp`. Split into two groups
  by what each site actually asks: plain trust ("can I do wall-clock
  arithmetic with this instant") routes through `Clock::isTrusted()`;
  open/closed decisions route through `Clock::openness()`.
- **The behavioural core**: `handleSleepingState()`'s night-sleep-vs-nap
  decision (`State_Sleep.cpp`) now commits to `nightSleepSec` (up to 546
  minutes) only when `Clock::openness() == Closed`. `Unknown` takes the
  normal short interval-based nap instead - the device declines to commit
  to an overnight hibernate while untrusted, and reconnects on the normal
  schedule so a resync can occur.
- **`Unknown`'s handling is not uniform, deliberately.** Open-equivalent
  (stay awake/connected, permit a report - itself a resync opportunity) at
  every CONNECTED-mode and reporting site. Two named exceptions:
  `State_Report.cpp`'s long-term webhook-escalation gate treats `Unknown` as
  `Closed` (skip this cycle) - the least change from its existing fail path,
  for a block whose worst case is an unwarranted `ERROR_STATE` reset.
  `State_Idle.cpp`'s idle-connectivity-ceiling exemption also denies
  `Unknown` (ceiling applies) rather than granting it - that exemption has
  no separate timeout once granted, so an unbounded exemption under
  `Unknown` would be a real battery-drain regression for a device whose
  clock can go untrusted for extended periods (this fleet has one: Dev-11).
- **Two sites deliberately excluded from conversion, each with an in-place
  comment**, because both are solving a different problem than "is this
  instant trustworthy":
  - `setup()`'s `CONNECTING_STATE` gate (`!Clock::isTimeValid() ||
    neverConfirmedSyncEver`) - `Clock::isTrusted()` is false on every boot,
    including every hibernate wake, until a sync completes that boot;
    converting this term would force `CONNECTING_STATE` on every wake, the
    exact regression the persisted, cross-boot `neverConfirmedSyncEver` flag
    exists to prevent.
  - `connectivityFailsafeSupervisor()`'s early-return - this escalation
    ladder (radio reset -> system reset -> deep power-down) exists to
    recover a wedged modem/radio, and a device with a wedged radio has, by
    definition, never synced either. Gating the recovery on
    `Clock::isTrusted()` would disable the recovery mechanism in exactly the
    case it exists for.
- **`openness=0/1/2` added to the existing `TimeDiag:` log line**, alongside
  the pre-existing `isOpen=` (which stays sourced from the raw, fail-open
  `isWithinOpenHours()` on purpose). The two are expected to diverge exactly
  when the clock is untrusted - `isOpen=1`/`openness=2` is the signature of
  "fail-open would have said open, `openness()` correctly says Unknown" -
  and this divergence is the acceptance evidence the bench run below relies
  on.

## Verification

- **Host tests**, all mutation-tested (reintroduced the old pattern,
  confirmed failure, restored byte-identical via sha256):
  - `tests/clock_openness_test.cpp`/`.sh` - the full truth table
    (`{trusted,open}->Open`, `{trusted,closed}->Closed`,
    `{untrusted,*}->Unknown`), a host mirror plus a real-source fidelity
    check confirming `Clock::openness()` checks trust, then config
    validity, then the hour window, in that order.
  - `tests/sleep_duration_consumes_openness_test.py` - source-tracing test
    confirming `handleSleepingState()`'s night-sleep commitment is gated on
    `parkOpenness == Clock::Openness::Closed`, not a reintroduced
    `!isWithinOpenHours()`/`!openNow`.
  - `tests/clock_trust_standard_structural_test.py` - allowlist test: every
    remaining `Clock::isTimeValid()` call in the tree must be one of the two
    named exceptions above; all 14 converted sites present at their
    specific locations.
- **Full suite.** 33/33 (30 from Step 3a + these 3 new tests).
- **Cloud compile.** `particle compile boron . --target 6.4.1` succeeds.
  Flash 149486 / RAM 3406 (Step 3a baseline: 149430 / 3406).

## Bench acceptance

Dev-14, commit `7d56c25`. The acceptance run did not go exactly as designed
- recorded plainly below rather than described as executed-as-written.

### Untrusted-clock case - PASS, by a different route than planned

The intended induction mechanism (`ENABLE_RTC_SKEW_TEST`, reusing the
existing bench hook) **did not fire** - see the new WO filed alongside this
one, `WO-2026-09-20-001`, for the investigation. The untrusted state was
produced instead by a genuinely blocked cellular sync: an attenuator plus
foil shielding held the modem at `sig=0/0` (no serving cell) through the
full 660-second connect budget, so the device never completed a sync this
boot. Different route to the same state - arguably more field-realistic
than a synthetic skew, since it reproduces "device physically cannot see a
cell tower" rather than "clock reads a plausible-but-wrong value."

```
CloudRecover: exhausted stage=2 elapsed=660002
ConnSummary: fail ... last=CELLULAR_ACQUIRE ... sig=0/0
Connect: fail elapsed=660002ms budget=660000ms
StateReq: Connect->Sleep reason=connect-timeout
TimeDiag: valid=1 epoch=1789868958 local=2026-09-20 09:49:18 isOpen=1 trusted=0 openness=2 syncAgeMs=4294967295
Sleep: ULP standby=0 reason=scheduled dur=3600s occ=0
```

`openness=2` (Unknown), and the sleep decision took the short interval
(`dur=3600s`) - not `nightSleepSec`, not `sleepReason="closed"`. The device
declined to commit to an overnight hibernate while untrusted.

**Not field-proven, and why that's acceptable.** This run's `isOpen=1` means
the OLD predicate (`isWithinOpenHours()`) would also have said "open" at
this real wall-clock time (09:49 local, inside the 06:00-22:00 window) - so
this specific run did not demonstrate `openness()` *preventing* a
night-sleep that the old code would have wrongly committed to. The
`{untrusted, would-have-computed-closed}` case - the actual bug this step
fixes - is covered by the host truth-table test instead, not by this bench
run. This substitution is justified, not just convenient: `Clock::openness()`
checks trust FIRST and short-circuits to `Unknown` before ever evaluating
the hour window (see `clock_openness_test.sh`'s fidelity check, which
asserts this exact ordering against the real source) - so it structurally
cannot behave differently between "trust check fails, hour would have been
open" and "trust check fails, hour would have been closed." Both take the
identical code path. The bench run proves the trust short-circuit fires on
real hardware in a real untrusted state; the host test proves the branch
that would only be reachable at a different time of day.

### Trusted-clock control - PASS, behaviour unchanged from pre-3b

```
ClockResync: sync advanced, rtcUpdated=1 epoch=1789892065
TimeDiag: valid=1 local=2026-09-20 16:15:00 open=6 close=22 isOpen=1 trusted=1 openness=0 syncAgeMs=39987
Sleep: ULP standby=1 reason=debounce dur=300s
```

`trusted=1`, `openness=0` (Open, correct for 16:15 against 06:00-22:00),
normal short-interval sleep (`reason=debounce`, occupancy-driven) - no
behavioural difference from what pre-3b code would have done here.

### Bench-procedure notes for anyone repeating this

- **Antenna method changed from the original plan.** The draft procedure
  called for detaching the cellular antenna. Reviewed against Particle's
  own documentation before doing it: it does not address transmitting into
  an unterminated antenna port, and does document the U.FL connector as not
  designed for repeated connect/disconnect cycling and as static-sensitive.
  Attenuator + foil shielding avoids both concerns while still reliably
  producing `sig=0/0` - the actual requirement.
- **Incidental thermal side effect.** Foil wrapping triggered
  `Charging inhibited due to enclosure temperature: 37.19 C` during the
  induction window, clearing once the enclosure cooled to 31.79 C. Expected
  consequence of the shielding method (trapped heat), not a fault - noted
  here so it isn't mistaken for one on a repeat run.
- **Procedure defect found, and fixed for next time.** The original
  procedure's "confirm `RtcSkewTest: ONE-SHOT bench hook fired` in serial"
  step is unobservable by design: USB CDC has not enumerated yet at the
  point in `setup()` where that log line would print. Same structural blind
  spot that drove `hibernate_wake` to a cloud event during Step 1/2's bench
  work rather than a serial-only check. The corrected verification point is
  boot N+1's `TimeDiag:` epoch, read from the cloud event history, not
  serial.

## Not in scope for this step

`BatteryAuthority` (Step 4), the persistence-header split (Step 5),
`sensors/SensorManager.cpp:490` (explicit decision, same reasoning as Steps
2 and 3a), and the `AppBreadcrumb` enum cross-TU-visibility fix
(`WO-2026-09-16-001`) are all untouched. The `ENABLE_RTC_SKEW_TEST`
silent-no-op finding is filed separately (`WO-2026-09-20-001`) rather than
fixed here - investigation-first, no fix authorized.

## Provenance

Scoped by `docs/architecture-review-2026-09-03.md` and the Structural
Ownership Map roadmap agreed 2026-09-03 (Step 3b of that roadmap - the
roadmap's only behaviour-changing step). Implemented 2026-09-19 and
bench-accepted 2026-09-20 on branch `wo/2026-09-19-step3b-trust-standard`,
stacked on Step 3a's branch, commit `7d56c25`.

## Related work orders

- `docs/architecture-review-2026-09-03.md` - source of the roadmap this step
  is item 3b of.
- `WO-2026-09-18-001` - Step 3a, introduced the `Clock::isTimeValid()` seam
  this step changes the semantics behind.
- `WO-2026-09-20-001` - the `ENABLE_RTC_SKEW_TEST` silent-no-op finding
  surfaced by this step's bench acceptance run.
- `WO-2026-09-15-001` - the PowerDiag/LedgerPayloadStatus logging-noise
  finding, amended with stronger evidence from this run's bench log.
- `WO-2026-08-31-003` - the RTC skew bench hook this step's induction
  attempt reused (and which did not fire as expected - see
  `WO-2026-09-20-001`).
