# Legacy v50/v54 visitation counter: behaviour spec for GCC parity

**Date:** 2026-11-24

**Source**
- Repo: `/Users/chipmc/Documents/Maker/Particle/Projects/Visitation-Counters`
- Branch (from `.git/HEAD`): `v54-dos660`
- Version string: `PRODUCT_VERSION(54)` and `currentPointRelease = "54.00"` (src/Visitation-Counters.cpp:172-173). The change log says v54 is "v50 rebuilt for deviceOS@6.6.0", with SYSTEM_THREAD and MY_DEVICES removed and the retired `verizonSIM` flag always cleared; v51 to v53 are not included (src/Visitation-Counters.cpp:132). Target platform Boron, Device OS 6.6.0 (.vscode/settings.json:2-3).
- This spec was read from the working tree, without git. No git command was run in that repo, and nothing in it was changed.

**Summary.** The legacy firmware is a single state machine on a Particle Boron with an external FRAM and an AB1805 RTC/watchdog. A sensor pulse raises a RISING interrupt that sets one flag; the main loop turns the flag into one count on the hourly and daily counters, and a fixed 200 ms delay in that routine is the only debounce. Counting runs only inside the open hours (default 06:00 to 22:00 local, US DST rules). Once per local hour the device queues one `Ubidots-Counter-Hook-v1` event with the hourly count, the running daily count, battery, temperature, resets, alert code and connect time, then zeroes the hourly count and the alert. The daily count is zeroed at the first report in the opening hour, not at midnight. Outside open hours the sensor is off and the device sleeps, waking each hour without reporting. In low power mode the device naps between counts and connects for each report, less often as the battery falls. One alert code is held at a time and the last writer wins.

## Conventions

- Citations are `path:line`, relative to the legacy repo root. Within one bullet, a bare `:123` after a full citation means the same file.
- `src/Visitation-Counters.cpp` is generated from `src/Visitation-Counters.ino` (src/Visitation-Counters.cpp:1-6). For `.cpp` lines from 172 on, the `.ino` line is the `.cpp` line minus 43 (src/Visitation-Counters.cpp:171).
- **INFERRED** marks a statement worked out from the code rather than read as a direct statement. **NOT FOUND** marks something this repo does not contain.
- No compile-time flags change behaviour: the sources have no `#if`, `#ifdef` or `#define` apart from include guards (src/sys_status.h:1-2, src/time_zone_fn.h:1-2, src/particle_fn.h:3-4). Runtime configuration is by Particle functions (section 9).

## 1. What counts as one event

- **Sensor input.** One digital pin, `intPin = SCK`, set `INPUT_PULLDOWN` (src/Visitation-Counters.cpp:248, :294). The sensor module is enabled by driving `disableModule` (MOSI) low (:249, :740). For PIR, `ledPower` (MISO) low turns the sensor board LED on (:746).
- **Sensor type** is a stored setting: `0` pressure, `1` PIR (src/sys_status.h:25). The code never sets a default for it; after a FRAM erase it is 0, pressure (INFERRED from the zero-fill erase at lib/MB85RC256V-FRAM-RK/src/MB85RC256V-FRAM-RK.cpp:23-24 and no assignment in `loadSystemDefaults`, src/Visitation-Counters.cpp:1037-1057). A PIR trail counter must be set with `Set-SensorType "1"` (:1289-1295).
- **Interrupt model, not polling.** `attachInterrupt(intPin, sensorISR, RISING)` is attached at boot during open hours and at opening (src/Visitation-Counters.cpp:415, :483), and detached at close (:447).
- **ISR (PIR).** Every rising edge sets `sensorDetect = true` (src/Visitation-Counters.cpp:1020-1021). The flag is a `volatile bool`, not a counter (:282).
  - *Pressure (car) difference:* the ISR counts every second edge. The first edge sets `frontTireFlag`; the second sets `sensorDetect` and clears the flag (:1019-1024). `frontTireFlag` is a RAM static with no timeout, so an odd edge stays pending until the next edge, however late, and is lost on reset.
- **Main loop service.** At the end of every loop pass, in any state, `if (sensorDetect) recordCount();` (src/Visitation-Counters.cpp:707).
- **What one `recordCount()` does** (src/Visitation-Counters.cpp:771-804):
  - turns on the blue LED and restarts a 1000 ms one-shot timer that turns it off (:775-776, :285, :1031-1033);
  - updates `maxMinValue` (:778-782);
  - sets `current.lastCountTime = Time.now()` (:784);
  - `current.hourlyCount++` and `current.dailyCount++` (:785-786);
  - `delay(200)` (:788);
  - in verbose mode and connected, publishes a per-count event (:789-801, section 5);
  - sets `currentCountsWriteNeeded` so FRAM is written later in the same loop pass (:802, :723-726);
  - clears `sensorDetect` as its last step (:803).
- **Debounce and lockout.** There is no named debounce constant. The lockout is the time spent in `recordCount()`: the flag is cleared at the end, so any edge that arrives between the ISR and line 803 is absorbed into the same count. The fixed part is `delay(200)` (src/Visitation-Counters.cpp:788). INFERRED: the effective minimum interval between two PIR counts is a little over 200 ms, and longer when a verbose publish runs (:793-799).
  - *Pressure difference:* edges inside the lockout still toggle `frontTireFlag` in the ISR (:1019-1024), so an axle pair inside the lockout can be dropped or leave the pairing out of phase (INFERRED).
