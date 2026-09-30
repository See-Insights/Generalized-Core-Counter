# WO-2026-09-29-002 — Implementation Report (Stage 6 dispatch)

Branch `wo/2026-09-29-002-config-cleanup`, HEAD `c9c43b0` on top of `main` `6e7a3e1` (v28-CloseBeforeSleep).
**Nothing was committed, pushed, merged, tagged or released.** The working tree is left dirty; each
step is delivered as a cumulative patch in this directory for Chip to turn into commits.

Suite runner: `run-host-suite.sh` — 22 `.sh` via `zsh` + 23 `.py` via `python3` = 45 baseline.
Binary harness: `binary-check.sh` — from-scratch Boron build out of
`~/.particle/toolchains/deviceOS/6.4.1`, fresh `BUILD_PATH_BASE`, `-frandom-seed=$@`, then
`objcopy -O binary -j .module_info .dynalib .text .ARM.exidx .data .backup .module_info_product`.

**Harness validation:** on the unmodified tree it reproduced the WO's baseline
`ad322c5dd8517b78092fdaff95a581b8326382acac4c5183dc03817a4625bf3d` (151322 bytes) exactly, so every
"identical"/"differs" claim below is measured against a method the WO already blessed.

---

## Step 1 — Delete what is dead

**Patch:** `step-1.cumulative.patch`   **Net lines: −96**

| File | +/− |
|---|---|
| `src/Settings.h` | deleted (−51) |
| `src/ProjectConfig.h` | deleted (−35) |
| `src/cloud/ConfigMerge.cpp` | deleted (−1, empty stub) |
| `src/Generalized-Core-Counter.cpp` | −1 (`#include "ProjectConfig.h"`) |
| `src/Config.h` | −1/+1 (include repointed to `power/ConnectivityPolicy.h`) |
| `src/power/ConnectivityPolicy.h` | −1 (`CONNECT_BUDGET_DEFAULT_SEC`, no callers) |
| `src/sensors/SensorManager.cpp` | −2/+2 |
| `src/device_pinout.h` | −1/+2 |
| `project.properties` | −1/+1 (`platform=p2` → `boron`) |

`Config.h`'s three connection defaults previously came from `Settings.h`; they now alias the
`ConnectivityPolicy` constants (`CONNECT_BUDGET_DEFAULT_MS / 1000UL`, `DISCONNECT_CLOUD_DEFAULT_SEC`,
`DISCONNECT_MODEM_DEFAULT_SEC`) so the values still have exactly one owner.

`SensorManager.cpp` used `#if defined(MUON_HAS_TMP112)`, which was true for `-DMUON_HAS_TMP112=0`.
Changed to `#if MUON_HAS_TMP112` — the guard now means what its name says. This is the one
behavioural correction in step 1 and it does not alter the default build (macro undefined either way).

**Nothing needed to be moved out of `Settings.h`.** Its catalog text duplicated documentation that
already lives with the code: build flags in `BuildProfile.h`, the `MUON_*` flags at their use site in
`SensorManager.cpp`, sensor type IDs in `SensorFactory.h`. `device_pinout.h`'s stale pointer at
`Settings.h` was repointed at `SensorManager.cpp`.

**Tests changed:** none.

**Binary evidence:** `ad322c5dd8517b78092fdaff95a581b8326382acac4c5183dc03817a4625bf3d`, 151322 bytes
— **byte-identical to baseline.**  **Suite: 45/45.**

---

## Step 2 — One build profile

**Patch:** `step-2.cumulative.patch`   **Net lines: −7** (cumulative −103)

`src/BuildProfile.h` is now the only file that decides what a build contains:

- **`BUILD_PROFILE_BENCH`** (default `0` = release) with an `#error` if it is neither 0 nor 1.
  `ENABLE_DIAGNOSTICS_PUBLISH_MODE` now defaults to `BUILD_PROFILE_BENCH`, so `-DBUILD_PROFILE_BENCH=1`
  is the whole bench switch. `ENABLE_RTC_SKEW_TEST` is explicitly held at 0 with a comment saying the
  bench profile must not turn it on — it is a deliberate clock-corrupting test, not a diagnostic.
