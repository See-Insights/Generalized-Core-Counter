# WO-2026-10-14-004: Step 6 2b-4 — the wake pin and edge come from the sensor

**Author:** architect, 2026-10-14 · **Status:** ready (citation re-check and two device facts first)
**Branch:** `wo/2026-10-14-004-step6-2b4-wake-source`, from main after the 2b-3 PR merges
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.1–12.4; §13 (b), (c), (g), (j)
**Spec source:** `WO-2026-10-13-002-step0-round2-report.md`, Q2 "2b-4" (citations at `25dbc69`; re-check at the new base).

## Plain goal

The sleep code stops hard-wiring the PIR pin: it asks the active sensor which pin and edge (if any) should wake the device, so a sensor with a different pin — or none — needs no sleep-code change. **For PIR, the sleep configuration and wake handling are identical to today.**

## Design (round-2 report; do not widen)

- `ISensor::wakeSource()` returning `{pin, edge}`; default `{<invalid pin>, RISING}` meaning "no wake source". `PIRSensor` returns `{intPin, RISING}`.
- `State_Sleep.cpp`: the six live `intPin` uses take the pin and edge from `SensorManager::activeSensor()->wakeSource()` (2b-3's fallback, so a closed boot still configures today's PIR wake). Add the PIR pin to the sleep configuration only when the source is valid.
- `pirWake = sensorCanWake && wakePin == source.pin`, so a timer wake that reports an invalid pin can never match a sensor that has no pin.
- The PIR-wake handling block stays where it is (`State_Sleep.cpp`, pinned by `clock_resync_wiring_test.py`).

## Check before dispatch (Claude Code, read-only, one paragraph)

1. Citations at the new base.
2. **Device facts, from the Device OS 6.4.1 source in the toolchain:** the constant that means "no pin" (round-2 found no `PIN_INVALID` in the repo), and what `SystemSleepResult::wakeupPin()` returns on a timer or RTC wake. Choose the invalid-pin value so it can never equal a real pin or the timer-wake value. **Stop and report** if either can't be established from source.

## Pre-dispatch check (Claude Code, 2026-10-14, at main `9df359d`)

**Citations hold:** the six `intPin` uses in `State_Sleep.cpp` are at:
- `:1365`, the ultra-low-power config `.gpio(intPin, RISING)`;
- `:1426`, the wake-label log;
- `:1457`, the ultra-low-power failure log `pir=%d`;
- `:1465`, the STOP fallback config;
- `:1520`, the post-wake `pinMode(intPin, INPUT)`;
- `:1581`, `bool pirWake = (wakePin == intPin);`.

`SensorManager::activeSensor()` exists (2b-3), and the PIR-wake block is still in `State_Sleep.cpp`. The hibernate config (`:1135–1138`) uses `WAKEUP_PIN`, not the PIR pin, so it's out of scope.

**Device facts** (from the Device OS 6.4.1 source in the toolchain):
1. **"No pin":** `PIN_INVALID` is `0xff` (`hal/inc/pinmap_hal.h:75`).
2. **Timer or RTC wake:** `SystemSleepResult::wakeupPin()` returns `std::numeric_limits<hal_pin_t>::max()` for any wake that isn't by GPIO (`wiring/inc/spark_wiring_system.h:275–281`). `hal_pin_t` is `uint16_t` (`pinmap_hal.h:31`), so that's `0xFFFF`.

The app's `wakePin` is `pin_t` (`State_Sleep.cpp:1424`), and real Boron pins are all below `TOTAL_PINS`, which is at most 39 (`hal/src/boron/pinmap_defines.h`). **Chosen invalid-pin value: `PIN_INVALID` (`0xff`).** It can never equal a real pin or the timer-wake value `0xFFFF`.

**Existing tests that pin today's `intPin` text in `State_Sleep.cpp`, to re-point:**
- `tests/occupancy_report_by_mode_test.sh:191` (`bool pirWake = (wakePin == intPin);`);
- `tests/sleep_config_ownership_structural_test.py:69` (`[("BUTTON_PIN","FALLING"), ("intPin","RISING")]`);
- `tests/sleep_config_leak_test.cpp:117` (it defines `intPin` for its harness).

## Architect's addition (2026-10-14)

- **The wake-source pin is stored as `pin_t`, never `uint8_t`.** A `uint8_t` would narrow the timer-wake value `0xFFFF` to `0xFF`, which equals `PIN_INVALID`, so a timer wake would match a no-source sensor.
- **Required mutation:** narrow the stored pin to `uint8_t`. It **must fail the timer-wake test**.

## Budget

Net `src/` **≤ +20** (estimate +17). One commit. Tests: stop past +150 added lines.

## Tests

- The 2b-1 golden test unchanged.
- Wake parity: PIR `{intPin, RISING}`, a closed boot (null sensor) gives the same, and a fake no-source sensor gives the invalid pin.
- Extract both sleep-configuration statements and compare the wake-source calls: with PIR, `[BUTTON, intPin]` exactly as today; with no source, `[BUTTON]`.
- A structural check: zero `intPin` tokens left in `State_Sleep.cpp`.
- `pirWake` is false for a timer wake with every candidate "no pin" value.

## Agents and models

