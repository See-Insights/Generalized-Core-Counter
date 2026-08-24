# WO-2026-08-24-001: Compile PowerSourceOverride unconditionally on Boron; surface compiled build-flag state at runtime

## Context

The `PowerSourceOverride` block in `PowerManager::refreshInputProfile()` was
compiled out of the binary running on the bench Borons. It is gated behind
`ENABLE_BORON_USB_SOURCE_OVERRIDE`, which was off in that specific build
despite the source default being `1` and the version string being identical to
a build where it was on.

With the override absent, a genuine Device OS VIN misread on a USB-powered
device goes uncorrected and `selectInputProfile()` latches `Solar35W`. On a
~5.1 V USB source, `SOLAR_MIN_VOLTAGE_MV` (5080) leaves no headroom against
`USB_BENCH_MIN_VOLTAGE_MV` (3880); VINDPM engages, charge current collapses to
near zero while the PMIC still reports `chg=FAST`. Boron-Dev-14 discharged from
3.727 V to 3.069 V over 27 hours under exactly this condition and is now dead.

This is a recurrence — by a different proximate cause — of the Solar/USB
voltage-mismatch failure in `docs/postmortems/2026-06-pmic-charging-root-cause.md`.
That fix (removing `USE_VIN_SETTINGS_WITH_USB_HOST`, `PowerPlatform.cpp:90`)
remains correctly in place and is **not** implicated here.

### Evidence (verified against compiled artifacts, not source reasoning)

1. `target/6.4.1/boron/Generalized-Core-Counter.bin` (mtime 2026-08-12 15:25):
   `PowerSourceOverride` and `usb_enumerated` absent from both the `.bin` and
   the unstripped `.elf`, while `usbAddr` (the PowerDiag *logging* format
   string) is present.
2. Disassembly of `PowerManager::refreshInputProfile()` at `0xbd08c`:
   `bl PowerPlatform::readPowerSource()` (bd12e) → `str r7,[sp,#76]` (bd13c) →
   directly into the inlined profile switch (bd13e). No NRF register load, no
   `0x7F`/`0x03` compare, no `Logger::warn`, no branch.
3. Controlled rebuild of `3b9b39f`: default flags → override present,
   145,758 B. `ENABLE_BORON_USB_SOURCE_OVERRIDE 0` → absent, 145,582 B.
   Deployed → absent, 145,478 B. String-table diff between the flag-0 rebuild
   and the deployed image shows only newlib toolchain paths (local vs cloud
   build), nothing functional.
4. **Product firmware v22 is the defective build, and it is the only v22.**
   Particle product 42131 firmware v22: `size 145478`,
   `app_hash 64188ac3ea539d95b65c4ac0642d4cc3a35cd57da0df80447af2caebe682b6ba`,
   uploaded 2026-08-12T07:26:13Z. That hash is byte-identical to the local
   `target/6.4.1/boron` build in (1)/(2).
5. **Dev-11 is affected, confirmed directly.** Device API for product 42131:
   Dev-09, Dev-11 and Dev-14 all report `firmware_version=22`,
   `desired_firmware_version=22`, `development=False`, product-locked to 42131.
   All three therefore run the artifact in (4). Dev-09's build is no longer
   inferred; Dev-11 is a live second at-risk device. Dev-11 has not yet been
   harmed only because its PMIC reported `vbus=1/2` correctly all week — it has
   never been exposed, but it carries the same defect.
6. `boron_firmware_1786624583213.bin` (2026-08-13, also `v22-Diag-Soak`, also
   product version 22, `app_hash 3ae0fdcf84e7…`) **does** contain the override
   and was never released as a product firmware version. This is the build
   Dev-14 was running on 08-13 when it was still firing overrides; it was moved
   to product v22 on 08-14, which is exactly when its override firings stopped.

### Ruled out

Any timing/boot-sequence dependency on `ALLOW_BLOCKING_SERIAL_WAITS`.
`refreshInputProfile()` has three callers spanning a device's whole
operational life — `PowerManager.cpp:96`, `Generalized-Core-Counter.cpp:1278`,
and `SensorManager.cpp:756` on every battery sample — so a boot-time
enumeration race could not hold Solar for 27 hours. Dev-14's own override lines
on 08-13 fired at 331 s and 7,590 s uptime, well past `setup()`. This was the
original working hypothesis for the regression and is conclusively disproven.

