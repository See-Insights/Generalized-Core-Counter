# WO-2026-11-05-005 (S3) Stage 6 report: timed power-off

**Model:** claude-sonnet-5.5. **Reasoning level:** dispatched as medium; the level actually run could not be observed from inside the session.
**Interpreter:** Python 3.14.6 (`python3`), zsh for `*.sh`, clang++ -std=c++17 for host harnesses.
**State:** uncommitted on the integration head 62e3dbc (+ dispatch commit 67134b6). Nothing committed, pushed, flashed or merged. No hardware claim is made.

## Step 0 (read-only; Device OS 6.6.0 at `~/.particle/toolchains/deviceOS/6.6.0`). Nothing breaks the plan.

1. **`POWER_OFF` on 6.6.0.**
   - Needs the AM18x5 (`HAL_PLATFORM_EXTERNAL_RTC`=1 on boron, `hal/src/boron/hal_platform_config.h:103`; b5som `:49`) and, because it is "optional", the chip must report READY or the RTC wake source returns `NOT_SUPPORTED` (`hal/src/nRF52840/sleep_hal.cpp:~758-775`, `validateRtcWakeupSource`).
   - `duration()` is **milliseconds**; the HAL divides by 1000 (`sleep_hal.cpp:~1348-1352`: `sleep.duration = rtc->ms / 1000`, `>0` else `INVALID_ARGUMENT`). Minimum 1 s; 0 is `INVALID_ARGUMENT`. Only one RTC source is accepted.
   - Chip sequence (`hal/shared/am18x5.cpp`, `Am18x5::sleep`, `:991-1137`): `CONFIG_KEY_PRIMARY` 0x9D at `:1009` and again `:1038` (BATMODE_IO / OUTPUT_CTRL), OUT2S=6 and PWR2 at `:1047-1048`, then **`CONFIG_KEY_OSC_CONTROL` 0xA1 at `:1050`, PWGT=1 at `:1051`** (key values `am18x5_defines.h:180-181`), alarm armed from `duration` (`:~1078`), SLP written (`:1118`).
   - Returns (`ABORTED`) when: the SLP bit is not set after the write, i.e. a trigger or interrupt was pending (`:1129-1131`); EXTI already active, only if an EXTI wake and the check flag are set (`:1107-1113`, not our case: no GPIO); or, after SLP was set, if the chip never actually slept (`:1137`). Other errors return from `CHECK()` (I2C failure, `INVALID_STATE` for XTAL without RC fallback `:1000-1005`, `NOT_SUPPORTED` for unsupported sources).
   - 6.5.0 race fix (#2927) is in the 6.6.0 CHANGELOG (`CHANGELOG.md:91`); the code carries the single-threaded critical section around the SLP write (`:1100-1118`).
   - **USB:** I found no USB/battery check anywhere in this HAL path. Whether the rail collapses with USB attached is a hardware question I cannot answer from source; the vendored call used the same AM1805 PWR/nIRQ2 path, and P7 recorded the LED dark and the port gone on USB (spike WO, P7 entry). No change in expectation, but the bench should confirm the native path does the same.
2. **Wake sources.** `.gpio()` is accepted only for the RTC pin type (`HAL_PIN_TYPE_RTC`); any normal pin returns `NOT_SUPPORTED` (`sleep_hal.cpp:~737-750`, `:~1348`). The helper arms **RTC duration only, no `.gpio()`**.
3. **Vendored baseline** (`lib/AB1805_RK/src/AB1805_RK.cpp:496-`, `.h:325`): `deepPowerDown(int seconds = 30)`; both callers used the default, so **today's off time is 30 s** (P7 was run with an explicit 120 s). Sequence: `setWDT(0)`, countdown timer, PWR2, PWGT, OUT2S=sleep, `SLP|SLRES`. On any I2C failure it logs and **returns false** (the callers ignored it and fell into their after-return blocks). If the chip does not cut power it logs for `seconds`, then calls `System.reset()` **with no cause** and returns. The native path is at least as good: errors are returned and logged, and the fallback reset now names its cause. The constant is `kPowerOffRecoverySec = 30` (header). **Open:** 30 s vs the 120 s P7 observation; I kept the shipped value.
   - Persisted vs `retained`: the failsafe needs `connectivityRecoveryStage` / `lastConnectivityRecoveryAction` (persisted via `RecoveryState`, `ConnectivityFailsafe.cpp:77-79`) and the Error path's sentinel byte (`set_powerOffRequested`, `State_Error.cpp:178`, spare `reservedUint8_0`, `MyPersistentData.cpp:1068`). All persisted, none `retained`; no layout change.
4. **Callers/tests (bare-name search `deepPowerDown|deep-powerdown|deep power`).** Callers: `ConnectivityFailsafe.cpp` (stage 3) and `State_Error.cpp` (case 3) only. Tests mentioning it: `connectivity_failsafe_unknown_openness_test.cpp`, `connectivity_failsafe_stage1_retired_test.py`, `deliberate_reset_flush_test.py`, `modem_teardown_confirmation_logging_test.py`, `s2_alarm_watchdog_wake_structural_test.py`, `watchdog_inferred_classification_test.cpp` (comment only, untouched). Fleet log strings kept verbatim: `Failsafe: stage=3 action=deep-powerdown age=%lds` (`ConnectivityFailsafe.cpp:321`), `Executing deep power down from ERROR_STATE (alert=%d)` (`State_Error.cpp:177`).
5. **Way back.** `Generalized-Core-Counter.cpp:752-761`: on `RESET_REASON_PIN_RESET`, `isInferredWatchdog(sentinel, stage, lastAction, timeValid, now)` (`HibernateWakeDiagnostics.h:169-174`); the byte is cleared and flushed at `:761-762`; the watchdog re-armed at `:765`. Counted as a watchdog only if `inferredWatchdog` (`:778`), so a power-off recovery with evidence is not counted. Caveat for the architect: the **stage-3 evidence requires `timeValid`** at that check; P7 saw the RTC invalid at boot after a power-down (that was the older, unbound-chip run). If the clock is invalid at that point a stage-3 power-off is classified as an inferred watchdog. I did not touch S2 (out of scope); the bench's "RTC state after the wake" record settles it. The post-`POWER_OFF` reset reason stays unobserved (parent ruling 5).
   - Per caller: Error path field = sentinel byte (`State_Error.cpp:178`, written before `flushAllBeforeReset()` at `:179`); stage 3 field = persisted stage 3 + action time (`persistConnectivityFailsafeState(3, …)`, `ConnectivityFailsafe.cpp:~320`, before the flush at `:~323`).
6. **Count.** See below; within +20.

**Amendment 3 item 3 (watchdog).** (a) The app need not stop `ExternalWatchdog`: `Am18x5::sleep` disables the chip WDT itself when it is running (`am18x5.cpp:1053-1056`) and restores it in the scope guard if the call returns (`:1062-1072`); on success the chip is power-cycled. (b) 30 s off < 124 s window, and the first boot re-inits and starts the watchdog (`Generalized-Core-Counter.cpp:765`). No stop was added.
**TEST_MODE hook:** the #151 hook (`ConnectivityPolicy.h` test block) only shortens thresholds/cooldowns; unchanged. The helper's duration is always the real one.

## Change (net `src/` vs 62e3dbc: `git diff --numstat 62e3dbc -- src`)

| file | +/- |
|---|---|
| `src/power/ConnectivityFailsafe.cpp` | +10 / -1 (helper `powerOffForRecovery()`, caller after-return reset) |
| `src/power/ConnectivityFailsafe.h` | +2 / -0 (`kPowerOffRecoverySec`, declaration) |
| `src/state/State_Error.cpp` | +3 / -1 (include, helper call, fallback reset) |

**Net +13 (budget +20).** Both callers keep their log lines, `flushAllBeforeReset()` and S2 after-return blocks verbatim, then `System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE)` / `RESET_CAUSE_ERROR_STATE_SOFT`. The helper never resets. No `deepPowerDown`/`ab1805.` remains in `src/` code (the `ab1805` object definition is kept).

## Tests
- New `tests/s3_timed_power_off_test.{sh,cpp}`: extracts the REAL helper, supervisor body and State_Error `case 3:` block; ordered-event fakes. Checks, for success / `ABORTED` / `NOT_SUPPORTED`: POWER_OFF, `kPowerOffRecoverySec*1000` ms, zero GPIO wakes; stage 3 and Error path each reach the helper once, after the flush; on a return, helper log, evidence clear (+ stage 2 for failsafe), flush, then `System.reset(cause)`; verbatim log lines; also fails on any `deepPowerDown`/`ab1805.` left in `src/` code. `S3_SRC` lets it run on a copy of `src/`.
- **Mutations** (run on copies of `src/` in `build-tmp/wo-2026-11-05-005-stage6/`, the real tree never mutated, so no restore was needed): flush after the sleep (both callers), fallback removed (both), GPIO wake added (checked: fails on behaviour), duration 0 (call and constant), wrong cause (both), reset before the evidence clear (both), helper resetting inside, AB1805 call back: **13/13 caught**.
- Forced updates to existing tests (expectations otherwise unchanged): `connectivity_failsafe_unknown_openness_test.cpp` (fake helper replaces the AB1805 fake; ladder/cooldown cases untouched), `deliberate_reset_flush_test.py`, `modem_teardown_confirmation_logging_test.py`, `connectivity_failsafe_stage1_retired_test.py`, `s2_alarm_watchdog_wake_structural_test.py` (call renamed; after-return regexes now also require the fallback reset; no `ab1805.` allowance), `reset_cause_structural_test.py` (9 sites; the two S3 fallbacks may reuse their own caller's cause), `sleep_execution_structural_test.py` (6 fresh configs; depth check relaxed to "inside a function"), `sleep_config_ownership_structural_test.py` (file-scope check measured at the end of the declaration; its old form misfired when a declaration directly follows a function's `{`).
- **Suite (CI discovery): 121/121 (sh via zsh, py via python3)** (baseline 120 + the new test). `thermal_coupling_structural_test.py` also passes on a clean export of tracked files (it fails if a copy of the tree is left under `build-tmp/`, which I removed).

## Build (Device OS 6.6.0, boron, fresh `BUILD_PATH_BASE`)
text/data/bss = **150908 / 1114 / 2240** (head: 151508 / 1114 / 2240; -600 text). `nm -C app.elf`: `000bdf1c T powerOffForRecovery()`; no `deepPowerDown` symbol.

## Deviations
- The WO's "remove `#include "AB1805_RK.h"` at `State_Error.cpp`" did not apply: that file has no direct include (it reached `ab1805` via `StateMachine.h`). Nothing removed; `ConnectivityFailsafe.cpp` still includes `state/StateMachine.h` for other uses.
- Off time kept at 30 s (the default both callers used), not P7's 120 s.
- Scratch left in `build-tmp/wo-2026-11-05-005-stage6/`: `build.log`, `mut.py`.

## Open questions
1. 30 s vs 120 s off time.
2. Stage-3 evidence needs a valid clock at the reason-20 check (see Step 0 item 5); confirm on the bench, else S2's rule may need the failsafe action time without the `timeValid` gate.
3. Post-`POWER_OFF` reset reason (20 assumed) and USB-attached behaviour: bench only (Chip, from 14 Nov).
