# Stage 7 binary verification — WO-2026-09-25-001

Binary-verification subtask: **VERIFIED**. This does not override firmware or test findings in the full review. Reviewer model/reasoning inherited from root: gpt-6-astra / ultra.

Both candidates were built from the byte-identical scratch snapshot `build-source`, not from the repository's build directories. After both builds, all 427 source/library/test/README/project-property files matched the root's pre-review SHA-256 manifest. No original repository file was changed by this subtask.

## Builds and delivered candidates

| Candidate | Build result | Exact BIN bytes | SHA-256 |
|---|---|---:|---|
| `boron-local-6.4.1.bin` | GNU size text/data/bss **152572 / 1114 / 2468** | 153690 | `eb956a11227982f218fd1cac28e3aa3d3d7a3406f180cca9fc1ff3f2efed16a6` |
| `boron-cloud-6.4.1.bin` | Particle Flash/RAM **153782 / 3570** | 153786 | `8ad80f1968e89eedf33105b1e2f21b626f46d44454a041b1fa41236d16043899` |

The local build used the README environment and `make -f /Users/chipmc/.particle/toolchains/buildscripts/1.17.2/Makefile compile-user -s`, with APPDIR set to this scratch `build-source`, PLATFORM=boron and DEVICE_OS_VERSION=6.4.1. It created a fresh user archive in `deviceOS/6.4.1/build/target/user/platform-13-m/build-source`.

The cloud command was `particle compile boron . --target 6.4.1 --no-update-check --saveTo /private/tmp/codex-wo-2026-09-25-001-stage7-j_l3p071/boron-cloud-6.4.1.bin`. The first request failed with exactly `Compile failed: Compiler timed out or encountered an error`; the identical retry succeeded. Both logs are retained as `cloud-build.log` and `cloud-build-retry.log`. Only the authorized Particle compile service was accessed.

The local reference **152580 / 1110 / 2468** differs from the fresh candidate by **-8 / +4 / 0**. The maps fully account for it: `.text` alignment/fill is 274 bytes in the fresh build and 282 in the reference; `.backup` is 0xb0 versus 0xac because retained objects occur in a different order and require four more padding bytes. `.data` remains 0x3a4 and `.bss` remains 0x9a4. All common named symbols have unchanged sizes. The reference archive appends newer objects (including PublishDeliveryCounters.o) near the end; the fresh archive puts them in source order. The before/after archive orders are retained. This is demonstrated layout variation, not an assumed path-length effect.

## WITH_ACK normalization in both delivered binaries

`lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:334` computes `(curEvent->flags & ~PublishFlags(NO_ACK)) | WITH_ACK` immediately before dispatch. Disassembly proves that normalization is in each delivered candidate:

| Operation | Local address | Cloud address |
|---|---:|---:|
| Load stored curEvent flags | 0xc730a | 0xc64c6 |
| Clear NO_ACK: `bic.w r5, r5, r3` | 0xc730e | 0xc64ca |
| Set WITH_ACK: `orrs r5, r3` | 0xc7316 | 0xc64d2 |
| Pass normalized flags: `mov r3,r5` | 0xc7330 | 0xc64ec |
| Call BackgroundPublishRK::publish | 0xc7336 -> 0xc5fac | 0xc64f2 -> 0xc700c |

These are register operations, not immediate `bic #4`: actual Device OS 6.4.1 NO_ACK is **2** and WITH_ACK is **8**. The local queue static initializer at 0xc7910 writes 2/8 to RAM symbols 0x2003ddd0/0x2003ddd1; the dispatch literal pool loads those same addresses. The cloud initializer at 0xc6acc writes 2/8 through literal pointers 0x2003ddb4/0x2003ddb5, precisely the pointers loaded by the cloud dispatch at 0xc659c/0xc65a0. See `binary-verification-evidence.txt` and both disassembly excerpts for all instruction/literal evidence.

The local `.text` extracted from the saved ELF equals every corresponding byte in the saved BIN (0x2538c bytes), so local ELF inspection applies to the delivered candidate. The cloud was disassembled directly as raw Thumb at module base 0xb4000.

## Both new application-thread hooks in both binaries

The inline setter names need not survive as symbols; their registrations, indirect calls, and counter callbacks do survive.

| Hook evidence | Local | Cloud |
|---|---:|---:|
| Accepted-dispatch attempt hook indirect call `blx r3` | 0xc73c2 | 0xc657e |
| Result hook indirect call `blx r3` | 0xc77a0 | 0xc695c |
| setup attempt callback branch | 0xb5388 -> 0xbd514 | 0xb5178 -> 0xbee3c |
| setup result callback branch | 0xb538e -> 0xbd540 | 0xb517e -> 0xbee68 |
| noteAttempt function | 0xbd514, size 0x2c | 0xbee3c |
| noteResult function | 0xbd540, size 0x2c | 0xbee68 |

The queue callback fields at offsets 208/224 (invokers 220/236) are populated in setup. In cloud setup, literal 0xb6e68 points to Thumb callback 0xb5179 and literal 0xb6e70 points to 0xb517d. Stores at 0xb6c3a/0xb6c80 populate invokers 220/236; queue dispatch later loads those exact fields and calls them.

The cloud noteAttempt/noteResult bodies each uniquely match 40 local code bytes before their relocated literal pools. The attempt-hook block matches 34 bytes uniquely; the result-hook block matches 50; the flag initializer matches 20. This proves real production code and call sites, beyond string-presence evidence.

Local compiler dependency files show the consumed `.cpp`/`.h` under scratch `lib/PublishQueuePosixRK` and `lib/BackgroundPublishRK`. The cloud source list includes both vendored `.cpp`/`.h` files. More decisively, cloud machine code contains the new queue normalization, both hook invocations and their application registrations. Registry PublishQueuePosixRK 0.0.7 without these changes cannot explain that code. The commented-out root registry dependency therefore does result in the patched vendored queue being compiled by both builds.

## Retained placement (criterion 10)

Saved local linker map `boron-local-6.4.1.map`:

```text
159205:BACKUPSRAM_USER  0x000000002003f400 0x0000000000000c00 xrw
165901:.backup         0x000000002003f400       0xb0 load address 0x00000000000d977c
165903:                0x000000002003f400                link_global_retained_start = .
165904: *(.retained_user*)
165927: .retained_user
165928:                0x000000002003f450       0x14 ../../../build/target/user/platform-13-m/build-source//libuser.a(PublishDeliveryCounters.o)
165947:                0x000000002003f4b0                link_global_retained_end = .
```

GNU nm from the same saved ELF:

```text
2003f450 00000014 d retainedPublishDelivery
```

Thus `retainedPublishDelivery` occupies **20 bytes at 0x2003f450–0x2003f463**, inside `.retained_user` within `.backup`, within BACKUPSRAM_USER [0x2003f400, 0x20040000). The linker map prints the local symbol's input section/object rather than the local symbol's name; nm supplies the exact symbol identity at the identical address and size. `build/arm/linker/nRF52840/linker_backup_ram_user.ld:1–8` maps `*(.retained_user*)` into BACKUPSRAM_USER.

## Kept evidence / cleanup

Kept outside the repository: both distinct BIN candidates, the local ELF/MAP, SHA256SUMS, build logs, full and excerpted disassembly, symbol tables, local compiler dependency evidence, archive-order evidence and this report. Scratch build source and the new scratch-specific toolchain user-object directory were removed after verification. No flashing, commits, pushes, stash, reset, checkout, device-setting changes or AWS access occurred.
