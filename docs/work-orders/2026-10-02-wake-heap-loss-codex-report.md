**Found a definite per-cycle leak: the global sleep configuration loses ownership of its allocated wake-source nodes when it is reset by assignment.** This is the strongest explanation for Dev-09’s declining heap. The source proves the leak; it does **not yet prove that it accounts for every byte of the measured 126–141 B/cycle**.

Investigated `43b8a69`; its `src/`, `lib/`, and `project.properties` are unchanged from `691b6c7`. The workflow is at the repository root, not `docs/`. No edits, builds, commits, deletions, or device actions were performed.

Model: **gpt-6-astra**. Reasoning: **high**.

The ranked findings are:

| Rank | Suspect and source | Evidence and estimated loss | Confidence |
|---|---|---|---|
| **1** | Global configuration reset: [State_Sleep.cpp:1202](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:1202), invoking [Device OS move assignment:205](/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/system/inc/system_sleep_configuration.h:205) | Assignment overwrites the previous linked-list pointer without freeing it. Each subsequent configuration allocates two GPIO nodes, one RTC node, and, when standby is effective, one network node. Estimated minimum **104 B/cycle with standby**, **72 B without**, under the expected small-enum target layout. Allocator behavior can increase these figures; details below. | **Certain source defect; high confidence as principal cause.** |
| **2** | Additional allocation retention inside sleep/network restoration: [system_sleep.cpp:114](/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/system/src/system_sleep.cpp:114) | Runs every sleep, making it a reasonable alternative for any residual loss. The inspected promise, semaphore, wake-result, interrupt, USB, and peripheral paths have cleanup or reuse. **No additional unfreed per-cycle allocation identified; bytes unknown if one remains.** | Low as an additional cause. |
| **3** | Report/ledger/queue allocations: [Generalized-Core-Counter.cpp:2043](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:2043) | Events, queue nodes, JSON/ledger data and transport work allocate conditionally. These can change heap between report samples, including positive recovery, but do not execute on every ordinary PIR→Sleep cycle. **Variable per report; no demonstrated fixed per-wake leak.** | Low for the observed wake-correlated slope. |
| **4** | Unpublished PowerDiag/ChargeDiag accumulation: [PowerDiagnostics.cpp:86](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/power/PowerDiagnostics.cpp:86) | Fixed storage, bounded count, and capture compiled out with the release-default flag. **0 B of growing heap per wake**, even if capture were enabled. | High confidence this explanation is excluded. |

The configuration ownership failure is explicit:

- `config` is global: [Generalized-Core-Counter.cpp:131](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:131).
- `config = SystemSleepConfiguration()` invokes move assignment. Device OS copies the incoming empty configuration over the destination and nulls the incoming pointer. **It never frees the destination’s previous list.**
- Only the configuration destructor walks and deletes that list: [system_sleep_configuration.h:213](/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/system/inc/system_sleep_configuration.h:213). The global destination is not destroyed by assignment; the temporary is empty.
- `.gpio()`, `.duration()`, and `.network()` allocate new nodes at OS header lines **276, 332, and 433**. They could reuse existing matching nodes, but the assignment has already discarded their list.
- The same ownership failure occurs on the fallback resets at application lines **1443 and 1460**. Those can add loss during failed sleep attempts.

Thus the allocation occurs during **preparation for the next sleep**, after wake handling. The first configuration is live storage; subsequent resets orphan earlier configurations.

The byte estimate comes from the structures in [sleep_hal.h:110](/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/hal/inc/sleep_hal.h:110): expected GPIO 16 B, RTC 16 B, network 20 B. The allocator adds an 8-byte header and rounds to 8-byte alignment:

- Two GPIO + RTC: `24 + 24 + 24 = 72 B`.
- With network standby: `72 + 32 = 104 B`.

