# WO-2026-11-11-001 Stage 6 r3 report (saturating `consecutiveFaults`)

**Model:** claude-sonnet-5.5. **Level:** dispatched low; I cannot observe the level actually run.
**Interpreter:** CI discovery in this worktree, **118/118 (sh via zsh, py via python3)**. Changes uncommitted.

## Baseline
Not run separately before editing (deviation, below). The tree after the edit is 118/118; no file was added, so the file count matches the expected baseline of 118.

## Test first: fails on the current head
Scenario 16 `testAlert20HeldWhenFrozenFaultCounterWouldWrap` (`tests/pmic_fault_monitor_test.cpp`, invoked at `tests/pmic_fault_monitor_test.sh`). It uses production `pollAndRemediate()`: three thermal CHRG faults arm level 1 and run phase 0 (`disable==1`, `enable==0`); then 509 further polls with `safeToCharge=false` and the fault persisting (3 + 509 = 512, so a `uint8_t` wraps to 0), asserting alert 20 and the PMIC call counts on every poll. Then CHRG clears while frozen: alert 20 must remain, no enable call. Then safe: the cycle completes (`enable==1`), alert 20 still set; one further clean poll clears it.

Unfixed head: `Assertion failed: (testCurrent.alertCodeValue == 20) ... testAlert20HeldWhenFrozenFaultCounterWouldWrap ... line 634` (the alert cleared early). My first attempt used 300 polls, which does not land on a multiple of 256 and passed by accident; corrected to 509.

## `src/`
`PmicFaultMonitor.cpp:208`: `consecutiveFaults++;` -> `if (consecutiveFaults < UINT8_MAX) consecutiveFaults++;`
`git diff --numstat -- src`: **1 added, 1 removed** (+1/-1, net 0; the budget of +1 is met). Nothing else in `src/`.

## Mutations (backups in `build-tmp/.../bak/`, restored with `cp`, verified `cmp` identical)
| Mutation | Result |
|---|---|
| remove the saturation (`consecutiveFaults++;`) | CAUGHT: scenario 16, line 634 |
| drop `case 18` (ReportPublisher) | CAUGHT: alert19 test `testThrashReportedOnceThenClears` |
| drop `case 17` (connect clear is already gone) | CAUGHT: alert19 test `testBootStormReportedOnceThenClears` |
| restore a connect clear for 17 (`get_alertCode() == 17` in State_Connect.cpp) | CAUGHT: alert19 test script fails (grep guard) |
| drop the 20 predicate | CAUGHT: scenario 11 |
| drop the 23 predicate | CAUGHT: scenario 12 |
| drop guard 2a | CAUGHT: scenario 14 |
| drop guard 2b | CAUGHT: scenario 15 |
| `>` for `>=` at 4.05 V (mutated as `kPmicFullPackVcell+0.0001f`) | CAUGHT: scenario 13 |
| drop one `key1` vector (one golden fixture line disabled) | CAUGHT: `FAIL: 190 vectors, fixture has 189` |

## Verify
- Full suite after the edit: **118/118 (sh via zsh, py via python3)**; scenario 16 is a new scenario in an existing file.
- Device OS 6.6.0 boron build (`project.properties` `deviceOS=6.6.0`), fresh `BUILD_PATH_BASE` and `TARGET_DIR` as make arguments, gcc-arm 10.2.1:

| | `9b1072c` (Codex) | this change | delta |
|---|---|---|---|
| text | 153172 | **153180** | +8 |
| data | 1114 | 1114 | 0 |
| bss | 2240 | 2240 | 0 |

Nothing is claimed about hardware.

## Deviations
- No separate pre-edit baseline run; the post-edit count (118) equals the expected baseline.
- The tree HEAD is `90f9f08` (dispatch commit on top of `9b1072c`), not `9b1072c`; `src/` is identical.
- `$TMPDIR` was set to the scratch directory for test runs, because the shared default `$TMPDIR` binary was overwritten by another process mid-run.
- The 4.05 V and `key1` mutations are approximations of the named ones (the constant nudged; a fixture line disabled).
- Scratch files only in `build-tmp/wo-2026-11-11-001-stage6-r3/`.
