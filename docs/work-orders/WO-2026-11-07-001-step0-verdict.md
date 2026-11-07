UNDER REVIEW: tag v49.1 (the fleet firmware) @ 9a96854e1f85934ce843db3ed16f8630b3ce0dde

**VERDICT: PARTLY VERIFIED — CONCERN.** `git rev-parse HEAD` exactly matched the SHA. The 42 restarts are genuine reason-less software resets, but the available telemetry cannot identify the exact caller. Device OS’s runtime power-fail handler is the strongest concrete candidate.

### 1. Device OS reset paths — ANSWERED

`RESET_REASON_NONE` is invalid, not an expected wake reason (`services/inc/system_defs.h:51-69`). On an SREQ software reset, `Init_Last_Reset_Info()` reads DR_02/DR_03 and then clears them (`hal/src/nRF52840/core_hal.c:651-660`). Thus DR_02=0 yields reason 0.

`HAL_Core_System_Reset_Ex()` writes DR_02/DR_03 before calling the bare reset (`core_hal.c:549-556`). The bare `HAL_Core_System_Reset()` does not write a reason (`:537-546`).

Reason-less Boron reset sites in Device OS 6.6.0:

- **Runtime power-fail warning:** `power_failure_handler()` directly calls `NVIC_SystemReset()` (`platform/MCU/nRF52840/src/hw_config.c:87-90`). It is registered at the 2.8 V POF threshold during every boot (`:171-178`). This is reachable while awake, publishing, entering ULP/STOP, or before HIBERNATE reaches SYSTEM OFF. It produces the observed SREQ+DR_02=0 signature.
- Bootloader-update check calls bare `HAL_Core_System_Reset()` (`hal/src/nRF52840/core_hal.c:432-440`); update-only.
- `configure_uicr()` directly resets after changing UICR (`platform/MCU/nRF52840/src/hw_config.c:92-132`); early-boot/one-time.
- `Finish_Update()` directly calls `sd_nvic_SystemReset()` (`:314-328`); factory/DFU update paths.
- Legacy IWDG-disable calls `NVIC_SystemReset()` (`:472-492`), but the bootloader’s only call passes a nonzero timeout (`bootloader/src/main.c:439-440`).
- Nordic startup directly resets after first-time pin-reset UICR programming (`third_party/nrf5_sdk/.../system_nrf52840.c:168-180`; enabled by `third_party/nrf5_sdk/build.mk:38`). Its NFC equivalent at `:153-162` is not enabled by this build.
- Bootloader factory fallback directly resets (`bootloader/src/main.c:388-402`). Its nRF `HAL_Core_System_Reset_Ex()` ignores the supplied reason and directly invokes `NVIC_SystemReset()` (`bootloader/src/nRF52840/core_hal.c:34-35`); the visible call at `bootloader/src/main.c:422-423` is update-only.

All application/system runtime `_Ex` callers supply nonzero reasons: USB user 140, config 150, update 70/90, panic 130, factory 100, safe 110 and DFU 120 (`system/src/{usb_control_request_channel.cpp:544,main.cpp:815,916,firmware_update.cpp:280,system_mode.cpp:35-52}`, `services/src/panic.c:85-87`). Application `System.reset()` supplies 140 (`wiring/src/spark_wiring_system.cpp:40-54`).

The sleep path contains no reset call:

- It validates before disconnecting (`system/src/system_sleep.cpp:114-120`), performs modem-off with a 120 s wait whose result is not checked (`:70-98`), invokes the HAL and returns its error (`:203-246`).
- ULP/STOP contains blocking waits and resume assertions (`hal/src/nRF52840/sleep_hal.cpp:997-1036,1147-1203`); a stall should end as watchdog 60 and an assertion as panic 130, not reason 0.
- HIBERNATE alarm/source errors return error codes (`:1346-1365,1418-1475`); successful SYSTEM OFF wakes as reason 30 (`core_hal.c:668-671`).
- Firmware handles ULP rejection by STOP, then timer-only STOP; exhaustion calls `System.reset(RESET_CAUSE_SLEEP_ATTEMPTS_EXHAUSTED)`, reason 140 with nonzero data (`src/state/State_Sleep.cpp:853-915`).
- Current HIBERNATE failure similarly returns and falls back to ULP (`State_Sleep.cpp:690-758`).

`Init_Last_Reset_Info()` clears DR_02/03 before application setup (`core_hal.c:786-792`). A second direct reset after that clearing naturally reports 0 again, explaining how repeated reason-less resets can retain no earlier reason.

### 2. Fleet data — PARTLY

Source: `rr0/raw/<day>/{status,Ubidots-Sensor-Hook-v1,watchdog,hibernate_wake}/<deviceId>/`; all times UTC, local time EDT through 31 Oct and EST thereafter. `ct` is the nearest hook’s `connecttime` immediately before→after the status, not teardown time. `u` is prior-boot `appBreadcrumbMs`.

