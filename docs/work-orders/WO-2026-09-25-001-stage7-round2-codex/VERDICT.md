**NOT VERIFIED.** The original five findings are resolved, but three integration defects remain. [Full review and evidence](/private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/REVIEW.md).

- **P1 — Teardown can proceed with a publish in flight.** After the 25-second hold, [State_Sleep.cpp:698](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:698) explicitly proceeds despite the outstanding attempt. A delayed-completion reproduction reached this boundary with the current RAM event absent from disk. This is fault-injection evidence, not observed hardware loss.
- **P2 — `sleptWithQueued` undercounts repeated ULP sleeps.** The latch is reset only on state entry. SLEEPING→SLEEPING cycles bypass that reset: two queued sleep commits produced **s=1 instead of 2**. [Evidence](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:531).
- **P2 — An expired delivery budget can survive into the next connection.** The disconnected sleep branch resets only the older timers. The next connection can immediately permit sleep without a fresh delivery window. [Evidence](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:803).

The baseline suite passes **49/49: 25/25 shell tests via zsh, 24/24 Python tests via python3**, with no skips.

| Mutation | Passing tests | Detection |
|---|---:|---|
| (i) Drop WITH_ACK | 48/49 | Queue test fails |
| (ii) Remove before ACK | 48/49 | Queue test fails |
| (iii) Remove `queuePermitsSleep` conjunct | 47/49 | Budget and sleep-gate tests fail |
| (iv) Remove on Future failure/timeout | 48/49 | Queue test fails |

All mutations ran on byte-identical candidate snapshots outside the repository. Every mutation was restored and hash-checked. [Patches, assertions, and results](/private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/tests/test-review.md).

The required ordinary failure reproduction now passes:

| Reproduction | Result |
|---|---|
| Every publish fails after 20 seconds | Safe modeled sleep boundary at **90.001 s** |
| Every publish fails after 1 ms | Safe boundary at **90.001 s** |
| Failure attempt crosses budget expiry | Held until completion; safe boundary at **92.002 s** |
| RAM events at either sleep commit | Written to real host queue files; payload verified |
| Attempt → reset/`begin()` → retry → success | **a=2, k=1, f=0, abandoned=1, r=1** |

The ordinary failure cases retain the event, never release the gate in flight, increment `sleptWithQueued`, and log expiry. These are host gate/commit timings with instantaneous modeled modem teardown; physical sleep latency was not measured. [Reproduction details](/private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/delivery/delivery-review.md).

| Criterion | Result | Evidence |
|---|---|---|
| 1. Explicit ACK, including persisted events | PASS | [Queue:334](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:334); both binaries |
| 2. ACK-success removal; failure retention | PASS for queue outcome handling | [Queue:376](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:376); failure reproduction and mutation iv |
| 3. Bounded wait and safe teardown | **FAIL** | Ordinary wedge fixed; [in-flight timeout bypass remains](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:698) |
| 4. Status counters; unchanged ledger schema | PASS | [Status:2272](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:2272), second variant at 2308 |
| 5. B tracing | PASS | [Attempt trace:143](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/lib/BackgroundPublishRK/src/BackgroundPublishRK.cpp:143); [logging filters:24](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/cloud/Particle_Functions.cpp:24) |
| 6. Awake-time increase measured | **FAIL—measurement pending** | [Both-path instrumentation exists](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:224); hardware comparison remains unperformed |
| 7. Revised duplicate criterion | **FAIL—SUM-widget record missing** | [Retries counted](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/cloud/PublishDeliveryCounters.cpp:90); consumer tolerance tracked in WO-004 and explicitly nonblocking. Ubidots SUM usage remains unknown |
| 8. Required version | PASS | [Version.cpp:6](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Version.cpp:6) |
| 9. Exact binary verification | PASS | [Disassembly and call-chain evidence](/private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/binaries/binary-review.md) |
| 10. Retained placement | PASS | Map quotation below; version 2 |
| 11. Budget, in-flight exclusion, persistence, counter, expiry | **FAIL** | Three integration defects above |
| 12. Hibernate telemetry, reset invariant, mutation iv, q documentation | PASS | [Hibernate commit:1238](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:1238); [reset reconciliation:83](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/cloud/PublishDeliveryCounters.cpp:83); [corrected q:72](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/docs/FIELD_MEANINGS_REFERENCE.md:72) |

| Round-1 finding | Disposition |
|---|---|
| P1: never-recovering ACK failures wedge IDLE | Resolved for the original scenario |
| P2: HIBERNATE skips telemetry | Resolved: commit at 1238 precedes sleep at 1254; ULP commit at 1341 precedes sleep at 1511 |
| P2: reset-unbalanced counters | Resolved; invariant and retry attribution reproduced |
| P2: mutation iv survives | Resolved; now caught |
| P3: `GateFail q` documentation | Resolved |

Both fresh builds succeeded. Each exact binary contains the WITH_ACK dispatch, registered hooks, in-flight accessor, and delivery-gate code.

| Candidate | Build sizes | File bytes |
|---|---|---:|
| [Local ARM binary](/private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/binaries/boron-local-6.4.1.bin) | text/data/bss: **153508 / 1114 / 2484** | 154626 |
| [Particle cloud binary](/private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/binaries/boron-cloud-6.4.1.bin) | Flash/RAM: **154718 / 3578** | 154722 |

Local SHA-256:
```text
93a3c7641cd344db04f1c4dca4653b72222e94646208dc702e2d0f7047d3127a
```

Cloud SHA-256:
```text
b216044914dc497bdef57bcb4b7e7c13e39d233fb1c5a5d2849354f3134efc4a
```

Local text is eight bytes below the reference, fully explained by linker padding. Cloud RAM is eight bytes below the reported reference; its historical cause could not be established without the earlier cloud binary.

The [local map](/private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/binaries/boron-local-6.4.1.map:168522) records:

```text
.backup         0x000000002003f400       0xb0 load address 0x00000000000d9b24
 *(.retained_user*)
 .retained_user
                0x000000002003f450       0x18 ../../../build/target/user/platform-13-m/local-source-FbwtgJ//libuser.a(PublishDeliveryCounters.o)
```

Matching `nm`:

```text
2003f450 00000018 d retainedPublishDelivery
```

Thus the retained block is **24 bytes in backup SRAM**, with **version 2 verified in both binaries**.

The `build-tmp/` hygiene defect comes from **three** scripts: [queue:14](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/publish_with_ack_queue_test.sh:14), [counters:17](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/publish_delivery_counters_test.sh:17), and [budget:17](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/publish_delivery_budget_test.sh:17). Their cleanup removes only the binary. Alphabetical suite execution first creates the directory in the budget test.

**Repository integrity: 2,326/2,326 files byte-identical**, directories, Git status, and diff unchanged; `build-tmp/` absent. Temporary build/test artifacts were removed. Requested binaries and review evidence remain outside the repository.

Model used: **gpt-6-astra**, reasoning **ultra**.