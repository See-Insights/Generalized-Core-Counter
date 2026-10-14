AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-14, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- **Edit:** `src/sensors/ISensor.h`, `src/sensors/PIRSensor.h`, `src/sensors/PIRSensor.cpp`, `src/sensors/SensorManager.h`, `src/sensors/SensorManager.cpp`, and `src/Generalized-Core-Counter.cpp`. In that last file, only `publishData()`'s payload construction (`:2011–2050`) and any include it needs.
- **Tests:**
  - add fallback vectors to the 2b-1 golden test harness, **without changing any existing fixture row**;
  - re-point `tests/report_payload_fields_test.py`;
  - adjust only what this change requires in other tests, reporting each.
- **Write one patch file:** `build-tmp/WO-2026-10-14-003-commit-a.patch`.
- **Run** the host suite and local ARM release builds.
- **Scratch directory:** **`build-tmp/wo20261014-003-stage6/`**. Remove **only it** when you finish; keep the patch.

**Not authorized:**
- **Any change to field names, order, formats or values;** a golden fixture row changing is **stop-and-report**.
- **The serial `Report: occ=… / Report: hourly=…` log line** at `Generalized-Core-Counter.cpp:2124–2135`. It's outside the payload, and soak grading parses it. **Leave it unchanged** (see "Scope note").
- The device-data ledger, the report request (2b-7), settings (2b-8), and MEASUREMENT behaviour.
- Any other `src/` change, `docs/`, `lib/` or Device OS.
- Commits, pushes, merges, branch changes, stash, reset or checkout. Device or network access beyond the Particle compile.
- Deleting anything you didn't create.

**SIZE BUDGET:** net `src/` **≤ +20** (estimate +18). **Over +20, STOP and report** with a proposed split. Test lines: **stop past +150**.

# Stage 6 dispatch: WO-2026-10-14-003 (Step 6 2b-3: the sensor supplies its own report fields)

**Goal, in plain language:** `publishData()` stops knowing which sensor fields exist.
- **The sensor's part:** the active sensor writes it. For PIR that's `occupancy`/`dailyoccupancy` (OCCUPANCY) or `hourly`/`daily` (COUNTING and MEASUREMENT).
- **The common part stays central:** battery, vc, key1, temp, alerts, resets, connecttime, fh, lfb, cyc, slp, timestamp.
- **All 175 golden vectors stay byte-identical.**

**Repository:** this worktree, branch `wo/2026-10-14-003-step6-2b3-sensor-fields` (from main `e149123`, with 2a, 2b-1 and 2b-2 merged). HEAD is at the commit that adds this dispatch. Don't switch branches.

**Binding spec:** `docs/work-orders/WO-2026-10-14-003-step6-2b3-sensor-fields.md`, whose "Design" section says **do not widen**. The detailed design is in `docs/work-orders/WO-2026-10-13-002-step0-round2-report.md`, Q2 "2b-3", with the Stage 4 findings in `WO-2026-10-13-002-stage4-verdict.md` (null sensor, getter order and truncation). Where this dispatch and the WO differ, the WO wins; report the difference.

## Citations, re-checked by Claude Code at `e149123` (structure as round 2; lines shifted by about −1)

- **`publishData()`** (`src/Generalized-Core-Counter.cpp`) is at `:1964`:
  - `batteryContext` is at `:1966`, and `char data[256]` at `:1972`;
  - the mode branch `if (sensorMode == SystemConfig::OCCUPANCY)` is at `:2011`;
  - **occupancy `snprintf` at `:2016`:** `occupancy`, `dailyoccupancy`, then common fields; **`alerts` before `resets`** (`:2024–2025`);
  - **counting `snprintf` at `:2035`** (the `else`, so it also serves MEASUREMENT): `hourly`, `daily`, then common fields; **`resets` before `alerts`** (`:2043–2044`);
  - the timestamp rule: occupancy uses `stampOverride` or now; counting uses `endOfPrevHourStampSec`;
  - the queue call is at `:2057`.
- **Serial log:** `:2124–2135` is the `Report:` log (see "Scope note").
- **`SensorManager`:**
  - `SensorManager.cpp:295` `initializeFromConfig()` calls `SensorFactory::createSensor(sensorType)` (`:297`) and `setSensor` (`:305`);
  - `_sensor` is a raw `ISensor*`, not owned;
  - `SensorFactory.h:71` `createSensor()`: `case SensorType::PIR` builds the PIR sensor; unknown types log `Sensor type %d not yet implemented` and return `nullptr` (`:86–87`).
