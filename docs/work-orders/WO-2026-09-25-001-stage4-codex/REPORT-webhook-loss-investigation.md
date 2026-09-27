# Webhook loss investigation — 2026-09-25

**Finding: the queue's delivery guarantee is broken by PRIVATE-only publishes, but the precise cause of the observed packet losses is still unproven.** The installed Device OS resolves these publish Futures after a successful local/channel send, without waiting for the cloud ACK. PublishQueuePosix then deletes the event. Its normal sleep path can disconnect non-gracefully and destroy the lower-layer retransmission copy. This is a concrete mechanism for “accepted → drained → never delivered,” and the first bench candidate should require explicit `WITH_ACK`. It is not evidence that Particle's cloud lost the events, nor proof of a defective fixed window immediately after connection.

Investigator: **Codex, gpt-6-astra, ultra reasoning**, with delegated investigators using the inherited model/reasoning configuration. The older model assignment in the repository workflow was superseded by this dispatch.

Scope honored: source, AWS and Particle inventory reads; scratch-only reports, evidence and two **unapplied** draft diffs. No checkout, stash, reset, commit, push, build, flash, device operation, configuration change or infrastructure write was performed. Shell commands used **zsh**; data parsing used Python 3/Node.js. No shell test suite was run.

## Evidence and confidence

| Conclusion | Confidence | Basis |
|---|---|---|
| PRIVATE-only Future success is not a cloud ACK | High; proven in installed source | Device OS `communication/src/publisher.cpp:88–98` |
| Queue can remove PRIVATE-only events without waiting for cloud confirmation | High; proven in installed source | Queue dispatch and success deletion; BackgroundPublish Future handling |
| Non-graceful disconnect can erase outstanding default CoAP retries | High for mechanism | Actual app disconnect calls plus Device OS TERMINATE/reset path |
| That mechanism caused the 15:00 losses | Plausible, moderate; **not proven** | Very short connection service window, reports absent at ingest, no per-attempt ACK trace |
| All losses occur in a fixed initial N-second window | Unsupported; deterministic version contradicted by delivered early events | First `pdiag` delivered at 15:00; successful offline queues; no observed send timestamps |
| A session-resume implementation bug caused these losses | Unproven | Session and send/ACK traces absent; Device OS has explicit handshake readiness checks |
| Production devices have the same measured loss rate | Unknown | Delivery history exists, but no serial generation denominator or historical Ledger record for them |

Repository inspected: `/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter`, branch `wo/2026-09-24-001-daily-cleanup-boundary-fix`, HEAD `8e7a72fda8ff7c0d7eb71e48132a81d98403eb1a`, **including its pre-existing staged working-tree changes**. Drafts target these current file contents, not HEAD. Device OS source root: `/Users/chipmc/.particle/toolchains/deviceOS/6.4.1`. Library versions: PublishQueuePosixRK **0.0.7**, BackgroundPublishRK **0.0.2**, SequentialFileRK **0.0.2**. Unless prefixed “Device OS,” source references below are relative to that repository. `repo-status-before.txt`, `repo-head.txt` and `repo-checksums-before.json` preserve the initial reference state.

Detailed, line-numbered source evidence is retained in [QUEUE-findings.md](QUEUE-findings.md) and [DOS-findings.md](DOS-findings.md); raw Dev-14 delivery/ingest evidence and timestamps are in [DEV14-evidence.md](DEV14-evidence.md). Source analysis establishes the behavior of this tree, not a cryptographically verified match to the flashed binary. The shared `v24-Thermal-Inhibit` string does not identify an exact source revision.

## 1. What success means, and the actual flags

The report passes **PRIVATE only** (`0x01`):

```cpp
// src/Generalized-Core-Counter.cpp:2061
bool queued = PublishQueuePosix::instance().publish(webhookName, data, PRIVATE);
```

`status` at line 2283, `watchdog` at 2323, `hibernate_wake` at 2358 and `pdiag` in `src/power/PowerDiagnostics.cpp:439` likewise pass PRIVATE. `publishDiagnosticSafe()` forwards its supplied flags unchanged at app line 2471; its default and current callers use PRIVATE. The queue exposes `WITH_ACK` through its existing second flags argument or `PRIVATE | WITH_ACK`; no new library API is needed:

```cpp
// lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h:181
inline bool publish(const char *eventName, const char *data, PublishFlags flags1, PublishFlags flags2 = PublishFlags()) {
// lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:71
PublishQueueEvent *event = newRamEvent(eventName, eventData, flags1 | flags2);
// :114
 event->flags = flags;
// :331–333
if (BackgroundPublishRK::instance().publish(curEvent->eventName, curEvent->eventData, curEvent->flags,
    [this](bool succeeded, const char *eventName, const char *eventData, const void *context) {
        publishCompleteCallback(succeeded, eventName, eventData);
```

There are **three different results**, which must not be conflated:

1. Queue `publish()` returns acceptance (`Report: q=1`), not transmission. It can even return true after unchecked persistence failure; see removal paths below.
2. BackgroundPublish `publish()` returns whether the worker accepted a request. The queue does not delete on this boolean alone.
3. The worker waits for the Device OS Future and supplies its success to the queue. Only when explicit WITH_ACK was supplied does that success represent an ACK.

```cpp
// lib/BackgroundPublishRK/src/BackgroundPublishRK.cpp:83–94
 auto ok = Particle.publish(event_name, event_data, event_flags);
 while(!ok.isDone() && state != BACKGROUND_PUBLISH_STOP)
 {
     delay(1);
 }
 if(completed_cb)
 {
     completed_cb(ok.isSucceeded(),
```

