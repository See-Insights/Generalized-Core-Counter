# WO-2026-10-07-001: Step 6 extensibility evidence

- **Base:** main `41ad238`. `src/` and `lib/` are identical at HEAD `db8a429` (diff verified by the caller). Line numbers below are working-tree lines = lines at `41ad238`.
- **Date:** 2026-10-07. **Stage:** 2 (Evidence), read-only. One file written (this one).
- **Model used:** Sonnet 5.5 (`claude-sonnet-5-5`), reasoning: high as requested; a subagent cannot set reasoning explicitly. It was handled by the harness default; I compensated by re-opening every cited line before finishing.
- **Conventions:** `Main` = `src/Generalized-Core-Counter.cpp`. **OBS** = read in source. **INF** = inferred (marked). Paths are relative to `src/` unless they start with `docs/`.

**Plain goal:** find where today's firmware handles sensors, payloads, wakes and temperature, so the architect can map the Step 6 plan onto the extensibility requirements.

## 1. Payload fields (webhook report vs data ledger)

**The template is not in the repo (OBS).** Searched: `grep "{{"` over the repo, webhook/template/JSON file names, `docs/`, and `particle-fleet-operations/docs`. The only hits are the planned `"vc": "{{vc}}"` line (`docs/RECOVERY_PLAN_2026-09-26.md:89`). **The field list below is DERIVED from the `snprintf` formats (`Main:2018-2019` occupancy, `Main:2037-2038` counting), not read from the template.**

The firmware sends 14 keys per mode (occupancy and counting differ in 2 names), 16 distinct keys across both modes. Minus `vc` (not in the template per the recovery plan) that is 13 per mode, not 12. **I cannot tell which one is not in the template (gap, section 8).** Webhook name comes from `cloud/Cloud.cpp:647-673` (configured name, else `occupancy-webhook-v1` / `counting-webhook-v1` / `measurement-webhook-v1`); queued at `Main:2059`.

Webhook = `publishData()` (`Main:1966`), `char data[256]` (`Main:1974`). Ledger = `Cloud::publishDataToLedger()` (`cloud/DeviceStatusPublisher.cpp:474-637`, buffer 512 at `:54`, `deviceDataLedger.set()` at `:593`). `cloud/LedgerClient.cpp` only creates the ledgers (`:38`, `device-data`) and handles sync callbacks (`:79-86`); it does not build the data payload. Ledger write callers: `Main:2106`, `state/State_Connect.cpp:573`, deferred retry `cloud/Cloud.cpp:713`.

All "serialized" cites for the webhook are the argument lines in `Main`; the occupancy format is `:2020-2031`, the counting format `:2039-2050`.

