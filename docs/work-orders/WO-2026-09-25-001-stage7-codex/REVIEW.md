**NOT VERIFIED — WO-2026-09-25-001, Stage 7**

Reviewed the uncommitted working tree on `wo/2026-09-25-001-publish-with-ack` against `599038e`, including untracked source/tests, under Stage 5 decisions 1–5 and criteria 1–10. Model actually used: **gpt-6-astra**, reasoning **ultra**; delegated reviewers inherited the same settings. No implementation fixes were made.

The explicit ACK dispatch and normal success/failure dequeue logic are correct. The review does not pass because the bounded wait is unreachable from an ordinary connected low-power IDLE path, successful Boron HIBERNATE bypasses the new sleep telemetry, interrupted attempts permanently unbalance retained counters, and mutation (iv) survives the entire suite.

**Findings**

1. **P1 — Make the bounded cloud-sync deadline reachable with a pending queue.** [State_Idle.cpp:233](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Idle.cpp:233) requires `getCanSleep()` before entering SLEEPING_STATE at line 260. Its fallback ceiling also requires `queueCanSleep` at lines 278–310. With a connected device and repeated ACK failures, neither route reaches the timer initialized in [State_Sleep.cpp:476](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:476). Ordinary connection success goes to IDLE (`State_Connect.cpp:651–659`), as do already-connected reports (`State_Report.cpp:299`); ThrashGuard excludes IDLE. An exact-source host reproduction ran the power-management/ceiling tail for **3,600,000 simulated ms**, with one queued event, no OTA/sensor/LED work, and a 300,000 ms ceiling: **zero sleep transitions, zero disconnects**. The queue can therefore keep the modem awake without ever reaching the promised `GateFail`. These gate lines predate this change, but the new ACK wait exposes the binding bounded-wait acceptance gap. The fix must enter/enforce the bounded gate while retaining the queue condition on actual disconnect, rather than simply deleting safety conditions. Evidence: [source reproduction and output](delivery-probes/idle-bounded-wait-reproduction.txt), [full trace](delivery-review.md).

2. **P2 — Record q and awake time before successful HIBERNATE.** The new [State_Sleep.cpp:1224](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:1224) snapshot and line 1234 `CycleDelivery` log occur **after** the normal Boron overnight `System.sleep(config)` at [line 1136](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:1136). Successful HIBERNATE resets without returning; only failure falls through. Thus the close-cycle bench gets no new awake sample and the next status event reports the previous q. Put the accounting at both real sleep commit paths. Criteria 4 and 6 fail for successful HIBERNATE.

3. **P2 — Reconcile a retained attempt interrupted by reset.** [PublishDeliveryCounters.cpp:58](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/cloud/PublishDeliveryCounters.cpp:58) preserves valid counters but does not record/reconcile an outstanding attempt. `noteAttempt()` at line 67 increments a; only `noteResult(false)` sets retryPending. Compiling the real module and running `noteAttempt(); begin(); noteAttempt(); noteResult(true);` to represent dispatch, retained reboot, and successful resend gives **a=2, k=1, f=0, r=0**, with nothing in flight. The lost callback permanently violates `a == k + f` and misses the retry. A defined retained outstanding-attempt outcome is needed. Separately, independent saturation at 65535 makes the equality inapplicable once a counter is censored: one failure plus 65,535 successful attempts yields **a=65535, k=65535, f=1, r=1**. Saturation was disclosed by Copilot; the unqualified equality must not be used at that limit. Actual-module probe source/output are in [counter-probes](counter-probes/); full interpretation is in [counters-review.md](counters-review.md).

4. **P2 — Mutation (iv) survives: tests do not enforce failure-path retention.** In an external byte-identical source snapshot, I appended a real `getFileFromQueue(true)` / `removeFileNum(...)` dequeue to `statePublishWait()`'s Future-failure/ACK-timeout branch. **All 48/48 tests passed.** This removes a persisted failed event; it also removes the just-persisted event in the single-RAM-event case. The structural check in [publish_with_ack_queue_test.sh:73](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/publish_with_ack_queue_test.sh:73) finds the first expected success-branch removal and checks its position; it never rejects an additional failure-branch removal. The C++ behavior test executes an independent QueueMirror. Evidence: [exact mutation](iv-remove-on-failure.patch), [full suite output](iv-remove-on-failure-suite.log). Add coverage that exercises the real failure transition or exhaustively forbids a failure-path dequeue.

