# WO-2026-09-25-001 — Stage 6 Implementation Report (Copilot)

Copied verbatim from the run output by Claude Code. Agent: Copilot CLI 1.0.88, `claude-opus-5`, reasoning high. Dispatch: `WO-2026-09-25-001-stage6-copilot-dispatch.md`.


## Files changed

**Modified (tracked):**
| File | What |
|---|---|
| `lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp` | `WITH_ACK` normalization on dispatch; two user-callback fire sites; `PubqDispatch:` rejection log |
| `lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h` | `withPublishAttemptUserCallback()` / `withPublishResultUserCallback()` + members |
| `lib/BackgroundPublishRK/src/BackgroundPublishRK.cpp` | `PubqAttempt:` per-attempt trace, FNV-1a payload hash, cloud-status observer *(from Codex's bench draft)* |
| `src/cloud/DeviceStatusPublisher.cpp` | `"d"` counter object; `dataSize()` overflow guard |
| `src/state/State_Sleep.cpp` | `noteQueuedAtSleep()`; `CycleDelivery:` line; `GateFail:` gains `elapsed=`/`qn=`; alert-43 message |
| `src/Generalized-Core-Counter.cpp` | `PublishDeliveryCounters::begin()` + hook registration |
| `src/cloud/Particle_Functions.cpp` | log levels: `app.pubq`/`app.seqfile`/`comm.coap` → TRACE; `comm.dtls`/`comm.protocol.handshake` → INFO *(bench draft)* |
| `src/Version.cpp` | `v24-Pubq-Ack-B` |
| `project.properties` | **removed** `dependencies.PublishQueuePosixRK=0.0.7` (see Deviations) |
| `docs/contracts/ledger-contracts.md` | documents the `d` object |
| `docs/FIELD_MEANINGS_REFERENCE.md` | documents `CycleDelivery:`, `GateFail:`, `PubqAttempt:`, `PubqDispatch:` |
| `tests/clock_status_republish_test.sh`, `tests/power_source_override_test.sh` | link the counters module, `-Dretained=` |
| `tests/stubs/Particle.h` (+2 override stubs) | added `Log.error()` |

**New:** `src/cloud/PublishDeliveryCounters.{h,cpp}`, `tests/publish_with_ack_queue_test.{cpp,sh}`, `tests/publish_delivery_counters_test.{cpp,sh}`, `tests/publish_ack_sleep_gate_structural_test.py`, `tests/device_status_payload_budget_test.py`.

`docs/work-orders/` untouched (read-only). fleet-ops untouched.

## Behavior changed

1. **Every queued publish now carries explicit `WITH_ACK`.** `PublishQueuePosixRK.cpp:334`:
   `const PublishFlags sendFlags = (curEvent->flags & ~PublishFlags(NO_ACK)) | WITH_ACK;`
   `NO_ACK` is stripped first because it takes precedence in Device OS. Previously Device OS 6.4.1 (`communication/src/publisher.cpp:88–98`) only registered an ACK handler when `WITH_ACK` was explicitly set, so `publishSuccess` meant "handed to transport" and the queue deleted the event immediately — the loss mechanism.
2. **Removal is now acknowledged removal.** `statePublishWait()` still removes only inside `if (publishSuccess)`, but `publishSuccess` now means a real cloud ACK. Failures requeue and retry after `waitAfterFailure` (30 s). Pacing constants unchanged.
3. **Retained delivery counters** are maintained and published.
4. **Observability:** one unconditional per-cycle `CycleDelivery:` line; richer `GateFail:`; per-attempt `PubqAttempt:` trace.

No change to queue pacing, sleep policy, report cadence, or the state machine.

## Counters: definitions, increment sites, survival

| Key | Name | Increments |
|---|---|---|
| `a` | attempted | each real `Particle.publish()` started for a queued event — `stateWait()` accepted-dispatch branch |
| `k` | acknowledged | future succeeded = cloud ACK — `statePublishWait()` |
| `f` | failed | future failed/timed out (20 s ACK timeout) — `statePublishWait()` |
| `r` | retried | an attempt following an earlier failure, via retained `retryPending` flag |
| `q` | queuedAtSleep | **snapshot** of `getNumEvents()` at the sleep commit — `State_Sleep.cpp` |

`a == k + f` except while an attempt is in flight. All counters are `uint16_t` and **saturate at 65535** (no wrap). Storage is a single `retained RetainedPublishDelivery` block (magic `0x50444301`, version 1); `arm-none-eabi-nm` confirms it at `0x2003f494`, inside `.backup` (`0x2003f400`).

**Survives:** hibernate/ULTRA_LOW_POWER wake, software/watchdog/pin reset.
**Resets to zero:** power loss, reflash, `kRetainedVersion` bump.

Both hooks are **new** and fire on the **application thread** (`stateWait()` / `statePublishWait()`), deliberately *not* the existing `publishCompleteUserCallback`, which runs on the background publish thread — this keeps the retained counters lock-free.

## Criterion 3 evidence (sleep/teardown gating) — no behavioral change needed

- `PublishQueuePosixRK.cpp:326` sets `canSleep = false` on dispatch; elsewhere it is derived from `getNumEvents() == 0` (`:275, :297, :350`).
- A failed publish requeues (file left in place, or RAM `push_front` + `writeQueueToFiles()`), so `getNumEvents() ≥ 1` and `canSleep` stays false.
- `State_Sleep.cpp:512` — `bool allComplete = queueEmpty && ledgersSynced && updatesChecked && webhookConfirmed;` where `queueEmpty = PublishQueuePosix::instance().getCanSleep()`. The gate returns without disconnecting while `elapsedMs < cloudSyncBudgetMs`.
- `State_Idle.cpp` gates `SLEEPING_STATE` on `canSleepGate` when connected, and the IDLE ceiling on `noMeaningfulWorkRemains` (includes `queueCanSleep`).
- `setPausePublishing()` is **not** used anywhere in `src/` (it would falsely report the queue sleep-safe); two tests assert this.

Offline sleep with queued events remains intended behavior — events are durable on disk and retried on the next connection. **Change made: logging only** (`GateFail:` now carries `elapsed=` and `qn=`; alert 43 names the unacknowledged-event count and the budget).

## Reading awake time per cycle

New unconditional INFO line at the sleep commit (`State_Sleep.cpp:1234`):

```
CycleDelivery: awake=41213ms a=412 k=409 f=3 r=3 q=0
```

`awake=` is wake → sleep-commit for **that** cycle. This is the reliable source: the pre-existing `CYCLE end awake=...` is gated on verbose mode, and `Sleep: td=...` only fires on cycles that actually tear down a connection. Healthy shape is `a == k`, `f=0 r=0 q=0`.

## Duplicate findings

**Verdict: duplicates are safe in the AWS path; the Ubidots path is undocumented but low-risk for this fleet. No firmware change made.**

**fleet-ops ingest Lambda** (read-only inspection, no changes):
- Entry `lambda/src/handler.ts:34` → `lambda/src/ingestion.ts:42`.
- Primary index `lambda/src/storage/dynamo.ts:76-80` — unconditional `PutCommand`, key `{deviceId, eventTime}` (`dynamo.ts:42-46`), with `eventTime` taken from the **device** payload (`utils/parse.ts:38-43`). Schema `infra-stack.ts:54-61`. → a duplicate **overwrites the same item**; no second row.
- Current state `storage/current-state.ts:56-64` — `UpdateCommand` keyed `{projectId, deviceId}`; one item regardless.
- Event history `storage/event-history.ts:67-72` with a deterministic sort key `${publishedAt}#${eventType}#${eventId}#${sha256(payload)}` (`:214-225`) → duplicates collide on the same key.
- S3 `utils/parse.ts:101-113` — same object key; bucket is `versioned: true` (`infra-stack.ts:38-45`), so a duplicate creates an extra **version**, not a new key.
- Counts (`resetCount`, `alertCount`, `occupancy`, `dailyOccupancy`) are parsed as **absolute values** (`utils/parse.ts:151-165`); no delta accumulation anywhere. No SQS/SNS/EventBridge in the ingest path. **No explicit dedup logic exists**, but deterministic keying makes ingest effectively idempotent.

**Ubidots** (public docs; not present in fleet-ops — Ubidots is a separate Particle webhook integration):
- Whether two dots with the same `(variable, timestamp, value)` are stored twice, overwritten, or rejected is **undocumented**. No uniqueness constraint, dot ID, update-by-ID endpoint, or idempotency/`force` flag is documented.
- **Documented:** out-of-order/late timestamps are accepted, not rejected (bulk-datalogger example) — so a retried publish will be ingested normally.
- **Inference:** a duplicate is harmless for an **absolute** value (last-value reads are unaffected) but would **double-count under SUM aggregation** for per-period counts.

**Proposed fix (not implemented, backend-side):** if strict idempotency is ever required, reuse the deterministic payload hash already computed in `event-history.ts` as a conditional `PutItem` (`attribute_not_exists`) guard before side effects. For Ubidots, prefer absolute/cumulative variables over SUM-aggregated per-period counts.

## Tests added and mutations caught

| Test | Interpreter | Catches |
|---|---|---|
| `publish_with_ack_queue_test.{cpp,sh}` | zsh | (i) `WITH_ACK` dropped; (ii) removal not gated on `publishSuccess`; (ii-b) `publishSuccess = true` hard-wired; also asserts no `setPausePublishing(` in `src/`, and that `stateWait()`'s only `removeFileNum` is the corrupted-file discard |
| `publish_delivery_counters_test.{cpp,sh}` | zsh | (iv) counter accounting broken; (v) counters dropped from the payload. Compiles the **real** `PublishDeliveryCounters.cpp` (`-Dretained=`); checks `a == k + f`, saturation at 65535, `begin()` idempotence, and every wiring site |
| `publish_ack_sleep_gate_structural_test.py` | python3 | (iii) `queueEmpty` dropped from `allComplete`; also `GateFail` tokens, alert-43 text, IDLE `canSleepGate`, `canSleep = false;` on dispatch |
| `device_status_payload_budget_test.py` | python3 | renders real JSON from the writer calls and fails if the counters stop fitting the 896-byte cap |

Mutation smoke test (temporary script, injected → confirmed failure → reverted): **6/6 mutations caught**, working tree restored clean.

## Commands run and results

| Command | Result |
|---|---|
| Host suite: all `tests/*.sh` via **zsh**, all bare `tests/*.py` via **python3** | **47/47 (sh via zsh, py via python3)** — baseline 43/43 + 4 new scripts |
| Mutation smoke test | **6/6 caught** |
| Local ARM boron build (README command, repo root) | **text 152628 / data 1110 / bss 2468** (reference 150576 / 1090 / 2444 → +2052 / +20 / +24) |
| `particle compile boron . --target 6.4.1` | **Flash 153830 / RAM 3570** (reference 151754 / 3530 → +2076 / +40). `.bin` deleted. |

**Payload size vs. the 896-byte cap** (usable 895, one byte reserved for the NUL):

| Measurement | Bytes |
|---|---|
| Observed baseline before the counters | 839 |
| Delivery object, worst case | 56 |
| **Observed baseline + counters** | **895 (exactly at the cap, 0 B headroom)** |
| Deployed worst case, existing fields only | 903 |
| Deployed worst case, with counters | 959 |
| Structural worst case (type maxima) | 1044 |

## Known limitations

- `retried` is attributed by a retained `retryPending` flag, not per-event. If a corrupted-file discard intervenes between a failure and the next dispatch, the retry can be attributed to a different event.
- `q` is a snapshot, not a cumulative total — a device that repeatedly sleeps with one queued event shows `q=1`, not a running count.
- Counters saturate at 65535 and silently stay there; long-lived devices will need the value read as "≥65535".
- A duplicate still creates an extra S3 object **version** in fleet-ops (storage cost only).
- Ubidots duplicate-dot behavior is genuinely undocumented; the low-risk conclusion is an inference, not a vendor guarantee.
- `WITH_ACK` costs one extra round trip per publish and holds the modem up longer; awake time per cycle may rise. `CycleDelivery: awake=` was added specifically so this is measurable in the field.

## Deviations from the WO

1. **`project.properties`: removed `dependencies.PublishQueuePosixRK=0.0.7`.** **This was required for the WO to work at all.** `particle compile` replaces `lib/PublishQueuePosixRK` with the unpatched registry 0.0.7 sources, so the first cloud compile failed with *"`class PublishQueuePosix` has no member named `withPublishAttemptUserCallback`"* — proving the cloud build never saw the vendored library. Without this change the `WITH_ACK` fix would be present in local ARM builds and **absent from every cloud-built binary**. I verified the vendored `PublishQueuePosixRK.{cpp,h}` and `BackgroundPublishRK.{cpp,h}` at `HEAD` are **byte-identical** to the registry 0.0.7 / 0.0.2 sources, so dropping the declaration changes nothing except which copy is compiled. Precedent: `BackgroundPublishRK`, `SequentialFileRK`, and `DeviceInfoLedger` are already vendored-only and appear on the cloud include path.
2. **Spec correction — added an overflow guard in `DeviceStatusPublisher.cpp`.** `JSONBufferWriter::dataSize()` "can be greater than the buffer size" (6.4.1 `spark_wiring_json.h:232`), so the pre-existing `bufferBase[writerBase.dataSize()] = '\0'` was an **unguarded stack overwrite**. It now logs `LedgerPayloadStatus: overflow ...` at error level and returns false. Pre-existing bug, but the counters move the payload close enough to the cap that leaving it was not defensible.
3. **Two new library callback hooks** rather than reusing `publishCompleteUserCallback` — see the threading rationale above.
4. **Payload budget finding (reported, not fixed).** `DEVICE_STATUS_PAYLOAD_CAPACITY = 896` (`Cloud.h:397`) is a **self-imposed app-RAM constant for duplicate suppression, not a Particle Ledger limit**. Observed 839 + 56 = **895 — zero headroom**, and the **existing fields' own deployed worst case (903 B) already exceeds the cap before the counters**. Raising the constant is the clean fix; it is out of scope for this WO, and the new guard now contains the failure mode (loud error + no publish) instead of corrupting the stack. **Decision for Chip.**
5. **Test-stub edits:** added `Log.error()` to three stubs and linked `PublishDeliveryCounters.cpp` into two existing host builds — mechanical, required to keep the suite green.
6. **No firmware change for criterion 7**; the duplicate investigation concluded no firmware-side mitigation is warranted.

## Model and reasoning level

Claude Opus 5 (`claude-opus-5`), high reasoning effort.

---

**State:** uncommitted working-tree diff on `wo/2026-09-25-001-publish-with-ack`; no commit, push, merge, or release. All temporary artifacts (`build-tmp/`, the compiled `.bin`, the registry-comparison checkout) removed.