| Key (webhook) | Source variable | Set (assigned) | Webhook serialized (occ / count) | Ledger |
|---|---|---|---|---|
| `occupancy` | `CurrentReadings::occupied` | `state/State_Modes.cpp:65`, `State_Sleep.cpp:1695`, `State_Report.cpp:74`; cleared `State_Common.h:401`, `MyPersistentData.cpp:703` | `:2020` / absent | `occupancy.occupied` `DeviceStatusPublisher.cpp:514` (bool) |
| `dailyoccupancy` | `totalOccupiedSeconds` (sent as minutes) | `State_Common.h:374`; reset `MyPersistentData.cpp:706` | `:2015,2021` / absent | `occupancy.totalOccupiedSec` `:515` (**seconds**, not minutes) |
| `hourly` | `hourlyCount` | `State_Modes.cpp:29`, `State_Sleep.cpp:1660`; reset `State_Report.cpp:87`, `MyPersistentData.cpp:699` | absent / `:2039` | **absent** |
| `daily` | `dailyCount` | `State_Modes.cpp:30`, `State_Sleep.cpp:1661`; reset `MyPersistentData.cpp:700` | absent / `:2040` | **absent** |
| `battery` | `stateOfCharge` (local copy of `PowerManager::soc()`) | stored `sensors/SensorManager.cpp:712,768,899`; local `Main:1995`, NaN/range guard `:1996-1999` | `:2022` / `:2041` | `battery.soc` `:521` (1 decimal, **no NaN guard**) |
| `vc` | local `vc` from `SensorManager::cachedBatteryVoltage()` | cache `SensorManager.cpp:750-752`; local `Main:2009-2010` | `:2023` / `:2042` | **absent** |
| `key1` | `batteryContext[battState]` | `battState` `Main:1985`; stored `SensorManager.cpp:711,757,918,1205`; table `Main:1968` | `:2024` / `:2043` | **absent** |
| `temp` | `internalTempC` | `SensorManager.cpp:1011,1032,1094` (see section 4) | `:2025` / `:2044` | `environment.temperature` `:518` (1 decimal) |
| `alerts` | `reportedAlertCode` = `RecoveryState::get_alertCode()` | local `Main:1991`; raised `MyPersistentData.cpp:895-902` | `:2026` / `:2046` | **absent** |
| `resets` | `sysStatus.resetCount` | `++` `Main:1107`; cleared `MyPersistentData.cpp:696` | `:2027` / `:2045` | **absent** (ledger has `system.resetReason`) |
| `connecttime` | `lastConnectionDuration` | `state/State_Connect.cpp:252,328` | `:2028` / `:2047` | **absent** |
| `fh` | `System.freeMemory()` read at serialization | `Main:2029/2048` (no stored variable) | `:2029` / `:2048` | `system.freeHeap` `:524` (separate read at `:508`) |
| `lfb` | `rtInfo.largest_free_block_heap` | `Main:2004-2006` | `:2029` / `:2048` | **absent** |
| `cyc`, `slp` | `AwakeCycles::cycles/sleeps` | `observability/AwakeCycleCounters.h:17-18`; `++` via `recordSleepReturn` at `State_Sleep.cpp:1157,1410,1462,1477` | `:2030` / `:2049` | **absent** |
| `timestamp` | `stampOverride`/`Time.now()` (occ) or end-of-previous-hour (count) | `Main:1981-1982,2014,2034` | `:2031` / `:2050` (printed as `%lu000`, ms) | `timestamp` `:512` (**seconds**, `Time.now()` only) |

- **Ledger-only keys (OBS):** `schemaVersion` `:511`, `system.resetReason` `:525`, `system.resetReasonData` `:526`.
- **Ledger has no counting fields (OBS):** `DeviceStatusPublisher.cpp:510-528` has no mode branch; it always writes the occupancy block, even in counting mode, and never `hourly`/`daily`.
- **Unit/format differences (OBS):** minutes vs seconds (`dailyoccupancy`), ms vs s (`timestamp`), 2 vs 1 decimals (`battery`, `temp`).
- **Template wording conflict (OBS):** `docs/work-orders/WO-2026-10-02-001-recovery-visibility.md:99` says "Ubidots will create a variable for each new top-level key", which reads as a pass-through integration; the turnover (`docs/handover/2026-10-07-step6-turnover.md:115`) says the template names each field. Possibly different integrations (Ubidots vs AWS copy); not established.

## 2. Daily totals

| Item | Where |
|---|---|
| Reset function | `dailyCleanup()` `Main:2713-2734`; the reset is `CurrentReadings::resetEverything()` `Main:2733` -> `currentStatusData::resetEverything()` `MyPersistentData.cpp:689-707` |
| Trigger (OBS) | `handleReportingState()` `state/State_Report.cpp:31`: `DailyBoundary::check(now)` `:39` (`time/DailyBoundary.cpp:26-46`: trusted clock, `boundary = todayAt(closeTime)`, due when `lastDailyCleanup < boundary`). Then `dailyCleanup()` `:72`, `set_lastDailyCleanup(now)` `:77`. Only caller. |
| Order (OBS) | Open session closed at boundary `:58-60`; report published with `boundary - 1` `:69` **before** the reset `:72`; if `close == 24` and was occupied, the session is reopened at the boundary `:73-76`. |
| Other call sites | `currentStatusData::initialize()` `MyPersistentData.cpp:772` (new install/invalid file); `validate()` zeroes `hourly/daily` if >10000/>100000 `:713-716`, `totalOccupiedSeconds` if over a day `:722-725`. `State_Sleep.cpp:994` only diverts to REPORTING when the close is due. |
| Hourly reset (counting only) | `State_Report.cpp:86-88`, after each report. |