5. **P3 — GateFail documentation reverses q.** [FIELD_MEANINGS_REFERENCE.md:55](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/docs/FIELD_MEANINGS_REFERENCE.md:55) says q=1 means sleep-safe and describes q=0 as queue-blocked. The actual [State_Sleep.cpp:516](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:516) sets `queuePending = queueEmpty ? 0 : 1` and emits that at line 600. In this log, **q=1 means queue-blocked**; `qn` is the reported count. This matters when interpreting the bench timeout evidence.

**Criteria 1–10**

| Criterion | Result | Evidence / qualification |
|---|---|---|
| 1. Explicit WITH_ACK, including old persisted events | Verified | Queue cpp:301–314 converges on normalization at :334–335; NO_ACK cleared, WITH_ACK set. Both binaries contain it. |
| 2. Remove only on successful ACK Future; retain failures | Source path verified; test coverage fails | Background cpp:126–153 waits for Future; queue cpp:359–408 success-gates dequeue and retains normal failures. Mutation iv survives. This is the normal valid-event publish path, not a claim that inherited capacity/corruption/I/O loss paths do not exist. |
| 3. Sleep/teardown gate and bounded timeout | **Not verified** | Gate itself holds/then logs and disconnects without explicit dequeue, but ordinary connected IDLE never reaches its deadline with a stuck queue. See finding 1. |
| 4. Counters, ledger format, overflow protection | **Not verified overall** | Both event variants include d; ledger format unchanged and guard retained. Hibernate skips q; retained-reset accounting fails. q/r qualifications below. |
| 5. Tracing | Verified | Particle_Functions.cpp:24–32; BackgroundPublishRK.cpp:119–146; queue rejection log :349. |
| 6. Awake-time measurement | **Not verified for every cycle** | CycleDelivery at State_Sleep.cpp:1234 correctly uses finalized awake time for the ULP path; successful HIBERNATE bypasses it. Before/after awake-cost measurement remains bench work. |
| 7. Duplicate analysis / tolerance | **Not established; bench pending** | Firmware retries preserve event payload/timestamp, but a new publish does not preserve a cloud envelope identity. Copilot's fleet-ops key premise is incorrect; see below. No live AWS/Ubidots test performed. |
| 8. Version | Verified | Version.cpp:6 is `v24-Pubq-Ack-B`; present in both candidates. |
| 9. Exact binary paths/hooks | Verified | Both builds succeeded; normalization, dispatch, attempt/result hooks and counter code inspected in each candidate. See binary-review.md and hashes below. |
| 10. Retained placement | Verified | Local map .backup/.retained_user input at 0x2003f450, length 0x14; nm identifies retainedPublishDelivery there. |

**Exact ACK evidence**

[PublishQueuePosixRK.cpp:334](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:334):

```cpp
const PublishFlags sendFlags = (curEvent->flags & ~PublishFlags(NO_ACK)) | WITH_ACK;
if (BackgroundPublishRK::instance().publish(curEvent->eventName, curEvent->eventData, sendFlags,
```

Stored file events are read at :301–304; RAM events are selected at :312–314; both use this dispatch. Stored flags need not be rewritten. PRIVATE remains 0x01; NO_ACK=0x02 is cleared, WITH_ACK=0x08 is set.

[BackgroundPublishRK.cpp:126](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/lib/BackgroundPublishRK/src/BackgroundPublishRK.cpp:126), :129–132 and :150:

```cpp
auto ok = Particle.publish(event_name, event_data, event_flags);
while(!ok.isDone() && state != BACKGROUND_PUBLISH_STOP)
{
    delay(1);
}
// ...
completed_cb(ok.isSucceeded(),
```