The decisive Device OS 6.4.1 distinction (`communication/src/publisher.cpp:49–57,88–98`) is:

```cpp
bool confirmable = channel.is_unreliable();
if (flags & EventType::NO_ACK) {
    confirmable = false;
} else if (flags & EventType::WITH_ACK) {
    confirmable = true;
}
// ...
e.type(confirmable ? CoapType::CON : CoapType::NON);
// ...
err = channel.send(msg);
if (err != ProtocolError::NO_ERROR) {
    return err;
}
// Register completion handler only if acknowledgement was requested explicitly
if ((flags & EventType::WITH_ACK) && msg.has_id()) {
    add_ack_handler(msg.get_id(), std::move(handler));
} else {
    handler.setResult();
}
```

Thus the effective default is **neither explicit WITH_ACK nor explicit NO_ACK**. On Boron's DTLS/UDP channel PRIVATE-only normally creates a CoAP CON message and internally retries it. However its Future succeeds immediately after `channel.send()` succeeds. Explicit NO_ACK disables confirmability; explicit WITH_ACK also binds Future completion to the cloud acknowledgment. Constants are PRIVATE `0x01`, NO_ACK `0x02`, WITH_ACK `0x08` (`system/inc/system_cloud.h:178–181`). Wiring combines supplied flags without adding WITH_ACK (`wiring/inc/spark_wiring_cloud.h:298–300`). `publishCompletionCallback()` sets the Future result to true on completion success (`wiring/src/spark_wiring_cloud.cpp:27–33`).

This agrees with Particle's primary documentation: default UDP retransmission can happen internally while only WITH_ACK makes API completion wait for acknowledgment. [Particle classic publish reference](https://docs.particle.io/reference/device-os/api/publish/particle-publish-classic-api-publish/). The installed 6.4.1 source above is the version-specific authority.

WITH_ACK confirms the Particle protocol exchange, **not execution of either integration or acceptance by Ubidots**. The ACK timeout is 20 seconds (`communication/inc/protocol_defs.h:99`); timeout/reset/error produces Future failure. A lost ACK can cause duplicate delivery on retransmission/requeue; it is an at-least-once mechanism, not an exactly-once integration transaction.

## 2. Every queue removal or loss path

The following includes intentional removal, transfer out of RAM, administrative clearing, invalid-file handling and failure paths. Exhaustive surrounding excerpts are in QUEUE-findings.md.

**Successful Future — real dequeue.** `lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:348–365`:

```cpp
if (publishSuccess) {
    // Remove from the queue
    _log.trace("publish success %d", curFileNum);
    if (curFileNum) {
        int fileNum = fileQueue.getFileFromQueue(false);
        if (fileNum == curFileNum) {
            fileQueue.getFileFromQueue(true);
            fileQueue.removeFileNum(fileNum, false);
            _log.trace("removed file %d", fileNum);
        }
        curFileNum = 0;
    }
    delete curEvent;
    curEvent = NULL;
    durationMs = waitBetweenPublish;
}
```

**Failure — retry, with no retry-count limit.** Lines 367–385:

```cpp
else {
    // Wait and retry
    _log.trace("publish failed %d", curFileNum);
    durationMs = waitAfterFailure;
    if (curFileNum) {
        delete curEvent;
        curEvent = NULL;
    }
    else {
        WITH_LOCK(*this) {
            ramQueue.push_front(curEvent);
        }
        _log.trace("writing to files after publish failure");
        writeQueueToFiles();
    }
}
```

For a file event, only the temporary RAM copy is deleted; the queued file remains. For RAM, the event is put back then persisted. `waitAfterFailure=30000` ms. There is **no “drop after N failed attempts” path**. A worker-dispatch rejection at lines 331–336 is not handled at all: state remains publish-wait with `publishComplete=false`, so it stalls rather than intentionally deleting. The drafts log this without changing it.

**Invalid/unreadable file — unconditional discard.** Lines 301–308:

```cpp
curFileNum = fileQueue.getFileFromQueue(false);
if (curFileNum) {
    curEvent = readQueueFile(curFileNum);
    if (!curEvent) {
        // Probably a corrupted file, discard
        _log.info("discarding corrupted file %d", curFileNum);
        fileQueue.getFileFromQueue(true);
        fileQueue.removeFileNum(curFileNum, false);
    }
}
```

`readQueueFile()` lines 153–195 returns null after invalid header/length/string checks or allocation failure; “corrupted” is not a proven diagnosis. Its own `if (fd)` also mishandles open errors: −1 is treated as valid, and unchecked `fstat`/reads can then use uninitialized metadata. Failed opens are therefore not safely or predictably handled. The source also rejects an event name of length 64 on file read while enqueue permits it; this is irrelevant to the shorter webhook name here.

**800-file capacity — discard oldest queued file references without publishing.** Lines 233–238:

```cpp
while(fileQueue.getQueueLen() > (int)fileQueueSize) {
    int fileNum = fileQueue.getFileFromQueue(true);
    if (fileNum) {
        fileQueue.removeFileNum(fileNum, false);
        _log.info("discarded event %d", fileNum);
    }
}
```

App setup selects `.withFileQueueSize(800)` (`src/Generalized-Core-Counter.cpp:1132–1134`). Setup and each enqueue enforce the cap. This is a count of **all event types**, not 800 reports; pdiag traffic consumes it. Queue depths 3–4 do not support overflow as the explanation for the supplied wakes. No observed low depth excludes earlier overflow without a complete capture.

**RAM → file transfer — can lose data on unchecked I/O.** Lines 124–147:

