AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-08, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- Edit `src/sensors/SensorManager.cpp:307` and `src/Generalized-Core-Counter.cpp:1127` (the getter swaps).
- Edit `src/cloud/ConfigApply.cpp:351` (the validation limit).
- Correct comments that would become wrong, such as `src/persist/SystemConfig.h:149-151` and `src/sensors/SensorManager.h` around `:92`, if they describe which field creation reads.
- Add or update tests under `tests/`.
- Run the host suite and a local ARM release build.
- Your scratch directory is **`build-tmp/wo20261008-002-stage6/`**. Remove **only it** when you finish.

**Not authorized:**
- Any other `src/` change. In particular, **don't touch `sensorISR`** (`Generalized-Core-Counter.cpp` around `:2711`; ruling 3) or persistence (`MyPersistentData.*`; the layout must not change).
- Any edit to `docs/`, `CHANGELOG.md`, `lib/` or Device OS.
- Commits, pushes, merges, branch changes, stash, reset or checkout. Flashing, device or network access beyond the Particle compile.
- Any new state, timer, flag, persisted field or log-string change.
- Deleting anything you didn't create, including `build-tmp/` itself.

**SIZE BUDGET:** at most **+20 net `src/` code lines**; **about +2 expected**. Over +20 means STOP and report. Don't compress code.

# Stage 6 dispatch: WO-2026-10-08-002 (Step 6 WO 1c, `sensor.type` reaches sensor creation)

**Goal, in plain language:** the `sensor.type` set in the ledgers is the value sensor creation uses, so a configured sensor type is never silently ignored.

**Repository:** this worktree, branch `wo/2026-10-08-002-sensor-type`, HEAD at the commit that adds this dispatch (on top of `2404ad9`, main after #77 merged in). Don't switch branches.

**Binding spec:** `docs/work-orders/WO-2026-10-08-002-sensor-type.md`, fix A and rulings 1–4. **Evidence:** `docs/work-orders/WO-2026-10-08-002-step0-report.md`. Where this dispatch and the WO differ, the WO wins; report the difference.

## What to implement

| Item | Where | Change |
|---|---|---|
| A1 | `SensorManager.cpp:307` | `SystemConfig::get_sensorType()` becomes `SystemConfig::SensorSettings::get_sensorType()`. |
| A2 | `Generalized-Core-Counter.cpp:1127` | The same swap (the `configuredType` used for `SensorDefinitions::getDefinition` and the LED power). |
| V | `ConfigApply.cpp:351` | `validateRange(sensorType, 0, 255, "sensor.type")` becomes `validateRange(sensorType, 1, 1, "sensor.type")`. 0 net lines. |
| Comments | `SystemConfig.h:149-151`, `SensorManager.h` around `:92` | Fix any comment that says creation reads the `sysStatus` field. `sysStatus.sensorType` is now an unused legacy field, kept only for layout. Don't change persistence. |

**Confirm and report:** after A1 and A2, `SystemConfig::get_sensorType()` (the `sysStatus` getter) has no production reader except `sensorISR` (dead, left for WO 2a). List every remaining caller.

## Tests (outside the budget)

1. **Creation reads the sensor store** (new, or an extension of an existing SensorManager/persistence test): with `SensorSettings` type 1 and `sysStatus.sensorType` deliberately different (for example 0), `initializeFromConfig` builds PIR, and `Main:1127`'s definition lookup uses the sensor-store value. Use real code where the harness allows; if you copy blocks, check them byte-for-byte against `src/` and fail loudly on a mismatch (see `tests/occupancy_report_by_mode_test.sh`, `COPY_MISMATCH`).
2. **Validation:** `sensor.type` 1 is accepted. 0, 2 and 255 are rejected, the stored value is unchanged, and the section reports failure. Extend `tests/reporting_interval_cap_test` or follow its pattern.
3. **`tests/persistence_facade_behavior_test.cpp:236-243`** (the Step 5 test that keeps the two fields distinct) and **`tests/persisted_layout_preservation_test.py`** must pass **unchanged**.
4. **Mutations** (each caught by its targeted check, not by compilation):
   - revert A1;
   - revert A2;
   - restore `0, 255`.

   Run each one on a copy.

## Verification

1. **Full host suite,** before and after, as `N/N (sh via zsh, py via python3)`. The baseline on this branch should be 72/72. Have no copy of `src/` under the worktree while it runs.
2. **Local ARM release build,** with a fresh `BUILD_PATH_BASE`. Give text/data/bss against 1b's 151028 / 1090 / 2196 (main `23ccd81`). Show in the disassembly that `initializeFromConfig` and `setup` call `SystemConfig::SensorSettings::get_sensorType`.
3. **Net `src/` lines** and `git diff --stat`.

## Implementation Report (required, as your final message)

- The change per item, with net lines.
- The remaining callers of the `sysStatus` getter.
- Tests and mutations, with results.
- Commands and results, with interpreters and sizes.
- Confirmation that you deleted only `build-tmp/wo20261008-002-stage6/`.
- Deviations (or "none").
- The model and reasoning level actually used.
