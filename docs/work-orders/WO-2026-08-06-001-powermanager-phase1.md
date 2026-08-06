# WO-2026-08-06-001: Power Management Consolidation Phase 1 — wire up PowerManager scaffolding

## Context

Per `docs/architecture/power-management-consolidation-findings.md`:
`PowerManager` already has almost exactly the vision doc's four-function
shape at the interface level, but is dead scaffolding — zero consumers
anywhere in the codebase, landed in one commit back in May, well before the
vision doc existed. Reads as intentionally broad-but-unfinished.

Phase 1's goal: make `PowerManager` the actual call path for power/battery
queries and decisions, migrating existing scattered call sites (per the
full inventory in the findings doc) to go through it — **without changing
current behavior**. The messy/inconsistent parts already found (incompatible
low-battery thresholds, sleep-mode taking no battery input, the dead
critical-battery shutdown, thermal-charging fusing state-classification
with actuation) are explicitly Phase 2 scope. Phase 1 consolidates *where*
things live, not *what* they decide.

This is a refactor of central, safety-relevant plumbing that WO-2026-08-05-002/
003's battery-authority logic already depends on — treat regression risk
accordingly. Full Codex Stage 7 verification applies (not the lighter
tooling-diff pattern used for the CLI work), per the standing two-tier
verification rule.

## Investigation needed (this WO doesn't yet know enough to specify the fix)

1. Read the actual current `PowerManager.h`/`.cpp` interface in full —
   what's already declared/stubbed for each of the four functions, and how
   closely it actually matches the vision doc's shape versus how closely
   Claude Code's summary characterized it.
2. Using the findings doc's full call-site inventory, enumerate exactly
   which call sites in `SensorManager.cpp` (and anywhere else) would need
   to redirect through `PowerManager` instead of reading the fuel gauge/PMIC/
   config directly.
3. Propose a migration plan: incremental (one call site/function at a time,
   verified at each step) versus a single larger cutover — with a
   recommendation and tradeoffs, not just an implementation.
4. Specify how function 1 (best SOC estimate) and function 4 (corrective
   actions) expose the already-built `BatteryAuthorityPolicy`/consistency-
   check logic from WO-2026-08-05-002/003 through `PowerManager` — does
   `PowerManager` wrap/call into that existing logic, or does logic need to
   move? Avoid duplicating logic that already exists and is tested.
5. For functions 2 and 3, where Phase 2 will eventually need to change
   real behavior (thermal-charging fusion, the incompatible low-battery
   thresholds) — does Phase 1's wiring need any interface shape
   considerations now to make Phase 2 easier later, or is that premature
   optimization better left until Phase 2 actually scopes it?
6. Flag anything found where preserving exact current behavior through
   `PowerManager` isn't straightforward — don't silently resolve it,
   surface it for a decision.

## Decisions needed (not prescribed by this WO)

- Migration strategy (incremental vs. single cutover) — pending
  investigation into the actual scope/risk per call site.
- Exact mechanism for function 1/4 to expose existing WO-002/003 logic
  without duplication.

## Acceptance Criteria

TBD pending investigation. At minimum: `PowerManager` becomes the actual
call path for SOC estimate and battery state (the two functions with
already-built logic to wire up), with byte-for-byte identical behavior to
today's scattered call sites for every migrated site, and full regression
coverage proving it.

## Required Tests

- For each migrated call site: a regression test confirming identical
  output before and after migration.
- Unit tests for `PowerManager`'s own public interface.
- Full existing WO-002/003 test suite continues passing unchanged.

## Permitted Files

Likely `src/power/PowerManager.h`/`.cpp`, `src/sensors/SensorManager.cpp`,
and test files. Codex to confirm exact scope once the migration plan is
proposed.

## Status

Drafted at investigation-plus-migration-plan scope, since the actual
`PowerManager` interface isn't yet known in detail. Ready to dispatch for
investigation — implementation scope to be reconciled once findings and a
migration plan come back.