- Retired `ENABLE_PMIC_TRACE` and `ENABLE_PMIC_CHARGE_CYCLE_TEST` (no readers) and the
  `ENABLE_PMIC_REGISTER_DUMP` doc line (the flag itself no longer existed).
- Deleted the `DEBUG_SERIAL` convenience block — nothing referenced it; a stale comment in
  `Generalized-Core-Counter.cpp` that still mentioned it was corrected.
- **Serial logging policy moved here**: `SERIAL_LOG_LEVEL` under `#ifndef` with an `#error` range
  check, plus a `SERIAL_LOG_CATEGORY_FILTERS` macro holding the eight WARN filters.
  `cloud/Particle_Functions.cpp` had its own `#define SERIAL_LOG_LEVEL 3` and an inline copy of the
  filter list; both are gone and its level-3 handler now expands `{SERIAL_LOG_CATEGORY_FILTERS}`.
- **`COMPILED_BUILD_FLAGS`** — the 33-line build-flags bitmask was duplicated verbatim in
  `Generalized-Core-Counter.cpp` and `cloud/DeviceStatusPublisher.cpp`. Both are now
  `const uint16_t compiledBuildFlags = COMPILED_BUILD_FLAGS;`, with
  `BUILD_FLAGS_BORON_USB_SOURCE_OVERRIDE_BIT` named next to it.

**Ordering hazard checked.** The `0x4000` bit resolves `PLATFORM_ID == PLATFORM_BORON` where
`BuildProfile.h` is preprocessed, so a macro could have silently gone false. Both emitting TUs reach
`Particle.h` before `BuildProfile.h` (via `Config.h → power/ConnectivityPolicy.h → Particle.h`), and
the evidence confirms it: the release binary is unchanged, so the word is still `0x4008` and no bit
was renumbered.

**Tests changed: none.** `build_flags_witness_test.sh` and `power_source_override_test.cpp:294` both
pin the flags word and both pass **unchanged**.

**Binary evidence:**
- Release: `ad322c5dd8517b78092fdaff95a581b8326382acac4c5183dc03817a4625bf3d`, 151322 bytes —
  **byte-identical to baseline.**
- Bench (`-DBUILD_PROFILE_BENCH=1`): `461350653071e79a…851c`, 152286 bytes. Contains `pdiag` and
  `PowerDiagnostics::flushDiagBatch`; contains no `RtcSkewTest`.

**Suite: 45/45.**

> **Size-rule event.** The first working version of this step measured **+6 net lines** and I stopped
> rather than continue. The added code was already minimal, so I paid for it by cutting verbose
> comment blocks in `BuildProfile.h` until the step measured −7, then re-ran the binary check and the
> full suite on the trimmed version. No behaviour was changed to meet the budget.

---

## Step 3 — The version in one place

**Patch:** `step-3.cumulative.patch`   **Net lines: −51** (cumulative −154)

`src/FirmwareVersion.h` is now the only place the version exists:

```c++
#define FIRMWARE_PRODUCT_VERSION 28
inline const char* FIRMWARE_VERSION = "v28-CloseBeforeSleep";
```

- `src/Version.h` and `src/Version.cpp` deleted.
- `FIRMWARE_RELEASE_NOTES` **removed from the firmware entirely.** It was a 147-character string
  compiled into flash and never read by the device. Its text is now the
  `## [v28-CloseBeforeSleep] - 2026-09-30` entry in `CHANGELOG.md`, which is where release notes
  belong.
- Consumers repointed: `Generalized-Core-Counter.cpp` lost its `#include "Version.h"`, its
  `extern` block and an orphaned comment; `cloud/DeviceStatusPublisher.cpp` swapped its
  `extern const char* FIRMWARE_VERSION;` for `#include "../FirmwareVersion.h"`.
- `bump_version.sh` no longer touches the deleted `Version.cpp`; it edits `src/FirmwareVersion.h` for
  both the string and the integer. `PRODUCT_VERSION_INT` is derived with
  `sed -E 's/^v//; s/[.-].*$//'` and a numeric guard, so `v28-CloseBeforeSleep` → `28`.
- `release.sh` stages `src/FirmwareVersion.h` instead of `src/Version.cpp` (two lines).
- `README.md:11` and `Doxyfile:5` corrected to `v28-CloseBeforeSleep` (both said `v27-…`).