Queue cpp:265–266 records `publishComplete = true; publishSuccess = succeeded;`. At :359–360, `if (!publishComplete) { return; }`. At :371 the branch is `if (publishSuccess) {`; only that branch pops/removes the queued file (:379–380) or deletes the current RAM event (:386–387). Failure selects `durationMs = waitAfterFailure;` (:394), leaves file storage intact (:396–399), or executes `ramQueue.push_front(curEvent);` / `writeQueueToFiles();` (:404,408). Pacing remains 2 s after connect, 1 s after success, 30 s after failure.

Installed [Device OS 6.4.1 publisher.cpp:49](/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/src/publisher.cpp:49):

```cpp
if (flags & EventType::NO_ACK) {
    confirmable = false;
} else if (flags & EventType::WITH_ACK) {
    confirmable = true;
}
```

The same file :88–98 sends the message and, critically:

```cpp
// Register completion handler only if acknowledgement was requested explicitly
if ((flags & EventType::WITH_ACK) && msg.has_id()) {
    add_ack_handler(msg.get_id(), std::move(handler));
} else {
    handler.setResult();
}
```

The installed `SEND_EVENT_ACK_TIMEOUT` is 20000 ms (`communication/inc/protocol_defs.h:99`). Explicit ACK closes the proven local-send-success gap. It confirms the Particle protocol exchange, not downstream webhook execution.

**Bounded wait and disconnect persistence**

Inside SLEEPING_STATE, queue `canSleep` is false during a pending dispatch; `stateWait()` derives it from `getNumEvents()==0` during pacing. Application code never calls `setPausePublishing()`. `State_Sleep.cpp:482,512` includes queueEmpty in allComplete; :571–590 returns while still within budget. The queue-aware timeout is 30–120 s, with the output-ledger budget able to extend a smaller budget. On expiry, :596–604 emits **timeout, elapsed, q, qn**, and :618–622 names the unacknowledged-event count and raises alert 43. :637–640 resets only timing bookkeeping. :772–790 requests cloud-only or full teardown; the remaining teardown path proceeds to sleep. **The gate timeout itself does not dequeue anything.** Finding 1 is that this bounded path is not reachable on ordinary pending-queue IDLE flows.

Queue cpp:425–428 handles `cloud_status_disconnecting` (and reset) with `writeQueueToFiles()`. That loops over `ramQueue` at :124–147 and transfers pending RAM entries to files; existing files remain. An already-failed RAM current event is pushed back and persisted by :404–408. A still-in-flight RAM event is held separately in `curEvent`, so the disconnect flush alone does not save it. Device OS TERMINATE aborts the Future; the subsequent queue loop ordinarily processes failure and persists that current event. There is no explicit pre-hibernate completion barrier, and the application's sleep handler precedes its queue loop. This inherited interleaving/I/O limitation is a separate qualification, not a newly reproduced loss or an assertion that timeout intentionally removes events.

**Counter correctness and payloads**

The application registers both hooks at Generalized-Core-Counter.cpp:1139–1151. The attempt hook runs on accepted worker dispatch at queue cpp:342–343; the result hook runs on the application thread at :367–368 before its success/failure decision. All new counter writes are on the application thread, avoiding a new cross-thread counter race. `a` counts accepted dispatches; `k` counts successful Futures; `f` counts failed Futures; `r` uses one retained retryPending flag; `q` replaces the prior sleep snapshot. Normal uninterrupted, unsaturated attempts satisfy a=k+f outside an active attempt; finding 3 establishes the reset exception.

Both status variants have `"d":{"a":%u,"k":%u,"f":%u,"r":%u,"q":%u}` at app cpp:2269,2303, populated from one snapshot at :2265 and queued at :2331. It is boot-time telemetry, not a new per-report publish. Compared with 599038e, DeviceStatusPublisher.cpp has no emitted-field changes, no d object, and no counter read; the guard at :370–376 checks `dataSize() >= sizeof(bufferBase)` before the NUL store at :378.