## Fix

### (a) Delete `ENABLE_BORON_USB_SOURCE_OVERRIDE`; compile the override unconditionally on Boron

Measured cost: 176 bytes. The predicate's own `usbEnumerated` gate already
self-limits: `USBADDR & 0x7F != 0` is unsatisfiable with nothing enumerated, so
a solar/cellular-only device pays two register reads and a branch. Those same
two reads already execute on every `logPowerState()` call on every Boron in the
fleet — park devices included — and have for months, so the read itself is
field-proven safe.

If a bench-only disable is still genuinely wanted, invert it to an explicit
opt-**out** flag, and make that opt-out stamp a distinct `FIRMWARE_VERSION`
suffix so a build with the override disabled can never again be
version-string-identical to one where it is present.

### (b) Surface actually-compiled build-flag state at runtime

No build flag is currently observable from a running device: every one is a
bare `#if`, and `FIRMWARE_VERSION` is a fixed string (`src/Version.cpp:6`) that
does not reflect what was compiled in. Add a flags witness to:

- the `Boot:` serial line (`Generalized-Core-Counter.cpp:1226`), and
- the ledger firmware object (`DeviceStatusPublisher.cpp`, the
  `writerBase.name("firmware")` object).

The witness must be derived from the same `#if` conditions that gate the
features, so it cannot drift from reality. Keep it compact — the ledger payload
has a byte ceiling and `deviceStatusLedgerSizeBytes` is already 445–466.

## Explicitly out of scope — separate WO, not bundled here

**(c) Runtime profile interlock**: if `activeInputProfile == Solar35W` with DPM
engaged, charge current at floor, and SOC falling across N samples, fall back
to `UsbBench` and alert. Genuinely valuable and catches this failure class
regardless of upstream cause, but it is active runtime correction from observed
symptoms rather than a static predicate, with its own design questions
(threshold values, interaction with the existing charge-fault detector,
false-positive risk on a device genuinely on solar). Scope separately once
(a)/(b) land.

## Acceptance Criteria

1. Override present in a default Boron build, confirmed via `strings` **and**
   disassembly of the actual compiled artifact — not source inspection alone.
   The check must show the override branch inside
   `PowerManager::refreshInputProfile()`, not merely that the string exists
   somewhere in the image.
2. Near-zero size/behavior cost confirmed by before/after build comparison
   against a real build, with the byte delta reported.
3. `Boot:` line and ledger firmware object both accurately reflect
   actually-compiled flag state, tested across at least two different flag
   configurations if an opt-out flag is kept.
4. If an opt-out flag exists, confirmed to produce a distinct
   `FIRMWARE_VERSION` when set — no two builds may again share a version string
   while differing in whether the override is present.
5. Dev-11's deployed binary result reported regardless of outcome.
   **Already completed by Claude before dispatch — see Evidence (4)/(5).
   Copilot must not re-derive this from build timing or commit correlation;
   carry the finding forward as stated.**

## Required Tests

- **Compile-time**: override code present in the resulting binary by default,
  proven by `strings` + disassembly, not "compiles without error".
- **Size/behavior regression**: confirm the near-zero cost claim against a real
  build, not an estimate.
- **Runtime**: flags witness on `Boot:`/ledger accurately reflects the actual
  compiled configuration in at least two scenarios.
- Existing test suites under `tests/` must continue to pass; do not weaken or
  delete tests to obtain a pass.

## Permitted Files

- `src/BuildProfile.h` — flag removal or inversion
- `src/power/PowerManager.cpp` — removing `#if` gating at the override's
  definition/call site (**confirm the exact current scope directly; do not
  assume it is only the one block**)
- `src/Version.cpp` / `src/FirmwareVersion.h` — only if an opt-out flag is kept
  and needs to stamp a version suffix