- **One count per loop pass at most.** Because the ISR sets a flag, several edges during any blocking call collapse into one count (INFERRED). Blocking calls include the disconnect, up to 15 s plus 30 s (src/Visitation-Counters.cpp:1136, :1147), and `delay(2000)` in the error state (:646).
- **Counters incremented:** `hourlyCount` and `dailyCount` only (src/Visitation-Counters.cpp:785-786). There is **no lifetime counter** in the structures (:185-197, src/sys_status.h:6-27).
- **Limits.** No minimum or maximum is applied to the counts; both are `int` (src/Visitation-Counters.cpp:186, :188). The only bound is on the manual seed: `setDailyCount` accepts 0 to 2000 (:1412; the comment at :1403 says 1000).
- **`maxMinValue` ("maxmin").** On the first count of a new minute it is set to 1 and then incremented, so it reads 2 after one count in a minute (src/Visitation-Counters.cpp:778-782). It is not a maximum: it holds the value for the minute of the latest count, and nothing else resets it. The struct comment calls it "Highest count in one minute" (:192).
- **Counting during a nap.** The nap sleep wakes on `intPin` RISING (src/Visitation-Counters.cpp:505). No code checks whether the sensor pin caused the wake (only the user switch is checked, :511). INFERRED: the wake pulse is counted only because the attached ISR runs on wake and sets `sensorDetect`; this depends on Device OS, not on code in this repo.
- **No nap while a count is in progress:** the nap is skipped while `sensorDetect` is set or the 1000 ms LED timer is active (src/Visitation-Counters.cpp:490).

## 2. Hourly and daily totals and rollover

- **Hour change test.** In IDLE, `Time.hour() != Time.hour(lastReportedTime)` moves to REPORTING (src/Visitation-Counters.cpp:441). Hours are local time (zone and DST, :378-380).
- **Hourly rollover.** `sendEvent()` builds the payload, then sets `hourlyCount = 0` and `alerts = 0` (src/Visitation-Counters.cpp:823-827). This happens when the event is queued, not when it is delivered. The hourly count is zeroed on every report, including mid-hour forced reports (section 4), so one hour can have more than one data point.
- **`lastReportedTime`** is set to `Time.now()` on entry to REPORTING (src/Visitation-Counters.cpp:517). It is RAM only (:261). At boot it is `lastCountTime` if `hourlyCount` is non-zero, otherwise now (:402-403).
- **Timestamp of the hourly point.** If `hourlyCount` is non-zero, the timestamp is `lastCountTime`, the time of the last count, which lies in the hour just ended. If it is zero, the timestamp is `lastReportedTime`, which was set a moment earlier, so it is the report time at the start of the new hour (src/Visitation-Counters.cpp:817-822, :517).
- **Daily rollover is at the opening hour, not at midnight.** Any REPORTING pass where `Time.hour() == sysStatus.openTime` calls `dailyCleanup()` before `sendEvent()` (src/Visitation-Counters.cpp:520-521). `dailyCleanup()` calls `resetEverything()` (:1502), which zeroes `dailyCount` and `hourlyCount`, sets `lastCountTime` to now, resets `maxConnectTime` to 0, `minBatteryLevel` to 100, `updateAttempts` to 0 and `resetCount` to 0 (:1204-1220).
  - Result: the first report of the day carries `hourly` 0 and `daily` 0 (change log, :86).
  - There is no once-per-day guard. INFERRED: any report in the opening hour (`SendNow`, user button, or a second pass) zeroes the daily and hourly counts again before sending (:520, :1186-1195, :709-713).
- **Daily reset at boot.** In `setup()`, if `Time.day() != Time.day(current.lastCountTime)` (local day), `resetEverything()` runs (src/Visitation-Counters.cpp:406-408). So a reset after local midnight zeroes the daily count before opening.
- **Nothing happens at midnight** for a device with open hours inside the day: it is asleep and makes no report (section 3). `dailyCount` keeps its closing value in FRAM until the opening report or a boot on a new day (INFERRED from :442, :520, :406).
- **24-hour devices** (`openTime` 0, `closeTime` 24) never enter SLEEPING, because `Time.hour() >= 24` and `Time.hour() < 0` are never true (src/Visitation-Counters.cpp:442). Their opening hour is hour 0, so `dailyCleanup()` runs at the 00:00 report. INFERRED: because `resetEverything()` runs before `sendEvent()` (:520-521), the 23:00 to 24:00 hourly count is zeroed without being sent, and the final daily total sent is the one in the 23:00 report.
- **At close.** In IDLE, when the hour reaches `closeTime`, the REPORTING choice at line 441 is overridden by SLEEPING at line 442. SLEEPING detaches the interrupt, turns the sensor off, and if `hourlyCount` is non-zero goes to REPORTING to send the last hour (src/Visitation-Counters.cpp:441-442, :447-452). INFERRED: if the last open hour has zero counts, no report is sent at close.
- **Open and close hours.** `openTime` and `closeTime` are whole local hours (src/sys_status.h:20-21). "Open" means `hour >= openTime && hour < closeTime` (src/Visitation-Counters.cpp:413, :481).
  - Defaults: open 6, close 22 (:1052-1053).
  - `Set-OpenTime` accepts 0 to 23 (:1357); `Set-Close` accepts 0 to 24 (:1384).
  - Boot-time bounds check: `openTime > 12` becomes 0; `closeTime < 12` or `> 24` becomes 24 (:1074-1075). So an open hour of 13 or later, or a close hour before 12, does not survive a reset.
- **Time zone.** `sysStatus.timezone` is a float; `Set-Timezone` accepts whole hours -12 to 12 (src/time_zone_fn.cpp:22-25). Default -5; out-of-range values become -5 at boot (src/Visitation-Counters.cpp:1050, :1072).
- **DST.** `dstOffset` default 1; `Set-DSTOffset` accepts 0 to 2 (src/Visitation-Counters.cpp:1051, src/time_zone_fn.cpp:53-55). Only US rules exist (`isDSTusa()`, src/time_zone_fn.cpp:78-108; the New Zealand rule is commented out, :120-156).
  - DST is evaluated at boot (src/Visitation-Counters.cpp:378-379), once a day in `dailyCleanup()` (:1495), and in `setDSTOffset` (src/time_zone_fn.cpp:58). It is not evaluated at 02:00.
  - INFERRED: the change takes effect by calendar day. April to October is DST; December to February is not; March is DST from the second Sunday on; November is DST only before the first Sunday (src/time_zone_fn.cpp:84-98). The time-of-day branch at :100-107 cannot be reached, because `beforeFirstSunday` and `secondSundayOrAfter` (:93-94) cannot both be true.
