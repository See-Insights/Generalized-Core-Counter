# GCC versus legacy v50/v54 counting: gap analysis

**Date:** 2026-11-24

**Compared**
- Legacy: `docs/counting/v50-parity-spec.md` (Visitation-Counters, branch `v54-dos660`). Legacy lines marked † were re-read in the legacy source for this analysis; the others are taken from the spec.
- GCC: `main` at `4ce134e` (`FIRMWARE_VERSION "v50.1-NativeRTC"`, product version 5001, `src/FirmwareVersion.h:20-23`). Every GCC row was read in the source at that commit.

**Scope.** PIR trail counting only. GCC has no separate pressure (car) path: `SensorFactory::createSensor()` implements only `SensorType::PIR` (`src/sensors/SensorFactory.h:71-88`) and the config ledger rejects any `sensor.type` other than 1 (`src/cloud/ConfigApply.cpp:351`). The legacy two-edge axle pairing (spec §1, L:1019-1024†) does not exist in GCC.

**Headline.** GCC counts with the same model as legacy (RISING interrupt sets a flag, the main loop turns it into one hourly and one daily count), and queues the hourly report before zeroing the hourly count, as legacy does. It differs in the debounce (500 ms, was about 200 ms), in a second count path on a sleep wake that may double count, in the report timestamp, in when the day rolls over (at close, report first; legacy at opening, reset first), in the payload (types, units, missing `maxmin`, extra fields) and in the alert model. No row in the spec's deviation register covers any of these yet.

## Conventions

- **L** = legacy `src/Visitation-Counters.cpp`. GCC files, relative to `src/`: **G** `Generalized-Core-Counter.cpp`; **PIRh** `sensors/PIRSensor.h`; **PIRc** `sensors/PIRSensor.cpp`; **Modes** `state/State_Modes.cpp`; **Rep** `state/State_Report.cpp`; **Idle** `state/State_Idle.cpp`; **Slp** `state/State_Sleep.cpp`; **Err** `state/State_Error.cpp`; **Conn** `state/State_Connect.cpp`; **RP** `cloud/ReportPublisher.cpp`; **MPD** `MyPersistentData.cpp`; **DB** `time/DailyBoundary.cpp`; **Clk** `time/Clock.cpp`.
- Verdicts: **MATCH** (GCC code read, same behaviour), **DIFFERS**, **MISSING** (legacy behaviour has no GCC equivalent), **UNKNOWN** (cannot be settled from source).
- "Risk" is the effect on counts or on Ubidots if the difference stays.

## 1. What counts as one event

