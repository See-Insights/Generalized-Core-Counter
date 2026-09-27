# WO-2026-09-25-001 — Stage 7 round 2

**NOT VERIFIED.** All four required mutations are caught, both fresh candidate builds contain the required code, and the original ordinary ACK-failure wedge plus all four other named round-1 findings are resolved. Three integration defects remain in decision 6: teardown can proceed while a publish is still in flight; repeated ULP sleeps can undercount sleptWithQueued; and a disconnected sleep interval can leave the previous delivery budget expired on reconnect.

Model actually used: **gpt-6-astra**, reasoning **ultra**, including inherited review subagents. Review target: the uncommitted tree on `wo/2026-09-25-001-publish-with-ack` versus `599038e`, including untracked src/tests. Binding decisions 1–8 and criteria 1–12 were read. No changes were made to repository files, documents, Git state, devices, or AWS. Network use was limited to the authorized Particle compile operation.

## Findings requiring implementation changes

1. **P1 — A timeout releases teardown with an outstanding publish.** [src/state/State_Sleep.cpp:684](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:684) checks the real queue accessor, but after the 25-second hold it explicitly logs that publishing is still in flight and proceeds at [src/state/State_Sleep.cpp:698](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:698); cloud/radio teardown follows at [src/state/State_Sleep.cpp:882](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:882). An exact-source gate reproduction with a delayed completion releases at55 seconds (30-second existing gate plus25-second hold), with inflight=1 and the sole current RAM event absent from disk. This violates the explicit decision-6 no-in-flight rule. The ordinary20-second ACK failures do **not** trigger this path; the reproduction deliberately injects a missing/delayed completion. Device OS's20-second ACK timer does not establish a bound from background-dispatch acceptance through application-thread result consumption. The offline branch also bypasses the guard ([src/cloud/PublishDeliveryBudget.cpp:61](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/cloud/PublishDeliveryBudget.cpp:61), [src/state/State_Sleep.cpp:554](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:554)). These are gate/persistence exposures; no hardware sleep or hardware data loss was observed. Make the in-flight requirement hold across every teardown/sleep exit and add an integrated regression for the timeout/offline paths.
2. **P2 — sleptWithQueued undercounts distinct ULP cycles.** The helper increments only while sleepDeliveryCommitted is false ([src/state/State_Sleep.cpp:205](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:205)); the only reset is gated by enteredState ([src/state/State_Sleep.cpp:531](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:531)). A successful ULP wake starts a new cycle at :1674 but does not clear the latch. SLEEPING→SLEEPING transitions at :1856/:1891 leave state==oldState, so the next actual sleep skips the increment. The exact-helper probe produced two sleep commits, two CycleDelivery lines, q=1, but **s=1 instead of2**. Reset this per successful sleep/wake cycle while preserving the intended same-attempt HIBERNATE→ULP fallback deduplication.
3. **P2 — A new connection can inherit an expired budget.** The budget resets only when evaluate observes offline/empty ([src/cloud/PublishDeliveryBudget.cpp:52](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/cloud/PublishDeliveryBudget.cpp:52)), but the SLEEPING caller evaluates only while connected ([src/state/State_Sleep.cpp:554](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:554)). Its offline branch resets only the older local cloud timers ([src/state/State_Sleep.cpp:803](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:803)). The ordinary ULP timer wake→REPORT→CONNECT→IDLE route (:1860; State_Report.cpp:281; State_Connect.cpp:658) can therefore miss every offline evaluation. The real-module probe permits sleep immediately on the next connection with a queued event; no fresh90-second window or new expiry log occurs. This can shorten later delivery opportunities; it does not prove every reconnect starves. Reset the episode on the actual disconnect/cycle boundary and test two consecutive connected cycles.
4. **P3 — New test wrappers leave build-tmp behind.** [tests/publish_with_ack_queue_test.sh:14](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/publish_with_ack_queue_test.sh:14), [tests/publish_delivery_counters_test.sh:17](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/publish_delivery_counters_test.sh:17), and [tests/publish_delivery_budget_test.sh:17](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/publish_delivery_budget_test.sh:17) each create build-tmp and remove only the binary in the EXIT cleanup. Independent runs reproduced an empty remaining directory. Alphabetical full-suite execution first creates it in publish_delivery_budget_test.sh. rtc_skew_test.sh correctly removes only a directory it created. This violates the workflow's required directory cleanup; no fixes were made under this review-only authorization.

