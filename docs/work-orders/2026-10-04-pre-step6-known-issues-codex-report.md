**A. Watchdog resets**

**Findings.** Reason 60 is the **MCU watchdog**, configured for **60 seconds**, not proof of a 124-second AB1805 timeout. Device OS maps the MCU watchdog reset flag to reason 60; an AB1805 reset arrives through the reset pin and requires separate confirmation. Current watchdog setup/service locations are `Generalized-Core-Counter.cpp:1278/1641`, replacing `1266/1629`. See [MCU watchdog configuration](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:904), [reset decoding](/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/hal/src/nRF52840/core_hal.c:664), and [AB1805 classification](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/time/HibernateCycle.cpp:47).

The four cited US connection events match `bc=18, stage=connectivity, state=4`. The earlier archive contains **30 matching events: 22 US, eight SG**. Their publication timestamps are delivery times, not necessarily reset times.

Breadcrumb 18 is written **after** the queue returns, now at [line 1662](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:1657). It survives into the next loop’s connection handler. Relevant calls are:

| Calls/sites | Blocking assessment in supplied source |
|---|---|
| `Particle.connect()` — `State_Connect.cpp:93,100,401` | Sets connection intent; does not wait for cloud connection. Initial call has breadcrumb 6. |
| `Cellular.ready/isOn`, `Network.ready`, `Particle.connected` — `:333–337` | State queries, not synchronous AT transactions. |
| `Cellular.RSSI()` — `:75,390,627`; helper callers `:451,519,693` | **Leading suspect:** synchronous modem access, including during acquisition and timeout handling. Still called when trace output is disabled. |
| Recovery: `Cellular.disconnect/off/on`, `Particle.disconnect/connect` — `:91–104`, `Connectivity.h:25–48` | Cellular operations dispatch asynchronously; default cloud disconnect requests teardown. |
| Ledger loading/status/data — `:582,594,597,610` | Local reads/writes and locks can block; cloud synchronization is asynchronous. |
| Publish queue / `Particle.publish` | Queue performs local storage work; actual publish/ack waiting runs in `BackgroundPublishRK`’s worker. Breadcrumb 18 excludes an unfinished preceding queue-loop call. |

The RSSI path reaches a modem-client lock and AT commands: [Device OS cellular HAL](/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/hal/network/ncp/cellular/cellular_hal.cpp:321), [modem implementation](/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/hal/network/ncp_client/sara/sara_ncp_client.cpp:776). Existing breadcrumbs do **not** prove which call stalled.

For sleep, breadcrumb 28 immediately precedes `System.sleep()` ([ULP site](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:1404)). It covers OS preparation, sleeping and restoration—not solely application teardown. The gate incorrectly equates `!isOn()` with fully off: Device OS defines `isOn()` and `isOff()` separately, leaving intermediate states where both are false. `System.sleep()` can then wait **120 seconds** for modem shutdown. [State definitions](/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/system/src/system_network_manager.cpp:1045), [internal wait](/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/system/src/system_sleep.cpp:79). Dev-14’s October 2 hibernate record shows interruption after **61 seconds**. Slow teardown supports this hypothesis; PPP error 5 does not establish causation.

**One proposed change — keep modem waits outside the application’s blocking path.** Remove acquisition/timeout RSSI reads; require `Cellular.isOff()` throughout the existing bounded non-standby teardown gate; add call-entry/return breadcrumbs. **Estimate: 45–70 changed `src/` lines.** This fixes the existing gate from history (`ef5be17`), without a new supervisor. It agrees with Particle’s state-machine/asynchronous guidance and preserves its recommended connection windows: [cached guidance](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/build-tmp/connectivity-archive/particle-docs/reference_device-os_api_cellular_connect.txt:66). It cannot yet guarantee elimination of every sleep-path reset.

**B. Restart limitation**

**Findings.** The proposed session-start persistence **already exists**: `occupied`, epoch `occupancyStartTime`, `lastOccupancyEvent` **in millis**, and `totalOccupiedSeconds` are stored in `/usr/current.dat`. Starts are recorded at [State_Modes.cpp:65](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Modes.cpp:65); the schema is at [MyPersistentData.h:694](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/MyPersistentData.h:694). Saving is deferred until 250 ms after the latest modification, not forced immediately at session start. [Persistence setup](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/MyPersistentData.cpp:660). History traces this behavior to `eda6b7e`; `8de332b` later repaired boot-time start validation.

