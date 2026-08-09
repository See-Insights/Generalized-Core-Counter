# WO-2026-08-07-001: Reconcile State_Report.cpp reporting-policy conflict blocking PR #14

**Status: architecture decision recorded 2026-08-07. Ready for
implementation.**

## Decision

Reviewed against the evidence below and approved by Chip:

1. **Adopt main's `ReportingPolicyResolver`/two-arg `applyBatteryAwareConnectionModePolicy`
   flow in full — not a close call.** `windowOpen && isBoundaryDue(...)` is
   strictly more correct than HEAD's alignment-only check. Losing the
   window-open gate would reintroduce connecting outside open hours, a
   real behavioral regression, not a style difference. Confirmed by direct
   source reading rather than trusting whichever side "looks newer."
2. **`set_lastReport()` timing move — no action needed**, already resolves
   correctly via auto-merge (confirmed above, outside the marked conflict
   region).
3. **Keep `PowerManager::instance().soc()` as the SOC source at all four
   call sites** (not just `State_Report.cpp`) — both getters are confirmed
   equivalent everywhere they're used here, so this isn't a correctness
   question, but consistency with Phase 1's whole point (make
   `PowerManager` the actual call path) argues for keeping the migrated
   form rather than reverting to `current.get_stateOfCharge()` just
   because it happens to also work.
4. **`DeviceStatusPublisher.cpp`'s new `ReportingPolicyResolver` call is
   explicitly out of scope for this merge.** Whether it should also route
   through `PowerManager` is a legitimate question — but a merge-conflict
   reconciliation is the wrong moment to expand its own scope. Leave it
   exactly as main's auto-merge produces it (it isn't even one of the 4
   conflicting files). Track as a Phase 2 or standalone follow-up, not
   resolved here.

Resolution should stay mechanical and low-risk given this scope: adopt
main's resolver, keep the accessor, touch nothing else.

## Context

PR #14 (`refactor/powermanager-phase1` → `main`) is blocked —
`mergeStateStatus: DIRTY`, `mergeable: CONFLICTING`. `main` has advanced
since this branch diverged: PR #10 ("fix/effective-reporting-policy"), PR
#12 (battery-authority delta guard), and PR #13 (ledger write ordering)
have all merged. A three-way `git merge-tree` against the current refs
confirms conflict markers in exactly four files:

- `src/Version.cpp`, `Doxyfile`, `README.md` — simple version-string
  collisions (this branch bumped to `20.1-PowerMgt`, main is at
  `21.0_Test`). **Already decided by Chip: resolve to `20.1-PowerMgt`, not
  a fresh bump.** No further investigation needed on these three.
- `src/state/State_Report.cpp` — a real architectural conflict, not a
  mechanical one. This document covers that conflict only.

`CHANGELOG.md` is changed on both sides but merges cleanly.

This branch's whole purpose (Phase 1 PowerManager consolidation) is
migrating scattered SOC/battery-state reads to `PowerManager::instance()`
accessors, behavior-preserving, per
`docs/work-orders/WO-2026-08-06-001-powermanager-phase1.md`. Main's PR #10
independently introduced a new `src/reporting/` module
(`ReportingPolicy.h`, `ReportingPolicy.cpp`, `RuntimeReportingPolicy.cpp`,
`tests/reporting_policy_test.cpp`) that replaces `State_Report.cpp`'s
manual inline tier/interval/alignment arithmetic with a single resolver,
and changed `applyBatteryAwareConnectionModePolicy`'s signature and return
type. Both branches touched the same function for unrelated reasons at the
same time.

Investigated via an independent Codex pass (read-only, no repo
modifications) plus direct verification of its two most consequential
claims by re-reading the source and re-running a scratch merge in an
isolated worktree.

## Evidence

### 1. Full comparison of both sides

- Main has two `applyBatteryAwareConnectionModePolicy` overloads: a
  two-arg `void(float currentSoC, BatteryTier resolvedTier)` that does
  **not** recalculate the tier, and a convenience one-arg
  `void(float currentSoC)` that calls
  `ReportingPolicyResolver::resolveRuntime(currentSoC, Time.now())`
  internally and delegates. HEAD has only the old
  `BatteryTier(float currentSoC)` — one overload, returns the tier. The
  only caller anywhere that consumes that return value is HEAD's
  `State_Report.cpp:140`.