The first three findings are new code/integration findings, not repetition of the original one-hour ACK-failure wedge. Existing source-presence tests pass despite these integration errors. The old direct-SLEEPING30-second GateFail exception remains logged and is not separately made a blocker merely because it precedes90 seconds: criterion3 expressly permits that exception.

## Required host reproductions

The delivery harness compiles the unmodified queue, budget, gate, and counters; extracts the exact relevant IDLE/SLEEPING gate and commit code; uses real POSIX queue files; and stubs only device/cloud interfaces, background completion scheduling, and the file index. It preserves handler-before-queue.loop order. Times below are modeled sleep/teardown boundaries with instantaneous modem teardown, not measured physical System.sleep times. Full source, command, limitations, and logs: [delivery review](/private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/delivery/delivery-review.md).

| Probe | Result | Observed evidence |
|---|---|---|
| Connected queue; every publish fails after20s | PASS | Boundary90.001s; inflight0; a2/k0/f2/r1/q1/s1; one durable file with identical payload; one expiry log |
| Connected queue; every publish fails after1ms | PASS | Boundary90.001s; inflight0; a3/f3/r2/q1/s1; payload retained; one expiry log |
| Connected queue; every publish fails after10s, expiry crosses an attempt | PASS | Expiry90s with inflight1; boundary92.002s after outcome consumption; inflight0; payload retained; s1 |
| Undispatched RAM events at HIBERNATE / ULP commit | PASS, both | RAM-only before; file/payload present afterward; q1/s1; CycleDelivery before modeled sleep |
| Delayed completion in direct SLEEPING | FAIL safety | Teardown boundary55s with inflight1; no persisted current RAM event |
| Successive ULP self-transition sleeps | FAIL accounting | Two queued sleep commits; s1, expected2 |
| Next connection after offline sleep | FAIL episode reset | First connected evaluation immediately permits sleep using prior expired budget |
| Attempt→begin(reset)→attempt→success, real counters | PASS | **a2 = k1 + f0 + abandoned1; r1**; repeated begin does not abandon twice |

The delivery driver executed11/11 probes:6 expected-behavior passes and5 documented fault/edge exposures (details include offline in-flight and never-completing IDLE completion). This is separate from the repository's49-test suite. The standalone reset probe is [reset log](/private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/counters/reset-during-publish.log); its counter implementation is [src/cloud/PublishDeliveryCounters.cpp:83](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/cloud/PublishDeliveryCounters.cpp:83). Below saturation, the requested reset invariant and retry attribution pass.

HIBERNATE calls commitDeliveryAccountingBeforeSleep at [src/state/State_Sleep.cpp:1238](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:1238) before System.sleep at :1254. ULP calls it at [src/state/State_Sleep.cpp:1341](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:1341) before :1511. The helper flushes RAM at :204, snapshots q at :214 and logs CycleDelivery at :224. The old missing-hibernate-telemetry finding is resolved; the new s undercount does not suppress q/CycleDelivery.

## Host suite and required mutation table

Baseline: **49/49 — 25/25 sh via zsh, 24/24 py via python3**, no skips. Each mutation was run against a byte-identical candidate snapshot outside the repository, including its real source/library/test files and untracked additions. Every full suite was run after each mutation. Original bytes were rewritten in a finally block after every run, and every one of the1,244 snapshot file hashes matched afterward. The repository itself never needed mutation.

| Mutation | Passing/total | sh via zsh | py via python3 | Detecting test(s) | Restored |
|---|---:|---:|---:|---|---|
| (i) Drop WITH_ACK |48/49 |24/25 |24/24 |publish_with_ack_queue_test.sh |Byte-identical |
| (ii) Delete before ACK |48/49 |24/25 |24/24 |publish_with_ack_queue_test.sh |Byte-identical |
| (iii) Remove queuePermitsSleep conjunct |47/49 |24/25 |23/24 |publish_delivery_budget_test.sh; publish_ack_sleep_gate_structural_test.py |Byte-identical |
| (iv) Delete on Future failure/ACK timeout |48/49 |24/25 |24/24 |publish_with_ack_queue_test.sh |Byte-identical |

