**Stage 7 round 2 binary verification — WO-2026-09-25-001: VERIFIED for linkage/build checks.** This result is limited to candidate builds and linkage; it does not override findings from the full review. Model/reasoning inherited from the parent: **gpt-6-astra / ultra**.

The candidate source/configuration comprised **296 files**, copied before mutations began. Every hash in `source-sha256.json` matched both build copies after compilation. The two builds used independent scratch copies; no original repository source, library, config, or build artifact was changed by this task. The unique local application-object directory did not exist before the build, so every application/library object was freshly compiled.

| Candidate | Build-reported sizes | Exact BIN bytes | SHA-256 |
|---|---|---:|---|
| [Local BIN](boron-local-6.4.1.bin) | text/data/bss **153508 / 1114 / 2484** | **154626** | `93a3c7641cd344db04f1c4dca4653b72222e94646208dc702e2d0f7047d3127a` |
| [Cloud BIN](boron-cloud-6.4.1.bin) | Flash/RAM **154718 / 3578** | **154722** | `b216044914dc497bdef57bcb4b7e7c13e39d233fb1c5a5d2849354f3134efc4a` |

Both builds succeeded on the first request. The exact commands were run from their respective scratch application roots:

```zsh
DEVICE_OS_PATH='/Users/chipmc/.particle/toolchains/deviceOS/6.4.1' \
APPDIR='/private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/binaries/local-source-FbwtgJ' \
PLATFORM=boron DEVICE_OS_VERSION=6.4.1 \
GCC_ARM_PATH='/Users/chipmc/.particle/toolchains/gcc-arm/10.2.1/bin/' \
PATH="$PATH:/Users/chipmc/.particle/toolchains/gcc-arm/10.2.1/bin" \
make -f '/Users/chipmc/.particle/toolchains/buildscripts/1.17.2/Makefile' compile-user -s

particle compile boron . --target 6.4.1 --no-update-check \
  --saveTo /private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/binaries/boron-cloud-6.4.1.bin
```

The local command follows [README.md:194](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/README.md:194). Build output is retained in `local-build.log` and `cloud-build.log`. Network use was limited to the authorized Particle compilation operation.

The local reference was **153516 / 1114 / 2484**. The fresh local candidate is **8 bytes smaller in text**; `.text` linker padding is 271 bytes in 135 fill entries, versus 279 bytes in 137 entries in the reference map. All common named symbols have the same sizes, with no added or removed symbol sizes; `.data` (0x3a4), `.bss` (0x9b4), and `.backup` (0xb0) are identical. The reference archive appends newly introduced objects, whereas the fresh archive follows source order. The 8-byte delta is therefore fully accounted for by linker padding. See `reference-size-comparison.txt`, `reference-nm.txt`, and `archive-order.txt`.

The cloud Flash result matches reference **154718**; current RAM **3578** is **8 bytes below** reported reference **3586**. The earlier cloud binary was deleted, according to the round-3 report. Consequently the exact cause of that historical cloud RAM difference cannot be established from the available artifact. It is recorded as an unexplained size-report discrepancy, not represented as an exact reference match or proof that a source feature is absent. The code/linkage evidence below is from the exact newly delivered cloud binary.

The local ELF `.text` equals the corresponding **0x25734 bytes** in the local BIN byte-for-byte, at address **0xb4020**. Thus local ELF disassembly applies to the delivered BIN. The cloud BIN was disassembled directly as Thumb, using module base **0xb4000**. `cloud-function-matches.json` records unique function matches after ignoring relocated branch/call encodings and literal-pool words; all other opcode bytes had to match. External calls, literal values, and production callers were then inspected directly. Presence claims do not rely on `strings`.