Two recovery defects remain:

- The persisted debounce timestamp belongs to the **previous boot’s millis clock**. Subtraction after reboot can immediately expire occupancy. [State_Idle.cpp:53](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Idle.cpp:53).
- Closing requires trusted time, but **clears occupied/start even when trust fails**, discarding the recoverable session. [State_Common.h:301](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Common.h:301).

The archived reports support the examples: PCKL1’s payload timestamps span **12:32:27–12:33:29Z**, while total stays 56 minutes; MAFC-1 enters occupancy at **18:59:54 EDT**, and its later closing report still totals 163 minutes; Dev-09 totals rise **193→213**, with serial showing occupancy restarted around 12:50 despite the earlier 12:39 session. Approximately 11 minutes are absent, but that interval includes both a pin reset and the OTA restart—not solely the update.

| Endpoint option | Assessment |
|---|---|
| Last alive at reports | Cheap, but hourly/tier-spaced reports can lose substantial session time. |
| AB1805 RAM heartbeat | Best resolution without frequent flash writes; 256 bytes available. Must validate record integrity and retention. |
| 300-second cap | Bounds overcredit; cannot recover an 11-minute session and can still credit powered-off time. |

**One proposed change — recover confirmed session time without counting a power-off interval.** Reuse the persisted start; maintain a small checksummed session/last-alive record in [AB1805 RAM](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/lib/AB1805_RK/src/AB1805_RK.h:635), updated periodically while occupied and before intentional sleep. On reboot, preserve it until clock trust is established, then credit through the last confirmed endpoint and close exactly once, respecting the daily boundary. Force durability at session start/close; avoid heartbeat flash writes.

**Estimate: 120–160 `src/` lines.** None of these options identifies the exact reset instant universally: a RAM heartbeat avoids long power-off overcredit but may undercount the final heartbeat interval or watchdog stall.

**C. Hibernate**

**Findings.** `result=fail DEEP_POWER_DOWN` means the **classification gate rejected a non-ALARM label**; it does not establish failed sleep. The library prioritizes `SLST → DEEP_POWER_DOWN` ahead of the alarm flag. The application then rejects that label and zeroes `actual/err`. [Library priority](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/lib/AB1805_RK/src/AB1805_RK.cpp:181), [gate](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/time/HibernateCycle.cpp:94), [zeroing](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/time/HibernateWakeDiagnostics.h:135).

Dev-09’s recent scheduled wakes have `rtcAt − rtcBefore − req = +1 second`. The older “hours late” examples also woke on time by their recorded RTC values: the event delivered **September 23 at 01:50Z** records waking **September 22 at 22:00:19Z**. Thus the older “7/7 failures” characterization confuses classification/delivery with wake timing. Dev-14’s recent normal overnight wakes report **ALARM/ok**, although its October 2 watchdog-interrupted attempt is a real failure.

The October 1 **12:43:05Z** status confirms failsafe stage 3 and `DEEP_POWER_DOWN`. That demonstrates the separate `ab1805.deepPowerDown()` path; scheduled hibernate uses `System.sleep(HIBERNATE)`.

Trail02’s device setting was captured with update time **October 1, 23:17:10Z**. Opening is **06:00 EDT**:

| Local night | Hibernate event | RTC wake, EDT | First report generated / delivered, EDT | Heap-guard reset |
|---|---|---|---|---|
| Oct 1→2 | Yes; UNKNOWN/fail | 06:00:28 | 06:01:11 / 06:01:18 | None observed |
| Oct 2→3 | Yes; UNKNOWN/fail | 06:00:21 | 06:00:38 / 06:01:05 | None observed |
| Oct 3→4 | Yes; UNKNOWN/fail | 06:00:35 | 06:00:41 / 06:00:50 | None observed |

All three recorded durations equal requested duration plus one second. Evidence: [trial archive](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/61f517c9-3c76-4b18-afd3-fa4f31a7be63/scratchpad/codex-data).

Enablement is merged `modes.enableHibernateSleep`; default false is now **MyPersistentData.cpp:153**, with JSON line 27 unchanged. Additional gates: Boron, closed-hours overnight path, Device OS ≥6.4, **900–36,000 seconds**, valid/readable RTC, wake pin high, successful interrupt cleanup/alarm programming, and sleep preparation. [Configuration](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/cloud/ConfigApply.cpp:523), [eligibility](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:269), [entry](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:1106).

