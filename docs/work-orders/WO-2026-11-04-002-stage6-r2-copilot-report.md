# WO-2026-11-04-002 Stage 6 round 2 report (binding-delete guard, fixes Stage 7 r1 B1)

Agent: Copilot CLI 1.0.105. **Model: claude-sonnet-5.5; reasoning level not exposed to the agent (dispatch asked for medium).** Interpreters: zsh (`.sh`), Python 3.14.6 (`.py`), clang++ -std=c++17; arm-none-eabi-gcc 10.2.1. Nothing committed, pushed, flashed or tagged. Base `4e0b8d5` + Amendment 2 record (`76343db`).

## Fix (guard block only)

```
const int r = ExternalTime.disable();
Log.warn("AB1805: stored Device OS binding deleted (r=%d)", r);
const bool ours = System.resetReason() == RESET_REASON_USER && System.resetReasonData() == RESET_CAUSE_EXRTC_BINDING_DELETE;
if (r == 0 && !ExternalTime.status().bound() && !ours) System.reset(RESET_CAUSE_EXRTC_BINDING_DELETE);
else if (ours && ExternalTime.status().bound()) Log.warn("AB1805: binding survived the delete");
```

Reset only if `r == 0`, the re-read shows unbound, and this boot was not caused by the guard. A guard-caused boot with the chip still bound logs the survived message and continues.

## Device OS 6.6.0 evidence (`~/.particle/toolchains/deviceOS/6.6.0`)

- `ExternalTime.status()` → `hal_exrtc_get_status` each call, no caching (`wiring/inc/spark_wiring_exrtc.h:481-490`); `bound()` = `HAL_EXRTC_STATUS_BOUND` bit (`:452-454`).
- `hal_exrtc_get_status` (`hal/shared/exrtc_hal.cpp:307-336`): `loadCurrentBinding` (`:225-234`) reads `SystemCacheKey::EXRTC_CONFIG_DATA` (`:227`, `load` `:107-115`); `BOUND` is set when that load succeeded (`:328-330`). After `disable()`, `Am18x5` is not present (`end()`), so the else branch (`:320-326`) is used: no I2C.
- `hal_exrtc_unbind` ignores `end()`/`del()` errors and returns 0 (`:298-305`); so a failed delete is visible only through the cache entry, which the re-read checks. Boron has no platform default binding (`hal_exrtc_default_binding()` null), so nothing masks the result. Conclusion: the dispatch's premise holds; no stop needed.
- Edge: if `get_status` itself errors, `bound()` is false (`RtcStatus(r)`), which would be read as "unbound"; not reachable from a SystemCache lookup in practice (stated, not changed).

## src/ budget

`git diff 4e0b8d5~1 --numstat -- src`: `Generalized-Core-Counter.cpp` +9, `ResetCause.h` +1 = **+10 net** (at the budget). Round-1 was +8; the fix adds 2 net lines.

## Tests (`tests/binding_delete_guard_test.sh`, real extracted block)

Fake models the real contract (`disable()` returns 0 even when the delete failed; a `deleteWorks` flag decides whether `bound` clears). Cases: (a) delete worked → one reset with 8; (b) `disable()`=0, binding survives → no reset, continues, one warning; (c) reason 140 / data 8, still bound → no reset, continues, survived message logged; (c2) same boot, delete worked this time → no reset; (c3) reason 140 with other data and delete worked → one reset; (d) unbound → nothing; (e) `disable()` error (deleted or surviving) → no reset, continues. Boot-storm pin and its control kept.

Mutations, all caught: guard after `ab1805.setup()`; unconditional `disable()`; guard removed; reset when `r != 0`; different reset cause; delete warning removed; error path returns; **re-check dropped; "caused by this guard" dropped; survived message removed**. (`no-ours` was first NOT caught; c2 was added to pin it, since condition 1 alone already covers the surviving case.)

## Verification

- Host suite: **113/114 in-tree** (sh via zsh, py via python3); the one failure is `thermal_coupling_structural_test.py`, which scans the pre-existing scratch source copies under `build-tmp/wo-2026-11-04-002-stage6/` and `-stage7-r1/` (not mine; I may not delete them). On a clean copy without `build-tmp/` (`/tmp/r2export-51490`) it passes (rc 0), so **114/114 effective**. My own scratch copy `build-tmp/wo-2026-11-04-002-stage6-r2/app` also sits in `build-tmp/` and will trip that test in-tree: remove it (`rm -rf build-tmp/wo-2026-11-04-002-stage6-r2/app`; deletion of my files was refused by the sandbox earlier, so it is left).
- Build: Device OS 6.6.0 Boron, fresh `BUILD_PATH_BASE`, `-frandom-seed=$$@`: text/data/bss **152204 / 1114 / 2216** (round 1: 151988; +216 text for the new logic, bss unchanged).
- `objdump -d setup` (`0xb5d50`): `ExRtc::instance` + `ExRtcBase::status` (calls `hal_exrtc_get_status`, `0xb543c`) → `ExRtc::instance` → `hal_exrtc_unbind` (`0xb5da6`) → `Logger::warn` → `resetReason`/`HAL_Core_Get_Last_Reset_Info` → `ExRtc::instance` + `status` (second `hal_exrtc_get_status`, `0xb5de4`) → `movs r0,#8; SystemClass::reset` (`0xb5df8`) → `ensureRetainedLoopForensicsInitialized` (`0xb5dfe`). The survived-message and the second `bound()` read of the `else if` are in the cold path after the reset branch.
- Layouts: `git status` shows only `src/Generalized-Core-Counter.cpp`, `tests/binding_delete_guard_test.sh`, `docs/recovery-architecture.md` modified (+ this report); no retained/persisted layout, `AB1805_RK`, `lib/`, `ResetCause.h` change.
- Docs: one sentence added to "Deliberate Resets And Persistence".

## Deviations

- Scratch deletion not possible; see above. The clean-export copy lives in `/tmp/r2export-51490`.
- `ours` and `!ours` also suppress the reset for a guard-caused boot where the delete worked (literal spec); the one deliberate consequence is tested (c2).

## Open questions

1. `resetCount` +1 on the reason-140 boot (N1) unchanged, as instructed.
2. Hardware-watchdog survival after a failed `Am18x5::end()` (verdict, B1 tail) is not addressed here; the guard now bounds the loop but not that hazard.
