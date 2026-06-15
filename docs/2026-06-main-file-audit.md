**File Metrics**
[src/Generalized-Core-Counter.cpp](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:1)

| Metric | Value |
|---|---:|
| Total lines | 2175 |
| Function definitions | ~50 |
| Largest functions | `setup()` 501 lines, `publishData()` 157, `loop()` 124, `connectivityFailsafeSupervisor()` 112, `publishStartupStatus()` 93 |

Other notable medium functions: `UbidotsHandler()` 48, `pauseAwakeWatchdogForSleep()` 44, `applyBatteryAwareConnectionModePolicy()` 43, `logTimeDiag()` 40, `dailyCleanup()` 36, `restoreAwakeWatchdogAfterWake()` 35.

**Recommendations**
| Class | Recommendation | Evidence | Effort | Risk |
|---|---|---|---:|---|
| DELETE | Remove stale commented unit-test hook | [lines 862-863](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:862) contain commented `Cloud::testBatteryBackoffLogic()`; actual test helper lives in `src/cloud/BatteryBackoff.cpp` and is not called | Tiny | Low |
| DELETE | Remove no-op automatic low-power placeholder from `dailyCleanup()` | [lines 2161-2172](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:2161) only comments and an empty conditional with commented `setLowPowerMode("1")` | Tiny | Low |
| DELETE | Review/delete legacy `sensorISR()` | Declared as “legacy tire-counting sensor” at [line 99](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:99), defined at [line 2124](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:2124), but `rg` found no attach/use; active PIR interrupt handling appears in sensor layer | Small | Medium, confirm no legacy pressure deployment |
| DELETE | Consolidate duplicate `drainSerialBeforeSleep()` | Main copy at [line 400](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:400), sleep-state copy at `src/state/State_Sleep.cpp:84`; same bench-oriented behavior | Small | Low |
| DELETE | Review unused breadcrumb values 21-25 | Enum/name cases exist at [lines 190-248](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:190), but no `setAppBreadcrumb()` calls use them | Small | Low-Med, retained old breadcrumb values may matter |
| DELETE | Review `testConnectionDurationOverride` initialization | [lines 858-859](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:858) only disables a test field; no runtime use found outside storage accessors | Small | Low-Med, avoid changing persisted schema casually |
| EXTRACT | Report/status publishing module | `publishData()` [line 1570](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:1570), `publishStartupStatus()` [line 1738](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:1738), `publishWatchdogForensics()` [line 1832](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:1832), `UbidotsHandler()` [line 1875](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:1875), `publishDiagnosticSafe()` [line 1938](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:1938) | 1-2 days | Medium, payload compatibility |
| EXTRACT | Watchdog and retained loop forensics | Retained forensics struct/state starts [line 425](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:425); helpers `refreshRetainedLoopForensics()`, `noteLoopStageDuration()`, WDT pause/restore at [line 1270](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:1270) | 1-2 days | Medium |
| EXTRACT | Connectivity failsafe supervisor | Failsafe helpers begin around [line 308](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:308); main supervisor at [line 2009](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:2009) | 1 day | Medium |
| EXTRACT | Time/open-hours policy helpers | `isWithinOpenHours()` [line 1464](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:1464), `logTimeDiag()` [line 1482](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:1482), `secondsUntilNextOpen()` [line 1524](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:1524) | 0.5-1 day | Low |
| EXTRACT | Battery-aware connection mode policy | `applyBatteryAwareConnectionModePolicy()` at [line 1365](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:1365); used from report/sleep/setup paths | 0.5-1 day | Medium |
| EXTRACT | Break `setup()` into lifecycle helpers | `setup()` is 501 lines from [line 626](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:626), mixing boot storm, watchdog, subscriptions, serial, RTC, config, startup status, sensor init, battery/power setup | 2-3 days | Medium-High |
| KEEP | `setup()` and `loop()` as thin orchestration points | Particle requires these entry points; goal should be shrinking them, not removing them | N/A | N/A |
| KEEP | PMIC/watchdog/startup forensics behavior | v19 reliability depends on power/watchdog visibility; delete only obsolete wrappers/comments, not production incident data | N/A | Medium if removed |
| KEEP | `transitionTo()` and `publishStateTransition()` | Recent v20 initiative made these the runtime transition path; keep central | N/A | Low |

**Recommended Sequence**
1. DELETE tiny dead/stale items first: commented test hook, no-op `dailyCleanup()` block, unused breadcrumb review, duplicate serial drain plan.
2. EXTRACT report/status publishing next. It removes ~300+ lines without touching sleep/connect state-machine logic.
3. EXTRACT time/open-hours helpers and battery policy. These are bounded and improve readability across state handlers.
4. EXTRACT watchdog/retained forensics and startup diagnostics.
5. Split `setup()` last, after dependencies have homes. Keep behavior identical and validate with builds after each slice.

Best branch name: `maint/v20-main-lifecycle-extraction`.