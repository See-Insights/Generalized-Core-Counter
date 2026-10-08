AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-07, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-07-004-occupancy-reports`, HEAD `400ff48`).
- Run the host suite in place.
- Run a local ARM build in a scratch copy.
- Write temporary harnesses and build copies under your scratch directory **`build-tmp/wo20261007-004-stage7/`**, and remove **only that directory** when done.
- Temporarily mutate files for checks, preferably in scratch copies. If you mutate in place, restore byte-identically by rewriting in place.

**Not authorized:**
- Lasting edits, commits, pushes, stash, reset or checkout.
- Flashing, device settings, network access or AWS access.
- Deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`.

# Stage 7 dispatch: WO-2026-10-07-004 (occupancy changes report by mode)

**Goal, in plain language:** every occupancy change, start or end, is reported right away when the mode in use is KEEP_ALIVE or CONNECTED. In every other mode, changes wait for the scheduled report, as they do today.

**Binding spec:** `docs/work-orders/WO-2026-10-07-004-occupancy-reports-by-mode.md`, including the architect's rulings and "Stage 6 result and the architect's decisions".

**Inputs:**
- Design and evidence: `docs/work-orders/WO-2026-10-07-004-step0-report.md`. Background rules: `docs/work-orders/WO-2026-10-07-003-occupancy-report-rules.md`.
- Implementation: `docs/work-orders/WO-2026-10-07-004-stage6-copilot-dispatch.md`, and Copilot's report (the final section of `build-tmp/WO-2026-10-07-004-stage6-copilot-output.log`).

Treat every claim in them as unverified.

**What changed** (five `src/state` files, +14 net code lines claimed):
- `reportsOccupancyChangesNow()` in `State_Common.h` (KEEP_ALIVE or CONNECTED only), used at the five decision sites.
- A pending-flag check (`occupancy change pending`) after the state-entry bookkeeping in `handleSleepingState()`, and another in `handleIdleState()`.
- In `State_Report.cpp`, the `already connected` branch clears `session.occupancyChangeTriggered`.

## Test-suite hygiene

`tests/thermal_coupling_structural_test.py` scans the whole repo, including `build-tmp/`. Run the host suite before you create any copy of `src/`, or after removing it. A failure caused by your own copy is not a finding.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

### Mandatory (workflow §3, Stage 7)

1. **Linkage.**
   - `reportsOccupancyChangesNow()` and both pending checks have production call sites and are present in the linked ELF (`nm` or disassembly; the predicate may be inlined, so show its call sites or inlined body).
   - Show the chain for one path: decision site → predicate → `PowerManager::effectiveConnectionMode()`.
2. **Local toolchain build.** Boron, Device OS 6.4.1, the §3 command, with a **fresh `BUILD_PATH_BASE`** (a directory that doesn't exist yet). Record text/data/bss against the base (`400ff48`) build of 150964 / 1090 / 2196.
3. **Tests.** Report the full suite as `N/N (sh via zsh, py via python3)`. Run `.sh` files with zsh, never bash.
4. **Verify the binary (§12.5).** In the ELF, confirm that the sleep-prep pending check comes before any `System.sleep()` call on the same pass, and that Report's `already connected` branch stores false to the flag.
5. **Alerts.** Confirm that no alert is raised, ranked or cleared differently.

### WO-specific checks

6. **No INTERMITTENT path gained an immediate report.**
   - Enumerate every path that can set `session.occupancyChangeTriggered` or reach REPORTING because of an occupancy change.
   - Show that with mode in use INTERMITTENT, DISCONNECTED, or downgraded KEEP_ALIVE (OCCUPANCY, configured 3, `lowBatteryMode`), none goes to REPORTING right away, apart from the stale-flag case in check 8.
7. **The rule table, independently.** Rebuild modes {INTERMITTENT, KEEP_ALIVE, CONNECTED, DISCONNECTED, downgraded KEEP_ALIVE} × {start, end} × {awake in Idle, main-loop handler outside Idle, wake from sleep}, from the code. Compare it with Copilot's table, and flag any cell that differs.
   - Include the soak case (end latched by the main-loop handler while SLEEPING, in KEEP_ALIVE): the next pass must reach REPORTING before any sleep.
   - Confirm that CONNECTED reaches a deciding point for start and end while open.
8. **The stale flag after a mode change (the architect's condition).** A flag is latched while the mode in use is KEEP_ALIVE or CONNECTED. Before it is consumed, the mode in use becomes INTERMITTENT (downgrade or ledger change). Confirm this produces **at most one report per occupancy change, never a repeat**, on every path:
   - Report consumes the flag offline, at `:216-217`;
   - Report clears it when already connected (the new `:275` clear);
   - check that no branch of `handleReportingState()` can return to Idle or Sleep with the flag still set. Look for early returns before `:216`, the config-invalid path at `:158`, and anything else.

   If any path can leave the flag set and re-trigger, that is a **FAIL**, and it becomes the round-2 fix.
9. **Livelock check (ruling 1).**
   - With `Particle.connected()` true and a pending flag, confirm that Idle → Report → Idle happens once, then stops.
   - Confirm that no other loop can form between the two pending checks and any state that doesn't clear the flag: Sleep → Report → Connect → Sleep, firmware-update state, error state, or the daily-close branch at `:223`.
10. **Ordering and side effects of the new sleep-prep exit.**
    - It sits after the `enteredState` bookkeeping in `handleSleepingState()`. Confirm that leaving there doesn't skip anything a SLEEPING → REPORTING transition needs: loop-stage span handling (in `transitionTo()`), breadcrumbs, modem or network-standby state, the wake-cycle stats, and the PIR or sensor re-enable.
    - Confirm that it can't fire in the middle of a teardown that has already started (for example, after the cloud-disconnect or radio-off steps on an earlier pass of the same SLEEPING span).
11. **Test copies match `src/`** (the architect's condition). The new `tests/occupancy_report_by_mode_test.cpp` (and any updated test) extracts code blocks from `src/`.
    - Confirm that every extracted block matches the current `src/` byte-for-byte at the verified commit.
    - Say whether the extraction would fail loudly or silently if `src/` changed.
12. **Tests and mutations.**
    - Re-run Copilot's six mutations and confirm each is caught by its targeted check:
      - drop CONNECTED from the predicate;
      - add INTERMITTENT;
      - change the predicate to `!= INTERMITTENT`;
      - remove the sleep-prep consumer;
      - remove the Idle consumer;
      - remove the `:275` clear.
    - Add one: move the sleep-prep consumer **below** the suppress decision. The soak test should catch it.
    - Review the edits to the four existing tests (`connection_mode_downgrade_structural_test.py`, `connection_mode_downgrade_test.sh`, `loop_stage_sleep_prep_exclusion_test.py` 17 → 18, `occupancy_session_restart_test.sh`): did any change weaken an assertion?
13. **Budget.** Net `src/` code lines (nonblank, non-comment) against **+20**; moved lines (`git diff --color-moved=zebra`); replaced call-site lines.

## Verdict (your final message; it is saved as the verdict file)

- **Header:**
  - one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED;
  - the model and reasoning level actually used;
  - the test interpreter line.
- **A table of checks 1–13:** result and one-line evidence each.
- **Check 7's rule table.**
- **Check 8's path list:** the flag's fate on each path through Report.
- **Findings,** each with file:line and severity, and whether it needs a round 2. Separate observation from inference.
- **Budget versus actual**, as one row for the closing record: | Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |.
- **Confirmations:** `git status` and `git diff --stat` match the start, and you deleted only `build-tmp/wo20261007-004-stage7/`.