| Status event profile | PMIC variant | Non-PMIC variant |
|---|---:|---:|
| Typical model, not a captured event | 702 B | 535 B |
| Deployed maximum field-width model | 896 B | 709 B |
| Test's structural model | 965 B | 742 B |

Both fit the **1023 usable bytes** in `char status[1024]` and Device OS / PublishQueuePosixRK's **1024-byte MAX_EVENT_DATA_LENGTH**. The structural float model uses positive FLT_MAX; negative FLT_MAX adds one byte and still fits. Worst-case d cost is 56 B. The ledger test confirms its unchanged 903 B deployed model and existing overflow guard; ledger capacity work is excluded under WO-003.

Two inherited queue behaviors limit the new measurements and are reported separately from new implementation blockers:

- **q is not an exact inventory.** Exact-source extraction of real `writeQueueToFiles()`, `statePublishWait()`, and `getNumEvents()` reproduces one persisted file reporting **2** events after a RAM failure, because curEvent is not cleared after persistence and :251–257 counts it again during the 30 s backoff. New q/timeout logs inherit this known Stage 4 defect. Other mixed RAM/file cases can undercount. [Probe result](counter-probes/queue_depth_result.txt).
- **r lacks event identity.** Ordinary disconnect can persist queued RAM B before in-flight RAM A fails and gets persisted. The global retryPending flag then credits B's first attempt as A's retry. Aggregate r may balance later, but event attribution is not guaranteed. Corrupt-file discard can similarly redirect the flag. The retained-reset case produces an actual missing retry count.

**Criterion 7 analysis correction**

Firmware retries send the original eventName/eventData unchanged; the report timestamp remains in that data. The worker nevertheless makes a **new Particle.publish call** for each queue reattempt. Copilot's premise that fleet-ops keys storage on the timestamp inside that data is false: read-only local `lambda/src/utils/parse.ts:41–42` returns `body.published_at || body.timestamp || now`, while `ingestion.ts:101` separately parses `body.data`. Lines 99,113,137–145 use that top-level time for S3 and the Dynamo event key; history also includes ctx.publishedAt. If two queue attempts receive different envelope published_at values, the same payload creates distinct raw/index/history keys. [Conditional key demonstration](counter-probes/distinct_envelope_keys_result.json) verifies the key logic, without assuming live timestamp behavior or accessing AWS.

Thus the firmware-side preservation of payload is sound, but it does not prove the supplied deduplication conclusion. Decision 5 leaves actual fleet-ops/Ubidots outcomes to the bench, including SUM-widget behavior. That bench must include a true new firmware publish after a lost ACK, not just replay of the identical cloud envelope. No backend changes or Ubidots investigation beyond the supplied context were performed.

**Host suite and mutations**

**48/48 (sh via zsh, py via python3): 24/24 zsh scripts and 24/24 Python scripts.** All ran against a byte-identical external source snapshot, with TMPDIR outside the repository. Two tests require Git history; the first snapshot-only run failed those two because it had no Git context. Supplying read-only original GIT_DIR and snapshot GIT_WORK_TREE fixed that environment issue; the complete baseline rerun passed 48/48. No tests were modified.

| Mutation | Suite pass count | Detection |
|---|---:|---|
| (i) Stored flags only; drop WITH_ACK | 47/48 | publish_with_ack_queue_test.sh failed normalization check |
| (ii) Remove persisted current event at dispatch before ACK | 47/48 | publish_with_ack_queue_test.sh failed stateWait removal-count check |
| (iii) Remove queueEmpty from sleep gate | 47/48 | publish_ack_sleep_gate_structural_test.py failed |
| (iv) Dequeue on Future failure / ACK timeout | **48/48** | **SURVIVED — finding 4** |

Every mutation was restored immediately in a finally block and checked byte-for-byte. After all four, all **1,223 snapshot files** matched the original SHA-256 manifest. [Machine-readable mutation results](mutation-results.json), [restoration verification](mutation-restoration-verification.json), and per-test logs are retained. The bounded-wait, reset, saturation and q probes are additional review reproductions; they are not counted as members of the 48-test repository suite.