```cpp
while(!ramQueue.empty()) {
    PublishQueueEvent *event = ramQueue.front();
    ramQueue.pop_front();

    int fileNum = fileQueue.reserveFile();

    int fd = open(fileQueue.getPathForFileNum(fileNum), O_RDWR | O_CREAT);
    if (fd) {
        PublishQueueFileHeader hdr;
        hdr.magic = FILE_MAGIC;
        hdr.version = FILE_VERSION;
        hdr.headerSize = sizeof(PublishQueueFileHeader);
        hdr.nameLen = sizeof(PublishQueueEvent::eventName);
        write(fd, &hdr, sizeof(hdr));

        write(fd, event, sizeof(PublishQueueEvent) + strlen(event->eventData));
        close(fd);

        // This message is monitored by the automated test tool. If you edit this, change that too.
        _log.trace("writeQueueToFiles fileNum=%d", fileNum);
    }
    fileQueue.addFileToQueue(fileNum);

    delete event;
}
```

The test is `if (fd)`, not `fd >= 0`; `-1` passes and descriptor zero fails. Writes are unchecked, RAM is popped before I/O and deleted regardless. An enqueue can therefore return true even if its durable copy was not successfully written. This is an independent credible defect, **not an observed I/O failure in these wakes**. Failed RAM publishes use this same path.

**Administrative clear.** Lines 198–209:

```cpp
while(!ramQueue.empty()) {
    PublishQueueEvent *event = ramQueue.front();
    ramQueue.pop_front();
    delete event;
}
fileQueue.removeAll(true);
```

`clearQueues()` is exposed but no application caller was found. It does not explicitly dispose the current in-flight RAM event. SequentialFileRK implements physical deletion with `unlink()`; `removeAll(true)` clears the file-number list. See its exact line excerpts in QUEUE-findings.md.

**RAM dequeue for sending is a transfer, not final success.** Lines 312–314 move `ramQueue.front()` to `curEvent`, then pop it. Failure normally requeues it; a reset while that separate RAM event is in flight can lose it because reset/disconnecting persistence flushes only `ramQueue` (lines 402–405), not `curEvent`.

**Reset/hibernate qualification.** An offline enqueue is normally written to files immediately (lines 82–89), and orderly reset/disconnecting notifications flush queued RAM. Therefore “a reset clears the RAM queue” cannot by itself explain an event independently established to have been queued offline. A pin/power reset may bypass the callback; an in-flight RAM event is vulnerable. The supplied flash/reset attribution requires the missing trace to establish which storage/state it occupied.

**Answer:** yes, an event can be removed without an ACK, through successful PRIVATE-only Future completion. It can also disappear without any attempt through overflow, invalid-file discard, clear, failed persistence, or unprotected RAM reset. An explicitly reported publish failure is normally retained/retried; there is no retry-limit deletion.

## 3. Connection timing, readiness and sleep

The queue already waits **2000 ms after its application loop observes `Particle.connected()`**, then spaces successful completions by 1000 ms. The settings are `waitAfterConnect=2000`, `waitBetweenPublish=1000`, `waitAfterFailure=30000` in `PublishQueuePosixRK.h:414–416`.

```cpp
// PublishQueuePosixRK.cpp:277–280
if (Particle.connected()) {
    stateTime = millis();
    durationMs = waitAfterConnect;
    stateHandler = &PublishQueuePosix::stateWait;
}
// :285–298
if (!Particle.connected()) {
    stateHandler = &PublishQueuePosix::stateConnectWait;
    return;
}
if (pausePublishing) {
    canSleep = true;
    return;
}
if (millis() - stateTime < durationMs) {
    canSleep = (getNumEvents() == 0);
    return;
}
```

It has no additional handshake/application readiness test. `setPausePublishing()` exists and is unused by the app. **Simply pausing is dangerous in this application:** the queue marks itself sleep-safe while paused even if events remain, so a delay experiment must also hold the application's sleep gate. Neither delivered candidate adds a pause.

Device OS already gates “connected” on handshake readiness:

```cpp
// Device OS system/src/system_task.cpp:367–380
if (SPARK_CLOUD_HANDSHAKE_NOTIFY_DONE) {
    // ... one more iteration ... all handshake messages
    // have been acknowledged successfully
    if (!Spark_Communication_Loop()) {
        err = protocol::MESSAGE_TIMEOUT;
    } else {
        INFO("Cloud connected");
        SPARK_CLOUD_CONNECTED = 1;
        // ...
        system_notify_event(cloud_status, cloud_status_connected);
```

“Failed to load session data from persistent storage” (`system/src/system_cloud_connection.cpp:107`) is a failed saved-session restore followed by normal handshake fallback. It is not evidence that a publish was accepted or discarded. Successful resume returns SESSION_RESUMED; with matching state Device OS sends a CoAP ping, while changed state takes the HELLO/description/subscription path (`communication/src/protocol.cpp:496–512` and related excerpts in DOS-findings.md). The system waits for pending handshake work before reporting connected. There is no source-level requirement to wait an arbitrary additional N seconds. The supplied two USB excerpts contain neither the saved-session warning nor resume trace, so those paths cannot be assigned retrospectively to each wake from these excerpts.

The app's queue gate is only a library state check (`State_Idle.cpp:233–260`, `State_Sleep.cpp:481–511`). Teardown calls `Particle.disconnect()` without graceful options (`src/power/Connectivity.h:38–47`). Device OS defaults to non-graceful disconnect (`system/inc/system_cloud_internal.h:138`) and selects TERMINATE; TERMINATE resets immediately (`communication/inc/dtls_protocol.h:103–105`), clearing the CoAP retransmit store (`communication/inc/coap_channel.h:584–588`). Default retransmissions otherwise occur after roughly 4–6, then 8–12, then 16–24 seconds. A report deleted at +3 or +4 seconds can lose its remaining transport chance when cloud teardown begins around +4.85 seconds.