These are **minimum block costs, not measured device sizes**. [heap_4_lock.c:318](/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/hal/src/portable/FreeRTOS/heap_4_lock.c:318) keeps the whole free block when splitting would leave at most 16 bytes. Four leaked allocations can therefore consume **104–168 B** under that layout. The observed slope fits that range, but attributing the difference to allocator slack remains an inference. `lfb` tracking `fh` does not exclude this internal allocation slack.

The reported `+616 B` interval also does not disprove a leak: other allocations can be released during that interval.

The successful occupied-wake path, with allocation ownership, is:

| Step | Execution and allocation findings |
|---|---|
| **1. `System.sleep()` returns** | [State_Sleep.cpp:1402](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:1402). The HAL allocates one GPIO or RTC wake-reason structure at OS `hal/src/nRF52840/sleep_hal.cpp:185` or `:200`. The wiring wrapper copies the result into the system’s last-result object, then returns another copy (`wiring/src/spark_wiring_system.cpp:62`). The original is freed on scope exit; the previous system copy is freed on replacement; the application’s local copy is freed when this handler returns. Cleanup is explicit in [spark_wiring_system.h:312](/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/wiring/inc/spark_wiring_system.h:312). **Transient allocation, plus one bounded retained result—not an accumulating list.** |
| **2. Counters and wake classification** | Application lines **1403–1407** update integer counters and inspect the result. No allocation. `cyc` actually increments after **every sleep return, including errors**; `slp` increments only on success (`observability/AwakeCycleCounters.h:17`). Failed calls must be separated in bench analysis. |
| **3. Pins, watchdogs, progress, optional diagnostic flush** | Application lines **1498–1517** restore pins/watchdogs and overwrite fixed progress/breadcrumb fields. `ThrashGuard::markProgress()` copies into a fixed character array (`ThrashGuard.cpp:44`). A pdiag flush is conditional; its payload buffer is stack storage, although publishing it would allocate a queue event. |
| **4. USB serial restoration** | Application line **1523** calls `Serial.begin()`, followed by bounded waits and optional `Particle.process()`. OS `hal/src/nRF52840/usb_hal.cpp:78` attaches USB; its TX/RX buffers are allocated only when their static pointers are null (`:61`). **Reused buffers, not a new pair each wake.** Background processing may complete unrelated work. |
| **5. Button interrupt reattachment** | Application line **1557** passes a plain function pointer. [spark_wiring_interrupts.cpp:85](/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/wiring/src/spark_wiring_interrupts.cpp:85) uses the raw-handler overload, with no `new`. The nRF HAL updates fixed channel storage. Even the unused `std::function` overload deletes the prior wrapper before allocating its replacement (`:34`). |
| **6. Cycle reset and `PowerDiag … post-wake`** | Application lines **1568–1573** overwrite one static `WakeCycleStats` object, mark battery stabilization pending, read power data, and log. `PMIC` objects here are automatic objects using existing I²C infrastructure. No growing container. |
| **7. Sensor wake** | [SensorManager.cpp:387](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/sensors/SensorManager.cpp:387) calls the existing sensor’s `onWake()`. `PIRSensor.h:144` powers pins and reattaches the raw ISR. The fallback factory returns the same static PIR singleton (`SensorFactory.h:71`, `PIRSensor.h:30`), not a newly allocated sensor per wake. |
| **8. Battery, `post-refreshInputProfile`, ChargeDiag** | [SensorManager.cpp:719](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/sensors/SensorManager.cpp:719) refreshes the power profile, measures temperature, polls/remediates PMIC faults and logs ChargeDiag. `PowerManager.cpp:191` applies a power configuration only when invalid/changed, then replaces its fixed report structure. Charge/trend history consists of scalar baselines (`PmicFaultMonitor.cpp:449`). No per-wake heap history found. |
| **9. Occupancy and LED handling** | Application lines **1629–1732** update stored counters/timestamps and restart the LED deadline. `signalLED()` writes a pin and scalar deadline (`device_pinout.cpp:97`); it does not create a timer object. Occupancy messages use literals and numeric arguments. |
| **10. Sleep→Sleep / Sleep→Report and LoopStage** | [State_Sleep.cpp:1745](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:1745) reports on a timer only when the occupied suppression rule permits it; a PIR wake can also report if overdue. Otherwise it takes `sleep-pir-return-to-sleep` at **1787**. `transitionTo()` at `Generalized-Core-Counter.cpp:2472` closes scalar timing state and logs LoopStage/StateReq. No transition-history allocation. |
| **11. Remainder of `loop()`** | [Generalized-Core-Counter.cpp:1637](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:1637) services ThrashGuard, RTC, deferred persistence, Cloud, publish queue and occupancy. Persistence uses open/write/close; Cloud’s deferred work requires pending work **and** connection (`cloud/Cloud.cpp:674`). A disconnected publish queue waits without creating another event (`PublishQueuePosixRK.cpp:274`). |
| **12. Optional reporting branch** | `State_Report.cpp:66` measures and calls `publishData()`. Its webhook JSON is a stack `char[256]`; queue events use `new char[]` (`PublishQueuePosixRK.cpp:112`), and queue containers grow per event. Release sites exist after disk persistence and publish completion (`:147`, `:363`, `:375`). Ledger JSON is parsed into owning `LedgerData` at `cloud/DeviceStatusPublisher.cpp:538`; transport/filesystem work may remain pending. These are report-dependent allocations. |
| **13. Next sleep preparation** | Gates and teardown precede `logTimeDiag()` at application line **987**. `logTimeDiag()` constructs a local timezone converter (`Generalized-Core-Counter.cpp:1853`), whose timezone-name `String` copies allocate; their destructors free them. Steady clock checks use `LocalTimeCache.cpp:39`. The reset at **1202** then loses the previous configuration list; `.gpio()` at **1344**, optional `.network()` at **1356**, and `.duration()` at **1360** allocate the next list. Pre-sleep battery/diagnostic logs, watchdog suspension and serial drain lead to the next **1402** call. |

