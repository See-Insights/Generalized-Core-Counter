# Counter and telemetry review

Model: gpt-6-astra, reasoning ultra. All evidence from the uncommitted candidate against 599038e.

## Reset reproduction

Compiled real `src/cloud/PublishDeliveryCounters.cpp`, with only Particle's retained attribute defined away:

```sh
clang++ -std=c++17 -Wall -Wextra -pedantic -Dretained= -Itests/stubs -Isrc /private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/counters/reset-during-publish.cpp src/cloud/PublishDeliveryCounters.cpp -o /private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/counters/reset-during-publish-bin
```

The standalone probe starts in a fresh process and performs begin, attempt, begin (reset), repeated begin (idempotence), attempt, success. Output:

```text
before_reset a=1 k=0 f=0 r=0 b=0
after_boot a=1 k=0 f=0 r=0 b=1
after_retry a=2 k=1 f=0 r=1 b=1 invariant=PASS
```

Both the accounting invariant and retry attribution pass. Production implementation: `src/cloud/PublishDeliveryCounters.cpp:67` validates retained version2, :83–86 reconciles outstanding attempt, :90–96 marks attempts/retries, :99–108 records results; `src/Generalized-Core-Counter.cpp:1139` initializes before registering application-thread callbacks at :1142/:1147. The intended boundary is a reset during an outstanding publish; this does not prove atomicity against a reset between the individual writes inside these accounting functions. Independently saturating counters at65535 are expressly excluded from the invariant by the dispatch.

## Payload and telemetry source review

Both status format variants contain d.a/k/f/r/q/s/b at `src/Generalized-Core-Counter.cpp:2272` and :2308; argument lists are :2297–2303 and :2327–2333. No delivery fields are added to the device-status ledger. The ledger diff consists of explanatory comments and an overflow guard at `src/cloud/DeviceStatusPublisher.cpp:370`, before the NUL write at :378. The unchanged capacity/headroom concern belongs to WO-003, excluded from this review.

`src/state/State_Sleep.cpp:195` writes queued RAM records to flash at :204, increments s at :206, snapshots q at :214, and logs CycleDelivery at :224. Calls at :1238 and :1341 precede the HIBERNATE and ULP System.sleep calls, respectively. HIBERNATE awake time is computed by :236; ULP uses finalized cycleStats. This resolves the missing hibernate instrumentation. Measurability is not the measured before/after hardware awake-time increase required by AC6: the work order still describes an unrun post-flash bench, and no device actions are authorized here.

## Duplicate boundary and documentation

Failed queue sends preserve the payload for retransmission (`lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:395`–414); r counts retry attempts, not independently observed duplicate downstream records. PubqAttempt carries id/hash at `lib/BackgroundPublishRK/src/BackgroundPublishRK.cpp:143`. The binding decision8 at WO:116 expressly tracks consumer tolerance in WO-004 and removes downstream deployment as a blocker here; no WO-004 review or AWS access performed. Whether any Ubidots dailyoccupancy widget uses SUM is not established in the permitted evidence. Record as unknown/pending, not as safe or confirmed absent.

The prior GateFail q polarity issue is corrected at `docs/FIELD_MEANINGS_REFERENCE.md:72` (q=1 means blocking). The new claim at :57 that an in-flight attempt is never abandoned conflicts with the State_Sleep.cpp:698 fallthrough; that is part of the delivery-gate finding, not the old q issue.

Inherited limitations, already noted in round1: getNumEvents() is not an exact combined RAM+file count (`lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:243`); retryPending attributes the next global dispatch, not per-event identity. These were not changed in round3 and are kept separate from its new decision6 failures.
