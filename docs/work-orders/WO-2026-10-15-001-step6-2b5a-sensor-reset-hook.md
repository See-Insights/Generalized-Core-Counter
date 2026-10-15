# WO-2026-10-15-001: Step 6 2b-5a — the sensor's own reset of its totals (hook only, not wired)

**Author:** architect, 2026-10-15 · **Status:** ready (citation re-check first)
**Branch:** `wo/2026-10-15-001-step6-2b5a-reset-hook`, from main after the 2b-4 PR merges
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.2–12.4; §13 (b), (c), (g), (j)
**Spec source:** `WO-2026-10-13-002-step0-round2-report.md`, Q2 "2b-5a" (citations at `25dbc69`; re-check at the new base).

## Plain goal

Give the sensor one operation that clears its own hourly or daily totals, exactly as the central code clears them today — **added but not yet called**, so this WO changes no device behavior. 2b-6 then switches the central reset over to it.

## Design (round-2 report; do not widen)

- `ISensor::resetTotals(ResetPeriod)` with `ResetPeriod { Hourly, Daily }`; default does nothing.
- `PIRSensor` implements it with the same setters, in the same order, as today's central reset (`MyPersistentData.cpp` `resetEverything()`, round-2 cites `:699–706`):
  - Hourly: hourly count to 0.
  - Daily: hourly, daily, occupied false, lastOccupancyEvent 0, occupancyStartTime 0, totalOccupiedSeconds 0.
- **Must not reuse `ISensor::reset()`**: PIR's `reset()` clears `_motionDetected`, so an interrupt that fired just before the reset would be lost. The new hook leaves `_motionDetected` alone.
- Not touched: `lastCountTime` and `resetCount` (they stay central in 2b-6), any caller.

## Budget

Net `src/` **≤ +20** (estimate +16). One commit. Tests ≤ +100 added lines.

## Tests

- Golden test unchanged.
- Recorded setter order for Hourly and Daily matches the central reset's order exactly.
- A PIR interrupt fired before either reset still makes the next `loop()` report motion.
- Structural: no caller of `resetTotals` exists yet in `src/` (this WO is hook-only).

## Agents and models

- **Stage 6:** Copilot, Sonnet tier, **medium**. Small, uncalled addition.
- **Stage 7:** Codex `gpt-5.6-sol`, **medium**. Checks: setter list and order equal the central reset's; `_motionDetected` untouched; no caller; ELF adds only the new function and vtable slot.

## Bench

None; no behavior change.

## Stage 6: architect's rulings (2026-10-15)

1. **Test budget raised to +165 added lines,** with no trim (the actual is +161: the new 154-line test and 7 lines of link stubs). **From 2b-6 onward, the default test budget is +175 added lines when a new harness is needed, and +100 otherwise.** This is recorded in `AI_DEVELOPMENT_WORKFLOW.md` §13 (b).
2. **The link stubs are accepted:** no-op `CurrentReadings` setter stubs in `report_payload_golden_test.cpp`, `pir_sensor_lifecycle_test.cpp` and `sensor_wake_source_test.cpp`, needed because `PIRSensor.cpp` now calls the setters. The golden fixture and assertions are unchanged.
3. **Stage 7 rebuilds both sides fresh,** expecting +56 text. Copilot's "before" of 150876 came from `make compile-all`, not the release recipe.

## Closing record

Budget versus actual; ELF changes.