| Spec item (section, legacy line) | GCC behaviour (file:line) | Verdict | Risk |
|---|---|---|---|
| Sensor pin `intPin`, `INPUT_PULLDOWN`; module enabled by `disableModule` low; LED by `ledPower` low (§1, L:248, :294, :740, :746) | `pinMode(intPin, INPUT_PULLDOWN)`, `digitalWrite(disableModule, LOW)`, `digitalWrite(ledPower, LOW)` (PIRh:41-47) | MATCH | None. |
| Sensor type stored; default 0 (pressure); PIR needs `Set-SensorType "1"` (§1, src/sys_status.h:25) | Default `set_sensorType(1)` (MPD:554); only PIR exists (SensorFactory.h:71-88); ledger accepts only 1 (ConfigApply.cpp:351) | DIFFERS | None for PIR. A pressure counter cannot be migrated. |
| Counting versus occupancy: legacy counter firmware has one mode (§1) | `sensorMode` 0 COUNTING, 1 OCCUPANCY, 2 MEASUREMENT (persist/SystemConfig.h:40-42). Firmware default COUNTING (MPD:141), but the product 42131 default ledger sets `"sensorMode": 1` (docs/config/default-settings-42131.json:6). The main loop counts only in COUNTING (G:1196-1201); in OCCUPANCY the payload carries `occupancy`/`dailyoccupancy`, not `hourly`/`daily` (PIRc:26-33) | DIFFERS | A trail counter given the 42131 defaults sends no counts at all. The counting product needs its own ledger default with `sensorMode` 0. |
| `attachInterrupt(intPin, sensorISR, RISING)` at boot in open hours and at opening; detached at close (§1, L:415, :483, :447†) | `attachInterrupt(intPin, pirISR, RISING)` in `initializeHardware()` and `onWake()` (PIRh:50, :166); boot init only when `isWithinOpenHours()` (G:1000-1012); detached in `onSleep()` (PIRh:143), which runs only when the night sleep is committed (Slp:515) | DIFFERS | See "counting after close" below: the detach is later than legacy. |
| ISR sets a `volatile bool`, not a counter (§1, L:1020-1021†, :282) | `pirISR()` sets `_motionDetected = true` (PIRc:10-12; `static volatile bool`, PIRh:190) | MATCH | None. Edges during a blocking call collapse into one count in both. |
| Flag serviced at the end of every loop pass, in any state (§1, L:707) | End of `loop()`: `handleCountingMode()` when `sensorMode == COUNTING` (G:1196-1198), after the state handler | MATCH | None. |
| One count: `hourlyCount++`, `dailyCount++`, `lastCountTime = Time.now()` (§1, L:784-786†) | `set_hourlyCount(+1)`, `set_dailyCount(+1)`, `set_lastCountTime(isClockTrusted() ? Time.now() : 0)` (Modes:33-41) | MATCH | `lastCountTime` is 0 on an untrusted clock; GCC never reads it (no consumer), so no count effect. |
| **Debounce.** Lockout is the time inside `recordCount()`: `delay(200)`, flag cleared last, so edges in that window merge into the count (§1, L:788†, :803†) | `loop()` clears the flag first, then `if (nowMs - _lastEventMs < 500) return false;` (PIRh:77-81). No delay. An edge less than 500 ms after the last accepted count is discarded | DIFFERS | **Counts.** Two walkers 200 to 500 ms apart: legacy 2, GCC 1. GCC undercounts busy trails relative to legacy. The 500 is a literal with no config. |
| Hold-off / retrigger: none in firmware (§10) | None beyond the 500 ms above | MATCH | None. |
| Blue LED on for 1000 ms per count (§1, L:775-776†) | `signalLED(true, 1000)` (Modes:49, :189) | MATCH | None. |
| No nap while a count is in progress or the LED timer runs (§1, L:490†) | Sleep deferred while `signalLEDTimeRemaining() > 0` in COUNTING (Idle:246-248; Slp:592-599) | MATCH | None. A set `_motionDetected` flag alone does not block sleep, but it is consumed at the end of the same loop pass. |
| **Count on a nap wake.** Wake on `intPin` RISING; no code checks the wake pin; the count depends on Device OS running the attached ISR on wake (§1, L:505†, :511†) | Sensor wake source `{intPin, RISING}` added to the ULP sleep (PIRh:127; Slp:750-755). After the sleep, `pirWake = (wakePin == sensorWake.pin)` (Slp:972) and `processSleepWakeEvent()` adds one count itself, "even if the ISR flag did not survive" (Modes:177-191). The ISR stays attached across the nap, and the synthesized count does not clear `_motionDetected` or set `_lastEventMs`, so if the ISR also ran on wake, `handleCountingMode()` adds a second count at the end of the same loop pass | UNKNOWN | **Counts.** If Device OS runs the ISR on wake (legacy relies on exactly this), every wake event in low-power modes is counted twice. Needs a bench test. |
| Verbose per-count publish (`Count` / `Verbose_Counts_Hook`), which lengthens the lockout (§1, L:789-801†) | Log line only (Modes:44-45) | MISSING | No count effect in normal running. No remote per-count calibration view. |
| `maxMinValue` updated per count (§1, L:778-782†) | No such field (CurrentData, MyPersistentData.h:650-679) | MISSING | Ubidots `maxmin` variable stops updating. |
| No lifetime counter (§1) | None (MyPersistentData.h:670-672) | MATCH | None. |
| Counts are `int`, no limits (§1, L:186, :188) | `uint16_t` (MyPersistentData.h:671-672); on load, `hourlyCount > 10000 \|\| dailyCount > 100000` zeroes both (MPD:667-670) | DIFFERS | Wrap at 65535 and zeroing above 10000 per hour are outside real trail volumes. |
| Counting after close: SLEEPING detaches the interrupt and turns the sensor off as its first act, before the last-hour report and connection (§2/§3, L:447-452†) | The close report is queued, the day reset, and the device may connect (`closeDue` → CONNECT, Rep:209) with the sensor still on; the sensor goes off only in `sleepPlan()` (Slp:504-516). `handleCountingMode()` has no open-hours test (Modes:29-53) | DIFFERS | **Counts.** PIR events between the close report and the night sleep (up to the connect time) are added to the new day and appear in the next morning's first report. |
| Counting while the clock or config is not usable: not a concept; legacy uses the RTC hour as it is (§2, L:406, :413) | `isWithinOpenHours()` returns true when `!Time.isValid()` or `!Config::isValid(false)` (Clk:43-50). `Config::isValid` needs a ledger-sourced config (Config.cpp:111-127). `Clock::openness()` returns Unknown on an untrusted clock (Clk:577-587) and Unknown is treated as open for staying awake (Idle:127-132; Slp:1160) | DIFFERS | **Counts.** A device with an untrusted clock or factory-default config keeps the sensor on and counts through closed hours. |

## 2. Hourly and daily totals and rollover

