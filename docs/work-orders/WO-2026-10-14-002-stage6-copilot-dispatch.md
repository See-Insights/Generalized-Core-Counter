AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-14, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- **Edit:** `src/sensors/ISensor.h`, `src/sensors/PIRSensor.h`, `src/Generalized-Core-Counter.cpp` (`:156` only), `src/state/StateMachine.h` (`:36` only), `src/state/State_Sleep.cpp` (`:1094–1096` only), `src/state/State_Idle.cpp` (`:255` only).
- **Add the new test** `tests/pir_sensor_lifecycle_test.{sh,cpp}` and any fakes it needs under `tests/`.
- **Write three patch files** under `build-tmp/` (see "Three commits").
- **Run** the host suite and local ARM release builds.
- **Scratch directory:** **`build-tmp/wo20261014-002-stage6/`**. Remove **only it** when you finish; keep the patch files.

**Not authorized:**
- Any other `src/` change. Any other test's assertions, any `docs/` file, `lib/` or Device OS.
- A change to the body of `PIRSensor::setup()`: it is renamed, not edited.
- Commits, pushes, merges, branch changes, stash, reset or checkout. Device or network access beyond the Particle compile.
- Deleting anything you didn't create.

**SIZE BUDGET:** net `src/` about **−8** (≤ 0 required); gross `src/` additions only where the rename or a simplification requires them. Test lines separately: **stop and report past +150**.

# Stage 6 dispatch: WO-2026-10-14-002 (Step 6 2b-2: merge `setup()` into `initializeHardware()`, remove `sensorDetect`)

**Goal, in plain language:** the sensor has one start operation instead of two overlapping ones, and the dead `sensorDetect` flag is gone. Device behaviour doesn't change, and the golden payload test passes unchanged.

**Repository:** this worktree, branch `wo/2026-10-14-002-step6-2b2-lifecycle` (from main `3092077`, with the 2b-1 golden test on the base). HEAD is at the commit that adds this dispatch. Don't switch branches.

**Binding spec:** `docs/work-orders/WO-2026-10-14-002-step6-2b2-sensor-lifecycle-cleanup.md`. The design comes from `WO-2026-10-13-002-step0-round2-report.md`, Q2 "2b-2". Where this dispatch and the WO differ, the WO wins; report the difference.

## Citations, re-checked by Claude Code at `3092077` (no shift from round 2)

- **`ISensor.h`:**
  - `:79–84`: the `setup()` doc comment and `virtual bool setup() = 0;` at `:83`;
  - `:114–120`: the `initializeHardware()` doc, and its default body `{ return setup(); }` at `:120`, **the only caller of `ISensor::setup()`**. A bare-name and qualified search found no other caller in `src/` or `tests/`; `occupancy_report_by_mode_test.cpp:537`'s `e.setup()` is that test's own `ExitCase`.
- **`PIRSensor.h:39–55`:** `bool setup() override { … }`. The body configures pins, drives `disableModule`/`ledPower` low, does `attachInterrupt(intPin, pirISR, RISING)`, calls `reset()`, and sets `_isReady = true`.
- **`sensorDetect`:**
  - declared at `Generalized-Core-Counter.cpp:156`;
  - `extern` at `StateMachine.h:36`;
  - read at `State_Sleep.cpp:1094` and `:1096` (in a `Log.info` with `sensor=%d`) and `State_Idle.cpp:255`;
  - **never written** anywhere.

## Three commits (you don't commit)

1. **Test first.** Add `tests/pir_sensor_lifecycle_test.{sh,cpp}`. It compiles the **real** `src/sensors/PIRSensor.h` (and `.cpp` if needed) against fakes for `pinMode`, `digitalWrite`, `attachInterrupt`, `detachInterrupt` (and whatever else it needs).
   - **Record the exact call order** through the start operation, `onSleep()` and `onWake()`.
   - **Check that the ISR captured by the fake `attachInterrupt` sets the motion flag**, so that `loop()` or the documented getter sees motion.
   - **Write the test against the start operation by its interface name** (`initializeHardware()`), so it passes **unchanged** before and after the rename: today `initializeHardware()` forwards to `setup()`.
   - **Run it on the unchanged code** and show it passes. Then `git diff > build-tmp/WO-2026-10-14-002-commit-1-test.patch` (only the new test files).
2. **(a) 6a, merge `setup()` into `initializeHardware()`:**
   - delete `ISensor::setup()` with its doc (`:79–84`);
   - make `initializeHardware()` pure virtual (`virtual bool initializeHardware() = 0;`), and update its doc comment so it no longer mentions `setup()`;
   - rename `PIRSensor::setup()` to `initializeHardware()` and update its doc. **The body is unchanged.**

   The lifecycle test and the golden test must pass. Then `git diff > build-tmp/WO-2026-10-14-002-commit-12-a.patch` (cumulative: test + (a)).
3. **(b) 6b, remove `sensorDetect`:**
   - delete `:156` and the `extern`;
   - at `State_Sleep.cpp:1094`, simplify the condition to `ledRemaining > 0`, and drop `sensor=%d` and its argument from the log line;
   - at `State_Idle.cpp:255`, drop `sensorDetect ||`.

   Leave every other token unchanged. Claude Code computes (b) as the rest of the tree.

## Tests and checks

- **The 2b-1 golden test** (`tests/report_payload_golden_test.sh`) must pass **unchanged** after each step.
- **Mutation:** in a scratch copy under `$TMPDIR` (never in `src/`), swap two calls in the start operation's body, for example `attachInterrupt` before the `pinMode` calls. Show that the lifecycle test fails, naming the check that caught it.
- **Full suite** before and after: `N/N (sh via zsh, py via python3)`. **The baseline is 77/77.** Run shell tests with `zsh`. No copy of `src/` under the worktree while the suite runs.
- **Local ARM release build** with a fresh `BUILD_PATH_BASE` under your scratch directory. Give text/data/bss before (built from `git show HEAD:` sources into scratch) and after. List `nm -C -S` differences for `PIRSensor`, `ISensor` and the vtable.
- **Size:** net `src/` lines for (a), (b) and the total, gross additions, the test lines, and `git diff --stat`.

## Implementation Report (required, as your final message)

- **Per commit:** files, net and gross lines.
- **The lifecycle test:**
  - the recorded call sequences;
  - proof it passed on the unchanged code;
  - the mutation result.
- **Results:** the golden test after each step; the suite before and after; builds and sizes; the `nm` differences.
- **Confirmations:** the patch files exist, and you removed only your scratch directory.
- **Deviations** (or "none"), and **the model used**.