There is a **second unresolved gate detail**: current source arms a webhook response wait on the report and should normally wait for a response/timeout before disconnecting. At 15:00 the excerpt shows teardown before even the minimum allowed 5-second response timeout (configured snapshot says 20 seconds), with no timeout log. The app subscribes to both device-ID and broad `hook-response/` prefixes (`src/Generalized-Core-Counter.cpp:955–964`), and `UbidotsHandler()` clears the single wait flag on **any nonempty response**, before checking status (`src/Generalized-Core-Counter.cpp:2394–2411`). An unrelated response could release it, silently when verbose mode is false; that is plausible but unproven without response-topic trace. Binary mismatch or missing capture detail also remain possible. Do not claim queue drain alone mechanically explains every gate transition.

A conditional old SARA-R410 modem branch can return success after deliberately dropping a burst, but **it does not fit Dev-14's identified BRN404X/SARA-R510 hardware**. See DOS-findings.md for primary hardware documentation and source conditions. It is not the recommended diagnosis.

## 4. Five wakes and the counterexamples

**Clock correction:** some supplied labels mix UTC and SGT. The raw object for “06:18:24” is `2026-09-25T06:18:24.356Z`, which is **14:18:24 SGT**; its report payload timestamp is 06:15:35Z / 14:15:35 SGT. The flash/reset sequence called “06:13 / 06:15:32” appears to refer to that UTC cluster, whereas 14:41, 15:00, 15:22 and 15:49 are SGT. The 06:13Z delivered reports carry total 493 minutes; the 06:18Z report carries zero. I preserve the supplied flash-loss claim separately instead of silently treating these as the same clock convention.

`published_at` below is Particle's event timestamp, not the device's send time. `receivedAt` is AWS receipt. The prior 15:00 `.574Z` figure is **receipt**; the pdiag Particle timestamp is `.182Z`. USB uptime deltas are better evidence for in-session order than serial-forwarder wall times, which visibly lag and drop lines. No send-attempt trace exists for these historical wakes, so exact send times and a complete event roster cannot be recovered.

| Wake / evidence | Best-supported queue order and timing | Delivered / absent | What can be concluded |
|---|---|---|---|
| Supplied “06:13 flash” / apparent 14:13 SGT cluster | Raw order: `status` 14:13:19.147, report total 493 14:13:20.187, pdiag 14:13:44.671, another total 493 report 14:13:44.847. Exact enqueue/connection trace for the claimed extra zero report is unavailable. | Those four events delivered. The separately claimed zero report lost before the 06:15:32Z reset cannot be independently identified in the provided USB excerpts. | Do not attribute this to early connection timing. The stated RAM-reset cause is supplied context, not proven by current-source storage semantics; offline enqueues normally persist immediately. |
| 14:41 occupancy end | Likely carried-over pdiag first, then report, consistent with ULP-wake flush followed by report enqueue. The report/connection send-attempt lines are unavailable. | pdiag 14:41:19.698; no report in S3 for this wake. Current-cycle pdiag was delivered at 15:00. | Report generation/loss is supported by supplied observation, while send slot and exact age since connected remain inference. |
| 15:00 daily boundary | At uptime 2,667,657 queue already has one event. Two accepted reports at 2,668,168 and 2,668,349; ConnSummary at 2,671,631, q=3. FIFO strongly supports `[prior-cycle pdiag, total9 report, total 0 report]`. Nominal earliest slots are around connection observation +2,+3,+4s, with actual loop/network delays unknown. | Only pdiag published 15:00:11.182, received 15:00:11.574. No Particle report request during 07:00:00–07:00:40Z; no matching report in the wider S3 history. | Queue drains 4.830s after ConnSummary; Sleep enters at +4.836s, PPP teardown appears +4.848s. First event succeeds while later reports fail: the deterministic “first N seconds are lost” rule does not fit. Early teardown of unconfirmed traffic is plausible. |
| 15:22 reset, no report | ConnSummary uptime 17,380 q=4; drained 24,022 (+6.642s). Known logical events: persisted prior-cycle pdiag, new hibernate_wake, new startup status. Fourth identity/order unknown. Nominal four rapid successes would occupy about +2,+3,+4,+5s, but this is not an observed trace. | pdiag 15:22:27.643, hibernate_wake 15:22:28.821, status 15:22:31.216. Ingest independently agrees. | q=4 vs three events is a count discrepancy, not proof of the missing event's identity. Ledger gate keeps connection until uptime 39,030; PPP error at 39,042 (+21.662s), still possibly short of final lower-layer retry. |
| 15:49 reset, report generated online | Supplied connected time≈15:49:22, q=3 before the later report. Likely persisted pdiag, hibernate_wake, status, then online report. Report payload stamp 15:49:28; exact queue send times absent. | pdiag 15:49:31.723, hibernate_wake 15:49:34.261, report 15:49:39.485 (AWS 15:49:39.833). No status for this wake in inspected timeline. | Successful report is a useful later-slot control. Status absence suggests another loss but is not conclusively identified by q alone. |

The two supplied 15:00 Report lines are truly **two reports**. `State_Report.cpp:77` publishes pre-cleanup data, `dailyCleanup()` runs at 78, and line 93 publishes again. The zero/nonzero payloads must not be deduplicated just because they have the same second-level timestamp. `Report:q=1` is acceptance, not queue depth; ConnSummary q uses `getNumEvents()`.