| Spec item | GCC behaviour | Verdict | Risk |
|---|---|---|---|
| Hour change test `Time.hour() != Time.hour(lastReportedTime)` in IDLE, local time (§2, L:441†) | Awake path: `(now - lastReport) >= intervalSec` with `lastReport` persisted and `intervalSec` from `reportingIntervalSecForRuntime()`, default `DEFAULT_REPORT_INTERVAL_SEC = 3600` (Idle:197-209; Config.h:34). Sleep path: timer wake aligned to `now % interval`, plus 1 s, then REPORTING with no test (Slp:575-585, :1054-1064) | DIFFERS | **Hour attribution.** In CONNECTED mode reports are 3600 s after the last one, not on the clock hour. A boot or button report at 10:20 makes every later report cover xx:20 to xx:20. Low-power modes stay on the hour. |
| Hourly count zeroed when the event is queued, not when delivered (§2, L:823-827†) | `publishData()` queues (Rep:65; RP:124), then `resetTotals(ResetPeriod::Hourly)` → `set_hourlyCount(0)` (Rep:91-93; PIRc:15) | MATCH | None. Same "reset when queued" ordering. |
| Hourly zeroed even if the queue rejects the event (publish result unused, L:824†) | Reset is unconditional; `queued == false` only logs (RP:127-129; Rep:91-93) | MATCH | Same loss in both: the hour's counts drop out of `hourly`, stay in `daily`. |
| `lastReportedTime` is RAM only, rebuilt at boot (§2, L:402-403†) | `lastReport` persisted in `/usr/sysStatus.dat` (Rep:87) | DIFFERS | After a reset GCC does not wait for the next hour change; in CONNECTED mode it reports as soon as 3600 s have passed since the stored time. |
| **Timestamp.** Non-zero hour: `lastCountTime` (inside the hour just ended). Zero hour: report time, start of the new hour (§2, L:817-822†) | Counting mode always uses `endOfPrevHourStampSec = nowStampSec - (Time.minute() * 60L + Time.second() + 1L)`, i.e. xx:59:59 of the previous clock hour (RP:64-65, :102). The `stampOverride` argument is used only in occupancy mode (RP:102) | DIFFERS | On-the-hour reports land in the same hour as legacy when counts are non-zero. Zero hours move back one hour. **Two reports in one clock hour (button, overdue, CONNECTED drift) carry the identical timestamp**; if Ubidots keeps one dot per timestamp, the earlier `hourly` value is replaced and its counts vanish from the hourly series. |
| Daily rollover at the **opening hour**: `dailyCleanup()` runs **before** `sendEvent()`, so the first report is 0/0 (§2, L:520-521†) | Daily rollover at the **close hour**: `DailyBoundary::check()` is due when `lastDailyCleanup < boundary` (today's `closeTime`, or yesterday's if not reached) (DB:35-42). REPORTING queues the report **first** (Rep:65), then `dailyCleanup()` (Rep:72-73), then stamps `lastDailyCleanup` (Rep:79) | DIFFERS | Daily totals for a normal day are the same. The last report of the day carries the full `daily`; the opening report is 0/0 in both. Edge cases differ (next four rows). |
| No once-per-day guard: any report in the opening hour zeroes daily and hourly again (§2, INFERRED, L:520†) | Guarded by `lastDailyCleanup` (DB:41-42; Rep:79) | DIFFERS | **Counts.** Legacy loses the opening hour's counts on a `SendNow` or button press in that hour; GCC keeps them. GCC daily can be higher. |
| Daily reset at boot when the local day changed; unsent hourly count is zeroed (§2, L:406-408†) | No boot-time day test (the only `resetEverything()` callers are the file initialiser, MPD:726, and `dailyCleanup()`, Rep:265). A missed close is caught at the next report, which sends the stored counts and then resets | DIFFERS | **Counts.** After an overnight outage GCC sends yesterday's residual `hourly` and `daily` stamped in today's hour; legacy discards them. |
| 24-hour devices: reset before send at 00:00 loses the 23:00 hour (§2, INFERRED) | Close 24 → boundary is local midnight (DB:13-14, :36-39); the 00:00 report is sent with the 23:00 hour and the full day, then reset | DIFFERS | **Counts.** GCC reports one more hour per day than legacy on 24-hour devices. |
| At close: last-hour report only if `hourlyCount` is non-zero (§2, L:449-452†) | A close report is always produced: timer wake → REPORTING (Slp:1054-1064), or `sleepPlan()` refuses the night sleep while the close is due (Slp:504-509) | DIFFERS | One extra zero dot per day at close. Final `daily` dot always present. |
| `resetEverything()` also zeroes `resetCount`, `maxConnectTime`, `minBatteryLevel` (§2, L:1204-1220†) | `resetEverything(false)` sets `lastCountTime` and `resetCount = 0` (MPD:648-650); the sensor zeroes hourly, daily and occupancy fields (PIRc:14-22). No max-connect or min-battery fields | DIFFERS | `resets` now restarts at close, not at opening. |
| Nothing happens at midnight for a day-time device (§2) | Same: boundary is the close hour, never midnight unless close is 24 (DB:35-39) | MATCH | None. |
| Daily cleanup needs no clock check (§2, L:406, :413) | `due` is false whenever `!Clock::isTrusted()` (DB:34) | DIFFERS | **Counts.** While the clock is untrusted the daily count is never reset and runs across days. |
| Open means `hour >= openTime && hour < closeTime`, whole local hours; defaults 6 and 22 (§2, L:413, :1052-1053) | `(hour >= openHour) && (hour < closeHour)` (Clk:12-16); `DEFAULT_OPEN_HOUR = 6`, `DEFAULT_CLOSE_HOUR = 22` (Config.h:32-33) | MATCH | None. |
| Bounds: open 0-23 by function, `openTime > 12` → 0 at boot; close `< 12` or `> 24` → 24 (§2, L:1074-1075) | Rules: open 0-12, close ≤ 24, close > open (Config.h:46-57); a stored pair that breaks them fails validation (MPD:96-99) | DIFFERS | A legacy site with close before 12 has no equal GCC setting. No count effect for normal hours. |
| Time zone: whole-hour float offset plus `dstOffset`; US DST by calendar day (§2, src/time_zone_fn.cpp:78-108) | POSIX TZ string through LocalTimeRK, default `"EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00"` (Config.h:31); `Time.zone()` is never set, so `Time.minute()`/`Time.second()` in the timestamp are UTC (RP:65) | DIFFERS | On the two DST days the legacy hour boundary moves at the day's first cleanup; GCC moves at 02:00. Counts unaffected; one hour's bucket differs. |
| Clock source: AB1805 RTC at boot, cloud sync once a day (§2, L:588-590) | Trust model with resync (Clk; `requestClockResync("daily-cleanup")`, Rep:260) | DIFFERS | See the untrusted-clock rows above. |

