# Stage 7 independent counters / telemetry / duplicate analysis

Model: gpt-6-astra; reasoning: ultra (inherited from parent). Reviewed uncommitted implementation against 599038e and binding WO Stage 5 decisions 1–5. Read Stage 4 report sections 1–3/7–9 and both Stage 6 dispatches/reports. No repository files edited, no AWS access, no network. Two scratch probes were compiled against the real counter module at parent's request; temporary executable removed below. Main suite/builds belong to parent agent.

## Findings

### [P2] HIBERNATE skips the new sleep snapshot and awake-time log (criteria 4, 6)

Changed lines: `src/state/State_Sleep.cpp:1224` (`noteQueuedAtSleep`) and `1234` (`CycleDelivery`).

Normal closed-hours Boron sleep takes `overnightFallbackSleep` at :1094 and, when the enabled RTC-hibernate policy succeeds, configures HIBERNATE at :1104. It calls `System.sleep(config)` at :1136. Its own :1129–1130 and :1138 explicitly document that successful HIBERNATE resets the MCU without returning. The new counter snapshot/log occur only afterwards, in the ULTRA_LOW_POWER path at :1203–1241. Therefore successful hibernate cycles never refresh `q` and never emit `CycleDelivery: awake=`. The next boot's `status` reports a stale q, and the requested close-cycle bench lacks its promised awake-time sample. Move/factor the accounting into both real sleep commit paths. This is introduced instrumentation placement, not a request to change sleep policy.

### [P2] Retained attempts are never reconciled after reset during an attempt (criterion 4)

Changed lines: `src/cloud/PublishDeliveryCounters.cpp:58–64`, :67–83; header promises at :38–40 and :52–58.

`noteAttempt()` increments retained attempted, but no retained field records an outstanding attempt. `begin()` preserves all valid counters and performs no reconciliation. A watchdog/pin/software reset between attempt dispatch and `noteResult()` loses the volatile Future and callback while retaining `a`. Once rebooted there is no in-flight attempt, yet a != k+f indefinitely; the real resent queued event also misses `r` because retryPending is set only by a failed callback. This is particularly relevant to telemetry advertised as surviving watchdog/pin resets.

Actual-module scratch probe (clang++ C++17, `-Dretained=`, `tests/stubs/Particle.h`, real `src/cloud/PublishDeliveryCounters.cpp`):

```
noteAttempt();                 -> a=1 k=0 f=0 r=0
begin(); // retained reboot     -> a=1 k=0 f=0 r=0
noteAttempt(); noteResult(true) -> a=2 k=1 f=0 r=0  (nothing in flight)
```

The invariant assertion exits 1. Boot bookkeeping needs a defined outstanding-attempt policy (e.g. retained pending/unknown failure accounting), or the claimed invariant and telemetry must represent unresolved outcomes explicitly. A boot-only status snapshot is intentionally allowed by decision 5; the issue is false accounting, not sampling frequency.

### [P2 / declared long-term limitation] Independent saturation also breaks a == k+f

`PublishDeliveryCounters.cpp:44–47` saturates every counter independently. Actual-module separate clean-process probe: one failed completed attempt followed by 65,535 successful completed attempts produces `a=65535 k=65535 f=1 r=1`, no attempt in flight. Thus a != k+f once a saturates and outcomes are mixed. The report acknowledges saturation but still states the invariant without qualifying it. This is deterministic and currently untested: tests/publish_delivery_counters_test.cpp checks the invariant at :96–104 before its saturation test :119–135, not afterwards. Treat counters at ceiling as censored values; agreement/loss-rate claims require a documented reset/epoch/saturation policy.

### Criterion 7: firmware preserves payload, but Copilot's fleet-ops idempotency argument is unsound

No backend changes reviewed or proposed. Read-only local inspection was used solely to check the supplied analysis.