- **Clock source.** At boot the system clock is set from the AB1805 RTC if the RTC has been set and system time is not valid (lib/AB1805_RK/src/AB1805_RK.cpp:33-38). The cloud time sync is requested once a day on the first connection after `dailyCleanup()` (src/Visitation-Counters.cpp:588-590, :1494). `setup()` does not check `Time.isValid()` before using the hour or day (:406, :413).

## 3. Open-hours behaviour

- **Outside open hours: no counting.** SLEEPING detaches the sensor interrupt and disables the sensor module (src/Visitation-Counters.cpp:447-448, :751). The sleep wake sources are the user switch and a timer only, not the sensor pin (:465-467).
- **Outside open hours: no reporting.** On each hourly wake the device returns to IDLE, where SLEEPING overrides REPORTING (src/Visitation-Counters.cpp:462, :441-442). The one exception is the last-hour report at close when `hourlyCount` is non-zero (:449-452).
- **Outside open hours: disconnected.** Before sleeping, the device disconnects from the cloud and powers the modem off; failure raises alert 15 (src/Visitation-Counters.cpp:453-459). This applies in every power mode: SLEEPING does not test `lowPowerMode`.
- **Sleep length.** `wakeInSeconds = constrain(wakeBoundary - Time.now() % wakeBoundary, 1, wakeBoundary) + 1`, with `wakeBoundary = 3600` (src/Visitation-Counters.cpp:464, :253). The device wakes about one second after each UTC hour boundary, in `ULTRA_LOW_POWER` mode (:465-468).
- **User button during the closed-hours sleep.** It wakes the device, clears low power mode, and sets `openTime = 0` and `closeTime = 24`, which are then written to FRAM (src/Visitation-Counters.cpp:472-479). The device stays a 24-hour device until the hours are set again.
- **At opening.** On the first timer wake inside open hours, the sensor is enabled, the interrupt attached, and `stayAwake` set to 90000 ms (src/Visitation-Counters.cpp:481-485, :254). IDLE then sees the hour change and goes to REPORTING (:441), which runs `dailyCleanup()` and sends the zero report (:520-521).
- **`dailyCleanup()` at opening** (src/Visitation-Counters.cpp:1490-1505): turns verbose mode off (:1493); clears `clockSet` so the next connection syncs time and sends cellular information (:1494, :588-592); re-evaluates DST (:1495); **forces low power mode if `solarPowerMode` is set or state of charge is 65 or less** (:1497-1499); queues the Google Sheets event (:1501); runs `resetEverything()` (:1502).
- **Boot inside open hours.** The sensor is enabled and the interrupt attached; `stayAwake` is 90000 ms. The device goes to CONNECTING if it is not in low power mode and not backed off, or if a staged connection attempt is in progress (`currentConnectionLimit != 180`); otherwise IDLE (src/Visitation-Counters.cpp:413-427).
- **Boot outside open hours.** State is IDLE, and the first loop pass moves to SLEEPING (src/Visitation-Counters.cpp:427, :442).

## 4. Reporting cadence

- **Interval.** One report per local clock hour during open hours, triggered by the hour-change test in IDLE (src/Visitation-Counters.cpp:441). In low power mode the nap timer wakes the device at each UTC hour boundary to make this happen: `constrain(wakeBoundary - Time.now() % wakeBoundary, 1, wakeBoundary)` (:502). The interval is a compile-time constant, `wakeBoundary = 1*3600` (:253); no function changes it.
- **Report first, connect second.** REPORTING always calls `takeMeasurements()` and `sendEvent()`, which queues the event whether or not the device is connected (src/Visitation-Counters.cpp:519-521, :824). It then decides whether to connect (:522-549):
  - already connected: go to RESP_WAIT (:525-529);
  - low battery mode and user switch not pressed: do not connect (:531-534);
  - low power mode and user switch not pressed: connect every hour above 65 % charge; at 50 % or less only when `hour % 4 == 0`; at 51 to 65 % only when `hour % 2 == 0` (:536-548);
  - otherwise: CONNECTING (:522).
- **Skipped hours are not lost.** The hourly event is already in the publish queue and is sent on the next connection (src/Visitation-Counters.cpp:824; queue behaviour below).
- **Extra reports.**
  - `SendNow "1"`: publishes cellular information and forces REPORTING (src/Visitation-Counters.cpp:1186-1195).
  - User button while connected: the switch interrupt is attached on each successful connection (:606) and detached on disconnect (:1133); a press forces REPORTING and publishes cellular information (:709-713, :1027-1029).
  - At close, a last-hour report if `hourlyCount` is non-zero (:449-452).
  - On alert: when ERROR_STATE runs, it queues a separate `Ubidots_Alert_Hook` event (:643-644). Alerts that do not go through ERROR_STATE (10, 11, 13) are sent only in the next hourly payload (section 7).
  - On wake: there is no report on a count wake or on a closed-hours wake (:488-512, :441-442).
  - Once a day: the Google Sheets event at opening (:1501) and the location/signal event on the first connection after it (:588-592).
