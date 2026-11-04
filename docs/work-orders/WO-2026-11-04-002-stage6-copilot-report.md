# WO-2026-11-04-002 Stage 6 report (v49 binding-delete guard)

Agent: Copilot CLI 1.0.105. **Model: claude-sonnet-5.5; reasoning: not exposed to the agent (dispatch asked for medium).** Interpreters: zsh (`.sh`), Python 3.14.6 (`.py`), clang++ -std=c++17. Nothing committed, pushed, flashed or tagged. Dev-14 bench is Chip's later step; nothing here claims it.

## Step 0 (Device OS 6.6.0, `~/.particle/toolchains/deviceOS/6.6.0`)

**Q1: is the chip free for `ab1805.setup()` in the same boot after `disable()`?** Source says mostly yes; not provable.
- `ExternalTime.disable()` = `hal_exrtc_unbind(instance_)` (`wiring/inc/spark_wiring_exrtc.h:477-479`). `hal_exrtc_unbind` (`hal/shared/exrtc_hal.cpp:298-304`) calls `Am18x5::end()`, then deletes `SystemCacheKey::EXRTC_CONFIG_DATA`, and **always returns 0** (errors from `end()` are ignored).
- `Am18x5::end()` (`hal/shared/am18x5.cpp:529-558`): WDI pin (D5) → `INPUT`; if an INT pin is set: joins/destroys the `exrtc` worker thread and semaphore, `hal_interrupt_detach`, INT pin (WKP/D8) → `INPUT`; if initialized: `reset()` (`:560-570`), a **chip software reset** (CONFIG_KEY) plus `HAL_Delay_Milliseconds(100)`; `initialized_ = false` (SCOPE_GUARD), so `isPresent()` is false (`am18x5.h:210`).
- I2C is **not** released: `begin()` only inits/begins it if not yet enabled (`am18x5.cpp:490-497`), `end()` never calls `hal_i2c_end`. Wire stays enabled (shared bus; fine for `AB1805_RK`).
- Device OS-side time source: `hal_rtc_init` (`hal/src/nRF52840/rtc_hal.cpp:77-86`) seeds the internal clock from the chip once at boot. `hal_rtc_get_time` (`:93-114`) reads the chip only while `hal_exrtc_is_default()` (= `initialized_ && default_rtc`, `am18x5.cpp:477-479`); after `end()` it falls back to the internal clock. So after the delete, `Time` continues from the internal clock (no step; it drifts the way an unbound device's does).
- Nothing else in `hal_exrtc_*` keeps using the chip after `end()` (all of `get_time_internal`, `set_time_internal`, alarms gate on `isPresent()`, `exrtc_hal.cpp:~405-435`). `hal_exrtc_init()` runs only from `hal_rtc_init` (`rtc_hal.cpp:80`; rtl872x `:331`), i.e. not again this boot.
- **What the source cannot show:** the chip state after the software reset (oscillator/config registers back to power-on defaults, whether the time counters survive, the 100 ms delay) as seen by `AB1805_RK::setup()` in the same boot. The Dev-14 spike only observed the state after a reboot.

**Decision: (B).** Delete, then one `System.reset(RESET_CAUSE_EXRTC_BINDING_DELETE)` when `r == 0`. Reason: the source gives a plausible same-boot path but cannot prove that the vendored library sees a clean chip after `end()`'s software reset, and the dispatch says to choose (B) in that case. The next boot finds nothing bound (the delete is persisted), so it cannot loop.

**Q2: placement.** `setup()` starts with `validateRetainedBoot` (`persist/RetainedBoot.h:14-18`, plain struct) and `waitForDebugSerialIfConnected()` (`Generalized-Core-Counter.cpp:279-283`, only `Serial.isConnected()` and `delay`). `Log` works there: the global `SerialLogHandler` is constructed before `setup()` (`:274-277`). The guard is now the next statement after `waitForDebugSerialIfConnected()` (`:301-306`). Nothing earlier reads or sets Time or touches `Wire`. Before it: no `Time.*`; after it: `ensureRetainedLoopForensicsInitialized`, then the boot-storm block (`Clock::isTrusted()`/`Time.now()`), `Time.isValid()` and `ab1805.withFOUT(WKP).setup()`. (The WO's "future-date clamp at ~370-400" is now the Clock-gated storm window; the occupancy clamp re-run is after `ab1805.setup()`; the structural test checks all of them.)

**Q3: cost when unbound.** `hal_exrtc_get_status` (`exrtc_hal.cpp:~340-370`): `loadCurrentBinding` reads `SystemCache` (stored key, then `hal_exrtc_default_binding()`, weak, null on Boron `:437-439`); with the chip not present it does not touch I2C. No delay. It only copies a cached/DCT-backed record. Not measured on hardware.

**Q4: compile surface.** `ExternalTime` is declared under `#if HAL_PLATFORM_EXTERNAL_RTC` (`spark_wiring_exrtc.h:22,653`). Defined 1 for boron, argon, b5som, tracker, msom, tron, electron2 (`hal/src/*/hal_platform_config.h`); default 0 (`hal/inc/hal_platform.h:405`), so **P2/Photon 2 lacks it**. The repo builds only Boron (`project.properties`, `.github/workflows/build.yml:85`); README:25,190 says P2 and Argon do not currently build (PMIC errors). No `#if` added; a P2 build would need one, and that build is already broken.

**Q5: time.** Stored binding with `USE_AS_MAIN_RTC`: Time was read from the chip. After `disable()` Time falls back to the internal clock seeded from the chip at `hal_rtc_init`, so it is valid and continuous. With (B) it is moot: the reset happens before any Time use, and the next boot sees no binding.

## Implementation

`git diff --numstat -- src`: `Generalized-Core-Counter.cpp` +7/−0, `ResetCause.h` +1/−0 = **+8 net** (budget +10). Guard (one comment line, 5 code lines + closing brace); `RESET_CAUSE_EXRTC_BINDING_DELETE = 8` with the "withdrawn planned-reset design used 8" comment. No status/ledger field, no persisted/retained change (retained layout, fixtures untouched: `git status` shows only the files below).

Files: `src/Generalized-Core-Counter.cpp`, `src/ResetCause.h`, `tests/reset_cause_structural_test.py` (edited), `tests/binding_delete_guard_test.sh`, `tests/binding_delete_guard_order_test.py` (new).

## Boot-storm exclusion (B)

- `qualifiesForStormCount` is true only for NONE, PANIC, WATCHDOG, UNKNOWN, USER_APPLICATION (`Generalized-Core-Counter.cpp` switch after `bool qualifiesForStormCount = false;`); `RESET_REASON_USER` (140) is not in it. The count increment and the trip both require `qualifiesForStormCount` (the two `if` blocks that follow, and the trip condition `bootStormCount >= 6 && previousBootFailedEarly && qualifiesForStormCount`). The guard sits before `bootInProgress = true`, so the reset leaves `bootInProgress` exactly as the previous boot left it; even if it were true, the next boot does not count, trip, set `bootStormAlertPending`, or log the "Previous boot reset early" warning.
- Pinned by `binding_delete_guard_test.sh` (real switch/count/trip blocks extracted; reason 140 data 8 with `bootInProgress` true and count 5 stays 5, no trip, no alert; control: a watchdog reset in the same state reaches 6 and trips).
- **Finding, not changed:** the reason-140 boot is counted by `RecoveryState::set_resetCount(+1)` (`Generalized-Core-Counter.cpp` "Track how often the device has been resetting" switch: PIN_RESET/USER/WATCHDOG/USER_APPLICATION). `resetCount` feeds `resolveErrorAction()` (`state/State_Error.cpp:34-47`: ≥2 escalates to hard power-cycle, ≥4 suppresses resets for alerts 15/31/44). A one-time +1 per stored binding only moves that ladder by one step for a device already in a connectivity alert; it trips nothing, and every other firmware reset code does the same. Excluding it would need a change outside the guard; left for the architect (open question 1).

## Tests

- `tests/binding_delete_guard_test.sh` (zsh): fake `ExternalTime` driving the **real guard block** extracted from `setup()`. Cases: bound/r=0 → `disable()` once, one warning, one reset with 8 (also checked against the real header value); unbound → no call, no warning, no reset, setup continues; r=−1 → disable once, warning, **no reset, setup continues**; plus the boot-storm pin above.
- `tests/binding_delete_guard_order_test.py` (also run by the sh test on every mutant): in `setup()` (comments stripped) the guard precedes `Time.isValid()`, `Time.now()`, `Clock::isTrusted()`, `Wire.` and `ab1805.withFOUT(WKP).setup()`.
- `tests/reset_cause_structural_test.py`: site count 7→8; requires `RESET_CAUSE_EXRTC_BINDING_DELETE = 8` used by a site; every enum value has a `///<` description; existing distinct/non-zero/documented checks apply to code 8.
- Mutations, all caught for the intended reason (checked by output): guard after `ab1805.setup()` (order test); `disable()` unconditional (`|| true`; "unbound: the guard acted"); guard removed (extraction/order fails); reset when `r != 0` ("expected one reset with 8"); different cause (OUT_OF_MEMORY; same check); warning removed ("expected exactly one warning"); `else return;` on the error path ("error: setup() did not continue").

## Verification

- Host suite: **114/114** (sh via zsh, py via python3; 64 `.sh` + 48 `.py` existing = 112 before, plus the new `.sh`, plus the new order `.py` = 114). In the working tree `thermal_coupling_structural_test.py` fails only because it scans my scratch source copies under `build-tmp/wo-2026-11-04-002-stage6/{app,base}`; it passes (rc 0) on a clean export of the tracked files (`git ls-files` minus `build-tmp/`). I could not delete the scratch copies (`rm -rf` was refused by the sandbox), so **delete `build-tmp/wo-2026-11-04-002-stage6/{app,base,clean,obj,objbase}` before running the suite in-tree.**
- Local build: Device OS 6.6.0, Boron, gcc-arm 10.2.1, buildtools 1.1.1, fresh `BUILD_PATH_BASE`, `-frandom-seed=$$@`. Baseline (`git archive HEAD`) text/data/bss **151732 / 1114 / 2200**; with the guard **151988 / 1114 / 2216** (+256 text, +16 bss).
- `nm -C -S` diff vs baseline: new `hal_exrtc_unbind` and `hal_exrtc_get_status` thunks (12 B each), `particle::ExRtc::instance()` (0x38) + its static `x` and guard variable, two `TimeClass::TimeClass(TimeSource)` (0x10 each), `setup` 0xc2c → 0xc90 (+100 B). This is the `ExternalTime` singleton being instantiated for the first time (the +16 bss), not growth in app logic.
- `objdump -d setup`: `hal_exrtc_get_status` (bit 1 of status = bound) → `hal_exrtc_unbind` → `Logger::warn` → `cbnz r4` → `movs r0,#8; bl SystemClass::reset(unsigned long, …)`, then `ensureRetainedLoopForensicsInitialized`. The warning string is present in the ELF.

## Deviations

- Guard comment: the "Only comment what needs it" rule was followed with one line.
- Scratch dirs not removed (permission refused); see Verification.
- Test count includes a small helper `.py` that is also invoked by the `.sh`.

## Open questions

1. `resetCount` +1 on the reason-140 boot (above): accept (my recommendation, one-time and per binding) or exclude?
2. The binding-present same-boot behaviour (option A) remains unobserved; v50 pre-ship checklist per Amendment 1.4.
3. `unbind` never returns non-zero, so the `r != 0` branch is effectively unreachable on 6.6.0; kept as specified.