| Counter (`persist/CurrentReadings.h`) | Belongs to |
|---|---|
| `hourlyCount`, `dailyCount`, `lastCountTime` (`MyPersistentData.h:690-691`) | Counting mode (any sensor that raises an event; PIR today) |
| `occupied`, `lastOccupancyEvent`, `occupancyStartTime`, `totalOccupiedSeconds` (`:694-697`) | PIR occupancy mode |
| `sysStatus.resetCount` (`MyPersistentData.h:127`) | Not a sensor counter; cleared by the same daily reset (`MyPersistentData.cpp:696`) |
| RAM `AwakeCycles` | Not reset daily (reset by MCU reset) |

`resetEverything()` zeroes both the counting and occupancy sets regardless of mode (`:699-706`). MEASUREMENT mode has no counters (OBS: `State_Idle.cpp:131-144` only samples battery).

## 3. Wakes

**Configured before sleep (all in `state/State_Sleep.cpp`, `handleSleepingState()` `:323`):**

| Path | Wake sources configured | Cite |
|---|---|---|
| Night HIBERNATE (Boron, closed hours) | **RTC (AB1805):** alarm `ab1805.interruptAtTime(wakeTime)` `:1118`, stale alarm cleared first `:1116` (fn `:239`), eligibility `shouldUseBoronRtcAlarmHibernate` `:269`; GPIO `WAKEUP_PIN` FALLING `:1125`. **Button:** `BUTTON_PIN` FALLING `:1126`. **No PIR** (PIR rails are cut by `onEnterSleep()` `:1003`, `PIRSensor.h:130-139`). |
| Day/default ULTRA_LOW_POWER | Button FALLING `:1352`; PIR `intPin` RISING `:1353`; **timer** `.duration()` `:1367` (Device OS timer, not the AB1805); cellular standby `:1363` |
| STOP fallbacks | same GPIOs + duration `:1451-1454`; timer-only `:1467-1469` |
| Boot-storm guard | timer-only ULP, `Main:881-884` |
| USB | **No USB wake source configured anywhere (OBS, grep).** USB is only a power source (`power/PowerManager.cpp:32-36`) and serial re-init after wake (`State_Sleep.cpp:1527-1546`). |

**Pin definitions:** `intPin` `device_pinout.cpp:63/68/76` (S2 or SCK), `BUTTON_PIN` `:42` (D4), `WAKEUP_PIN` `:44` (WKP). PIR ISR attach `sensors/PIRSensor.h:50,158` (`pirISR` sets a flag, `PIRSensor.cpp:11-14`).

**Read after sleep:**
- **ULP/STOP (returns inline):** `SystemSleepResult result = System.sleep(ulpConfig)` `:1409`; `result.wakeupPin()` `:1412`; label at `:1413-1414`; error branch `:1443`. Dispatch booleans `pirWake/buttonWake/timerWake` `:1569-1572`.
- **HIBERNATE:** a wake resets the MCU, so the reason is read in `setup()`: `System.resetReason()` `Main:792`; `HibernateCycle::classifyWake(reason, ab1805)` `Main:1268` (`time/HibernateCycle.cpp:44`, uses `ab1805.getWakeReason()`); results `Main:1351-1393`. A returning `System.sleep(hibernateConfig)` `:1156` is treated as failure `:1159-1164`.

**Dispatched after sleep (ULP path):**