All four mutations are caught; these reduced pass counts are expected detection, not baseline failures. In particular [tests/publish_with_ack_queue_test.sh:81](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/publish_with_ack_queue_test.sh:81) now inspects the failure branch and :98 rejects mutation(iv). Exact patches, assertions and logs: [test and mutation review](/private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/tests/test-review.md).

## Criteria 1–12

PASS means independently supported within this review. FAIL includes either a demonstrated defect or an explicitly identified unfulfilled evidence requirement; hardware/consumer evidence pending is distinguished from code defects.

| Criterion | Result | File/line evidence and qualification |
|---|---|---|
|1. Explicit WITH_ACK, including persisted events |PASS |[lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:334](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:334) normalizes at dispatch; both binaries verified |
|2. Remove only on ACK success, retain failures |PASS for normal queue outcome handling |[lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:364](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:364), :376, :395; real failure payload retention; mutation(iv) caught. F1 separately exposes sleep with unresolved current RAM |
|3. Bounded sleep/teardown wait |FAIL |Ordinary failure case fixed at [src/state/State_Idle.cpp:240](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Idle.cpp:240); [src/state/State_Sleep.cpp:698](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:698) still allows in-flight teardown. Existing GateFail timeout logged at :706 |
|4. Status delivery counters; unchanged ledger |PASS |Both status variants [src/Generalized-Core-Counter.cpp:2272](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:2272) and :2308 contain a/k/f/r/q/s/b; ledger field schema unchanged; overflow guard at DeviceStatusPublisher.cpp:370. Correct s semantics fail separately under11 |
|5. B tracing |PASS |[lib/BackgroundPublishRK/src/BackgroundPublishRK.cpp:143](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/lib/BackgroundPublishRK/src/BackgroundPublishRK.cpp:143); [src/cloud/Particle_Functions.cpp:24](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/cloud/Particle_Functions.cpp:24) enables pubq/seqfile/coap TRACE and session INFO |
|6. Awake-time increase measured/recorded |FAIL — measurement pending |CycleDelivery is now emitted on both paths ([src/state/State_Sleep.cpp:224](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:224), :1238, :1341), but no pre-B versus candidate hardware measurement exists in permitted evidence; WO:141 describes an unrun bench |
|7. Revised duplicate criterion |FAIL — SUM-widget record missing |Retries preserve payload and are counted ([lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:395](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:395), PublishDeliveryCounters.cpp:90). Decision8/criterion7 in WO:116/:134 track consumer tolerance in WO-004, expressly nonblocking here. Whether any Ubidots dailyoccupancy widget uses SUM remains **unknown**, not confirmed absent. No WO-004 review/deployment or AWS access |
|8. Version v24-Pubq-Ack-B |PASS |[src/Version.cpp:6](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Version.cpp:6) |
|9. Exact binary verification |PASS |Local+cloud instructions/call chains in [binary review](/private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/binaries/binary-review.md) |
|10. Retained placement |PASS |Map quoted below; 24-byte block, version2 at [src/cloud/PublishDeliveryCounters.cpp:30](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/cloud/PublishDeliveryCounters.cpp:30) |
|11. Delivery budget, no in-flight sleep, flash, s, expiry |FAIL |Default90s configurable constant [src/power/ConnectivityPolicy.h:163](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/power/ConnectivityPolicy.h:163), flush :204 and expiry PublishDeliveryGate.cpp:43 work normally; F1/F2/F3 above violate integrated behavior |
|12. Pre-sleep q/CycleDelivery, abandoned, mutation(iv), GateFail q |PASS |State_Sleep.cpp:1238/:1341; PublishDeliveryCounters.cpp:83; test:98; [docs/FIELD_MEANINGS_REFERENCE.md:72](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/docs/FIELD_MEANINGS_REFERENCE.md:72). Requested reset probe passes |

Status event budget test reports714 observed /916 deployed worst case /985 structural worst case, below1023 usable bytes (1024 event limit). Counters saturate independently at65535, so the accounting invariant is expressly limited to below saturation and outside in-flight work. Retry counters quantify retry attempts, not independently confirmed downstream duplicate records.

## Round-1 finding disposition

