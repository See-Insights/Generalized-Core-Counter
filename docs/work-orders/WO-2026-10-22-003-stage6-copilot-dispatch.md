AGENT: Copilot · MODEL: claude-sonnet-5.5 (probed 2026-10-20, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- **Edit in `src/`:** `src/MyPersistentData.h`, `src/MyPersistentData.cpp`, `src/persist/SystemConfig.h`, `src/persist/PowerConfig.h` and `src/persist/CurrentReadings.h` **only**. **Tests:** `tests/persisted_layout_preservation_test.py` (becomes the freeze guard) and `tests/persistence_facade_behavior_test.cpp` (retire section 6); report each change.
- **Run** the host suite (serially) and the local ARM release build (Boron only, 6.4.1). Argon is not built.
- **Scratch directory:** `build-tmp/wo20261022-003-stage6/`. Remove **only it** when you finish.

**Not authorized:**
- Any other `src/` file; any change to a field's type, width, order or offset; **any new stored field**; removing `solarPowerMode` or `lastCountTime` (kept, rulings/Step 0 §2); any behavior change.
- `docs/`, `lib/`, `project.properties`, Device OS. Commits, pushes, merges, branch changes, stash, reset, checkout or restore. Device or network access beyond the Particle compile. Deleting anything you didn't create.

**SIZE BUDGET:** `src/` net ≤ +20 hard, counted over all lines (additions minus deletions); expected **−143** (Claude Code's prototype: +11/−154). Tests: guideline +100, expected +50 to +100. **Record ruling 9's numbers** against main `0c565fa` (baseline 28,893 / 834 / 91 / 22,306; test lines excluding `tests/fixtures/`, fixture lines, suite, `src/` lines).

# Stage 6 dispatch: WO-2026-10-22-003 (Step 7 P7-A: remove the dead persistence accessors, freeze the stored layout)

**Goal:** delete the code for 11 stored fields nothing uses, and keep the three stored records byte-for-byte as they are (272, 36, 88 B) so v45 can read them after a downgrade. Each field's bytes stay as a reserved slot of the same type and width. **No behavior change.**

**Repository:** this worktree, branch `wo/2026-10-22-003-step7-p7a-remove-dead-persistence` (from main `df6d99d`). HEAD adds this dispatch. Don't switch branches. Compute the `src/` tree hash before you start (`find src -type f | LC_ALL=C sort | xargs shasum -a 256 | shasum -a 256`); it should be `9d3053682ed531f250e84718b4f585957c4d15c1b4ec05ecb514b0182a921535`.

**Binding spec:** `WO-2026-10-22-003-step7-p7a-remove-dead-persistence.md` and `WO-2026-10-22-003-step0-report.md` (the field table, `solarPowerMode` kept, ruling 8). `WO-2026-10-22-003-prototype.diff` is Claude Code's compiled `src/` sketch (net −143); a reference, not a patch to apply blindly. Where this dispatch and the WO differ, the WO wins; report it.

## Do

1. **Rename the 11 declarations in place** to reserved slots (same type, same position): SysData `structuresVersion`→`reservedUint8_0`, `lowPowerMode`→`reservedBool0`, `sensorType`→`reservedUint8_1`, `updatesPending`→`reservedBool1`, `disconnectedMode`→`reservedBool2`, `verboseModeStartTime`→`reservedTime0`; CurrentData `faceNumber/faceScore/gestureType/gestureScore`→`reservedUint16_0..3`, `externalTempC`→`reservedFloat1`; comment "Reserved: was <name>; kept to preserve the stored layout".
2. **Delete** their accessors (`get_`/`set_`) in `MyPersistentData.h/.cpp`, the facade forwarders in `MyPersistentData.cpp` and `persist/*.h`, the four `initialize()` writes (`set_structuresVersion(2)`, `set_lowPowerMode(false)`, `set_sensorType(1)` on **sysStatus** (not `sensorConfig.set_sensorType(1)`), `set_verboseModeStartTime(0)`) and the now-orphaned doc comment above `SystemConfig::get_sensorType` (SysData). Re-check each by bare name before deleting; if anything reads a field, STOP and report.
3. **The freeze guard** (`persisted_layout_preservation_test.py`): replace the name-based probe/baseline comparison by a **position-based** one: extract every data member of `SysData`, `SensorData`, `CurrentData` from the current header in order, compile an `offsetof`/`sizeof` table on ARM (the existing toolchain path and flags), and compare to a **frozen list** of (offset, size) per member committed in the test (SysData 51 members, 272 B; SensorData 5, 36 B; CurrentData 17, 88 B; you can generate the list from the unmodified header on `HEAD`, and cross-check it against `WO-2026-10-22-002-step0-report.md` §1), plus the three `*_DATA_VERSION` (4, 1, 1) and magics. Fails on any appended, inserted, removed, resized or moved member and on a version bump.
4. **Retire section 6** of `persistence_facade_behavior_test.cpp` (lines 264–273 and its `OK:` line) and anything else that only served it.

## Verify (all required)

- **Mutations (each in a `git worktree add --detach` copy):** append a field to each of the three classes; change a reserved slot's width; bump a version; move a member; each must fail the guard. A rename of a reserved slot must **pass** (the guard is positional).
- **Full suite** before and after, `N/N (sh via zsh, py via python3)`, serial; baseline **91**; golden payload unchanged.
- **ARM (Boron 6.4.1, fresh `BUILD_PATH_BASE`, `-frandom-seed=p6`; "before" from `git archive HEAD`):** release build text/data/bss before and after; explain any change over ±64 B by symbol with `nm -C -S`; **the whole `.backup` section before and after** (retained outcome A); the guard's own offsets for the three records.
- `git diff --check` clean on new lines (carried whitespace on moved or deleted lines is waived).

## Implementation Report (≤ 150 lines, your final message)

Files changed with net and gross `src/` lines against +20; test lines added and removed and ruling 9's numbers; the guard's frozen table and mutation results; the suite before and after; the ARM build with symbol deltas and the `.backup` outcome; the `src/` hash before and after; scratch removal; deviations (or "none"); the model used.