| Required behavior / compiled function | Local ELF address | Exact cloud BIN address | Matching unrelocated opcode bytes |
|---|---:|---:|---:|
| Queue dispatch `stateWait()` | 0xc7478 | 0xc6638 | 316 |
| Queue result processing `statePublishWait()` | 0xc79a0 | 0xc6b60 | 320 |
| Background `publish()` | 0xc61d4 | 0xc7244 | 158 |
| Background worker `thread_f()` | 0xc5edc | 0xc6f4c | 444 |
| Counters `begin()` | 0xbd588 | 0xbee24 | 48 |
| Counters `noteAttempt()` | 0xbd5cc | 0xbee68 | 24 |
| Counters `noteResult(bool)` | 0xbd5f4 | 0xbee90 | 28 |
| Counters `noteQueuedAtSleep()` | 0xbd61c | 0xbeeb8 | 8 |
| Counters `snapshot()` | 0xbd634 | 0xbeed0 | 32 |
| Gate `publishInFlight()` | 0xbd658 | 0xbf460 | 8 |
| Gate `queuePermitsSleep()` | 0xbd664 | 0xbf46c | 76 |
| Budget `evaluate()` | 0xbd510 | 0xbf3f8 | 84 |
| Pre-sleep delivery accounting | 0xc2a9c | 0xc1ac8 | 64 |

`noteSleptWithQueued()` is at local **0xbd628**, cloud **0xbeec4**: its small generic instruction pattern alone is nonunique, so its actual accounting caller (cloud **0xc1ae0**) and counter literal **0x2003f470**, followed by the saturating bump at **0xbee14**, establish identity. Likewise the budget getter is proven by the gate call (cloud **0xbf4b6 → 0xbf3d4**) and its literal **90000** at **0xbf3d8**, not by a nonunique short getter pattern.

The explicit `WITH_ACK` dispatch is present in both candidates. The production source [PublishQueuePosixRK.cpp:334](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:334) clears `NO_ACK` and sets `WITH_ACK` before the background dispatch. The binary path is:

| Instruction / operation | Local | Cloud |
|---|---:|---:|
| Clear `NO_ACK` (`bic.w r5,r5,r3`) | 0xc753a | 0xc66fa |
| Set `WITH_ACK` (`orrs r5,r3`) | 0xc7542 | 0xc6702 |
| Pass normalized flags (`mov r3,r5`) | 0xc755c | 0xc671c |
| Background dispatch call | 0xc7562 → 0xc61d4 | 0xc6722 → 0xc7244 |
| Store flags in background object, offset 1103 | 0xc627c | 0xc72ec |
| Worker loads those stored flags | 0xc5f66 | 0xc6fd6 |
| Worker calls `CloudClass::publish_event` | 0xc5f7e | 0xc6fee |

Device OS 6.4.1 flag values are **NO_ACK=2, WITH_ACK=8**. The local queue initializer at **0xc7b44** writes 2 and 8 through literals **0xc7b58/0xc7b5c → 0x2003ddd0/0x2003ddd1**; the dispatch loads those same addresses from **0xc7610/0xc7614**. Cloud initialization at **0xc6d04** writes 2 and 8 through **0xc6d18/0xc6d1c → 0x2003ddb4/0x2003ddb5**, and dispatch loads those exact addresses at **0xc67d0/0xc67d4**. This establishes actual dispatch flag values, including clearing conflicting `NO_ACK`.

Both application-thread hooks have registrations, queue invocations, and counter targets. Production registrations are at [Generalized-Core-Counter.cpp:1142](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:1142). The accepted-dispatch hook runs at local **0xc75f4**, cloud **0xc67b4**, through callback-invoker field offset **224**. The result hook runs at local **0xc79d0**, cloud **0xc6b90**, through invoker offset **240**. Cloud setup loads Thumb callback pointers **0xb5179/0xb517d** from **0xb6e74/0xb6e7c**, and stores the invokers into offsets 224/240 at **0xb6c46/0xb6c8c**. The callback thunks branch **0xb5178 → 0xbee68** (`noteAttempt`) and **0xb517e → 0xbee90** (`noteResult`). Local thunks similarly branch **0xb5388 → 0xbd5cc** and **0xb538e → 0xbd5f4**. Counters `begin()` has a live setup caller at local **0xb6de6**, cloud **0xb6bf2**.

