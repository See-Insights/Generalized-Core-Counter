AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-14, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-14-002-step6-2b2-lifecycle`).
- Run the host suite and the new tests; run local ARM builds in scratch copies.
- Write temporary files under **`build-tmp/wo20261014-002-stage7/`** or `$TMPDIR`, and remove **only** what you create.
- Mutate only in scratch copies.

**Not authorized:**
- Changes outside your scratch locations, including the patch files in `build-tmp/`.
- Commits, pushes, stash, reset or checkout.
- Device, network or AWS access.

# Stage 7 dispatch: WO-2026-10-14-002 (Step 6 2b-2: merge `setup()` into `initializeHardware()`, remove `sensorDetect`)

**Goal, in plain language:** the sensor has one start operation instead of two overlapping ones, and the dead `sensorDetect` flag is gone. Device behaviour doesn't change, and the golden payload test passes unchanged.

**Binding spec:** `docs/work-orders/WO-2026-10-14-002-step6-2b2-sensor-lifecycle-cleanup.md`.

**Inputs:** `docs/work-orders/WO-2026-10-14-002-stage6-copilot-dispatch.md`, and Copilot's report (the end of `build-tmp/WO-2026-10-14-002-stage6-copilot-output.log`). Treat every claim as unverified.

**What changed** (uncommitted, against HEAD):
- **`src/`:** 6 files, +9/−19, net −10. That's (a) −8 in `ISensor.h`/`PIRSensor.h`, and (b) −2 in `Generalized-Core-Counter.cpp`, `StateMachine.h`, `State_Sleep.cpp`, `State_Idle.cpp`.
- **New test:** `tests/pir_sensor_lifecycle_test.{sh,cpp}` and `tests/stubs/pir_lifecycle/Particle.h` (147 lines).
- **Patches:** `build-tmp/WO-2026-10-14-002-commit-1-test.patch` holds the test alone; `build-tmp/WO-2026-10-14-002-commit-12-a.patch` holds the test plus (a).
- **Claude Code has already checked:**
  - the test patch alone on a clean HEAD copy: the lifecycle test passes;
  - the test-plus-(a) patch on a clean copy: both the lifecycle and golden tests pass;
  - the full tree: 78/78, and the golden test passes.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

1. **No caller of `ISensor::setup()` left.** Search by bare name and qualified name (§13 (c)) across `src/`, `lib/` and `tests/`. No remaining declaration or override of a sensor `setup()`, and no doc text that still names it as the sensor start operation.
2. **Body unchanged.** `PIRSensor::initializeHardware()`'s body is byte-identical to the old `PIRSensor::setup()` body. Every caller of `initializeHardware()` (`SensorManager.cpp`) is unchanged, and still calls it through the interface.
3. **`sensorDetect` gone with no behaviour change.**
   - No remaining reference in `src/` or `tests/`.
   - It was never written, so both simplified conditions are equivalent to the old ones.
   - The `State_Sleep.cpp` log line drops only `sensor=%d` and its argument; the format and arguments still match.
   - `State_Idle.cpp:255` keeps redundant parentheses (Copilot's note). Judge whether that matters.
4. **ELF.** Build HEAD (`git archive HEAD` into scratch) and the working tree, each with a fresh `BUILD_PATH_BASE`. Give text/data/bss for both (Copilot claims 151372 → 151308). Compare `nm -C -S`:
   - **Expected:** `PIRSensor`'s vtable loses one slot; `ISensor::initializeHardware()` (the default forwarder) disappears; `PIRSensor::setup()` becomes `PIRSensor::initializeHardware()` with the same size; `sensorDetect` disappears.
   - **Anything else:** explain it.
   - **Confirm no other logic change.**
5. **ISR attach order identical.** `attachInterrupt(intPin, pirISR, RISING)` comes after the pin and output setup in the start operation, as before, and `onWake()` and `onSleep()` are unchanged.
6. **The lifecycle test bites.** In a scratch copy, change the call order in `initializeHardware()` (at least two different reorderings, plus one removal of `attachInterrupt`). Each must fail the test.
   - Also check the test compiles the **real** `src/sensors/PIRSensor.h`, not a copy.
   - Check it passes on the unchanged HEAD: apply `commit-1-test.patch` alone in scratch.
7. **Golden test unchanged.** `tests/report_payload_golden_test.sh` passes at HEAD + test, at HEAD + test + (a), and on the full tree. Its fixture is untouched.
8. **Suite:** full host suite, `N/N (sh via zsh, py via python3)`. The baseline was 77/77, and 78/78 is expected with the new test.
9. **Budget:**
   - net `src/` ≤ 0 (about −8 expected), with gross additions only from the rename or simplification;
   - test lines ≤ 150;
   - the patch files split as described: (a) is the delta from commit-1 to commit-12-a, and (b) is the remainder of the working tree.

## Verdict (your final message; it is saved as the verdict file)

- **Header:** one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED; the model and reasoning used; the test interpreter line.
- **A table of checks 1–9.**
- **Findings,** with file:line and severity, separating observation from inference.
- **Budget versus actual,** as one row.
- **Confirmations:** the tree matches the start, and you removed only your own scratch files.

Limit: 120 lines.