- `src/Generalized-Core-Counter.cpp` — the `Boot:` line only
- `src/cloud/DeviceStatusPublisher.cpp` — the firmware object only
- `tests/` — new or updated tests
- `docs/` — CHANGELOG and this WO's status section

Codex to confirm full scope. Anything outside this list requires returning to
Claude and Chip, not improvisation.

## Build/verification commands

```
particle compile boron . --target 6.4.1 --saveTo /tmp/wo24/after.bin
strings -a <bin> | grep -c PowerSourceOverride
arm-none-eabi-objdump -d -C target/6.4.1/boron/Generalized-Core-Counter.elf   # toolchain: ~/.particle/toolchains/gcc-arm/10.2.1/bin
particle binary inspect <bin>
```

## Status

Ready for implementation. Diagnosis verified against the actual compiled
artifact (disassembly + controlled rebuild + product-firmware hash match), not
source reasoning alone.

This is the second real behavior fix in this code path in two weeks, and the
first one (`66c4c6e`, the `overrideActive` reporting fix) passed its own Stage 7
and then never fired in the field — because the code it reported on was not in
the shipped binary. **Full Codex Stage 7 required before this returns for final
approval, no exceptions.** Stage 7 must verify every claim against the compiled
artifact; source-level agreement is not sufficient evidence for any acceptance
criterion in this WO.

Implementation leaves the working tree uncommitted. No commit, push, merge, or
device flash without Chip's explicit sign-off.

### Implementation status (2026-08-24, Copilot/Implementer)

Implemented as approved, uncommitted in the working tree. Summary (full detail
in `/tmp/wo24/IMPLEMENTATION_REPORT.md`):

- `ENABLE_BORON_USB_SOURCE_OVERRIDE` removed from `src/BuildProfile.h`. The
  override in `PowerManager::refreshInputProfile()` now compiles
  unconditionally on Boron, gated only by
  `defined(PLATFORM_ID) && defined(PLATFORM_BORON) && (PLATFORM_ID == PLATFORM_BORON)`
  (hardened from a bare `PLATFORM_ID == PLATFORM_BORON`, which silently
  evaluated true — and required `NRF_USBD`/`NRF_POWER` — in host test builds
  that define neither macro).
- Confirmed via `strings` (single `PowerSourceOverride` occurrence) **and**
  disassembly of the real compiled `.bin` (no debug-symbol `.elf` was
  reachable in this sandboxed session; disassembly instead locates the format
  string's literal-pool reference and traces the enclosing branch, which
  reads `NRF_POWER->USBREGSTATUS`/`NRF_USBD->USBADDR`, calls `Log::warn`,
  sets `source = kPowerSourceUsbHost` (`movs r5, #2`), sets
  `overrideActive = true`, and flows directly into the profile-selection
  calls) that the override is present and reachable, not merely stringified
  elsewhere in the image.
- Size delta: 145,806 B (after) vs. 145,758 B (default-flags-ON rebuild cited
  in Evidence (3)) = **+48 bytes**, all attributable to the new flags witness
  (the override itself was already compiled in at 145,758 B).
- Added a compact `compiledBuildFlags` bitmask witness to the `Boot:` serial
  line (`flags=0x%04x`) and the ledger `firmware` object (`"flags"`),
  independently derived at each call site from the same `#if` flags (verified
  byte-for-byte identical by `tests/build_flags_witness_test.sh`, which also
  compiles the real extracted expression against the real `BuildProfile.h`
  under two different flag configurations and confirms the resulting value
  changes as expected: `0x2008` default vs. `0x2103` flipped).
- No opt-out flag was reintroduced (fix (a) applied in its unconditional
  form), so `FIRMWARE_VERSION`-suffix acceptance criterion 4 does not apply.
- Dev-11 finding carried forward unchanged from Evidence (4)/(5), not
  re-derived: Dev-11 runs the same defective product-v22 artifact as Dev-09
  and Dev-14 and carries the same defect, unexercised only because its PMIC
  has reported `vbus=1/2` correctly all week.
- All existing `tests/*.sh` pass, plus the new `build_flags_witness_test.sh`
  and the extended `power_source_override_test.cpp` ledger-flags assertion.

