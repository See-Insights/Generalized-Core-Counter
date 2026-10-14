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

## Closing record (2026-10-14)

**Result:** **VERIFIED** at Stage 7 (`WO-2026-10-14-002-stage7-verdict.md`), with no findings. **Stage 8 approved** by the architect. **Release:** rides the next release (v43) with 2a and 2b-1.

| Stage | Agent / model | Result |
|---|---|---|
| Citation re-check | Claude Code, at main `3092077` | no shift from round 2; `ISensor::setup()` had one caller (the default `initializeHardware()`); `sensorDetect` never written |
| Stage 6 | Copilot, `claude-sonnet-5.5`, medium | −10 net `src/`; lifecycle test 147 lines; 77/77 → 78/78 |
| Stage 7 | Codex, `gpt-5.6-sol`, medium | VERIFIED; 3 of 3 order mutations caught; golden test 175/0 at every step |

Every model was probed first (§5). One round; no controller edits.

| Item | Budget | Actual |
|---|---|---|
| Commit 1: lifecycle test | tests ≤ +150 | **147** (`pir_sensor_lifecycle_test.{sh,cpp}`, `tests/stubs/pir_lifecycle/Particle.h`) |
| Commit 2: (a) `setup()` merge | net ≤ 0 | **+5/−13 = −8** (`ISensor.h`, `PIRSensor.h`) |
| Commit 3: (b) `sensorDetect` removal | net ≤ 0 | **+4/−6 = −2** (`Generalized-Core-Counter.cpp`, `StateMachine.h`, `State_Sleep.cpp`, `State_Idle.cpp`) |
| **WO `src/` total** | about −8, ≤ 0 | **−10** (+9/−19); every addition is from the rename or a simplification |

- **ARM build:** text 151372 → **151308** (−64); data 1094 and bss 2212 unchanged.

**ELF symbol changes and their reasons** (Stage 7 `nm -C -S`, HEAD against the working tree):

| Symbol | Change | Reason |
|---|---|---|
| `PIRSensor::setup()` → `PIRSensor::initializeHardware()` | renamed, size unchanged (0x6c) | (a) the rename; the body is byte-identical (793 source bytes) |
| `ISensor::initializeHardware()` (default forwarder) | removed (0x06) | (a) `initializeHardware()` is now pure virtual; the `{ return setup(); }` default is gone |
| `vtable for PIRSensor` | 0x40 → 0x3c | (a) one slot fewer: `ISensor::setup()` removed |
| `sensorDetect` | removed (1 byte, bss) | (b) the variable deleted; it was never written |
| `handleIdleState()` | 0x4e8 → 0x4d8 | (b) the always-false `sensorDetect ||` test removed at `State_Idle.cpp:255` |
| `handleSleepingState()` | 0x1c98 → 0x1c80 | (b) the always-false test and the `sensor=%d` log argument removed at `State_Sleep.cpp:1094–1096` |
| `CSWTCH.*` | renumbered, sizes unchanged | compiler-local numbering after the edits |

- **Lifecycle test:** it compiles the real `PIRSensor` against fakes and records the start, sleep and wake call order, with `attachInterrupt` last on start and wake. It also checks that the captured interrupt handler sets the motion flag. It passed on the unchanged HEAD before (a) and (b). Moving the attach first, swapping the first two pin calls, or removing the attach each fails it.
- **Golden payload test:** unchanged, passing at every commit.
- **Observations, not findings** (the architect's pre-ruling): the redundant outer parentheses at `State_Idle.cpp:255` stay. `State_Sleep.cpp:1095` has trailing whitespace, already present at HEAD.
- **Bench:** none before merge; rides the v43 soak.