- **Webhook response wait.** RESP_WAIT sets `dataInFlight` on entry and waits up to `webhookWait = 30000` ms for the response handler to clear it (src/Visitation-Counters.cpp:553-561, :255). The handler accepts a response whose data is `200` or `201`, records `lastHookResponse`, and clears `dataInFlight` (:881-886). On timeout it raises alert 40 and goes to ERROR_STATE (:562-565).
- **Retries (publish queue).** All webhook events go through PublishQueuePosixRK (src/Visitation-Counters.cpp:374-375). The firmware calls `setup()` with no options, so library defaults apply:
  - RAM queue 2 events, file queue 100 events (lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h:367-368); when the file queue is over its limit, events are discarded from the front of the queue (lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:220-225; INFERRED: oldest first);
  - queue directory `/usr/pubqueue` (lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:369);
  - wait 2000 ms after connecting, 1000 ms between publishes, 30000 ms after a failed publish, then retry (lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h:381-383; lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:341-360). No retry limit was found in the retry path (lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:341-360).
  - The change log mentions "96 messages in POSIX queue" (src/Visitation-Counters.cpp:95), but no call sets a size in this source, so the default of 100 applies (INFERRED).
- **No re-send after a missing webhook response.** The firmware does not queue the data again on alert 40; it only decides whether to reset (src/Visitation-Counters.cpp:689-695).

## 5. Payloads, event names and Ubidots variables

All events are published `PRIVATE`. Numbers are integers unless stated.

### 5.1 Hourly counter event

- **Event name:** `Ubidots-Counter-Hook-v1`, flags `PRIVATE | WITH_ACK`, through the publish queue (src/Visitation-Counters.cpp:824).
- **Format string, exact** (src/Visitation-Counters.cpp:823):

```
{"hourly":%i, "daily":%i,"battery":%i,"key1":"%s","temp":%i, "resets":%i, "alerts":%i,"maxmin":%i,"connecttime":%i,"timestamp":%lu000}
```

| Order | Field | Source | Unit and scaling |
|---|---|---|---|
| 1 | `hourly` | `current.hourlyCount` | counts since the last report |
| 2 | `daily` | `current.dailyCount` | counts since the daily reset |
| 3 | `battery` | `sysStatus.stateOfCharge` = `int(fuelGauge.getSoC())` (:905) | percent, truncated to an integer |
| 4 | `key1` | `batteryContext[sysStatus.batteryState]` | string, one of "Unknown", "Not Charging", "Charging", "Charged", "Discharging", "Fault", "Diconnected" (spelling as in source, :240) |
| 5 | `temp` | `current.temperature` | degrees Fahrenheit, integer, from the TMP36 on A4 (:243, :945-953) |
| 6 | `resets` | `sysStatus.resetCount` | resets since the daily reset (:370, :1219) |
| 7 | `alerts` | `current.alerts` | one alert code (section 7) |
| 8 | `maxmin` | `current.maxMinValue` | see section 1 |
| 9 | `connecttime` | `sysStatus.lastConnectionDuration` | seconds |
| 10 | `timestamp` | `lastCountTime` or `lastReportedTime` (section 2) | Unix seconds with `000` appended, giving milliseconds, UTC |

- The buffer is 256 bytes (src/Visitation-Counters.cpp:815).
- `battery` is "Not Charging" (`batteryState` 1) whenever the temperature is outside the charging range, regardless of the PMIC state (src/Visitation-Counters.cpp:963).
- **Ubidots mapping.** The webhook definition (URL, request template, response template) is **NOT FOUND** in this repo. INFERRED: the Ubidots variable labels are the JSON keys `hourly`, `daily`, `battery`, `temp`, `resets`, `alerts`, `maxmin`, `connecttime`, with `key1` as the context of `battery` and `timestamp` as the dot timestamp. The handover note in the repo says the Ubidots events depend on `battery` (with `key1` context), `alerts`, `resets` and `connecttime` (Claude outputs/2026-11-11-alert-reporting-handover.md:47, :81).
- **Response route.** The device subscribes to events whose name begins with its own device ID (src/Visitation-Counters.cpp:300-303) and expects the data to be the HTTP status code (:877-881). The webhook's response topic and template are NOT FOUND in this repo.

### 5.2 Other events

| Event name | Payload (exact format) | When | Cite |
|---|---|---|---|
| `Ubidots_Alert_Hook` | `{"alerts":%i,"timestamp":%lu000 }` with `Time.now()`; `PRIVATE`, queued | ERROR_STATE; boot with 3 or more update attempts; daily reset clearing alert 23 | src/Visitation-Counters.cpp:643-644, :388-389, :1213-1214 |
| `Verbose_Counts_Hook` | `{"hourly":%i, "daily":%i}`; direct publish | each count, if verbose mode, connected and `verboseCounts` set | src/Visitation-Counters.cpp:797-799 |
| `Count` | `Count, hourly: %i, daily: %i` (text) | each count, if verbose mode, connected and `verboseCounts` clear | src/Visitation-Counters.cpp:792-794 |
| `GoogleSheetsExport` | JSON array of 10 strings: solar or utility, low power string, UTC offset, open time, close time, sensor type, verbose string, `"%i sec"` max connect time, `"%i%%"` min battery, SIM string; `PRIVATE | WITH_ACK`, queued | once a day in `dailyCleanup()` | src/Visitation-Counters.cpp:846-847, :1501 |
| `Daily-Location-Signal-Webhook` | `{"cellstr":%4.2f, "cellId":%li,"locationAreaCode":%li,"mobileCountryCode":%li, "mobileNetworkCode":%li}`; `PRIVATE | WITH_ACK`, queued; also calls `Particle.publishVitals()` | first connection of the day, `SendNow`, user button | src/Visitation-Counters.cpp:864-870, :591, :712, :1190 |
| `Cellular` | `Connected in %i secs` | on connection, verbose mode only | src/Visitation-Counters.cpp:603-605 |
| `State Transition` | `From %s to %s` | each state change, verbose mode and connected | src/Visitation-Counters.cpp:1472-1478 |
| `Ubidots Hook` | response text | webhook response, verbose mode and connected | src/Visitation-Counters.cpp:890-893 |
| `Mode`, `Time`, `Daily`, `Reset`, `Daily Cleanup` | text | configuration functions and daily cleanup | src/Visitation-Counters.cpp:1040, :1230, :1363, :1417, :1178, :1491; src/time_zone_fn.cpp:30-32 |