Full Codex Stage 7 against the compiled artifact remains required before this
returns for final approval, per the paragraph above.

---

## Addendum A (2026-08-24): witness bit for the override's platform guard

Raised as Concern 1 during Claude's post-implementation artifact verification
and approved by Chip for immediate inclusion in this WO's diff.

### Problem

The `compiledBuildFlags` witness added by fix (b) covers 14 build flags but has
**no bit for the guard that actually gated the override in this incident**:

```c
#if defined(PLATFORM_ID) && defined(PLATFORM_BORON) && (PLATFORM_ID == PLATFORM_BORON)
```

Because `ENABLE_BORON_USB_SOURCE_OVERRIDE` was deleted, there is nothing left
for the witness to report about the override. If `PLATFORM_BORON` is ever
undefined in a future build, that guard evaluates false and the override
compiles out **silently and invisibly** — the exact failure shape that produced
this incident, now un-witnessed by the very mechanism built to catch it.

`PLATFORM_BORON` is confirmed defined in a real Boron build today (verified by
disassembly of `PowerManager::refreshInputProfile()` at `0xbd34c`, which
contains the NRF register loads, the `0x7f` enumeration mask, the `cmp #3`
regulator check and the `Logger::warn` call). This closes a latent gap; it is
not an active defect.

### Required change

Add one bit to the `compiledBuildFlags` bitmask, at both emission sites, using
the **next free bit `0x4000`**, derived from the *same* `#if` condition that
gates the override in `PowerManager.cpp` — not from a separate or restated
condition, and not from `PLATFORM_ID` alone.

Semantics must match the existing bits (feature compiled in ⇒ bit set):

- bit set ⇒ the USB source override **is** compiled into this binary
- bit clear ⇒ it is **not**

**Document explicitly, at both call sites and in the CHANGELOG, that a clear
bit is expected and correct on non-Boron platforms** (P2, Argon, MSOM). The bit
is a defect signal only on a Boron. Without that note the witness invites a
false alarm on every non-Boron build.

### Acceptance Criteria (Addendum A)

A1. The new bit is present in both the `Boot:` line and the ledger firmware
    object, and the two bit layouts remain byte-for-byte identical
    (`tests/build_flags_witness_test.sh` fidelity check still passes).

A2. **The bit actually flips.** Extend `tests/build_flags_witness_test.sh` to
    compile the verbatim-extracted expression in a configuration where
    `PLATFORM_BORON` is undefined (or defined to a non-matching value) and
    assert the resulting *executed* value has bit `0x4000` clear, while the
    real Boron configuration has it set. Source presence is not sufficient
    evidence for this criterion.

A3. Verified against the compiled artifact: the witness constant emitted in a
    real default Boron build changes from `0x2008` to `0x6008`, confirmed by
    disassembly of both emission sites (`setup` and
    `Cloud::writeDeviceStatusToCloud`), not by reading source.

A4. Ledger payload still fits: `DEVICE_STATUS_PAYLOAD_CAPACITY` is 896 B and
    observed status payloads are 575–583 B. Report the new size.

### Explicitly NOT in scope for this addendum

**Concern 2 — the duplicated bitmask constant across two translation units.**
It is currently protected against drift by the fidelity check in
`tests/build_flags_witness_test.sh` and confirmed byte-identical. Consolidating
it into a shared `constexpr` in `BuildProfile.h` is worth doing as a follow-up
WO and is deliberately **not** bundled here. Do not refactor it.

### Permitted Files (Addendum A)

Same list as the parent WO. Realistically:
`src/Generalized-Core-Counter.cpp`, `src/cloud/DeviceStatusPublisher.cpp`,
`tests/build_flags_witness_test.sh`, `tests/power_source_override_test.cpp`
(expected-value update `0x2008` → `0x6008`), `CHANGELOG.md`, and this WO.

### Status (Addendum A)

Additive to the existing uncommitted work on `main` @ `01d4685`. Do not revert,
rework or re-verify the parent WO's changes — they are already verified against
the compiled artifact. Full Codex Stage 7 runs on the **combined** diff once
this addendum is in place, not before.
