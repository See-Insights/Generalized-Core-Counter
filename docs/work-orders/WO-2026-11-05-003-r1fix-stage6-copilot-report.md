# WO-2026-11-05-003 Amendment 4 - Stage 6 follow-up (round 1 fixes), report

AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: not observable from inside the session; the harness did not report a level, so I cannot confirm the dispatched medium (treat as possibly lower). · python3 3.14.6 (.py), zsh (.sh). Nothing committed, pushed, merged or flashed. Bench not run.

## Diff summary
- `src/Generalized-Core-Counter.cpp`: fleet config gains `.clockSource(particle::RtcClockSource::EXTERNAL)` (appended to the existing `.capabilities(...)` line).
- `src/time/ExternalRtc.h`: `sameFleetConfig` also compares `clockSource()`; the comment line updated. A stored internal-RC configuration is now a mismatch, so `ensureBound` re-binds with the logged warning.
- `tests/stubs/external_rtc/Particle.h`: `RtcClockSource` enum and `clockSource()` getter/setter on the fake configuration.
- `tests/external_rtc_test.cpp`: `fleet()` sets EXTERNAL; new `ensureBound` case (stored INTERNAL -> one re-bind, one warning, stored becomes EXTERNAL); configs differing only in clock source are unequal in both orders.
- `tests/external_rtc_structural_test.py`: structural check that the fleet config has `.clockSource(...EXTERNAL)`; 3 new mutations.
- `docs/FIELD_MEANINGS_REFERENCE.md`: "schema 2" -> "schema 3" in the DATA and STATUS byte ranges and the "typically 3" line (lines 205-207).
- `docs/contracts/ledger-contracts.md`: checked, no change needed (lines 101, 108, 199, 212 already say schema 3; the only other "2" hits are the generic evolution rules and the Schema Version History).

## Net `src/` lines
This change: **+0** (no line added or removed; both edits extend existing lines). S1 total `git diff --numstat 06aacd9 -- src`: **+19**, within +21. Device OS 6.6.0 `RtcClockSource::EXTERNAL` and `clockSource()` exist (`wiring/inc/spark_wiring_exrtc.h:73-77`).

## Tests
Host suite: **116/116** (`*_test.sh` via zsh, `*_test.py` via python3, run in the checkout). `thermal_coupling_structural_test.py` also passes on a clean export of tracked files.
Mutations (applied to temp copies of the sources, so the working files were never touched; no backup/restore needed), all caught: clock source not compared; compare against the wrong value (`== INTERNAL`); `.clockSource(EXTERNAL)` removed from the fleet config (structural). All earlier mutations still caught.

## Build and nm
Invocation: Device OS 6.6.0, Boron, gcc-arm 10.2.1, buildscripts 1.17.2, `DEVICE_OS_PATH=$T/deviceOS/6.6.0 APPDIR=$PWD PLATFORM=boron DEVICE_OS_VERSION=6.6.0 GCC_ARM_PATH=... make -f $T/buildscripts/1.17.2/Makefile compile-user -s BUILD_PATH_BASE=$PWD/build-tmp/wo-2026-11-05-003-r1fix/bpb` (fresh base passed as a make argument). Head = `3261210` plus this uncommitted change.
Result: text/data/bss **152708 / 1114 / 2240** (Amendment 3 build: 152692, so +16 text).
`nm`: `ExRtcT<Am18x5Configuration>::enable`, `hal_exrtc_bind`, `hal_rtc_get_time`, `hal_rtc_set_time` present. `objdump` of `setup()`: `Am18x5Rtc::instance()`, `Am18x5Configuration()`, `getConfig`, and `enable` at two sites (unbound and mismatch): linkage unchanged.

## Corrections to the Stage 6 report
(a) The Stage 6 inventory omitted four comment-only files:
- `src/BuildProfile.h`: "immediately after ab1805.setup()" -> "immediately after the bind".
- `src/MyPersistentData.h`: "ab1805.setup() seeds Time, so that clamp can never fire" -> "Device OS seeds Time only on a bound boot, so that clamp can miss the first boot".
- `src/persist/CurrentReadings.h`: same rewording ("can miss the first boot").
- `src/time/ClockTrust.h`: two `ab1805.setup()` mentions -> "Device OS sets it from the RTC before setup()" / "Device OS had seeded system time".
(Exact wording is in `git diff 06aacd9 -- <file>`.)

(b) The "v49.1 baseline 151732 / 1114 / 2200" in the Stage 6 report is not the tagged build. Codex's measurements under the same Device OS/toolchain invocation: tag `v49.1` (`9a96854`) **152692 / 1114 / 2232**; integration base `06aacd9` **153100 / 1114 / 2240**; S1 `ce2e597` **152604 / 1114 / 2240**. I reproduced one: the S1 head with this change, invocation above, **152708 / 1114 / 2240**. I did not rebuild the other commits.

## Deviations
- Reasoning level unverifiable (above).
- The build writes its ELF to the git-ignored `target/6.6.0/boron/` as before; I did not delete anything there.
- Findings 1 and 2 are not addressed (carried to S1b by the architect).
