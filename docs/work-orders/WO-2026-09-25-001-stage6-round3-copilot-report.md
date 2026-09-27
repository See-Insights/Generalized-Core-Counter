_Copied verbatim from the run output by Claude Code. Agent: Copilot CLI 1.0.88, `claude-opus-5`, reasoning high._

## Implementation Report — WO-2026-09-25-001 Stage 6 round 3

### Files changed
**New source:** `src/cloud/PublishDeliveryBudget.{h,cpp}` (pure decision module, host-compilable), `src/cloud/PublishDeliveryGate.{h,cpp}` (device adapter + single expiry log site).
**Modified source:** `src/power/ConnectivityPolicy.h`, `lib/PublishQueuePosixRK/src/PublishQueuePosixRK.{h,cpp}`, `src/state/State_Sleep.cpp`, `src/state/State_Idle.cpp`, `src/cloud/PublishDeliveryCounters.{h,cpp}`, `src/Generalized-Core-Counter.cpp`.
**Docs:** `docs/FIELD_MEANINGS_REFERENCE.md`.
**Tests:** new `tests/publish_delivery_budget_test.{cpp,sh}`; updated `publish_with_ack_queue_test.sh`, `publish_ack_sleep_gate_structural_test.py`, `publish_delivery_counters_test.{cpp,sh}`, `status_event_payload_budget_test.py`.

### Behavior changed
- **P1 — bounded wait (decision 6).** All three gate sites (SLEEPING cloud-sync gate, IDLE low-power sleep entry, IDLE connectivity ceiling) now take one verdict from `PublishDeliveryGate::queuePermitsSleep()`. Sleep is permitted when the queue is sleep-safe **or** the device is offline **or** the budget expired with no publish in flight. A permanently stuck queue can no longer pin the device awake.
- **In-flight hold.** After `cloudSyncBudgetMs`, if a publish is outstanding the gate holds (feeding `thrashGuard`/watchdog) up to 25 s before teardown, so a CoAP retransmit store is never reset mid-attempt.
- **P2 — hibernate telemetry.** `commitDeliveryAccountingBeforeSleep()` is now called before `System.sleep()` on **both** the HIBERNATE and ULTRA_LOW_POWER paths (previously hibernate never returned, so `CycleDelivery` and `noteQueuedAtSleep()` were skipped). ULP log content unchanged.
- **P2 — counter balance.** `begin()` reconciles an attempt interrupted by a reset into `abandoned` and sets `retryPending`.
- **P3 — docs.** `GateFail q` polarity corrected (q=1 = queue blocking).

### Budget: start condition, value, configuration
Starts at the first evaluation where the device is **cloud-connected AND the queue is not sleep-safe**; cancelled (restarts from zero) when the queue drains or the connection drops, so offline time never consumes it. Value `ConnectivityPolicy::PUBLISH_DELIVERY_BUDGET_MS = 90000UL`; hold cap `PUBLISH_IN_FLIGHT_HOLD_MAX_MS = 25000UL` (above Device OS's 20 s `SEND_EVENT_ACK_TIMEOUT`). **Configured as a compile-time named constant** in the existing policy location — no existing config path could carry it without a persisted layout change, which the dispatch forbids. Expiry logs once per episode: `DeliveryBudget: expired budget=… elapsed=… qn=… inflight=…`.

### In-flight accessor
`bool PublishQueuePosix::getPublishInFlight() const`. Set `true` in `stateWait()`'s accepted-dispatch branch; cleared at the end of `statePublishWait()` **after** the result has been acted on (removed, or requeued and persisted) — not merely when the Future completes.

### Counters (retained, version 1→2, 20→24 bytes)
`a` attempted, `k` acknowledged, `f` failed, `r` retried, `q` queuedAtSleep (snapshot), **`s` sleptWithQueued** (cumulative sleeps that carried events over), **`b` abandoned** (attempts whose outcome was lost to a reset). Invariant **`a == k + f + abandoned`** outside an in-flight attempt and below saturation; at 65535 each counter saturates independently and the invariant is censored (asserted and documented). Status `d` object is now 7 keys; nothing added to the ledger payload.

### Tests and the mutations they catch
| Test | Catches |
|---|---|
| `publish_with_ack_queue_test.sh` | (i) drop WITH_ACK, (ii) remove before ack, **(iv) remove on failure** — new awk rejects `removeFileNum(`/`getFileFromQueue(`/`curFileNum = 0;` in the failure branch |
| `publish_delivery_budget_test.{cpp,sh}` | (iii); 7 host cases on the real budget module + structural checks on all 3 gate sites, the in-flight hold, expiry logging, flush-before-sleep |
| `publish_ack_sleep_gate_structural_test.py` | (iii); new asserts on the shared gate, budget cancel/offline rules, `sleepPermitted && !inFlight` |
| `publish_delivery_counters_test.{cpp,sh}` | reset-during-attempt → `abandoned`, idempotent reconciliation, retry-after-abandon, `sleptWithQueued`, both sleep-commit paths |
| `status_event_payload_budget_test.py` | new `s`/`b` keys vs the 1023-byte limit |

### Commands and results
- **Host suite: 49/49** (sh via **zsh**, py via **python3**) — round-2 baseline 48/48, +1 new test.
- **Mutations (i)–(iv): all caught.** i→`publish_with_ack_queue_test.sh`; ii→same; iv→same; iii→`publish_delivery_budget_test.sh` + `publish_ack_sleep_gate_structural_test.py`. Tree restored byte-identically (MD5 verified).
- **Local ARM build** (README command, boron/6.4.1): **text 153516 / data 1114 / bss 2484** (round 2: 152580 / 1110 / 2468 → +936 / +4 / +16).
- **Cloud compile** `particle compile boron . --target 6.4.1`: **Flash 154718 / RAM 3586** (round 2: 153782 / 3570). Downloaded `.bin` deleted.
- **Status event worst case:** observed 714, deployed 916, structural 985 — all under 1023 usable / 1024 limit. Delivery object worst case 76 B.
- **Retained block (`arm-none-eabi-nm -S`):** `2003f494 00000018 d retainedPublishDelivery` — 24 bytes (was `2003f450 00000014`). Retained region ends at `0x2003f4b0` = 176 B of the 3072 B user backup RAM.

### Deviations
1. **Mutation (iii) patch adapted.** Its context line `bool allComplete = queueEmpty && …` was renamed to `queuePermitsSleep` by this round, so `patch` no longer applies. The identical mutation (drop the queue conjunct) was applied by `sed` and is caught by two tests.
2. `kRetainedVersion` 1→2 means existing devices re-zero these counters on first boot of this build (intended; no persisted struct layout was touched).

### Model
`claude-opus-5`, reasoning level **high**.

