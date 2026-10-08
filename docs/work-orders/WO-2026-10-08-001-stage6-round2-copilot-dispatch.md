AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-08, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- Edit `src/Generalized-Core-Counter.cpp`: **move** the existing cadence-rule block in `connectivityFailsafeSupervisor()` only.
- Edit `tests/failsafe_cadence_rule_test.{cpp,sh}`, and any test that pins the moved lines.
- Run the host suite and a local ARM release build.
- Your scratch directory is **`build-tmp/wo20261008-001-stage6r2/`**. Remove **only it** when you finish.

**Not authorized:**
- Any other `src/` change, including `ConfigApply.cpp` and `State_Report.cpp`, which stay as round 1 left them.
- Any edit to `docs/`, `CHANGELOG.md`, `lib/` or Device OS.
- Commits, pushes, merges, branch changes, stash, reset or checkout. Flashing, device or network access beyond the Particle compile.
- Any new state, timer, flag or log-string change.
- Deleting anything you didn't create, including `build-tmp/` itself.

**SIZE BUDGET:** this round, **0 net `src/` lines** (a move only). The WO total stays at +9 against +20. Any net `src/` change means STOP and report.

# Stage 6 round 2 dispatch: WO-2026-10-08-001 (Step 6 WO 1b)

**This is the last round under the two-round rule.**

**Goal, in plain language:** the connectivity failsafe escalates only when a connection the device was expected to make is overdue, never just because time passed while no connection was due.

**Repository:** this worktree, branch `wo/2026-10-08-001-failsafe-overdue`. Round 1's implementation is the **uncommitted** diff; build on it. Don't switch branches.

**Binding spec:** `docs/work-orders/WO-2026-10-08-001-failsafe-overdue.md`, especially "Stage 7 round 1 and the architect's rulings". The findings are in `docs/work-orders/WO-2026-10-08-001-stage7-verdict.md`.

## What to change

| Item | Change |
|---|---|
| F3 | **Move the cadence-rule block** (the comment, `#if !CONNECTIVITY_FAILSAFE_TEST_MODE` … `#endif`, now at about `Generalized-Core-Counter.cpp:2629-2639`) **below the stage and cooldown returns**: after `if (currentStage != 0 && lastAction != 0 && (now - lastAction) < requiredDelay) { return; }` (about `:2664-2666`), and before `if (activeConnectAttemptWithinBudget())`. Keep the block's text byte-identical apart from its position. **Confirm that the behaviour is the same:** nothing between the old and new positions has a side effect. Check `RecoveryState::get_*`, `connectivityFailsafeJitterSec()` (random or persisted?) and the stage-3 return. If anything there writes state, logs, or consumes randomness that the rule's early return previously skipped, STOP and report. Also say how often `resolveRuntime()` now runs once age ≥ 3 h. |
| F2 | **Fix `tests/failsafe_cadence_rule_test`** so it drives the **real** supervisor decision. Either compile the real `connectivityFailsafeSupervisor()` block, or extract the real cadence-rule block from `src/` with a byte-for-byte check that fails loudly on mismatch (see `tests/occupancy_report_by_mode_test.sh`'s `COPY_MISMATCH` pattern). It must **not** re-implement `STALE + cadence` in the test. Acceptance: the mutation `STALE_SEC + cadenceSec` → `cadenceSec` makes the **4 h behaviour checks** fail, not only a structural check. Keep the existing cases (1 h, 2 h 59 m, 3 h, 4 h, all three modes, test-build absence). |

## Mutations (each caught by its targeted behaviour check, not by compilation alone)

1. `STALE_SEC + cadenceSec` → `cadenceSec`: the 4 h checks fail.
2. Drop the `cadenceSec >= STALE_SEC` condition: the 1 h or 2 h 59 m checks fail.
3. Remove the rule: the 4 h checks fail.
4. Move the block back above the stage-3 return: a structural order check fails. Add one if needed.
5. Round 1's other mutations (the `#if` guard, the refresh, the 86400 bound): re-run them, all still caught.

## Verification

1. **Full host suite,** before and after, as `N/N (sh via zsh, py via python3)`. The round-1 result was 72/72. No `src/` copy under the worktree while it runs.
2. **Local ARM release build,** with a fresh `BUILD_PATH_BASE`. Give text/data/bss against round 1's 151020 / 1090 / 2196. Show in the disassembly that the `resolveRuntime` call now comes after the stage and cooldown checks.
3. **A failsafe test-mode build,** with its own fresh path: the rule is still absent.
4. **Net `src/` lines:** 0 this round. Show `git diff --color-moved=zebra` for the move.

## Implementation Report (required, as your final message)

- **F3:** the new position, the side-effect confirmation, and how often `resolveRuntime()` now runs.
- **F2:** how the test now reaches the real formula.
- The mutations, with results.
- Commands and results, with interpreters and sizes.
- Confirmation that you deleted only `build-tmp/wo20261008-001-stage6r2/`.
- Deviations (or "none").
- The model and reasoning level actually used.