## 3. Open-hours behaviour

| Spec item | GCC behaviour | Verdict | Risk |
|---|---|---|---|
| Closed: sensor off, interrupt detached (§3, L:447-448†) | `onEnterSleep()` → `detachInterrupt(intPin)`, `disableModule` HIGH (Slp:515; PIRh:143-146) | MATCH | None once the night sleep starts (see §1 for the delay before it). |
| Closed sleep wakes on user switch and timer only, each hour, no report (§3, L:465-467, :462) | One sleep to the next opening: `secondsUntilNextOpen()`, clamped to `546 * 60` s (Slp:519-531). ULP config still lists the sensor pin (Slp:755) but the sensor is powered off. A timer wake always goes to REPORTING (Slp:1054-1064) | DIFFERS | No counts at night in either. A closed period longer than 9.1 h produces a mid-night zero report in GCC. |
| Closed: disconnected, modem off, in every power mode (§3, L:453-459) | Teardown before every sleep (`SleepTeardown::run`, Slp:1226); CONNECTED mode sleeps only when `Clock::openness() == Closed` (Idle:127-130) | MATCH | None for counts. |
| User button in the closed sleep sets hours to 0/24 and clears low power (§3, L:472-479) | Button wake re-enables the sensor and forces a service report (Slp:994-1001); hours are not changed there | DIFFERS | **Counts.** After a button wake at night the sensor is on (`onExitSleep()`, Slp:996) until the next night-sleep commit; legacy instead becomes a 24-hour counter. |
| At opening: sensor on, interrupt attached, report with daily cleanup (§3, L:481-485, :520-521) | Timer wake in open hours → `onExitSleep()` (Slp:1006-1013) → REPORTING (Slp:1064); the day was already reset at close | MATCH | First report 0/0 in both. GCC stamps it 1 s before the opening hour (RP:65). |
| `dailyCleanup()` forces low power mode for solar or SoC ≤ 65, clears verbose, queues `GoogleSheetsExport` (§3, L:1493-1501) | `dailyCleanup()` requests a clock resync and resets counts only (Rep:245-267) | MISSING | No count effect. No daily Google Sheets row. |
| Boot outside open hours: straight to sleep (§3, L:427, :442) | Sensor rails forced off when not open (G:1008-1011); state flow not traced further | UNKNOWN | Low. Bench: boot at night and confirm no counts. |

## 4. Reporting cadence

| Spec item | GCC behaviour | Verdict | Risk |
|---|---|---|---|
| One report per local clock hour in open hours; interval is a compile-time 3600 (§4, L:253, :441†) | `reportingIntervalSec` from the ledger, default 3600 (Config.h:34; Config.cpp:136-143). Low power: on the UTC hour + 1 s (Slp:575-585). CONNECTED: elapsed time (Idle:197-209) | DIFFERS | See §2 hour-attribution row. A ledger value other than 3600 changes the meaning of `hourly`. |
| Nap wake aligned: `constrain(wakeBoundary - Time.now() % wakeBoundary, 1, wakeBoundary)` (§4, L:502†) | `aligned = boundary - now % boundary`, clamped 1..boundary, `wakeInSeconds = aligned + 1` (Slp:576-585) | MATCH | None. |
| Report first, connect second; the event is queued whether or not connected (§4, L:519-524†) | `publishData()` (Rep:65) precedes the connect decision (Rep:168-225) | MATCH | None. |
| Connect every hour above 65 %; every 2nd hour at 51-65 %; every 4th at ≤ 50 %; never below 30 % (§4, L:531-548) | Tier multiplier 1/2/4/12 for HEALTHY/CONSERVING/CRITICAL/SURVIVAL; tier edges 75/70, 55/50, 35/30 with hysteresis (cloud/BatteryBackoffPolicy.h:20-57); `cadenceDue` when the epoch is within `CONNECT_ALIGNMENT_TOLERANCE_SEC = 30` of a multiple (reporting/ReportingPolicy.cpp:19-26, :82-83). Close report always connects (power/ConnectDecision.cpp:18). After a failed connect a hold of `HOLD_BASE_MS` 1 h doubling to `HOLD_MAX_MS` 6 h (ConnectDecision.cpp:7-12, :28-30) | DIFFERS | No count effect; delivery is later at different charge levels. |
| Skipped hours stay in the queue (§4) | Same; file queue (G:650-652) | MATCH | None. |
| Publish queue: RAM 2, file 100, oldest discarded (§4) | `.withFileQueueSize(800)` only (G:650-652) | DIFFERS | GCC holds 8 times more backlog before discarding. |
| Extra reports: `SendNow`, user button while connected, last hour at close (§4, L:1186-1195, :709-713) | User button any time → REPORTING + connect (G:1184-1189; Slp:994-1001). No `SendNow` function: the only cloud registrations are the variables `thrashTrips` and `thrashResets` (cloud/Particle_Functions.cpp:52-53). PIR wake when `(now - lastReport) >= intervalSec` → REPORTING (Slp:1081-1087) | DIFFERS | Each extra report zeroes `hourly` and is stamped xx:59:59 of the previous hour (see §2 timestamp row). |
| Webhook response wait `webhookWait = 30000` ms; alert 40 on timeout; no re-send (§4, L:553-565) | `webhookTimeoutMs`, fallback `20000UL` (G:1161-1164), started after connect; alert 40 on timeout (G:1171); no re-send | DIFFERS | No count effect. More alert 40 at the shorter wait. |
| Response handler: data `200` or `201` (§4, L:881-886) | Any non-empty response records `lastHookResponse` and clears alert 40; 200/201 treated as success, other codes logged (RP:273-317) | DIFFERS | A 4xx from Ubidots clears alert 40 in GCC; rejected data is not flagged. |

