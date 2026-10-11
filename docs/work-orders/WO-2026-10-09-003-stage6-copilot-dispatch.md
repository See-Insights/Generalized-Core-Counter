AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-11, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- Edit `src/state/State_Report.cpp`: inside the existing `if (due)` block only, after `SystemConfig::set_lastDailyCleanup(now);` (`:82`).
- Add or update tests under `tests/`.
- Run the host suite and a local ARM release build.
- Your scratch directory is **`build-tmp/wo20261009-003-stage6/`**. Remove **only it** when you finish.

**Not authorized:**
- Any other `src/` change. In particular, **nothing in `State_Sleep.cpp`** (not `maybeClearModemUnstable()`, the trip thresholds `:31-32`, or the trip check), and nothing in `StateMachine.h`.
- Any `docs/` file, `lib/` or Device OS.
- Commits, pushes, merges, branch changes, stash, reset or checkout. Flashing, device or network access beyond the Particle compile.
- Any new persisted field, timer, schedule or RAM field.
- Deleting anything you didn't create, including `build-tmp/` itself or the files already in it.

**SIZE BUDGET:** at most **+20 net `src/` code lines** (the WO expects about +8). Over +20 means STOP and report. Don't compress code.

# Stage 6 dispatch: WO-2026-10-09-003 (the modem standby latch clears once a day)

**Goal, in plain language:** a device whose modem standby was suppressed after a slow teardown gets standby back without a reboot, unless its modem is still failing.

**Repository:** this worktree, branch `wo/2026-10-09-003-standby-latch`, HEAD at the commit that adds this dispatch (main is merged in). Don't switch branches.

**Binding spec:** `docs/work-orders/WO-2026-10-09-003-standby-latch.md`, including the rulings 1–5 and the tests. **Design and citations:** `docs/work-orders/WO-2026-10-09-003-step0-report.md`, option (b). Where this dispatch and the WO differ, the WO wins; report the difference.

## What to implement

| Item | Where | Change |
|---|---|---|
| C | `State_Report.cpp`, inside `if (due) { … }`, after `SystemConfig::set_lastDailyCleanup(now);` (`:82`) | If `session.modemUnstable` is set, clear the four RAM `session` fields (`StateMachine.h:62-65`): `modemUnstable = false`, `modemStandbySuppressed = false`, `modemUnstableReason = 0`, `modemConnectTimeoutCount = 0`. Log exactly one line: `MODEM_POLICY: standby re-allowed at daily close; night modem-off decides`. Add a short comment saying why: the night's full modem-off is the verdict, and a still-slow modem re-trips through the existing check (`State_Sleep.cpp:888-889` at this HEAD; the WO says `:887-888`, before #86 added a line). If the device isn't latched, do nothing and log nothing. |

Don't add any other trigger, and don't touch `lastTeardownEndMs` or any other field.

## Tests (outside the budget)

1. **New host test** (for example `tests/standby_latch_daily_clear_test.{cpp,sh}`). Drive the **real** block: compile it, or extract it byte-for-byte with a loud `COPY_MISMATCH` check (§13 g), following `tests/occupancy_report_by_mode_test.sh`. Cover each case:
   - **Latched at the boundary:** a latched device (`modemUnstable` true, `modemStandbySuppressed` true, reason 1, timeout count 2) reaching the boundary (`due` true) clears all four fields and logs the line **exactly once**.
   - **Not latched:** a device that isn't latched at the boundary logs nothing, and the fields stay as they were.
   - **`due` false** (no boundary, or an untrusted clock): no clear and no log.
   - **Re-trip still works:** after a clear, a teardown over 10 s re-trips through the existing path. Use the existing `State_Sleep.cpp` check unchanged, by compiling it or by a byte-checked copy. If that isn't feasible on the host, say so and show the structural evidence that the path is untouched.
2. **Mutations,** each caught by its own targeted check, not by a compile failure:
   - the clear moved outside `if (due)`;
   - one of the four fields not cleared (report which one you dropped);
   - the clear made unconditional (no `modemUnstable` check).
3. **Existing tests** that pin `State_Report.cpp` lines: update only what they pin, and report each change.

## Verification

1. **Full host suite,** before and after, as `N/N (sh via zsh, py via python3)`. The baseline on this branch is **75/75**. No copy of `src/` under the worktree while it runs.
2. **Local ARM release build:** a fresh `BUILD_PATH_BASE` under your scratch directory. Give text/data/bss.
3. **Size:** net `src/` code lines, and `git diff --stat`.
4. **Confirm** that `git diff -- src/state/State_Sleep.cpp src/state/StateMachine.h` is empty.

## Implementation Report (required, as your final message)

- The change, with net lines.
- Tests and mutations, with results.
- Commands and results, with interpreters and sizes.
- Confirmation that you deleted only `build-tmp/wo20261009-003-stage6/`.
- Deviations (or "none").
- The model and reasoning level used.
