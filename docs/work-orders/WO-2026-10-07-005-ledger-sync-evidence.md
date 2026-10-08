# WO-2026-10-07-005 Stage 2 evidence: can the sleep gate wait for an inbound ledger sync that has started?

- **Date:** 2026-10-07
- **Base:** committed merge `400ff48b093ba046853b889bb4c815769245209a` (PR #72), parents `73f752e` and `79e3c5f`. Application citations are `git show 400ff48:<path>` with line numbers, marked `@400ff48`.
- **Model:** `claude-sonnet-5-5`, confirmed by the caller's probe. Effort `high` is set by the launch CLI (`--effort high`). The probe reported numeric 10, so it did not confirm the word "high".
- **Goal:** find out whether the sleep gate can wait for an incoming ledger sync that has already started, using what Device OS and the gate already provide.
- **Paths:** application `State_*` files are under `src/state/`, other application files under `src/`, all at the committed base; unrelated working-tree edits are excluded. Device OS root: `/Users/chipmc/.particle/toolchains/deviceOS/6.4.1`; bare `ledger*.cpp/h` paths mean `system/src/ledger/`, bare `spark_wiring_ledger.cpp` means `wiring/src/` unless otherwise qualified.

**STOP.** Device OS 6.4.1 gives the application no signal that an inbound (cloud-to-device) sync is pending or in progress. The stop rule applies, so Q2 and Q5 are not reached as full answers. Q1, Q3 and Q4 are answered. Incidental facts under Q2 and Q5 are labelled as such.

## Q1. What the application can read in Device OS 6.4.1

- **`Ledger` class** (`wiring/inc/spark_wiring_ledger.h`):
  - Readable: `lastUpdated()` :142, `lastSynced()` :149, `dataSize()`, `name()`, `scope()`, `isWritable()` :177. Callbacks: `onSync()` :200 and :213.
  - There is no pending or in-progress accessor. `lastSynced()` returns `ledger_info.last_synced` (`wiring/src/spark_wiring_ledger.cpp:325-331`).
- **C API, not wrapped by the class:**
  - `ledger_get_info()` (`system/inc/system_ledger.h:236`) fills `ledger_info.flags` (:140). Its one flag is `LEDGER_INFO_SYNC_PENDING` (:90), documented as "changes that have not yet been synchronized with the Cloud".
  - It is exported at dynalib index 8 (`system/inc/system_dynalib_ledger.h:40`); `ledger_get_instance` is index 0 (:32).
  - The flag is set from `srcInfo.syncPending()` (`system/src/system_ledger.cpp:80-94`). The `Ledger` class never reads it; its only `ledger_get_info` call site is `wiring/src/spark_wiring_ledger.cpp:233`.
  - An application could reach it by calling `ledger_get_instance` and `ledger_get_info` itself.
- **What the flag means:**
  - The internal field is "local changes not yet synchronized" (`system/src/ledger/ledger.h:159`). It is set only on a user write (`ledger.cpp:863-864`, `LedgerWriteSource::USER`).
  - It is cleared after a successful send (`ledger_manager.cpp:744-746`) and after a completed receive (:1308).
  - For an inbound ledger the system writer updates only `lastUpdated` (`ledger_manager.cpp:822`), so the flag stays false while data arrives. It is set true for inbound ledgers only in rare scope-change cases (:974, :995).
- **Pending versus in progress inside the OS:**
  - Pending is `PendingState::SYNC_FROM_CLOUD` (`ledger_manager.h:90`), set at `ledger_manager.cpp:647` and :878 and cleared when the request is sent (:1122). In progress is `State::SYNC_FROM_CLOUD` (`ledger_manager.h:83`, set at `ledger_manager.cpp:1126`).
  - Both are private to `LedgerManager`. `system/inc` has no accessor for them and no ledger system event: only `system_ledger*.h` and `system_error.h` mention ledgers. The `firmware_update_*` codes the v31 handler uses are at `system/inc/system_event.h:77-80`.
- **`onSync` fires on completion only** (`ledger_manager.cpp:754, 1318` → `ledger.cpp:415-420`), reaching the application thread via `application_thread_invoke` (`spark_wiring_ledger.cpp:194-201`). There is no "started" notification.
- **`lastSynced` is not a pending signal.** It is persisted: loaded from flash at init (`ledger.cpp:561`) and written on sync (`ledger_manager.cpp:1307`). A device that has synced once reads non-zero before any new sync begins.
- **`lastUpdated()` for an inbound ledger** holds the version time of data already received (`ledger_manager.cpp:822`). It says nothing about a newer cloud version the device hasn't learned of.

## Q2. Timing of learning a newer cloud version (NOT REACHED as a full trace)

Incidental facts from the Q1 trace:
- The cloud-connected flag is set and the CoAP channel opened in the same step (`system/src/system_task.cpp:374-380`), triggering `notifyConnected` → `startSync` (`ledger_manager.cpp:553-559, 1375-1399`).
- For inbound ledgers, `startSync` only queues a SUBSCRIBE (:1386-1388). Work then runs one task at a time in the order GET_INFO :497, SUBSCRIBE :502, SYNC_TO_CLOUD :508, SYNC_FROM_CLOUD :533, so outbound sends are due before inbound fetches.
- The device learns of a newer version from the subscribe response (:877) or a cloud push (`receiveNotifyUpdateRequest`, :615-655, compared at :644-647). A push can arrive at any time while connected.
- INF: when `Particle.connected()` first reads true, no ledger request has necessarily been sent yet, so "nothing pending" cannot be trusted at that point.

Not traced: cloud-side notification latency, timer scheduling of `run()`, and a trustworthy "no sync pending" point. The OS exposes no signal that would mark one.

## Q3. What the gate waits for (baseline for Q4)

All @400ff48:
- **Pre-gate diversion:** `src/state/State_Sleep.cpp:485-489` sends the state to FIRMWARE_UPDATE while `firmwareUpdateInProgress` is set and no teardown has been requested.
- **Conditions** (`State_Sleep.cpp:498-528`):
  - Queue: `getCanSleep()`.
  - Ledgers: `areLedgersSynced() && !hasPendingOutputLedgerSync()`.
  - Updates: `!System.updatesPending()`.
  - Webhook: `!session.awaitingWebhookResponse`.
- **Budget:**
  - Base 30 s (`ConnectivityPolicy.h:136`), raised queue-aware to at most 120 s (`State_Sleep.cpp:150-170`).
  - Raised to 70 s (cellular) or 30 s only when the *output* ledger is pending (:516-519; `ConnectivityPolicy.h:151-155`). Inbound ledgers never extend it.
- **On timeout:** `GateFail`, alert 44 for ledgers (`State_Sleep.cpp:629-634`), then teardown proceeds.
- **Release label:** `GateRelease reason=` is the last blocker before release (:549-551, :669-673). `ledger` merges inbound and output (:505, :533), so the label cannot say which one.
- **Inbound ledger test** (`LedgerClient.cpp`):
  - True at once when both `lastSynced()` values are > 0 (:138-152). Otherwise a 10 s cellular window applies (`ConnectivityPolicy.h:170`).
  - After the window, neither synced counts as empty and returns true (:213-226); a partial sync returns false (:196-212).
  - This is the same `lastSynced`-alone test Q1 rules out as a pending signal.
- **Outbound test:** `lastUpdated > lastSynced` (`LedgerClient.cpp:15-23, 88-95`), plus the app flags `pendingDeviceStatusSync` and `pendingDeviceDataSync`.
- **Connect path:** `src/state/State_Connect.cpp:557` calls `loadConfigurationFromCloud`, which reads the ledgers once with no wait (`Cloud.cpp:634-645`). New data is applied later from `Cloud::loop` after `onSync` (`Cloud.cpp:678-690`).
- **v31 OTA-aware dwell wiring:**
  - `System.on(firmware_update, firmwareUpdateHandler)` at `Generalized-Core-Counter.cpp:902`; the handler records state at :2519-2524 into the globals at :145-146.
  - FIRMWARE_UPDATE_STATE treats begin/progress events as forward progress (`State_Connect.cpp:725-727`). It leaves when no update is in progress (:732-736) and on a no-progress cap of 5 min (:748-753; `ConnectivityPolicy.h:98`).
  - That pattern depends on an OS event. Q1 found no ledger equivalent.

## Q4. History (§12.1)

Searches, all `git log 400ff48 --format='%h %ad %s' --date=short` over `src lib` unless noted:
- **`-S` on gate terms** (`areLedgersSynced`, `ledgersSynced`, `LEDGER_SYNC_TIMEOUT_MS`, `hasPendingOutputLedgerSync`, `OUTPUT_LEDGER_SYNC_TIMEOUT_MS`, `LedgerInputWindow`, `Particle.ledger`, `lastUpdated()`, `firmwareUpdateInProgress`) hit b6dd353, c3a8a1d, b4491c5, 8f889ae, eda6b7e, 4c2b734, 737ceb3, c254467, a95e284, 589421d, 3d2b5d6 and 37437a3.
- **`-S"isSyncing"`** returned nothing.
- **`-S"syncPending"`** returned only two Clock commits (b517cbf, 293f4f7). I judged them unrelated from the subject lines and did not open the diffs.
- **`-G` on `src/state` and the main file** (`ledgersSynced|areLedgersSynced|ledgerSync`) returned a95e284, c254467, 737ceb3, b4491c5, eda6b7e, b6dd353. `-G"areLedgersSynced"` on the same paths returned c254467, b4491c5, b6dd353.
- **`git grep`** at `8f889ae` and `b6dd353~1` for `ledgersSynced|waitFor|loadConfigurationFromCloud`.

- **Before b6dd353 (2026-02-03):** the sleep path had no ledger wait. Only a callback-set `ledgersSynced` flag existed (`Cloud.cpp:62, 70` at `b6dd353~1`).
- **b6dd353:** added the gate. It tests `lastSynced() > 0` plus a 5 s window, and its message says it replaced the callback flag.
- **4c2b734 (v8, 2026-04-23):** made the window per-connection and added the early return when both ledgers are synced (the Alert 44 fix).
- **c254467 (2026-05-22):** added the output-ledger gate (`hasPendingOutputLedgerSync`, `OUTPUT_LEDGER_SYNC_TIMEOUT_MS`). Its rationale in `ConnectivityPolicy.h:139-155` is outbound delivery, not an inbound wait.
- **Result:** no commit shows the gate or connect path ever waiting on an inbound sync that had started, and none shows such a wait being removed. No removed inbound wait was found in these searches.
- **Limits:** `-S` and `-G` can miss renamed code. I did not open the c254467 or 589421d diffs beyond these hits.

## Q5. Does `LedgerDuplicateStillInflight` share the cause? (NOT REACHED)

No logs or payloads were reviewed. Incidental code facts:
- **When it fires** (`Cloud.cpp:281, 311-312` @400ff48): when a tracker record exists for the pointer, `lastUpdated > lastSynced` (:37-45), and age is over 30 s. It covers only `deviceStatusLedger` and `deviceDataLedger` (:275-279), so it is outbound only.
- **Tracker cleanup:** a record is removed only by `onSync` completion (`Cloud.cpp:391`, via `LedgerClient.cpp:70-86`) or a local `set()` failure (`Cloud.cpp:452`; `DeviceStatusPublisher.cpp:626`). A transport failure does not clear it.
- **Source of `-1001`:** it is `SYSTEM_ERROR_COAP_CONNECTION_CLOSED` (`services/inc/system_error.h:57`).
  - Closing the channel fails in-flight requests (`communication/src/v2/coap_channel.cpp:911-914`).
  - `requestErrorCallback` logs "Request failed" (`ledger_manager.cpp:1570-1575`). `handleError` logs "Synchronization failed… retrying in %us" (:1438-1446), with a 30 s minimum (:78, :1441).
  - These match the two bench messages.
- **Retry after teardown:** the persisted outbound flag re-queues the send at the next connect (`ledger_manager.cpp:1380-1383`).
- **INF:** a `-1001` on an outbound request would leave an app tracker entry and an unsynced ledger, which fits this warning on a later write. Nothing in the bench OBS says which ledger was in flight, so this is not established.
- **Same cause?** Both involve a transfer in flight when the connection closes, but in different directions. Only the outbound direction has a gate predicate (`State_Sleep.cpp:500, 516-519`). Whether one teardown produced both was not examined.

## OBS, code facts and INF

- **OBS** (Chip's bench, not reproduced): three cold releases (webhook 2.7 s, ledger 7.5 s, queue 3.3 s) followed by `-1001` within 20 ms. One warm KEEP_ALIVE succeeded with the input callback about 2.9 s after Connect, inside a 9.1 s gate wait.
- **INF:**
  - On the two releases whose last blocker was webhook or queue, the ledger predicate was already true. An inbound fetch could have been in flight with the gate open. This is consistent with the OBS but unproven.
  - Whether `lastSynced` was already non-zero on Dev-14 at those connects is not in the OBS.

## Open points and limits

- No documentation was checked on whether the C API is supported for app use. Particle's cloud-side notification behavior is not in the source tree.
- Sleep-mode RAM retention, which would affect tracker carry-over, was not examined.
- Q2 and Q5 are not reached. No design or fix is offered, per the instructions.

## Budget versus actual

| Item | Budget | Actual |
|---|---|---|
| `src/` lines | 0 | 0 |
| `lib/` lines | 0 | 0 |
| Tests | none run | none run (no interpreter used) |
| Report length | ≤120 lines | 118 lines |

Every application range was re-opened with `git show 400ff48:<path> | cat -n`: `State_Sleep.cpp`, `LedgerClient.cpp`, `Cloud.cpp`, `State_Connect.cpp`, `Generalized-Core-Counter.cpp` and `ConnectivityPolicy.h`. Every Device OS range was re-read with line numbers under `/Users/chipmc/.particle/toolchains/deviceOS/6.4.1`. The re-checks found no line-number discrepancies.