- `ReportingPolicyResolver` replaces more than interval arithmetic: battery-tier
  hysteresis/multiplier selection, boundary-alignment, open-hours gating
  (`cadenceDue`), next-open-window-boundary lookup, zero-interval/overflow
  handling, and standardized tier names/reasons.
- **HEAD's manual logic is not behaviorally equivalent to main's resolver —
  verified directly, not just claimed.** HEAD's connect decision (`State_Report.cpp:212`,
  `else if (isAligned)`) gates purely on boundary alignment
  (`now % effectiveInterval` within tolerance) — no open-hours check at
  this decision point. Main's equivalent gate uses
  `reportingPolicy.cadenceDue` (`State_Report.cpp:201` on main), and
  `ReportingPolicy.cpp:66` defines that as
  `policy.cadenceDue = inputs.windowOpen && isBoundaryDue(...)` — alignment
  **and** the reporting window being open. Confirmed by reading both files
  directly, not inferred from the diff alone. Losing this during
  reconciliation would reintroduce a real regression: connecting on a
  boundary-aligned timer tick even outside open hours.
- **`sysStatus.set_lastReport(now)` moved from before battery sampling to
  after `publishData()` on main — verified directly** (HEAD: line 55,
  before `publishData()` at line 63; main: `publishData()` at line 61,
  `set_lastReport()` at line 65). This sits just above the marked conflict
  region and auto-merges cleanly on its own — a scratch worktree merge
  confirmed the auto-merged result already carries main's ordering. Not
  something that needs manual resolution, but worth the implementer
  knowing this is an intentional main-side change being inherited, not an
  artifact to second-guess.
- The source comment above main's two-arg overload still says
  `@return BatteryTier` despite the implementation returning `void` —
  pre-existing doc drift on main, unrelated to this merge.

### 2. Does the resolver have hidden dependencies on the SOC source?

No. `ReportingPolicyResolver::resolveRuntime()` uses the supplied float
only as input to `BatteryBackoff::calculateTier()`. It does not read
`current.get_stateOfCharge()`, `PowerManager`, or any hardware/PMIC/fuel-gauge
state internally — its other dependencies are `sysStatus.get_currentBatteryTier()`
(hysteresis), `Config::reportingIntervalSecForRuntime()`, `Time.isValid()`,
`isWithinOpenHoursAt()`, and `ConnectivityPolicy::CONNECT_ALIGNMENT_TOLERANCE_SEC`.

At the `State_Report.cpp` call site specifically, `PowerManager::soc()` is
confirmed a direct pass-through to `current.get_stateOfCharge()` with no
caching (`PowerManager.cpp:218`), and the call sequence in
`handleReportingState()` is: capture `now` → `measure.loop()` →
`measure.batteryState()` (which internally commits or intentionally
retains the previous accepted SOC via `BatteryAuthorityPolicy::resolveSocCommit()`)
→ `publishData()` → the reporting-policy read. Nothing between the end of
`batteryState()` and the reporting-policy read writes SOC, so both getters
return the identical post-resolution value at this call site. The
WO-2026-08-06-001 caching-risk warning (stale pre-commit reads) applies
specifically to reads *inside* `refreshInputProfile()`, not to reads after
`batteryState()` returns — this call site is safe either way.

Two independent, pre-existing gaps in the resolver were noted (unrelated to
SOC source, not currently covered by `tests/reporting_policy_test.cpp`):
`cadenceDue` doesn't explicitly check `inputs.timeValid` (an aligned epoch
with an invalid clock could still read as due, while `nextReportEpoch`
comes back zero), and the open-window search is bounded to 64 candidate
boundaries. Flagging for awareness, not proposing a fix here — out of this
WO's scope.

### 3. `tests/reporting_policy_test.cpp` coverage

