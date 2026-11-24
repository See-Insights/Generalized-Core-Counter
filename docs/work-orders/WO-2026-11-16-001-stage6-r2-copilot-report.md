# WO-2026-11-16-001 Stage 6 r2 report

Model/level: claude-sonnet-5.5 (low reasoning, as dispatched). Interpreter: Python 3.15.2 via `python3`; shell tests via zsh.

## Baseline
The first full run was 121/122. The one failure, `tests/thermal_coupling_structural_test.py`, was caused by my own scratch `src-mut` copy under `build-tmp/` in the worktree, which that test scanned. It was not a product failure. I deleted the copy (my own file) and the final run was 122/122.

## Failing-before evidence
On a scratch copy of `src/`, I moved the `static int lastRefreshRc` ... `lastRefreshRc = refreshRc;` block into `case IDLE_STATE:`. The test at `99ece4e` printed `ok (20 probes detected)` with rc=0, so the finding reproduces. With the fix, the same copy printed `FAIL: loop order must be thrashGuard.loop(state, millis()) < ExternalWatchdog.refresh() < checkClockResync(): ...` with rc=1.

## Change
`tests/s2_alarm_watchdog_wake_structural_test.py`: +15/-2 lines.
- The refresh-before-`checkClockResync()` check is replaced by a three-way order check: `thrashGuard.loop(state, millis())` < `ExternalWatchdog.refresh()` < `checkClockResync()`. The failure message names the order.
- New probe "refresh block moved into case IDLE_STATE", built by `refresh_in_idle_case()`. It is caught; the probe count is now 21. The existing probes are kept.

## Verification
- Full suite: `122/122 (sh via zsh, py via python3)`.
- `git diff --numstat -- src`: empty.
- The mutation was applied to a scratch copy (not via git checkout). The scratch directory `build-tmp/wo-2026-11-16-001-stage6-r2/` keeps only `suite.sh`, my runner script.

## Deviations
None. Changes are uncommitted.
