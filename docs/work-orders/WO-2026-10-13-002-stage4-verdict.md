# NOT SOUND — gpt-5.6-sol (standard tier), high reasoning

Read-only source/history review at HEAD `47dc2e7`; no builds, tests, or edits were performed, as dispatched.

## Checks

| # | Verdict | Evidence-backed conclusion |
|---|---|---|
| 1 | **DISAGREE** | The payload table is mostly accurate, but Q1 conflates sensor mode with report kind, misses MEASUREMENT sampling behavior, understates the sleep/PIR gap, and incorrectly implies settings 3–4 have runtime readers. Details below. |
| 2 | **AGREE** | `git log --all` and `8f889ae:src/Generalized-Core-Counter.cpp:520–588` show no sensor-built payload. The initial tree did have a dead `SensorManager::getSensorData()` forwarding to `getData()` (`8f889ae:src/SensorManager.cpp:104–109`), removed by `29f9003`; restoring it alone would not restore report behavior. |
| 3 | **DISAGREE** | 2b-1 does not observe the queue call; 2b-3/4/5 have null-sensor and daily-close risks; fragment formatting can change truncation/evaluation; 2b-6a changes virtual dispatch and initialization timing. |
| 4 | **DISAGREE** | Extract-verbatim is sound in principle, but `Generalized-Core-Counter.cpp:1973–2051` omits `batteryContext` at `:1967–1971` and the actual `publish()` call at `:2053–2058`. The proposed input list also omits queue name/flags, locale, queue return, and explicit invalid-SOC boundaries. |
| 5 | **CONCERN** | `+15`, `+18`, and `+14` are optimistic once purity, null fallback, reset separation, and API wiring are included (`step0-report.md:153–156`). The cited 2b-2 move blocks total 45 physical lines (`:1967–1971`, `:2012–2051`), not about 75; the full construction region is 85 physical lines (`:1967–2051`). |
| 6 | **DISAGREE** | Corrected 2b-4 or 2b-6a/b could follow the golden test. All implementation is stacked on 2a: `git log` shows `f666d40` as HEAD’s ancestor, while `origin/main` is `de8faab`; therefore PR #94 must merge first or 2b must remain explicitly stacked. |
| 7 | **CONCERN** | Agree on removing 6a and 6b; move 6c/6d to a power cleanup WO. Keep the legacy accessor for now, but only `persistence_facade_behavior_test.cpp:238` truly consumes production behavior. The mutation test defines its own fake accessor (`sensor_type_source_test.cpp:32–36`), so `sensor_type_source_test.sh:114,120` does not require the production accessor to exist. |
| 8 | **DISAGREE** | Both cuts leave the settled design incomplete (`sensor-shape-step0.md:13–15`). Settings 3–4 have no runtime consumer, `samplingMode` is ignored at runtime, and no sensor operation requests a report or specifies its kind. |
| 9 | **CONCERN** | Recommendations for all open points appear below; several require sequence changes rather than simple acceptance. |

## Facts rechecked

**OBS — `ISensor` callers.** Bare-name and qualified searches find: `initializeHardware` at `SensorManager.cpp:307`; `getSensorType` at `:311,372`; `isReady` at `:312,318,352`; `usesInterrupt` at `:313,325`; `loop` at `:326,340,345`; `onSleep` at `:357`; and `onWake` at `:371`. `setup` is reached through `ISensor.h:120`; `reset` through `PIRSensor.h:52`.

**OBS — no callers.** Bare-name searches for `getData(`, `isHealthy(`, `lastErrorCode(`, and `toJSON(` find only declarations/overrides or definitions (`ISensor.h:66,95,147,152,156`; `PIRSensor.h:93`). `SensorData` itself is live as `PIRSensor::_data` (`PIRSensor.h:176`), so calling the type “uncalled” is inaccurate.

**OBS — payload order and sources.**

- OCCUPANCY has 14 keys in this order: `occupancy`, `dailyoccupancy`, `battery`, `vc`, `key1`, `temp`, `alerts`, `resets`, `connecttime`, `fh`, `lfb`, `cyc`, `slp`, `timestamp` (`Generalized-Core-Counter.cpp:2017–2030`).
- COUNTING and MEASUREMENT have 14 keys: `hourly`, `daily`, `battery`, `vc`, `key1`, `temp`, `resets`, `alerts`, `connecttime`, `fh`, `lfb`, `cyc`, `slp`, `timestamp` (`:2032–2049`). MEASUREMENT takes this `else` branch.

