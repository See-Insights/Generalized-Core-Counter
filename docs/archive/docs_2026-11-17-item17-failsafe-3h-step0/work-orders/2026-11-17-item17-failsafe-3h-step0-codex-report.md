UNDER REVIEW: 9a96854e1f85934ce843db3ed16f8630b3ce0dde

MODEL/REASONING: gpt-5.6-sol / medium — VERIFIED by session configuration; repository citation N/A.

## 1. The clock

**VERIFIED — Answer:** Stage 2 computes `Time.now() - max(lastConnection, today’s opening)`; it does not use the webhook-response or publish timestamp, so a pre-open connection starts the effective clock at opening, while a later successful cloud connection restarts it. `src/power/ConnectivityFailsafe.cpp:210-233`

- **VERIFIED:** Production staleness is three hours, but an effective reporting cadence of at least three hours extends action until `stale + cadence`; ChurchSt’s hourly cadence therefore retains the plain three-hour threshold. `src/power/ConnectivityPolicy.h:108-118`; `src/power/ConnectivityFailsafe.cpp:264-275`
- **VERIFIED:** `lastConnection` means successful Particle-cloud connection: Connect writes it when `Particle.connected()` succeeds, and an already-online device refreshes it on every report. `src/state/State_Connect.cpp:446-493`; `src/state/State_Report.cpp:282-285`
- **VERIFIED:** `lastHookResponse` is separate and is written by the Ubidots response handler on any non-empty webhook response, including responses outside the short response window. `src/cloud/ReportPublisher.cpp:267-301`
- **VERIFIED:** Both timestamps are fields of the persisted `SysData` record stored in `/usr/sysStatus.dat`, loaded during setup and flushed asynchronously after a 100 ms save delay. `src/MyPersistentData.h:115-133`; `src/MyPersistentData.cpp:57-80`; `src/MyPersistentData.cpp:85-87`
- **VERIFIED:** Thus the timestamps are designed to survive ordinary close, reset, and hibernate; daily close clears only RAM modem-health state, not either timestamp. `src/power/ModemHealth.cpp:71-78`; `src/MyPersistentData.cpp:226-245`
- **VERIFIED:** Hibernate is an MCU-resetting sleep, while the persistent file is reloaded on setup; the sleep path does not zero either timestamp. `src/state/State_Sleep.cpp:704-743`; `src/MyPersistentData.cpp:69-80`
- **VERIFIED:** Alert 40 is a different clock: during trusted OPEN hours, Report raises it when `now-lastHookResponse > 3 h`; this does not itself enter ERROR_STATE. `src/state/State_Report.cpp:99-125`; `src/power/ConnectivityPolicy.h:223-227`
- **VERIFIED:** Alert 40’s ERROR_STATE escalation requires webhook age over six hours, a recent cloud connection, and its three-hour escalation cooldown; only then does ERROR_STATE’s separate `lastHook > 3 h` check select a soft reset. `src/state/State_Report.cpp:136-150`; `src/state/State_Error.cpp:57-78`
- **VERIFIED:** Such an alert-40 reset would use reset data 6, whereas the observed data 2 uniquely identifies connectivity-failsafe stage 2. `src/state/State_Error.cpp:160-168`; `src/ResetCause.h:15-22`
- **VERIFIED:** Alert 40 fired first, 21–33 seconds after opening; stage 2 followed at opening plus three hours and 1–2 seconds. `build-tmp/item17-step0/evidence.md:7-10`; `build-tmp/item17-step0/evidence.md:20-22`
- **VERIFIED:** `failsafeLastAction` is set by `persistConnectivityFailsafeState(stage, now, ...)`; stage 2 calls that before flushing and resetting. `src/power/ConnectivityFailsafe.cpp:77-85`; `src/power/ConnectivityFailsafe.cpp:301-308`
- **VERIFIED:** A normal device reporting while online from 06:00 cannot reach this stage-2 condition at 09:00 because every successful Connect or already-connected report advances `lastConnection`. `src/state/State_Connect.cpp:489-499`; `src/state/State_Report.cpp:282-285`
- **INFERRED:** The alert-at-open means its persisted webhook timestamp was already more than three hours old and the overnight-hibernate suppression was not active for that session; the archive cannot show why suppression was absent. `src/Generalized-Core-Counter.cpp:999-1006`; `src/state/State_Report.cpp:112-125`; `build-tmp/item17-step0/evidence.md:22-23`

## 2. Do the archive facts fit the code?

**VERIFIED — Answer:** They fit a genuine three-hour Particle-cloud outage correctly handled by the failsafe, not a counting defect. `src/power/ConnectivityFailsafe.cpp:218-235`; `build-tmp/item17-step0/evidence.md:14-22`