For the logging calls throughout this path, **long formatting does not create an ever-growing heap buffer**. Device OS formats `Log` messages into a fixed stack buffer in [logging.cpp:80](/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/services/src/logging.cpp:80). `Serial.printf()` likewise uses stack buffers (`wiring/src/spark_wiring_print.cpp:221`). `String` storage, where actually used by timezone/version helpers, has explicit destruction/free at `spark_wiring_string.cpp:197`.

The PowerDiag index feeds **only the printed counter**: declaration at `PowerDiagnostics.cpp:230`, increment at **318**, formatting at **320/331/340/349**. It does not select a storage slot. The separate batch:

- Has 12 fixed entries, approximately **240 static bytes**.
- Stops appending at capacity and increments a saturating dropped counter.
- Resets its count after flushing.
- Is guarded together with its capture calls by `ENABLE_DIAGNOSTICS_PUBLISH_MODE`.

Release defaults set that flag to zero through [BuildProfile.h:241](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/BuildProfile.h:241). This is a source-level conclusion; I did not certify the flashed binary’s flags.

Device OS also allocates a system-thread promise, its semaphore, and potentially callable storage for the synchronous sleep request (`system/inc/active_object.h:172`, `:332`). The caller deletes the promise at `system/inc/system_threading.h:105`; its destructor destroys the semaphore. GPIO/RTC hardware re-arming walks existing configuration nodes and updates registers; it does not require another configuration list. Peripheral restoration reuses existing driver state, and BLE initialization is guarded against repeating initialization (`hal/src/nRF52840/ble_hal.cpp:3814`).

