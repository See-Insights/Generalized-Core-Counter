# WO-2026-10-15-002: Step 6 2b-6 — wire the sensor's reset (central work stays central)

**Author:** architect, 2026-10-15 · **Status:** ready (citation re-check first)
**Branch:** `wo/2026-10-15-002-step6-2b6-wire-reset`, from main **after the 2b-5a PR merges**
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.2–12.4; §13 (b), (c), (g), (j)
**Spec source:** `WO-2026-10-13-002-step0-round2-report.md`, Q2 "2b-6" (cited at `25dbc69`). Architect pre-check at the 2b-5a head (`1203856`) below; re-check at the new base.

## Plain goal

Switch the hourly and daily count resets over to the sensor's own `resetTotals()` hook (added in 2b-5a), while the non-sensor parts of the daily reset (`lastCountTime`, `resetCount`) stay in the central code. **No device behavior changes:** the same fields are cleared, in the same order, at the same moments. This is the last 2b WO before the v43 release.

## Architect's citation pre-check (at `1203856`)

| Round-2 cite | Now | Note |
|---|---|---|
| `resetEverything()` `MyPersistentData.cpp:695–706` | `:689–706` | `lastCountTime` `:695`, `resetCount` `:696`, sensor setters `:699–706` |
| `initialize()` call `:772` | `:772` | keeps the default (full reset) |
| wrapper | `MyPersistentData.cpp:1200`, `persist/CurrentReadings.h:39` | **not in round 2**: the bool must pass through this wrapper too |
| `dailyCleanup` `:2736` | `Generalized-Core-Counter.cpp:2697`; the call is `:2717` | moved by 2a/2b |
| `State_Report.cpp:101` | `src/state/State_Report.cpp:101`, inside the `COUNTING` guard `:100` | unchanged |
| `SensorManager::activeSensor()` | `sensors/SensorManager.cpp:317–321` | already public; the factory returns singletons (`SensorFactory.h:74`), so no heap use |
| `validate()` `set_hourlyCount(0)` `:715` | unchanged | **out of scope**: corrupt-data repair, not a period reset |

## Claude Code's citation re-check (2026-10-15, at main `d89a4ef`, after #101)

**Every row of the architect's table holds unchanged:**
- **`MyPersistentData.cpp`:** `resetEverything()` at `:689–707` (`lastCountTime` `:695`, `resetCount` `:696`, sensor setters `:699–706`); the `initialize()` call at `:772`; the wrapper at `:1200`; the `validate()` repair at `:713–716`.
- **Declarations:** `CurrentReadings.h:39`; `MyPersistentData.h:636`.
- **`dailyCleanup()`:** at `Generalized-Core-Counter.cpp:2697`, with its `CurrentReadings::resetEverything()` call at `:2717`.
- **The hourly reset:** `State_Report.cpp:101`, inside the `COUNTING` guard at `:100`.
- **`activeSensor()`:** at `SensorManager.cpp:317–321`.

**Callers of `resetEverything()`:** only `:2717` and `:772`.

**Tests that anchor on today's text:**
- `daily_cleanup_boundary_test.py:162` looks for `CurrentReadings::set_hourlyCount(0)` in `handleReportingState()`;
- `persistence_facade_behavior_test.cpp:223` exercises the no-argument `resetEverything()`.

`pir_sensor_reset_totals_test.sh:16–20` extracts the six sensor-total setters from `resetEverything()`. They remain in its body after the early return, so it should keep passing; confirm.

## Design (do not widen)

1. `resetEverything(bool includeSensorTotals = true)`: after `lastCountTime` and `resetCount`, `if (!includeSensorTotals) return;`. Same parameter on the `CurrentReadings::resetEverything` wrapper. `initialize()` keeps the default `true`.
2. `dailyCleanup()`: `CurrentReadings::resetEverything(false);` then `SensorManager::instance().activeSensor()->resetTotals(ResetPeriod::Daily);` Order inside `dailyCleanup()` is otherwise unchanged; the daily close order (publish → `dailyCleanup()` → restart) is untouched.
3. `State_Report.cpp:101`: replace `CurrentReadings::set_hourlyCount(0);` with `activeSensor()->resetTotals(ResetPeriod::Hourly);` **inside the existing `COUNTING` guard** (the guard stays central).
4. Callers use `activeSensor()` directly. A `SensorManager::resetTotals()` wrapper (round-2's +6) is **optional**; add it only if it removes an include from a central file. Either way, the `_sensor == nullptr` path (closed boot) must still reach PIR's hook through the fallback.
5. Not touched: `validate()`'s repair at `:715`, `_motionDetected`, any other caller.

## Budget

Net `src/` **≤ +20, hard** (estimate +7 without the wrapper, +13 with it). One commit, or two if gross must stay ≤ 40. Tests: guideline +100 (no new harness expected); record the actual.

## Tests

- **Golden test unchanged** (fixture byte-identical); add payload-after-reset vectors only if the existing harness takes them cheaply.
- `persistence_facade_behavior_test.cpp` (~`:223`): `resetEverything(false)` sets `lastCountTime` and clears `resetCount` but leaves all six sensor fields; `resetEverything()` still clears all eight.
- `daily_cleanup_boundary_test.py` (~`:162`): update the token; **keep the order test** (publish → cleanup → restart).
- End to end on the host: the daily path's recorded setter sequence equals today's eight in today's order; the hourly path clears only `hourlyCount`, and only in COUNTING mode (OCCUPANCY: nothing).
- `_sensor == nullptr` still reaches the hook for both periods.
- Structural: no `set_hourlyCount(0)` left in `State_Report.cpp`; `dailyCleanup()` no longer calls the no-argument `resetEverything()`.

## Agents and models

- **Stage 6:** Copilot, Sonnet tier, **high**. Touches persistence and the daily close.
- **Stage 7:** Codex `gpt-5.6-sol`, **high**. Checks:
  - the setter sets and order equal today's on both paths and in both modes;
  - `initialize()` is still a full reset;
  - the close order is unchanged;
  - the null-sensor fallback works;
  - ELF: a fresh `BUILD_PATH_BASE` on both sides; text delta is small and explained by symbol (`nm`).

## Bench

None in this WO. Bench validation is the v43 soak (Dev-09 and Dev-14): hourly counts restart each hour, daily totals are zero after the close, and PIR and timer wakes behave as on v42.

## Closing record

Budget versus actual (`src/` and tests), ELF changes by symbol, suite `N/N`.
