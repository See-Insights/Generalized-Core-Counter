_Copied verbatim from the run output by Claude Code. Agent: Copilot CLI 1.0.88, `claude-opus-5`, reasoning high._

# WO-2026-09-25-001 — Stage 6 round 2 Implementation Report (Copilot)

Dispatch: `WO-2026-09-25-001-stage6-round2-copilot-dispatch.md`. Decision 5 only; round 1 otherwise untouched.

## Files changed (this round)

| File | What |
|---|---|
| `src/cloud/DeviceStatusPublisher.cpp` | **Removed** the `"d"` object and the `PublishDeliveryCounters.h` include. Overflow guard kept. |
| `src/Generalized-Core-Counter.cpp` | `"d":{"a","k","f","r","q"}` added to **both** `status`-event `snprintf()` variants (PMIC and non-PMIC), fed by one `PublishDeliveryCounters::snapshot()`; `publishStartupStatus()` `@note` re-measured |
| `src/cloud/PublishDeliveryCounters.h` | Header doc now says `status` event, with decision 5's reason |
| `docs/contracts/ledger-contracts.md` | `d` rows and JSON example reverted; replaced by an explicit "not in this payload" note |
| `docs/FIELD_MEANINGS_REFERENCE.md` | New section "Publish Delivery Counters in the `status` Event"; `CycleDelivery:` cross-reference corrected; `LedgerPayloadStatus: overflow` documented |
| `tests/device_status_payload_budget_test.py` | Now asserts the counters are **absent** and the payload's worst case is **exactly** the recorded 903 B |
| `tests/publish_delivery_counters_test.sh` | Criterion-4 checks moved to the event; new `refute()` checks on the ledger |
| **new** `tests/status_event_payload_budget_test.py` | `status`-event byte budget + counters-present |

`docs/work-orders/` read-only. No other changes.

## Behavior changed

The device-status ledger payload's emitted JSON is byte-for-byte the pre-WO format — the only residue in that writer is comments; the guard's behavior differs only in the overflow case it was added for. The five counters now ride in the `status` event.

**Flagged:** `publishStartupStatus()` is called once per `setup()` (line 1408), so the counters reach the cloud once per boot, not once per report. On hibernating devices that is every wake; on `INTERMITTENT` devices it is per reset. Retained storage means each event reports the accounting carried over from prior cycles. This is what the dispatch specifies; noted because it changes the bench's sampling rate versus round 1.

## Size check (stop condition — not triggered)

**Limits.** App buffer `char status[1024]` → 1023 usable. Device OS 6.4.1 `particle::protocol::MAX_EVENT_DATA_LENGTH = 1024` (`communication/inc/protocol_defs.h:94`). PublishQueuePosixRK's *enforced* ceiling is **that same constant** — `PublishQueuePosixRK.cpp:106` rejects `strlen(eventData) > MAX_EVENT_DATA_LENGTH`. The "622" in `PublishQueuePosixRK.h:210/230/254` and in `src/power/PowerDiagnostics.cpp:28` is stale Device OS 0.8.0-era prose, enforced nowhere. **Binding limit: 1023 B.**

| Shipped (PMIC-forensics) variant | Bytes | Headroom |
|---|---:|---:|
| Observed (typical cycle) | 702 | 321 |
| **Deployed worst case** | **896** | **127** |
| Structural worst case | 965 | 58 |

Delivery object worst case: **56 B**. `ENABLE_PMIC_FORENSICS=0` variant: 535 / 709 / 742. Structural 965 assumes `%.2f` of `FLT_MAX` for `lastPmicAnomalySoc` (42 chars); real values are ≤ 6.

*Caveat:* "observed" is a **modelled** field-width profile — no full `status` payload capture exists in this repository. The deployed worst case is the binding number and is measured from the shipped format string.

**Ledger payload:** deployed worst case **903 B**, identical to round 1's recorded figure — proof the format did not move. `"d"` absent (asserted three ways).

## Tests and the mutations they catch

| Test | Interpreter | Catches |
|---|---|---|
| `status_event_payload_budget_test.py` | python3 | counters missing from either event variant; counters hard-coded instead of read from the module; counters present in the ledger; an unbudgeted field; event over the 1023 B limit; library ceiling no longer `MAX_EVENT_DATA_LENGTH` |
| `device_status_payload_budget_test.py` | python3 | any `d`/`a`/`k`/`f`/`r`/`q` key or `PublishDeliveryCounters` reference back in the ledger; ledger worst case ≠ 903 B; overflow guard removed |
| `publish_delivery_counters_test.sh` | zsh | all of round 1, plus: object missing from either variant, any counter arg missing from either `snprintf()`, snapshot not read, and `refute()` on every ledger re-insertion form |

**Mutation smoke test — 6/6 caught**, working tree SHA-256-verified restored:
M1 counters back in ledger (all 3 tests) · M2 overflow guard removed (ledger budget) · M3 counters dropped from event (2) · M4 dropped from one variant only (wiring) · M5 counters hard-coded (2) · M6 unbudgeted field (status budget).

## Commands and results

| Command | Result |
|---|---|
| All `tests/*.sh` via **zsh**, all bare `tests/*.py` via **python3** | **48/48 (sh via zsh, py via python3)** — round 1 was 47/47; +1 new file |
| Mutation smoke test | 6/6 caught |
| Local ARM boron build (README command) | **text 152580 / data 1110 / bss 2468** — round 1: 152628 / 1110 / 2468 → **−48 / 0 / 0** |
| `particle compile boron . --target 6.4.1` | **Flash 153782 / RAM 3570** — round 1: 153830 / 3570 → **−48 / 0**. `.bin` deleted. |
| `arm-none-eabi-nm -S` (criterion 10) | `retainedPublishDelivery` at `0x2003f494`, 0x14 bytes — unchanged from round 1 |
| `strings` on the cloud `.bin` (criterion 9) | `"d":{"a":%u,"k":%u,"f":%u,"r":%u,"q":%u}` ×1, `PubqAttempt:`, `CycleDelivery:`, `LedgerPayloadStatus: overflow` all present |

Both builds shrank by the same 48 bytes — the `snprintf` format costs less code than five `JSONBufferWriter` call pairs.

## Deviations

1. **Overflow-guard comment edited.** Dispatch says keep it "exactly as it is". The guard's **code is byte-identical**; one sentence of its justification comment still claimed the counters were being added above it, which is now false. Corrected to cite the existing fields' 903 B worst case instead.
2. **A new test file** (`tests/status_event_payload_budget_test.py`) rather than folding the event budget into the ledger test — two different payloads, two different limits. Suite count 47 → 48.
3. **Stale `~622-byte` comment in `src/power/PowerDiagnostics.cpp:28` left in place.** The dispatch asked me to confirm the real figure (1024), not to fix that comment, and said "no other changes". Reported, not changed.
4. **Reported, not fixed:** the `status` event is boot-only (see "Behavior changed"). If per-cycle counter telemetry is wanted, a second publish site is needed — out of scope here.

## Model and reasoning level

Claude Opus 5 (`claude-opus-5`), high reasoning effort.

**State:** uncommitted working-tree diff on `wo/2026-09-25-001-publish-with-ack`. No commit, push, merge, or release. `build-tmp/` and `tests/__pycache__/` removed; the downloaded `.bin` deleted (`proof_prod_boron.bin` is pre-existing and untouched).

