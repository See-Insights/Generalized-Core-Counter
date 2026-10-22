# WO-2026-10-22-003: Step 7 P7-A: remove the dead persistence accessors and freeze the stored layout

**Author:** architect, 2026-10-22 · **Status:** ready (Step 0 first)
**Branch:** `wo/2026-10-22-003-step7-p7a-remove-dead-persistence`, cut from main `df6d99d` or later. The PR is marked **"do not merge until v46 is cut"**; this ships in v47.
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.1–12.4; §13 (b), (c), (g), (j)
**Spec source:** `WO-2026-10-22-002-step0-report.md` §1 (classes d), §5 (shape A) and §8 (P7-1, P7-2, P7-3); the architect's rulings below. Re-check every citation.

## Plain goal

Delete the code for stored fields that nothing uses, and add a test that keeps the three stored records exactly as they are. **The stored bytes don't change:** each removed field's bytes stay in place as a named reserved slot, so record sizes stay 272, 36 and 88 B and v45 can still read the files after a downgrade. **No behavior change.** Expect a net **reduction** in `src/` lines.

## Architect's rulings on the Step 7 Step 0 (22 Oct)

1. **Shape A plus freeze is accepted.**
   - The three records stay byte-for-byte as they are, and no field is ever appended to them.
   - A future stored field goes in a file owned by its concern, when one is first needed.
   - Shapes B and C are rejected for now.
2. **Compatibility lasts while a rollback target reads these records.** "Forever" is too strong: the freeze holds while v45 (or any version reading these records) is a rollback target, and it is revisited after the fleet convergence.
3. **The two removal WOs (P7-1, P7-2) and the guard (P7-3) are one WO,** this one: all deletions plus one test change.
4. **`lastCountTime` waits.** Its writers are in `State_Sleep.cpp`/`State_Modes.cpp`, which the open sleep PRs (#128 to #139) change. Remove it in the cleanup series once those PRs merge.
5. **P7-4 is deferred, not cancelled.** That is the Device OS local-ledger bench test: whether `.get()` returns config before connecting. It decides P7-5 (config replay at boot), a behavior improvement rather than a line saving, and it is scheduled after v46 settles.
6. **P7-6 is dropped.** That WO would have stopped persisting `lastOccupancyEvent`. Boot already clears that value (report §3), so stopping its persistence would cost up to +20 lines for no change in behavior. It is recorded in the recovery plan as a known wrong-to-persist field, handled at boot.

## What goes (class d in the report; re-check each by bare name, §13 (c))

**SysData:** the accessors, facade forwarders and init/default writes for:
- `structuresVersion`;
- `lowPowerMode`;
- `sensorType` (the **SysData** field, not `SensorData::type` or the `SensorType` enum);
- `updatesPending` (the field, not Device OS's `System.updatesPending()`);
- `disconnectedMode`;
- `verboseModeStartTime`.

`reservedFloat0` already has no accessor.

**CurrentData:** the accessors, facade forwarders and reset writes for `faceNumber`, `faceScore`, `gestureType`, `gestureScore` and `externalTempC`. Check the `ISensor.h` references first: remove them only if nothing implements or calls them.

**Not removed:**
- `solarPowerMode`: the report marks it `d*` because `PowerManager.cpp:12–18` reads it as an input-profile fallback. Step 0 decides whether that reader is live. If it is, the field stays.
- `lastCountTime`: ruling 4.

**Each removed field's declaration** becomes a reserved slot of the same type and width, keeping its offset (the pattern of `reservedFloat0`, `MyPersistentData.h:158–160`).

## The freeze guard

Extend `tests/persisted_layout_preservation_test.py` so it covers:
- every offset and size of all three records (272/36/88), on ARM;
- the version constants (`SYS_DATA_VERSION = 4`, `SENSOR_DATA_VERSION = 1`, `CURRENT_DATA_VERSION = 1`);
- failing on any appended, inserted or resized member.

Mutations: append a field; change a reserved slot's width; bump a version. Each must fail the guard.

## Step 0 (Claude Code, read-only, §13 (c))

- **Re-check every field** by bare name across `src/` and `tests/`, and list each accessor, forwarder and write to delete.
- **Decide `solarPowerMode`**, live or dead, with the evidence.
- **Prototype and count** the net `src/` lines, which should be negative.
  - **PROCEED** if net `src/` ≤ +20 (a deletion WO will be well under).
  - **STOP** if any field turns out to have a live reader.
- **Ruling 8:** list the tests that read the removed accessors and say how each is re-pointed or retired.
- **Report:** at most 150 lines, committed on the WO branch.

## Budget

- **`src/`:** net ≤ +20 hard; expected about −100 to −127 (report §5, an estimate).
- **Tests:** guideline +100. Record ruling 9's numbers against `0c565fa`. Retiring tests for removed accessors counts toward the shrink.

## Tests and checks

- The suite reports `N/N`, run serially, and the golden payload is unchanged.
- **The ARM layout probe** emits 272/36/88 with every live field at its old offset.
- **`.backup`** is unchanged (retained outcome A); the ELF delta is explained by symbol.
- **Whitespace:** carried byte-for-byte is waived; new lines must be clean.

## Bench (Chip; Dev-14, laptop USB, branch build)

The report's matrix, done once:
1. Seed distinctive v45 values: counts, occupancy totals, a non-default ledger setting.
2. OTA or flash the candidate.
3. Reboot, then power-cycle, and check each live value reads back.
4. Flash v45 again and check the same values read back, with no re-initialization (no "initialize" log line).

## Agents and models

- **Step 0:** Claude Code (controller), read-only.
- **Stage 6:** Copilot, Sonnet tier, **medium**. Deletions plus one test extension, with a clear list.
- **Controller, before Stage 7 (§13 (g)):** reproduce the layout probe, the build, `nm`, the guard mutations and the suite, serially.
- **Stage 7:** Codex `gpt-5.6-sol`, **high**, stdin closed. It checks:
  - every removed field has no live reader;
  - offsets and sizes are unchanged;
  - downgrade safety;
  - the guard catches the three mutations.
- **Stage 8:** pre-approved on VERIFIED, or on VERIFIED WITH CONCERNS when every concern is Low, with net `src/` ≤ +20. Controller edits follow §13 (d). PR marked **"do not merge until v46 is cut"**.

## Closing record

- Budget versus actual.
- Ruling 9's numbers.
- The layout-probe output before and after.
- ELF by symbol.
- The retained outcome.
- The bench result.
- Fields removed and fields kept, with the reason for each.