Queue depth is not an event inventory. `getNumEvents()` returns only RAM queue size when nonzero, omitting file count/in-flight RAM in that branch. A failed RAM attempt also leaves a stale non-null `curEvent` after transferring it to disk, permitting a temporary overcount. Files are processed before RAM; boot directory enumeration does not sort file numbers (`SequentialFileRK.cpp:48–75`). These caveats especially constrain conclusions drawn solely from reset-wake q values.

**Carried-over pdiag is directly established by payloads.** The pdiag delivered at 15:00 contains captured uptime 1,534,792–1,544,904 from the preceding cycle, not the 15:00 connect. At 15:22 it contains the 15:00 observations through uptime 2,685,396. At 15:49 it contains the 15:22 observations through 60,944. This matches `flushDiagBatch()` after ULP wake (`State_Sleep.cpp:1483–1489`) and before hibernate after disconnection (`1118–1125`), with offline queue persistence. Do not use a pdiag capture timestamp as its send time.

**14:36 delayed success.** S3 contains a report published **14:36:05.562 SGT**, received 14:36:05.862, with payload timestamp **14:31:14**, occupancy 1, total 0, resets 1. Generation-to-publication delay is **291.562 seconds**. This demonstrates successful delayed delivery from an offline interval. The raw record itself does not establish that this particular report was generated by a flash, or its exact connection age at send. pdiag precedes it at 14:36:04.567.

**DNS-failure delayed success.** S3 contains report **14:18:24.356 SGT**, received 14:18:24.670, payload timestamp 14:15:35, occupancy 0, total 0, resets 1: **169.356 seconds** delayed. Startup status arrived one second earlier at 14:18:23.301. The later pdiag contains connect-success capture around uptime 164,397, consistent with a roughly 2.7-minute connection wait. DNS failures are supplied context; their individual log lines are absent from the retrieved forwarder capture. This is a counterexample to “queued offline is intrinsically unsafe,” though the missing actual send timestamp prevents assigning a numeric safe threshold.

**Pattern verdict:** there is no defensible N that explains every case from the evidence. The data support a broader reliability gap around short connection service periods; they do not establish a universal startup blackout. The fleet comparison below includes both failures and successes in the same preconnection/q=2 class.

## 5. Fourteen-day fleet scope

Read-only extraction window: **2026-09-11 08:00:00 UTC ≤ event time < 2026-09-25 08:00:00 UTC**, exactly 14 days (2026-09-11 16:00 to 2026-09-25 16:00 SGT). Snapshot inventory was fetched at approximately 08:01 UTC Sep 25. Product 42131 inventory and project current state both contain the same 12 devices, including all nine production-named devices. Daily rows are UTC; first and last rows are partial days.

**Observed result:** 5,992 device Ubidots deliveries, 115 captured `Report:` lines, 105 matched captured reports, and 10 unmatched captured reports. This is an **8.7% unmatched fraction in a sparse, biased serial cohort**, not a measured fleet loss rate. True per-device/per-firmware/per-day loss rates cannot be determined from this archive because generated-but-undelivered reports without serial capture leave no complete historical denominator. All nine production devices have zero serial records. The 41 captured Dev-14 reports all match deliveries, despite independently established Sep 25 Dev-14 losses outside forwarder coverage. Thus even 0% here emphatically does not mean loss-free.

### Per device

| Device | Event firmware | S3 delivered | Serial rows | Report lines | Matched | Unmatched | Cohort unmatched |
|---|---:|---:|---:|---:|---:|---:|---:|
| Boron-Dev-11 | 24 | 277 | 14467 | 27 | 24 | 3 | 11.1% |
| Boron-Dev-14 | 24 | 574 | 6563 | 41 | 41 | 0 | 0.0% |
| Boron-Dev-09 | 24 | 387 | 11839 | 47 | 40 | 7 | 14.9% |
| ToM-MCP-PCKL1-JAN14-2025 | 21 | 475 | 0 | 0 | 0 | 0 | N/A |
| ToM-MCP-PCKL3 | 21 | 667 | 0 | 0 | 0 | 0 | N/A |
| Morrisville-Tennis-MAFC-1-SWAPPED | 21 | 500 | 0 | 0 | 0 | 0 | N/A |
| Morrisville-Tennis-MAFC-2 | 21 | 466 | 0 | 0 | 0 | 0 | N/A |
| ToM-MCP-Court2 | 21 | 721 | 0 | 0 | 0 | 0 | N/A |
| ToM-MCP-Court1 | 21 | 630 | 0 | 0 | 0 | 0 | N/A |
| ToM-MCP-Court3-DEC28-2024 | 21 | 439 | 0 | 0 | 0 | 0 | N/A |
| SAMIT-TRAIL02 | 24 | 263 | 0 | 0 | 0 | 0 | N/A |
| ToM-MCP-PCKL2 | 21 | 593 | 0 | 0 | 0 | 0 | N/A |

Production-named devices use current build `21.0_Test` except SAMIT-TRAIL02, which uses `v24-Thermal-Inhibit`. Dev09/11/14 currently report `v24-Thermal-Inhibit`. Event `fw_version` is numeric 21/24; semantic version strings from the latest ledger do not establish a historical build identity for every event (several changes can reuse version 24). All currently report Device OS 6.4.1. No firmware/version serial lines survived this window.

### Per firmware

| Event fw_version | S3 delivered | Report lines | Matched | Unmatched | Cohort unmatched | Actual loss rate |
|---|---:|---:|---:|---:|---:|---|
| 21 | 4491 | 0 | 0 | 0 | N/A | Not identifiable |
| 24 | 1501 | 115 | 105 | 10 | 8.7% | Not identifiable |