| Fields/formats | Source |
|---|---|
| `occupancy %d`, `dailyoccupancy %lu` | occupied boolean and total seconds divided by 60 (`:2014,2019–2020`) |
| `hourly %i`, `daily %i` | persisted counters (`:2038–2039`) |
| `battery %4.2f`, `key1 "%s"` | SOC and `batteryContext[battState]`; state above 6 becomes 0, and invalid/out-of-range SOC makes both SOC and state 0 (`:1967–1971,1983–1998`) |
| `vc %.2f` | starts at `0.0`, overwritten only for a valid cached sample (`:2008–2009`; `SensorManager.cpp:248–254`) |
| `temp %4.2f` | `CurrentReadings::get_internalTempC()` with no finite guard (`:2024,2043`); this is an environment field, not one of the listed common/system fields |
| `alerts/resets %i` | alert snapshot and reset count; their order swaps between branches (`:1990,2025–2026,2044–2045`) |
| `connecttime %i`, `fh/lfb/cyc/slp %lu` | system/config/runtime values (`:2027–2029,2046–2048`) |
| `timestamp %lu000` | occupancy uses nonzero override or `Time.now`; counting/measurement use end of the previous hour and ignore the override (`:1980–1981,2013,2033`) |

**OBS — missing wire detail.** `snprintf` writes into `char data[256]`, its return is ignored, and the queue receives the resulting C string (`:1973,2017,2036,2058`). Thus truncation at 255 payload bytes is part of current behavior and must be pinned.

## Findings

- **BLOCKER — OBS/INF:** The sequence does not deliver the settled six-operation shape. Q1 calls `sensorMode` the report “kind” (`step0-report.md:15`), but it only selects payload layout (`Generalized-Core-Counter.cpp:1989,2012–2033`). Actual trigger kinds are session flags and daily/health causes (`State_Report.cpp:166–243`). Cutting the report-request operation therefore leaves the agreed design unmet.
- **BLOCKER — OBS/INF:** Wake-pin substitution is insufficient for “new sensor without touching sleep code.” `intPin` has six live sleep uses (`State_Sleep.cpp:1365,1426,1457,1465,1520,1581`), and PIR-specific wake processing occupies `:1666–1806`, directly updating counts, occupancy, LEDs, report requests, and transition reasons. 2b-4 must cover wake metadata and wake-event dispatch, not four substitutions.
- **HIGH — OBS:** The golden range cannot compile verbatim without recreating `batteryContext`, and it stops seven lines before `publish()` (`Generalized-Core-Counter.cpp:1967–1971,2058`). Use an observer fake around the real queue invocation and assert event name, exact payload bytes, and `PRIVATE | WITH_ACK`.
- **HIGH — INF:** Required golden controls include every listed getter plus `Cloud::getWebhookName`, `PublishQueuePosix::publish` return/arguments, `millis`, C numeric locale, SOC `NaN`, infinities and both range failures, battery state above 6, valid/unavailable voltage, temperature `NaN/Inf`, all three sensor modes, and both override paths (`:1980–2059`). The `alert19` braced-block pattern (`alert19_clears_after_report_test.sh:19–39,80–87`) is a good extraction mechanism, but not the proposed boundary.
- **HIGH — OBS/INF:** 2b-5 must not reuse existing `ISensor::reset()`: PIR reset clears `_motionDetected` (`PIRSensor.h:114–120`), so a post-report interrupt could be lost. A distinct daily/hourly hook is required.
- **HIGH — OBS:** Daily reset is not sensor-only: `resetEverything()` also stamps `lastCountTime` and clears cross-store `resetCount` (`MyPersistentData.cpp:689–706`; `CurrentReadings.h:30–39`). Daily-close publishes before resetting and may restart an always-open occupancy session afterward (`State_Report.cpp:53–82`). Those operations and ordering must remain central.
- **HIGH — OBS/INF:** Null-sensor parity is mandatory. Closed boot intentionally leaves `_sensor == nullptr` (`Generalized-Core-Counter.cpp:1523–1535`), but the closing report still runs (`State_Report.cpp:44–77`). Field, wake and reset accessors need configured-PIR fallback that preserves today’s `intPin` behavior even when initialization was skipped or failed.
- **MEDIUM — INF:** Splitting one `snprintf` into fragments can change getter evaluation order, rounding inputs, intermediate-buffer truncation, and final NUL placement (`Generalized-Core-Counter.cpp:2017–2049`). Fixed-value fixtures alone will not detect getter timing/order changes; include call-order/count fakes and overflow-edge fixtures.
- **MEDIUM — OBS:** MEASUREMENT sampling is not “fully” covered: its Idle path samples only battery (`State_Idle.cpp:145–160`), while the active sensor is called incidentally during reporting (`State_Report.cpp:66`). `samplingMode` is stored and hashed (`ConfigApply.cpp:478–489`; `DeviceStatusPublisher.cpp:115`) but never drives `SensorManager.cpp:325`.
- **MEDIUM — OBS:** Settings 3–4 have no runtime consumers; bare searches find only persistence/config/hash accessors. Only setting1 and setting2 affect runtime (`State_Common.h:318–320`; `SensorManager.cpp:333–345`). The report’s “settings 1–4 are already read” claim is unsupported.
- **MEDIUM — INF:** 2b-6a preserves the PIR body but removes one virtual slot and replaces a default virtual forwarding call (`ISensor.h:79–83,115–120`) with direct override dispatch. ISR attachment order at `PIRSensor.h:40–53`, not merely an ELF symbol diff, needs parity coverage.
- **LOW — OBS:** 6b is safe in firmware terms: `sensorDetect` has one initialized declaration and two reads, with no writer (`Generalized-Core-Counter.cpp:156`; `State_Sleep.cpp:1094–1096`; `State_Idle.cpp:255`).
- **LOW — OBS:** 6c names four test stubs, not three: bare-name search finds `tests/stubs/power`, `pmic_fault_monitor_overrides`, `power_composition_overrides`, and `power_source_override_overrides`. Its two production functions span `PowerDiagnostics.cpp:133–169` plus declarations at `PowerDiagnostics.h:7–8`.
- **LOW — OBS:** 6d is written twice and never read (`PowerManager.h:62`; `PowerPlatform.cpp:123,131`), but it is unrelated power cleanup and changes `PowerCapabilities` layout.

