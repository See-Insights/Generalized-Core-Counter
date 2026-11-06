37ea3a21196044341cdabee7831aff065f5e0bd7

VERDICT: **PASS**

BLOCKING FINDINGS: **None.**

### B1 — Resolved

The guard resets only when all three conditions hold: `r == 0`, the second `bound()` read is false, and the current boot is not USER/data 8 (`src/Generalized-Core-Counter.cpp:303-308`).

Device OS 6.6.0 confirms the second read is fresh:

- `hal_exrtc_unbind()` calls `end()` and deletes `EXRTC_CONFIG_DATA`, but ignores both results and returns zero (`hal/shared/exrtc_hal.cpp:298-305`).
- `end()` always marks the native instance absent (`hal/shared/am18x5.cpp:529-557`).
- `hal_exrtc_get_status()` reloads the binding through `loadCurrentBinding()` and sets `BOUND` only when that load succeeds (`hal/shared/exrtc_hal.cpp:225-234,307-335`).
- Therefore, a valid surviving cache entry is observed as bound. A partial cache failure or status error is not equivalent: `RtcStatus(error).bound()` is false (`wiring/inc/spark_wiring_exrtc.h:403-405,452-454,465-489`).

Sequence proof:

1. Delete works: one reset with data 8; next boot is unbound and does nothing.
2. Delete fails and the valid entry remains: second read is bound; no reset.
3. Guard-caused second boot: `ours` is true; whether the retry fails or succeeds, there is no second reset. A surviving entry logs the diagnostic.
4. Delete works on a later ordinary boot: that new cause permits one reset; its data-8 successor cannot reset again.
5. Another reset reason between attempts: it starts a new cause and permits at most one new guard reset.
6. Status error: an initial error skips the guard. A post-disable error reads false and may cause one reset while the binding survives, but the following data-8 boot suppresses another.
7. Lost reset metadata: a clean software reset preserves USER/data 8 (`hal/src/nRF52840/core_hal.c:549-557,651-661`); hardware resets replace it with their cause and data 0 (`:662-690`). Thus there is no absolute lifetime or strict per-power-on bound if recurring watchdog/power events continually erase data 8. Each external cause can permit another guard reset. This is not the self-sustaining, ordinary failed-delete loop from round 1.

### NOTE N1 — Hardware-watchdog residue

`Am18x5::end()` attempts a chip software reset, but `writeRegister()` ignores the I2C transmission result (`hal/shared/am18x5.cpp:529-569,1331-1347`). An already armed external watchdog can therefore survive.

The vendored driver initializes at `src/Generalized-Core-Counter.cpp:667`, but its first watchdog reprogramming is at `:809`. If the inherited remaining timeout repeatedly expires before line 809, it can still prevent boot progress. The binding guard itself no longer loops once the cache entry is gone, but the separate physical-watchdog loop is not source-proven bounded.

This remains a **non-blocking note**: it requires an independent silent I2C failure plus an armed watchdog and is covered by the deferred binding-present/rollback v50 pre-ship exercise.

### NOTE N2 — Regression and bookkeeping

- Unbound path: one SystemCache lookup and no I2C; the `TimeClass` singleton constructor only initializes fields (`wiring/src/spark_wiring_time.cpp:84-87`; `hal/shared/exrtc_hal.cpp:307-335`).
- USER reason 140 remains excluded from boot-storm qualification (`src/Generalized-Core-Counter.cpp:346-363`).
- `resetCount` still increments by one on the data-8 boot (`:621-628`) and can move alert 16 to hard recovery (`src/state/State_Error.cpp:81-93`). Accepted note.
- Cause 8 is unique and documented (`src/ResetCause.h:23`).
- Placement is immediately after the debug-serial wait and before all Time/Wire/native-driver use.
- `src/` delta is exactly `Generalized-Core-Counter.cpp +9`, `ResetCause.h +1`: **+10 net**, with no other branch source changes.
- `docs/recovery-architecture.md:85` correctly documents the no-flush exception and per-cause bound.

### Tests and linkage — PASS

- The fake models the real contract: `disable()` can return zero while deletion fails (`tests/binding_delete_guard_test.sh:106-110`).
- Cases (a)–(e), c2 and c3 pass (`:152-196`).
- Ten mutations were independently caught: guard moved after `ab1805.setup()`, unconditional disable, guard removed, reset on `r != 0`, wrong cause, warning removed, error return, re-check removed, `ours` removed, survived message removed.
- Clean tracked-files export: **114/114** passed—65 zsh tests and 49 Python tests, counted with `p=$((p+1))`. Thermal-coupling structural test passed.
- Fresh Device OS 6.6.0 Boron build: text/data/bss **152204 / 1114 / 2216**.
- ELF `setup` linkage:
  - initial status wrapper at `0xb5d94`, calling `hal_exrtc_get_status` at `0xb543c`;
  - `hal_exrtc_unbind` at `0xb5da6`;
  - second status read at `0xb5de4`;
  - immediate value 8 at `0xb5df8`;
  - `SystemClass::reset` at `0xb5dfa`;
  - survived-path status read at `0xb5faa`.
- Both warning strings are present.
- `git diff --check` passed.

### NOTE N3 — Bench and moving base

No Stage 6/7 record claims the binding-present or rollback hardware cases passed. The work order explicitly defers them to the v50 pre-ship checklist (`docs/work-orders/WO-2026-11-04-002-v49-binding-delete-guard.md:48,66`). The unbound control remains a separate v49.1 release gate and is not claimed here.

`origin/main` advanced externally during review from merge-base `824f78a` to `136e099`. The review was anchored to `824f78a`, reproducing the initial dispatched six-file diff; the newer main-only release-upload test change is not part of this SHA.

Model: **gpt-5.6-sol**  
Reasoning level: **xhigh**  
Interpreter/toolchain: **zsh 5.9; Python 3.14.6; Apple clang 14.0.3; arm-none-eabi-gcc 10.2.1 (2020-q4-major); Device OS 6.6.0**