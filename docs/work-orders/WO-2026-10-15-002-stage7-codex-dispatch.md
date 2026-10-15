AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-15, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-15-002-step6-2b6-wire-reset`).
- Run the host suite and the tests; run local ARM builds in scratch copies.
- Write temporary files under **`build-tmp/wo20261015-002-stage7/`** or `$TMPDIR`, and remove **only** what you create.
- Mutate only in scratch copies.

**Not authorized:**
- Changes outside your scratch locations.
- Commits, pushes, stash, reset or checkout.
- Device, network or AWS access.

# Stage 7 dispatch: WO-2026-10-15-002 (Step 6 2b-6: wire the sensor's reset; central work stays central)

**Goal, in plain language:** switch the hourly and daily count resets over to the sensor's `resetTotals()` hook, while `lastCountTime` and `resetCount` stay central. **No behaviour change:** the same fields are cleared, in the same order, at the same moments.

**Binding spec:** `docs/work-orders/WO-2026-10-15-002-step6-2b6-wire-reset.md`: the design points 1–5, Claude Code's citation re-check at `d89a4ef`, and the architect's Stage 6 rulings (+320 test lines under the +175 guideline; the `pir_sensor_reset_totals_test.sh` edit accepted).

**Inputs:** `docs/work-orders/WO-2026-10-15-002-stage6-copilot-dispatch.md`, and Copilot's report (the end of `build-tmp/WO-2026-10-15-002-stage6-copilot-transcript.md`). Treat every claim as unverified.

**What changed** (uncommitted):
- **`src/`:** +8/−6, net **+2**. No wrapper was added.
  - `MyPersistentData.cpp`, `MyPersistentData.h` and `persist/CurrentReadings.h`: `resetEverything(bool includeSensorTotals = true)` with an early return after `lastCountTime`/`resetCount`.
  - `Generalized-Core-Counter.cpp`: `dailyCleanup()` calls `resetEverything(false)`, then `activeSensor()->resetTotals(Daily)`.
  - `State_Report.cpp`: the hourly COUNTING reset becomes `activeSensor()->resetTotals(Hourly)`.
- **Tests:** +320/−11.
  - **New:** `tests/reset_wiring_test.{sh,cpp}`.
  - **Edited:** `persistence_facade_behavior_test.{cpp,sh}`, `daily_cleanup_boundary_test.py` and `pir_sensor_reset_totals_test.sh`.
- **Claude Code's run:** 81/81, and the golden test passes.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

1. **Setter sets and order equal today's, on both paths and in both modes.**
   - **Daily** (`dailyCleanup()` → `resetEverything(false)` → `resetTotals(Daily)`): exactly `lastCountTime`, `resetCount`, then `hourlyCount`, `dailyCount`, `occupied`, `lastOccupancyEvent`, `occupancyStartTime`, `totalOccupiedSeconds`. Same values, same order as HEAD's `resetEverything()`, connected or not.
   - **Hourly:** COUNTING clears only `hourlyCount`; OCCUPANCY (and MEASUREMENT) clear nothing, as at HEAD.
   - **Show it** both from the code and from the test's recorded sequences.
2. **`initialize()` is still a full reset:** `MyPersistentData.cpp:772` uses the default `true`. Check every other caller of `resetEverything` by bare name and qualified name (§13 (c)). The default appears in the declarations only, not the definition, and the wrapper passes the flag through.
3. **The close order is unchanged:** publish → `dailyCleanup()` → restart in `handleReportingState()`, and nothing else in `dailyCleanup()` moved. `daily_cleanup_boundary_test.py`'s order test is kept and still fails if the order changes.
4. **The null-sensor fallback:** with `_sensor == nullptr` (configured PIR, and an unimplemented type), both periods reach PIR's hook through `activeSensor()`, with no heap use.
5. **`reset_wiring_test` extracts by unique markers** (architect's added check). Every region it extracts from `src/` (`dailyCleanup()`, the hourly block, `resetEverything(bool)` and its wrapper, `activeSensor()`, `PIRSensor`) is found by **unique markers** that **fail loudly** (a non-zero exit with a clear message) if a marker is missing or matches more than once. **Prove it:** in scratch copies, delete each marker and duplicate each marker, and show the failure. Flag any extraction that relies on line numbers or a silent fallback.
6. **The +40 text, explained per function** (architect's added check). Build HEAD (`git archive HEAD`) and the working tree, each with a fresh `BUILD_PATH_BASE`, using the release recipe (not `compile-all`). Give text/data/bss for both; Copilot claims 151364 → 151404. Then explain each changed function with `nm -C -S` and the disassembly where needed:
   - `CurrentReadings::resetEverything` 0xe → 0x14;
   - `currentStatusData::resetEverything` 0x72 → 0x78;
   - `currentStatusData::initialize` 0x28 → 0x2c;
   - `dailyCleanup` 0x44 → 0x54;
   - `handleReportingState` 0x3d0 → 0x3e0;
   - `CSWTCH` renumbering;
   - the remainder against the +40 total (alignment).

   Say why each grew: the new argument, the early return, the virtual call through `activeSensor()`, and so on. Confirm there are no new symbols and no logic change beyond the design.
7. **The test changes hold up.**
   - **`pir_sensor_reset_totals_test.sh`** (ruling 2): its exact-callers check fails if a third caller is added or one of the two is removed.
   - **`persistence_facade_behavior_test`:** the `resetEverything(false)` case fails if the early return moves before `resetCount`.
   - **Copilot's mutations:** re-run its six, and add one of your own: `dailyCleanup()` calling `resetTotals(Hourly)` instead of `Daily`.
8. **Golden, suite and budget:**
   - the golden test passes, with the fixture unchanged;
   - the full suite, `N/N (sh via zsh, py via python3)`; the baseline is 80/80, plus the new test;
   - net `src/` ≤ +20 (hard);
   - tests +320 against the +175 guideline (accepted).

## Verdict (your final message; it is saved as the verdict file)

- **Header:** one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED; the model and reasoning used; the test interpreter line.
- **A table of checks 1–8.**
- **Findings,** with file:line and severity, separating observation from inference.
- **Budget versus actual,** as one row.
- **Confirmations:** the tree matches the start, and you removed only your own scratch files.

Limit: 120 lines.