| Wake | Handling |
|---|---|
| Button | `onExitSleep()`, post-wake battery, `serviceRequestTriggered`, to REPORTING `State_Sleep.cpp:1585-1592`. Awake presses: `userSwitchISR` `Main:2694`, handled `Main:1705-1710`. ISR re-attached `:1564`, setup `Main:1591`. |
| PIR | Counting: synthesize one count `:1659-1673`. Occupancy: open session / restart debounce `:1674-1740`. Then opportunistic report if overdue `:1771-1788`; PIR in non-CONNECTED returns to sleep `:1793-1796`. |
| Timer | To REPORTING `:1752-1764` (occupied+KEEP_ALIVE suppression `:1756-1760`). |
| Sensor re-init | `onExitSleep()` `:1601` / `initializeFromConfig()` `:1603` only inside open hours `:1597`. |

OBS: `sensorISR` (`Main:2696`, legacy tire counter) is declared and defined but never attached; `sensorDetect` is therefore never set true (`grep sensorISR`: no `attachInterrupt`). Wake routing today is by pin comparison in `State_Sleep.cpp`, not per sensor (the only sensor-owned wake pieces are the ISR and rails in `PIRSensor.h`).

## 4. Temperature (TMP36 and friends)

**Sampled and stored (OBS):** one function, `SensorManager::measureTemperatureAndApplyChargeDecision()` `sensors/SensorManager.cpp:938-1239` (declared `SensorManager.h:316`). TMP36: 8 inline ADC reads on `TMP36_SENSE_PIN` (`device_pinout.cpp:34-39`) `:1053-1062`, convert `:1069` (`tmp36TemperatureC` `:403`), range check/fallback `:1078-1092`, stored `CurrentReadings::set_internalTempC` `:1094` (also TMP112 `:1001-1011`, P2 stub `:1014-1032`). Persisted field `internalTempC` `MyPersistentData.h:682`, accessor `MyPersistentData.cpp:817-823`.
**Triggered by `batteryState()`** (`SensorManager.cpp:484`): called from `Main:1572` (setup), `State_Report.cpp:67`, `State_Sleep.cpp:1224,1588,1607`, `State_Idle.cpp:141`, `State_Connect.cpp:542`. Coupled call sites: `SensorManager.cpp:729` (cellular non-M-SoM) and `:931` (others).

| Reader of the TMP36 value | File:line | Charging decision | Payload / ledger |
|---|---|---|---|
| Thermal inhibit decision (uses the **local** `tempC` / `measuredThisCall`, not the stored field) | `SensorManager.cpp:1173-1175` -> `power/ChargeInhibitPolicy.h:168-178`; apply `:1201-1202` -> `power/ChargeInhibit.cpp:11-39` | **Yes** | No (see note) |
| `safeToCharge` return into PMIC fault handling and forensics | `SensorManager.cpp:729,736,776` | **Yes** (remediation gating) | No |
| Webhook `temp` | `Main:2025,2044` | No | **Payload** |
| Ledger `environment.temperature` | `DeviceStatusPublisher.cpp:518` | No | **Ledger** |
| `PMIC_ANOM` log | `SensorManager.cpp:815` | No (log) | No |
| PMIC remediation log text | `power/PmicFaultMonitor.cpp:265` | No (log; it acts on `safeToCharge`) | No |
| Fallback seed/reuse of the stored value when a read fails | `SensorManager.cpp:971,1007,1022,1079` | Indirect (INF: a fallback gives `measuredThisCall=false`, so the policy cannot arm or release, `ChargeInhibitPolicy.h:171-177`) | Yes (fallback value is what gets reported) |
| Charge-cycle test (dead code, section 7) | `SensorManager.cpp:1297,1340` | Safety gate only | No |

