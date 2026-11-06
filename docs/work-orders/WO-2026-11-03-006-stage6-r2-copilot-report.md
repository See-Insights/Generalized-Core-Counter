# WO-2026-11-03-006 (C2) — Stage 6 round 2 report

Model: claude-sonnet-5.5; reasoning effort: the dispatch said "medium"; I did not set or observe a different level. Interpreters: host `.sh` via zsh (macOS), `.py` via Python 3.14.6, C++ via `clang++ -std=c++17`/`c++`. Uncommitted, unpushed.

## Net `src/` lines

`git diff --numstat d48dc27 -- src`: **+26 / −10 = +16 net** (budget 16; round 1 was +15). The one added line is the boundary reopen in `State_Report.cpp`; the other two edits fold onto existing lines (the close clear and the boot restore), so they add no lines. No check trimmed.

## Fixes

1. **Stale `occupancyStartMs`.**
   - Close clears both fields on every successful close: `State_Common.h:454`.
   - Boundary reopen (`close == 24`, trusted piece) sets `occupancyStartMs = millis(); pieceUntrusted = !Clock::isTrusted()`: `State_Report.cpp:82`.
   - Boot restore sets `occupancyStartMs = millis()` next to the `pieceUntrusted` line: `Generalized-Core-Counter.cpp:503`.
   - `handleOccupancyMode` (`State_Modes.cpp:72-73`) and `processSleepWakeEvent` (`:234-235`) already set both, unchanged.
2. **Wall-start checks on a piece.**
   - `start == 0` applies to the wall path only: `State_Common.h:398` (`invalid = !piece && (!timeValid || start == 0)`).
   - `start > now + 5` applies to the wall path only: `:416`.
   - The one-day session cap and the daily-total validation are untouched, so they still apply to pieces.
3. **Acceptance numbers** (test only; the WO text is untouched). `tests/untrusted_piece_c2_test.sh` replay now asserts:
   - pre-boundary 5,700 s (control) and 5,431 s (fault);
   - after the boundary close the eleventh piece is credited its full 570 s: 6,270 s (control) and 6,001 s (fault);
   - straddler 301 s at a 300 s debounce with no PIR after boot; the 236 s case is kept and labelled as the 60 s debounce case.
   - No "96-98 min" text remained in the test (the only match was "236").

## Tests (all in `tests/untrusted_piece_c2_test.sh`, using the real extracted close, predicate and `updateOccupancyState`)

- `testClosedPieceClearsFieldsAndReopenSetsThem`: close an untrusted piece, run the boundary reopen, assert the new session's `occupancyStartMs == millis()` and `pieceUntrusted == false`, then credit it on the wall path. A structural regex check pins the reopen line in `State_Report.cpp` and the boot line in `Generalized-Core-Counter.cpp`, because those sites cannot be driven on the host.
- `testPieceIgnoresWallStartChecks`:
  - untrusted piece measured 360 s with a ten-hour backward correction credits 360 s;
  - the same with a zero wall start credits 360 s;
  - a trusted piece with a backward step is discarded as before (total 0, one `OccAnom`);
  - a trusted piece with a zero start is discarded as before.
- `testNextPieceStartsClean` now reads the start while the piece is open, because close now clears it.
- `testDev09Replay` carries the corrected figures above.
- `tests/standby_latch_daily_clear_test.cpp` needed stubs: its harness compiles the real `State_Report.cpp` block, which now references `session.occupancyStartMs`, `session.pieceUntrusted`, `millis()` and `Clock::isTrusted()`. I added those four stubs (+4 lines). This file is not named in the dispatch, but it is under `tests/`, which is authorized.

## Mutations (all caught: 16/16 in the C2 test)

The 10 round-1 mutations are still caught. New, all caught:

- close leaves the piece start stale;
- close leaves the piece flag set;
- boundary reopen without the piece fields;
- boot restore without `occupancyStartMs`;
- restore the `start == 0` check for pieces;
- restore the `start > now + 5` check for pieces.

The round-1 mutations in `occupancy_report_by_mode_test` are unchanged; that test passed.

## Verification

- Host suite: **109/109** (sh via zsh, py via python3). Counted with `p=$((p+1))`; no new test file, so 109 as expected. Logs and runner are in `build-tmp/wo-2026-11-03-006-stage6-r2/`.
  - Caveat: `thermal_coupling_structural_test.py` scans the whole repo, so inside the repo it fails on `SensorManager` copies left under `build-tmp/wo-2026-11-03-006-stage6/` and `-stage7-r1/` (not mine, not deleted). It passes on a clean copy of the tracked files, which is what the 109 counts. `standby_latch_daily_clear_test.sh` failed after the `src` change until I added the stubs above, then passed.
- `persisted_layout_preservation_test.py` and `retained_boot_structural_test.py`: green.
- S-4/S-6 fixtures (`tests/fixtures/`): untouched, no diff against the round-1 head.
- Build: Device OS 6.6.0, boron, fresh `BUILD_PATH_BASE=build-tmp/wo-2026-11-03-006-stage6-r2/obj-release`, rc=0, text/data/bss **152036 / 1114 / 2208** (the same as round 1). `nm tree.elf`: `closeOccupancySessionSafely` (W, `c1d40`), `updateOccupancyState` (`c25ac`), `processSleepWakeEvent` (`c2770`), `debounceRemainingMs` (`c2510`). Four `bl c1d40` call sites, at `c2076`, `c25d2`, `c27a4` and `c2ac6` (idle, `updateOccupancyState`, wake, report). The credit log string is in the ELF.

## Deviations and open questions

- The `standby_latch_daily_clear_test.cpp` stub edit, above.
- The boot restore sets `occupancyStartMs = millis()` at `setup()` time, not 0. A straddler with no PIR credits `(first evaluation − setup) + 300 s`, which is about 301 s but depends on the gap between `setup()` and the first loop pass. The test fake restarts `millis()` at 0 at boot.
- The pre-existing `build-tmp/` scratch trees break `thermal_coupling_structural_test.py` in-repo. The controller may want them cleaned before Stage 7.
- Not done, Chip's step: Dev-14 bench.