- `verboseCounts` is never set to true anywhere in the source; `loadSystemDefaults` sets it false (src/Visitation-Counters.cpp:1045). INFERRED: `Verbose_Counts_Hook` is sent only if the flag is already set in FRAM.
- Verbose mode is cleared every day at opening (src/Visitation-Counters.cpp:1493).

### 5.3 Particle variables

`HourlyCount`, `DailyCount`, `Signal`, `ResetCount`, `Temperature`, `Release`, `stateOfChg`, `lowPowerMode`, `OpenTime`, `CloseTime`, `Alerts`, `TimeOffset`, `BatteryContext`, `SensorStatus`, `SIM-Status` (src/Visitation-Counters.cpp:306-320).

## 6. Low-power and connection behaviour

- **System mode.** `SYSTEM_MODE(SEMI_AUTOMATIC)`; the cloud connection is made only by `Particle.connect()` in CONNECTING (src/Visitation-Counters.cpp:219, :580). There is no SYSTEM_THREAD call (:132).
- **Three power settings.**
  - `lowPowerMode` (src/sys_status.h:13): set by the `LowPowerMode` function (src/Visitation-Counters.cpp:1433-1461), forced on each day at opening for solar devices or at 65 % charge or less (:1497-1499), and forced on below 30 % (:917-920).
  - `lowBatteryMode` (src/sys_status.h:14): set when state of charge is below 30, cleared otherwise, on every `takeMeasurements()` (src/Visitation-Counters.cpp:917-921).
  - `solarPowerMode` (src/sys_status.h:12): default true (src/Visitation-Counters.cpp:1054); selects the charger configuration (:987-1007).
- **Nap (open hours, low power mode).** In IDLE, when `lowPowerMode` is set and `stayAwake` has elapsed, go to NAPPING (src/Visitation-Counters.cpp:440). NAPPING disconnects and powers the modem off if needed (:491-498), sets `stayAwake = 1000` (:499), stops the watchdog (:501), and sleeps in `ULTRA_LOW_POWER` (:503).
  - **Wake sources:** user switch (CHANGE), sensor pin `intPin` (RISING), and a timer to the next hour boundary (:504-506).
  - After a wake the watchdog resumes, the fuel gauge is woken, and the stay-awake timer restarts (:508-510). INFERRED: after a count wake the device stays awake about 1 second and naps again.
  - A user-switch wake clears low power mode, which starts a connection (:511, :1448-1453).
- **Stay-awake times.** `stayAwakeLong = 90000` ms after boot in open hours, at opening, when already connected at report time, and after each successful connection (src/Visitation-Counters.cpp:254, :416, :484, :526, :599). INFERRED: a low power device stays connected about 90 seconds after each connection, then naps.
- **Not in low power mode.** The device stays awake and connected during open hours and reports each hour over the existing connection (src/Visitation-Counters.cpp:440, :525-529). It still disconnects and sleeps outside open hours (section 3).
- **When it connects.** At boot in open hours if not in low power mode (src/Visitation-Counters.cpp:417-420); after each hourly report, subject to the battery rules in section 4 (:522-549); when low power mode is cleared (:1451-1453).
- **Connection timeout, staged with resets** (src/Visitation-Counters.cpp:583, :610-636). The elapsed time is compared with `current.currentConnectionLimit`, which is kept in FRAM:
  - limit 180 s exceeded: limit becomes 300, alert 32, `System.reset()` (:615-619, :685-687);
  - limit 300 s exceeded: limit becomes 420, alert 32, reset (:620-624);
  - limit 420 s exceeded: limit back to 180, alert 31, give up for this hour; if not in low power mode, `backOff` is set until the next report (:625-630, :518);
  - in any of the three cases, if `Cellular.ready()` is true the alert is overwritten with 30 (:632-635);
  - after a reset the device goes straight back to CONNECTING because the limit is not 180 (:421-424).
- **On success.** Sync time and publish cellular information if `clockSet` is false (src/Visitation-Counters.cpp:588-592). Add back the earlier stages to the duration: +180 if the limit was 300 or more, and a further +300 if it was 420 (:593-596). Reset the limit to 180 (:597), record `lastConnection` (:598), update `maxConnectTime` (:601), read signal strength (:602). Go to RESP_WAIT if the connection was started from REPORTING, otherwise IDLE (:607).
- **Disconnect.** `Particle.disconnect()` with a wait of up to 15000 ms; then `Cellular.disconnect()`, `Cellular.off()` and a wait of up to 30000 ms. If either step fails the function returns false and the caller raises alert 15 (src/Visitation-Counters.cpp:1128-1157, :454-458, :492-496). Cloud disconnect is graceful with a 5 s timeout (:337).
- **Low battery behaviour.**
  - Below 30 %: `lowBatteryMode`; no connection at all unless the user switch is held (src/Visitation-Counters.cpp:917-920, :531-534). Counting and hourly queuing continue.
  - 50 % or less: connect every fourth hour; 51 to 65 %: every second hour (:540-548).
  - Below 65 % with battery state "Not Charging": reset the PMIC configuration and raise alert 11 (:907-910).
- **Charging and temperature.** Charging is disabled and alert 10 raised when the enclosure temperature is below 36 °F or above 100 °F (src/Visitation-Counters.cpp:961-965). Solar charger settings: 900 mA source limit, 5080 mV minimum source voltage, 900 mA charge current, 4208 mV termination (:988-991). Utility settings: 900 mA, 4208 mV, 900 mA, 4112 mV (:1000-1003).
- **What is skipped in low power:** the cloud connection between reports, and therefore Particle functions, variables and verbose publishes, which all need a connection (:789, :1474-1475). Counting, FRAM writes and hourly queuing are not skipped (:707, :719-726, :824).
- **Watchdog.** AB1805 watchdog set to `WATCHDOG_MAX_SECONDS`, 124 s (src/Visitation-Counters.cpp:341; lib/AB1805_RK/src/AB1805_RK.h:758), stopped during sleeps and resumed after (src/Visitation-Counters.cpp:463, :469, :501, :508), serviced by `ab1805.loop()` in the main loop (:715).
- **Out of memory.** The handler stores the event parameter; the main loop then raises alert 14 and goes to ERROR_STATE (src/Visitation-Counters.cpp:304, :1012-1014, :728-732).