Firmware-level retry facts are sound: `PublishQueuePosixRK.cpp:334–335` sends `curEvent->eventName/eventData` unchanged; failed file attempts retain their file at :396–399 and RAM attempts requeue/persist at :402–408; the worker makes a new `Particle.publish(event_name, event_data, event_flags)` at `BackgroundPublishRK.cpp:126`. The report timestamp is part of the stored data string (`src/Generalized-Core-Counter.cpp:2044,2060`), so it remains unchanged across reattempts. This provides at-least-once delivery, not a stable cloud publication envelope or downstream deduplication identity.

Copilot claimed fleet-ops uses the **device payload** timestamp as its ingestion key. The cited implementation actually uses top-level cloud envelope time:

- `/Users/chipmc/Documents/Maker/AWS/particle-fleet-operations/lambda/src/utils/parse.ts:38–42`: `return body.published_at || body.timestamp || new Date().toISOString();`
- `lambda/src/ingestion.ts:99` reads that `publishedAt`; :113 passes it to `generateS3Key`; :137–145 passes it as `indexEvent`'s eventTime.
- `lambda/src/utils/parse.ts:103–110` forms the raw S3 object key from `publishedAt`.
- `lambda/src/storage/dynamo.ts:51–54` keys the indexed item by deviceId/eventTime.
- `lambda/src/storage/event-history.ts:225` prefixes the history key with `ctx.publishedAt` as well.

The device payload is separately decoded from `body.data` at `ingestion.ts:101`; its inner `timestamp` is not used by those keys. The countermeasure therefore covers repeated delivery of one identical cloud envelope; it does **not** establish idempotency for a new firmware publish after a lost ACK. If the second publication has a different `published_at`, it creates different raw/index keys despite identical inner payload timestamps. The firmware has no ability in this call path to preserve cloud `published_at`.

Decision 5 permits bench resolution; do not claim criterion 7 backend behavior verified. Bench must induce an actual queue re-publish after loss of the ACK and compare cloud envelope times / raw keys, as well as Ubidots dot count and SUM aggregation. No live cloud assertion was made.

## Other counter limitations: separate inherited library behavior from new consumers

- New `q` is a replacement snapshot (`PublishDeliveryCounters.cpp:87–89`), not cumulative, fed from `State_Sleep.cpp:1205,1224`.
- **Inherited inaccurate depth**: `PublishQueuePosixRK.cpp:243–259` reports RAM count *instead of* summing RAM + file + current. More concretely, after an in-flight RAM failure, :403–408 persists/deletes curEvent but does not null it; during the 30 s retry backoff `getNumEvents():251–257` counts that stale pointer as another live RAM event. One actual pending file can report q=2. The old library bug was documented in Stage 4 and is not introduced by this diff, but new q and timeout event-count logs inherit it and cannot be certified exact inventories.
- **Retry attribution is global, not per event**: `retryPending` at counters.cpp:40 and :68–83 assumes the next dispatch is the same event. Copilot mentions only corrupt-file discard. Ordinary disconnect can also reorder: A in-flight from RAM; B pending in RAM; disconnect persists B via queue.cpp:425–428; A failure persists A afterwards via :403–408; next dispatch B is incorrectly called a retry, later A is not after B succeeds. Aggregate r may happen to balance in this two-event example, but identity attribution is not reliable. Retained reset case above creates a real missing retry count.

## Positive evidence

