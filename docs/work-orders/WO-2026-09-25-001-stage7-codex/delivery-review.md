# Stage 7 delivery, queue, and sleep independent review

Model/reasoning inherited from dispatcher: gpt-6-astra / ultra. Read-only source review; no repository writes, network, or firmware builds. A scratch C++ reproduction compiled the exact existing State_Idle.cpp power-management and ceiling blocks against controlled host stubs.

## Findings

### P1 — Ordinary low-power IDLE never reaches the bounded ACK wait

The ACK change introduces potentially repeated asynchronous failures into a flow that waits for queue drainage before even entering the state containing the bounded gate. `src/state/State_Connect.cpp:651–659` sends ordinary connect success to IDLE (`transitionTo(IDLE_STATE, "connect-complete");`); only `session.returnToSleepAfterReport` selects direct SLEEPING. `src/state/State_Report.cpp:299` also sends already-connected reports to IDLE.

In `src/state/State_Idle.cpp:233–238`:

```cpp
bool canSleepGate = true;
if (Particle.connected()) {
  canSleepGate = PublishQueuePosix::instance().getCanSleep();
}
if (!updatesPending && canSleepGate) {
```

The transition is inside that condition at line 260. The safety ceiling has the same exclusion at lines 278–280,303–310:

```cpp
queueCanSleep = PublishQueuePosix::instance().getCanSleep();
const bool noMeaningfulWorkRemains = !updatesPending && queueCanSleep;
const bool shouldApplyIdleCeiling =
  connectivityPowered &&
  noMeaningfulWorkRemains &&
  !healthyConnectedAwakePath;
if (!shouldApplyIdleCeiling) {
  idleCeilingStartMs = 0;
```

Therefore with Particle still connected and an unacknowledged event queued, neither route enters SLEEPING_STATE. Its timer at `State_Sleep.cpp:478–479` never starts. `ThrashGuard.cpp:71–74,92–95` explicitly disables IDLE supervision. Scheduled reports cycle back to IDLE and do not provide a bounded deadline. Failures retry forever after `waitAfterFailure` (queue lines 390–413), holding the radio awake with no `GateFail` for this backlog. The gate lines are preexisting, but the new ACK requirement makes this an explicit bounded-wait acceptance gap, not an unrelated cleanup request.

Scratch reproduction: `/private/tmp/codex-wo-2026-09-25-001-stage7-j_l3p071/delivery-probes/idle-bounded-wait-reproduction.cpp`, exact source tail extracted from the Power Management marker through the end of `handleIdleState()`. Scenario INTERMITTENT, connected, queueCanSleep=false, one pending event, no OTA/sensor/LED work, 300-second idle ceiling. Simulated 3,600,000 ms gives sleep transitions=0 and disconnects=0. Executable deliberately returns failure for the bounded-wait expectation. Text output retained beside the source. Not a whole-firmware simulation; it executes the actual relevant gate/ceiling expressions under controlled conditions.

Recommendation: ensure low-power IDLE can enter a state that starts/enforces the cloud-sync deadline while preserving the actual sleep/disconnect prohibition until expiry. Do not simply remove the queue condition from the disconnect ceiling. Add an end-to-end reachability/timeout regression. Current structural sleep test explicitly requires both problematic blockers and does not verify reachability.

### P2 — Successful Boron overnight hibernate skips q and CycleDelivery

`src/state/State_Sleep.cpp:1093–1104` selects the actual Boron overnight RTC-alarm hibernate path. It calls `System.sleep(config)` at line 1136. Lines 1138–1142 explain that a successful hibernate resets the MCU and only failure returns. The new sleep-commit queue snapshot and awake logging are later at lines 1205,1211–1224,1234–1240:

```cpp
const uint16_t qDepth = (uint16_t)PublishQueuePosix::instance().getNumEvents();
PublishDeliveryCounters::noteQueuedAtSleep(qDepth);
Log.info("CycleDelivery: awake=%lums a=%u k=%u f=%u r=%u q=%u", ...);
```

Consequently successful overnight hibernate neither updates retained `q` nor emits `CycleDelivery: awake=`. Next boot's `status` reports the previous sleep's q. This is a direct criterion 4 / 6 defect in newly placed code; the bench explicitly includes a close. The comment claiming one unconditional INFO line per wake cycle is false for this successful sleep mode. Capture q/finalize/log on both actual sleep-commit paths, preferably by a shared helper before the nonreturning hibernate call.

## ACK evidence (criteria 1 and 2)

`lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:301–304` reads a persisted file into `curEvent`; lines 312–314 select a RAM event. Both converge at lines 334–337:

```cpp
const PublishFlags sendFlags = (curEvent->flags & ~PublishFlags(NO_ACK)) | WITH_ACK;
if (BackgroundPublishRK::instance().publish(curEvent->eventName, curEvent->eventData, sendFlags,
    [this](bool succeeded, const char *eventName, const char *eventData, const void *context) {
        publishCompleteCallback(succeeded, eventName, eventData);
```

Thus old stored flags need no rewrite; both storage sources normalize on every actual dispatch, clearing 0x02 and setting 0x08 (PRIVATE remains 0x01).

`lib/BackgroundPublishRK/src/BackgroundPublishRK.cpp:126–133,148–153`:

```cpp
auto ok = Particle.publish(event_name, event_data, event_flags);
while(!ok.isDone() && state != BACKGROUND_PUBLISH_STOP) {
    delay(1);
}
// ...
completed_cb(ok.isSucceeded(), event_name, event_data, event_context);
```

Queue lines 264–266 record the Future callback result:

```cpp
publishComplete = true;
publishSuccess = succeeded;
```

Queue lines 359–360 return before completion. Lines 371–380 gate real file deletion on success:

```cpp
if (publishSuccess) {
    // ...
    fileQueue.getFileFromQueue(true);
    fileQueue.removeFileNum(fileNum, false);
```

RAM success deletion is at 386–387. Failure lines 394–408 select `waitAfterFailure`, leave a file event's file in place, or `ramQueue.push_front(curEvent);` then `writeQueueToFiles();`. There is no deliberate Future-failure dequeue in the candidate source.

Installed Device OS 6.4.1 `/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/src/publisher.cpp:49–54`:

```cpp
if (flags & EventType::NO_ACK) {
    confirmable = false;
} else if (flags & EventType::WITH_ACK) {
    confirmable = true;
}
```

Lines 88–98:

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

Line 31 uses `SEND_EVENT_ACK_TIMEOUT`, defined as 20000 ms at `communication/inc/protocol_defs.h:99`. `communication/src/protocol.cpp:260–274` completes ACK handlers with success on successful CoAP response and error on other response classes. This closes the proven PRIVATE-only remove-after-local-send mechanism. It proves Particle protocol acknowledgment, not webhook execution/delivery to Ubidots.

Normal valid-event success/failure handling passes criteria 1–2. Preexisting capacity eviction/corruption/persistence issues remain outside the newly changed normal ACK path and should not be silently interpreted as a universal no-loss guarantee.

## Sleep gate, timeout, and RAM persistence (criterion 3)

Queue `canSleep` is false while dispatching (`PublishQueuePosixRK.cpp:326`), remains false while publish-wait is unfinished, and is recomputed from `(getNumEvents() == 0)` at 275/297. No application `setPausePublishing()` caller was found. `getNumEvents()` lines 243–261 include an isolated current RAM event via 251–257; counts are not a reliable inventory with mixed RAM/file/current items (known Stage 4 issue).

Once SLEEPING_STATE is reached, `State_Sleep.cpp:482,512` reads queue sleep safety and requires `queueEmpty && ledgersSynced && updatesChecked && webhookConfirmed`. Lines 571–590 return without teardown inside budget. The queue-aware budget is 30 s minimum and 120 s cap (`:148–167`, `ConnectivityPolicy.h:133`), potentially expanded to output-ledger budget (`:500–502`). On expiry, lines 596–604 log `GateFail: reason=%s timeout=%lu elapsed=%lu q=%d qn=%u ledger=%d webhook=%d update=%d`; lines 618–622 log count/elapsed/budget and raise alert 43. Lines 637–640 reset timer bookkeeping without queue deletion, then lines 772–790 initiate the existing cloud-only/full disconnect, returning until teardown finishes. Under successful teardown it proceeds to sleep. The timeout deliberately permits a nonempty queue, and has no queue-clear operation.

`PublishQueuePosixRK.cpp:425–429`:

```cpp
if ((event == reset) || ((event == cloud_status) && (param == cloud_status_disconnecting))) {
    _log.trace("reset or disconnect event, save files to queue");
    PublishQueuePosix::instance().writeQueueToFiles();
}
```

That flush loops over `ramQueue` only (`:124–147`). Files already queued remain queued. A RAM event with a completed failure is pushed back and persisted at 404–408.

Important preexisting qualification: an in-flight RAM event was removed from `ramQueue` into `curEvent` at 312–314; `writeQueueToFiles()` does not save it. Device OS disconnect emits disconnecting (`system_task.cpp:730`) and TERMINATE (`:751`); DTLS TERMINATE calls reset (`dtls_protocol.h:103–105`), which clears ACK handlers (`protocol.cpp:575`), aborting their Futures (`services/inc/completion_handler.h:299–303`). The worker then invokes failure; a subsequent application queue loop requeues/persists the current RAM event. But `Generalized-Core-Counter.cpp:1640–1708` executes the sleep handler BEFORE the queue loop, so there is no explicit barrier guaranteeing that application completion processing occurs before a nonreturning hibernate. This was already noted in Stage 4. Do not claim that disconnect persistence itself covers every in-flight RAM event. I have not hardware-reproduced a loss, and do not elevate the possible interleaving to a newly introduced confirmed regression.

## Tracing, awake data, and version

Criterion 5: `src/cloud/Particle_Functions.cpp:24–32` enables `app.pubq` and `app.seqfile` TRACE, `comm.coap` TRACE, `comm.dtls` INFO, handshake INFO. `BackgroundPublishRK.cpp:119–146` emits one `PubqAttempt` result line per worker call, including ID, FNV hash of name/NUL/payload, event, effective flags, Future state, ACK interpretation, error, connection-age-at-attempt and duration. Queue dispatch rejection has a separate `PubqDispatch` error line at 349–350. Clock/connected callback caveat is correctly labeled by cObs/ck.

Criterion 6: actual `CycleDelivery` logging is readable at sleep line 1234 and uses total_awake_ms finalized at 1211; all-cycle coverage fails for overnight hibernate as above. Actual awake-time increase versus a pre-B device is bench evidence, not established by code inspection.

Criterion 8: `src/Version.cpp:6`: `const char* FIRMWARE_VERSION = "v24-Pubq-Ack-B";`.

## Review limits and disposition for parent

I recommend NOT VERIFIED for the requested bounded-wait behavior and the missed hibernate q/awake capture. Normal ACK normalization/Future dequeue proof and trace/version requirements are sound. No hardware or backend conclusion is implied. Root reviewer owns mutations, suite, builds, hashes, linker map, binary verification, and final overall disposition.
