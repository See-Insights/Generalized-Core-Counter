AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-15, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-15-001-step6-2b5a-reset-hook`).
- Run the host suite and the tests; run local ARM builds in scratch copies.
- Write temporary files under **`build-tmp/wo20261015-001-stage7/`** or `$TMPDIR`, and remove **only** what you create.
- Mutate only in scratch copies.

**Not authorized:**
- Changes outside your scratch locations.
- Commits, pushes, stash, reset or checkout.
- Device, network or AWS access.

# Stage 7 dispatch: WO-2026-10-15-001 (Step 6 2b-5a: the sensor's own reset of its totals, hook only)

**Goal, in plain language:** give the sensor one operation that clears its own hourly or daily totals, exactly as the central code clears them today. It is **added but not yet called**, so there's no behaviour change.

**Binding spec:** `docs/work-orders/WO-2026-10-15-001-step6-2b5a-sensor-reset-hook.md`, including the architect's Stage 6 rulings 1–3 (test budget +165; link stubs accepted; rebuild both sides fresh, expecting +56 text).

**Inputs:** `docs/work-orders/WO-2026-10-15-001-stage6-copilot-dispatch.md`, and Copilot's report (the end of `build-tmp/WO-2026-10-15-001-stage6-copilot-transcript.md`, from "## Implementation Report"). Treat every claim as unverified.

**What changed** (uncommitted, one commit):
- **`src/`:** +20/−0. `ISensor.h` +6 (`enum class ResetPeriod`, the default `virtual void resetTotals(ResetPeriod) {}`), `PIRSensor.h` +3, `PIRSensor.cpp` +11.
- **Tests:** +161/−1.
  - **New:** `tests/pir_sensor_reset_totals_test.{sh,cpp}`.
  - **Link stubs:** no-op `CurrentReadings` setters added to `pir_sensor_lifecycle_test.cpp`, `report_payload_golden_test.cpp` and `sensor_wake_source_test.cpp`.
- **Claude Code's run:** 80/80, golden test passing, golden fixture unchanged.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

1. **Setter list and order equal the central reset's.**
   - **Daily** calls exactly the six sensor-total setters of `currentStatusData::resetEverything()` (`src/MyPersistentData.cpp:689–707`, the setters at `:699–706`), in the same order, with the same values.
   - **Hourly** calls only `set_hourlyCount(0)`, matching `State_Report.cpp:101`.
   - **Neither touches** `lastCountTime` or `resetCount`.
   - **Confirm the test** extracts the setters from the **real** `resetEverything()`, and fails if they diverge.
2. **`_motionDetected` untouched.** `resetTotals` doesn't clear it, or any other `PIRSensor` member. `PIRSensor::reset()` is unchanged. The ISR-pending check (fire the ISR, reset, then `loop()` reports motion) is real, and fails if `resetTotals` clears the flag.
3. **No caller.** By bare-name and qualified search (§13 (c)), `resetTotals` appears in `src/` only in its declaration, the default, and the PIR override and definition. The structural check fails if a caller is added.
4. **The ELF adds only the new function and a vtable slot.** Build HEAD (`git archive HEAD`) and the working tree, each with a fresh `BUILD_PATH_BASE`, **using the same release recipe as earlier Stage 7s** (not `compile-all`).
   - Give text/data/bss for both. The architect expects **+56 text**.
   - Explain every `nm -C -S` change. Expected: `PIRSensor::resetTotals(ResetPeriod)` (about 0x34), `vtable for PIRSensor` 0x44 → 0x48, and `CSWTCH` renumbering.
5. **The link stubs (ruling 2).** They're no-ops used only to link. The golden fixture and assertions are unchanged, and the golden test passes.
6. **Mutations.** Re-run Copilot's four (two Daily setters swapped, a Daily setter dropped, `resetTotals` clearing `_motionDetected`, a caller added in `State_Report.cpp`) and confirm each is caught. Add one of your own: Hourly also clearing `dailyCount`.
7. **Suite and budget:**
   - full suite, `N/N (sh via zsh, py via python3)`; the baseline is 79/79, plus the new test;
   - net `src/` ≤ +20, in one commit;
   - tests ≤ +165 added.

## Verdict (your final message; it is saved as the verdict file)

- **Header:** one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED; the model and reasoning used; the test interpreter line.
- **A table of checks 1–7.**
- **Findings,** with file:line and severity, separating observation from inference.
- **Budget versus actual,** as one row.
- **Confirmations:** the tree matches the start, and you removed only your own scratch files.

Limit: 100 lines.
