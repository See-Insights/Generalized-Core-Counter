# Publish queue investigation and draft bench patches

Read-only source investigation of `/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter`, current working-tree files, on 2026-09-25. All generated files are in the scratch directory. No repository file, Device OS file, AWS resource, device, setting, or Git state was changed. This subtask inherited the requested GPT-6 Astra / ultra context; no independent runtime model/effort introspection is available to the subagent.

## Finding and confidence

**Proven code defect: a PRIVATE-only queue event can be permanently removed after local transport acceptance, without its Future waiting for a cloud acknowledgement.** The queue correctly waits for its BackgroundPublish Future, but PRIVATE does not request an ACK completion handler. The Boron UDP transport can still send a confirmable CoAP message and retry it independently. These two different meanings of acknowledgement must not be conflated. The actual loss mechanism for the reported five wake cycles remains unproven without per-attempt and CoAP traces. Explicit WITH_ACK is the narrow candidate fix for the proven reliability gap; it does not establish exactly-once integration delivery.

## Flags, completion, and timing (exact current source)

`src/Generalized-Core-Counter.cpp:2061`:

```cpp
bool queued = PublishQueuePosix::instance().publish(webhookName, data, PRIVATE);
```

The other direct queue sends also use PRIVATE: main application `status` at 2283, `watchdog` at 2323, `hibernate_wake` at 2358, and `src/power/PowerDiagnostics.cpp:439` for `pdiag`. The generic diagnostic wrapper passes its `flags` at main application line 2471; its callers use PRIVATE. Report `q=` is `queued ? 1 : 0` at main application 2129–2133; connection `q=` comes from `getNumEvents()`, e.g. `src/state/State_Connect.cpp:504`.

`lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h:174–183` exposes the option without imposing it:

```cpp
 * @param flags2 (optional) You can use NO_ACK or WITH_ACK if desired.
 ...
inline bool publish(const char *eventName, const char *data, PublishFlags flags1, PublishFlags flags2 = PublishFlags()) {
    return publishCommon(eventName, data, 60, flags1, flags2);
}
```

`PublishQueuePosixRK.cpp:71`, `114`, and `331–334`:

```cpp
PublishQueueEvent *event = newRamEvent(eventName, eventData, flags1 | flags2);
event->flags = flags;
...
if (BackgroundPublishRK::instance().publish(curEvent->eventName, curEvent->eventData, curEvent->flags,
    [this](bool succeeded, const char *eventName, const char *eventData, const void *context) {
        publishCompleteCallback(succeeded, eventName, eventData);
    })) {
```

`lib/BackgroundPublishRK/src/BackgroundPublishRK.cpp:78–98`:

```cpp
// kick off the publish
// WITH_ACK does not work as expected from a background thread
// use the Future<bool> object directly as its default wait
// (used by WITH_ACK) short-circuits when not called from the
// main application thread
auto ok = Particle.publish(event_name, event_data, event_flags);

// then wait for publish to complete
while(!ok.isDone() && state != BACKGROUND_PUBLISH_STOP)
{
    // yield to rest of system while we wait
    delay(1);
}

if(completed_cb)
{
    completed_cb(ok.isSucceeded(),
        event_name,
        event_data,
        event_context);
}
```

The background-thread comment describes why the code polls the Future; it does **not** mean WITH_ACK is disabled. The Future completion semantics are defined below.

Device OS 6.4.1 `communication/src/publisher.cpp:49–54`:

```cpp
bool confirmable = channel.is_unreliable();
if (flags & EventType::NO_ACK) {
    confirmable = false;
} else if (flags & EventType::WITH_ACK) {
    confirmable = true;
}
```

And `publisher.cpp:88–98`:

```cpp
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

Thus neither a blanket “PRIVATE means NO_ACK packets” nor “Boron defaults WITH_ACK therefore safe” is correct. PRIVATE's Future completes after `channel.send` succeeds, while explicit WITH_ACK connects completion to `add_ack_handler`.

`PublishQueuePosixRK.cpp:264–266`:

```cpp
publishComplete = true;
publishSuccess = succeeded;
```

The queue tests these fields later; it does not treat the boolean returned by `BackgroundPublishRK::publish` as successful delivery. That boolean only indicates worker admission. Its failure (`!thread`, worker busy, null name; BackgroundPublishRK.cpp:119–128) is ignored by the original queue, leaving `statePublishWait` waiting indefinitely; it does not directly delete the event.

Queue setup only sets the file cap to 800 (`src/Generalized-Core-Counter.cpp:1132–1134`); no application override changes the library's pacing. Defaults at `PublishQueuePosixRK.h:414–416`:

```cpp
unsigned long waitAfterConnect = 2000;
unsigned long waitBetweenPublish = 1000;
unsigned long waitAfterFailure = 30000;
```

`PublishQueuePosixRK.cpp:274–299`:

```cpp
void PublishQueuePosix::stateConnectWait() {
    canSleep = (pausePublishing || getNumEvents() == 0);
    if (Particle.connected()) {
        stateTime = millis();
        durationMs = waitAfterConnect;
        stateHandler = &PublishQueuePosix::stateWait;
    }
}
...
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