## 5. Payloads, event names and Ubidots variables

Legacy (L:823†): `{"hourly":%i, "daily":%i,"battery":%i,"key1":"%s","temp":%i, "resets":%i, "alerts":%i,"maxmin":%i,"connecttime":%i,"timestamp":%lu000}`

GCC counting mode (PIRc:31 + RP:104): `{"hourly":%i,"daily":%i,"battery":%4.2f,"vc":%.2f,"key1":"%s","temp":%4.2f,"resets":%i,"alerts":%i,"connecttime":%i,"fh":%lu,"lfb":%lu,"cyc":%lu,"slp":%lu,"timestamp":%lu000}`

| Spec item | GCC behaviour | Verdict | Risk |
|---|---|---|---|
| Event name `Ubidots-Counter-Hook-v1`, `PRIVATE \| WITH_ACK`, queued (§5.1, L:824†) | Name from the ledger (`reporting.webhook.name`, ConfigApply.cpp:548-553); if empty, `"counting-webhook-v1"` in COUNTING (Cloud.cpp:647-661). The "legacy fallback (Ubidots-Counter-Hook-v1)" is only a comment (Cloud.h:217). Product 42131 default is `"Ubidots-Sensor-Hook-v1"` (default-settings-42131.json:8). Flags `PRIVATE \| WITH_ACK`, queued (RP:124) | DIFFERS | **Ubidots.** With no configured name the event matches no legacy webhook and no data arrives. The counting product's name is a config decision. |
| 1 `hourly`, 2 `daily`, integers (§5.1) | Same keys, same order, `%i` (PIRc:31-32) | MATCH | None. |
| 3 `battery`: integer percent, truncated (§5.1, L:905) | `%4.2f`, clamped to 0-100, NaN → 0 (RP:78-82, :104) | DIFFERS | Ubidots shows two decimals; an event on `battery == 30` would stop matching. |
| (none) | `vc` cell voltage inserted between `battery` and `key1` (RP:104, :107) | DIFFERS | New variable. If the webhook template takes `key1` as `battery` context by name, no effect. |
| 4 `key1`: `batteryContext[]`, seventh string "Diconnected" (§5.1, L:240) | Same seven strings, seventh spelled "Disconnected" (RP:51-55) | DIFFERS | Only if anything matches the misspelled text. |
| 5 `temp`: integer degrees **Fahrenheit** (§5.1, L:945-953) | `%4.2f` of `get_internalTempC()`, degrees **Celsius** (RP:109; CurrentReadings.h:89-91) | DIFFERS | **Ubidots.** Same variable, different unit: a 75 °F day reads 23.9. Temperature events and history break. |
| 6 `resets`, 7 `alerts` in that order (§5.1) | Counting mode: `resets` then `alerts` (RP:110-113). Occupancy mode reverses them | MATCH | None for counting. |
| 8 `maxmin` (§5.1) | Absent | MISSING | Variable goes stale. |
| 9 `connecttime`: seconds, with +180/+300 added for staged retries (§5.1, §6, L:593-596) | `lastConnectionDuration = int(elapsedMs / 1000)` for the single attempt (Conn:306-307) | DIFFERS | Values never exceed one attempt's budget; no staged add-back. |
| (none) | `fh`, `lfb`, `cyc`, `slp` before `timestamp` (RP:104, :115-116) | DIFFERS | Four new variables per device (auto-created). |
| 10 `timestamp`: Unix seconds + `000`; last count time or report time (§5.1) | `%lu000` (RP:104); value is end of previous hour (RP:65, :102) | DIFFERS | See §2. |
| Buffer 256 bytes (§5.1, L:815) | `char data[256]` (RP:57); the GCC payload is longer and the `snprintf` result is not checked. Hand estimate of the worst case from the format string (5-digit counts, 10-digit `fh`/`lfb`/`cyc`/`slp`): about 235 bytes | DIFFERS | About 20 bytes of margin. A truncated payload would be invalid JSON and lose the hour; any added field needs the length rechecked. |
| Response route: subscribe to own device ID; data is the HTTP code (§5.1, L:300-303) | `Particle.subscribe(deviceID, UbidotsHandler)` (G:466-471) | MATCH | None. |
| Webhook definition and Ubidots labels: NOT FOUND in legacy repo (§5.1, §10) | Not in the GCC repo either | UNKNOWN | Whether the template passes fields by name, and what it does with extra keys. |
| `Ubidots_Alert_Hook` on ERROR_STATE (§5.2, L:643-644) | No such event; alerts travel only in the report and the `status` event (G:1352) | MISSING | **Ubidots.** Anything fed by the alert hook gets nothing from GCC. |
| `GoogleSheetsExport`, `Daily-Location-Signal-Webhook`, `Verbose_Counts_Hook`, `Count`, `State Transition` (§5.2) | Not published. GCC publishes `status`, `watchdog`, `pdiag`, `hibernate_wake`, and diagnostics `Cellular`, `Ubidots Hook`, `Daily Cleanup`, `Mode` (G:1352, :1392; power/PowerDiagnostics.cpp:394; RP:242, :320; Conn:531; Rep:247; MPD:128) | MISSING | No daily sheet row, no cell-location event. |
| Particle variables `HourlyCount`, `DailyCount`, … (§5.3) and functions `setDailyCount`, `resetCounts`, `SendNow`, … (§9) | None; configuration is by ledger (Particle_Functions.cpp:52-53) | MISSING | **Ops.** No remote way to seed or zero the daily count or force a report. |

