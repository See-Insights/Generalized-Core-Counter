# WO-2026-09-14-002: Hibernate wake-validation gate had two implementations kept in sync by hand

**Type:** Structural fix (Step 1 of the Structural Ownership Map roadmap).

**Status:** CLOSED 2026-09-16. Code complete, fully tested on the host and via
cloud compile (Boron, target 6.4.1), and bench-validated on Dev-09 and Dev-14
across two nights each - see Bench validation below. No behaviour change was
intended, and none was observed.

## The observation

`docs/architecture-review-2026-09-03.md` ("Incident 1") flagged that the
production hibernate wake-validation gate at `Generalized-Core-Counter.cpp`
was a six-term `&&` chain, and that
`time/HibernateWakeDiagnostics.h`'s `classifyGateArm()` mirrored those same
six terms, in the same order, purely so a forensic event could name which
term failed. Nothing enforced the two staying in sync except a doc comment
asking a reviewer to check them by hand:

> This order MUST match, position for position, the short-circuit `&&`
> chain of the real gate `if` in `setup()` ... Any change to either the
> real gate's condition order or this list must change both together.

Two implementations of one decision, synchronized only by review discipline,
is exactly the shape of defect this class of bug takes: it is silent until
someone changes one side and not the other. It is also why
`WO-2026-08-31-002` Amendment B (the `DEEP_POWER_DOWN` finding) had nowhere
single to land - there were two candidate places to add a seventh
condition, not one.

## The fix

Retired the duplicate. The production `if` in `setup()` now calls
`HibernateWakeDiagnostics::classifyGateArm()` directly and branches on
`GateArm::kNone`, instead of re-evaluating the six terms itself:

```cpp
const HibernateWakeDiagnostics::GateArm gateArm = HibernateWakeDiagnostics::classifyGateArm(gateInputs);
if (gateArm == HibernateWakeDiagnostics::GateArm::kNone) {
  // success path, unchanged
} else {
  // failure path, unchanged
}
```

`gateInputs` was already being constructed, once, a few lines further down
- feeding the same six fields into `buildEventFields()` for the forensic
event - so this moves that single construction earlier and has both the
gate decision and the forensic event consume it, rather than constructing
it twice (once implicitly, as the raw `if`'s six terms, and once explicitly
for the event).

`classifyGateArm()` itself was not touched - same six checks, same order,
same return values. This step changes who calls it, not what it does.

The header's "ORDERING CONTRACT" doc comment
(`time/HibernateWakeDiagnostics.h`, above `classifyGateArm()`) was deleted:
it existed only to tell a reviewer to keep two implementations in sync by
hand, and there is no longer a second implementation for it to describe.

## A fidelity check had to change too

`tests/hibernate_wake_diagnostics_test.sh` compiles and runs
`hibernate_wake_diagnostics_test.cpp` (unchanged - it tests
`classifyGateArm()`/`buildEventFields()` in isolation, which did not
change), then runs a second pass of textual "fidelity checks" against the
real `Generalized-Core-Counter.cpp` source, so the host-side test cannot
silently drift from production. Six of those checks pinned the exact text
of the old six-term chain - the very thing this WO retires - so they failed
immediately after the fix, correctly, because the pattern they searched for
no longer exists.

This was not a new discovery mid-fix (unlike Step 0.5's `Config.h` finding
in `WO-2026-09-14-001`): it is a direct, mechanical consequence of doing
exactly what this WO describes, surfaced the moment the real test script
ran. The six checks were replaced with two that assert the new shape
instead:

- `gateArm` is assigned from `classifyGateArm(gateInputs)` (not a literal or
  a different input).
- the `if` branches on `gateArm == GateArm::kNone` (not a hand-duplicated
  chain).

The six individual conditions did not lose coverage: the `GateInputs`
field-mapping checks lower in the same file (unchanged by this WO) already
prove each condition's *input* is wired from the correct production
variable, and `hibernate_wake_diagnostics_test.cpp`'s
`testEachGateArmFailureIsIdentified` already proves `classifyGateArm()`
combines those six inputs into the correct arm - both untouched by this
step. The two new checks close the one remaining gap: that the production
`if` actually consumes that classification, rather than an independently
reconstructed condition.

## Verification

- **Regression proof.** Reconstructed the exact pre-fix file by mechanical
  string substitution (asserted single-occurrence matches, not hand
  editing), installed it in place of the real file, and confirmed both
  `tests/wake_gate_single_owner_structural_test.py` (new) and
  `tests/hibernate_wake_diagnostics_test.sh` (existing) fail - for the
  correct reason (missing call / missing pattern), not a compile error.
  Restored the real file and confirmed byte-identical via `sha256sum`
  before and after.
- **New structural test.** `tests/wake_gate_single_owner_structural_test.py`
  asserts, against the real checked-in source with comments stripped:
  exactly one `classifyGateArm(` call in `Generalized-Core-Counter.cpp`,
  and zero occurrences of the old `WakeReason::ALARM &&` chain fragment.
- **Existing test, unmodified function.** `hibernate_wake_diagnostics_test`
  (Part 1, the host-side pure-logic test of `classifyGateArm()` /
  `buildEventFields()`) passes unmodified - confirming this step did not
  change the classifier itself.
- **Full suite.** 28/28 (27 pre-existing + 1 new), no regressions.
- **Cloud compile.** `particle compile boron . --target 6.4.1` succeeds.
  Flash 149550 / RAM 3414 bytes - consistent with a call-site refactor
  (no new retained state, no new library dependency).