There is already a 2-second delay after the queue observes `Particle.connected()`, then a 1-second delay after each successful Future completion; application loop scheduling adds time. There is no additional readiness, Ledger, time-sync, DTLS-resume, or integration readiness check. `setPausePublishing()` exists but has no call in `src`. Pausing advertises `canSleep=true` even with queued events, so adding a pause without reviewing sleep gates is unsafe: `State_Sleep.cpp:481` uses `getCanSleep()` as queue readiness and `511` includes it in its all-complete decision. The Idle path sometimes also tests depth, but the Sleep gate does not require depth zero.

## Every queue removal / release path

All line references in this section are `lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp`, unless named otherwise. The library has **no retry count or retry limit**.

1. **Successful Future completion** (348–364): the only normal sent-event deletion. With current PRIVATE flags this does not require an ACK.

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

2. **Failure retry branch** (367–386): no disk event is removed. For a file-backed event only its temporary read buffer is freed. A failed RAM event is pushed to the front then moved to disk. The branch waits 30 seconds before trying again; indefinite retries are possible.

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

3. **Unreadable/corrupt-file discard** (301–308): a NULL result from `readQueueFile` is discarded without a send or ACK. NULL also covers allocation failure and file open/read trouble, not just proven corruption.

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

   `readQueueFile:167–190` checks minimum size and header magic/version/header/name lengths, allocates the remaining size, then checks the last byte and name length. The temporary allocation is freed on bad name/data (`184–185`); returning NULL causes the permanent deletion above. The strict name test `strlen(result->eventName) < (sizeof(PublishQueueEvent::eventName) - 1)` also rejects a maximum-length 64-character name in Device OS 6.4.1 (`communication/inc/protocol_defs.h:92` defines MAX_EVENT_NAME_LENGTH=64, so this buffer has 65 bytes). Current report name is shorter; the library's 63-character API comments are stale.

4. **800-file cap** (233–237), triggered on setup and enqueue / size adjustment, discards oldest file-list entry regardless of send/ACK status:

   ```cpp
   while(fileQueue.getQueueLen() > (int)fileQueueSize) {
       int fileNum = fileQueue.getFileFromQueue(true);
       if (fileNum) {
           fileQueue.removeFileNum(fileNum, false);
           _log.info("discarded event %d", fileNum);
       }
   }
   ```

   This can remove an in-flight file if more events fill the cap. An eventual failure cannot recover a file already discarded by this path. No evidence connects an 800-event backlog to the supplied wakes.

5. **Explicit clear** (198–207): discards RAM and all files. No call exists in application `src`.

   ```cpp
   void PublishQueuePosix::clearQueues() {
       WITH_LOCK(*this) {
           while(!ramQueue.empty()) {
               PublishQueueEvent *event = ramQueue.front();
               ramQueue.pop_front();
               delete event;
           }
           fileQueue.removeAll(true);
       }
   }
   ```

6. **RAM to disk transfer** (124–147): ordinarily a move rather than loss, but the current implementation removes and frees RAM despite unchecked persistence failures.

   ```cpp
   while(!ramQueue.empty()) {
       PublishQueueEvent *event = ramQueue.front();
       ramQueue.pop_front();
       int fileNum = fileQueue.reserveFile();
       int fd = open(fileQueue.getPathForFileNum(fileNum), O_RDWR | O_CREAT);
       if (fd) {
           ...
           write(fd, &hdr, sizeof(hdr));
           write(fd, event, sizeof(PublishQueueEvent) + strlen(event->eventData));
           close(fd);
           ...
       }
       fileQueue.addFileToQueue(fileNum);
       delete event;
   }
   ```

   `if (fd)` is incorrect for POSIX (`-1` is truthy; valid descriptor `0` is false); neither write checks byte count. A missing/partial file is subsequently discarded through path 3. These are independent plausible loss paths, unproven in this incident.