## 7. Alert codes

- **One code at a time.** `current.alerts` is a single `int` (src/Visitation-Counters.cpp:191). Each raise is a plain assignment, so the last writer wins; there is no severity order.
- **Cleared** after every hourly payload is built (src/Visitation-Counters.cpp:827). Alert 10 also clears itself when the temperature is back in range (:969). Alert 23 is cleared by `resetEverything()` (:1210-1212).
- INFERRED: `takeMeasurements()` runs just before `sendEvent()` (src/Visitation-Counters.cpp:519-521), so a 10 or 11 raised there replaces a pending code such as 31 before it is reported in the hourly payload. Out of range temperature sets battery state to 1 (:963), so alert 10 is replaced by 11 in the same call when charge is below 65 (:907-910).
- **ERROR_STATE** waits until `millis() > resetTimeStamp + resetWait` (30000 ms, src/Visitation-Counters.cpp:641, :256), queues `Ubidots_Alert_Hook`, waits 2000 ms, disconnects, then acts by code (:642-700). The connection-timeout path does not set `resetTimeStamp` (:613).

| Code | Meaning (header comment, src/Visitation-Counters.cpp:20-40) | Raised at | ERROR_STATE action | Ubidots/Slack event |
|---|---|---|---|---|
| 0 | Normal, no alert | cleared at :827 | none | none |
| 10 | Battery temperature too high or low to charge | src/Visitation-Counters.cpp:964 | none (not routed to ERROR_STATE) | none (handover :55) |
| 11 | PMIC reset required | src/Visitation-Counters.cpp:909 | none (not routed) | E30-11 |
| 12 | Initialization error (likely FRAM) | src/Visitation-Counters.cpp:354 | `ab1805.deepPowerDown()`, 30 s default (:651-653; lib/AB1805_RK/src/AB1805_RK.h:325) | E30-12 |
| 13 | Excessive resets (`resetCount > 8`) | src/Visitation-Counters.cpp:371 | reset count to 0, write FRAM, deep power down (:655-660) | E30-13 |
| 14 | Out of memory | src/Visitation-Counters.cpp:729 | `System.reset()` (:662-664) | E30-14 |
| 15 | Particle disconnect or modem power-down failure | src/Visitation-Counters.cpp:456, :494 | deep power down (:666-668) | E30-15 |
| 20, 21, 22 | Firmware update completed, timed out, failed: marked "deleted" | never raised | falls to default: `System.reset()` (:697-699) | E30-20, E30-21, E30-22 exist |
| 23 | Update attempt limit reached | src/Visitation-Counters.cpp:387 | not routed; default would reset | E30-23 |
| 30 | Cloud connection timed out but cellular connected | src/Visitation-Counters.cpp:634 | `System.reset()` (:670-672) | E30-30 |
| 31 | Failed to connect; skip to next hour | src/Visitation-Counters.cpp:627 | deep power down if more than `2 * 3600` s since `lastConnection`; otherwise back to IDLE (:674-683) | E30-31 |
| 32 | Failed to connect quickly; reset and keep trying | src/Visitation-Counters.cpp:617, :622 | `System.reset()` (:685-687) | none (handover :65) |
| 40 | No webhook response when connected | src/Visitation-Counters.cpp:563 | `System.reset()` if more than `3 * 3600` s since `lastHookResponse`; otherwise back to IDLE (:689-695) | E30-40 |

- **E30-xx mapping.** The firmware source does not document it. The only record in the repo is the handover note: Ubidots events exist for `alerts ==` 11, 12, 13, 14, 15, 20, 21, 22, 23, 30, 31 and 40, each posting to Slack (Claude outputs/2026-11-11-alert-reporting-handover.md:43). The Ubidots event definitions themselves are NOT FOUND in this repo.
- **Alert 13 does not reach ERROR_STATE** (INFERRED). `setup()` sets the code but not the state (src/Visitation-Counters.cpp:369-372), and every transition into ERROR_STATE sets its own code or follows one (:352-354, :455-456, :493-494, :563-565, :613-634, :729-731). So 13 is only reported in the next hourly payload, and the case at :655-660 is not reached.
- **Alert 12 may not reach ERROR_STATE either** (INFERRED). After a FRAM failure `setup()` continues; with zeroed settings the device counts as open and not in low power, so line 418 replaces ERROR_STATE with CONNECTING (src/Visitation-Counters.cpp:352-356, :413-420, :1074-1075).
- **Alert 23 is not reachable in normal running** (INFERRED). `updateAttempts` is never incremented in this source (declared :195; read :384, :1210; zeroed :1216), and the firmware-update state is declared but unused (:233, :216, :266).
- **Reset counting.** `resetCount` increases on a boot whose reason is `RESET_REASON_PIN_RESET` or `RESET_REASON_USER` (src/Visitation-Counters.cpp:369-370). INFERRED: `System.reset()` calls from alerts 14, 30, 32 and 40 count as user resets and so count toward alert 13.

## 8. State kept across a reset

