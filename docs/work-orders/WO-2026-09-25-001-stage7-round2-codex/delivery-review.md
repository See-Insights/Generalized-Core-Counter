# Stage 7 round 2 — delivery, gate, queue, and sleep review

Model/reasoning inherited from dispatcher: **gpt-6-astra / ultra**. Repository read only; no network, devices, or repository mutations. At the start, src/lib/tests were copied to this scratch directory, avoiding races with root's separately authorized mutation checks. The review covers the candidate against base 599038e under decisions 6–7 and the relevant acceptance criteria.

## Disposition

**NOT VERIFIED** for decision 6 / criterion 11 as an unconditional safety guarantee. The original ordinary connected / ACK-fails-forever P1 is resolved in the host reproduction. Three new integration issues remain: the 25 s hold deliberately permits teardown with a publish still in flight; the sleptWithQueued latch is not reset on real same-state ULP sleep cycles; the delivery budget can survive a disconnected/sleep interval without being reset because no evaluator observes that interval. Hibernate q/CycleDelivery placement is fixed.

## Findings

### P1/P2 safety: the timeout branch still tears down with a live attempt

`src/state/State_Sleep.cpp:684–699` checks `PublishDeliveryGate::publishInFlight()`, waits while `holdElapsedMs < 25000`, then logs **“publish still in flight … proceeding to teardown”** and falls through. It does not cancel or finish the attempt, consume the Future result, or preserve a current RAM event before teardown. The real disconnect occurs at :882–900 (cloud-only at :888 or full radio-off at :892). This contradicts decision 6 and its dispatch's explicit rule that a publish in flight must never be abandoned by disconnect. `src/power/ConnectivityPolicy.h:165–170` documents that the cap is intentionally a never-completing-Future backstop; this is an implementation deviation from the authorized rule, not merely unreachable dead code.

Exact-source gate + real queue reproduction (`direct_hung_future`) reaches the teardown boundary at **55000 ms** (30000 ms old cloud gate plus 25000 ms hold), **inflight=1**, a=1/f=0, current queued RAM event=1. The exact commit helper then has q=1/s=1 but **zero event files and no payload on disk** because the real `writeQueueToFiles()` iterates `ramQueue` only (`PublishQueuePosixRK.cpp:121–149`), while the event is isolated in `curEvent` (`:312–314`). This demonstrates an unsafe gate release and failed persistence under this fault injection. It is **not a claim that actual hardware slept or lost data**: the host intentionally models instantaneous teardown and invokes the helper at the resulting boundary.

The injected delayed callback exceeds Device OS's ordinary 20 s ACK timeout. All simulated ordinary <=20 s failure attempts pass the in-flight rule below. However, the 20 s ACK timer is not a proof that accepted background dispatch to application result consumption is bounded by 25 s: queue `publishInFlight=true` is set on accepted background dispatch (`PublishQueuePosixRK.cpp:335–349`), before the worker's `Particle.publish()` completes. The root reviewer separately traced the Device OS system-thread dispatch and ACK registration interval. The source explicitly handles the exceptional delayed case by violating the required invariant. Tests at `tests/publish_delivery_budget_test.sh:134–145` require a hold condition and its ordering before GateFail but never exercise its fallthrough; the pure budget tests do not execute this second policy layer.

### P2: sleptWithQueued misses repeated same-state ULP sleep cycles

The new `sleepDeliveryCommitted` latch (`State_Sleep.cpp:173–177`) prevents double counting the same attempted hibernate and ULP fallback. It is cleared only under `if (enteredState)` at :521–531; enteredState is `(state != oldState)` at :402. After a successful ULP wake, :1674 resets wake-cycle observability but does not clear the latch. Two real paths transition SLEEPING to SLEEPING: :1856 (occupied timer report suppression) and :1891 (PIR return to sleep). `Generalized-Core-Counter.cpp:2542` sets oldState on state entry, and `transitionTo()` only sets state at :2566. Therefore the second real cycle has enteredState=false. The commit helper sees the previous latch and skips `noteSleptWithQueued()` at :205–206, despite another actual sleep with an event still queued.

Exact commit helper + real counters/queue probe, with two commits separated by the self-transition and an offline exact-source sleep-gate evaluation: **actual commits=2, q=1, s=1, expected s=2**; both emit CycleDelivery and the file remains intact. See `probe-logs/repeat_sleep_self_transition.log`. The repro simulates successful ULP return/state self-transition, not a physical sleep. This is a direct source/accounting defect in the new cumulative counter, not an old queue issue.

### P2: disconnected sleep intervals can leave the previous budget expired

