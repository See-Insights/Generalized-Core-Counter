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

1. **The +161 test lines are accepted** (the new 154-line test plus 7 lines of link stubs, against the +100 stated). **New rule, also in `AI_DEVELOPMENT_WORKFLOW.md` §13 (b):**
   - **`src/` budgets are hard:** stop and report.
   - **Test budgets are a guideline:** record the actual and note an overrun, but don't stop; flag only if tests exceed about twice the guideline.
   - **The default guideline from 2b-6 onward:** +175 added lines when a new harness is needed, +100 otherwise.
2. **The link stubs are accepted:** no-op `CurrentReadings` setter stubs in `report_payload_golden_test.cpp`, `pir_sensor_lifecycle_test.cpp` and `sensor_wake_source_test.cpp`, needed because `PIRSensor.cpp` now calls the setters. The golden fixture and assertions are unchanged.
3. **Stage 7 rebuilds both sides fresh,** expecting +56 text. Copilot's "before" of 150876 came from `make compile-all`, not the release recipe.

## Closing record (2026-10-15)

**Result:** **VERIFIED** at Stage 7 (`WO-2026-10-15-001-stage7-verdict.md`), with no findings. **Stage 8 approved** by the architect. **Release:** rides v43; no behaviour change (the hook has no caller until 2b-6).

| Stage | Agent / model | Result |
|---|---|---|
| Citation re-check | Claude Code, at main `2f18c38` | held: `resetEverything()` at `MyPersistentData.cpp:689–707`, its setters at `:699–706`; the hourly reset at `State_Report.cpp:101`; `PIRSensor::reset()` clears `_motionDetected` |
| Stage 6 | Copilot, `claude-sonnet-5.5`, medium | +20 `src/`; new `pir_sensor_reset_totals_test`; 79/79 → 80/80 |
| Rulings | architect | tests +161 accepted (§13 (b): `src/` budgets hard, test budgets a guideline); link stubs accepted |
| Stage 7 | Codex, `gpt-5.6-sol`, medium | VERIFIED, no findings; exactly +56 text; 4 + 1 mutations caught |

Every model was probed first (§5). One round; no controller edits.

| Item | Budget | Actual |
|---|---|---|
| `src/` (one commit) | ≤ +20 (estimate +16) | **+20** (+20/−0): `ISensor.h` +6, `PIRSensor.h` +3, `PIRSensor.cpp` +11 |
| Tests | +100 stated (a guideline under the revised §13 (b)) | **+161 / −1**: the new 154-line test, and 7 lines of link stubs in three tests |

**ELF changes** (Stage 7, fresh release builds): text 151308 → **151364** (**+56**); data and bss unchanged.

| Symbol | Change | Reason |
|---|---|---|
| `PIRSensor::resetTotals(ResetPeriod)` | new, 0x34 | the PIR override (the six setters for Daily, the hourly count only for Hourly) |
| `vtable for PIRSensor` | 0x44 → 0x48 | one slot for `resetTotals()` |
| `CSWTCH.215/218` → `216/219` | renumbered, same sizes | compiler-local |

- **The design as built:** `enum class ResetPeriod { Hourly, Daily };` and `virtual void resetTotals(ResetPeriod) {}` (the default does nothing). PIR's override doesn't touch `_motionDetected`, `reset()`, `lastCountTime` or `resetCount`.
- **The tests:**
  - **Sequences:** they record the Hourly and Daily setter sequences, and compare Daily with the six setters extracted from the real `resetEverything()`.
  - **ISR-pending:** an interrupt fired before either reset still shows motion on the next `loop()`.
  - **No-caller check:** structural.
  - **Mutations caught:** swapped and dropped Daily setters, the hook clearing `_motionDetected`, an added caller, and Hourly clearing the daily count.
- **Golden test:** fixture and assertions unchanged, 186 vectors with 0 failures.
- **Bench:** none; there's no behaviour change.
