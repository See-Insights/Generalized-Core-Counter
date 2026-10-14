AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-14, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-14-004-step6-2b4-wake-source`).
- Run the host suite and the tests; run local ARM builds in scratch copies.
- Write temporary files under **`build-tmp/wo20261014-004-stage7/`** or `$TMPDIR`, and remove **only** what you create.
- Mutate only in scratch copies.

**Not authorized:**
- Changes outside your scratch locations.
- Commits, pushes, stash, reset or checkout.
- Device, network or AWS access.

# Stage 7 dispatch: WO-2026-10-14-004 (Step 6 2b-4: the wake pin and edge come from the sensor)

**Goal, in plain language:** the sleep code stops hard-wiring the PIR pin; it asks the active sensor for its wake pin and edge. **For PIR, the sleep configuration and wake handling are identical to today.**

**Binding spec:** `docs/work-orders/WO-2026-10-14-004-step6-2b4-sensor-wake-source.md`, including the **"Pre-dispatch check"**:
- the six `intPin` sites;
- `PIN_INVALID` = `0xff` (`hal/inc/pinmap_hal.h:75`);
- `wakeupPin()` = `0xFFFF` on a non-GPIO wake (`wiring/inc/spark_wiring_system.h:275–281`);
- `wakePin` is `pin_t`.

And the **"Architect's addition"**: the pin is stored as `pin_t`, never `uint8_t`, and the narrowing mutation must fail the timer-wake test.

**Inputs:** `docs/work-orders/WO-2026-10-14-004-stage6-copilot-dispatch.md`, and Copilot's report (the end of `build-tmp/WO-2026-10-14-004-stage6-copilot-transcript.md`, from "# Implementation Report"). Treat every claim as unverified.

**What changed** (uncommitted, one commit):
- **`src/`:** +22/−9, net **+13**: `ISensor.h` +9 (the `SensorWakeSource` struct and the default `wakeSource()`), `PIRSensor.h` +2, `State_Sleep.cpp` +11/−9.
- **Tests:** +150 added / −13 removed (the limit is +150):
  - new `tests/sensor_wake_source_test.{sh,cpp}`;
  - re-pointed `occupancy_report_by_mode_test.sh`, `sleep_config_ownership_structural_test.py` and `sleep_config_leak_test.{sh,cpp}`;
  - `PIN_INVALID`/`FALLING` added to two stubs.
- **Claude Code's run:** 79/79; the golden test passes; zero `intPin` tokens left in `State_Sleep.cpp`.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

1. **Every former `intPin` use maps to the sensor's pin, with the same edge.** Go through the six sites at HEAD (`State_Sleep.cpp:1365`, `:1426`, `:1457`, `:1465`, `:1520`, `:1581`) against the new code. For PIR, each must resolve to `intPin`, and `RISING` where an edge is used.
2. **The PIR sleep configuration is equivalent,** in source and in the ELF.
   - **Order:** the ULP and STOP-fallback configs still add `BUTTON_PIN FALLING`, then the PIR pin `RISING`, in that order.
   - **STOP fallback split:** the config was broken into separate statements, so confirm `.duration(...)` and anything else chained after it are unchanged.
   - **ELF:** build HEAD and the working tree (fresh `BUILD_PATH_BASE` each). Give text/data/bss for both; Copilot claims 151220 → 151308, which is +88. Explain every `nm -C -S` change: `handleSleepingState` +0x2c, `PIRSensor::wakeSource()` new, the vtable +4, `CSWTCH`. Show from the disassembly or the call sequence that, for PIR, the `SystemSleepConfiguration::gpio` calls carry the same arguments in the same order as before.
3. **No path where a closed boot loses the PIR wake.** With `_sensor == nullptr`, `activeSensor()` falls back to the configured type or to PIR, so the wake source is `{intPin, RISING}`. Check every sleep entry path, including after a failed sensor init.
4. **Timer wake never takes the PIR path.**
   - `pirWake = sensorCanWake && (wakePin == sensorWake.pin)`, with `wakePin` as `pin_t`.
   - It's false for `0xFFFF` with PIR, and false with a no-source sensor.
   - It's true for a real PIR GPIO wake.
5. **The `pin_t` rule and the narrowing mutation.**
   - Re-run the mutation narrowing the stored wake-source pin to `uint8_t`, and confirm it **fails the timer-wake test**.
   - Copilot says it's caught by a width check: "the wake-source pin can hold `0xFFFF`", at `sensor_wake_source_test.cpp:48`. The behaviour stays safe because of the `sensorCanWake` guard. Say whether that meets the requirement, and whether any narrowing (the stored pin, the `wakePin` local, or both) can produce a wrong wake decision.
6. **The wake read happens once per sleep pass,** with no repeated `activeSensor()` calls. The wake source is read before every use (no stale or uninitialised use on any path, including the STOP fallback).
7. **Copilot's two noted deviations.**
   - The wake label at `:1426` is unguarded, so a no-source sensor with `wakePin == 0xFF` would log "PIR". Is it reachable on real hardware? It's log-only.
   - Mutants were dropped to fit the budget. Do the remaining tests still cover the dropped cases?
8. **The re-pointed tests keep their intent.** `occupancy_report_by_mode_test.sh`, `sleep_config_ownership_structural_test.py` (now order-sensitive) and `sleep_config_leak_test` still fail on a wrong or missing wake source. Prove at least two with your own scratch mutations, including dropping the sensor `.gpio` from the ULP config.
9. **Golden test, suite and budget:**
   - the golden test passes unchanged;
   - the full suite, `N/N (sh via zsh, py via python3)`; the baseline is 78/78, plus the new test;
   - net `src/` ≤ +20, in one commit;
   - tests ≤ +150 added.

## Verdict (your final message; it is saved as the verdict file)

- **Header:** one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED; the model and reasoning used; the test interpreter line.
- **A table of checks 1–9.**
- **Findings,** with file:line and severity, separating observation from inference.
- **Budget versus actual,** as one row.
- **Confirmations:** the tree matches the start, and you removed only your own scratch files.

Limit: 120 lines.