- **VERIFIED:** With `lastConnection` before opening, the age base becomes the 06:00 opening; the `< 10800` return stops action before 09:00 and permits it at or after 09:00. `src/power/ConnectivityFailsafe.cpp:218-234`; `src/power/ConnectivityPolicy.h:113-116`
- **VERIFIED:** The first action is stage 2 when persisted stage is 0 or 1. `src/power/ConnectivityFailsafe.cpp:237-254`
- **VERIFIED:** Although a deterministic 0–1799 second jitter exists, it is applied only to the inter-stage cooldown guard, and that guard is bypassed when `currentStage == 0`; therefore the first stage-2 action is intentionally unjittered. `src/power/ConnectivityFailsafe.cpp:59-74`; `src/power/ConnectivityFailsafe.cpp:245-261`
- **VERIFIED:** The supervisor also waits for any currently active Connect attempt to leave its budget before resetting. `src/power/ConnectivityFailsafe.cpp:277-279`; `src/state/State_Connect.cpp:151-172`
- **VERIFIED:** A failed deep attempt records up to 660 seconds, raises alert 31, powers down connectivity, and leaves Connect; that matches the archived `conn 660` reports. `src/state/State_Connect.cpp:608-655`; `build-tmp/item17-step0/evidence.md:14-20`
- **VERIFIED:** Reports are queued before Connect begins, explaining why their device stamps precede their post-reset burst delivery and why `connecttime` describes the prior attempt rather than successful delivery of that report. `src/state/State_Report.cpp:64-70`; `src/state/State_Connect.cpp:228-285`; `build-tmp/item17-step0/evidence.md:13-20`
- **INFERRED:** The +1–2 seconds beyond exactly three hours are ordinary loop/state scheduling after the threshold, not an accumulating-clock error. `src/Generalized-Core-Counter.cpp:1126-1159`; `build-tmp/item17-step0/evidence.md:8-10`
- **VERIFIED:** If a unit really had successful Particle-cloud contact, `lastConnection` would move and the three-hour age would restart; a stage 2 at 09:00 despite such contact would require stale/corrupt persistence, a plausible-but-wrong RTC/open boundary, or contact that never reached `Particle.connected()`. `src/state/State_Connect.cpp:446-499`; `src/power/ConnectivityFailsafe.cpp:188-202`
- **INFERRED:** Evidence separating those cases would be serial `Connect: ok`/`ConnSummary` logs with timestamps, persisted last-connection telemetry before reset, Particle connection history, and hook-response events; none of those response/serial records is present here. `src/state/State_Connect.cpp:464-499`; `build-tmp/item17-step0/evidence.md:20-23`

## 3. Fleet-wide pattern

**VERIFIED — Answer:** Three of the five listed events cluster at exactly three hours after opening; PCKL3 does not, and the 1 November event cannot be placed relative to opening because its identity/time zone are unknown. `build-tmp/item17-step0/evidence.md:25-32`

- **VERIFIED:** ChurchSt’s 10, 13, and 16 November events are the code’s normal first-stage threshold for an hourly-cadence unit with no post-open successful connection. `src/power/ConnectivityFailsafe.cpp:218-275`; `build-tmp/item17-step0/evidence.md:28-31`
- **VERIFIED:** A trusted, correctly configured PCKL3 at 22:02 would be `Closed` and the supervisor would return without acting. `src/power/ConnectivityFailsafe.cpp:223-229`; `src/time/Clock.cpp:567-583`
- **INFERRED:** Given the stated 22:00 close, PCKL3’s viable code path is `Clock::Openness::Unknown`, caused by an untrusted clock or invalid/unloaded configuration; Unknown deliberately counts as open, allowing the already-stale 16-hour age to trigger. `src/time/Clock.cpp:567-583`; `src/power/ConnectivityFailsafe.cpp:226-229`; `build-tmp/item17-step0/evidence.md:30-32`
- **VERIFIED:** PCKL3’s alert 0 is consistent because connectivity stage 2 neither requires nor raises alert 40. `src/power/ConnectivityFailsafe.cpp:301-308`; `build-tmp/item17-step0/evidence.md:30-32`
- **VERIFIED:** Firmware 47 introduced this same “Unknown counts as open; only known Closed skips” behavior, so the 1 November unit could have fired while known-open and stale or while openness was Unknown. `CHANGELOG.md:33-40`; `src/power/ConnectivityFailsafe.cpp:218-229`
- **INFERRED:** The archive cannot determine which v47 branch fired, or whether it was about three hours after opening, because the device name and time zone are unavailable. `build-tmp/item17-step0/evidence.md:25-27`

## 4. Smallest fix

**VERIFIED — Answer:** No counting fix is warranted at this SHA; the source implements the observed timing intentionally, so there is no `src/` line estimate or new host-test proposal. `src/power/ConnectivityFailsafe.cpp:218-279`; `src/power/ConnectivityPolicy.h:100-118`