```text
Court1 UTC (local)       breadcrumb/u ms       ct s
10-28 13:00:48 (09:00)  28/127677714         14→11
10-28 13:00:51 (09:00)  18/12414             11→7
10-28 13:00:52 (09:00)  18/10730             11→7
10-28 14:00:30 (10:00)  18/1442340            1→6
10-28 22:00:26 (18:00)  18/29881870          11→15
10-30 11:12:43 (07:12)  28/128666861         11→1
11-02 20:39:35 (15:39)  18/292790167         11→8
11-03 11:52:28 (06:52)  28/39580804           1→11
11-03 12:00:33 (07:00)  28/435392             11→13
11-03 13:00:55 (08:00)  28/128269             13→17
11-06 15:00:23 (10:00)  28/10683351            2→2
11-06 16:00:51 (11:00)  28/88651               2→15
11-06 21:01:19 (16:01)  18/14451327           15→32
11-07 02:04:04 (21:04)  28/18033264           45→1

PCKL3 UTC (local)        breadcrumb/u ms       ct s
10-24 11:02:15 (07:02)  18/117485698           1→5
10-24 14:35:30 (10:35)  18/13907971            1→1
10-24 15:12:10 (11:12)  28/1653675             1→1
10-24 22:01:36 (18:01)  18/22587035            1→17
10-25 03:02:07 (23:02)  18/12738492            1→6
10-25 03:02:10 (23:02)  18/2578570             6→45
10-25 03:02:13 (23:02)  18/1036534            45→15
10-25 10:08:18 (06:08)  18/28828388           17→14
10-25 11:13:06 (07:13)  18/3857703             1→4
10-25 19:20:56 (15:20)  18/27730455            1→1
10-25 20:02:38 (16:02)  18/1950585             1→24
10-26 21:01:36 (17:01)  18/84834328            1→6
10-27 16:26:31 (12:26)  18/55464488            1→5
10-27 20:42:44 (16:42)  15/12876702            2→4
10-30 00:01:16 (20:01)  18/83904656            1→1
10-30 00:01:19 (20:01)  18/2067088             1→14
10-30 18:04:20 (14:04)  18/66399222            1→1
10-31 00:50:20 (20:50)  18/26431605            1→6
10-31 10:50:18 (06:50)  18/33104519            1→14
11-02 23:00:41 (18:00)  18/218526971           1→4
11-03 14:01:55 (09:01)  18/46806836            1→80
11-03 18:51:13 (13:51)  18/19336582            1→5
11-03 19:57:07 (14:57)  18/1898613            16→14
11-05 02:03:08 (21:03)  18/97764883            1→1
11-05 11:21:56 (06:21)  18/36013191            1→19
11-05 23:00:21 (18:00)  17/40024900            1→4
11-06 11:25:20 (06:25)  18/46383779            1→39
11-07 00:01:07 (19:01)  28/44434986            1→1
```

Breadcrumb totals: Court1 28×8 and 18×6; PCKL3 18×24, 28×2, 15×1 (`CLOUD_LOOP_ENTER`) and 17×1 (`PUBLISH_QUEUE_ENTER`). Twenty-six events occurred during minutes 00–05.

Connect-time comparison:

- Court1 all hooks: median 10 s, p90 14 s, max 317 s; healthy Court2: median 1 s, p90 2 s, max 108 s.
- PCKL3: median 1 s, p90 7 s, max 100 s; healthy PCKL1: median 7 s, p90 15 s, max 660 s.
- Healthy status boots’ nearest hook times were Court2 `[1,1,1,1]` and PCKL1 `[1,16,2,11,2]`.
- There is no common affected-pair signature: Court1 is slower than Court2, while PCKL3 is faster than PCKL1. This cannot be attributed to hardware/installation differences per Chip’s ruling.

Every reason-0 status has `failsafeStage=0`, `failsafeCount=0`, `lastConnectionAgeSec=-1`. Adjacent battery voltage is 3.99–4.16 V and adjacent hook alerts are 0. No Court1 hook contains alert 15 or 19. PCKL3’s only alert-19 hooks belong to its separate reason-60 watchdog boots on 25 Oct and 3 Nov.

`lastWatchdog*` is historical, not evidence for the reason-0 boot: Court1 stays at count 20/breadcrumb 28/uptime 16,795,082 ms; PCKL3 advances 7→8→9 only after its two separate reason-60 boots.