- **Note (OBS):** the decision also reaches the payload indirectly: `set_batteryState(1)` when inhibited `SensorManager.cpp:1205`, which becomes `key1` (`Main:1985,2024`).
- **OBS:** thermal thresholds are persisted `PowerConfig::get_thermalCharge*` (`SensorManager.cpp:1123-1128`), settable via ledger `power.thermalChargeInhibit` (`cloud/ConfigApply.cpp:610-651`).
- **OBS:** the stored value is the only temperature the payload/ledger read; no other sampler exists. `externalTempC` exists (`MyPersistentData.h`/`CurrentReadings.h:93`) but nothing in `src/` outside the persistence layer writes or reads it (grep).

## 5. Occupancy versus counting mode

**Selection (OBS):** persistent field only. `sysStatus.sensorMode` `MyPersistentData.h:146` (0 counting, 1 occupancy, 2 measurement); default COUNTING `MyPersistentData.cpp:144`; enum `persist/SystemConfig.h:39-43`; accessors `SystemConfig.h:154-155`. Written only by ledger apply `cloud/ConfigApply.cpp:431-437` (range 0-2). **No compile-time flag** selects the mode (`BuildProfile.h` and `Config.h` define none; the retired `ENABLE_PMIC_CHARGE_CYCLE_TEST` is unrelated, `BuildProfile.h:282-283`). Sensor *type* is separate (section 6).

| # | Divergence | File:line |
|---|---|---|
| 1 | Event dispatch (MEASUREMENT gets neither) | `Main:1717-1722` -> `State_Modes.cpp:25-49` (count) vs `:59-113` + `updateOccupancyState` `:123-159` |
| 2 | Webhook payload format and timestamp rule | `Main:2013-2031` vs `:2033-2050` |
| 3 | Report log line | `Main:2126-2140` |
| 4 | Hourly counter reset after report (counting only) | `State_Report.cpp:86-88` |
| 5 | Debounce expiry, LED upkeep (occupancy) | `State_Idle.cpp:45-83` |
| 6 | Report-while-occupied gating | `State_Idle.cpp:182-188`, `State_Sleep.cpp:1756,1775` |
| 7 | Low-power idle sleep deferral (counting LED) | `State_Idle.cpp:238` |
| 8 | Measurement-mode sampling (battery only) | `State_Idle.cpp:131-144` |
| 9 | Sleep entry: counting LED defer / occupancy sleep length | `State_Sleep.cpp:1080-1092` / `:1095-1108` |
| 10 | Post-wake: occupancy LED-timeout close; PIR wake counted vs occupied | `State_Sleep.cpp:1636-1652`; `:1659-1740`; counting LED off `:1746` |
| 11 | Webhook name convention (if none configured) | `cloud/Cloud.cpp:655-670` |
| 12 | Low-battery KEEP_ALIVE downgrade, occupancy only | `power/BatteryAuthorityCommand.cpp:76` |
| 13 | Dead legacy divergence: `sensorISR` keyed on sensor type | `Main:2696-2703` (never attached) |

No mode branch in the ledger writer (`DeviceStatusPublisher.cpp:510-528`). `samplingMode` (INTERRUPT/POLLING, `SystemConfig.h:68-71`) is persisted, ledger-applied (`ConfigApply.cpp:484-488`) and hashed (`DeviceStatusPublisher.cpp:115`) but read by no behavior (grep).

## 6. Sensor settings

**Persistent (OBS):** third store `/usr/sensor.dat`, class `sensorConfigData` `MyPersistentData.h:456`; struct `SensorData` `:504-517`: `type` `:512`, `setting1..4` `:513-516` (PIR debounce ms in `setting1`). Defaults `MyPersistentData.cpp:601-605` (type 1, setting1 = `Config::DEFAULT_OCCUPANCY_DEBOUNCE_MS` 60000, `Config.h:34`). Facade `SystemConfig::SensorSettings` `persist/SystemConfig.h:232-262`; forwards `MyPersistentData.cpp:1101-1124`. Related `sysStatus` fields: `sensorType` `MyPersistentData.h:135`, `sensorMode` `:146`.