I searched for a matching published Particle issue and did **not find one establishing this configuration-assignment leak**. The older network-cycle leak [Particle PR #1862](https://github.com/particle-iot/device-os/pull/1862) concerned delayed cleanup of terminated threads and was merged in 2019. It is a different mechanism; its fix is already listed in the local 6.4.1 changelog.

History searches using `git log -S` show:

| Change | History and relevance |
|---|---|
| Configuration reset by assignment | **`41bc674`, 2026-01-17**, introduces the pattern; **`2c19b08`, 2026-01-21**, carries it into the reorganized path attributed by current blame. Long predates v33. |
| Network standby node | **`217a4ae`, 2026-02-05**. Adds another allocated node when standby is effective. |
| STOP fallback configuration resets | **`8d8ca46`, 2026-02-11**. Additional occurrences on sleep errors. |
| PowerDiag counter / diagnostic batch | **`e3abeaf`, 2026-06-20** / **`a55d34c`, 2026-08-07**. Neither implements unbounded heap history. |
| LoopStage helper | **`20b4a05`, 2026-08-11**. Fixed timing state and logging. |
| Release pdiag suppression | **`41fb02b`, 2026-09-29**; profile-based default consolidated by **`261518e`**. Does not leave an allocating capture path running. |
| Live TimeDiag converter | **`e3173f3`, 2026-09-29**. Adds temporary String-copy churn, with destruction. |
| Heap/cycle visibility and hourly occupied reports | **`52ab58d`** and **`691b6c7`, 2026-10-02**. Make the loss observable across occupied stretches; v33 does not introduce the configuration ownership failure. |

The five-line bench diagnostic below is **draft only**. Preserve release behavior, particularly `ENABLE_DIAGNOSTICS_PUBLISH_MODE=0`; enabling the general bench profile would otherwise introduce pdiag publishes.

Insert these two lines immediately before `drainSerialBeforeSleep()` at **1399**, so the pre-sleep line is drained:

```cpp
Log.info("Heap pre cyc=%lu fh=%lu standby=%d",
         (unsigned long)AwakeCycles::cycles, (unsigned long)System.freeMemory(), (int)useNetworkStandby);
```

Insert this line immediately after the ULP sleep call at **1402**:

```cpp
const unsigned long heapPostWake = System.freeMemory();
```

Insert these two lines after serial restoration, immediately before `WakeReturn` at **1541**. This prints the earlier snapshot even if USB needed time to reconnect:

```cpp
Log.info("Heap post cyc=%lu fh=%lu wr=%u pin=%u err=%d",
         (unsigned long)AwakeCycles::cycles, heapPostWake, (unsigned)result.wakeupReason(), (unsigned)result.wakeupPin(), (int)result.error());
```

Use the existing subsequent `StateReq` line to distinguish **PIR→Sleep**, **PIR→Report**, **Timer→Sleep**, and **Timer→Report**. A timer wake does not necessarily report. Exclude any fallback/error cycle from this initial diagnostic, because the snapshot above belongs to the first ULP return.

For the five-minute bench run, after separately authorized preparation:

1. **Minute 0–1:** Capture serial with the device open-hours, occupied, and using the same keep-alive configuration. Let startup/report activity settle. Record `standby` and obtain a complete pre/post pair.
2. **Minutes 1–4:** Trigger isolated PIR wakes after each confirmed sleep entry, allowing the PIR output to return inactive between triggers. Aim for 15–30 complete cycles. Keep the session occupied and retain only cycles whose StateReq says `sleep-pir-return-to-sleep`, with no report/connect activity.
3. **Minute 4–5:** Stop stimulation and capture the next timer wake if the existing deadline permits. To compare a scheduled report within five minutes, arrange the run to straddle the existing report boundary. Classify it by StateReq rather than assuming timer means report.
4. For cycle `c`, let `Wc` be post-wake heap and `Pc` pre-sleep heap. Calculate **awake loss = `Wc − Pc`**, **sleep-call loss = `Pc − W(c+1)`**, and **whole-cycle loss = `Wc − W(c+1)`**. Also compare consecutive pre-sleep readings.

Expect a fixed checkpoint offset because the local wake-result allocation exists at post-wake and is freed before the next pre-sleep reading. **Repeated same-checkpoint decline is the decisive signal.** The configuration leak predicts decline on ordinary PIR→Sleep cycles, with the main loss appearing during the awake/preparation portion. A further declining component across the sleep call would justify investigating an additional OS-path leak.