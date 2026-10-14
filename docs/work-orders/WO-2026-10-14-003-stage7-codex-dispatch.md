AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-14, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-14-003-step6-2b3-sensor-fields`).
- Run the host suite and the tests; run local ARM builds in scratch copies.
- Write temporary files under **`build-tmp/wo20261014-003-stage7/`** or `$TMPDIR`, and remove **only** what you create.
- Mutate only in scratch copies.

**Not authorized:**
- Changes outside your scratch locations, including `build-tmp/WO-2026-10-14-003-commit-a.patch`.
- Commits, pushes, stash, reset or checkout.
- Device, network or AWS access.

# Stage 7 dispatch: WO-2026-10-14-003 (Step 6 2b-3: the sensor supplies its own report fields)

**Goal, in plain language:** `publishData()` stops knowing which sensor fields exist.
- **The sensor's part:** the active sensor writes it. For PIR that's `occupancy`/`dailyoccupancy`, or `hourly`/`daily`.
- **The common part stays central.**
- **All 175 original golden vectors stay byte-identical.**

**Binding spec:** `docs/work-orders/WO-2026-10-14-003-step6-2b3-sensor-fields.md`, including the architect's Stage 6 rulings 1–4. The design source is `docs/work-orders/WO-2026-10-13-002-step0-round2-report.md` Q2 "2b-3", and the Stage 4 risks are in `WO-2026-10-13-002-stage4-verdict.md`.

**Inputs:**
- `docs/work-orders/WO-2026-10-14-003-stage6-copilot-dispatch.md`;
- Copilot's report: the end of `build-tmp/WO-2026-10-14-003-stage6-copilot-transcript.md`, from "## Implementation Report (model".

Treat every claim as unverified.

**Correction to the Stage 6 dispatch (architect's ruling 4):** the dispatch said `createSensor()` uses `new`. **It doesn't.** `createSensor(SensorType::PIR)` returns `&PIRSensor::instance()`, a function-local static (`src/sensors/SensorFactory.h:74`). Judge the fallback's heap behaviour on that basis.

**What changed** (uncommitted, against HEAD):
- **`src/`:** +51/−41, net **+10**.
  - (a) is the field writer, `ISensor.h` +3, `PIRSensor.h` +3, `PIRSensor.cpp` +13; it's in `build-tmp/WO-2026-10-14-003-commit-a.patch`, along with its test change.
  - (b) is `SensorManager.{h,cpp}` +9 and `Generalized-Core-Counter.cpp` +23/−41.
- **Tests:** +178 added / −33 removed, against a budget **raised to +180 added**, including the new `tests/stubs/report_golden/Particle.h`.
- **Golden fixture:** 11 vectors appended. **Claude Code checked** that the 177 original fixture lines are byte-identical.
- **Claude Code's run:** 78/78, and the golden test passes.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

1. **Golden unchanged at both commits.**
   - Apply `commit-a.patch` to a clean HEAD copy: the golden test passes, with the fixture's original rows untouched.
   - On the full tree it passes with all 186 vectors.
   - Re-record the original 175 rows from a clean HEAD build and diff them byte for byte against the committed fixture.
2. **Fallback vectors.**
   - With `_sensor == nullptr`, a valid type and an invalid type (falling back to PIR) each give today's bytes in both layouts.
   - **`createSensor()` is not called per report while `_sensor` exists:** show it from the code and from the call counters.
   - `activeSensor()` never returns null.
3. **Zero heap in the report path:** the allocation counter is zero on every vector, including the fallback ones.
4. **No sensor fields left in `publishData()`'s payload construction.**
   - **Search** the payload part of `publishData()` (from `batteryContext` to the queue call) for `occupancy`, `hourly`, `daily` and the PIR getters. Only mode logic should remain: the timestamp rule and the `alerts`/`resets` key order by mode.
   - **The serial `Report:` log** (`Generalized-Core-Counter.cpp`, after the ledger write) still reads the PIR getters. That is deliberately out of scope (soak grading parses it), so grade it as an **observation**, not a finding.
5. **Evaluation order and truncation.**
   - Every common getter is called exactly once, in the same relative order as before. Compare the argument order of the old two `snprintf`s with the new single one.
   - The PIR getters are now read **before** the main `snprintf` (in `reportFields`). Say whether that can change any output, for example a value changing between reads. Inference is allowed here, marked as such.
   - The 255/256-byte truncation vectors pass, and the combined output truncates at the same byte as before.
6. **PIR writer.**
   - Integer-only, at most 40 bytes, leading comma placement correct.
   - It produces exactly today's sensor keys and formats for OCCUPANCY, COUNTING and MEASUREMENT. The MEASUREMENT layout is counting's.
   - The default `ISensor::reportFields()` writes an empty string.
7. **ELF.** Build HEAD and the working tree (fresh `BUILD_PATH_BASE` each; `git archive HEAD` for HEAD). Give text/data/bss for both; Copilot claims 151308 → 151220. Explain every `nm -C -S` change:
   - `publishData` shrinking;
   - `reportFields` and `activeSensor` added;
   - `PIRSensor::instance()` and `SensorFactory::createSensor` now out of line;
   - `initializeFromConfig` shrinking;
   - the `PIRSensor` vtable gaining a slot and now emitted as `T`.
8. **The test changes hold up.**
   - **`watchdog_alert_code_test.sh` (ruling 2):** in a scratch copy, rename the `alerts` key and confirm the test fails.
   - **`report_payload_fields_test.py`:** it now reads the common format plus `PIRSensor.cpp`'s formats. Confirm it still enforces widths and quoting, and fails if a sensor field reappears in `publishData()`.
   - **The golden harness:** it compiles the real `PIRSensor.cpp` and takes `activeSensor()` verbatim. Confirm both, with a marker-mismatch failure for the verbatim part.
   - **Copilot's 16 built-in mutations:** re-run at least five, including PIR fields swapped and `activeSensor()` returning null on a closed boot. Add one mutation of your own.
9. **Suite and budget.**
   - Full suite: `N/N (sh via zsh, py via python3)`. The baseline is 78/78.
   - Net `src/` ≤ +20.
   - Test lines ≤ +180 added.
   - The patch split: (a) is exactly the field writer plus the `pir_sensor_lifecycle_test.cpp` link change, and (b) is the remainder.

## Verdict (your final message; it is saved as the verdict file)

- **Header:** one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED; the model and reasoning used; the test interpreter line.
- **A table of checks 1–9.**
- **Findings,** with file:line and severity, separating observation from inference.
- **Budget versus actual,** as one row.
- **Confirmations:** the tree matches the start, and you removed only your own scratch files.

Limit: 120 lines.
