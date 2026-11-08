# WO-2026-11-05-005 (S3) Stage 6 round 2: Copilot report

Model: claude-sonnet-5.5. Reasoning level: dispatched as medium; the level actually run could not be observed.
Interpreter/toolchain: Python 3.14.6; zsh 5.9; Apple clang++ (`-std=c++17`); GNU Arm Embedded 10.2.1; Device OS 6.6.0.
Nothing committed, pushed, merged or flashed. No hardware claim is made.

## F1 (src)
`powerOffForRecovery()` (`src/power/ConnectivityFailsafe.cpp`) now calls `ExternalWatchdog.stop()` immediately before `System.sleep(cfg)` and `ExternalWatchdog.start()` immediately after it (every return), with a one-line comment. Same calls S2 uses around HIBERNATE: stop at `State_Sleep.cpp:706`, restart on the failure path at `:886`; ULP stop `:773`, restart `:914`. Callers' logic and log lines, off time (30 s) and the evidence logic are untouched.
Residual (Amendment 5): between the stop and the HAL cutting power there is no chip watchdog; a hang there is not recovered by the watchdog.

**Net `src/` against 62e3dbc: +16** (+18/-2; `git diff --numstat 62e3dbc -- src`: ConnectivityFailsafe.cpp 13/1, ConnectivityFailsafe.h 2/0, State_Error.cpp 3/1). Within +20 (round 1 was +13; +3 now).

## F2 (tests only)
`sleep_execution_structural_test.py` and `sleep_config_ownership_structural_test.py` now use a new `in_function_body()` (brace stack; `namespace`/`extern "C"` blocks do not count as function bodies). The file-level helper is accepted; a namespace-scope `SystemSleepConfiguration` is rejected by both.

## F3 (tests only)
`s3_timed_power_off_test.cpp`: the fake now has a distinct `kPowerCut` mode (System.sleep never returns) versus `SYSTEM_ERROR_NONE` (returns success), `ABORTED`, `NOT_SUPPORTED`. It also fakes `ExternalWatchdog` (`wd.stop`/`wd.start`). The helper, stage 3 and ERROR_STATE cases assert: power cut gives stop then sleep and nothing else; every return gives stop, sleep, start, then the caller's after-return block and fallback reset (including the returned-success case).

## Mutations (on scratch copies of `src/`; the real tree was never mutated, so no restore was needed)
- stop removed: caught (`s3_timed_power_off_test.sh` exit 1, 13 FAIL lines).
- start removed: caught (exit 1, 10 FAIL lines).
- namespace-scope `SystemSleepConfiguration` added (F2): caught by both structural tests.

## Verification
- CI discovery in the worktree: **121/121 (sh via zsh, py via python3)**. A first run gave 120/121 because my scratch `src/` copies under `build-tmp/` tripped `thermal_coupling_structural_test.py`; I deleted them and it passes. It also passed on a clean export of tracked files. No new helper was added under `tests/`.
- Local toolchain build (boron, Device OS 6.6.0, fresh `BUILD_PATH_BASE` as a make argument): rc 0, text/data/bss **150924 / 1114 / 2240** (round 1: 150908; +16 text).
- `nm -C app.elf`: `000bdf1c T powerOffForRecovery()`; no `deepPowerDown` symbol.

## Deviations
None from scope. Mutations used scratch copies instead of back-up/restore of the real files. Scratch: `build-tmp/wo-2026-11-05-005-stage6-r2/` holds only `m_stop.log`, `m_start.log`, `build.log`-less leftovers; build trees were removed.
