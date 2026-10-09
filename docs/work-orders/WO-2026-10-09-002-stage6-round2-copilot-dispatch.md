AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-09, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- Edit `src/cloud/LedgerClient.cpp`: the hold code in `Cloud::areLedgersSynced()` only.
- Edit `tests/config_window_hold_test.{cpp,sh}`.
- Run the host suite and a local ARM release build.
- Your scratch directory is **`build-tmp/wo20261009-002-stage6r2/`**. Remove **only it** when you finish.

**Not authorized:**
- Any other `src/` change (`State_Sleep.cpp` stays as round 1 left it).
- Any new persisted field, timer, schedule or constant.
- `docs/`, `lib/` or Device OS.
- Commits, pushes, merges, branch changes, stash, reset or checkout. Flashing, device or network access beyond the Particle compile.
- Deleting anything you didn't create, including `build-tmp/` itself.

**SIZE BUDGET:** the **WO total** must stay at **≤ +20 net `src/` code lines** (nonblank, non-comment). Round 1 is +18. **Pre-ruled fallback:** if F1 + F2 would take the total over +20, implement **F1 and F3 only**, leave F2 out, and say so in your report. Don't compress code.

# Stage 6 round 2 dispatch: WO-2026-10-09-002 (the once-a-day config window)

**This is the last round under the two-round rule.**

**Goal, in plain language:** a ledger config change reaches every sleeping device within a day. On one connection per day, and on the boot connection, the sleep gate holds briefly after the outbound syncs and ends early when an input `onSync` arrives. No other connection pays anything.

**Repository:** this worktree, branch `wo/2026-10-09-002-config-window`. Round 1's implementation is the **uncommitted** diff; build on it. Don't switch branches.

**Binding spec:** `docs/work-orders/WO-2026-10-09-002-config-window.md`, especially "Round 2". The findings are in `docs/work-orders/WO-2026-10-09-002-stage7-verdict.md`.

## What to change (`LedgerClient.cpp`)

| Item | Change |
|---|---|
| F1 | Replace the single `holdBaseSynced` (the `max` of both) with **one snapshot per input ledger** (for example `holdBaseDefault`, `holdBaseDevice`), taken at the new-connection edge. `inputLanded = (defaultSync != holdBaseDefault) \|\| (deviceSync != holdBaseDevice)`. Keep `!=`, not `>`, so a backward clock step still ends the hold. |
| F2 | At hold completion: `holdDoneEpoch = currentConnectionEpoch ? currentConnectionEpoch : 1;`. Same line, 0 net. |

## Tests

| Item | Change |
|---|---|
| F3 | Add a case to `tests/config_window_hold_test`: **different baselines** (for example default 200, device 100), then one ledger's `lastSynced` moves **backwards** (a clock step, for example device 100 → 50) while the other is unchanged. The next call must end the hold (`reason=onSync`) and return true. With `!=` mutated to `>`, this case must fail. |
| F2 test | A completion with `lastConnection == 0`. The next connection must **not** pay the boot hold again. |

**Mutations** (each caught by its targeted check):
- F1 mutated back to `max` of both;
- `!=` → `>`;
- F2 reverted to `holdDoneEpoch = currentConnectionEpoch`;
- plus re-run round 1's six mutations.

## Verification

1. **Full host suite,** before and after, as `N/N (sh via zsh, py via python3)`. The baseline is 74/74. No copy of `src/` under the worktree while it runs.
2. **Local ARM release build:** a fresh `BUILD_PATH_BASE`. Give text/data/bss against round 1's 151308 / 1090 / 2212.
3. **Size:** net `src/` code lines (nonblank, non-comment) for the **WO total** against `83df4a3`, and say whether the fallback was used.

## Implementation Report (required, as your final message)

- The change per item, with net lines, and the WO total.
- Fallback used? Yes or no, and why.
- Tests and mutations, with results.
- Commands and results, with interpreters and sizes.
- Confirmation that you deleted only `build-tmp/wo20261009-002-stage6r2/`.
- Deviations (or "none").
- The model and reasoning level used.
