AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-15, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- **Edit:** `src/sensors/ISensor.h`, `src/sensors/PIRSensor.h` and `src/sensors/PIRSensor.cpp`.
- **Tests:** add the new test, and adjust only what this change requires elsewhere, reporting each.
- **Run** the host suite and local ARM release builds.
- **Scratch directory:** **`build-tmp/wo20261015-001-stage6/`**. Remove **only it** when you finish.

**Not authorized:**
- **Any caller of the new hook.** This WO is hook-only; 2b-6 wires it.
- Any change to `MyPersistentData.cpp`, `State_Report.cpp`, `Generalized-Core-Counter.cpp`, or the central reset.
- Changing `PIRSensor::reset()`.
- Touching `lastCountTime` or `resetCount`.
- Any other `src/` change, `docs/`, `lib/` or Device OS.
- Commits, pushes, merges, branch changes, stash, reset or checkout. Device or network access beyond the Particle compile.
- Deleting anything you didn't create.

**SIZE BUDGET:** net `src/` **≤ +20** (estimate +16), **one commit**. Over +20: STOP and report. **Tests: stop past +100 added lines.**

# Stage 6 dispatch: WO-2026-10-15-001 (Step 6 2b-5a: the sensor's own reset of its totals, hook only)

**Goal, in plain language:** give the sensor one operation that clears its own hourly or daily totals, exactly as the central code clears them today. It is **added but not yet called**, so this WO changes no device behaviour.

**Repository:** this worktree, branch `wo/2026-10-15-001-step6-2b5a-reset-hook` (from main `2f18c38`, with 2b-1 to 2b-4 merged). HEAD is at the commit that adds this dispatch. Don't switch branches.

**Binding spec:** `docs/work-orders/WO-2026-10-15-001-step6-2b5a-sensor-reset-hook.md`. The design source is `docs/work-orders/WO-2026-10-13-002-step0-round2-report.md`, Q2 "2b-5a"; the Stage 4 ISR-pending finding is in `WO-2026-10-13-002-stage4-verdict.md`. Where this dispatch and the WO differ, the WO wins; report the difference.

## Citations, re-checked by Claude Code at `2f18c38` (no `src/` drift from the pre-check)

- **The central daily reset:** `currentStatusData::resetEverything()` is at `src/MyPersistentData.cpp:689–707`.
  - **Stays central:** `lastCountTime` (`:695`) and `resetCount` (`:696`).
  - **The sensor totals, in this order:** `set_hourlyCount(0)` (`:699`), `set_dailyCount(0)` (`:700`), `set_occupied(false)` (`:703`), `set_lastOccupancyEvent(0)` (`:704`), `set_occupancyStartTime(0)` (`:705`), `set_totalOccupiedSeconds(0)` (`:706`).
- **The hourly reset:** `src/state/State_Report.cpp:101`, `CurrentReadings::set_hourlyCount(0);`, inside the `if (... == SystemConfig::COUNTING)` guard.
- **`PIRSensor::reset()`** (`src/sensors/PIRSensor.h:114–120`) clears `_motionDetected` (`:119`). **Don't reuse it.**
- **Nothing named `resetTotals` or `ResetPeriod`** exists yet.

## Design (do not widen)

- **The interface:** add `enum class ResetPeriod { Hourly, Daily };` and `virtual void resetTotals(ResetPeriod period)`, with a default that does nothing.
- **PIR's implementation,** through the same `CurrentReadings` setters and in the same order as the central code:
  - **Hourly:** `set_hourlyCount(0)`.
  - **Daily:** `set_hourlyCount(0)`, `set_dailyCount(0)`, `set_occupied(false)`, `set_lastOccupancyEvent(0)`, `set_occupancyStartTime(0)`, `set_totalOccupiedSeconds(0)`.
- **`_motionDetected` is left untouched,** and so is every other sensor member.
- **No caller anywhere in `src/`.**

## Tests

1. **The 2b-1 golden test passes unchanged.**
2. **A new test,** for example `tests/pir_sensor_reset_totals_test.{sh,cpp}`, compiling the real `PIRSensor` with recording fakes for the `CurrentReadings` setters. You may extend the 2b-2 lifecycle test's fakes. Cover:
   - **Hourly:** the recorded sequence is exactly `[hourly 0]`.
   - **Daily:** the recorded sequence is exactly the six setters above, in order.
   - **Order matches the central reset:** extract the six sensor-total setter calls from the real `resetEverything()` in `MyPersistentData.cpp` and assert the PIR Daily sequence equals them. That way any future central change is caught.
   - **ISR-pending:** fire the captured PIR ISR, call `resetTotals(Hourly)` (and separately `Daily`), then `loop()` still reports motion.
   - **Structural:** no call to `resetTotals(` anywhere in `src/` outside its declarations and definitions.
3. **Mutations** (in `$TMPDIR` scratch copies, never in `src/`), each failing a named check:
   - swap two Daily setters;
   - drop one Daily setter;
   - make `resetTotals` clear `_motionDetected`;
   - add a call to `resetTotals` in `State_Report.cpp`.
4. **Full suite** before and after: `N/N (sh via zsh, py via python3)`. **The baseline is 79/79.** Run shell tests with `zsh`. No copy of `src/` under the worktree while the suite runs.
5. **Local ARM release build:** fresh `BUILD_PATH_BASE`; text/data/bss before and after, and the `nm -C -S` differences. Expect only the new function, a vtable slot, and possibly `CSWTCH` renumbering.

## Implementation Report (required, as your final message)

- **Files:** net and gross `src/` lines with the arithmetic, and test lines added and removed.
- **Design:** the exact enum and signature.
- **Test results:** the recorded sequences; the central-order comparison; the ISR-pending result; the structural no-caller check; each mutation and its catching check; the golden test; the suite before and after.
- **Builds:** sizes and the `nm` differences.
- **Confirmations:** you removed only your scratch directory.
- **Deviations** (or "none"), and **the model used**.