### Per UTC day

| UTC day | S3 delivered | Report lines | Matched | Unmatched | Cohort unmatched |
|---|---:|---:|---:|---:|---:|
| 2026-09-11 (partial) | 288 | 2 | 2 | 0 | 0.0% |
| 2026-09-12 | 402 | 3 | 3 | 0 | 0.0% |
| 2026-09-13 | 505 | 5 | 5 | 0 | 0.0% |
| 2026-09-14 | 435 | 15 | 15 | 0 | 0.0% |
| 2026-09-15 | 413 | 3 | 3 | 0 | 0.0% |
| 2026-09-16 | 427 | 8 | 8 | 0 | 0.0% |
| 2026-09-17 | 426 | 8 | 8 | 0 | 0.0% |
| 2026-09-18 | 448 | 15 | 13 | 2 | 13.3% |
| 2026-09-19 | 504 | 10 | 10 | 0 | 0.0% |
| 2026-09-20 | 441 | 6 | 3 | 3 | 50.0% |
| 2026-09-21 | 438 | 10 | 9 | 1 | 10.0% |
| 2026-09-22 | 346 | 8 | 7 | 1 | 12.5% |
| 2026-09-23 | 354 | 6 | 4 | 2 | 33.3% |
| 2026-09-24 | 457 | 10 | 9 | 1 | 10.0% |
| 2026-09-25 (partial) | 108 | 6 | 6 | 0 | 0.0% |

The delivered column is grouped by Particle published time, and serial cohort counts by forwarder capture time. These columns are not a subtraction-based loss calculation: a report can be delivered on a later day. Full device × firmware × day counts are in `fleet-data/scope-by-device-firmware-day.csv`.

### Connection association and unmatched reports

Seven of the ten unmatched reports were logged **2.173–4.057 seconds before `ConnSummary: ok` by device uptime**, with queue depth **2** at that connection. Each of those seven connections has a received `pdiag` but no matching report. This supports a connection/sleep-boundary problem extending to Dev09 and Dev11. It does not identify the actual transmit time or prove the first N seconds are universally unsafe: **32 matching reports were also generated ≤10 seconds before a q=2 connection**, and 60 of 67 observed reports generated ≤10 seconds before any recorded connection delivered. In the broader comparison: preconnect ≤10 s has 7/67 unmatched; other nearby-connect cases have 0/39; cases without a connect line within 15 min have 3/9. Serial gaps and delayed logs limit these comparisons.

| Device | Forwarder report time UTC | Unmatched evidence / connection |
|---|---|---|
| Boron-Dev-11 | 2026-09-18T05:21:59.041952+00:00 | q=2; report 2.432 s before connect; pdiag received |
| Boron-Dev-11 | 2026-09-18T10:00:20.106194+00:00 | q=2; report 2.173 s before connect; pdiag received |
| Boron-Dev-09 | 2026-09-20T07:35:14.568083+00:00 | q=2; report 2.424 s before connect; pdiag received |
| Boron-Dev-11 | 2026-09-20T08:17:39.306281+00:00 | q=67 before report; prolonged connection outage, next recorded successful connect next day after a boot/uptime reset |
| Boron-Dev-09 | 2026-09-20T11:00:05.589006+00:00 | ConnSummary omitted by forwarder; Report→Connect transition, pdiag received about 4 s after report capture |
| Boron-Dev-09 | 2026-09-21T05:57:04.055544+00:00 | q=2; report 2.608 s before connect; pdiag received |
| Boron-Dev-09 | 2026-09-22T12:00:06.473657+00:00 | q=2; report 2.768 s before connect; pdiag received |
| Boron-Dev-09 | 2026-09-23T02:35:36.048024+00:00 | q=2; report 3.005 s before connect; pdiag received |
| Boron-Dev-09 | 2026-09-23T02:55:55.387112+00:00 | q=2; report 4.057 s before connect; pdiag received |
| Boron-Dev-09 | 2026-09-24T08:17:43.117784+00:00 | q=494 before report; DNS failures; q=497 on first recorded connect at 09:02; later queue drains |

The two long-backlog cases are not evidence of loss during the first few seconds of a fresh connection. A reset is a separate plausible cause in the Dev11 outage case. The Dev09 backlog had another captured report eventually delivered roughly six hours after generation; delayed delivery was therefore searched by payload time, not a short arrival window. Exact serial/S3 references and neighboring events are retained in `report-matches.json` and `unmatched-report-context.txt`.

### Matching and coverage