**`bump_version.sh` was executed, not just read.** In a scratch copy: `v29-NextThing` produced
`FIRMWARE_PRODUCT_VERSION 29` and `FIRMWARE_VERSION = "v29-NextThing"` and updated Doxyfile, README
and CHANGELOG; the legacy `4.00` form produced `FIRMWARE_PRODUCT_VERSION 4`; a version with no digits
is rejected with a clear error and exit 1.

**Tests changed (2, both forced by the header now defining the symbol):**
`tests/stubs/clock_status_republish_overrides/CloudLinkStubs.cpp` and
`tests/power_source_override_test.cpp` each defined their own
`const char *FIRMWARE_VERSION = "test-fw";`, which is now a duplicate definition. Both definitions
were deleted; the tests take the real value from the header. No assertion depended on `"test-fw"`, so
no expectation was weakened.

**Binary evidence** (the WO's four-part step-3 check):
- `a37a5d07adc90774c0346ec572fa15abe9698c491340e246d7c520236614738b`, 151178 bytes.
- **`nm -S --defined-only` names and sizes: zero differences.** The `inline` variable preserved the
  `FIRMWARE_VERSION` symbol and its size exactly.
- **Size difference explained:** −144 = the 148-byte release-notes string (147 chars + NUL) minus
  4 bytes of alignment padding the linker re-packed. Only `.text` moved: 150196 → 150052.
- **Disassembly identical apart from addresses.** After normalising addresses and symbol offsets,
  every remaining difference is a raw rodata byte line, and their content is exactly the removed
  release-notes text and the relocated version string. **No instruction differs.**
- **Version unchanged:** `v28-CloseBeforeSleep` still in `strings`; the release-notes text is gone.

**Suite: 45/45.**

---

## Step 4 — One webhook timeout range

**Patch:** `step-4.cumulative.patch`   **Net: +6 `src/` lines** (budget "about 5") **plus its test.**
Cumulative excluding the new test: **−148**.

The bug this closes: the configuration check accepted **1000–60000 ms** while the runtime check
discarded anything outside **5000–120000 ms**. A webhook timeout of 1000 ms was accepted by the cloud,
reported as applied, and then silently replaced with the 20000 ms fallback on every loop. Values
between 60001 and 120000 were refused despite being perfectly usable.

**Owner: `SystemConfig`**, which owns `webhookTimeoutMs`. In `src/persist/SystemConfig.h`:

```c++
constexpr uint32_t kWebhookTimeoutMinMs = 5000UL;
constexpr uint32_t kWebhookTimeoutMaxMs = 120000UL;
```

- `src/cloud/ConfigApply.cpp:575` — `validateRange(webhookTimeout, 1000, 60000, …)` now passes the two
  constants (2 lines; one line would be 146 characters).
- `src/Generalized-Core-Counter.cpp:1659` — the runtime guard's literals replaced by the same two
  constants (no line change).

The WO's range is used as specified: **5000–120000 ms**, the runtime check's range, because it is the
one the device actually honours.

**Test added: `tests/webhook_timeout_range_test.py`** (suite 45 → 46).

It deliberately does **not** restate the bounds — that would only prove the test and the header were
typed by the same person. It lifts the two *real* predicate expressions out of the shipping sources by
regex, compiles them against the real `SystemConfig.h`, and asserts the config check and the runtime
check agree at **4999, 5000, 120000 and 120001**. It also asserts the constants are defined exactly
once anywhere under `src/`, and that neither call site uses a bare literal.

**The test was mutation-checked.** Restoring `validateRange(webhookTimeout, 1000, 60000, …)` makes it
fail with the original defect stated in its own words:

```
- 4999 ms: config check accepts it but runtime check discards it -
  an accepted value must be the value used
- 120000 ms: config check rejects it but runtime check uses it -
  an accepted value must be the value used
```

The source was then restored and verified byte-identical to its backup.

**Binary evidence:** `1673f72e35ddc2237fd435dc102ec128cf85f4dcd9a04c7a4a3ad3fab902e2b6`, 151178 bytes.
Differences are confined to the timeout range:
- `nm -S` reports **exactly one** changed symbol: `Cloud::applyReportingConfig` grows 4 bytes
  (0x1e0 → 0x1e4) — the literal-pool word for 120000, which no longer fits an immediate.
- Normalised disassembly diff is **23 lines**, all at that call site: `movw #60000 / mov.w #1000`
  becomes `ldr r3,[pc] / movw #5000`, plus one alignment `nop`, one `.word` added and one removed,
  and the resulting rodata/jump-table byte shifts. Nothing else in the image changed.

**Suite: 46/46.**

---

## Final release build

Normal local build (`make clean` then `make PLATFORM=boron APPDIR=…` in
`deviceOS/6.4.1/modules/boron/user-part`):

| | v28 baseline | after | delta |
|---|---|---|---|
| text | 150308 | **150164** | **−144** |
| data | 1090 | **1090** | 0 |
| bss | 2196 | **2196** | 0 |

The −144 is the step-3 release-notes string and nothing else; `data` and `bss` are untouched, so no
RAM footprint moved. `strings` on the release image: `v28-CloseBeforeSleep` present exactly once,
**no `pdiag`**, no `RtcSkew`, and the release-notes text absent.

## Totals

| Step | Net lines | Cumulative | Binary |
|---|---|---|---|
| 1 | −96 | −96 | identical to baseline |
| 2 | −7 | −103 | identical to baseline |
| 3 | −51 | −154 | −144 bytes, release notes only |
| 4 | +6 `src/` + test | −148 (excl. test) | +0 bytes, one symbol +4 |

Files deleted: 5 (`Settings.h`, `ProjectConfig.h`, `cloud/ConfigMerge.cpp`, `Version.h`, `Version.cpp`).
Suite: **45/45 → 46/46** (sh via zsh, py via python3). `tests/publish_with_ack_structural_test.py`
passes unchanged throughout.

## Files shared between steps

- `src/Config.h` — step 1 (include repointed, defaults aliased). Step 2 did not touch it, but its
  include chain is what makes step 2's `PLATFORM_BORON` check resolve correctly.
- `src/BuildProfile.h` — step 2 only, but it is now the single owner of the build profile, the serial
  logging policy and `COMPILED_BUILD_FLAGS`, so later steps had to avoid adding to it.
- `src/Generalized-Core-Counter.cpp` — step 1 (include), step 2 (bitmask, stale comment), step 3
  (version externs), step 4 (timeout guard).
- `src/cloud/DeviceStatusPublisher.cpp` — step 2 (bitmask) and step 3 (`FIRMWARE_VERSION` extern).
- `src/persist/SystemConfig.h` — step 4 only.

## Deviations

1. **CHANGELOG entry is `v28`, not `v27`.** The WO says the release notes become the v27 entry; the
   dispatch says v28. The notes describe v28's close-before-sleep work, so v28 is right, and the
   dispatch is the later instruction. Followed the dispatch.
2. **`DeviceStatusPublisher.cpp:38` is a `FIRMWARE_VERSION` extern, not `FIRMWARE_RELEASE_NOTES`.**
   The WO lists it under step 1's deletions. There is no `FIRMWARE_RELEASE_NOTES` declaration in that
   file. Handled in step 3, where the WO assigns consumer repointing, so the reference is repointed in
   the same step that changes what it points at.
3. **No catalog text was relocated out of `Settings.h`.** The WO anticipates moving the useful parts
   to the code they describe; every part was already documented at its owner. Recorded rather than
   invented work.
4. **`src/device_pinout.h` comment fix (+1 line, step 1)** — not named in the WO. It pointed readers at
   `Settings.h` for the `MUON_*` flags; deleting the file without fixing the pointer would have left a
   dangling reference.
5. **Step 2 first measured +6 lines** and was trimmed to −7 before acceptance (see step 2 above). No
   step was ever delivered over budget.
6. **Step 4 is +6 `src/` lines, not 5.** The `validateRange` call cannot fit the two qualified constant
   names on one line within the file's line length, so it is two lines instead of one.

## Not done, as instructed

No commit, no push, no merge, no tag, no `release.sh` run, no branch change, no `docs/` edits.

## Model

**claude-opus-5, reasoning level medium.**