- **VERIFIED:** Existing coverage includes a host mirror tied to source-extracted thresholds/structure and a separate harness that compiles the real extracted supervisor body against fakes. `tests/connectivity_failsafe_open_hours_test.sh:4-13`; `tests/connectivity_failsafe_unknown_openness_test.sh:4-11`
- **VERIFIED:** The cadence test similarly compiles the real extracted cadence block, while the full production function cannot run natively without Particle/platform fakes. `tests/failsafe_cadence_rule_test.sh:4-13`; `tests/failsafe_cadence_rule_test.cpp:81-93`
- **INFERRED:** The real corrective work would require diagnosing the intermittent modem/SIM/carrier/antenna/power/Device-OS connectivity failure that exhausted consecutive 660-second attempts; the archive alone cannot select among those causes. `src/state/State_Connect.cpp:249-285`; `src/state/State_Connect.cpp:608-655`; `build-tmp/item17-step0/evidence.md:14-21`

## 5. v49.1 chip-watchdog feed

**VERIFIED — Answer:** v49.1 feeds the AM1805/AB1805 watchdog by periodically rewriting its I²C `REG_WDT`; it does not feed it by toggling the carrier’s D5/WDI pin and is therefore not exposed to the described v50 D5 reload defect. `src/Generalized-Core-Counter.cpp:809`; `lib/AB1805_RK/src/AB1805_RK.cpp:50-72`; `lib/AB1805_RK/src/AB1805_RK.cpp:208-242`

- **VERIFIED:** Setup initializes the AB1805 over I²C/FOUT and then arms it with `setWDT(WATCHDOG_MAX_SECONDS)`. `src/Generalized-Core-Counter.cpp:664-667`; `src/Generalized-Core-Counter.cpp:779-809`
- **VERIFIED:** `WATCHDOG_MAX_SECONDS` is 124 seconds. `lib/AB1805_RK/src/AB1805_RK.h:755-759`
- **VERIFIED:** `setWDT(124)` clamps to 31 four-second units, writes `REG_WDT` with reset mode and the ¼-Hz clock, and sets a 62,000 ms update period. `lib/AB1805_RK/src/AB1805_RK.cpp:208-242`; `lib/AB1805_RK/src/AB1805_RK.h:858-864`
- **VERIFIED:** Main calls `ab1805.loop()` every application loop; once the update period expires, the library calls default `setWDT(-1)`, restoring the saved 124-second register setting. `src/Generalized-Core-Counter.cpp:1171-1179`; `lib/AB1805_RK/src/AB1805_RK.cpp:50-72`; `lib/AB1805_RK/src/AB1805_RK.h:118-131`
- **VERIFIED:** Application watchdog call sites are setup arm, per-loop service, hibernate stop/resume, and ULP/STOP stop/resume—including recovery before a sleep-failure reset. `src/Generalized-Core-Counter.cpp:809`; `src/Generalized-Core-Counter.cpp:1174`; `src/state/State_Sleep.cpp:709-743`; `src/state/State_Sleep.cpp:783-798`; `src/state/State_Sleep.cpp:893-926`
- **VERIFIED:** Library-internal `setWDT(0)` sites disable the watchdog for repeating alarms, countdown interrupts, deep power-down, and reset-system events. `lib/AB1805_RK/src/AB1805_RK.cpp:367-375`; `lib/AB1805_RK/src/AB1805_RK.cpp:468-477`; `lib/AB1805_RK/src/AB1805_RK.cpp:496-507`; `lib/AB1805_RK/src/AB1805_RK.cpp:1021-1025`
- **VERIFIED:** D5 appears only as a carrier-map comment; the implemented common pins are D4, D7 and WKP, and initialization drives only the blue LED while treating the AB1805 FOUT wake pin as an input. `src/device_pinout.h:31-43`; `src/device_pinout.cpp:42-50`; `src/device_pinout.cpp:81-87`
- **VERIFIED:** The library’s optional `SET_D8_LOW` block is disabled at this SHA and, even if enabled, manipulates AB1805 registers for FOUT/D8 rather than toggling D5/WDI. `lib/AB1805_RK/src/AB1805_RK.cpp:7-10`; `lib/AB1805_RK/src/AB1805_RK.cpp:509-544`

**RECOMMENDATION — INVESTIGATE FURTHER:** Treat ChurchSt-1a as a real intermittent three-hour connectivity outage and investigate the failed 660-second acquisitions; do not revise the stage-2 counter. `src/state/State_Connect.cpp:608-655`; `build-tmp/item17-step0/evidence.md:14-21`

**UNVERIFIED:** No serial log or hook-response archive exists; PCKL3’s trust/config state, the v47 unit’s identity/time zone, the reason for ChurchSt’s three-day recurrence, and live electrical watchdog behavior could not be verified. `build-tmp/item17-step0/evidence.md:21-27`; `build-tmp/item17-step0/evidence.md:30-32`