- **Storage.** External I2C FRAM, declared `MB85RC64 fram(Wire, 0)` (src/Visitation-Counters.cpp:222). Map: version byte at `0x00`, `sysStatus` at `0x01`, `current` at `0x50` (:175-181). `FRAMversionNumber = 3` (:183).
- **No retained RAM and no EEPROM** are used; neither appears in the sources (INFERRED from reading all files under `src/`).
- **Write model.** Changes set `systemStatusWriteNeeded` or `currentCountsWriteNeeded`; the whole structure is written at the end of the loop pass (src/Visitation-Counters.cpp:719-726). Each count is therefore in FRAM within the same pass (:802). Direct writes happen in `loadSystemDefaults` and before a deep power down (:1056, :657, :677).
- **`sysStatus` fields** (src/sys_status.h:6-27): `structuresVersion`, `placeholdervalue`, `verboseCounts`, `clockSet`, `verboseMode`, `solarPowerMode`, `lowPowerMode`, `lowBatteryMode`, `stateOfCharge`, `batteryState`, `resetCount`, `timezone`, `dstOffset`, `openTime`, `closeTime`, `lastHookResponse`, `lastConnection`, `lastConnectionDuration`, `sensorType`, `verizonSIM`.
- **`current` fields** (src/Visitation-Counters.cpp:185-197): `hourlyCount`, `currentConnectionLimit`, `dailyCount`, `lastCountTime`, `temperature`, `alerts`, `maxMinValue`, `maxConnectTime`, `minBatteryLevel`, `updateAttempts`, `backOff`. The comment "currently 10 bytes long" is out of date (:185).
- **Survives a reset and a power loss** (FRAM is non-volatile): hourly and daily counts, last count time, the pending alert code, the staged connection limit, back-off flag, all settings, reset count. The publish queue is on the flash file system at `/usr/pubqueue` (lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:369), so events that have been written to files survive too; up to 2 events held only in RAM can be lost (lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h:367; INFERRED).
- **Lost on a reset (RAM only):** `lastReportedTime` (src/Visitation-Counters.cpp:261, rebuilt at :402-403), `sensorDetect` (:282), the pressure sensor's `frontTireFlag` (:1019), `dataInFlight` (:265), the state machine state (:235), and the minute tracker for `maxmin` (:773).
- **Time across a reset or power loss:** kept by the AB1805 RTC and restored at boot if the RTC was set (lib/AB1805_RK/src/AB1805_RK.cpp:33-38). The RTC is set from cloud time after a sync (lib/AB1805_RK/src/AB1805_RK.cpp:51-55).
- **Across a firmware update.** The FRAM is kept. If the version byte differs from `FRAMversionNumber`, the FRAM is erased and defaults are loaded, so counts and settings are lost (src/Visitation-Counters.cpp:346-356). If the byte matches, the structures are read as they are (:358-361). INFERRED: a layout change without a version bump would misread stored values; only the bounds checks at :1066-1081 guard against that.
- **Boot-time checks every boot** (src/Visitation-Counters.cpp:1066-1081): `sensorType > 1` becomes 0; `resetCount` outside 0 to 255 becomes 0; `timezone` outside -12 to 12 becomes -5; `dstOffset` outside 0 to 2 becomes 1; `openTime > 12` becomes 0; `closeTime` outside 12 to 24 becomes 24; `verizonSIM` is always cleared; `currentConnectionLimit < 180` becomes 180.
- **Defaults on first boot** (FRAM erased, then `loadSystemDefaults`, src/Visitation-Counters.cpp:1037-1057): `structuresVersion` 1; verbose mode and verbose counts off; low battery mode off; low power mode on unless the user switch is held (:1047-1048); time zone -5; DST offset 1; open 6; close 22; solar mode on; last connection duration 0.
  - Not set by defaults: `sensorType`, `resetCount`, `clockSet`, `lastHookResponse`, `lastConnection`. INFERRED: they are 0 (zero-initialised global at :213), so the sensor type is pressure.
  - `current` on first boot: `maxConnectTime` 0, `minBatteryLevel` 100, `updateAttempts` 0 from the initialisers (:193-195); other fields 0 (INFERRED); `currentConnectionLimit` raised to 180 (:1077); then `resetEverything()` runs because the stored day differs (:406-408).
- **Holding the user switch at boot** reloads the system defaults and leaves low power mode off (src/Visitation-Counters.cpp:393, :1047-1048). It does not zero the counts.

## 9. Configuration that changes behaviour

Particle functions (src/Visitation-Counters.cpp:322-334). All need a cloud connection.

| Function | Accepts | Effect | Default | Cite |
|---|---|---|---|---|
| `setDailyCount` | 0 to 2000 | seeds `dailyCount`, sets `lastCountTime` to now | n/a | src/Visitation-Counters.cpp:1407-1419 |
| `resetCounts` | "1" | zeroes daily, hourly and reset counts | n/a | src/Visitation-Counters.cpp:1159-1172 |
| `HardReset` | "1" | deep power down for 10 s | n/a | src/Visitation-Counters.cpp:1174-1184 |
| `SendNow` | "1" | forces a report and cellular information | n/a | src/Visitation-Counters.cpp:1186-1195 |
| `LowPowerMode` | "0", "1" | sets or clears low power mode; clearing starts a connection | on, unless switch held at defaults | src/Visitation-Counters.cpp:1433-1461 |
| `Solar-Mode` | "0", "1" | charger configuration; also drives the daily forced low power | on | src/Visitation-Counters.cpp:1223-1242 |
| `Verbose-Mode` | "0", "1" | extra publishes; cleared daily | off | src/Visitation-Counters.cpp:1313-1330 |
| `Set-Timezone` | -12 to 12 | UTC offset, whole hours | -5 | src/time_zone_fn.cpp:16-35 |
| `Set-DSTOffset` | 0 to 2 | DST hours | 1 | src/time_zone_fn.cpp:48-67 |
| `Set-OpenTime` | 0 to 23 | opening hour | 6 | src/Visitation-Counters.cpp:1352-1366 |
| `Set-Close` | 0 to 24 | closing hour | 22 | src/Visitation-Counters.cpp:1379-1391 |
| `Set-SensorType` | "0", "1" | pressure or PIR | 0 (pressure, INFERRED) | src/Visitation-Counters.cpp:1278-1299 |
| `setVerizonSIM` | "0", "1" | keep-alive interval; cleared at every boot in v54 | off | src/Visitation-Counters.cpp:1244-1265, :1076 |

