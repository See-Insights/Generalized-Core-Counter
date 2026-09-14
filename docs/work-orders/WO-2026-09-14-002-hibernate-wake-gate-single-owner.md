# WO-2026-09-14-002: Hibernate wake-validation gate had two implementations kept in sync by hand

**Type:** Structural fix (Step 1 of the Structural Ownership Map roadmap).

**Status:** Code complete and fully tested on the host and via cloud compile
(Boron, target 6.4.1). **Bench validation outstanding** - two nights each on
Dev-09 and Dev-14 - before this closes. No behaviour change is intended;
bench data is the check that none occurred.

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

## Outstanding

- **Bench validation (blocks closing this WO).** Dev-09 and Dev-14, two
  nights each, post-flash. Acceptance: `hibernate_wake` payload shape and
  gate-arm classification match the prior week's pattern exactly - Dev-14
  `ok`, Dev-09 `fail`/`wake_reason`, byte-for-byte comparable field sets.
  This is the check that moving the decision produced no behaviour change.
- Not in scope for this step, named per the roadmap and not touched: moving
  the retained hibernate fields, `time/HibernateCycle.{h,cpp}`,
  `sensors/SensorManager.cpp:490`. Those are Step 2.

## Provenance

Scoped by `docs/architecture-review-2026-09-03.md` ("Incident 1") and the
Structural Ownership Map roadmap agreed 2026-09-03 (Step 1 of that
roadmap). Implemented 2026-09-14 on branch
`wo/2026-09-14-002-wake-gate-single-owner`.

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