## 6. Low-power and connection behaviour

| Spec item | GCC behaviour | Verdict | Risk |
|---|---|---|---|
| `SYSTEM_MODE(SEMI_AUTOMATIC)`, no system thread (§6, L:219, :132) | `SYSTEM_MODE(SEMI_AUTOMATIC)`; system thread on by default (Particle_Functions.cpp:8-9) | DIFFERS | Loop is blocked less by connection work, so fewer edges collapse during a connect. Slightly higher counts than legacy while connecting. |
| `lowPowerMode` flag, forced daily for solar or SoC ≤ 65 (§6) | `connectionMode` 0 CONNECTED, 1 INTERMITTENT, 2 DISCONNECTED, 3 INTERMITTENT_KEEP_ALIVE (SystemConfig.h:49-52); firmware default CONNECTED (MPD:142); battery tiers adjust the effective mode (Rep:184-186) | DIFFERS | Mode decides which report trigger applies (§2). |
| Nap wake sources: user switch, sensor pin RISING, timer to the hour (§6, L:504-506†) | Button FALLING, sensor `{intPin, RISING}`, timer (Slp:753-755, :769) | MATCH | None (see §1 for the count on wake). |
| After a count wake, awake about 1 s, then nap (§6, L:499†) | Wake → count → SLEEPING (Slp:1095-1097), deferred while the 1000 ms LED runs (Slp:592-599) | MATCH | None. |
| Stay awake 90 s after connecting (§6, L:254) | Sleep gate waits for queue drain, ledger sync, update check, webhook response (Slp:268-284) | DIFFERS | No count effect. |
| Staged connect timeout 180/300/420 s with resets, alerts 32, 31, 30 (§6, L:610-636) | One attempt per decision; `CONNECT_BUDGET_DEFAULT_MS` 5 min, `CONNECT_BUDGET_DEEP_MS` 11 min (power/ConnectivityPolicy.h:62-63); on timeout alert 31 and radio off (Conn:641-645); no reset between stages | DIFFERS | Fewer resets, so fewer chances to lose an unsaved count. |
| Disconnect failure → alert 15 → deep power down (§6, L:454-458) | Alert 15 on teardown budget overrun (state/SleepTeardown.cpp:201, :317); cleared without reset in non-CONNECTED modes (Err:135-141) | DIFFERS | No count effect. |
| Charging inhibit below 36 °F / above 100 °F, alert 10 (§6, L:961-965) | Ledger thresholds `armHighC 37.0`, `armLowC 0.0` (default-settings-42131.json:9); no alert 10 | DIFFERS | No count effect. |
| Watchdog 124 s, stopped in sleep (§6, L:341) | `ExternalWatchdog` `timeout(124000)` (G:299), stopped before sleep (Slp:773); plus a 60000 ms awake watchdog (G:159) | MATCH | None. |
| Out of memory → alert 14 → ERROR_STATE → reset (§6, L:728-732) | `delay(100); System.reset(RESET_CAUSE_OUT_OF_MEMORY)` directly (G:1177-1181), without `flushAllBeforeReset()` | DIFFERS | A count made in the last 250 ms may not be saved (§8). |

## 7. Alert codes

Model: legacy holds one code, last writer wins, cleared after every hourly payload (§7, L:827†). GCC holds one code, replaced only by a strictly more severe one (MPD:811-830, tiers MPD:767-809), and cleared after a queued report only for 15, 19, 31, 41, 42, 43, 44 (RP:25-37, :143-152); the others stay until their condition clears. **Verdict DIFFERS.** Risk: a held code repeats in every hourly payload, so an Ubidots event on `alerts ==` fires every hour, and a held tier-3 code hides 31 and 40.