1. Read CloudFormation outputs, current-state inventory, and Product inventory through the telemetry CLI. Direct AWS CLI DynamoDB Query auto-pagination retrieved all **47,106** records over the exact interval for all 12 devices (no `--limit`/`--max-items`). Of those, **32,869** were forwarder records (this includes collector/path rows labeled serial); only **115** contain `Report: occ=...`. All 115 have `q=1`.
2. Listed all 15 intersecting UTC S3 `Ubidots-Sensor-Hook-v1` prefixes with automatic pagination. There are **6,002** objects inside the interval, including **10** under `deviceId=api`; those are excluded as manual/API publishes. The remaining **5,992** object keys exactly equal the 5,992 device-report S3 keys in DynamoDB: no missing keys on either side. All 6,002 bodies were fetched successfully, including the 10 excluded objects. No added device IDs appeared in S3.
3. Matched captured report to a unique device delivery using numeric occupancy, dailyoccupancy/totalMin, alerts, and embedded payload timestamp. The actual window is payload time minus capture time ∈ [−60 s, +2 s]; one-to-one nearest matching avoids reusing an event. All 105 selected matches have payload times **2.029–36.803 seconds before** capture. One initial symmetric-window candidate ambiguity was a later scheduled report with identical values 35 seconds after capture; the selected preceding event is unambiguous when requiring payload ≤ capture + 2 seconds. There are no duplicate selected event IDs/files. This is a correlation, not an application-provided unique report ID.
4. Searched deliveries throughout the complete 14-day period, so queuing delays do not cause false gaps simply because arrival is late. Five matches arrived >10 min after the log, including about 61 min and six hours. Future deliveries after the cutoff, old untrusted device time, same-valued hidden reports, reset loss, and archival coverage still prevent treating every unmatched observation as a proven device/session loss.
5. The current device-status ledger provides only a latest `reporting.lastReportEpoch`, not a historical count. Eleven of the twelve latest ledger report times have a delivery with payload time within 2 seconds; Dev14 has `lastReportEpoch=1790319603` (07:00:03 UTC) with none, corroborating the supplied 15:00 capture. Court1 differs by one second and is matched, not counted missing. The fleet snapshot reports current `deviceData` projection coverage 0/12; historical report-generation records are unavailable in the queried archive. `lastApplicationReportAt` is not used as an independent report denominator because the projection can advance on other event types.
6. Semantics and source boundaries: the figures count receipt at the AWS Particle webhook ingest/S3 archive, not separate confirmations from Ubidots or Particle integration history for all 14 days. Fleet integration-history matching is unavailable here. Missing S3 events alone do not identify where transport failed.

Machine-readable results: [per device](fleet-data/scope-by-device.csv), [per firmware](fleet-data/scope-by-firmware.csv), [per UTC day](fleet-data/scope-by-day.csv), and [every device × firmware × day](fleet-data/scope-by-device-firmware-day.csv). [Individual matches](fleet-data/report-matches.csv) preserve the audit trail.

The manual Dev-14 captures supplement, rather than replace, this sample. At least the **two accepted 15:00 reports** are independently shown absent at ingest/S3 and corroborated by the latest status Ledger's `lastReportEpoch=1790319603`. The supplied 14:41 report is another incident-level loss observation. They are absent from the forwarded Report-line cohort, so the cohort's zero unmatched Dev-14 reports is **not** evidence of zero loss. The flash/reset report and unnamed reset-wake event must remain separate uncertain identities/causes.

## 6. Why library logs are absent

The library creates `Logger _log("app.pubq")` (`PublishQueuePosixRK.cpp:11`). Normal enqueue, attempt, success/failure and removal messages are **TRACE**; app logging is globally INFO (`src/cloud/Particle_Functions.cpp:12,22–31`) without an `app.pubq` override. Their absence is expected and conveys no send/ACK outcome. Corrupt-file/capacity-discard messages are INFO, but an incomplete USB/forwarder capture can still miss them.

The narrow logging change is:

```cpp
{"app.pubq", LOG_LEVEL_TRACE},
{"app.seqfile", LOG_LEVEL_TRACE},
```

Both drafts additionally enable `comm.coap` TRACE and session-relevant `comm.dtls` / `comm.protocol.handshake` INFO. Device OS latches the CoAP debug setting at protocol begin; it logs outgoing CoAP ID/URI and incoming ACK IDs for correlation. Capture from boot/connection. Setting global logging to ALL would also expose these library messages but creates much more serial traffic.

## 7. Two unapplied bench drafts

| Draft | Version | Change |
|---|---|---|
| [BENCH-A-publish-diagnostics.diff](BENCH-A-publish-diagnostics.diff) | `v24-Pubq-Diag-A` | Trace queue/file operations and CoAP/session exchange; one worker result line per actual publish attempt, plus dispatch-rejection diagnostics; no intentional delivery-policy change |
| [BENCH-B-explicit-ack.diff](BENCH-B-explicit-ack.diff) | `v24-Pubq-Ack-B` | Same diagnostics; normalize every queued send to explicit WITH_ACK, clearing NO_ACK; preserve existing 2s startup/1s success pacing |

Both are standalone diffs against the **same current working tree**, not sequential patches. B's intervention is central at dispatch:

```cpp
const PublishFlags sendFlags = (curEvent->flags & ~PublishFlags(NO_ACK)) | WITH_ACK;
```

This also covers existing persisted PRIVATE-only queue entries, unlike changing only new application enqueue calls. Current callers never intentionally use NO_ACK. The normalization deliberately makes future NO_ACK callers ACK-gated in this bench build.

The `PubqAttempt` line records boot-local attempt ID, FNV-1a hash of event name + NUL + full data, event name, effective flags, Future result/error, ACK interpretation, connection observation age (`cObs`), and attempt duration. `ck=0` means no known connection observation. Cloud-status callbacks run asynchronously; cObs measures time since the application observed that callback, **not an exact hardware/network transition time**. At worker-start fallback it is an approximate observation. Existing report names fit the 160-character log limit, but long arbitrary names or existing payload TRACE lines may truncate; correlate the hash with raw S3 event data, not only a rendered/truncated log line.

**A cannot honestly emit a per-event cloud ACK verdict from the PRIVATE-only Future.** Its ACK field says `not-observed`; CoAP traces are the independent source for actual ACKs. Changing flags merely to obtain a different Future result would cease to be a diagnostics-only experiment. B's `ack=confirmed` means explicit ACK-gated Future success; an error is `unconfirmed`, with error code. A missing completion line after a reset/hang must be reconciled with the queue's preceding attempt trace, not counted as success.