**Builds, binary verification and retained placement**

Binary results and the final repository integrity/cleanup record are below. Detailed disassembly, exact commands, map and symbol evidence are in [binary-review.md](binary-review.md).

| Candidate | Build sizes | Binary bytes | SHA-256 |
|---|---|---:|---|
| [Local Boron 6.4.1](boron-local-6.4.1.bin) | text 152572 / data 1114 / bss 2468 | 153690 | `eb956a11227982f218fd1cac28e3aa3d3d7a3406f180cca9fc1ff3f2efed16a6` |
| [Particle cloud Boron 6.4.1](boron-cloud-6.4.1.bin) | Flash 153782 / RAM 3570 | 153786 | `8ad80f1968e89eedf33105b1e2f21b626f46d44454a041b1fa41236d16043899` |

The README local command used a fresh APPDIR snapshot and fresh user objects outside the repository. The reference was 152580/1110/2468; this build is −8/+4/0. Map comparison accounts for the difference: text fill/alignment totals 274 versus 282 bytes, and retained-section ordering/padding adds four bytes. Common symbol sizes match. This is an actual clean candidate, not the older reference ELF relabeled. The fresh retained counter address consequently differs from Copilot's 0x2003f494. The cloud compile used `particle compile boron . --target 6.4.1` against the exact source snapshot, with CLI update checks disabled; its first service attempt timed out, and the retry succeeded with the exact reference Flash/RAM sizes.

Machine-code addresses (local / cloud): NO_ACK clear **0xc730e / 0xc64ca**, WITH_ACK set **0xc7316 / 0xc64d2**, background dispatch **0xc7336 / 0xc64f2**, attempt hook **0xc73c2 / 0xc657e**, result hook **0xc77a0 / 0xc695c**. Initializers and literal pointers establish that the cleared/set values are 2 and 8. Both local dependencies/map and cloud compilation/disassembly establish that the vendored queue/background code is compiled. `project.properties` no longer activates the registry PublishQueuePosixRK=0.0.7 dependency; this accepted decision is essential to retain the patched dispatch and hook interface in the cloud build. Strings alone were not used to assert WITH_ACK correctness: normalization, flag constants, dispatch and both hook paths were checked as executable instructions. The local linked ELF also exposes the new counter module symbols and setup callback call sites.

Local linker map exact lines (165901, 165904, 165927–165928):

```text
.backup         0x000000002003f400       0xb0 load address 0x00000000000d977c
 *(.retained_user*)
 .retained_user
                0x000000002003f450       0x14 ../../../build/target/user/platform-13-m/build-source//libuser.a(PublishDeliveryCounters.o)
```

The map identifies the input section rather than printing this local symbol name. `arm-none-eabi-nm -S -C` supplies the matching name, address and size:

```text
2003f450 00000014 d retainedPublishDelivery
```

Thus the 20-byte symbol lies at **[0x2003f450, 0x2003f464)** within the retained `.backup` range **[0x2003f400, 0x2003f4b0)**. The preserved [ELF](boron-local-6.4.1.elf), [map](boron-local-6.4.1.map), symbols and disassembly identify these exact candidate artifacts.

**Scope, integrity and cleanup**

No commits, pushes, stash, reset, checkout, flashing, device-setting changes or AWS access occurred. Network activity was limited to the authorized Particle compile. No files under docs/work-orders, the workflow, or WO-002/003 were edited or included as implementation review targets; they were read only for the supplied scope/context. Backend source was read only to verify the requested duplicate analysis.

Original repository verification: **1,223/1,223 files unchanged byte-for-byte**, no added files, identical git status and identical diff against 599038e; see [final validation](repository-final-validation.json). All four source mutations were made only in a byte-identical external test snapshot and restored byte-identically after each. Test/build source copies, mutation drivers, host executables and temporary queue files were removed; the requested two candidate binaries and their review evidence are intentionally retained outside the repository. The preserved binaries have verified contents and hashes; the firmware review verdict remains **NOT VERIFIED**.