7. **RAM to current-send handoff** (312–314) pops the RAM queue but preserves the event in `curEvent`; it is not yet a permanent deletion:

   ```cpp
   if (!ramQueue.empty()) {
       curEvent = ramQueue.front();
       ramQueue.pop_front();
   }
   ```

   A hard reset/power loss can nevertheless lose the RAM event. The reset / disconnect callback only saves `ramQueue`; it does not save an in-flight RAM `curEvent`.

8. **Reset / disconnect handler**, not itself a removal (402–405):

   ```cpp
   if ((event == reset) || ((event == cloud_status) && (param == cloud_status_disconnecting))) {
       _log.trace("reset or disconnect event, save files to queue");
       PublishQueuePosix::instance().writeQueueToFiles();
   }
   ```

   Setup registers `System.on(reset | cloud_status, systemEventHandler)` at line 51. It does not register a sleep handler. Graceful reset normally tries to persist RAM; an offline enqueue already writes to disk (82–89). “A reset clears the RAM queue” is insufficient to prove the specific 06:15 report's loss without confirming RAM/in-flight state and reset kind.

`lib/SequentialFileRK/src/SequentialFileRK.cpp:103–115` implements logical file-list removal with `queue.pop_front()`. Physical deletion is `unlink(path)` at 173–175 for `removeFileNum(..., false)`. `removeAll()` unlinks regular files at 193–195 then `queue.clear()` at 201. No retry-limit removal exists in this dependency either.

## Ordering, counts, and reconstruction limits

Within an uninterrupted run, offline enqueues append numbered files and file-list retrieval is FIFO. Existing file-backed entries are selected before RAM (`PublishQueuePosixRK.cpp:301–314`). However `SequentialFileRK.cpp:48–75` scans directory entries with `readdir()` and directly `queue.push_back(fileNum)` without sorting. Therefore source alone does not guarantee chronological order of files found after a reboot.

`getNumEvents()` has important limitations (243–259):

```cpp
result = ramQueue.size();
if (result == 0) {
    result = fileQueue.getQueueLen();
    if (curEvent && curFileNum == 0) {
        result++;
    }
}
```

It omits an in-flight RAM event while other RAM events remain, and omits file count if RAM size is nonzero. Conversely failed-RAM handling pushes `curEvent` into RAM and writes/deletes it to disk without clearing `curEvent`; until a later read replaces that stale non-null pointer, this counter can add one to the disk count for the same event. A logged q value is not an exact roster of unique event identities.

The prior-cycle `pdiag` handling can constrain order better than arrival order alone:

- `State_Sleep.cpp:1483–1489` flushes the completed ULP/STOP cycle's diagnostic batch after wake, before normal post-wake/report activity.
- `State_Sleep.cpp:1118–1125` flushes `pdiag` immediately before hibernate; at that point the cloud/radio have already been disconnected. `PowerDiagnostics.cpp:439` queues it with PRIVATE and `PublishQueuePosixRK.cpp:82–89` moves offline events immediately to disk.
- Therefore the `pdiag` delivered in the 15:00 wake can be an older item queued ahead of the two 15:00 reports; an apparently later cloud arrival does not establish a later send. Transport retransmissions can further separate send order from receive order. Per-event send times cannot be recovered from the supplied INFO logs.

This precludes proving a deterministic “all events sent in the first N seconds are lost” rule from the available logs. In particular an older first-in-queue event may arrive while later report events do not. Also, a five-minute old event or one queued throughout DNS failures proves that offline age itself is not deterministically fatal; it does not measure milliseconds after the first successful connection.

## Logging and draft bench artifacts

`PublishQueuePosixRK.cpp:11` uses `static Logger _log("app.pubq")`. Normal enqueue, send, success, failure and file-write logs are TRACE (`75`, `80`, `85`, `143`, `329`, `350`, `358`, `370`). `src/cloud/Particle_Functions.cpp:12` sets SERIAL_LOG_LEVEL to 3, choosing INFO at 22; no app.pubq override exists. The absence of normal `[app.pubq]` lines is expected. Corrupt-file and overflow discards use INFO (306 and 237), and would pass this filter; missing forwarded lines still cannot prove they never happened.

Files generated, never applied:

