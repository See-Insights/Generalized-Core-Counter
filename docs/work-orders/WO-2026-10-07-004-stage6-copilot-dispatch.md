AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- Edit:
  - `src/state/State_Common.h` (the predicate and its include);
  - `src/state/State_Idle.cpp` (`:59` and the new consumer);
  - `src/state/State_Modes.cpp` (`:80`, `:143`);
  - `src/state/State_Sleep.cpp` (`:1638`, `:1719` and the new consumer near the top of sleep prep);
  - `src/state/State_Report.cpp` (`:275` only).
- Add or update tests under `tests/`, including `tests/stubs/`.
- Run the host suite and a local ARM release build.
- Your scratch directory is **`build-tmp/wo20261007-004-stage6/`**. Keep every temporary file in it and remove **only it** when you finish.

**Not authorized:**
- Commits, pushes, merges, branch changes, stash, reset or checkout.
- Flashing, device settings, or AWS or network access beyond the Particle compile.
- Any edit to `lib/`, `project.properties`, `docs/` or Device OS, and running `bump_version.sh`.
- Any new state, timer, flag, persisted field, status field or alert change.
- Changing any log string.
- Fixing the `led=` unit bug at `State_Sleep.cpp:76`, or anything else in the Step 0 "Open points".
- **Deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`.**

**SIZE BUDGET:** at most **+20 net `src/` code lines** (nonblank, non-comment, including braces, declarations and includes). Step 0 estimated +18. Over +20 means STOP and report. Don't compress code to meet it (`AI_DEVELOPMENT_WORKFLOW.md` §12 guardrail 3).

# Stage 6 dispatch: WO-2026-10-07-004 (occupancy changes report by mode)

**Goal, in plain language:** every occupancy change, start or end, is reported right away when the mode in use is KEEP_ALIVE or CONNECTED. In every other mode, changes wait for the scheduled report, as they do today.

**Repository:** branch `wo/2026-10-07-004-occupancy-reports`, cut from main at **`400ff48`** (after PR #72 merged). Don't switch branches. Files under `docs/` are records; don't touch them.

**Binding spec:** `docs/work-orders/WO-2026-10-07-004-occupancy-reports-by-mode.md`, including the architect's two rulings. The design and evidence are in `docs/work-orders/WO-2026-10-07-004-step0-report.md`, section 3. Where this dispatch and the WO differ, the WO wins; report the difference.

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Work only from the WO, and report any correction to the spec as a deviation. If the change needs a decision this dispatch doesn't settle (for example, the consumer placement turns out to need a new flag or state), stop and report it.

## What to implement (Step 0's option B, amended by the rulings)

| Item | Where | Change |
|---|---|---|
| P | `State_Common.h` | Add `inline bool reportsOccupancyChangesNow()`. It returns true **only** when `PowerManager::instance().effectiveConnectionMode()` is `INTERMITTENT_KEEP_ALIVE` **or** `CONNECTED` (ruling 2: a positive list; INTERMITTENT and DISCONNECTED never report right away). Add the `power/PowerManager.h` include if it isn't already reachable, and a short doc comment in the file's style. |
| R | `State_Idle.cpp:59`, `State_Modes.cpp:80,143`, `State_Sleep.cpp:1638,1719` | `const bool reportNow = reportsOccupancyChangesNow();`. Same line count. Leave the comments describing "KEEP_ALIVE only" accurate; update them where they'd now be wrong. |
| S | `State_Sleep.cpp`, at the top of sleep prep (Step 0 report: after `:333`) | If `session.occupancyChangeTriggered` is set, `transitionTo(REPORTING_STATE, "<label>")` and return, before any sleep-or-suppress decision. **New `StateReq` label:** this is a new transition reason, not a changed log string. Use `occupancy change pending`. Make sure this runs on the same pass as the wake handling that may set the flag, and before any `System.sleep()` is committed. Explain in your report exactly where it sits relative to the wake handler and the suppress decision at `:1752`. |
| I | `State_Idle.cpp`, after the occupancy block (Step 0 report: after `:75`) | The same consumer: a pending flag goes to REPORTING with the same label. This covers CONNECTED, which doesn't sleep while open, and every awake case where the handler latched outside Idle. |
| C | `State_Report.cpp:275` | In the `already connected` branch, clear `session.occupancyChangeTriggered` (ruling 1). The queued payload already carries the change. |

## Tests (outside the budget)

1. **Rule-table host test**, new (for example `tests/occupancy_report_by_mode_test.cpp` + `.sh`, matching the existing pairs).
   - Use the real state functions with stubs as existing state tests do; `tests/hourly_while_occupied_test.*` and `tests/occupancy_session_restart_test.sh` are good models.
   - Cover 3 modes (INTERMITTENT, KEEP_ALIVE, CONNECTED) × start/end × the awake path and the wake-from-sleep path.
   - Add DISCONNECTED for start and end: no immediate report.
   - Add a **downgraded KEEP_ALIVE** case (OCCUPANCY, configured 3, `lowBatteryMode` set): it must behave as INTERMITTENT.
   - **Expected:** KEEP_ALIVE and CONNECTED go to REPORTING right away. INTERMITTENT, DISCONNECTED and downgraded-KEEP_ALIVE set no flag and don't go to REPORTING.
2. **The soak case.** Occupancy end is latched by the main-loop handler while `state == SLEEPING_STATE` (KEEP_ALIVE). The next sleep-prep pass must go to REPORTING (`occupancy change pending`), not to sleep or suppress.
3. **Ruling 1's case.** A CONNECTED device (`Particle.connected()` true) with a pending flag reports once (`already connected`) and returns to Idle with the flag cleared. On the next pass there is no second REPORTING.
4. **Mutations** (each must be caught by its targeted check, not by a compile failure):
   - drop CONNECTED from the predicate;
   - add INTERMITTENT to the predicate;
   - remove the sleep-prep consumer;
   - remove the Idle consumer;
   - remove the `:275` clear.

   Run each one on a copy, or restore it byte-identically by rewriting in place.
5. **Existing tests** that pin the changed lines or the `reportNow` expression (for example `connection_mode_downgrade_structural_test.py` counts mode-in-use reads, and `hourly_while_occupied_test`): update only what they pin, and report each change with its reason.

## Verification (run all; report results)

1. **Full host suite,** before and after, as `N/N (sh via zsh, py via python3)`. **Run it with no copy of `src/` anywhere under the repo**, because `thermal_coupling_structural_test.py` scans the whole repo, including `build-tmp/`.
2. **Local ARM release build:** boron, Device OS 6.4.1, the §3 command, with a **fresh `BUILD_PATH_BASE`** (a directory that doesn't exist yet) under your scratch directory. Give text/data/bss against the base build. Show with `nm` or disassembly that `reportsOccupancyChangesNow` is present or inlined at the call sites.
3. **Size:** net `src/` code lines against +20, and `git diff --stat`.

## Implementation Report (required, as your final message)

- The change per item, with net lines.
- The rule table as tested: modes × start/end × path → result.
- Exactly where S sits in the sleep handler, and why it runs before any sleep is committed.
- Tests and mutations, with results.
- Commands and results, with interpreters and sizes.
- Confirmation that you deleted only `build-tmp/wo20261007-004-stage6/`.
- Deviations (or "none").
- The model and reasoning level actually used.
