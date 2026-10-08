AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-08, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- Edit `src/state/State_Report.cpp` and `src/state/State_Sleep.cpp` (only the items below).
- Edit `tests/occupancy_report_by_mode_test.{cpp,sh}`, and add or update other tests under `tests/` only where they pin lines you change.
- Run the host suite and a local ARM release build.
- Your scratch directory is **`build-tmp/wo20261007-004-stage6r2/`**. Keep every temporary file in it and remove **only it** when you finish.

**Not authorized:**
- Commits, pushes, merges, branch changes, stash, reset or checkout.
- Flashing, device settings, or AWS or network access beyond the Particle compile.
- Any edit to `lib/`, `project.properties`, `docs/` or Device OS, and running `bump_version.sh`.
- Any new state, timer, flag, persisted field, status field or alert change.
- Changing any log string.
- Re-testing the mode in the pending checks. The architect accepted the stale-flag case, provided it is never a repeat.
- **Deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`.**

**SIZE BUDGET:** the **WO total** must stay at **≤ +20 net `src/` code lines** (nonblank, non-comment). Round 1 is +14, so this round has **at most +6**. The architect expects about +15 in total. Over the cap means STOP and report. Don't compress code (`AI_DEVELOPMENT_WORKFLOW.md` §12 guardrail 3).

# Stage 6 round 2 dispatch: WO-2026-10-07-004 (occupancy changes report by mode)

**This is the last round under the two-round rule.** If round 2 doesn't reach VERIFIED, the WO stops.

**Goal, in plain language:** every occupancy change, start or end, is reported right away when the mode in use is KEEP_ALIVE or CONNECTED. In every other mode, changes wait for the scheduled report, as they do today.

**Repository:** branch `wo/2026-10-07-004-occupancy-reports`, HEAD `951c934` (records commit on top of main `400ff48`). Round 1's implementation is the **uncommitted** working-tree diff; build on it. Don't switch branches. Don't touch `docs/`.

**Binding spec:** `docs/work-orders/WO-2026-10-07-004-occupancy-reports-by-mode.md`. **Why this round:** `docs/work-orders/WO-2026-10-07-004-stage7-verdict.md`, findings 1–3. The architect approved the fixes below on 2026-10-08, using existing patterns only.

## What to change

| Item | Where | Change |
|---|---|---|
| F1 | `State_Report.cpp`: just after `publishData(...)` (`:69`) | **Take the flag at Report entry,** the way Report already handles `serviceRequestTriggered` (`:150-153`): `const bool occupancyChangeTriggered = session.occupancyChangeTriggered;` then `session.occupancyChangeTriggered = false;`. The payload is queued at `:69`, before every early exit (alert-40 escalation `:136`, config-invalid `:157`, service request `:210`), so it already carries the change on every path. The occupancy branch (`:216`) tests the local and no longer clears the session flag. **Remove round 1's `already connected` clear** (`:275`), which is now redundant. |
| F2 | `State_Sleep.cpp`: the round-1 pending check (now at `:366-372`) | **Move it below the existing `static bool disconnectRequested` declaration and its `enteredState` reset block** (around `:444-482`), and change the condition to `session.occupancyChangeTriggered && !disconnectRequested`. A flag latched after this sleep span's teardown has been requested stays latched and is reported on the first pass after the wake. It must still sit **before** any standby decision, any `System.sleep()` call and the `sleep-timer-occupied-suppress-report` decision. **Confirm, and say in your report,** that `disconnectRequested` is reset once per sleep cycle on every path, including a Sleep→Sleep re-entry (for example after suppress) and after a wake. If it is not, STOP: the check would be gated off permanently. |
| F3 | `tests/occupancy_report_by_mode_test.{cpp,sh}` | Fix the harness (outside the budget): **(a)** Copy blocks **byte-identically**: stub `SensorManager` (or whatever the copied Modes code calls) instead of rewriting `SensorManager::instance().loop()`, and assert in the `.sh` that each copied block equals the `src/` text, so a mismatch fails loudly. **(b)** The soak case must drive the **real order** of `handleSleepingState()`, not a model with the check at the top: if the real pending check moved below the suppress decision, a behaviour assertion must fail, not only a structural one. **(c)** Add no-repeat cases: with a pending flag, each of config-invalid, service request and alert-40 escalation produces **one** report, and the next Idle/Sleep pass does **not** report again. **(d)** Add the teardown case: a flag latched after `disconnectRequested` doesn't go to REPORTING on that span; it goes on the first pass after the wake. **(e)** Keep ruling 1's case: CONNECTED with `Particle.connected()` reports once, then nothing on the next 5 passes. Correct the rule table in your report: CONNECTED normally never reaches the wake-from-sleep checks while open (verdict, check 7). |

## Mutations (each must be caught by its targeted behaviour check, not by compilation alone)

1. **Move the flag take back below the early exits.** The no-repeat cases must fail.
2. **Drop `!disconnectRequested`.** The teardown case must fail.
3. **Move the sleep pending check below the suppress decision.** The soak behaviour case must fail.
4. **Restore the `:275` clear and remove the entry take.** The no-repeat cases must fail.
5. **Round 1's six mutations, re-run.** Adjust any that targeted the removed `:275` line.

## Verification (run all; report results)

1. **Full host suite,** before and after, as `N/N (sh via zsh, py via python3)`. The round-1 baseline is 69/69. **Run it with no copy of `src/` under the repo.**
2. **Local ARM release build:** fresh `BUILD_PATH_BASE`. Give text/data/bss against round 1's 150980 / 1090 / 2196.
3. **Size:** net `src/` code lines for this round, and for the WO total against `400ff48`.

## Implementation Report (required, as your final message)

- The change per item, with net lines for the round and the WO total.
- The corrected rule table.
- F2's placement and the `disconnectRequested` reset confirmation, with lines.
- Tests and mutations, with results.
- Commands and results, with interpreters and sizes.
- Confirmation that you deleted only `build-tmp/wo20261007-004-stage6r2/`.
- Deviations (or "none").
- The model and reasoning level actually used.