- Both status variants carry the full compact `"d":{"a":%u,"k":%u,"f":%u,"r":%u,"q":%u}`: `src/Generalized-Core-Counter.cpp:2269` and :2303. Snapshot at :2265; all five args at :2294–2298 and :2322–2326; event is queued at :2331.
- Counter initialization and both production hooks: `Generalized-Core-Counter.cpp:1139–1151`; accepted queue dispatch invokes attempt hook `PublishQueuePosixRK.cpp:342–343`; result hook is in main-thread queue statePublishWait at :367–368 before outcome branch. `PublishQueuePosix::loop():63–66` owns both state calls. The callback registration itself is consistent with application-thread-only counter writes; result processing cannot run before attempt hook on that thread.
- `git diff 599038e -- src/cloud/DeviceStatusPublisher.cpp` shows no emitted-field change at all: only explanatory comments and the accepted guard. No `d` object or counter-module reference exists in its production source. Guard at :370–376 rejects `writerBase.dataSize() >= sizeof(bufferBase)` before the old `bufferBase[dataSize()] = '\0'` at :378.
- Status payload test reads both real snprintf formats and bounds field widths. Reviewed estimates: PMIC profile 702 typical/modelled, 896 deployed worst-case, 965 structural; non-PMIC 535/709/742. These are modelled profiles, not captured device payloads. Buffer is 1024 bytes (1023 usable); queue enforces Device OS MAX_EVENT_DATA_LENGTH at PublishQueuePosixRK.cpp:106 (1024). The structural negative FLT_MAX case could be one byte wider than its positive-width estimate, still comfortably fits. No payload overflow found for the shipped fields.
- `src/cloud/Particle_Functions.cpp:12` selects SERIAL_LOG_LEVEL 3; :24–26 enable app.pubq/app.seqfile/comm.coap TRACE; :30,32 enable DTLS/handshake INFO. BackgroundPublishRK.cpp:143 prints the per-completed-attempt record. Trace is present, with existing log truncation/reset-interruption limitations described by Stage 4.
- On ULTRA_LOW_POWER paths that reach :1211, `WakeCycleStats::finalizeBeforeSleep` calculates awake time from current millis minus wake_start_ms (`src/observability/WakeCycleStats.cpp:86–87`), then CycleDelivery logs it at State_Sleep.cpp:1234–1240. This is a measurable wake-to-commit span, not a measured before/after power-cost result; hardware bench remains needed.
- `src/Version.cpp:6` is `v24-Pubq-Ack-B`.

## Probe command/results

```
clang++ -std=c++17 -Wall -Wextra -pedantic -Werror -Dretained= -Isrc -Itests/stubs \
  /private/tmp/codex-wo-2026-09-25-001-stage7-j_l3p071/counter-probes/counter_invariant_probe.cpp \
  src/cloud/PublishDeliveryCounters.cpp \
  -o /private/tmp/codex-wo-2026-09-25-001-stage7-j_l3p071/counter-probes/counter_invariant_probe_bin
```

Both reset and saturation scenarios intentionally failed the invariant assertion (exit 1). They are additional review probes, not members of the repository's test suite. No mutation was performed.

## Follow-up exact-source probes

At parent request, `queue_depth_exact_source_probe.cpp` was generated by extracting the **verbatim** bodies of production `writeQueueToFiles()`, `getNumEvents()`, and `statePublishWait()`. Only external Particle/SequentialFile dependencies were stubbed; production POSIX transfer wrote one real scratch queue record, which was removed. A completed failed RAM event yielded:

```
actual persisted files=1, RAM entries=0, current-pointer-still-set=1, getNumEvents=2, retry wait=30000 ms
invariant assertion exit=1
```

This reproduces the exact old code now feeding q: queue.cpp:403–408 deletes/persists the RAM event without clearing curEvent; :251–257 counts it again. New consumer locations are State_Sleep.cpp:1205 and :1224. No source mutation. Executable and record removed; extraction generator/source and result text remain as review evidence under `/private/tmp/codex-wo-2026-09-25-001-stage7-j_l3p071/counter-probes/`.

`distinct_envelope_keys.py` is an offline illustration of the inspected parse.ts logic. Two input cloud envelopes contain identical device/event/data (including inner timestamp 1790323200000) but published_at values `2026-09-25T08:00:00.000Z` and `2026-09-25T08:00:50.000Z`. They produce different DynamoDB eventTime keys and S3 object keys. Its result JSON explicitly states the conditional assumption: **this does not prove the live cloud assigns a new published_at; it proves that identical device payload timestamps do not establish ingestion idempotency if the cloud envelope timestamp changes.** See parse.ts:41–42,:103–110; ingestion.ts:99,:113,:137–145; dynamo.ts:51–54. Output is `distinct_envelope_keys_result.json` in that same scratch folder.