**Consumers (OBS):** `setting1` = occupancy debounce (`state/State_Common.h:301-304`, `State_Modes.cpp:69,92`, `State_Sleep.cpp:1098,1714,1732`, `Config.cpp:94,150`); `setting2` = polling interval for non-interrupt sensors (`SensorManager.cpp:347`); `setting3/4` are read nowhere except ledger apply and the config hash (`DeviceStatusPublisher.cpp:105-106`).

**How changed:**
- **Ledger/config apply: yes.** `Cloud::applySensorConfig` `cloud/ConfigApply.cpp:333-409` (`sensor.type` `:351-357`, `setting1-4` `:364-402`), called from `applyConfigurationFromLedger` `:123,133`, triggered from `Cloud::loop()` `cloud/Cloud.cpp:678-686` after ledger sync callbacks (`cloud/LedgerClient.cpp:46-68`). Validation `Config.cpp:94` requires `setting1 != 0`.
- **Particle function: no.** `Particle_Functions.cpp:40-56` registers only two variables (`thrashTrips`, `thrashResets`); `grep Particle.function` over `src/` and `lib/` finds none.
- **Initialization only:** the defaults at `MyPersistentData.cpp:601-605`.

**Finding (OBS, then INF):** the ledger writes `SensorSettings::type` (`ConfigApply.cpp:354`, the `/usr/sensor.dat` field), but sensor creation reads the **other** field: `SensorManager::initializeFromConfig()` `SensorManager.cpp:307` and `Main:1127` use `SystemConfig::get_sensorType()` = `sysStatus.sensorType` (`MyPersistentData.cpp:1044`). Nothing writes `sysStatus.sensorType` except its default 1 (`MyPersistentData.cpp:136`); `set_sensorType` has no callers outside persistence. **INF:** a ledger `sensor.type` change does not change which sensor is built; `SensorFactory` implements only PIR (`SensorFactory.h:73-74`, others return null `:85-87`). `initializeFromConfig` runs only at boot/wake/idle-enable (`Main:1526`, `State_Idle.cpp:28`, `State_Sleep.cpp:1603`), never after a config apply.

## 7. Dead-code list (Codex WO 2) at `41ad238`

Codex observed at `85bd316`; `git diff 85bd316 41ad238 -- src` shows only `FirmwareVersion.h` and `Main` (+2 lines at `:1313`, +2 at `:1945`). So every non-`Main` citation is unchanged; `Main` cites shift by +2 (`:1313-1944`) or +4 (`:1945` onward).

