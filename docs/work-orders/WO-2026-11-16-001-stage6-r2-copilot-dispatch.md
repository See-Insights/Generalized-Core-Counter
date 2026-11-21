AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: low
AUTHORIZATION SCOPE: edit `tests/s2_alarm_watchdog_wake_structural_test.py` only; write the report; run host tests; scratch `build-tmp/wo-2026-11-16-001-stage6-r2/` (create it; use only that) / NOT authorized: any change under `src/` or `lib/`, any other test file, committing, pushing, merging, flashing, creating PRs or tags, deleting anything you did not create (including `build-tmp/` itself), `git checkout`, `git restore` or `git stash` while your work is uncommitted.

# Stage 6 dispatch (fix after Stage 7 round 1, VERIFIED WITH FIXES): WO-2026-11-16-001
Binding spec: `docs/work-orders/WO-2026-11-16-001-watchdog-feed-through-register.md`; the verdict `docs/work-orders/WO-2026-11-16-001-stage7-r1-verdict.md` (one Medium, tests only; read the Finding and the mutation table). Base: this branch HEAD (`99ece4e` plus the verdict and this dispatch). **Budget: `src/` 0 lines; the test edit about +15 lines.** Anything outside the finding is listed in the report, not fixed.

## Do
1. **Baseline:** CI discovery in this worktree, `N/N (sh via zsh, py via python3)` (expect 122/122).
2. **Fix:** in `tests/s2_alarm_watchdog_wake_structural_test.py` (about line 110-112) require the loop order `thrashGuard.loop(state, millis())` < `ExternalWatchdog.refresh()` < `checkClockResync()`, with a failure message naming it (the refresh must sit after the whole state switch, so every state feeds the chip). Add one mutation to the file's mutation list: the refresh/latch/log block (the `static int lastRefreshRc` line through `lastRefreshRc = refreshRc;`) moved into `case IDLE_STATE:` of the state switch; it must be caught. Keep the existing mutations ("refresh missing", etc.).
3. **Show the failing-before evidence:** apply the IDLE_STATE move on a scratch copy and show the test at `99ece4e` passes it (that is the finding), then fails with your fix.
4. **Verify:** full suite `N/N (sh via zsh, py via python3)`; `git diff --numstat -- src` empty. Mutate from a backup (`cp`), never `git checkout`.

## Report
`docs/work-orders/WO-2026-11-16-001-stage6-r2-copilot-report.md` (at most 150 lines): model and level actually used, interpreter line, baseline, the failing-before evidence, the lines added, the mutation caught, the suite result, deviations. Leave your changes **uncommitted**.
