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

## Step 0, Stage 6 and controller reproduction (22 Oct)

**Step 0** (`WO-2026-10-22-003-step0-report.md`): PROCEED, prototype net −143. `solarPowerMode` is **live and kept** (read under `FIELD_BUILD`, which every release build defines; `PowerManager.cpp:12–18`). `lastCountTime` kept (ruling 4).
**Stage 6** (Copilot, Sonnet 5.5, medium): round 1 applied the design (11 declarations renamed in place to `reserved*` slots, accessors, facade forwarders, four `initialize()` writes and one orphaned comment deleted; the freeze guard rewritten positionally; facade-test section 6 retired). **Round 2** (after Stage 7 round 1) made the guard fail closed and match live members by name and type, and updated two header comments to say the records are frozen. `src/` tree hash (`find src -type f | LC_ALL=C sort | xargs shasum -a 256 | shasum -a 256`) `9d30…2535` → `3da3961c20e6308fabd1f36b37ffc18e083ba34e4fe3d176d03caefc1c5432d1`.
**Controller reproduction** (Boron 6.4.1, fresh `BUILD_PATH_BASE`, `-frandom-seed=p6`, "before" is `git archive HEAD`): suite **91/91** (sh via zsh, py via python3, serial); text/data/bss `151724/1094/2228 → 151636/1094/2228` (−88 text); **`.backup` byte-identical** with the same retained symbols (outcome A); guard mutations in git-backed copies: appended tail-padding member, end append, inserted member, resized live member, reorder, live rename and swap all **fail**; a `reserved*` rename and the clean tree **pass**.

## Stage 7 result (22 Oct)

**Round 1** (Codex `gpt-5.6-sol`, high; `…-stage7-verdict.md`): NOT VERIFIED on the guard only: **F1 (High)** a tail-padding member declared with an initializer was not parsed, so appending it passed; **F2 (High)** the frozen table had no member identity, so swapping `openTime`/`closeTime` passed; **F3 (Low)** two header comments still said "append at the end". Checks 1 (no live reader), 2 (layout identical, 272/36/88, all 161 probe words equal), 3 (downgrade safe: v45 reads none of the retired bytes; the four no-longer-initialized slots are now 0 on a first boot; existing devices keep their stored values), 5, 6 and 7 PASS.
**Round 2** (the last round; `…-stage7-round2-verdict.md`): **VERIFIED WITH CONCERNS (Low only)**; no real freeze gap with ordinary syntax. **C1 (Low), accepted by the architect under §12.4 ("accepted by the architect: parser edge case, not a regression"), not fixed:** the method-declaration allowlist also accepts a function-pointer data member (`void (*x)();`), which no real record uses.

## Closing record

| Item | Budget | Actual |
|---|---|---|
| `src/` net | ≤ +20, hard (expected about −100 to −127) | **−144** (+14/−158): `MyPersistentData.cpp` −96, `MyPersistentData.h` +14/−45, `SystemConfig.h` −10, `PowerConfig.h` −2, `CurrentReadings.h` −2; only the five authorized files; round 2 added no code lines |
| Tests | guideline +100 | **+55 net** (+150/−95): the guard rewritten (+105/−81 then +81/−36), facade-test section 6 retired (−14); no fixture; no new suite file |
| Suite | baseline 91 | **91/91**; golden payload unchanged |

- **Test complexity (ruling 9), against main `0c565fa`:** `src/` 22,306 → 22,162 (**−144**); test lines excluding fixtures 28,893 → 28,935 (**+42**; the +55 is against `df6d99d`); fixtures 834 → 834; suite 91 → 91. (Counts here are over `df6d99d`'s tree plus this WO: the open sleep PRs are not in it.)
- **Layout probe, before and after:** SysData 272 B (51 members + header), SensorData 36 B (5), CurrentData 88 B (17); versions 4/1/1; magics `0x20a15e75`/`0x20a47e74`/`0x20a99e74`; every offset unchanged (all 161 emitted words equal). The guard now freezes `(name, type, offset, size)` per member.
- **ELF:** −88 B text: `sysStatusData::initialize()` 0x1b0 → 0x188 (−40) and four setters gone (`set_lowPowerMode`, `set_sensorType`, `set_structuresVersion` 12 B each, `set_verboseModeStartTime` 10 B); static-init stubs unchanged.
- **Retained outcome A.** No `retained` keyword added.
- **Fields removed (11):** SysData `structuresVersion`, `lowPowerMode`, `sensorType` (the SysData one), `updatesPending`, `disconnectedMode`, `verboseModeStartTime`; CurrentData `faceNumber`, `faceScore`, `gestureType`, `gestureScore`, `externalTempC`: each now a `reserved*` slot of the same type and width. **Kept:** `solarPowerMode` (live reader), `lastCountTime` (ruling 4), `reservedFloat0`/`reservedByte0` (as before).
- **Bench (Chip; Dev-14, laptop USB, branch build): not run, waits for Chip.** Note for step 4: a first boot now leaves `structuresVersion`, SysData `sensorType`, `lowPowerMode` and `verboseModeStartTime` at 0 (they were initialized to 2, 1, false, 0); v45 reads none of them.
- **Deviations:** none from the design; Stage 6 took two rounds; the guard's frozen table includes each record's header entry.
- **Rounds:** Stage 6 two; Stage 7 two (NOT VERIFIED, VERIFIED WITH CONCERNS). Ships in v47. **The PR is marked "do not merge until v46 is cut".**

## For the architect

- C1 (function-pointer data member) accepted under §12.4, recorded.
- `solarPowerMode` is a de-facto constant after first boot (only `initialize()` writes it); replacing the read by the constant is a possible later simplification.
