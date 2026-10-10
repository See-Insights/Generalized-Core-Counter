AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-09, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- Edit `src/state/State_Idle.cpp`: the entry test at `:37-39` and the CONNECTED branch at `:119-133` only.
- Edit `docs/FIELD_MEANINGS_REFERENCE.md`: the TimeDiag entry only (ruling 3).
- Add or update tests under `tests/`.
- Run the host suite and a local ARM release build.
- Your scratch directory is **`build-tmp/wo20261009-001-stage6/`**. Remove **only it** when you finish.

**Not authorized:**
- Any other `src/` change, including `logTimeDiag()` itself, `State_Sleep.cpp:1002`, and the Idle ceiling (`:295-298`).
- Any other `docs/` file, `lib/` or Device OS.
- Commits, pushes, merges, branch changes, stash, reset or checkout. Flashing, device or network access beyond the Particle compile.
- A new persisted field, timer or flag beyond one function-local static last-key.
- Deleting anything you didn't create, including `build-tmp/` itself.

**SIZE BUDGET:** at most **+10 net `src/` code lines** (Step 0 estimated +6 to +8). Over +10 means STOP and report. Don't compress code.

# Stage 6 dispatch: WO-2026-10-09-001 (Idle logs TimeDiag only on change)

**Goal, in plain language:** in Idle's CONNECTED branch, `logTimeDiag()` runs only when its content changes or at a state transition, never on every pass.

**Repository:** this worktree, branch `wo/2026-10-09-001-timediag-on-change`, HEAD at the commit that adds this dispatch. Don't switch branches.

**Binding spec:** `docs/work-orders/WO-2026-10-09-001-timediag-on-change.md`, the rulings 1–3 and the constraints. **Design and citations:** `docs/work-orders/WO-2026-10-09-001-step0-report.md` §3. Where this dispatch and the WO differ, the WO wins; report the difference.

## What to implement

| Item | Where | Change |
|---|---|---|
| E | `State_Idle.cpp:37-39` | Capture `const bool enteredIdle = (state != oldState);` **before** `publishStateTransition()` runs (it sets `oldState = state`). Use that local in the existing `if`. |
| K | `State_Idle.cpp:119-133`, the CONNECTED branch | Compute `isWithinOpenHours()` once, and pass the same value to `logTimeDiag()`. Build one small integer key from `isOpen`, `parkOpenness`, clock-trusted (the same source `logTimeDiag()` prints as `trusted=`) and `Time.isValid()`. Keep one function-local `static` last-key, initialised to a value no real key can have. Call `logTimeDiag()` only if `enteredIdle` or the key differs from the last, then store the key. **Don't gate on `Time.isValid()` or `isWithinOpenHours()`:** they only feed the key. Keep `isOpen=` and `openness=` separate. Keep the existing comment's meaning, and add one short comment saying why it logs on change. The `park closed` transition is unchanged. |
| D | `docs/FIELD_MEANINGS_REFERENCE.md`, the TimeDiag entry (about `:27-29`) | **Represents:** "Time diagnostic. Logged on entry to Idle, when `isOpen`, `openness`, `trusted` or `valid` changes while CONNECTED in Idle, and once per sleep prep." Add `trusted=`, `openness=`, `syncAgeMs=` and `lastSyncEpoch=` to the heading. |

## Tests (outside the budget)

1. **New host test** (for example `tests/idle_timediag_on_change_test.{cpp,sh}`). Drive the **real** Idle CONNECTED branch: compile it, or extract it byte-for-byte with a loud `COPY_MISMATCH` check, following `tests/occupancy_report_by_mode_test.sh`. Stub `logTimeDiag()` to count calls.
   - **1000 passes** in CONNECTED and Open with no key change: **exactly 1** TimeDiag (the entry pass).
   - Then flip each key field once (`openness` Open→Unknown, `trusted` true→false, `isOpen`, `valid`). Each flip gives **exactly one** more TimeDiag, then silence for the rest of the passes.
   - **Re-entry:** leaving Idle and returning logs again on the entry pass.
   - **Ticking values don't count:** advancing `Time.now()` or `millis()` across the 1000 passes adds no lines.
   - **Closed:** `parkOpenness == Closed` still goes to Sleep with `park closed`.
2. **Mutations** (each caught by its targeted check, not by a compile failure):
   - make the call unconditional again;
   - drop `trusted` from the key;
   - use `state != oldState` after `publishStateTransition()` instead of the captured local (entry logging stops).
3. **Existing tests** that pin Idle lines or transition counts: update only what they pin, and report each change.

## Verification

1. **Full host suite,** before and after, as `N/N (sh via zsh, py via python3)`. The baseline is 73/73. No copy of `src/` under the worktree while it runs.
2. **Local ARM release build:** a fresh `BUILD_PATH_BASE`. Give text/data/bss against v40's 151012 / 1090 / 2196.
3. **Size:** net `src/` code lines, and `git diff --stat`.

## Implementation Report (required, as your final message)

- The change per item, with net lines.
- Tests and mutations, with results.
- Commands and results, with interpreters and sizes.
- Confirmation that you deleted only `build-tmp/wo20261009-001-stage6/`.
- Deviations (or "none").
- The model and reasoning level used.