- **Stage 6:** Copilot, Sonnet tier, **high**. *Why:* the sleep path; a mistake here costs wakes in the field.
- **Stage 7:** Codex `gpt-5.6-sol`, **high**. Extra checks: every former `intPin` use maps to the sensor's pin with the same edge; the ELF sleep-configuration code for PIR is equivalent; no path where a closed boot loses the PIR wake.

## Bench

Rides the v43 soak, with one added check there: PIR wakes from ULP sleep and from hibernate-adjacent sleeps still wake and report, and timer wakes never take the PIR path.

## Closing record (2026-10-15)

**Result:** **VERIFIED WITH CONCERNS** at Stage 7 (`WO-2026-10-14-004-stage7-verdict.md`), with no FAIL. **Stage 8 approved** by the architect, with all three low findings accepted. **Release:** rides v43.

| Stage | Agent / model | Result |
|---|---|---|
| Pre-dispatch check | Claude Code, at main `9df359d` | citations held (the six `intPin` sites); Device OS facts established; three pinned tests identified |
| Stage 6 | Copilot, `claude-sonnet-5.5`, high | net +13 `src/`; new `sensor_wake_source_test`; three tests re-pointed; 78/78 → 79/79 |
| Stage 7 | Codex, `gpt-5.6-sol`, high | VERIFIED WITH CONCERNS; PIR sleep config equivalent in the disassembly; three low findings |

Every model was probed first (§5). One round; no controller edits.

| Item | Budget | Actual |
|---|---|---|
| `src/` (one commit) | ≤ +20 (estimate +17) | **+13** (+22/−9): `ISensor.h` +9, `PIRSensor.h` +2, `State_Sleep.cpp` +11/−9 |
| Tests | stop past +150 added | **+150 / −13** (at the limit) |

**Invalid-pin value: `PIN_INVALID` (`0xff`).** Sources, from Device OS 6.4.1 in the toolchain:
- **The definition:** `PIN_INVALID` is `0xff` (`hal/inc/pinmap_hal.h:75`).
- **The timer-wake value:** `SystemSleepResult::wakeupPin()` returns `std::numeric_limits<hal_pin_t>::max()` for any non-GPIO wake (`wiring/inc/spark_wiring_system.h:275–281`). `hal_pin_t` is `uint16_t` (`pinmap_hal.h:31`), so a timer or RTC wake reports `0xFFFF`.
- **The app's side:** `wakePin` is `pin_t` (`State_Sleep.cpp`), and real Boron pins are all below `TOTAL_PINS`, which is at most 39 (`hal/src/boron/pinmap_defines.h`).
- **So** `0xff` can never equal a real pin or the timer-wake value. The wake-source pin is stored as `pin_t`, never `uint8_t` (the architect's addition).

**ELF changes** (Stage 7, fresh builds): text 151220 → **151308** (+88); data and bss unchanged.

| Symbol | Change | Reason |
|---|---|---|
| `handleSleepingState()` | 0x1c80 → 0x1cac (+0x2c) | reads the wake source once and guards the sensor GPIO on the ULP and STOP configs, the post-wake `pinMode` and `pirWake` |
| `PIRSensor::wakeSource()` | new, 0x24 | returns `{intPin, RISING}` |
| `vtable for PIRSensor` | 0x40 → 0x44 | one slot for `wakeSource()` |
| `CSWTCH.*` | renumbered, same sizes | compiler-local |
| (4 bytes) | — | linker alignment |

The disassembly shows PIR's ULP and STOP configs calling `gpio(BUTTON_PIN, FALLING)`, then `gpio(intPin, RISING)`, exactly as before. `.duration()` still follows on STOP.

**Low findings, all three accepted (architect, 2026-10-15):**
1. **The narrowing mutation:** narrowing the stored pin to `uint8_t` fails the timer-wake test on its `0xFFFF` width check (`sensor_wake_source_test.cpp:48`), not on a wrong wake decision. While the `sensorCanWake` guard is there, no narrowing produces a wrong wake branch. The `pin_t` rule is enforced.
2. **The wake-label log:** `State_Sleep.cpp` labels the wake by equality without `sensorCanWake`, so a narrowed no-source timer value could be logged as "PIR". It's log-only, and unreachable on real hardware: a timer wake gives `0xFFFF`, and GPIOs are below 39.
3. **`sleep_config_leak_test`:** it derives its expected wake-source count from the source, so it doesn't catch a wrong or missing sensor GPIO. Wake parity is enforced by `sleep_config_ownership_structural_test.py`, which is now order-sensitive and fails on both mutations.

**Tests:**
- **New:** `tests/sensor_wake_source_test.{sh,cpp}`, extracting the real `State_Sleep.cpp` and `SensorManager.cpp` lines. It checks PIR parity, a closed boot, a no-source sensor, another pin, and timer and GPIO wakes, with mutants including the `uint8_t` narrowing.
- **Re-pointed:** `occupancy_report_by_mode_test.sh`, `sleep_config_ownership_structural_test.py` and `sleep_config_leak_test.{sh,cpp}`.
- **Stubs:** `PIN_INVALID`/`FALLING` added to two stubs.
- **Golden test:** unchanged.

**Bench:** rides the v43 soak, with the WO's added check: PIR wakes from ULP sleep and from hibernate-adjacent sleeps still wake and report, and timer wakes never take the PIR path.
