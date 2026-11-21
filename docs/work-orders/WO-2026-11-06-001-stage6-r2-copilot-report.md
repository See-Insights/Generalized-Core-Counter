# WO-2026-11-06-001 (S4) Stage 6 round 2: Copilot report

Model: claude-sonnet-5.5. Reasoning level: dispatched as medium; the level actually run could not be observed from inside the session.
Interpreter: Python 3.14.6; zsh 5.9 (arm64-apple-darwin26.0); GNU Arm Embedded 10.2.1, Device OS 6.6.0, Boron, buildscripts 1.17.2.
Base: HEAD 4d8af80 (S4 code at 6a6a57d); changes are uncommitted.

## Changes
- `src/time/ExternalRtc.h`: `#include <atomic>`; the latch is `std::atomic<bool>` (function-local static, `{false}`); the callback stores with `std::memory_order_relaxed`.
- Tests: host test uses `auto&`, asserts the atomic type, and adds the combined-mask vector (`CALIBRATION_FAILURE | CLOCK_SOURCE_EXTERNAL_FAILURE` sets the latch from a clean latch); stub gains `operator|` for `RtcEvent`/`RtcEvents`; structural test requires atomic storage and the relaxed store, keeps the order test (first read -> initial set -> registration; no second read required).
- Net `src/` against e21a4a4: **+8** (`git diff --numstat`: 2 + 6). Budget +10 holds. Nothing outside `ExternalRtc.h` was needed.

## Tests
- Suite via CI discovery in this worktree: **121/121 (sh via zsh, py via python3)**; `external_rtc_structural_test.py` exits 0 bare.
- `thermal_coupling_structural_test.py` passes on a clean `git archive HEAD` export (scratch removed).
- Mutations (applied in memory to copies of the real source, so no source backup or restore was needed; no `git checkout`/`restore` used). All caught: latch initialised true, calibration failure also sets the latch, plain `bool`, `volatile bool`, plus the existing latch mutations (event ignored, wrong enum, cleared by a later event, registration-time check removed, cleared by later read, callback before bind / before the read, registered twice, persisted).

## Build and linkage
- Fresh `BUILD_PATH_BASE` as a make argument: rc 0, text/data/bss **151092 / 1114 / 2256** (round 1: 151084 / 1114 / 2256).
- `nm -C`: `000b53cc W ExternalRtc::onOscillatorEvent(...)`; `2003d728 V ExternalRtc::oscillatorFaultSeenSinceBind()::seen`.

## Deviations
- `Generalized-Core-Counter.cpp` (not authorized) still sets the latch with `= true`, which on `std::atomic<bool>` is a sequentially consistent store (stronger than relaxed, correct). The relaxed store is used in the header callback only. Switching the registration-time site to `.store(true, relaxed)` would need a `src/` edit outside `ExternalRtc.h`; the structural test pins the existing `= true` form there.

No claims about hardware.
