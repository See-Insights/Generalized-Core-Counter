**NOT VERIFIED.** The ACK fix is present in both binaries, but the bounded-wait behavior, counter accounting, and mutation coverage have blockers.

[Full review and evidence](/private/tmp/codex-wo-2026-09-25-001-stage7-j_l3p071/REVIEW.md) · [Binary verification](/private/tmp/codex-wo-2026-09-25-001-stage7-j_l3p071/binary-review.md)

The main findings are:

- **P1 — Pending events can prevent the bounded wait from starting.** [State_Idle.cpp:233](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Idle.cpp:233) requires queue drainage before entering SLEEPING_STATE; its fallback ceiling also excludes queued work. An exact-source reproduction remained connected for **one simulated hour**, with zero sleep transitions or disconnects.
- **P2 — Successful HIBERNATE skips the new telemetry.** [State_Sleep.cpp:1136](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:1136) normally resets without returning, bypassing `noteQueuedAtSleep()` at line 1224 and `CycleDelivery` at line 1234.
- **P2 — Reset during an attempt permanently unbalances counters.** [PublishDeliveryCounters.cpp:58](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/cloud/PublishDeliveryCounters.cpp:58) does not reconcile interrupted attempts. The actual-module probe produced **a=2, k=1, f=0, r=0**, after reboot and successful retry, with nothing in flight.
- **P2 — Mutation (iv) survived all 48 tests.** [publish_with_ack_queue_test.sh:73](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/publish_with_ack_queue_test.sh:73) checks the expected success-path removal but misses an additional failure-path dequeue.

| Criterion | Result |
|---|---|
| 1. Explicit ACK, including persisted events | Verified in source and both binaries |
| 2. ACK-success removal; failure retention | Normal source path verified; mutation coverage fails |
| 3. Bounded sleep/teardown wait | **Not verified:** ordinary pending-queue IDLE cannot reach timeout |
| 4. Counters, unchanged ledger, overflow guard | Payload placement and guard verified; accounting and hibernate snapshot fail |
| 5. Tracing | Verified |
| 6. `CycleDelivery: awake=` | Present for ULP; missing on successful HIBERNATE |
| 7. Duplicate analysis | Firmware preserves payload; downstream tolerance remains unverified |
| 8. `v24-Pubq-Ack-B` | Verified |
| 9. Both binaries’ ACK path and hooks | Verified by disassembly/call sites |
| 10. Retained placement | Verified from local map plus `nm` |

The decisive [queue line 334](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:334) is:

```cpp
const PublishFlags sendFlags = (curEvent->flags & ~PublishFlags(NO_ACK)) | WITH_ACK;
```

[BackgroundPublishRK.cpp:126](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/lib/BackgroundPublishRK/src/BackgroundPublishRK.cpp:126) publishes those flags, waits on `ok.isDone()`, and passes `ok.isSucceeded()` at line 150. Queue lines 359–408 gate dequeue on that result. Installed [Device OS 6.4.1 publisher.cpp:94](/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/src/publisher.cpp:94) establishes the ACK distinction:

```cpp
if ((flags & EventType::WITH_ACK) && msg.has_id()) {
    add_ack_handler(msg.get_id(), std::move(handler));
} else {
    handler.setResult();
}
```

**When the sleep gate is reached**, timeout logs elapsed time and queue count, requests teardown, and does not explicitly dequeue events. `cloud_status_disconnecting` calls `writeQueueToFiles()`. That flush covers `ramQueue`; an in-flight RAM `curEvent` requires subsequent failure processing. The report separates this inherited persistence qualification from the confirmed bounded-wait finding.

Both status variants contain all five counters. The ledger format is unchanged and its overflow guard remains. Modelled deployed event sizes are **896 B / 709 B** for PMIC/non-PMIC variants, within the **1023-byte usable buffer** and **1024-byte event limit**.

The duplicate analysis needs correction: firmware retries preserve the payload timestamp, but fleet-ops keys use the **top-level cloud `published_at`**, not the timestamp inside `data`. Different publication-envelope times therefore produce different keys. Actual retry behavior and Ubidots—including SUM widgets—remain bench checks. The report also documents inherited inaccurate `q` counts, global `r` attribution, saturation limits, and reversed `GateFail q` documentation.

**Suite: 48/48 (sh via zsh, py via python3)** — 24 shell scripts and 24 Python scripts.

| Mutation | Suite result | Outcome |
|---|---:|---|
| (i) Drop WITH_ACK | 47/48 | Caught |
| (ii) Dequeue before ACK | 47/48 | Caught |
| (iii) Remove queue condition from sleep gate | 47/48 | Caught |
| (iv) Dequeue on Future failure/ACK timeout | **48/48** | **Survived** |

Both candidate binaries are preserved:

| Binary | Build sizes | File bytes |
|---|---|---:|
| [Local Boron](/private/tmp/codex-wo-2026-09-25-001-stage7-j_l3p071/boron-local-6.4.1.bin) | text/data/bss: 152572 / 1114 / 2468 | 153690 |
| [Cloud Boron](/private/tmp/codex-wo-2026-09-25-001-stage7-j_l3p071/boron-cloud-6.4.1.bin) | Flash/RAM: 153782 / 3570 | 153786 |

Local SHA-256:
```text
eb956a11227982f218fd1cac28e3aa3d3d7a3406f180cca9fc1ff3f2efed16a6
```

Cloud SHA-256:
```text
8ad80f1968e89eedf33105b1e2f21b626f46d44454a041b1fa41236d16043899
```

The local reference difference is fully explained by linker padding: **−8 text / +4 data**. Cloud matches its reference after one service-timeout retry. Both compile the patched vendored libraries.

The local map records:

```text
.backup         0x000000002003f400       0xb0
 .retained_user
                0x000000002003f450       0x14 .../libuser.a(PublishDeliveryCounters.o)
```

Matching `nm` evidence:

```text
2003f450 00000014 d retainedPublishDelivery
```

Thus the counter block occupies **20 bytes in retained backup SRAM**.

All mutations were restored byte-identically. **1,223/1,223 repository files, git status, and the original diff remain unchanged.** Temporary build/test copies and executables were removed; binaries and review evidence remain outside the repository.

Model used: **gpt-6-astra**, reasoning **ultra**.