`PublishDeliveryBudget.cpp:52–64` resets the budget only when an evaluation observes a drained queue or offline cloud. In the production adapter, `PublishDeliveryGate.cpp:28–36`, this depends on actually calling queuePermitsSleep. The SLEEPING handler calls it only within `Particle.connected() && !disconnectRequested` (`State_Sleep.cpp:554–566`). Its disconnected branch at :803–810 resets only the old cloud gate timers. No other production call resets `PublishDeliveryBudget`. Thus IDLE budget expiry → SLEEPING → disconnect → ULP → REPORT/CONNECT → IDLE can bypass all offline evaluations, retain budgetRunning/expiryReported, and start a new connected cycle with sleep already permitted. Offline elapsed time is then included, contrary to the budget's documented connected-episode semantics (`PublishDeliveryBudget.h:21–27`).

The real adapter/budget and exact-source state-gate probe shows first cycle expiry at90000ms, then an offline sleep-gate evaluation, then reconnect: the first connected queuePermitsSleep immediately returns true at host time192001ms with q=1/inflight=0, rather than beginning a new90000ms budget. See `probe-logs/idle_second_connection.log`. Full report/connect handlers are not simulated; the source path contains no budget reset, so this probe isolates the integration omission. It can deprive subsequent retry cycles of the intended fresh delivery window; whether it completely suppresses an attempt depends on when the queue is serviced in the intervening report/connect path.

### Additional safety exposure: offline with an unresolved current RAM event

`PublishDeliveryBudget.cpp:61–64` returns sleepPermitted=true offline regardless of publishInFlight; `State_Sleep.cpp:554` also skips the entire in-flight guard when cloud is offline. A cloud drop can precede application-thread queue completion processing. In the main loop, the sleep state handler runs at `Generalized-Core-Counter.cpp:1652–1665`, before `PublishQueuePosix::loop()` at :1708. Thus the accessor's deliberately extended protection through application-side requeue/persistence (`PublishQueuePosixRK.h:319–327`, cpp:417–422) is not honored on that route.

Probe `offline_inflight`: an ordinary scheduled20s failure is still outstanding when cloud goes offline9s after the budget starts; the exact source gate releases with inflight=1, q=1, and the exact helper creates no file for current RAM. This is a gate/persistence exposure, **not a demonstrated Boron data-loss event**. Actual teardown/radio state and scheduling may allow a later queue loop to consume the failure before hardware sleep. In particular full-modem-off paths often return for another loop (`State_Sleep.cpp:857–877`), while standby/already-off paths need not. No final universal in-flight check exists at the actual sleep commits. Treat as supporting evidence that the absolute no-in-flight promise is incomplete, with that limitation explicit.

## Required ordinary ACK-failure reproduction

Harness: `delivery_harness.cpp`, generated by `generate_delivery_harness.py`, executed with **zsh run_delivery_probes.zsh**. The command compiles:

```
clang++ -std=c++17 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable \
  -Wno-unused-but-set-variable -Iharness-stubs -Isrc -Ilib/PublishQueuePosixRK/src \
  delivery_harness.cpp lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp \
  src/cloud/PublishDeliveryBudget.cpp src/cloud/PublishDeliveryGate.cpp \
  src/cloud/PublishDeliveryCounters.cpp -o delivery_harness_bin
```

The queue source/header, budget, gate and counters are **the unmodified production implementations**, not queue mirrors. Generated extraction contains exact `State_Idle.cpp:212–358`, `State_Sleep.cpp:149–243` (timeout computation + commit helper), :245–289 (blocker labels), and :494–811 (complete cloud-operations gate). It uses #line for auditability. It schedules the state handler before queue.loop, matching production. The background stub always dispatches PRIVATE|WITH_ACK and always completes false after the scenario's delay; this schedules Future outcomes, not Device OS's full network stack. The SequentialFile index is stubbed while the real queue writes/reads actual host POSIX event bytes. Its reserveFile precreates a readable file to model Particle file creation: the library uses two-argument open with O_CREAT, whose mode is undefined on host POSIX, and is not the issue under review.

Hardware/cloud/clock stubs select low-power INTERMITTENT, no OTA/sensor/LED work, all ledgers synced, no pending webhook response. Times below are **delivery-gate / modeled sleep-commit times**, with instant teardown and one-millisecond application polls. Existing modem teardown time is not added; this host test therefore does not certify actual System.sleep wall time <=115s on hardware. Gate passage starts real asynchronous teardown at `State_Sleep.cpp:882–900`; :924–956 can wait for it separately.