No function changes a counting threshold, the debounce delay or the reporting interval; those are compile-time constants (src/Visitation-Counters.cpp:788, :253).

## 10. Not found in this repo

- The Particle webhook definitions for `Ubidots-Counter-Hook-v1`, `Ubidots_Alert_Hook`, `Verbose_Counts_Hook`, `GoogleSheetsExport` and `Daily-Location-Signal-Webhook`: URL, request template, response template and response topic.
- The Ubidots variable labels and event (E30-xx) definitions, apart from the summary in the handover note.
- A lifetime or total counter.
- Any PIR-specific timing: no hold-off, retrigger or minimum pulse width is set in firmware. Any such timing is in the sensor hardware.
- The exact v50 source. This tree is v54; the change log line at src/Visitation-Counters.cpp:132 is the only statement of how it differs from v50.

## Deviation register (GCC versus v50/v54)

Rows are added only for a recorded, conscious decision.

| Behaviour | v50 | GCC | Decision | Date |
|---|---|---|---|---|
| Debounce / lockout | Edges merged inside a `delay(200)` lockout, flag cleared after (`src/Visitation-Counters.cpp:788`, `:803`) | Any edge under 500 ms after the last accepted count is discarded (`src/sensors/PIRSensor.h:79`) | **Match v50: about 200 ms merge.** Not a deviation; GCC is to change (gap analysis, work item 5). Open until implemented. | 2026-11-24 |
| Daily rollover | At the opening hour, reset first, then the first report (`:520-521`); boot on a new local day also resets (`:406-408`) | At the close hour, report first then reset, once-per-day guard (`State_Report.cpp:65-79`, `DailyBoundary.cpp:35-42`) | **Open**, pending fleet data on overnight counts. Nothing in this row changes until Chip decides. | 2026-11-24 |
| Report timestamp | Last count time for a non-zero hour, else report time (`:823-827`; spec §2) | Always xx:59:59 of the previous clock hour (`src/cloud/ReportPublisher.cpp:65`, `:102`) | **Deviation kept (GCC timestamp).** Risk to check: two GCC reports in one clock hour carry the same timestamp (gap analysis UNKNOWN 2). | 2026-11-24 |
| Counting payload | `hourly`, `daily`, `battery` (integer value, with `key1` as its context), `temp` (integer °F), `alerts`, `resets`, `maxmin`, `connecttime`, `timestamp`; `key1` strings as in `batteryContext[]` (`:240`), including the legacy misspelling "Diconnected" | `battery` `%4.2f`, extra `vc`, `fh`, `lfb`, `cyc`, `slp`, `temp` Celsius `%4.2f`, "Disconnected", no `maxmin` (`ReportPublisher.cpp:51-55`, `:104-116`) | **Match v50:** same fields, integer battery, integer °F temp, `key1` strings identical to legacy (spelling included). Extra GCC fields go to the status ledger, not the webhook. `maxmin`: **dropped** (Chip, 24 Nov; confirms the 20 Oct decision), so the GCC counting template is the legacy template minus `maxmin`: `hourly`, `daily`, `battery` (value plus `context.key1`), `temp`, `alerts`, `resets`, `connecttime`. Webhook template (Chip, 24 Nov): `{"hourly":"{{hourly}}","daily":"{{daily}}","battery":{"value":"{{battery}}","context":{"key1":"{{key1}}"}},"temp":"{{temp}}","alerts":"{{alerts}}","resets":"{{resets}}","connecttime":"{{connecttime}}"}`. New webhook name proposed in the C2 list (C2-2). Open until implemented. | 2026-11-24 |
| `Ubidots_Alert_Hook` event on error state | Queued from ERROR_STATE (`:643-644`) | Not published | **Deliberate drop.** No legacy path is ported. | 2026-11-24 |
| `GoogleSheetsExport` | Published at the daily cleanup | Not published | **Deliberate drop.** | 2026-11-24 |
| `Daily-Location-Signal-Webhook` | Published (spec §5.2) | Not published | **Deliberate drop.** | 2026-11-24 |
| `Verbose_Counts_Hook`, `Count` and `State Transition` events | Published when enabled (spec §5.2) | Not published | **Deliberate drop.** | 2026-11-24 |
| Particle variables and functions (`HourlyCount`, `DailyCount`, `setDailyCount`, `resetCounts`, `SendNow`, ...) | Spec §5.3 and §9 | None; configuration is by ledger | **Deliberate drop.** Remote operations go through the ledger. | 2026-11-24 |
| Alert codes | Legacy numbering, `Visitation-Counters.ino:14-35` | GCC numbering, `docs/reference/alert-codes.md` | **GCC alert codes, each with its own Ubidots event where one is wanted.** Mapping recorded: legacy 10 to GCC 20; 11 to 21/22/23 (event `E30-21 (GCC)`); 13 to 17 (event `E30-17 (GCC)`) and 19; 15 kept as 15; 31 kept as 31; 40 kept as 40 with a wider meaning; 14 retired (reset cause 7); 12, 30 and 32 not raised in GCC; GCC-only 16, 18 (event `E30-18 (GCC)`), 41 to 44. Legacy 20 to 23 collide with GCC 20 to 23 and are handled by the separate "GCC Devices" group (checklist 12 Nov). Digest-only versus log-only per `docs/handover/2026-11-11-ubidots-gcc-devices-checklist.md`. Which GCC codes get an event is still the 11 Nov ruling; **No new Ubidots events for 16, 19 and 41 to 44** (architect's recommendation, pending Chip's OK): they are digest-only through the health check. | 2026-11-24 |
