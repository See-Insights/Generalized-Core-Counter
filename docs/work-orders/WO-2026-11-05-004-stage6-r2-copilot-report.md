# WO-2026-11-05-004 (v50 S2) – Stage 6 round 2 – Copilot report

Nothing was committed, pushed, merged or released. Everything is uncommitted on `wo/2026-11-05-004-v50-s2-sleep-alarm-watchdog-wake` (base 3b5cfb8).
**No hardware claim is made.** Only host tests, a compile, and `nm` were run.

## Run details
- Model: claude-sonnet-5.5. Dispatched at medium reasoning; the actual level could not be observed.
- Interpreters: zsh 5.9 (arm64-apple-darwin26.0), Python 3.14.6, clang++ (Apple).
- Firmware build: Device OS 6.6.0, boron, buildscripts 1.17.2, gcc-arm 10.2.1. It compiled with no warnings. Size text/data/bss = 151468 / 1114 / 2240 (the baseline was 153100 / 1114 / 2240, i.e. -1632 text).

## src/ changes (net vs 3b5cfb8: **-140**, budget +20)
| file | +/- |
|---|---|
| Generalized-Core-Counter.cpp | +47/-70 |
| state/State_Sleep.cpp | +46/-58 (the hibernate block is de-indented, so its diff is inflated) |
| time/HibernateCycle.cpp / .h | +6/-48, +8/-26 |
| time/ClockDiagnostics.cpp / .h | 0/-30, 0/-2 |
| time/HibernateEligibility.cpp / .h | 0/-30, 0/-1 |
| time/HibernateWakeDiagnostics.h | +12/-2 |
| persist/RecoveryState.h, MyPersistentData.cpp | +6/-1, +3/0 |
| state/State_Error.cpp | +1/-1 |

What was done:
- **Alarm:** the hibernate config is `HIBERNATE` + `duration(wakeInSeconds*1000)` + both GPIO wakes. The AB1805 `interruptAtTime` pre-arm, the stale-interrupt clear and both bypass fallbacks are removed.
- **Chip watchdog:** `ExternalWatchdog.init(timeout 124000)` and `start()` in setup, and `refresh()` in the slot where `ab1805.loop()` was. `stop()` precedes and `start()` follows every sleep call (2 stops, 3 starts).
- **Wake classification:** `classifyWake(int)` no longer reads the AB1805. Reason 30 alone is the alarm wake, with no DPD window.
- **Reason 20 (PIN_RESET):** judged from persisted evidence only, via `isInferredWatchdog()`. `retained` is never evidence, because a power-down clears it.
  - Evidence is the new byte, or failsafe stage 3 with `lastConnectivityRecoveryAction` within 600 s.
  - The byte is `SysData::reservedUint8_0`, so there is no layout change.
  - It is written in State_Error `case 3` before `flushAllBeforeReset()` and `deepPowerDown()`, and cleared once at boot after classification.
  - `lastWatchdogSource = WATCHDOG_SOURCE_INFERRED (2)`.
- **Payload:** `ab1805WakeReason` is removed from the status payload. `watchdogSource` is now `"inferred"` or `"NONE"`.
- **Kept for S3:** the `deepPowerDown()` calls (State_Error, ConnectivityFailsafe), `extern AB1805` and its include in `StateMachine.h`, the object definition and the PreBindRtc call. `lib/` is untouched.

## Tests
- The full host suite is **118/118** (baseline was 117).
- `thermal_coupling_structural_test.py` passes on a clean copy.
- Binary `nm`:
  - Present: `RecoveryState::{get,set}_powerOffRequested`, `HibernateCycle::classifyWake(int)` and the ExternalWatchdog/Am18x5 symbols.
  - Absent: `clearAb1805StaleAlarmInterrupts`, `interruptAtTime`, `ab1805WakeReasonName`, `AB1805::stopWDT`, `AB1805::resumeWDT` and `AB1805::loop`.
  - `AB1805::setWDT` is still present: it is the vendored library's own code, called from the library's `setup()`, and the app no longer calls it.

