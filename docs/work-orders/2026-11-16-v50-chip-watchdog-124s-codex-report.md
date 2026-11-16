UNDER REVIEW: integration/v50-ab1805 @ 1d1d5af7a22dc47094a4c23b515b348bd8ae153d

## 1. Refresh behavior

**VERIFIED — `ExternalWatchdog.refresh()` reaches a real AM18x5 feed operation on every completed application-loop pass, but the configured D5 route feeds by GPIO edge rather than by rewriting the watchdog register.**

- VERIFIED — `ExternalWatchdog` is watchdog instance 2, while `Watchdog` is instance 1: `wiring/inc/spark_wiring_watchdog.h:328-356`.
- VERIFIED — On Boron/nRF52840 with external RTC support, instance 2 maps to `Am18x5Watchdog`: `hal/shared/watchdog_hal.cpp:49-63`.
- VERIFIED — The wiring call chain is `WatchdogClass::refresh()` → `hal_watchdog_refresh()` → virtual `refresh()`: `wiring/src/spark_wiring_watchdog.cpp:48-50`, `hal/shared/watchdog_hal.cpp:94-99`.
- VERIFIED — `Am18x5Watchdog::refresh()` requires `initialized_` and `started()`, then calls `feedWatchdog()`: `hal/shared/am18x5.h:423-427`.
- VERIFIED — Because setup supplies `.watchdogPin(WATCHDOG_DONE_PIN)` and that pin is D5, `feedWatchdog()` reads D5 and toggles it; it writes the WDT register only when `wdi_pin == PIN_INVALID`: `src/Generalized-Core-Counter.cpp:293-300`, `src/device_pinout.cpp:42-45`, `hal/shared/am18x5.cpp:934-946`.
- VERIFIED — There is no rate limiter in the wiring or AM18x5 refresh path; the HAL mutex only serializes calls: `hal/shared/watchdog_hal.cpp:28-47,94-99`.
- VERIFIED — Refresh return values are ignored by the application, so invalid-state, I2C-status-read, or feed failures are not logged: `src/Generalized-Core-Counter.cpp:1130-1134`.
- VERIFIED — `started()` reads WDT.BMB; however, it only promotes software state to STARTED and does not demote a previously STARTED state when hardware reports zero: `hal/shared/am18x5.cpp:958-963`, `hal/shared/am18x5.h:408-413`.
- INFERRED — That asymmetric state cache is a secondary weakness, but normal `stop()` explicitly sets STOPPED, so it does not explain the ordinary stop→start sequence by itself: `hal/shared/am18x5.h:415-420`.
- VERIFIED — `init(timeout=124000)` stores the configuration; `start()` calls `enableWatchdog()` with that stored timeout and therefore rewrites/reloads WDT after every successful stop: `hal/shared/am18x5.h:379-405`.
- VERIFIED — 124000 ms selects the 1/4 Hz clock and BMB=31, then writes the WDT register: `hal/shared/am18x5.cpp:900-925`.
- VERIFIED — `stop()` clears WDT.BMB, while the following `start()` rewrites the complete saved WDT value; `init()` is not required again after sleep: `hal/shared/am18x5.cpp:928-932`, `hal/shared/am18x5.h:400-420`.
- VERIFIED — The outer `loop()` contains no early return and calls refresh after every state handler, including CONNECTING and CloudRecover, and before cloud/publish-queue servicing: `src/Generalized-Core-Counter.cpp:1085-1154`.
- VERIFIED — A state handler can delay reaching refresh, especially intentional sleep and teardown work, but an ordinary handler `return` returns only to the outer loop: `src/Generalized-Core-Counter.cpp:1096-1134`, `src/state/State_Connect.cpp:379-405,593-605`.
- VERIFIED — Breadcrumb 18 is written after refresh and after the publish queue, and its timestamp is updated at the write: `src/Generalized-Core-Counter.cpp:1130-1154`, `src/observability/LoopForensics.cpp:277-280`.
- INFERRED — Therefore a reset retaining breadcrumb 18 with essentially the reset uptime proves a very recent refresh invocation, though not that D5 produced an edge at the AM1805 pin.

## 2. Timing of resets #52–#54

**INFERRED — If these inferred PIN_RESETs came from the AM1805, the evidence shows that the wake-time `start()` armed a fresh 124 s counter but repeated D5 refreshes failed to reload it; the data does not support a blocked loop or a counter continuing from sleep entry.**

