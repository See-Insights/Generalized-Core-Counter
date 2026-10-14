# WO-2026-10-13-002 Step 0 round 2: re-split 2b

**Base commit:** `25dbc69` (branch tip; main `7a5f5e7` with #94 merged is inside it; `git diff --stat 47dc2e7 HEAD -- src tests` is empty, so Stage 4 read this same `src/`) · **Date:** 2026-10-14 · **Model used:** claude-sonnet-5-5, reasoning: high (set by `--effort high`)
Read-only. Every `src/` file:line below was re-opened at HEAD. OBS = observed, INF = inferred. Bare-name and qualified searches both run (§13 (c)).

## Q1: Corrections to the Stage 4 findings
| Finding | Verdict | Evidence at HEAD and effect on the sequence |
|---|---|---|
| BLOCKER report request/kind cut | AGREE | `sensorMode` only picks the layout (`Generalized-Core-Counter.cpp:1989,2012`). The only "kind" is `session.occupancyChangeTriggered` (`StateMachine.h:58`): 5 writers (`State_Modes.cpp:86,154`, `State_Idle.cpp:71`, `State_Sleep.cpp:1660,1737`), readers `State_Report.cpp:73,230`, `State_Idle.cpp:88`, `State_Sleep.cpp:497`. Request gets its own WO (2b-6). |
| BLOCKER wake pin | AGREE on sites, DISAGREE on remedy | 6 live `intPin` sites (`State_Sleep.cpp:1365,1426,1457,1465,1520,1581`; round 1 said 4) and no edge or no-source metadata. But the PIR-wake block (`:1666–1753`) branches only on `get_sensorMode()` (`:1671,1687`), entered via `pirWake` (`:1670`). Once `pirWake` derives from the sensor's pin it serves any interrupt sensor, so no wake-event operation is needed (INF; relocation is Open point 2). |
| HIGH golden range | AGREE | Region is `1967–2058`, 92 lines (batteryContext `:1967–1971`, build `:2012–2051`, queue call `:2058`). Round 1's "≈75 moved" was wrong: 45 physical lines, 85 for the whole construction. |
| HIGH controls | AGREE | Extended in Q3 (host `unsigned long` width, libc NaN, flag values). |
| HIGH `reset()` reuse | AGREE | `PIRSensor.h:114–120` clears `_motionDetected`; `setup()` also calls it (`:52`). A distinct hook is required (2b-5a). |
| HIGH daily reset not sensor-only | AGREE | `MyPersistentData.cpp:695–706` also stamps `lastCountTime` and clears `resetCount`; `initialize()` reuses it (`:772`). Order publish `:69` → `dailyCleanup()` `:77` → restart `:78–81` stays central. |
| HIGH null sensor | AGREE | Closed boot leaves `_sensor` null (`Generalized-Core-Counter.cpp:1523–1535`), also when the factory returns null (`SensorManager.cpp:299–302`); the closing report still publishes (`State_Report.cpp:69`). All accessors resolve through a configured-type fallback (2b-3). |
| MEDIUM fragment formatting | AGREE, resolved by design | 2b-3 keeps one `snprintf`; only the sensor prefix is pre-rendered (integer-only, ≤40 bytes). |
| MEDIUM MEASUREMENT sampling | AGREE | Idle samples battery only (`State_Idle.cpp:145–160`); the sensor's `loop()` runs at report time (`State_Report.cpp:66`). Enough for a read-at-report sensor; scheduled sampling deferred. |
| MEDIUM settings 3–4, samplingMode | AGREE | setting3/4: no runtime reader (only `ConfigApply.cpp:388,396`, hash `DeviceStatusPublisher.cpp:105–106`); setting2 only `SensorManager.cpp:336`; samplingMode only stored (`ConfigApply.cpp:479–484`) and hashed (`:115`). Round 1's "already read" was wrong for 3–4. |
| MEDIUM 6a dispatch/ISR | AGREE | No host test compiles `PIRSensor.h` (only a stub, `sensor_type_source_test.cpp:39`); the parity test is new. |
| LOW 6b / 6c / 6d | AGREE | `sensorDetect` also has `extern` at `StateMachine.h:36` (not in the verdict). 6c/6d are power code and leave 2b (`PowerDiagnostics.cpp:133–169`, `PowerManager.h:62`, `PowerPlatform.cpp:123,131`). |
| Checks 2–5, 7 | AGREE | Check 6 is resolved: `f666d40` is an ancestor of HEAD and #94 is merged, so nothing is stacked. |

**New (OBS):**
- `Config.cpp:94–101` makes `setting1 == 0` invalidate the config for every sensor type, which forces connect at `State_Report.cpp:171` (Open point 5).
- `ConfigApply.cpp:1–6` includes no sensor header; the setting1 getter logs at trace on every call (`MyPersistentData.cpp:621`).
- **Dropped from Stage 4's sequence:** builder extraction and input snapshot (its 2b-2a/2b-2b). They are not in the agreed design and the goal does not need them: the single `snprintf` keeps bytes identical by construction, and the golden harness already compiles the real text.

## Q2: The full sequence, no cuts (nine WOs, each ≤ +20 net `src/`)
Counts are from drafted code and carry about ±3 lines; recount at each WO's Step 0. No WO needs a persisted field; all state added is RAM.

**2b-1 Golden payload test (test-only).** Files: `tests/report_payload_golden_test.{sh,cpp}`, `tests/fixtures/report_payload_golden.txt`; delete the stale mirror in `watchdog_alert_code_test.cpp:100–135`. `src/` 0, moves 0, test budget ≈ +300/−40. Spec in Q3.

**2b-2 Start: 2a leftovers (two commits).** 6a `setup()` merge: `ISensor.h:79–84` removed (−6), `:120` becomes `= 0` (0), `PIRSensor.h:39` renamed (0). 6b `sensorDetect`: `Generalized-Core-Counter.cpp:156` −1, `StateMachine.h:36` −1, `State_Sleep.cpp:1094–1096` and `State_Idle.cpp:255` simplified (0; log text loses `sensor=%d`). Net **−8**, moves 0. Test: new `tests/pir_sensor_lifecycle_test` (fake `pinMode/digitalWrite/attachInterrupt/detachInterrupt` recording order for `initializeHardware/onSleep/onWake`, captured ISR sets the flag); its commit passes on the unchanged code first. Handles init and ISR parity.

**2b-3 Add its fields, with the null-sensor fallback.**
- Per file: `ISensor.h` +3 (`reportFields()` default empty); `PIRSensor.h` +1; `PIRSensor.cpp` +14 (2 includes, 10-line function, 2 blanks); `SensorManager.h` +4; `SensorManager.cpp` +14 (`activeSensor()` 9, `reportFields()` 5); `publishData` −40 (`:2012–2051`) +22.
- Net **+18**, gross +58/−40; split ISensor+PIR | manager+publishData if gross must stay ≤40.
- The new `snprintf` is `"{%s\"battery\"…\"%s\":%i,\"%s\":%i,…"` with the alerts/resets names as arguments. The prefix buffer is `char[48]` (worst case 39 + NUL).
- `activeSensor()` = `_sensor`, else the factory for `SystemConfig::SensorSettings::get_sensorType()` (the source `sensor_type_source_test.sh:114,120` pins), else PIR.
- Moves 0. Test: golden fixtures unchanged, fake `reportFields` forwards to the real `PIRSensor.cpp`; vectors with `_sensor == nullptr`, type valid and type invalid; call-count and no-allocation asserts. Re-point `report_payload_fields_test.py:86,175`; remove `daily_cleanup_boundary_test.py:236–245`; add the new read site to the mutation list. Depends on 2b-1.

**2b-4 Wake sources from the sensor.**
- Per file: `ISensor.h` +4 (`WakeSource{pin,edge}`, default `{PIN_INVALID, RISING}`); `PIRSensor.h` +2; `SensorManager.h` +2; `State_Sleep.cpp` +9 (local +2, `sensorCanWake` +1, `:1365` +2, `:1465` +2, `:1520` +2; `:1426,1457,1581` 0 net).
- Net **+17**, moves 0.
- `pirWake = sensorCanWake && wakePin == sensorWake.pin`: a timer wake may report an invalid pin, which must not match a sensor with no pin.
- Test: PIR `{intPin, RISING}`, null-sensor same, no-source fake `PIN_INVALID`; extract both sleep-config statements and compare the `gpio` calls ([BUTTON] vs [BUTTON, pin]); zero `intPin` tokens left in `State_Sleep.cpp`. Depends on 2b-3 (`activeSensor()`).

**2b-5a Reset hook on the sensor (not wired).** `ISensor.h` +4 (`ResetPeriod{Hourly,Daily}`, default no-op), `PIRSensor.h` +1, `PIRSensor.cpp` +11. Net **+16**. Test: recorded setters are Hourly `[hourly 0]` and Daily `[hourly, daily, occupied false, lastOccupancyEvent 0, occupancyStartTime 0, totalOccupiedSeconds 0]` (the order of `MyPersistentData.cpp:699–706`); ISR fired before either period still yields `loop() == true`. Handles ISR-pending.

**2b-6 Wire the reset (central work stays central).**
- Per file: `MyPersistentData.cpp` `resetEverything(bool includeSensorTotals = true)` returns after `lastCountTime` and `resetCount` (+4); `.h` and `CurrentReadings.h` docs +2; `SensorManager.h` +2, `.cpp` +4 (`resetTotals()` via `activeSensor()`); `dailyCleanup` `:2736` becomes `resetEverything(false)` plus the hook (+1); `State_Report.cpp:101` calls the hook (0, only inside the `COUNTING` guard).
- Net **+13**. `initialize()` (`:772`) keeps the default `true`.
- Test: `persistence_facade_behavior_test.cpp:223` plus `(false)` cases; `daily_cleanup_boundary_test.py:162` token updated, order test kept; payload-after-reset vectors; `_sensor == nullptr` still reaches the hook. Depends on 2b-5a, 2b-3.

Order note: 2b-5b in the closing table is this WO, written as 2b-6 here; the request WO follows as 2b-7.

**2b-7 Ask for a report and its kind.** `ISensor.h` +4 (`ReportRequest{None,Change}`, default `None`), `SensorManager.h` +2, `.cpp` +4 (uses `_sensor`, ready only, so no factory call per loop pass), main loop after `:1722` +7 (sets `occupancyChangeTriggered`, IDLE→REPORTING, gated by `reportsOccupancyChangesNow()` `State_Common.h:55`). Net **+17**. PIR never overrides, so the path is inert. Test: fake sensor, extraction harness. Depends on 2b-3.

**2b-8a Settings 1–4 and samplingMode delivered at start.** `ISensor.h` +6 (`SensorParams`, default no-op `configure()`), `SensorManager.h` +3, `.cpp` +10 (`currentParams()` 8, call before `initializeHardware()` `:307` +2). Net **+19**. PIR ignores them, so `usesInterrupt()` (`SensorManager.cpp:325`) is unchanged. Test: fake records args equal the persisted values.

**2b-8b Delivery on ledger change.** `SensorManager.h` +2, `.cpp` +5, `ConfigApply.cpp` +5 (include and two calls after `:398` and `:489`). Net **+12**. Adds a cloud→sensors include (INF). Depends on 2b-8a.

## Q3: The golden test (2b-1)
- **Boundary:** two literal markers, `static const char *batteryContext[7] = {` (`:1967`) through `bool queued = PublishQueuePosix::instance().publish(webhookName, data, PRIVATE | WITH_ACK);` (`:2058`), wrapped as `bool run(time_t stampOverride)`; `EXTRACT_FAILED` if either is missing or not unique (the `alert19_clears_after_report_test.sh:19–39` pattern). The markers stay valid across 2b-3; only the region shrinks. Post-queue code (`:2060+`) stays with the alert19 test.
- **Observer fake:** `publish(const char *name, const char *data, PublishFlags f1, PublishFlags f2 = PublishFlags())` (`lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h:181`), recording call count (must be 1), name string and pointer (== the fake `getWebhookName()` return), data bytes and length, raw flags. Flag constants use the Device OS values mirrored in `lib/StorageHelperRK/automated-test/UnitTestLib/Particle.h:203–206` (PRIVATE 1, WITH_ACK 8): expect `f1 == 0x9`, `f2 == 0`. Do not reuse `tests/stubs/diag_overrides/PublishQueuePosixRK.h:10` (PRIVATE = 0 cannot detect a dropped PRIVATE).
- **Controlled inputs, each with a call counter asserted exactly once:**
  - `Time.now/minute/second`, `System.freeMemory()`, `HAL_Core_Runtime_Info`, `AwakeCycles::cycles/sleeps`, `millis()`;
  - `PowerManager::batteryState/soc`, `SensorManager::cachedBatteryVoltage` (bool and value);
  - `SystemConfig::get_sensorMode/get_lastConnectionDuration`, `RecoveryState::get_alertCode/get_resetCount`, `CurrentReadings` occupied, total, hourly, daily, internal temp;
  - `Cloud::getWebhookName`, `stampOverride`, queue return;
  - Stage 4's additions: locale pinned to C, a global `operator new` counter asserting no heap use in the region (protects `fh`).
- **Vectors (≈90):**
  - mode {0,1,2} × stampOverride {0, nonzero} × (min,sec) {(27,14),(0,0)};
  - alert {−128,−1,0,19,42,127} in both layouts (pins the swap `:2025–2026,2044–2045`);
  - SOC {NaN, ±Inf, −0.01, 0, 87.5, 100, 100.01} × battState {0,2,6,7,255};
  - vc {unavailable, 3.91, 4.20};
  - temp {23.4, −999.99, NaN, ±Inf, ±FLT_MAX} plus a 1e0..1e38 sweep that straddles 255/256 bytes (pins truncation: worst-case 237 B, FLT_MAX overflows 255);
  - all-max widths; name {"", configured, convention}; queue false.
- **Host caveats (INF):** host `unsigned long` is 64-bit, so keep every `%lu` input ≤ 4294967295 (the existing `report_payload_fields_test.py` uses 9999999999 as text only). NaN/Inf strings are host-libc; expected bytes are recorded from the unchanged code on the same host and committed, never regenerated.
- **Existing tests:** supersede `watchdog_alert_code_test.cpp:100–135` and `daily_cleanup_boundary_test.py:236–245`. Coexist with `alert19_clears_after_report_test.sh`, `occupancy_report_by_mode_test.sh` (only anchors `:235`), `report_payload_fields_test.py` (width and unquoted-number rule, re-pointed in 2b-3) and `clock_resync_wiring_test.py:635–652`.

## Q4: Size verdict
Nine WOs (eight with positive net, one test-only), net ≈ +104 `src/`. That is over about six, so cuts are needed.
- **Option A (recommended): six WOs** = 2b-1, 2b-2, 2b-3, 2b-4, 2b-5a, 2b-6 (net +56). Defer the request (2b-7) and settings/samplingMode (2b-8a/b) to the first new-sensor WO.
  - Leaves unmet vs the plain goal: nothing literal; fields, wake and reset need no report, sleep or connect edit.
  - Leaves unmet vs the agreed design: a sensor cannot itself ask for an early report; settings 3–4 and samplingMode reach a sensor only through the public accessors (no central code touched), and samplingMode stays ignored at runtime as today.
  - Rationale: `getData/isHealthy/lastErrorCode` have sat uncalled since `8f889ae`; interfaces with no consumer drift.
- **Option B: eight WOs** = A + 2b-7 + 2b-8a (net +92). Settings take effect at the next sensor start; defers only the ledger-push (2b-8b), the cloud→sensors include and the log-spam risk. Leaves unmet: change-time delivery of settings.

## Q5: STOP
The uncut 2b is nine WOs, over the spec's "about six" stop rule, and choosing a cut changes the agreed design, which is the architect's and Chip's call. No behavior-neutrality or persisted-field blocker was found: Option A is ready to dispatch as soon as it is chosen.

## Open points
1. Builder extraction (Stage 4's 2b-2a/b) is dropped. If wanted for device-data after Step 6, add two WOs (new file, ≈45 lines moved) with no goal gain.
2. Moving the PIR-wake block (`State_Sleep.cpp:1666–1753`) to `State_Modes.cpp` would be ≈ +7 net, 88 moved; not needed, and `clock_resync_wiring_test.py:641` pins it in `State_Sleep.cpp`.
3. `alerts`/`resets` order stays per mode, now carried as data; normalising it changes bytes (architect). MEASUREMENT keeps the counting layout; `temp` stays unguarded (pinned, not fixed).
4. "Fields grouped as device-data" holds by order (sensor, battery, environment, system); code-level grouping waits for the device-data step.
5. `Config.cpp:94–101` ties setting1 to config validity for every type; recovery-plan item, not 2b.
6. Device-only unknowns (INF): `PIN_INVALID` and `wakeupPin()` on timer wake (no repo hit for `PIN_INVALID`); newlib NaN rendering; `fh` heap parity (bench check). `createSensor()` logs an error per call for an unknown type.
7. 6c/6d go to a separate power WO (≈ −40).

| WO | Goal | Est. net `src/` | Moves | Test | Depends on | Risk handled |
|---|---|---:|---:|---|---|---|
| 2b-1 | Pin today's bytes through the real queue call | 0 | 0 | New golden (≈90 vectors), observer fake | none | baseline for all; truncation, flags |
| 2b-2 | `setup()` merge, `sensorDetect` removal | −8 | 0 | New PIR lifecycle parity + golden | none | init and ISR parity |
| 2b-3 | Sensor supplies fields; `activeSensor()` fallback | +18 | 0 | Golden + null-sensor vectors, call counts | 2b-1 | null sensor, getter order, truncation |
| 2b-4 | Wake pin and edge from the sensor | +17 | 0 | Wake parity, sleep-config extraction | 2b-3 | null sensor, no-source guard |
| 2b-5a | PIR reset hook (not wired) | +16 | 0 | Setter-order and ISR-pending test | 2b-1 | ISR-pending |
| 2b-6 | Route hourly/daily reset; central cross-store stays | +13 | 0 | Persistence + daily-close order + post-reset vectors | 2b-5a, 2b-3 | daily-close order, null sensor |
| 2b-7 (cut in A) | Sensor requests a report and kind | +17 | 0 | Fake sensor via extraction | 2b-3 | PIR inert |
| 2b-8a (cut in A, B) | Settings 1–4 and samplingMode at start | +19 | 0 | Fake records args | 2b-3 | init parity |
| 2b-8b (cut in A, B) | Deliver on ledger change | +12 | 0 | ConfigApply change test | 2b-8a | layering, trace log |
| Power WO (outside 2b) | 6c/6d removal | ≈ −40 | 0 | Power host tests | none | none |