| Round-1 finding | Result | Evidence |
|---|---|---|
|P1: connected never-recovering failures wedge in IDLE |Resolved for original scenario |Real-source reproduction reaches90.001s/92.002s safe boundary; State_Idle.cpp:240. New decision6 edge/cycle failures listed separately |
|P2: HIBERNATE skips telemetry |Resolved |State_Sleep.cpp:1238 before :1254; ULP :1341 before :1511 |
|P2: reset permanently unbalances counters |Resolved |PublishDeliveryCounters.cpp:83; actual-module a2/k1/f0/b1/r1 |
|P2: mutation(iv) survives |Resolved |48/49 under mutation; publish_with_ack_queue_test.sh:98 fails |
|P3: GateFail q polarity documentation |Resolved |FIELD_MEANINGS_REFERENCE.md:72 says q1 means blocking |

## Candidate binaries and retained map

Both the README local ARM command and `particle compile boron . --target 6.4.1` succeeded (cloud command additionally disables update checks and selects the external output path). Fresh source snapshots and a unique absent-before-build application object directory prevent stale application objects. Both build copies passed296/296 source/config hash checks afterward.

| Candidate | Build sizes | Exact BIN bytes | SHA-256 |
|---|---|---:|---|
|[Local Boron 6.4.1](/private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/binaries/boron-local-6.4.1.bin) |text153508 /data1114 /bss2484 |154626 |`93a3c7641cd344db04f1c4dca4653b72222e94646208dc702e2d0f7047d3127a` |
|[Cloud Boron 6.4.1](/private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/binaries/boron-cloud-6.4.1.bin) |Flash154718 /RAM3578 |154722 |`b216044914dc497bdef57bcb4b7e7c13e39d233fb1c5a5d2849354f3134efc4a` |

Each exact binary contains the NO_ACK-clear/WITH_ACK-set dispatch path, accepted/result hook registrations and targets, in-flight set/read/clear behavior, real budget evaluator, all three delivery gate callers, and both sleep-accounting callers. Local .text matches the BIN byte-for-byte; cloud code was directly disassembled and uniquely matched with relocation handling, then its calls/literals inspected. Full addresses and evidence: [binary review](/private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/binaries/binary-review.md).

Local text is8 bytes below the supplied reference: the reference map has8 more bytes of linker padding, with identical named symbol sizes, data and bss. Cloud Flash matches the reference; RAM is8 bytes below3586. Its exact historical cause cannot be established because the earlier cloud BIN was deleted; this discrepancy is recorded rather than asserted away.

Local map quotes ([map](/private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/binaries/boron-local-6.4.1.map), lines161788,168522,168525,168548–49,168567):

```text
BACKUPSRAM_USER  0x000000002003f400 0x0000000000000c00 xrw
.backup         0x000000002003f400       0xb0 load address 0x00000000000d9b24
 *(.retained_user*)
 .retained_user
                0x000000002003f450       0x18 ../../../build/target/user/platform-13-m/local-source-FbwtgJ//libuser.a(PublishDeliveryCounters.o)
                0x000000002003f4b0                link_global_retained_end = .
```

Matching nm:

```text
2003f450 00000018 d retainedPublishDelivery
```

Thus the block is24 bytes in retained backup SRAM; retained version2 comparison/initialization is proven in both candidates, not only in source.

## Scope and cleanup

No lasting repository edits. Final integrity audit: **2,326/2,326 files byte-identical**, identical directory inventory, Git status and git diff599038e; build-tmp absent. All mutated snapshot files were restored after every run. Temporary source/build copies, unique toolchain application objects, host executables and event directories were removed. Requested BINs, ELF/map, harness source/stubs, review reports, patches and logs remain only in this external scratch directory. [Final integrity audit](/private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/repository-final-validation.json).

Inherited queue inventory inaccuracies (one path q2 with one real file), global rather than per-event retry attribution, and older test wrappers' TMPDIR leftovers are separate observations, not new blockers attributed to this WO. The latter are filed at [out-of-scope test hygiene](/private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/tests/out-of-scope-test-hygiene.md); their generated artifacts were also cleaned. WO-002 through WO-005 and protected documents were not modified or reviewed as implementation targets. No commit, push, stash, reset, checkout, flashing or settings change was performed.
