# WO-2026-11-11-001 Stage 6 round 2 report (Copilot)

Changes are **uncommitted**. Model: claude-sonnet-5.5. Level: dispatched medium; I cannot observe the level actually run.
Interpreter line: **118/118 (sh via zsh, py via python3)**.

## Baseline (before editing)
118 files (`tests/*.sh` + `tests/*.py`); CI discovery measured **118/118** on this head (`257cd29`, round-1 code `007504c`).

## Tests first: both fail on the current head
Added to `tests/pmic_fault_monitor_test.cpp` (real `pollAndRemediate()` via `latchPoll`), wired as scenarios 14 and 15 in the `.sh`:
- **14** `testAlert20ThermalHeldThroughRemediationCycle`: thermal 0x20 x3 (raises 20, arms level 1, phase 0), then a clean poll (20 must stay, no enable yet), a clean poll after 600 ms (cycle completes, 20 stays), one more clean poll (20 clears).
- **15** `testAlert20HeldWhileNtcActiveAfterCombinedFault`: 0x21 raises 20; 0x01 (NTC only) keeps 20; 0x00 clears it.

Pre-fix: scenario 14 `Assertion failed: alertCodeValue == 20 ... line 577` (first clear poll); scenario 15 `... line 601` (NTC-only poll). Scenarios 1-13 passed.

## Guards (`src/power/PmicFaultMonitor.cpp`)
- 2b (`:376`): `if (currentAlert >= 20 && currentAlert <= 23 && !(currentAlert == 20 && ntcFaultCode != 0)) {`
- 2a (`:390`): `if ((currentAlert == 20 && ntcFaultCode == 0 && consecutiveFaults == 0) || (currentAlert == 23 && ...))`; still after the 21 clear.

Each is an edit of an existing line: **+2/-2, net 0** on `007504c`. Against `f00aa04`: `src/` still **+12/-13, net -1** (unchanged from round 1). Within the +2 budget.

**23 half not widened.** 23 is raised alone via BAT_FAULT (`:420`), which never touches `consecutiveFaults`; the charge-fault "other code" path that would raise 23 is unreachable (a nonzero two-bit field is always 1, 2 or 3). Neither scenario nor finding needs it.

## Mutations (original restored with `cp`; `src/` verified identical afterwards)
| Mutation | Result |
|---|---|
| remove guard 2a | CAUGHT: scenario 14 |
| remove guard 2b | CAUGHT: scenario 15 |
| drop the 20 predicate | CAUGHT: scenarios 11 and 15 |
| drop the 23 predicate | CAUGHT: scenario 12 |
| `>` for `>=` at 4.05 V (`kPmicFullPackVcell`) | CAUGHT: scenario 13 |

All 15 scenarios pass with both guards; no existing scenario changed result.

## Build (Device OS 6.6.0, boron, fresh `BUILD_PATH_BASE` and `TARGET_DIR` as make args, `make -C .../deviceOS/6.6.0/modules/boron/user-part`, gcc-arm 10.2.1)
| | round 1 (`007504c`) | this change | delta |
|---|---|---|---|
| text | 153164 | **153172** | +8 |
| data | 1114 | 1114 | 0 |
| bss | 2240 | 2240 | 0 |

`nm -S` `pollAndRemediate`: 0x68c (round 1) -> **0x698** (+12). Nothing is claimed about hardware.

## Docs
`docs/reference/alert-codes.md` row 20 now says the broad clear leaves 20 while NTC is active, and the NTC clear also needs `consecutiveFaults == 0`. Row 23 unchanged. Line references did not shift.

## Deviations / open questions
- None against the stop conditions. Scratch files are only in `build-tmp/wo-2026-11-11-001-stage6-r2/` (compiled test binary, `fw/` build output, `fixed.cpp`/`orig.cpp` backups, `fw.log`).
- Two small notes: a build with `-j8` was used; I removed an empty scratch file I created by mistake.
