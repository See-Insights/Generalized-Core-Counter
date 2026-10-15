AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-15, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- **Edit:**
  - `src/MyPersistentData.cpp`: `resetEverything()` and its wrapper at `:1200`;
  - `src/MyPersistentData.h` (the `resetEverything` declaration);
  - `src/persist/CurrentReadings.h` (the wrapper declaration);
  - `src/Generalized-Core-Counter.cpp`: `dailyCleanup()` only, plus any include it needs;
  - `src/state/State_Report.cpp`: the hourly reset at `:100–102` only, plus any include it needs;
  - `src/sensors/SensorManager.{h,cpp}`: **only** if you add the optional wrapper (see design point 4).
- **Tests:** update `tests/persistence_facade_behavior_test.cpp` and `tests/daily_cleanup_boundary_test.py`, add tests, and adjust only what this change requires elsewhere, reporting each.
- **Run** the host suite and local ARM release builds.
- **Scratch directory:** **`build-tmp/wo20261015-002-stage6/`**. Remove **only it** when you finish.

**Not authorized:**
- The `validate()` repair at `MyPersistentData.cpp:713–716`.
- `PIRSensor` or `ISensor`, `_motionDetected`, and any other caller.
- The daily-close order (publish → `dailyCleanup()` → restart).
- Any other `src/` change, `docs/`, `lib/` or Device OS.
- Commits, pushes, merges, branch changes, stash, reset or checkout. Device or network access beyond the Particle compile.
- Deleting anything you didn't create.

**SIZE BUDGET:**
- **`src/`:** net **≤ +20, HARD**: over means STOP and report (§13 (b)). The estimate is +7 without the wrapper, +13 with it. One commit, or two if the gross must stay ≤ 40; report which.
- **Tests:** a **guideline** of +100 added lines (no new harness expected). Record the actual and note any overrun; don't stop for it unless it passes about twice the guideline.

# Stage 6 dispatch: WO-2026-10-15-002 (Step 6 2b-6: wire the sensor's reset; central work stays central)

**Goal, in plain language:** switch the hourly and daily count resets over to the sensor's own `resetTotals()` hook (added in 2b-5a), while the non-sensor parts of the daily reset (`lastCountTime`, `resetCount`) stay central. **No device behaviour changes:** the same fields are cleared, in the same order, at the same moments.

**Repository:** this worktree, branch `wo/2026-10-15-002-step6-2b6-wire-reset` (from main `d89a4ef`, with 2b-1 to 2b-5a merged). HEAD is at the commit that adds this dispatch. Don't switch branches.

**Binding spec:** `docs/work-orders/WO-2026-10-15-002-step6-2b6-wire-reset.md`, including the architect's pre-check table, **Claude Code's citation re-check at `d89a4ef`**, and the "Design (do not widen)" points 1–5. Where this dispatch and the WO differ, the WO wins; report the difference.

## Design (from the WO; do not widen)

1. **`resetEverything()`:** becomes `currentStatusData::resetEverything(bool includeSensorTotals = true)`. It does `lastCountTime` and `resetCount` first, then `if (!includeSensorTotals) return;`, then the six existing setters, unchanged. The `CurrentReadings::resetEverything` wrapper and its declarations take the same parameter. `initialize()` (`:772`) keeps the default `true`.
2. **`dailyCleanup()`** (`Generalized-Core-Counter.cpp:2697`, the call at `:2717`): `CurrentReadings::resetEverything(false);`, then `SensorManager::instance().activeSensor()->resetTotals(ResetPeriod::Daily);`. Everything else in `dailyCleanup()`, and the close order around it, is unchanged.
3. **`State_Report.cpp:101`:** inside the **existing** `COUNTING` guard, replace `CurrentReadings::set_hourlyCount(0);` with `SensorManager::instance().activeSensor()->resetTotals(ResetPeriod::Hourly);`. The guard stays central.
4. **The optional wrapper:** add `SensorManager::resetTotals()` **only** if it removes an include from a central file. Either way, `_sensor == nullptr` (a closed boot) must still reach PIR's hook through `activeSensor()`'s fallback.
5. **Not touched:** `validate()`'s repair, `_motionDetected`, and any other caller.

## Tests

- **Golden test:** passes unchanged, with the fixture byte-identical. Add payload-after-reset vectors only if the harness takes them cheaply; report whether you did.
- **`persistence_facade_behavior_test.cpp`** (around `:223`): keep the existing `resetEverything()` checks, which now clear all eight. Add a `resetEverything(false)` case: it stamps `lastCountTime` and clears `resetCount`, but leaves all six sensor fields as they were.
- **`daily_cleanup_boundary_test.py`** (around `:162`): update the `set_hourlyCount(0)` token to the new hourly call. **Keep the order test** (publish → cleanup → restart).
- **End to end on the host:**
  - **The daily path** (`dailyCleanup()` → `resetEverything(false)` → `resetTotals(Daily)`) records **exactly today's eight setters in today's order**: `lastCountTime`, `resetCount`, then the six.
  - **The hourly path** clears only `hourlyCount`, and only in COUNTING mode. OCCUPANCY clears nothing.
- **The null sensor:** `_sensor == nullptr` still reaches the hook for both periods.
- **Structural:** no `set_hourlyCount(0)` left in `State_Report.cpp`, and `dailyCleanup()` no longer calls the no-argument `resetEverything()`.
- **Confirm `pir_sensor_reset_totals_test` still passes,** because the six setters remain in `resetEverything()`.
- **Mutations** (in `$TMPDIR` copies, never in `src/`), each failing a named check:
  - `dailyCleanup()` calls `resetEverything()` with no argument, so a double reset (count the setters);
  - the hourly hook moved outside the `COUNTING` guard;
  - `resetEverything(false)` returning before `resetCount`;
  - `initialize()` passing `false`.
- **Full suite** before and after: `N/N (sh via zsh, py via python3)`. **The baseline is 80/80.** Run shell tests with `zsh`. No copy of `src/` under the worktree while the suite runs.
- **Local ARM release build,** with the same release recipe as earlier Stage 6s (not `compile-all`) and a fresh `BUILD_PATH_BASE`: text/data/bss before (from `git show HEAD:` sources) and after, with the `nm -C -S` differences explained by symbol.

## Implementation Report (required, as your final message)

- **Per commit:** files, with net and gross `src/` lines and the arithmetic; test lines added and removed.
- **Design choices:** whether the wrapper was added and why; the exact signatures.
- **Test results:** the recorded daily and hourly sequences, by mode; the null-sensor result; the structural checks; each mutation and its catching check; the golden test; the suite before and after.
- **Builds:** sizes and the `nm` differences.
- **Confirmations:** you removed only your scratch directory.
- **Deviations** (or "none"), and **the model used**.