Five cases: effective-interval/adjustment-reason per tier, a 4.3%
survival-tier field case (12× multiplier, cadence, next boundary), tier
hysteresis/recovery, closed-window suppression, and pure-resolver output
consistency. It does not exercise `resolveRuntime()`,
`handleReportingState()`, either `applyBatteryAwareConnectionModePolicy()`
overload, either SOC getter, or SOC-commit timing — and the documented
host-test build only links `ReportingPolicy.cpp`, not
`RuntimeReportingPolicy.cpp` or state code. Changing only
`State_Report.cpp`'s SOC source while keeping main's resolver/two-arg call
would not touch anything this test covers — it would need no changes and
would still pass.

### 4. The other three `applyBatteryAwareConnectionModePolicy` call sites

`Generalized-Core-Counter.cpp` (setup) and both `State_Sleep.cpp` sites all
call the one-arg convenience overload as a statement (return value
unused), so main's `void` signature is already source-compatible there —
these three are not part of the actual conflict.

- **Setup site**: called after `measure.batteryState(Setup)` and a further
  `refreshInputProfile()` (which doesn't write SOC) — commit has already
  resolved. Switching to `PowerManager::soc()` here has no behavioral
  effect.
- **First `State_Sleep.cpp` site**: immediately after
  `measure.batteryState(PostWake)` — same fresh post-resolution guarantee.
- **Second `State_Sleep.cpp` site** (PIR-wake occupancy handling): during
  open hours, follows the same post-wake sample. During closed hours, the
  open-hours sampling block is skipped, so this can read the previously
  accepted ledger SOC rather than a fresh sample — but that staleness
  already exists today with `current.get_stateOfCharge()`; the pass-through
  accessor changes nothing about it. Pre-existing call-site characteristic,
  not a new risk from Phase 1.

One consideration for the architecture review: main keeps all three of
these on the one-arg convenience overload (each internally resolves its
own tier from `sysStatus.get_currentBatteryTier()` hysteresis state). If
these were instead converted to explicit two-arg calls, they'd need to
resolve a tier from the same SOC/persisted-tier state first — passing an
independently or earlier-resolved tier into a different call site risks
divergence. Not needed for this merge (they already compile fine as-is
against main's one-arg overload); flagging only because it's an available
follow-on and adjacent to this conflict.

### 5. Other latent conflicts/risks beyond the 4 known files

- No unaccounted callers rely on the old `BatteryTier` return value —
  `State_Report.cpp` is the only assignment; the other three call sites use
  it as a statement. Forward declarations in `State_Common.h` merge
  cleanly. `BatteryTier`'s move from `MyPersistentData.h` to
  `cloud/BatteryBackoffPolicy.h` on main also merges cleanly, no duplicate
  definitions.
- The reconciled `State_Report.cpp` needs to retain **both**
  independently-added includes: HEAD's `power/PowerManager.h` and main's
  `reporting/ReportingPolicy.h`.
- **A genuine Phase-1 coverage gap exists outside the conflict files**:
  main independently added a new direct SOC consumer in
  `DeviceStatusPublisher.cpp` —
  `ReportingPolicyResolver::resolveRuntime(current.get_stateOfCharge(), Time.now())`.
  This call site didn't exist at the branches' common ancestor, so Phase 1
  never had a chance to migrate it. It's behavior-neutral today (`PowerManager::soc()`
  is a live pass-through), but it's a loose end against Phase 1's stated
  "all external consumers through PowerManager" goal — worth an explicit
  decision on whether to fold it in now or track separately, not silently
  leave unmigrated.
- Generated `docs/html` still documents the obsolete return-valued,
  one-arg signature — non-runtime doc drift, unrelated to the merge
  itself.

## Implementation notes

- Files touched: `src/state/State_Report.cpp`, `src/Generalized-Core-Counter.cpp`,
  `src/state/State_Sleep.cpp` (SOC-source substitution only at the three
  non-conflicting call sites), plus the three version-string files per the
  earlier decision. `src/DeviceStatusPublisher.cpp` not touched.
- Verify: both `ENABLE_DIAGNOSTICS_PUBLISH_MODE` flag states still compile
  (`particle compile p2 . --target 6.4.1`), existing test suite still
  passes, and an independent Codex pass confirms the resolved merge
  actually implements decisions 1-4 above before this is considered ready
  to commit.
- Per this project's process: leave the merge uncommitted for independent
  verification — implementation prepares the resolved diff, only Chip
  commits/pushes/merges.