**Verdict: not yet fleet-wide.** Three timely US trial nights are encouraging; unresolved sleep watchdogs and ambiguous wake-source classification remain. The captured ledgers also correct the dispatch’s fleet version premise: **all eleven report v36 by October 4**, including US updates that morning.

**One proposed change — report wake timing independently of wake-source classification.** Preserve raw reason/gate diagnostics, calculate elapsed/error whenever RTC operands are valid, and distinguish timely wake, interruption and unknown cause. Do not simply whitelist UNKNOWN/DEEP_POWER_DOWN. **Estimate: 25–45 `src/` lines.** History (`303cde2`) contains the original strict classification, not a prior corrected implementation.

**D. Battery trust and tier**

**Findings.** A historical fleet-wide rate is **not measurable from these archives**: **9,079 deduplicated reports and 642 status events contain no vcell**, and archived `pdiag` entries lack it too. The older archive additionally contains **Dev-11 serial**, correcting the two-device coverage premise.

Applying today’s piecewise table to **1,516 deduplicated ChargeDiag pairs**, September 14–October 4:

| Device | Absolute disagreement ≥20 percentage points |
|---|---:|
| Dev-09 | 2/638 — 0.3% |
| Dev-14 | 82/591 — 13.9% |
| Dev-11 | 1/287 — 0.3% |
| Total available paired serial samples | **85/1,516 — 5.6%** |

These are printed **accepted SOC/vcell pairs**, not necessarily fresh raw-gauge pairs: accepted SOC can retain an earlier authoritative sample. [Sample selection](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/sensors/SensorManager.cpp:640).

- **Charging:** disagreement is 1/39 during FAST, 20/1,214 during DONE, and 64/263 during OFF. The association with noncharging is substantial but heavily concentrated in Dev-14.
- **Source:** raw BATTERY gives 52/163 versus 23/983 for raw USB_HOST/USB_ADAPTER. Raw VIN gives 10/291, but **VIN is not reliable evidence of solar**: USB override deliberately leaves ChargeDiag’s raw source unchanged. [Source semantics](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/power/PmicFaultMonitor.cpp:396).
- **Radio / wake age:** neither is recorded alongside these voltage reads. Nearby connection logs establish some loaded sampling, but cannot support an exact on/off or time-since-wake comparison.

The captured ledger values differ from the dispatch: Court3 **79.8%, 3.89 V, Untrusted/CRITICAL**; PCKL1 **76.5%, 3.87 V, Untrusted/CRITICAL**; MAFC-2 **84.9%, 3.97 V, Suspect/CONSERVING**. These are snapshots, not historical rates. [Ledger evidence](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/61f517c9-3c76-4b18-afd3-fa4f31a7be63/scratchpad/codex-data/ledgers).

Voltage is read at [SensorManager.cpp:522](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/sensors/SensorManager.cpp:519): setup, reporting, connection success, pre-sleep and post-wake. Post-wake sampling precedes later connection logic ([State_Sleep.cpp:1573](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:1573)), but standby can retain the modem, charging can continue, and sensors are already active. **No sample is proven genuinely rested.** Radio status is queried later at line 829, after intervening power operations.

The table explicitly says it is **not device-validated**. Also, 28-point allowance applies only to positive residual under radio load; charging widens the negative side. Both Suspect and Untrusted substitute OCV SOC for tiering. [Table/thresholds](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/power/BatteryHealth.cpp:14), [tier substitution](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/reporting/BatteryTierGuard.cpp:5). History identifies this substitution’s introduction in `516332a`.

**Neither gauge nor table is established as correct.** The settling bench test is a measured remaining-capacity discharge from a suspect state, compared with that cell’s measured usable full capacity. A resting multimeter reading alone validates voltage, not SOC accuracy.

**One proposed change — capture the evidence needed to correct tiering.** Add one coherent battery decision snapshot containing raw/accepted SOC, vcell, charge state, actual radio state, sample context/age, effective source and resulting tier. **Estimate: 40–60 `src/` lines.** Threshold or curve changes are not justified before that measurement.

Model: **gpt-6-astra · reasoning: high**. Read-only investigation; no edits, builds, network calls or device actions.