New tests:
- `watchdog_inferred_classification_test.{cpp,sh}` runs the real `isInferredWatchdog` against a table (no record, byte, recent and stale stage 3, future timestamp, other stages, time invalid). It has 5 mutants, all caught: always wake, never wake, no recency, no byte, any stage.
- `s2_alarm_watchdog_wake_structural_test.py` has 10 in-memory probes, all detected:
  - reason 20 always a wake
  - evidence read from `retained`
  - byte not cleared
  - byte cleared before read
  - byte written after the call
  - alarm armed twice
  - no watchdog stop before hibernate
  - refresh missing
  - hibernate-fallback restart removed
  - stale-clear back
  - It also checks that no `ab1805.` call other than `deepPowerDown` remains.

Updated tests (they pinned behaviour that S2 intentionally changes):
- `watchdog_ab1805_classification_test.{cpp,sh}` is **deleted** and replaced by the test above (the AB1805 PIN_RESET path no longer exists).
- `hibernate_wake_diagnostics_test`: the DPD-window mirror is replaced by a reason-30-alone mirror, and the fidelity greps are updated.
- `clock_hibernate_helpers_move_structural_test.py`: the retired functions are now asserted absent, and the "HibernateCycle.cpp unchanged" check is dropped.
- `sleep_config_ownership_structural_test.py`: the hibernate `duration` is now expected. The baseline-drift check skips only that one key.
- `sleep_cloud_gate`, `sleep_plan`, `sleep_preflight` and `sleep_wake` structural tests: base comparisons now exclude `hibernateAttempt`, normalise the ExternalWatchdog calls and the one reworded comment, and ignore the removed AB1805 include.
- `sleep_execution_equivalence_test.{sh,cpp}` and `SleepExecutionFakes.h`: see the deviations.

## Deviations (for the architect)
1. **The equivalence fixture was re-frozen.** The test compared the current code to the pre-S-2 base blocks. S2 changes the hibernate block and the watchdog calls on purpose, so that comparison cannot hold.
   - I retired the base-block (mode a) comparison and froze the current output.
   - The scenarios hib==3/4 (stale clear and alarm failure) are gone. The ExternalWatchdog stub emits `EXTWDT stop` and `EXTWDT start`.
   - As a one-off check, I diffed the old fixture against the new one. The differences are `AB1805 stopWDT/resumeWDT` becoming `EXTWDT stop/start`, plus the removed `clearStale` and `interruptAtTime` lines and the dropped scenarios.
   - The 7 existing mutants and the required-line list are kept (updated for the new indentation and lines).
2. **Reason 20 with Time invalid:** stage-3 evidence is accepted, since recency cannot be judged. This is a judgement call.
3. **The `ab1805_watchdog` event label is kept** for fleet-query stability. The `ab1805WakeReason` payload field is dropped.
4. **The validity read uses `Time.isValid()`** (an existing allowlisted line shape). `Clock::isTimeValid()` and `isPlausibleEpoch` are barred from this file by the clock-owner and trust tests.
5. **Tooling:** several bulk edits used python scripts, not the edit tool: the de-indent of the hibernate block, the `HibernateCycle.cpp` and GCC setup rewrites, and the test rewrites. The scratch `runall.sh` was written with a heredoc.

## Open questions and residuals
- A manual reset (reason 20) reads as an inferred watchdog.
- If `deepPowerDown()` fails or returns, the byte stays set until the next boot, so it can be stale.
- Failsafe defers can refresh `lastConnectivityRecoveryAction` while stage==3, so stage-3 evidence can look recent when it is not.
- Reason 30 does not distinguish an alarm wake from a GPIO (button) wake. Stage 7 must check that a button wake is not mislabelled.
- Device OS does not stop the chip dog for sleep, which is why the app must stop and start it. Nothing was verified on hardware.