Sleep mode is **NOT VERIFIED**. The status payload adds `sleepMode` only for a successfully classified hibernate (`Generalized-Core-Counter.cpp:1322-1330`). No unit has a `hibernate_wake` event. Breadcrumb 28 therefore distinguishes only “at or just after a sleep call”; it cannot distinguish ULP from either STOP fallback. A reason-less software reset during the armed HIBERNATE call should preserve `HibernateCycle`’s retained pending record and emit a failed hibernate event on boot, so the ten breadcrumb-28 cases are more consistent with ULP/STOP than HIBERNATE.

Signal strength/quality and modem state are **NOT VERIFIED**: none of the archived payloads contains `sig`, RSRP or RSRQ. Those values exist only in serial `ConnSummary`/`Connect` logs (`State_Connect.cpp:458-487,570-577`).

Modem teardown is **NOT VERIFIED**: hook `connecttime` measures connection acquisition (`ReportPublisher.cpp:103-117`), not shutdown. Teardown duration exists only in RAM/serial (`SleepTeardown.cpp:171-249`). No payload contains cloud-disconnect, modem-off, total-teardown, standby or `MODEM_HEALTH` fields. Therefore I cannot show that any reason-0 event followed a slow or failed shutdown. The lack of alert 15/19 is negative evidence, but not a measured successful teardown.

The breadcrumb-28 cases remain in recovery-plan item 14’s watch. Unlike its known Dev-14 case, these records have neither serial context nor alert-15/19 evidence connecting them to failed modem power-off.

### 3. Published 3-boot clusters — ANSWERED

The “within 5 s” times are cloud `published_at`, not proven reset times. Court1’s three records carry prior uptimes 127,677,714 ms, 12,414 ms and 10,730 ms; PCKL3 carries 12,738,492 ms, 2,578,570 ms and 1,036,534 ms. The queued statuses drained rapidly after reconnection; the resets themselves cannot all fit inside five seconds.

These are not resets inside early `setup()`. Breadcrumb 18 is set after `PublishQueuePosix::loop()` (`Generalized-Core-Counter.cpp:1192-1195`), after setup cleared `bootInProgress` and set setup-complete (`:1119-1123`). Each short predecessor reached the normal loop.

The sequence is repeated reason-less resets after normal application startup, with a status from each boot queued and later delivered. It is not the application boot-storm holdoff: that requires six resets while `bootInProgress` remains true (`:341-406`), whereas breadcrumb 18 proves stable setup completion. The precise repeated trigger remains **NOT VERIFIED**.

### 4. S2 native sleep exposure — ANSWERED for the plan; implementation not present

S2 Stage 6 stopped at Step 0 with zero `src/` edits (`wt-s2/.../WO-2026-11-05-004-stage6-copilot-report.md:1-15`). Therefore this is a plan assessment, not a shipped-code result.

Native HIBERNATE with `duration()` adds the Device OS external-RTC alarm path (`sleep_hal.cpp:1352-1365`) and retains GPIO wake sources. Alarm/readiness failures return an error, after which S2 plans the existing ULP fallback. Native wake is reason 30. `ExternalWatchdog` resets through the chip/reset pin and is expected as reason 20, not reason 0.

S2 neither adds nor removes the runtime POF direct-reset handler, bootloader/UICR paths, DR_02 clearing, or ULP/STOP resume paths. It leaves reason-less reset exposure materially unchanged, although it exercises more of Device OS’s HIBERNATE alarm path.

### Ranked hypotheses and discriminators

1. **POF-warning direct software reset (`hw_config.c:87-90`) — strongest.** Exact reason-0 mechanism; runtime reachable; 26/42 events cluster around hourly radio/report activity. Confirm with raw 3V3 rail/POF evidence or a unique retained/DR_02 marker from that handler.
2. **Another bare Device OS/bootloader reset or lost DR_02 — possible, weaker.** Exact mechanism but known callers are update, UICR, factory or DFU paths and do not fit repeated ordinary operation. Confirm with raw RESETREAS/DR_02 captured before `Init_Last_Reset_Info()` clears them.
3. **Radio/modem activity induces the POF condition — plausible upstream trigger, unproved.** Court1’s longer connections support it locally; PCKL3’s faster-than-PCKL1 connections argue against a fleet-wide RF/connect explanation. Confirm with serial `ConnSummary` signal/phase times plus rail measurement at the reset.
4. **Sleep/modem teardown stall — weak for reason 0.** A stall should produce watchdog 60; known sleep failures produce reason 140. Confirm only with persisted last teardown elapsed/result and the last serial line before reset.
5. **Early-boot/UICR loop — lowest.** Contradicted by breadcrumb 18 and setup completion on the clustered predecessors.

Smallest missing evidence: raw reset registers/DR_02 before Device OS clears them, a POF marker or rail trace, and persisted last connect signal/phases plus teardown elapsed/result. Without those, the exact producer and any modem-shutdown causal link are **NOT VERIFIED**.

Model actually run: `gpt-5.6-sol`; reasoning level: high. No tracked file was edited.