| Code | Legacy meaning (§7) | GCC meaning and raise site | Verdict | Risk to Ubidots |
|---|---|---|---|---|
| 10 | Too hot or cold to charge | Not used | MISSING | None (no legacy event). |
| 11 | PMIC reset required | Not used; PMIC faults are 20-23 | MISSING | E30-11 never fires. |
| 12 | Initialization error (FRAM) | Not used | MISSING | E30-12 never fires. |
| 13 | Excessive resets | Not used; nearest are 17 and 19 | MISSING | E30-13 never fires. |
| 14 | Out of memory | Severity entry only (MPD:775); not raised (Err:42-45) | MISSING | E30-14 never fires. |
| 15 | Disconnect / modem-off failure | Same (SleepTeardown.cpp:201, :317) | MATCH | None. |
| 16 | (none) | ULP sleep failed (Slp:848) | DIFFERS | New code, no legacy event. |
| 17 | (none) | Boot storm (G:540-550) | DIFFERS | New code, no legacy event. |
| 18 | (none) | State-machine thrash (ThrashGuard.cpp:141, :150); no clear site (docs/reference/alert-codes.md) | DIFFERS | New code; once set it masks 15, 16, 20-23, 31, 40-44. |
| 19 | (none) | Watchdog reset (G:793) | DIFFERS | New code, no legacy event. |
| 20-23 | Firmware update (20-22 unused, 23 unreachable) | PMIC faults (power/PmicFaultMonitor.cpp:201-233, :406, :416) | DIFFERS | Known collision, handled in Ubidots. |
| 30 | Cloud timeout, cellular up | Severity entry only (MPD:796); not raised | MISSING | E30-30 never fires. |
| 31 | Failed to connect | Connect budget exceeded (Conn:644) | MATCH | Legacy raises once after three stages; GCC once per failed attempt. |
| 32 | Connect slow, reset and retry | Severity entry only (MPD:798); not raised | MISSING | None (no legacy event). |
| 40 | No webhook response | Timeout (G:1171); > 3 h without a response in open hours (Rep:116-121); no response at the sleep gate (Slp:283). Held until any response (RP:299-302) | DIFFERS | Same number, wider meaning, held as a condition. |
| 41 | (none) | Config / ledger apply failure (Conn:545) | DIFFERS | New code. |
| 42 | (none) | Data-ledger publish failure (RP:179; Conn:553) or OTA pending at sleep (Slp:280) | DIFFERS | New code. |
| 43 | (none) | Queue not drained before sleep (Slp:271) | DIFFERS | New code. |
| 44 | (none) | Ledger sync timeout before sleep (Slp:277) | DIFFERS | New code. |
| 45 | (none) | `CONNECTIVITY_FAILSAFE_ALERT = 45` (ConnectivityPolicy.h:125); no raise site found in `src/` | DIFFERS | None today. |

Reset counting: legacy counts `PIN_RESET` and `USER` (§7, L:369-370). GCC counts `PIN_RESET`, `USER`, `WATCHDOG` and `USER_APPLICATION` (G:611-619). **DIFFERS**: `resets` can read higher in GCC; it is zeroed at close, not at opening (§2).

## 8. State kept across a reset

| Spec item | GCC behaviour | Verdict | Risk |
|---|---|---|---|
| External FRAM; whole struct written at the end of the loop pass in which a count was made (§8, L:719-726, :802†) | Flash files through StorageHelperRK: `/usr/current.dat` (MPD:613) with `.withSaveDelayMs(250)` (MPD:634), flushed by `current.flush(false)` each pass (MPD:639) | DIFFERS | **Counts.** A count is durable up to 250 ms later, not in the same pass. A watchdog, OOM (G:1177-1181) or power loss inside that window loses it. Resets through `flushAllBeforeReset()` do not (Err:116-117). |
| Hourly, daily, last count time, alert survive reset and power loss (§8) | Same fields in `CurrentData` (MyPersistentData.h:662-672) | MATCH | None beyond the save delay. |
| FRAM version byte mismatch erases counts and settings (§8, L:346-356) | An invalid file is re-initialised and `resetEverything()` zeroes the counts (MPD:722-726); the version and hash checks are inside StorageHelperRK and were not read | UNKNOWN | Whether a power loss during a flash write, or a firmware update, can invalidate the file and zero the day. |
| Boot bounds checks on settings (§8, L:1066-1081) | `validate()` on hours, connection duration, recovery stage (MPD:89-121); counts sanity (MPD:667-670) | DIFFERS | None for counts in normal ranges. |
| RAM only, lost on reset: `lastReportedTime`, `sensorDetect`, minute tracker (§8) | RAM only: `_motionDetected`, `_lastEventMs` (PIRh:187-190), session flags. `lastReport` and `lastDailyCleanup` are persisted (Rep:79, :87) | DIFFERS | A pending edge is lost on reset in both. GCC's persisted `lastReport` avoids a second report in the same hour after a reset. |
| Publish queue on flash `/usr/pubqueue`, 100 files, 2 in RAM (§8) | Same library, 800 files (G:650-652); RAM queue size not set | MATCH | None. |
| Time across reset: AB1805 RTC (§8) | Native RTC support plus the trust model (Clk) | DIFFERS | See §1 and §2 untrusted-clock rows. |
| First-boot defaults: low power on, open 6, close 22, zone -5 (§8, L:1037-1057) | CONNECTED, COUNTING, open 6, close 22, 3600 s, Eastern TZ string (MPD:130-145) | DIFFERS | Until a ledger config arrives, `Config::isValid()` is false and open hours are not enforced (§1). |