## Bench validation

Two nights each, post-flash, Dev-09 and Dev-14. Acceptance: `hibernate_wake`
payload shape and gate-arm classification match the prior week's pattern
exactly - this is the check that moving the decision produced no behaviour
change.

Binary compiled from commit `aaf687a` (`particle compile boron . --target
6.4.1`, sha256 `1e0176ad5e9086891f74f8e7fa4afcf9134b5fa6bdd1c6d6f69b7978945e2756`).
The binary itself is not a committed artifact - there is nothing to look up
in the repository beyond the source at that commit; the hash is recorded
here as the record of exactly what was flashed.

**Dev-14 - two clean nights, exactly as required:**

| | Night 1 | Night 2 | Pre-flash baseline (6 nights) |
|---|---|---|---|
| result | ok | ok | ok |
| gateArm | none | none | none |
| wakeReason | ALARM | ALARM | ALARM |
| rtcOk | 1 | 1 | 1 |
| req | 28487 | 28798 | 28797-28798 |
| actual | 28488 | 28799 | 28798-28799 |
| err | 1 | 1 | 1 |
| count | 1 | 2 | (pre-flash: 12-17) |

`count` incremented by exactly 1 (1->2), continuing cleanly from the
post-flash retained-memory reset. Every other field matches both nights and
the 6-night pre-flash baseline exactly. **Dev-14's acceptance criterion is
met.**

**Dev-09 - correct classification both nights, matching its established
pre-flash pattern, validating the gate logic even under connectivity
stress:**

| | Night 1 | Night 2 | Pre-flash baseline |
|---|---|---|---|
| result | fail | fail | fail |
| gateArm | wake_reason | wake_reason | wake_reason |
| wakeReason | DEEP_POWER_DOWN | DEEP_POWER_DOWN | DEEP_POWER_DOWN |
| rtcOk | 1 | 1 | 1 |

Dev-09 is a Singapore poor-connectivity bench unit
([[poor-connectivity-test-devices]]) and independently hit an unrelated
`CELLULAR_ACQUIRE` registration stall during this validation window - filed
separately as `WO-2026-09-15-002`, confirmed not caused by and not related to
this change (it delayed when night 1's already-correct `hibernate_wake`
event reached the cloud, not what the gate decided). The gate itself
classified correctly, on time, on both nights regardless.

### Side note: `retainedHibernateCount` reset between Dev-09's two nights (1, 1, instead of 1, 2) - resolved, not a defect

Investigated in detail (call sites traced, publish-timing ruled out as a
mechanism, watchdog reset and battery brownout both ruled out against
telemetry) before the actual cause came to light: the field was cleared by a
**manual antenna swap** - a deliberate board power cycle at approximately
19:55 SGT on 2026-09-15, confirmed directly against Dev-09's serial log (the
boot-uptime counter resets from ~50,000,000 ms to `0000009733` ms at
`2026-09-15T11:55:09Z`) and corroborated by the subsequent signal-strength
jump (`sig=0/0` before, `sig=55/6` and similar immediately after - the new
antenna working as intended).

This is expected, documented behaviour, not a defect: `retained` SRAM
(what `retainedHibernateCount` and its four sibling fields live in) clears
on a genuine loss of board power, by design - `time/RtcSkewTest.h`'s own doc
comment already states retained RAM survives a soft reset, a watchdog
reset, and a HIBERNATE wake, only "as long as system power is maintained."
This is a different retention domain from `sysStatus` (a
`StorageHelperRK::PersistentDataFile` - a real file on non-volatile flash
storage) and from the AB1805's own battery-backed RTC, neither of which this
power cycle affected. No Step 2 precondition follows from this, and no
separate WO was needed.

## Not in scope for this step

Named per the roadmap and not touched here: moving the retained hibernate
fields, `time/HibernateCycle.{h,cpp}`, `sensors/SensorManager.cpp:490`.
Those are Step 2.

## Provenance

Scoped by `docs/architecture-review-2026-09-03.md` ("Incident 1") and the
Structural Ownership Map roadmap agreed 2026-09-03 (Step 1 of that
roadmap). Implemented 2026-09-14 and bench-validated 2026-09-14 through
2026-09-16 on branch `wo/2026-09-14-002-wake-gate-single-owner`.

## Related work orders

- `docs/architecture-review-2026-09-03.md` - source of Incident 1 and the
  roadmap this step is item 1 of.
- `WO-2026-08-29-001` - added the forensic event and
  `HibernateWakeDiagnostics.h` this step now routes production through
  directly instead of alongside.
- `WO-2026-08-31-002` Amendment B - the `DEEP_POWER_DOWN` finding that had
  nowhere single to land before this step; still not implemented, now has
  one place to land.
- `WO-2026-09-14-001` - the prior step in this same session, same
  narrow-seam-over-duplication pattern applied to a different coupling
  (persistence/config headers rather than a boolean gate).
- `WO-2026-09-15-001` - excessive repeat logging in PowerDiag/
  LedgerPayloadStatus, noticed during this WO's bench validation; unrelated
  to this change, filed separately.
- `WO-2026-09-15-002` - the `CELLULAR_ACQUIRE` registration stall that
  delayed (not altered) Dev-09's night 1 `hibernate_wake` publish; confirmed
  unrelated to this change, filed separately.