| Item (Codex cite) | Current file:line | Status |
|---|---|---|
| `SensorManager::setup` (`SensorManager.cpp:287`) | `:287-296`, decl `SensorManager.h:63`; no callers (only a doc example `SensorManager.h:45`) | STILL ACCURATE |
| `getSensorData` (`:362`) | `:362-367`, decl `.h:82`; no callers (doc example `.h:47`) | STILL ACCURATE |
| `getSignalStrength` (`:1241`) | `:1241-1272`, decl `.h:196`; no callers | STILL ACCURATE |
| `runPmicChargeCycleTest` (`:1241-1418` span) | function `:1284-1418` = **135 lines**; banner comment `:1274-1282` (+9) and blank `:1283`; decl+doc `SensorManager.h:198-208`; no callers in `src/`, `tests/`, `lib/` | STILL ACCURATE. Note: not behind a flag; guard is platform only (`:1285`). `ENABLE_PMIC_CHARGE_CYCLE_TEST` survives only as a retired-bit comment `BuildProfile.h:282-283`. |
| Cloud backoff wrappers (`BatteryBackoff.cpp:15-45`) | `:15-17` `getIntervalMultiplier`, `:20-45` `getConnectionBackoffMultiplier`; decls `cloud/Cloud.h:378,392`; no callers in `src/`, `tests/` | STILL ACCURATE |
| PowerManager capability/availability/context wrappers (`PowerManager.cpp:218-227,238-243`) | `capabilities` `:218-220`, `hasPmic` `:222-224`, `hasFuelGauge` `:226-228`; `availabilityLabel` `:238-240`, `batteryContextLabel` `:242-244`; no callers (`PowerManager::hasPmic` not called; `PowerDiagnostics.cpp:244` calls `PowerPlatform::hasPmic`) | STILL ACCURATE (Codex end lines omit each closing brace, `:228`, `:244`) |
| `PowerPlatform::hasFuelGauge` (`PowerPlatform.cpp:157`) | `:157-159`, decl `PowerPlatform.h:43`; no callers | STILL ACCURATE |
| `PowerTier::label` (`PowerTier.cpp:22`) | `:22-34`, decl `PowerTier.h:48`; no callers in `src/` or `tests/` | STILL ACCURATE |
| `SensorFactory::getSensorTypeName` (`SensorFactory.h:96`) | `:96-116`; no callers | STILL ACCURATE |
| Unused `PowerInputs` (`PowerManager.h:129-133`) | `:129-133`, referenced nowhere else | STILL ACCURATE |
| Write-only `setupComplete_` (`PowerManager.h:259`; `.cpp:98`) | `.h:259`; init `.cpp:89`, only write `.cpp:98`, never read | STILL ACCURATE |
| PIR ISR count (`PIRSensor.cpp:8,13`) | `PIRSensor.cpp:8,13`, decl `PIRSensor.h:183`; written, never read | STILL ACCURATE |
| Duplicate codes (`PowerManager.cpp:11-16`, `PowerPlatform.cpp:10-15`, `PowerDiagnostics.cpp:18-23`) | same | STILL ACCURATE |
| Duplicate masks (`SensorManager.cpp:77-78`, `PmicFaultMonitor.cpp:19-20`) | same | STILL ACCURATE |
| Battery labels (`SensorManager.cpp:3`, `Main:1964`) | `SensorManager.cpp:3`; `Main:1968` | **MOVED** (Main +4) |
| Tier labels (`BatteryAuthorityCommand.cpp:24-26`, `ReportingPolicy.cpp:96-107`, `State_Sleep.cpp:55`) | `:24-27`; `:96-108` (closing brace `:108`); `:55` | STILL ACCURATE (end lines off by one) |
| Fresh-temperature / thermal citations (`SensorManager.cpp:938-957,1158-1202`) | `:938-957`, `:1158-1202` | STILL ACCURATE |

Charge-cycle test, `SensorManager.cpp:1284-1418`: exactly **135 lines** (function only). Removing the banner and blank as well is 145 lines (`:1274-1418`); with `getSignalStrength` `:1241-1418` is 178. Header declaration and doc add 11 lines (`SensorManager.h:198-208`).
Other Codex `Main` cites in the report shift too (e.g. `Main:2004-2021` is now `:2008-2025`, `Main:2612-2671` now `:2616-2675`); not re-verified individually.

## 8. Open points / gaps

1. **Template not found.** The 12 template fields are not established; the 16-key table in section 1 is derived. Arithmetic: 14 keys per mode now, 13 without `vc`, 11 distinct without `vc` and the four heap/cycle keys (`fh`, `lfb`, `cyc`, `slp`). None of these is 12. Chip needs to supply the template text (or confirm which keys it omits).
2. Whether `fh/lfb/cyc/slp` reach Ubidots through the template or by pass-through is not established (section 1 conflict).
3. The `sensor.type` finding (section 6) is OBS for the code path, INF for the field effect; no device or ledger check was done.
4. Not verified: whether the linker drops the uncalled functions in section 7 (Codex cites a Stage 7 ELF check for the RSSI helper only).
5. M-SoM/Photon 2 branches were read for temperature only (`SensorManager.cpp:1014-1033`); none compiled.
6. `src/` change needed: **none**. No stop condition met.

## Closing record

| Item | Budget | Raised to (reason) | Actual net src/ lines | Tests |
|---|---|---|---|---|
| Evidence report | 0 | — | 0 | none run (out of scope) |
