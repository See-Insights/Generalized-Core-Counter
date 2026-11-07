UNDER REVIEW: wo/2026-11-05-005-v50-s3-timed-power-off @ dd856f82fc15e09f52d5450df6c3f882f376fb93

PASS for S3 as scoped — round 2 of 2.

`git rev-parse HEAD` matched the full SHA above. The tracked worktree remained clean.

| # | Verdict | Evidence |
|---|---|---|
| 1. F1 | **PASS** | `src/power/ConnectivityFailsafe.cpp:134-143`: ExternalWatchdog stop is immediately before `System.sleep()`, and start is the first operation on every return. `objdump` confirms instance 2 stop → sleep → instance 2 start. Caller cleanup and fallback resets remain at `ConnectivityFailsafe.cpp:322-329` and `State_Error.cpp:177-182`. These are the same `ExternalWatchdog.stop()/start()` APIs used by S2 at `State_Sleep.cpp:706,736,773,886,914`. |
| 2. Watchdog window | **PASS** | Fully traced below. There is no continuous wall-clock watchdog coverage, but the nRF52840 WDT provides partial CPU-awake coverage. |
| 3. F2/F3 tests | **PASS** | Scope checks at `sleep_execution_structural_test.py:40-49,118-131` and `sleep_config_ownership_structural_test.py:287-296,338-348` accept the helper and reject namespace scope. The fake separates power cut from returned `SYSTEM_ERROR_NONE` at `s3_timed_power_off_test.cpp:29-31,101-116,184-225` and checks stop/start, cleanup and fallback reset. |
| 4. Round-1 regression | **PASS** | All round-1 items rechecked. The helper remains fresh-local, 30 seconds, `POWER_OFF`, RTC-only; caller evidence/log/flush/reset ordering remains correct. Only the three intended S3 source files changed from `62e3dbc`; S1/S1b/S2, `State_Sleep.cpp`, boot classification, persistence and `lib/` are untouched. |
| 5. Budget/build/suite/mutations | **PASS** | `src/`: +18/−2 = **+16 net**. CI discovery: **121/121** inside the worktree; clean-export thermal check passed. Five mutations were caught: Copilot stop removal, restart removal, namespace global, plus new returned-success early-return and duplicate-sleep mutations. Fresh Device OS build passed. |
| 6. Other paths/S4 | **PASS** | No additional defect found in the failsafe, Error path, boot classification, or S4 hand-off. Reason-20 evidence is consumed before watchdog forensics at `Generalized-Core-Counter.cpp:745-800`; exact sentinel/stage rules remain at `HibernateWakeDiagnostics.h:163-174`; oscillator snapshot hand-off remains untouched at `Generalized-Core-Counter.cpp:657-662`. |

### Architect’s watchdog-window answer

Plainly: **no watchdog provides continuous wall-clock coverage from the external-watchdog stop until the AM18x5 cuts power.** The interval is covered by the internal nRF52840 WDT only while the CPU is awake; it is uncovered while the CPU is sleeping/idling.

- The 124-second AM18x5 external watchdog is stopped at `ConnectivityFailsafe.cpp:139`. Device OS can then spend up to 120 seconds shutting down cellular at `system/src/system_sleep.cpp:79-93`, before entering the HAL at `:203`. The AM18x5 driver’s later disable at `hal/shared/am18x5.cpp:1053-1056` is normally a no-op because S3 already stopped it.

- This Boron build enables the nRF52840 hardware WDT. It is configured for 60,000 ms at `Generalized-Core-Counter.cpp:159,415-432`. Boron clears `HAL_WATCHDOG_CAPS_SLEEP_RUNNING` at `:417-420`; Device OS consequently selects `NRF_WDT_BEHAVIOUR_PAUSE_SLEEP_HALT` at `hal/src/nRF52840/watchdog_hal_impl.h:57-65`.

- The nRF WDT is fed by `serviceAwakeWatchdog()` at `LoopForensics.cpp:294-305`, normally from `Generalized-Core-Counter.cpp:1156`. Nothing feeds it inside `System.sleep()`. However, modem-off polling uses `HAL_Delay_Milliseconds(100)` at `system_network_manager.cpp:1195-1203`, implemented as `vTaskDelay()` at `delay_hal.cpp:27-30`; the WDT pauses whenever the CPU enters sleep/idle.

- There is no separate 60-second `ApplicationWatchdog`/system-watchdog thread in this build. The fallback instance at `Generalized-Core-Counter.cpp:457-460` is compiled out because `Wiring_Watchdog=1`. The ELF contains the hardware `WatchdogClass` path but no instantiated `ApplicationWatchdog` worker.

- The replaced vendored path had the same external-watchdog gap: `AB1805::deepPowerDown()` called `setWDT(0)` first at `lib/AB1805_RK/src/AB1805_RK.cpp:496-506`. The nRF WDT supplied the same partial CPU-awake coverage.

Failure behavior:

- A CPU-awake hang accumulates 60 seconds and causes an nRF watchdog reset—an MCU watchdog reboot, not the intended AM18x5 rail-cut power cycle.
- An ordinary unresponsive modem reaches Device OS’s 120-second timeout and then proceeds to the HAL.
- If progress stalls while the CPU remains asleep/idle and the timeout machinery also stops progressing, the internal WDT is paused and the external watchdog is off; the device can remain stuck. This is the accepted Amendment 5 residual.

### Verification record

- Build: **150924 text / 1114 data / 2240 bss**.
- Copilot round 2: **150924 / 1114 / 2240**; round 1: **150908 / 1114 / 2240**.
- `nm -C`: `000bdf1c T powerOffForRecovery()`; no `deepPowerDown`.
- Scoped `git diff --check 62e3dbc HEAD -- src tests`: passed.
- Interpreter/toolchain: **Python 3.14.6; zsh 5.9; Apple clang++ 14.0.3; GNU Arm Embedded 10.2.1; Device OS 6.6.0**.
- Model/reasoning actually run: **Codex · gpt-5.6-sol · xhigh**.

Findings: **none**; therefore no corrective fix or budget impact is proposed.

No hardware was available. I could not verify actual rail collapse, modem power loss, measured 30-second off interval, post-wake reset reason, RTC state, or successful reconnect/normal boot. External PR CI status was not independently checked; the complete local CI discovery was run.