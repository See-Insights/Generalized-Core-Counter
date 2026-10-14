AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-14, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- **Edit:** `src/sensors/ISensor.h`, `src/sensors/PIRSensor.h`, `src/sensors/SensorManager.{h,cpp}` (only if needed), and `src/state/State_Sleep.cpp`, at the six `intPin` sites and a local for the wake source.
- **Tests:**
  - add the new tests;
  - re-point the three tests named below;
  - adjust only what this change requires elsewhere, reporting each.
- **Run** the host suite and local ARM release builds.
- **Scratch directory:** **`build-tmp/wo20261014-004-stage6/`**. Remove **only it** when you finish.

**Not authorized:**
- **Moving or editing the PIR-wake handling block** (`State_Sleep.cpp` from `if (pirWake)` onward). Its logic is unchanged; only the `pirWake` expression changes.
- **The hibernate configuration** (`:1135–1138`, `WAKEUP_PIN`), and any change to sleep modes, durations, network flags or the button wake.
- Any other `src/` change, `docs/`, `lib/` or Device OS.
- Commits, pushes, merges, branch changes, stash, reset or checkout. Device or network access beyond the Particle compile.
- Deleting anything you didn't create.

**SIZE BUDGET:** net `src/` **≤ +20** (estimate +17), **one commit**. Over +20: STOP and report. **Tests: stop past +150 added lines.**

# Stage 6 dispatch: WO-2026-10-14-004 (Step 6 2b-4: the wake pin and edge come from the sensor)

**Goal, in plain language:** the sleep code stops hard-wiring the PIR pin. It asks the active sensor which pin and edge, if any, should wake the device, so a sensor with a different pin, or none, needs no sleep-code change. **For PIR, the sleep configuration and wake handling are identical to today.**

**Repository:** this worktree, branch `wo/2026-10-14-004-step6-2b4-wake-source` (from main `9df359d`, with 2b-1 to 2b-3 merged). HEAD is at the commit that adds this dispatch. Don't switch branches.

**Binding spec:** `docs/work-orders/WO-2026-10-14-004-step6-2b4-sensor-wake-source.md`, including its **"Pre-dispatch check"** (citations, Device OS facts, chosen invalid pin) and the **"Architect's addition"**. The design source is `docs/work-orders/WO-2026-10-13-002-step0-round2-report.md`, Q2 "2b-4". Where this dispatch and the WO differ, the WO wins; report the difference.

## Design (do not widen)

- **The interface:**
  - add `ISensor::wakeSource()` returning a small struct, `{pin_t pin; InterruptMode edge;}`, or the closest equivalent; report it;
  - the default is `{PIN_INVALID, RISING}`, meaning "no wake source";
  - `PIRSensor` returns `{intPin, RISING}`.
- **The pin is stored as `pin_t`, never `uint8_t`** (the architect's addition). `wakeupPin()` returns `0xFFFF` on a timer wake, and a `uint8_t` would narrow it to `0xFF` = `PIN_INVALID`.
- **`State_Sleep.cpp`:** each of the six `intPin` uses takes the pin (and the edge, where an edge is used) from `SensorManager::instance().activeSensor()->wakeSource()`. That's 2b-3's fallback, so a closed boot still configures today's PIR wake.
  - **At the two sleep configs** (`:1365` ULP, `:1465` STOP fallback): add the sensor's `.gpio(pin, edge)` **only when the pin is valid**. With PIR, the configuration must be exactly `[BUTTON_PIN FALLING, intPin RISING]`, as today, **including order**.
  - **The two logs** (`:1426`, `:1457`) keep the same text, with the sensor's pin in place of `intPin`.
  - **`:1520` `pinMode(…, INPUT)`** only when the pin is valid.
  - **`:1581`:** `bool pirWake = sensorCanWake && (wakePin == source.pin);`, so a timer wake that reports `0xFFFF` can never match.
- **Read the wake source once per sleep pass.** Take it into a local; don't call `activeSensor()` repeatedly.

## Tests

1. **The 2b-1 golden test passes unchanged.**
2. **Wake parity:**
   - PIR gives `{intPin, RISING}`;
   - a closed boot (`_sensor` null, PIR configured) gives the same;
   - a fake no-source sensor gives `PIN_INVALID`.
3. **The sleep-config extraction:**
   - with PIR, both configs' wake-source calls are exactly `[BUTTON_PIN FALLING, intPin RISING]`;
   - with no source, `[BUTTON_PIN FALLING]`.

   Extend the existing extractor in `tests/sleep_config_ownership_structural_test.py` and `tests/sleep_config_leak_test.{sh,cpp}`, or add a new test; report which.
4. **Structural:** zero `intPin` tokens left in `State_Sleep.cpp`.
5. **Timer wake:**
   - `pirWake` is false for a timer wake (`wakePin == 0xFFFF`) with a PIR source;
   - it's false with a no-source sensor, and with every candidate "no pin" value (`PIN_INVALID`, `0xFFFF`);
   - it's true for a real PIR GPIO wake.
6. **Mutations** (in `$TMPDIR` scratch copies, never in `src/`), each failing a named check:
   - **narrow the stored wake-source pin to `uint8_t`: it must fail the timer-wake test** (the architect's addition);
   - drop the "only when valid" guard on a sleep config;
   - swap the edge to `FALLING` for PIR;
   - make `pirWake` compare against `intPin` again, or drop `sensorCanWake`.
7. **Re-point** the existing tests that pin today's `intPin` text, keeping their intent and reporting each:
   - `tests/occupancy_report_by_mode_test.sh:191`;
   - `tests/sleep_config_ownership_structural_test.py:69`;
   - `tests/sleep_config_leak_test.cpp:117`.
8. **Full suite** before and after: `N/N (sh via zsh, py via python3)`. **The baseline is 78/78.** Run shell tests with `zsh`. No copy of `src/` under the worktree while the suite runs.
9. **Local ARM release build:** fresh `BUILD_PATH_BASE` under scratch; text/data/bss before and after, and the `nm -C -S` differences explained.

## Implementation Report (required, as your final message)

- **Files:** net and gross `src/` lines with the arithmetic, and test lines added and removed.
- **Design choices:** the exact struct, its field types and default; how each of the six sites was changed.
- **Test results:** the golden test; wake parity; the extraction results; the timer-wake results; each mutation and the check that caught it; the suite before and after.
- **Builds:** sizes and the `nm` differences.
- **Confirmations:** you removed only your scratch directory.
- **Deviations** (or "none"), and **the model used**.