**Recommendation: A to capture the mechanism, then B as the first candidate fix.** Explicit ACK fixes the proven removal-before-ACK gap without inventing a safe delay. Added delay alone does not protect later transmissions or retries. If A's traces demonstrate a residual startup issue after ACK supervision, test a separate **N=10s observation window** as an experimental arm, not a proven threshold, while also blocking application sleep during the pause. That delay is intentionally absent from B so the experiment isolates the ACK change.

Risks/limits: ACK waits can extend awake time, reduce drain throughput and lead to duplicates after lost ACKs. The existing cap, persistence I/O defects, gate timeout and unprotected in-flight RAM reset are not corrected by these drafts. Logging itself can perturb timing; the dropped serial-forwarder stream cannot be the bench oracle. Neither draft changes the publishing rate algorithm; the 1s/30s pacing remains. No build or on-device behavior has been verified.

## 8. Bench protocol and pass criteria — proposed only, not executed

1. Use an isolated bench device and a full direct USB capture starting before setup/connection; preserve monotonic uptime, wall time and flashed build hash/version. Establish the exact application and Device OS 6.4.1 binary provenance. Record existing settings and integration routes. Applying/building/flashing and any settings change are subsequent operator actions, outside this dispatch.
2. Run A first. Use the actual daily-close flow: device offline, valid/trusted clock, nonzero occupied total, close boundary due. Let `handleReportingState()` enqueue the pre-cleanup report and the post-cleanup zero report **before** connecting. Preserve a prior-cycle pdiag so the target roster matches `[pdiag, report(total>0), report(total=0)]`. Record both accepted Report lines and both enqueue/dispatch identities. Do not force time/settings from this investigation.
3. Permit connection and the normal existing 2s queue startup wait. Let the firmware follow its usual gate and sleep path. Record each event's queue admission, actual Particle.publish attempt, effective flags, cObs, Future outcome, CoAP message ID/ACK/retry and dequeue. Also record fresh-handshake versus resumed-session trace and disconnect time. Capture response topic/data if available so an unrelated webhook response cannot masquerade as a report confirmation.
4. Independently enumerate **all** corresponding Particle integration-history rows and S3 raw objects by device/event/full payload. HTML-unescape the stored payload once before hashing; retain its original JSON bytes/order for FNV comparison. Use the differing daily total to distinguish same-second reports. ACK and “queue drained” alone never pass the test. Check Ubidots' timestamped points as an additional endpoint check; two same-timestamp points may visually overlap/overwrite, so the plot is not a count oracle.
5. Repeat the identical sequence with B. Expected successful flags are PRIVATE|WITH_ACK=`0x09`; Future success must follow a matching cloud ACK. A deliberately unavailable ACK/cloud connection should produce a failed/timeout attempt with the event retained and retried after the existing backoff/next connection, not a successful dequeue. Review queue/file traces across sleep/reset; distinguish this from the known hard-reset/in-flight-RAM limitation.
6. Exercise both fresh and resumed sessions, immediate connection after offline queueing, a 2.7-minute offline delay before connection, and a roughly 5-minute queued interval. Repeat a report generated after connection as the later-slot control. Include the three-event close-boundary roster and the reset-wake pdiag/hibernate/status roster. A useful initial matrix is 20 cycles per condition per build; expand only if failures or incomplete observations demand it. Timing sweeps are experiments, not evidence that any N is intrinsically safe.
7. **Pass only if every logical queued test event has exactly one matching integration-history delivery and matching raw AWS record**, including both boundary reports and all diagnostics in the declared roster. No unmatched enqueue, unknown identity, missing completion, unexpected discard, overflow or unexplained duplicate is permitted in a passing run. Classify retries and duplicates separately: B aims for at-least-once reliability, so a duplicate can demonstrate recovery while still failing this strict one-for-one bench criterion. Continue capture through at least the 20s ACK timeout, 30s queue backoff and a subsequent reconnect when testing failure recovery; a not-yet-retried persistent event is pending, not proven permanently lost.
8. Interpret outcomes: A removes before ACK and disconnect cancels retries → confirms proposed mechanism. A/B show ACK before removal but integration missing → investigate cloud/integration routing with CoAP/event identity evidence. No actual publish attempt, or explicit file discard → investigate queue/persistence path. B fixes failures without changing startup delay → supports ACK-supervision defect over fixed-N theory. B still fails with ACK failure then retained events → distinguish pending backlog/retry policy from silent deletion.

## 9. Validation and retained artifacts

The draft generator preserves original sources and writes diffs only in this scratch directory. Both drafts passed read-only **`git apply --check` under `/bin/zsh`**, and an independent source review checked the 6.4.1 `PublishFlags`, Future error and system-event APIs. Neither patch was applied, and neither has been compiled or flashed; these are reviewable bench drafts, not validated firmware releases.

Read-only evidence includes the fixed-window fleet inventories, all paginated event records, daily S3 report indices, raw report objects, normalized matches and scope CSVs; Dev-14 S3 objects and three Lambda windows are retained separately. Full input USB excerpts remain in `DISPATCH-webhook-loss-investigation.md`. Raw inventory artifacts were sanitized to remove credentials and unnecessary subscriber identifiers. No token is reproduced in this report.

Final integrity check: **all 1,178 originally hashed source/library/test/document files remain byte-for-byte unchanged**, and both final diffs still pass `git apply --check`. Two new untracked work-order documents appeared in the repository while this investigation ran: `docs/work-orders/WO-2026-09-25-001-report-webhook-loss.md` and `docs/work-orders/WO-2026-09-25-001-stage4-codex-investigation-dispatch.md`. This investigation did not create them; they explain why overall Git status differs from the initial snapshot. See [repo-final-validation.json](repo-final-validation.json). The patch targets themselves are unchanged.