- **Closed boot:** `initializeFromConfig()` is skipped when the device boots closed (`Generalized-Core-Counter.cpp:1524`, `:1533`), so `_sensor` is null for the closing report.
- **`ISensor.h`:** no field writer yet. `PIRSensor.cpp` is 10 lines.
- **Tests:** `report_payload_fields_test.py:86–87` and `:175` extract from `void publishData(` to `const char *webhookName`, and read the two format strings. `daily_cleanup_boundary_test.py:234` checks `stampOverride != 0 ? (unsigned long)stampOverride : nowStampSec` inside `publishData`; the timestamp stays central, so expect it unchanged.

## Design (from the WO; do not widen)

- **The field writer on `ISensor`:**
  - `ISensor::reportFields(char* out, size_t len, uint8_t sensorMode)` (or the closest exact signature; report it);
  - the default writes nothing (an empty string);
  - `PIRSensor` implements it with **integer-only output, at most 40 bytes**, producing exactly today's leading fields, comma included, for the mode it is given.
- **`SensorManager::activeSensor()`:**
  - returns `_sensor` if it was built;
  - else the factory result for `SystemConfig::SensorSettings::get_sensorType()`;
  - else PIR.
  - **`createSensor()` must not be called per report while `_sensor` exists.**
  - **Fallback ownership:** don't add heap use in the report path (the golden test counts allocations and requires zero). If the fallback needs an object, use a function-local static, or another no-allocation approach, and justify it. Note that `createSensor()` today uses `new`, so the fallback may not call it per report. Report how you resolved this; if it can't be done within the budget, STOP and report.
- **`publishData()`:**
  - keeps **one** `snprintf` per branch, or a single `snprintf`, into the same 256-byte buffer;
  - the sensor's prefix goes in as a `%s` argument rendered **before** the `snprintf`;
  - the `alerts`/`resets` key names go in as arguments, so each mode keeps its current order;
  - **the evaluation order of every common getter stays as today** (the golden test's call counters must still show one call each);
  - mind truncation: the combined output must truncate at exactly the same byte as today for the 255/256-byte sweep vectors.

## Scope note: the serial `Report:` log (`:2124–2135`)

- **What it is:** the log reads the PIR getters and prints `occ=` / `hourly=` / `daily=`. It is a log line, not payload.
- **Why it stays:** this WO's goal and the golden test concern the payload, and the soak grading parses this line. **Leave it unchanged.**
- **How it's graded:** Claude Code has told the architect it remains. Stage 7 will grade it as an observation.

## Two commits (you don't commit)

1. **(a):** the `ISensor` and `PIRSensor` field writer, not yet used. The golden test must pass. Then run `git diff > build-tmp/WO-2026-10-14-003-commit-a.patch`.
2. **(b):** `SensorManager::activeSensor()` and the `publishData()` switch-over. The golden test must pass **unchanged**.

## Tests

- **The 2b-1 golden test passes unchanged after (a) and after (b).**
  - If an extraction marker must move, re-point **only the marker**, and report why.
  - The test must also compile the new sensor code it now depends on. Re-point its build line, not its fixture.
  - **Any fixture row change: STOP.**
- **New fallback vectors** with `_sensor == nullptr`, in both layouts:
  - a valid configured type, giving today's bytes;
  - an invalid configured type, which falls back to PIR and gives today's bytes.

  Add call-count asserts showing each getter is read exactly as before. Add a check that `createSensor()` is **not** called when `_sensor` exists.
- **Re-point `report_payload_fields_test.py`** to the new format-string location. Keep its width and quoting rules.
- **Mutations** (in `$TMPDIR` scratch copies, never in `src/`):
  - make PIR's writer swap its two fields: the golden test must fail;
  - make `activeSensor()` return null on a closed boot: the fallback vector must fail.
- **Full suite** before and after: `N/N (sh via zsh, py via python3)`. **The baseline is 78/78.** Run shell tests with `zsh`. No copy of `src/` under the worktree while the suite runs.
- **Local ARM release build:** fresh `BUILD_PATH_BASE` under scratch; text/data/bss before and after, and the `nm -C -S` differences explained.

## Implementation Report (required, as your final message)

- **Per commit:** files, with net and gross lines, and the arithmetic.
- **Design choices:** the exact signature; how the fallback avoids the heap; how the evaluation order and truncation are preserved.
- **Test results:** the golden test after (a) and (b); the fallback vectors; the mutation results; the suite before and after.
- **Builds:** sizes and the `nm` differences.
- **Confirmations:** no fixture row changed; the patch exists; you removed only your scratch directory.
- **Deviations** (or "none"), and **the model used**.