- `BENCH-A-publish-diagnostics.diff`: **v24-Pubq-Diag-A**. Enables app.pubq and app.seqfile TRACE, comm.coap TRACE, comm.dtls and comm.protocol.handshake INFO. Adds a cloud-status timestamp observer and one `PubqAttempt` result line for each actual `Particle.publish` call in the worker. Reports event, flags, Future state, error code, ACK classification, connection age at the call, duration and event/payload hash. Worker admission failure gets `PubqDispatch accepted=0`; the pre-existing stall behavior is preserved.
- `BENCH-B-explicit-ack.diff`: **v24-Pubq-Ack-B**. Same diagnostics, plus one central dispatch normalization `(curEvent->flags & ~PublishFlags(NO_ACK)) | WITH_ACK`. It covers previously persisted PRIVATE-only events. NO_ACK is explicitly cleared because its transport branch takes precedence. Existing 2-second warmup and 1-second inter-event pacing remain.
- `BENCH-patches-manifest.json`: baseline hashes of all four touched files; `draft_queue_patches.py` generates both patches with Python difflib from the current source bytes.

`PubqAttempt` abbreviations: `id` = worker attempt counter within this boot; `e` = event; `f` = actual flags in hex; `future` = ok/failed/pending; `ack` = confirmed/unconfirmed/not-observed; `err` = Device OS Error enum number (0 if succeeded or still pending); `cObs` = elapsed ms from the cloud-status-connected **callback observation** to the actual publish call; `ck` = whether that origin was known; `dur` = duration through Future completion; `h` = 32-bit FNV-1a over event name, NUL, payload. Device OS delivers this callback asynchronously on the app thread, so callback latency makes `cObs` an underestimate of time since the system's connection transition. Startup while already connected also provides only an approximate origin. The hash is a correlation aid, not a collision-free ID; recompute it from the full S3 `event_data` payload. Device OS's default `LOG_MAX_STRING_LENGTH` is 160 (`services/inc/logging.h:235–236`), so existing queue payload TRACE lines can truncate. The structured line fits the actual report/status/pdiag names but longer future event names may truncate its tail; identity/hash are placed first. Do not assume the serial payload trace includes the full payload.

For PRIVATE-only attempts, `ack=not-observed` is deliberate: the callback cannot reveal an ACK it was never wired to await. `comm.coap` TRACE supplies transport message IDs and matching ACK traces separately. The Device OS investigator verified its runtime filter is effective when set from boot (`protocol.cpp:486–487` latches debug state at `Protocol::begin`; DTLS send/receive logs and `coap_util.cpp:213–214` print URI/type/code/ID). No Device OS source edit is part of these patches.

**Recommendation:** compare A against B first, isolating explicit ACK handling. A delay-only change lacks an evidenced N and could merely change timing. If A/B evidence later identifies a consistent session-readiness gap, add an independently tested holdoff with an explicit pending-queue sleep veto. Extra logs have timing/observer effects; neither draft is deployment approval, and neither has been built, flashed or behaviorally validated.

Verification performed: the generator ran under Python 3 invoked by **`/bin/zsh`**; both final diffs passed read-only `git apply --check` against the current working tree under **`/bin/zsh`**. No firmware compilation or execution test was performed. Source baseline hashes are included in the manifest. No patch was applied, including to a scratch source copy.

## Additional source/observed-runtime ambiguity: webhook gate

Current source arms `webhookExpectedOnConnect` for queued offline reports (`Generalized-Core-Counter.cpp:2094–2099`), starts awaiting response at connect (`State_Connect.cpp:546–549`), and makes sleep depend on `!session.awaitingWebhookResponse` (`State_Sleep.cpp:490–511`). No session-wide assignment/reset exists after the global initialization. The only explicit clears found are timeout (`Generalized-Core-Counter.cpp:1707–1710`) and a **nonempty response of any subscribed hook topic** (`2407–2411`). Setup subscribes to both the device-ID prefix (`955–958`) and broad `"hook-response/"` (`964`). Successful response logging is gated on verbose mode (`2438–2439`).

Thus the 15:00 disconnect roughly 4.8 seconds after connect, without a logged timeout or delivered report, needs independent resolution: a non-report integration response could have cleared this single uncorrelated boolean if it matched the subscription; alternatively the deployed binary/config may differ from inspected source. The configured webhook timeout cannot normally clear it before 5 seconds (`1701–1704`). Do not claim the report gate proves a per-report response or that every current source branch was present in the flashed binary. This also shows why the bench pass criterion must independently match **every payload** one for one against integration history, rather than relying on one webhook callback or “queue drained”.