- VERIFIED — #52 retained uptime/breadcrumb time was 1,785,675 ms; serial shows the ULP wake at 1,658,662 ms, giving 127.013 s wake-to-reset: `status/2026-11-15/2026-11-15T13-06-24-988Z.json:4`, `Dev-09_night1_snapshot.log:192-198`.
- VERIFIED — #52 reached CloudRecover stage 2 at 1,779,461 ms, only 6.214 s before the retained reset time: `Dev-09_night1_snapshot.log:206-233`.
- VERIFIED — #52’s watchdog event retained breadcrumb 18 and stage `connectivity`, showing a completed prior pass followed by entry into the next pass: `watchdog/2026-11-15/2026-11-15T13-06-22-905Z.json:4`.
- VERIFIED — #53 retained uptime/breadcrumb time was 3,735,353 ms; wake was at 3,611,988 ms, giving 123.365 s: `status/2026-11-15/2026-11-15T13-06-29-776Z.json:4`, `Dev-09_night1_snapshot.log:257-270`.
- VERIFIED — #53’s stage was `diag` with breadcrumb 18, placing expiry immediately before or during the next refresh site rather than inside a long state-handler block: `watchdog/2026-11-15/2026-11-15T13-06-28-168Z.json:4`, `src/Generalized-Core-Counter.cpp:1128-1134`.
- VERIFIED — #54 retained uptime/breadcrumb time was 3,726,839 ms; wake was at 3,607,908 ms, giving 118.931 s: `status/2026-11-15/2026-11-15T14-07-37-634Z.json:4`, `Dev-09_night1_snapshot.log:285-299`.
- VERIFIED — #54 connected at 3,711,151 ms and the event reports `connAge=15705`, breadcrumb 18, queue zero, and state SLEEPING: `Dev-09_night1_snapshot.log:300-321`, `watchdog/2026-11-15/2026-11-15T14-07-35-850Z.json:4`.
- VERIFIED — In all three status events, `appBreadcrumbMs` equals `lastWatchdogUptimeMs`; firmware persists the breadcrumb timestamp as watchdog uptime: the three status files above at line 4 and `src/Generalized-Core-Counter.cpp:785-787`.
- INFERRED — Thus all three had executed line 1133 essentially at expiry; if the AM1805 caused the resets, the GPIO feed was electrically ineffective despite the software call.
- VERIFIED — A watchdog that failed to stop would have expired during the 691 s and 3600 s sleeps, not after their wakes; those sleeps completed normally: `Dev-09_night1_snapshot.log:184-196,248-263,275-291`.
- VERIFIED — `stop()` clears BMB and `start()` rewrites WDT, so the code defines a new window at wake, not at the preceding stop: `hal/shared/am18x5.cpp:900-946`, `src/state/State_Sleep.cpp:771-774,908-915`.
- INFERRED — The long sleeps and wake-relative clustering strongly rule out “counting continuously from sleep entry” and make a silent stop failure unlikely.

**Ranked explanations**

1. INFERRED — Most likely: `.watchdogPin(D5)` selects the GPIO-feed path, but D5/WDI edges do not reach or are not recognized by the AM1805; start/stop still work because those use I2C register writes.
2. INFERRED — Less likely: another PIN_RESET source happens to recur at the AM1805’s exact 124 s wake-relative period; reason 20 remains only inferred because no chip-status proof survives.
3. INFERRED — Still less likely: ignored refresh return codes or stale `started()` state suppress feeding, although the repeated freshly armed 124 s windows show that start normally succeeds.
4. INFERRED — Not supported for #52–#54: modem-shutdown-before-HAL-disable ordering.

- VERIFIED — Device OS can wait up to 120 s for cellular shutdown before calling `hal_sleep_enter()`: `system/src/system_sleep.cpp:70-94,161-175,200-203`.
- VERIFIED — POWER_OFF reaches the AM18x5 sleep HAL only afterward, where it disables the watchdog: `hal/src/nRF52840/sleep_hal.cpp:1308-1340`, `hal/shared/am18x5.cpp:1053-1056`.
- VERIFIED — `powerOffForRecovery()` already explicitly stops the external watchdog before `System.sleep()`, which is why its comment warns about relying on HAL ordering: `src/power/ConnectivityFailsafe.cpp:135-143`.
- VERIFIED — #52 and #53 were CONNECTING/diagnostics cases, so this ordering cannot cover them.
- VERIFIED — #54’s breadcrumb was refreshed at the reset uptime, making a subsequent 120 s modem wait impossible; moreover it was the normal ULP path, whose application stop precedes `System.sleep()`: `src/state/State_Sleep.cpp:771-811`.
- INFERRED — For #54, sleep preparation was where the already-expiring counter happened to fire, not the cause of its 119 s wake-relative deadline.
- INFERRED — A D5 oscilloscope trace, logged refresh return codes, and an A/B build omitting `watchdogPin()` would separate explanation 1 from the remaining possibilities.