## (a) The 10 most count-affecting differences, ranked

1. **Possible double count on every sleep wake** (§1): synthesized wake count (Modes:180-183) plus the ISR flag, with no shared debounce. UNKNOWN until benched; if real it doubles counts in low-power modes.
2. **Debounce 500 ms versus about 200 ms** (PIRh:79 versus L:788): GCC drops followers 200 to 500 ms behind.
3. **Identical timestamps for two reports in one clock hour** (RP:65, :102): the hourly series can lose the first report's counts in Ubidots.
4. **CONNECTED-mode reports on elapsed time, not the clock hour** (Idle:197-209): hourly counts shifted by up to 59 minutes.
5. **Counting through closed hours, and no daily reset, on an untrusted clock or invalid config** (Clk:43-50; DB:34).
6. **Counting between the close report and the night sleep** (Rep:65-79 versus Slp:515): evening stragglers land in the next day.
7. **Daily rollover at close with report-before-reset** (Rep:65-73 versus L:520-521): 24-hour devices gain the 23:00 hour; opening-hour button or `SendNow` no longer wipes the day.
8. **No boot-time new-day reset** (L:406-408): GCC sends yesterday's residue stamped today; legacy drops it.
9. **250 ms save delay on flash versus same-pass FRAM write** (MPD:634): the last count before an unplanned reset can be lost; file invalidation zeroes the day.
10. **Fewer blocking calls and a system thread** (§6): fewer merged edges during connects, so GCC can read slightly higher than legacy at busy times.

Not ranked because it is configuration: a trail counter left on `sensorMode` 1 or on an unmatched webhook name reports no counts at all (§1, §5).

## (b) UNKNOWN items that need bench or fleet evidence

1. Does Device OS run the attached `pirISR` on a ULP wake from `intPin`? One PIR pulse into a sleeping COUNTING device: does hourly rise by 1 or 2? (Modes:180-183; PIRh:66-87.)
2. What does Ubidots do with two dots that share a timestamp on `hourly` (replace or keep both)?
3. The counting product's webhook: its name, whether the request template passes fields by name, and how it treats `vc`, `fh`, `lfb`, `cyc`, `slp`, a decimal `battery` and a Celsius `temp`.
4. Worst-case length of the GCC payload against the 256-byte buffer (RP:57, :104): the hand estimate is about 235 bytes; not measured.
5. Whether a power loss or OTA can invalidate `/usr/current.dat` and zero the day's counts (StorageHelperRK validation not read).
6. Boot outside open hours: confirm the sensor stays off and nothing is counted until opening (G:1008-1011).
7. Side-by-side trail test, legacy and GCC on the same PIR signal for a day, to measure the net effect of items 1, 2 and 10 in (a).
8. How often fleet devices run with an untrusted clock during open or closed hours (drives rank 5).

## (c) Proposed order of work, smallest first

| # | Work item | Changes counts? |
|---|---|---|
| 1 | Record in the deviation register the GCC behaviours to keep on purpose: once-per-day guard, close-time rollover, 24-hour last hour kept, residue sent after an outage (Chip decides each) | Yes, already different: needs a recorded decision, no code |
| 2 | Bench test (b)1 and Ubidots check (b)2 | No (evidence only) |
| 3 | Counting product ledger default: `sensorMode` 0, 3600 s, webhook name chosen under the prefix rule; webhook template checked against the GCC payload | No device counts; decides whether Ubidots receives them |
| 4 | Measure the worst-case payload length (estimate 235 of 256) and check the `snprintf` result | No |
| 5 | Debounce: restore the legacy lockout (about 200 ms, merge rather than discard) or record 500 ms as a decision | Yes |
| 6 | Make the wake count and the ISR flag one event (depends on item 2) | Yes |
| 7 | Counting-mode timestamp: legacy rule (last count time, else report time) or a unique stamp per report | Hourly attribution in Ubidots; not device totals. Decision needed |
| 8 | CONNECTED mode: report on the clock-hour change | Hourly attribution. Decision needed |
| 9 | Turn the sensor off at close before the close report and connection | Yes |
| 10 | Payload parity: `temp` unit and type, `battery` type, `maxmin` (add or retire), `key1` spelling | No counts; Ubidots variables. Decision per field |
| 11 | Alert parity: alert-hook event, clear-after-report rule, codes 10-14, 30, 32 with no GCC source, new codes 16-19 and 41-44 with no Ubidots event | No |
| 12 | Untrusted clock and invalid config: decide whether to count in closed hours and when to reset the day | Yes |
| 13 | Flush the count file before an OOM reset; review the 250 ms save delay | Yes (loss on reset only) |
| 14 | Remote operations: seed or zero the daily count, force a report | Only when used |
| 15 | Pressure (car) sensor path with axle pairing, before any car counter migrates | Out of scope here |
