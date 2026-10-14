# WO-2026-10-14-002: Step 6 2b-2 — merge `setup()` into `initializeHardware()`, remove `sensorDetect`

**Author:** architect, 2026-10-14 · **Status:** ready (citation re-check first, see below)
**Branch:** `wo/2026-10-14-002-step6-2b2-lifecycle`, from main after the 2b-1 PR merges (the golden test must be on the base)
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.2–12.4; §13 (b), (c), (g), (j)
**Spec source:** `WO-2026-10-13-002-step0-round2-report.md`, Q2 "2b-2" (round-2 citations are at `25dbc69`; re-check them at the new base before dispatch).

## Plain goal

The sensor has one start operation instead of two overlapping ones, and the dead `sensorDetect` flag is gone — with no change in device behavior and the golden payload test unchanged.

## Changes (two commits)

- **(a) 6a, `setup()` merge:** remove `ISensor::setup()` (the round-2 report's `ISensor.h:79–84`); make `initializeHardware()` pure virtual (`:120`); rename `PIRSensor::setup()` to `initializeHardware()` (`PIRSensor.h:39`). Body unchanged.
- **(b) 6b, `sensorDetect` removal:** delete the variable and its `extern` (`Generalized-Core-Counter.cpp:156`, `StateMachine.h:36`); simplify its two reads (`State_Sleep.cpp:1094–1096`, `State_Idle.cpp:255`). The log line that printed `sensor=%d` loses that field (it was always 0); accepted.

## Test

New `tests/pir_sensor_lifecycle_test` with fakes for `pinMode`, `digitalWrite`, `attachInterrupt`, `detachInterrupt`, recording the call order through `initializeHardware`, `onSleep`, `onWake`, and that the captured ISR sets the motion flag. **Commit it first and show it passes on the unchanged code**, then apply (a) and (b) with it still passing. The 2b-1 golden test must pass unchanged at every commit.

## Budget

Net `src/` about **−8** (≤ 0 required). Gross additions only where a rename or simplification requires them. Test lines separately; stop and report past +150.

## Agents and models

- **Citation re-check:** Claude Code, before dispatch, at the new base; report any shift in one line. Stop if anything is not as the round-2 report says.
- **Stage 6:** Copilot, Sonnet tier, **medium**.
- **Stage 7:** Codex `gpt-5.6-sol`, **medium**. Checks: no caller of `ISensor::setup()` left (bare and qualified search, §13 (c)); release ELF: `PIRSensor` vtable loses one slot, no other logic change; ISR attach order identical; golden test passes unchanged; the lifecycle test fails if the call order in `initializeHardware` changes.

## Bench

None before merge; rides the v43 soak.

## Stage 7 pre-ruling (architect, 2026-10-14)

The now-redundant outer parentheses at `State_Idle.cpp:255` (left when `sensorDetect ||` was dropped) **stay**. If Codex flags them, record them as an **observation**, not a finding.

## Closing record

Budget versus actual; ELF symbol changes and their reasons.