| Scenario | Result | Gate / commit elapsed | In flight at boundary | Counters | Persistence / logs |
|---|---|---:|---:|---|---|
| Connected IDLE, every attempt fails after20s | PASS |90001ms; IDLE transition90000ms |0 |a2/k0/f2/r1/q1/s1 |1 real file; payload verified; exactly1 expiry log |
| Connected IDLE, every attempt fails after1ms | PASS |90001ms |0 |a3/k0/f3/r2/q1/s1 |1 file/payload; exactly1 expiry log |
| Connected IDLE, every attempt fails after10s; expiry falls in third attempt | PASS |92002ms; transition92001ms |0 |a3/k0/f3/r2/q1/s1 |Expiry log at90000ms has inflight1; held until result consumed;1 file/payload |
| Already SLEEPING, every attempt fails after20s | PASS for existing AC3 timeout exception |30000ms |0 |a1/k0/f1/r0/q2/s1 |1 file/payload; GateFail logged; no90s DeliveryBudget expiry |
| Undispatched RAM event, ULP commit helper | PASS | immediate commit |0 |q1/s1 |RAM-only beforehand;1 file/payload afterward |
| Undispatched RAM event, HIBERNATE commit helper | PASS | immediate commit |0 |q1/s1 |RAM-only beforehand;1 file/payload afterward |
| Already SLEEPING, callback never arrives | FAIL safety |55000ms |1 |a1/f0/q1/s1 |0 files; current RAM not persisted |
| IDLE, callback never arrives | FAIL boundedness under injected hung completion | no gate by179000ms |1 |attempt1 |No unsafe release; expiry emitted with inflight1 |
| Already SLEEPING, cloud drops with ordinary20s failure outstanding | FAIL gate-level safety exposure |9000ms |1 |a1/f0/q1/s1 |0 files/current RAM not persisted; hardware loss not established |
| Two actual commit boundaries separated by same-state ULP cycle | FAIL counter |2 commits |0 |q1/s1 expected s2 |1 file/payload;2 CycleDelivery lines |
| Second connected cycle after offline exact gate evaluation | FAIL budget episode reset | immediate permit on reconnect |0 |q1 |Original expiry still applies; no fresh90s window |

These are **11/11 probes executed**, of which6 are expected-behavior passes and5 expose the fault/edge cases above. They are not part of the repository's49-test suite. The driver deliberately continues after failing probes to collect all evidence; individual statuses appear in `probe-results.log` and `probe-logs/*.log`.

The one-event direct SLEEPING case reports q2 with one physical file because the preexisting failed-RAM branch leaves curEvent nonnull after persisting/deleting it, and getNumEvents counts that stale current-RAM slot (`PublishQueuePosixRK.cpp:251–257,408–413`). This is the previously known queue-depth inventory issue, **separately noted and not a new blocker for this WO**. Do not silently equate q with physical file count in this trace.

The30s direct-SLEEPING exit retains the old cloud-gate timeout (`State_Sleep.cpp:149–168,655–675,747–750`), bypassing the new queuePermitsSleep=false verdict. The direct route is real (`State_Connect.cpp:653–658`, selected by `State_Report.cpp:233`). AC3 expressly preserves the existing logged timeout exception, so this is recorded as behavior/decision6 consistency concern, **not independently declared a blocker merely because it precedes90s**. Its GateFail correctly records q=1 as a boolean blocker, qn as the inventory count.

## Hibernate / ULP telemetry requirement

**Resolved**: the helper flushes at `State_Sleep.cpp:204`, increments s at :206, captures q at :214, then emits `CycleDelivery: awake=... a=... k=... f=... r=... q=...` at :224–230. HIBERNATE calls it at :1238–1240 **before** nonreturning System.sleep at :1254. ULP calls it at :1341 before sleep at :1511; STOP fallback calls :1562/:1576 inherit that commit. The retained q and log therefore no longer depend on HIBERNATE returning. Both RAM-commit probes execute the real helper/counters/queue and verify file payload and counter effects. `sleep-commit-order.log` records independent source-order checks. Successful hardware hibernate / USB output delivery is not exercised.

## Evidence against relevant criteria and prior findings

- AC1: valid queued file and RAM events converge on flags normalization to clear NO_ACK and add WITH_ACK at `PublishQueuePosixRK.cpp:301–337`.
- AC2: statePublishWait waits at :364–365; file removal only success at :376–394; false result retains file or requeues/persists RAM at :395–414. Ordinary failure probes confirm real payload retention.
- AC3: normal IDLE false-Future cases now reach bounded safe sleep preparation; new cap/offline paths leave the unconditional in-flight safety promise incomplete. Existing30s logged GateFail is present.
- AC6: awake data emitted on both sleep paths; actual awake-time increase against pre-B hardware is a later bench measurement and cannot be claimed from this host run.
- AC11: default compile-time90000ms at `ConnectivityPolicy.h:163`; actual queue in-flight field set :346 and cleared only after result handling :422; real failure reproduction passes, RAM helper persists, expiry log at `PublishDeliveryGate.cpp:43–47`. **Overall fail** because of live-attempt cap fallthrough, repeated-sleep s undercount, and missing per-connection reset.
- AC12's hibernate q/CycleDelivery portion passes. Root owns reset-counter, mutation(iv), and corrected-doc evidence.
- Round1 P1: **resolved for the exact originally reported ordinary failures-never-recover scenario**, no longer an hour awake. This does not resolve the new exceptional/integration issues.
- Round1 P2 hibernate telemetry: **resolved** on both actual source call sites.

No repository file was edited. The runner removes its generated executable and real event-file directories on completion. Temporary source snapshots were removed after verification. Generated harness/stubs, generator, runner, logs and this review remain in the authorized scratch output as review evidence. To rerun, copy the reviewed src/lib/tests into this scratch directory first.