The in-flight accessor is inline in [PublishQueuePosixRK.h:325](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h:325); its behavior survives in the out-of-line gate wrapper. Local **0xbd65e** and cloud **0xbf466** load queue field offset **180**. Dispatch sets that field at local **0xc75d0**, cloud **0xc6790**, and result processing clears it at local **0xc7a52**, cloud **0xc6c12**. These are the same field accessed by the gate adapter (local **0xbd688**, cloud **0xbf490**).

The delivery gate is called by all three production gates: [State_Idle.cpp:240](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Idle.cpp:240), [State_Idle.cpp:295](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Idle.cpp:295), and [State_Sleep.cpp:566](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:566). Local call addresses are **0xc1f0a, 0xc1f46, 0xc2f5c → 0xbd664**; cloud addresses are **0xc4256, 0xc4292, 0xc1f8c → 0xbf46c**. The adapter invokes the real budget evaluator at local **0xbd6a4 → 0xbd510**, cloud **0xbf4ac → 0xbf3f8**. The evaluator compares elapsed time to **89999**, allowing expiry from **90000 ms**, then reads/inverts the in-flight flag before returning permission. The expiry branch calls INFO logging, with a verified pointer to `DeliveryBudget: expired budget=%lums elapsed=%lums qn=%u inflight=%d`. The final sleep hold calls the in-flight wrapper at local **0xc3158**, cloud **0xc2188**. Two real sleep-path callers invoke delivery accounting at local **0xc3ab6/0xc3c68**, cloud **0xc2ae6/0xc2c98**.

The local linker map proves retained placement. Quoted from `boron-local-6.4.1.map`:

```text
161788:BACKUPSRAM_USER  0x000000002003f400 0x0000000000000c00 xrw
168522:.backup         0x000000002003f400       0xb0 load address 0x00000000000d9b24
168524:                0x000000002003f400                link_global_retained_start = .
168525: *(.retained_user*)
168548: .retained_user
168549:                0x000000002003f450       0x18 ../../../build/target/user/platform-13-m/local-source-FbwtgJ//libuser.a(PublishDeliveryCounters.o)
168567:                0x000000002003f4b0                link_global_retained_end = .
```

`nm -S -n -C` on the same ELF identifies the local symbol at the same address and size:

```text
2003f450 00000018 d retainedPublishDelivery
```

Therefore `retainedPublishDelivery` occupies **24 bytes**, **0x2003f450–0x2003f467**, in `.retained_user` within `.backup`, in the BACKUPSRAM_USER range **[0x2003f400,0x20040000)**. The linker mapping is in [linker_backup_ram_user.ld:1](/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/build/arm/linker/nRF52840/linker_backup_ram_user.ld:1). [PublishDeliveryCounters.cpp:30](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/cloud/PublishDeliveryCounters.cpp:30) sets retained version **2**. The linked `begin()` code compares version with **2** at local **0xbd596**, cloud **0xbee32**, and initializes it to **2** at local **0xbd5a4/0xbd5a8**, cloud **0xbee40/0xbee44**. The reset recovery path tests outstanding byte offset21, increments abandoned at offset18, and sets retryPending at offset20 in both images.

Compiler dependencies identify the patched vendored queue/background files as local-build inputs. The cloud submission list includes those vendored files, and the exact cloud machine code contains the newly added hooks, in-flight tracking, normalization, and delivery gate. This independently confirms that the registry queue does not replace the patched queue in this cloud candidate.

Evidence delivered outside the repository: the two BINs, local ELF/MAP, SHA256SUMS, source hashes, build logs, disassembly, nm/section/object/dependency evidence, normalized function-match results and reproduction scripts, reference padding/archive comparisons, and this report. Temporary source copies and the unique local application-object directory are removed after verification. Existing repository/toolchain outputs were not deleted. No flashing, device settings, AWS, git mutations, commits, pushes, stash, reset, or checkout occurred.
