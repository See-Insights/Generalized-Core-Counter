5e5e72e5f09f41efb61e10f3c7effccd286f5bcb

Verdict: **NOT VERIFIED**

Model/reasoning actually used: `gpt-5.6-sol` / `xhigh`  
108/108 (sh via zsh, py via python3)

### BLOCKING B1 — `Q` is not fully made durable

`writeQueueToFiles()` persists only `ramQueue` ([PublishQueuePosixRK.cpp:121](/private/tmp/gcc-step0/dur/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:121)). When publishing starts, the next event is removed from `ramQueue` and held in `curEvent` ([PublishQueuePosixRK.cpp:301](/private/tmp/gcc-step0/dur/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:301)). The library itself still counts that in-flight RAM event as queued ([PublishQueuePosixRK.cpp:243](/private/tmp/gcc-step0/dur/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:243), [PublishQueuePosixRK.h:269](/private/tmp/gcc-step0/dur/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h:269)).

This state is reachable: failsafe, state handling, and thrash run before the publish-queue loop ([Generalized-Core-Counter.cpp:1124](/private/tmp/gcc-step0/dur/src/Generalized-Core-Counter.cpp:1124), [Generalized-Core-Counter.cpp:1181](/private/tmp/gcc-step0/dur/src/Generalized-Core-Counter.cpp:1181)). An event started during the previous pass can therefore be in `curEvent` when a deliberate reset is selected. `flushAllBeforeReset()` does not persist it, and a reset before the completion state runs can lose it; failed in-flight events are only restored later by `statePublishWait()` ([PublishQueuePosixRK.cpp:343](/private/tmp/gcc-step0/dur/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:343)). The reset-event callback calls the same incomplete method ([PublishQueuePosixRK.cpp:402](/private/tmp/gcc-step0/dur/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:402)).

The fake records only a synthetic `Q` call and has no in-flight state ([deliberate_reset_flush_test.py:81](/private/tmp/gcc-step0/dur/tests/deliberate_reset_flush_test.py:81)), so tests do not detect this durability gap.

### BLOCKING B2 — the required no-allocation property does not hold

For every nonempty RAM queue, `writeQueueToFiles()` constructs the path through `SequentialFile::getPathForFileNum()` ([PublishQueuePosixRK.cpp:124](/private/tmp/gcc-step0/dur/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:124), [SequentialFileRK.cpp:127](/private/tmp/gcc-step0/dur/lib/SequentialFileRK/src/SequentialFileRK.cpp:127)). That path uses `String::format()` and `String::reserve()`, which call `realloc()` in Device OS 6.6.0 ([spark_wiring_string.cpp:223](/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/wiring/src/spark_wiring_string.cpp:223), [spark_wiring_string.cpp:249](/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/wiring/src/spark_wiring_string.cpp:249), [spark_wiring_string.cpp:1006](/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/wiring/src/spark_wiring_string.cpp:1006)).

The test’s allocation check only scans the four-line helper body for `new`, `String`, or `malloc` ([deliberate_reset_flush_test.py:149](/private/tmp/gcc-step0/dur/tests/deliberate_reset_flush_test.py:149)); it does not exercise the real queue implementation.

### NOTE N1 — helper, facade, and seven-site ordering otherwise match the WO

The helper is declared at [StateMachine.h:127](/private/tmp/gcc-step0/dur/src/state/StateMachine.h:127) and performs C, S, N, then Q at [State_Error.cpp:115](/private/tmp/gcc-step0/dur/src/state/State_Error.cpp:115). All three facades synchronously call their store’s `flush(true)` ([MyPersistentData.cpp:918](/private/tmp/gcc-step0/dur/src/MyPersistentData.cpp:918), [MyPersistentData.cpp:1014](/private/tmp/gcc-step0/dur/src/MyPersistentData.cpp:1014), [MyPersistentData.cpp:1108](/private/tmp/gcc-step0/dur/src/MyPersistentData.cpp:1108)).

All seven actions have nothing between the helper and reset/power-down:

| Site | Record/alert → flush → action |
|---|---|
| Thrash tier 3 | [ThrashGuard.cpp:149](/private/tmp/gcc-step0/dur/src/ThrashGuard.cpp:149) |
| Error soft | [State_Error.cpp:160](/private/tmp/gcc-step0/dur/src/state/State_Error.cpp:160) |
| Error hard | [State_Error.cpp:171](/private/tmp/gcc-step0/dur/src/state/State_Error.cpp:171) |
| Nightly heap guard | [State_Sleep.cpp:638](/private/tmp/gcc-step0/dur/src/state/State_Sleep.cpp:638) |
| Sleep exhausted | [State_Sleep.cpp:893](/private/tmp/gcc-step0/dur/src/state/State_Sleep.cpp:893) |
| Failsafe stage 2 | [ConnectivityFailsafe.cpp:301](/private/tmp/gcc-step0/dur/src/power/ConnectivityFailsafe.cpp:301) |
| Failsafe stage 3 | [ConnectivityFailsafe.cpp:311](/private/tmp/gcc-step0/dur/src/power/ConnectivityFailsafe.cpp:311) |

Alert 18 and Error alerts are in `current`: `RecoveryState::{set,raise}Alert` forward there ([MyPersistentData.cpp:1067](/private/tmp/gcc-step0/dur/src/MyPersistentData.cpp:1067)). Failsafe stage/action/count are written before flushing and forward to `sysStatus` ([ConnectivityFailsafe.cpp:77](/private/tmp/gcc-step0/dur/src/power/ConnectivityFailsafe.cpp:77), [MyPersistentData.cpp:1043](/private/tmp/gcc-step0/dur/src/MyPersistentData.cpp:1043)).

The low-battery defer path retains exactly one S-only flush and does not flush Q ([ConnectivityFailsafe.cpp:288](/private/tmp/gcc-step0/dur/src/power/ConnectivityFailsafe.cpp:288)); stage 2/3 do not duplicate the old S flush.

### NOTE N2 — reset-site audit found no additional gap

- OOM remains unchanged and unflushed at [Generalized-Core-Counter.cpp:1208](/private/tmp/gcc-step0/dur/src/Generalized-Core-Counter.cpp:1208).
- The Device OS watchdog and breadcrumb-only expiry callback remain excluded ([Generalized-Core-Counter.cpp:416](/private/tmp/gcc-step0/dur/src/Generalized-Core-Counter.cpp:416), [LoopForensics.cpp:390](/private/tmp/gcc-step0/dur/src/observability/LoopForensics.cpp:390)).
- The fallback `ApplicationWatchdog` callback remains excluded at [LoopForensics.cpp:395](/private/tmp/gcc-step0/dur/src/observability/LoopForensics.cpp:395).
- The external AB1805 watchdog remains armed/serviced without a flush ([Generalized-Core-Counter.cpp:800](/private/tmp/gcc-step0/dur/src/Generalized-Core-Counter.cpp:800), [Generalized-Core-Counter.cpp:1165](/private/tmp/gcc-step0/dur/src/Generalized-Core-Counter.cpp:1165)).
- HIBERNATE is untouched and excluded ([State_Sleep.cpp:690](/private/tmp/gcc-step0/dur/src/state/State_Sleep.cpp:690)).
- Boot-storm ULP sleep returns to the same execution and is not a deliberate reset ([Generalized-Core-Counter.cpp:387](/private/tmp/gcc-step0/dur/src/Generalized-Core-Counter.cpp:387)).
- Normal ULP/STOP sleeps are returnable; their all-attempts-failed reset is covered ([State_Sleep.cpp:823](/private/tmp/gcc-step0/dur/src/state/State_Sleep.cpp:823), [State_Sleep.cpp:875](/private/tmp/gcc-step0/dur/src/state/State_Sleep.cpp:875), [State_Sleep.cpp:890](/private/tmp/gcc-step0/dur/src/state/State_Sleep.cpp:890)).
- `deepPowerDown()`’s internal 30-second fallback reset remains unchanged, but both callers flush before entering it ([AB1805_RK.cpp:582](/private/tmp/gcc-step0/dur/lib/AB1805_RK/src/AB1805_RK.cpp:582)).
- No `System.dfu`, `resetNow`-style helper, or other reset call was found.

### NOTE N3 — failure, timing, and reentrancy behavior