## Six open points

1. **Golden approach:** keep 2b-1 first, but extract through the actual queue call and observe it; do not accept `:1973–2051`.
2. **`alerts`/`resets`:** preserve the branch-specific swap exactly (`Generalized-Core-Counter.cpp:2025–2026,2044–2045`).
3. **MEASUREMENT:** pin its current counting-format output (`:2032–2049`) until a separately approved behavior change.
4. **Temperature:** pin current `NaN/Inf` rendering in the baseline; any sanitizing belongs to another WO.
5. **Second sensor:** widen `sensor.type` validation only with a real compiled sensor and migration tests; today it accepts only PIR (`ConfigApply.cpp:349–359`; `SensorFactory.h:71–88`).
6. **Power leftovers:** move 6c/6d to a power-cleanup WO; keep sensor-related 6a/6b as separate commits.

## Recommended sequence

The binding stop condition applies: the corrected work exceeds the proposed six small WOs (`sensor-shape-step0.md:28–29`). Re-run Step 0 and split every implementation row to the `+20` ceiling (`AI_DEVELOPMENT_WORKFLOW.md:712–726`).

| WO | Goal | Est. net `src/` | Moves | Golden test | Depends on |
|---|---|---:|---:|---|---|
| **2b-1 CHANGED** | End-to-end queue-observed payload baseline | 0 | 0 | Exact queue arguments and payload matrix | PR #94 |
| **2b-6a/b MOVED EARLY** | Remove redundant setup slot and `sensorDetect`, separate commits | Negative; recount | 0 | Init/ISR parity plus golden | 2b-1 |
| **2b-2a NEW SPLIT** | Mechanical builder extraction, dependencies unchanged | ≤ +20 after Step 0 | Construction block | Same fixtures | 2b-1 |
| **2b-2b NEW SPLIT** | Introduce pure input snapshot without changing capture order | ≤ +20 after Step 0 | Input capture | Call-order plus byte fixtures | 2b-2a |
| **2b-3 CHANGED** | Sensor supplies ordered fields with configured-PIR null fallback | ≤ +20 after Step 0 | Sensor-field fragments | All modes, truncation edges | 2b-2b |
| **2b-4a NEW SPLIT** | Sensor supplies wake pin/edge/no-source metadata | ≤ +20 after Step 0 | Sleep substitutions | PIR sleep configuration parity | 2b-6a/b |
| **2b-4b NEW SPLIT** | Route wake event/sample outcome without PIR logic in sleep | Must be re-estimated/split | `State_Sleep.cpp:1666–1806` | Wake/count/occupancy parity | 2b-4a |
| **2b-5 CHANGED** | Separate hourly/daily hooks; retain central cross-store work | Must be re-estimated/split | Reset operations | Daily-close, null-sensor, ISR-pending | 2b-3 |
| **2b-7 NEW** | Deliver settings 1–4 and sampling mode to the sensor shape | ≤ +20 per split | Existing reads | PIR behavior unchanged | 2b-6a/b |
| **2b-8 NEW** | Sensor requests report and kind; preserve current trigger causes | Must be re-estimated/split | Existing trigger logic | Periodic/change/daily/service parity | 2b-4b |
| **Separate power WO** | Remove 6c/6d | Negative; recount | 0 | Power host tests and ABI review | PR #94 |