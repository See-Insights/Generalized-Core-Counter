AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-11, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-09-003-standby-latch`).
- Run the host suite in place, and local ARM builds in scratch copies.
- Write temporary files under **`build-tmp/wo20261009-003-stage7/`**, and remove **only that directory**.
- Mutate only in scratch copies.

**Not authorized:**
- Changes outside your scratch directory.
- Commits, pushes, stash, reset or checkout.
- Device, network or AWS access.
- Deleting anything you didn't create, including `build-tmp/` itself or the files already in it.

# Stage 7 dispatch: WO-2026-10-09-003 (the modem standby latch clears once a day)

**Goal, in plain language:** a device whose modem standby was suppressed after a slow teardown gets standby back without a reboot, unless its modem is still failing.

**Binding spec:** `docs/work-orders/WO-2026-10-09-003-standby-latch.md`, including rulings 1–5, the tests and the Stage 7 checks.

**Inputs:**
- Design: `docs/work-orders/WO-2026-10-09-003-step0-report.md`, option (b).
- Implementation: `docs/work-orders/WO-2026-10-09-003-stage6-copilot-dispatch.md`, and Copilot's report (the end of `build-tmp/WO-2026-10-09-003-stage6-copilot-output.log`).

Treat every claim as unverified.

**What changed** (uncommitted, against HEAD `c1c0f3a`):
- `src/state/State_Report.cpp`: inside `if (due)`, after `SystemConfig::set_lastDailyCleanup(now);`, it clears `session.modemUnstable`, `modemStandbySuppressed`, `modemUnstableReason` and `modemConnectTimeoutCount` when `modemUnstable` is set, and logs one `MODEM_POLICY` line. Claimed +9 lines (7 code, 2 comment).
- New `tests/standby_latch_daily_clear_test.{sh,cpp}`.

## Test-suite hygiene

`tests/thermal_coupling_structural_test.py` scans the whole worktree. Run the host suite before you create any copy of `src/`, or after removing it.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

1. **Linkage and build.**
   - Build HEAD (`git archive HEAD` into your scratch directory) and the working tree, each with a fresh `BUILD_PATH_BASE`. Give text/data/bss for both and the delta. Copilot claims 151468 / 1094 / 2220 for the change.
   - Show in the ELF (`nm` or disassembly) that the reporting handler contains the new path: the log string referenced, and the clears guarded by the `modemUnstable` test.
2. **Tests.** Report the full suite as `N/N (sh via zsh, py via python3)`. The baseline before the change is 75/75.
3. **The goal and the WO's four tests.**
   - A latched device at the daily boundary clears all four fields and logs once.
   - An unlatched device logs nothing.
   - With `due` false (no boundary, or an untrusted clock) nothing clears.
   - After a clear, a teardown over 10 s re-trips through the unchanged path (`State_Sleep.cpp:888-889` at this HEAD).
4. **Extra check A: no clear in open hours before the night's verdict.**
   - Establish when `due` is true. Trace `due`, `boundary` and `close` in `handleReportingState()`.
   - Confirm the clear can run only at the daily boundary, before the night's full modem-off, never in open hours. Cover CONNECTED, KEEP_ALIVE and INTERMITTENT, and the close-hour-24 case.
   - State any path where standby could return during open hours without a modem-off having run since the clear. Accepted edge (ruling 4): if a night has no modem-off, standby returns without a verdict.
5. **Extra check B: `State_Sleep.cpp` and `StateMachine.h` unchanged.** `git diff` for both is empty. `maybeClearModemUnstable()` and the trip thresholds (`:31-32`) are untouched.
6. **Retirement and writers.** List every writer of the four fields in `src/`. Confirm the only new one is this block, and that nothing else now behaves differently: for example, the connect-timeout trip path in `State_Connect.cpp`, and the `lastTeardownEndMs` reconnect defer in `State_Report.cpp`.
7. **Tests and mutations.**
   - Does the new test drive the real code, with byte-checked extraction (§13 g) and mutation copies outside the repo?
   - Re-run Copilot's mutations: clear outside `if (due)`, each field dropped, and clear unconditional.
   - Add one: change the log level or text so the line no longer matches the WO. It should be caught, or report that the test doesn't pin the text.
8. **Budget.** Net `src/` code lines against +20.

## Verdict (your final message; it is saved as the verdict file)

- **Header:** one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED; the model and reasoning used; the test interpreter line.
- **A table of checks 1–8.**
- **Findings,** with file:line and severity. Separate observation from inference.
- **Budget versus actual,** as one row.
- **Confirmations:** the tree matches the start, and you deleted only your scratch directory.

Limit: 120 lines.
