# WO-2026-11-06-001 (S4) Stage 6 report

**Agent:** Copilot CLI 1.0.107, model claude-sonnet-5.5. **Reasoning level:** dispatched `medium`; the level actually run could not be observed from inside the session.
**Interpreter:** Python 3.14.6, zsh 5.9 (arm64-apple-darwin26.0). **Base:** this worktree (76557c9 = e21a4a4 + the S4 dispatch docs commit; `src/` identical to e21a4a4). Changes are uncommitted.

## Step 0 (Device OS 6.6.0, `~/.particle/toolchains/deviceOS/6.6.0`)
Nothing breaks the plan. Line numbers differ slightly from the WO's quotes where noted.
1. **Facts.**
   - `HAL_EXRTC_EVENT_CLOCK_SOURCE_EXTERNAL_FAILURE = 0x02`: `hal/inc/exrtc_hal.h:88`. Wiring `RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE`: `wiring/inc/spark_wiring_exrtc.h:88`. Callback type `void(*)(RtcEvents, void*)`: `:93`; `onEvent(ExRtcOnEventCallback, void*)`: `:510`. `ExternalTime` = `ExRtc::instance()`: `:653`.
   - Event mapping XT_OSC_FAILURE -> external-failure: `hal/shared/am18x5.cpp:695-704` (the WO's `:695-715` is right).
   - OFIE enabled only with the external crystal: `:595-596` (config path), `:635-636` (`onOscillatorEvent`).
   - **Registering a handler clears the OF flag:** `addEventHandler` (`:719`) -> `updateEventHandlers` (`:761-768`) -> `onOscillatorEvent` (`:630`), which writes OSC_STATUS clearing OF|ACF at `:646`. (The init path clears it too, `:598`.) So the order read `clockSource()` first, then register, is the one that cannot lose an already-flagged fault.
   - **Context:** the event is delivered from the Am18x5 worker thread, not an ISR: `exRtcWorkerThread_` loop reads OSC_STATUS and calls `oscEventHandler_` (`:1449-1472`) -> `exRtcOscEventHandler` (`:695`) -> user handler. It is a thread, not the application loop; a single `bool` store is safe. The setter cannot run before the flag exists: the flag is a function-local static with constant initialiser, and the callback is registered only after `setup()` reaches that line.
2. **Home.** `src/time/ExternalRtc.h` (already included by `Generalized-Core-Counter.cpp:48`, host-testable via the existing stub): `ExternalRtc::oscillatorFaultSeenSinceBind()` returns `bool&` to a RAM static (process lifetime, never persisted). It is set at `setup()` (registration-time rule) before any later trust decision; I21 reads the same accessor (needs only `#include "time/ExternalRtc.h"`). `setup()` runs the set before `loop()`, so the first `isClockTrusted()` after boot sees the registration-time value.
3. **Mid-boot fault.** Nothing consumes the flag in S1-S3 (grep of `src/`: only the two new sites and the accessor). A fault flagged later only sets the boolean.
4. **Budget.** `git diff --numstat e21a4a4 -- src`: **net +7** (`Generalized-Core-Counter.cpp` +2, `time/ExternalRtc.h` +5). No log line added in the callback (callback runs on the RTC worker thread; I kept it to the store). The existing `OscState` log already shows `usingRC`.

## Change
- `ExternalRtc.h`: accessor + `onOscillatorEvent(RtcEvents, void*)` (sets the flag only for `CLOCK_SOURCE_EXTERNAL_FAILURE`).
- `setup()` (`Generalized-Core-Counter.cpp:663-664`), right after the `OscState` log: `if (startupOscUsingRC) ...oscillatorFaultSeenSinceBind() = true;` then `ExternalTime.onEvent(ExternalRtc::onOscillatorEvent);`. The existing `startupOscUsingRC` read (`:658`) is reused; no second read, no payload change.

## Tests (existing files extended; no new files)
- `tests/external_rtc_test.cpp` (real `ExternalRtc.h`): false initially; unrelated events (NONE, CALIBRATION_FAILURE) leave it false; a delivered external-failure event sets it; stays true after a second event and after a later non-fault event. Stub `tests/stubs/external_rtc/Particle.h` gained `RtcEvent`/`RtcEvents`.
- `tests/external_rtc_structural_test.py` (checks on the real `setup()` text): registered exactly once; order bind -> `clockSource()` read -> flag set from it (RC fallback => true; healthy crystal => no set) -> register; the callback body keyed on the right enum; no clear and no `set_`/`flushNow`/retained/persist/EEPROM on any line naming the latch (main, header, publisher).
- Limitation: "true after an RC-fallback bind" and "false after a normal bind" are checked structurally on `setup()` (it is not host-compilable), not by running it.
- **Mutations** (applied to copies, source never touched; all caught): `latch-event-ignored`, `latch-wrong-event-enum`, `latch-cleared-by-later-event` (header, via the C++ test); `latch-registration-time-check-removed`, `latch-cleared-by-later-read`, `latch-callback-before-bind`, `latch-callback-before-clocksource-read`, `latch-registered-twice`, `latch-persisted` (structural). All existing mutations still caught.

## Verification
- Suite (CI discovery in this worktree): baseline before editing **121/121**; after **121/121 (sh via zsh, py via python3)** (S4 adds no new test files).
- `thermal_coupling_structural_test.py` on a clean `git archive` export (with my two changed src files overlaid): rc 0. No `build-tmp/` scratch copies were present; no new helper under `tests/`.
- Build (Device OS 6.6.0, Boron, gcc-arm 10.2.1, buildscripts 1.17.2, fresh `BUILD_PATH_BASE` as make argument): rc 0. **text/data/bss 151084 / 1114 / 2256** (S3 head 150924 / 1114 / 2240: +160 / 0 / +16).
- `nm -C`: `W ExternalRtc::onOscillatorEvent(particle::EnumFlags<particle::RtcEvent, void>, void*)` at 0xb5358 (linked) and `V ExternalRtc::oscillatorFaultSeenSinceBind()::seen` at 0x2003d728.

## Deviations / open questions
- The first stub edit was made with a one-off python rewrite rather than the edit tool; content is as described.
- Scratch: `build-tmp/wo-2026-11-06-001-stage6/` holds `run.sh` and `build.log`; build output (`bpb`, `target/`) removed.
- No hardware claims. Bench confirmation (flag via `usingRC`/`OscState` on healthy crystal and RC fallback) is Chip's.
- Open: whether I21 wants a log line in the callback (budget allows ~3 lines).