A clean store is a no-op. A dirty store performs blocking open/write/close with no retry, explicit `fsync`, or returned status; `flush()` clears its dirty timestamp after `save()` regardless of success ([StorageHelperRK.cpp:25](/private/tmp/gcc-step0/dur/lib/StorageHelperRK/src/StorageHelperRK.cpp:25), [StorageHelperRK.cpp:258](/private/tmp/gcc-step0/dur/lib/StorageHelperRK/src/StorageHelperRK.cpp:258)). Queue-empty is also a no-op. On queue I/O failure, status is ignored, the file number is still queued, and the RAM event is deleted ([PublishQueuePosixRK.cpp:128](/private/tmp/gcc-step0/dur/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:128)). Reset proceeds in all cases.

The store and queue locks are recursive, so same-thread re-entry does not deadlock; cross-thread contention blocks without a deadline ([StorageHelperRK.h:69](/private/tmp/gcc-step0/dur/lib/StorageHelperRK/src/StorageHelperRK.h:69), [PublishQueuePosixRK.h:287](/private/tmp/gcc-step0/dur/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h:287)). Production setup initializes all stores and the queue before loop-based callers can run ([Generalized-Core-Counter.cpp:493](/private/tmp/gcc-step0/dur/src/Generalized-Core-Counter.cpp:493), [Generalized-Core-Counter.cpp:647](/private/tmp/gcc-step0/dur/src/Generalized-Core-Counter.cpp:647)).

Under the accepted ceilings, `3 × 250 ms + 2 s = 2.75 s`; a failed `deepPowerDown()` adds about 30 seconds, still below the 60-second internal watchdog ([Generalized-Core-Counter.cpp:157](/private/tmp/gcc-step0/dur/src/Generalized-Core-Counter.cpp:157), [AB1805_RK.h:325](/private/tmp/gcc-step0/dur/lib/AB1805_RK/src/AB1805_RK.h:325)). Error’s 30-second dwell is spread across serviced loop passes ([Generalized-Core-Counter.cpp:270](/private/tmp/gcc-step0/dur/src/Generalized-Core-Counter.cpp:270), [Generalized-Core-Counter.cpp:1188](/private/tmp/gcc-step0/dur/src/Generalized-Core-Counter.cpp:1188)).

There is nevertheless no finite code-level worst case: a filesystem call or mutex can block past 60 seconds while ThrashGuard/Error holds the main loop. This is a note because the WO expressly accepts no timeout and defers measurement to the bench ([WO:31](/private/tmp/gcc-step0/dur/docs/work-orders/WO-2026-11-03-005-item26-durability-flush.md:31)).

### NOTE N4 — layout, library, retained RAM, and budget

No persisted member or retained field changed. The frozen 272/36/88-byte layouts and offsets remain enforced by [persisted_layout_preservation_test.py:31](/private/tmp/gcc-step0/dur/tests/persisted_layout_preservation_test.py:31), and retained ownership remains pinned by [retained_boot_structural_test.py:57](/private/tmp/gcc-step0/dur/tests/retained_boot_structural_test.py:57). Diffs against both `main` and `origin/main` show no `lib/` changes.

`src/` is +21 net: 22 additions, one deletion. The one-line overage is cosmetic. Removing only the declaration’s doc comment at [StateMachine.h:126](/private/tmp/gcc-step0/dur/src/state/StateMachine.h:126) leaves the declaration and drops the result to +20. Per dispatch, I did not fail the WO on this line.

### NOTE N5 — tests and linkage

`deliberate_reset_flush_test.py` extracts and compiles the real helper body ([deliberate_reset_flush_test.py:100](/private/tmp/gcc-step0/dur/tests/deliberate_reset_flush_test.py:100), [deliberate_reset_flush_test.py:108](/private/tmp/gcc-step0/dur/tests/deliberate_reset_flush_test.py:108)). I separately materialized all eight requested mutations; each was detected: [C](/private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage7-r1/mutations/drop_c/result.log:3), [S](/private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage7-r1/mutations/drop_s/result.log:3), [N](/private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage7-r1/mutations/drop_n/result.log:3), [Q](/private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage7-r1/mutations/drop_q/result.log:3), [queue first](/private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage7-r1/mutations/queue_first/result.log:3), [missing site](/private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage7-r1/mutations/missing_site/result.log:10), [OOM added](/private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage7-r1/mutations/oom_added/result.log:13), and [failsafe record moved after flush](/private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage7-r1/mutations/failsafe_early/result.log:10).

The existing tests were not weakened:

- Connectivity keeps all Open/Closed/Unknown and cooldown cases while additionally requiring record→flush→action ([connectivity_failsafe_unknown_openness_test.cpp:164](/private/tmp/gcc-step0/dur/tests/connectivity_failsafe_unknown_openness_test.cpp:164), [connectivity_failsafe_unknown_openness_test.cpp:189](/private/tmp/gcc-step0/dur/tests/connectivity_failsafe_unknown_openness_test.cpp:189)).
- Heap guard still extracts the real whole function and checks both flag configurations and all non-reset thresholds ([nightly_heap_guard_flush_test.sh:78](/private/tmp/gcc-step0/dur/tests/nightly_heap_guard_flush_test.sh:78), [nightly_heap_guard_flush_test.sh:96](/private/tmp/gcc-step0/dur/tests/nightly_heap_guard_flush_test.sh:96), [nightly_heap_guard_flush_test.sh:119](/private/tmp/gcc-step0/dur/tests/nightly_heap_guard_flush_test.sh:119)).
- Cloud-gate compares includes, drain, S-2 tail, helpers, gate, and handler head ([sleep_cloud_gate_structural_test.py:238](/private/tmp/gcc-step0/dur/tests/sleep_cloud_gate_structural_test.py:238)).
- Plan compares includes, S-2 tail, cloud gate, plan, heap guard, and handler ([sleep_plan_structural_test.py:233](/private/tmp/gcc-step0/dur/tests/sleep_plan_structural_test.py:233)).
- Preflight compares includes, S-2 tail, cloud gate, sleep plan, summary, Wi-Fi guard, and handler ([sleep_preflight_structural_test.py:270](/private/tmp/gcc-step0/dur/tests/sleep_preflight_structural_test.py:270)).
- Wake compares retained use, includes, S-2/log blocks, existing State_Modes functions, reconstructed post-wake/event region, and handler tail ([sleep_wake_structural_test.py:193](/private/tmp/gcc-step0/dur/tests/sleep_wake_structural_test.py:193)).
- Execution compares retention/source ownership plus `effectiveStandby`, `hibernateAttempt`, `sleepAndResume`, and serial drain ([sleep_execution_structural_test.py:136](/private/tmp/gcc-step0/dur/tests/sleep_execution_structural_test.py:136), [sleep_execution_structural_test.py:164](/private/tmp/gcc-step0/dur/tests/sleep_execution_structural_test.py:164)). Its new fake is an unrecorded no-op ([SleepExecutionFakes.h:173](/private/tmp/gcc-step0/dur/tests/stubs/sleep_teardown/SleepExecutionFakes.h:173)).

A clean Device OS 6.6.0 Boron build with fresh `BUILD_PATH_BASE` succeeded: text 151,692, data 1,114, bss 2,200 ([build-2.log:20](/private/tmp/gcc-step0/dur/build-tmp/wo-2026-11-03-005-stage7-r1/build-2.log:20)). `nm -C` found the helper and all three facades. `objdump -C` found exactly seven `bl flushAllBeforeReset()` instructions: thrash 1, failsafe 2, Error 2, and sleep 2; none is in global `loop()`.

### NOTE N6 — runtime delta and the day-boundary goal

Apart from the seven synchronous flushes and their latency, runtime policy is unchanged. The failsafe low-battery path still performs its former single S flush; Error still uses its 30-second dwell; no new state, timer, publish, or persisted field was added.

On `main`, day-boundary reporting closes the session, queues the report, performs cleanup/reopen handling, and records `lastDailyCleanup` ([State_Report.cpp:45](/private/tmp/gcc-step0/dur/src/state/State_Report.cpp:45), [State_Report.cpp:70](/private/tmp/gcc-step0/dur/src/state/State_Report.cpp:70), [State_Report.cpp:77](/private/tmp/gcc-step0/dur/src/state/State_Report.cpp:77)). Previously, a same-pass thrash reset followed immediately without a barrier. The new helper runs before that reset and, when the underlying writes succeed, synchronously persists C and S and moves that newly queued report to a file. Thus the specific stale-open-session return described by the WO is removed. Overall durability remains **NOT VERIFIED** because B1 leaves an older in-flight RAM event unpersisted and B2 violates the no-allocation requirement.

The final repository check remained at the reviewed SHA with no tracked-file modifications; only the pre-existing untracked dispatch document is present.