AGENT: Copilot · MODEL: claude-opus-5 · REASONING: medium
AUTHORIZATION SCOPE: item B only. Edit `src/Generalized-Core-Counter.cpp` (the out-of-memory block in `loop()`, around `:1685`, plus any alert-14 code it makes unreachable), `src/ResetCause.h` (add code 7), `src/state/State_Error.cpp` (`resolveErrorAction()` case 14, if unreachable), `src/MyPersistentData.cpp` (alert 14's severity case, only if unreachable), and the tests (`tests/reset_cause_structural_test.py`, plus one new test); run the host suite and a local ARM release build. Your scratch directory is **`build-tmp/wo20261002-003-stage6-b/`**: keep every temporary file in it and remove **only it** when done / Not authorized: commits, pushes, merges, branch changes, stash, reset, checkout, flashing, device settings, AWS or network access, any edit to `lib/`, `project.properties` or `docs/`, **any change to item A's work** (the sleep-configuration changes in `State_Sleep.cpp` and `StateMachine.h`, its tests, the version files; frozen and hash-checked), and **deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`**. Don't re-run `bump_version.sh`; v34 is already set.
**SIZE BUDGET: about 5 net `src/` code lines for item B (separate from item A's −2). Don't compress code to meet it. Going over means STOP and report.**

# Stage 6 dispatch, item B — WO-2026-10-02-003 (restore the original out-of-memory reset)

**Goal, in plain language (item B):** if memory runs out, the device resets at once with a cause code that says so.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-10-02-003-sleep-config-leak`, `HEAD` `43b8a69`. The working tree holds item A's uncommitted diff (frozen). Files under `docs/` are records: don't touch them.
**Binding spec:** `docs/work-orders/WO-2026-10-02-003-sleep-config-leak.md`, section **"B. Restore the original out-of-memory reset"**, and its acceptance item 4.

## What to implement

1. **In `loop()`** (`Generalized-Core-Counter.cpp`, the `if (outOfMemory >= 0)` block, about `:1685–1694`): replace the log + `RecoveryState::raiseAlert(14)` + `transitionTo(ERROR_STATE, "out of memory")` with the original design:
   ```cpp
   Log.info("out of memory occurred size=%d", outOfMemory);
   delay(100);
   System.reset(RESET_CAUSE_OUT_OF_MEMORY);
   ```
   No `ERROR_STATE` transition, and no suppression at any reset count.
2. **`src/ResetCause.h`:** add `RESET_CAUSE_OUT_OF_MEMORY = 7`, commented "out of memory".
3. **Remove the now-unused out-of-memory route through `ERROR_STATE`, if nothing else uses it.** Decide each of these from the code, and report each decision with its reason:
   - `resolveErrorAction()` case 14 (`State_Error.cpp:41`);
   - the boot-time clear of alert 14 (`Generalized-Core-Counter.cpp:1081`, `clearOomAlertOnBoot`);
   - alert 14's severity case (`MyPersistentData.cpp:864`).

   Once nothing raises alert 14, code reached only through it is unused, but persisted alerts from older firmware may still carry 14. Keep whatever is still reachable, or whatever is needed to handle a persisted 14 safely.

## Tests

- **`tests/reset_cause_structural_test.py`:** extend it so every `System.reset(` in `src/` must carry a distinct, non-zero code, **including 7 at the out-of-memory site**. A bare reset or a duplicate code still fails it.
- **A new test (behavioral against the real source where an existing harness pattern fits, otherwise structural; say which):**
  - with `outOfMemory >= 0`, the loop's out-of-memory block calls `System.reset(RESET_CAUSE_OUT_OF_MEMORY)` directly;
  - it doesn't transition to `ERROR_STATE`, and it doesn't depend on any reset count;
  - **mutation:** restoring the `ERROR_STATE` route (`raiseAlert(14)` + `transitionTo(ERROR_STATE, "out of memory")`) fails it.
- Run mutations on a copy or restore byte-identically. Update an existing test only if it pins text this item changes, and report each one. `tests/publish_with_ack_structural_test.py` must pass unchanged.

## Verification (run all; report results)

1. Full host suite: every `tests/*.sh` with **zsh**, plus every bare `tests/*.py` with python3, as `N/N`, before (item A: 59/59) and after.
2. Local ARM release build (boron, Device OS 6.4.1, after `make clean-user`, in your scratch directory or with outputs restored): text/data/bss against item A's 150692 / 1090 / 2180; `strings` shows `v34-SleepConfigLeak`.
3. Linkage: the out-of-memory block's `System.reset` call is in the ELF.
4. Item B's net `src/` lines against about 5, and `git diff --stat`.

## Implementation Report (required, as your final message)

- The change, with line counts.
- Each alert-14 decision and its reason.
- Tests and mutations, with results.
- Commands and results, with interpreters and sizes.
- Confirmation that you deleted only `build-tmp/wo20261002-003-stage6-b/` and changed nothing of item A's.
- Deviations (or "none").
- The model and reasoning level actually used.