## 3. Smallest fix and host regression

**INFERRED — The smallest robust fix is to omit `.watchdogPin(WATCHDOG_DONE_PIN)` from the AM18x5 fleet configuration, forcing every refresh through the verified WDT-register-write path; this is about one changed `src/` line, or 2–4 lines with an explanatory comment.**

- VERIFIED — With no watchdog pin, `Am18x5Configuration` leaves pin 0 invalid and `feedWatchdog()` writes `watchdogValue_` to WDT: `wiring/inc/spark_wiring_exrtc.h:330-355`, `hal/shared/am18x5.cpp:934-940`.
- VERIFIED — This retains the same 124 s configuration and the same stop/start API; only the refresh transport changes.
- VERIFIED — The existing S2 test verifies call count and stop/start ordering but does not exercise a counter or feed transport: `tests/s2_alarm_watchdog_wake_structural_test.py:97-126`.
- VERIFIED — The current external-RTC structural and host tests explicitly require D5 and would need to be revised to require no watchdog pin: `tests/external_rtc_structural_test.py:34-37`, `tests/external_rtc_test.cpp:26-28,87-99`.
- VERIFIED — The POWER_OFF host test uses a fake watchdog and verifies only stop/sleep/start ordering: `tests/s3_timed_power_off_test.cpp:113-116,176-194`.
- VERIFIED — Device OS’s GCC host watchdog HAL returns success without implementing hardware behavior, so the real AM18x5 refresh cannot be meaningfully exercised by the current host target: `hal/src/gcc/watchdog_hal.cpp:20-43`.
- INFERRED — The appropriate host regression is therefore a mutation-checked mirror following the existing extraction/fake pattern: require `watchdogPin()==PIN_INVALID`, model Device OS’s route selection, start at wake, refresh every 30 s for over 124 s, assert one WDT-register write per refresh and no expiry.
- INFERRED — A mutation restoring `.watchdogPin(D5)` must make that test fail, which would have caught the present configuration even though electrical behavior remains bench-only.

## 4. One-step bench proof

**INFERRED — Flash one instrumented 5001 build, allow one ULP timer wake, hold it awake for at least 180 s, and pass only if logged register-path feeds continue past 124 s with no reset.**

- INFERRED — At `src/Generalized-Core-Counter.cpp:1133`, temporarily schedule refresh every 30 s and log every actual feed as `WdtFeed ms=<millis> rc=<return>`; scheduling avoids an every-loop serial flood while preserving a large safety margin.
- INFERRED — Expected evidence is feeds near wake+0/30/60/90/120/150/180 s, all `rc=0`, continuous serial uptime, and no reason-20 reboot.
- INFERRED — After the 180 s hold, allowing a normal sleep additionally proves that register feeding did not break stop/start behavior.
- INFERRED — If D5 is retained for diagnosis instead of applying the fix first, also log its post-toggle level and scope D5 at the AM1805 WDI pin; software level changes without physical edges would directly isolate the fault.

## Evidence consistency

- VERIFIED — Nothing in the #52–#54 raw evidence contradicts the supplied timing table; differences of roughly 1–2 s in derived UTC boot/wake times are consistent with truncated `TimeDiag` seconds and serial gaps.
- VERIFIED — Archive filenames are publish times, not reset times; queued #52/#53 events were delivered later when #54 connected.
- VERIFIED — The archive also contains older reset counts #47–#51, including several breadcrumb-28 sleep-entry resets and one 123.550 s breadcrumb-18 reset: `status/2026-11-15/2026-11-15T10-41-56-824Z.json:4` through `...T10-42-16-434Z.json:4`.
- INFERRED — Those older events do not contradict #52–#54, but they warn that the broader reset history also includes expiry at sleep-entry boundaries.
- VERIFIED — After the hibernate wake, the snapshot reaches more than 7,164 s uptime while each visible awake interval remains below 124 s, with no further reset: `Dev-09_night1_snapshot.log:440-478,483-546`.

**RECOMMENDATION: REVISE — force WDT-register refresh, add the host route/liveness regression, and require the >124 s post-wake bench proof before proceeding.**

**Could not verify:** INFERRED — the physical D5-to-WDI connection and edge waveform; INFERRED — refresh/start/stop return codes on Dev-09 because they were not logged; INFERRED — definitive AM1805 reset-source status because firmware classifies reason 20 only by absence of power-off evidence.