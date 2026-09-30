# WO-2026-09-29-002: configuration cleanup

**Goal, in plain language:** configuration lives in a few clear places, and the clutter is gone. The firmware behaves exactly as before, except for the one intentional fix in step 4.

**Status:** Opened 2026-09-29 (Chip). Stage 5 scope set in the opening instruction; the step-4 range was decided 2026-09-30. Branch rebased onto `main` at `6e7a3e1` (v28-CloseBeforeSleep). Stage 7 VERIFIED WITH NOTES (2026-09-30); at USER GATE 2 (Stage 8: four commits).

**Workflow:** Per `AI_DEVELOPMENT_WORKFLOW.md`, including §12: history first, a size budget, the two-round rule, and verify the binary, not the source.

**Spec:** Codex's read-only inventory, `docs/work-orders/2026-09-29-config-inventory-codex-report.md` (`gpt-6-astra`, reasoning high, at `a6a283c`, v27). It lists every setting with file:line, value, consumers, build variation, kind and status. Where this WO names a setting, the inventory's line references are authoritative as of `a6a283c`; re-confirm them on the new base.

**Branch:** `wo/2026-09-29-002-config-cleanup`, rebased onto `main` at `6e7a3e1` (v28). The inventory was taken at v27 (`a6a283c`); every reference below was re-checked on v28 (2026-09-30) and still holds, since v28 didn't touch these files. **Four steps, one commit each, in order.**

**Size rule:** every step reduces the line count. **Any step that would add net lines is stop-and-report.**

## Step 1: delete clutter

- **Remove the dead settings** (four; see the `BLUE_LED` correction):
  - `DEBUG_SERIAL` (`BuildProfile.h:253–256`; defined in DEV builds, never read);
  - the extra `FIRMWARE_RELEASE_NOTES` declarations (`Generalized-Core-Counter.cpp:82–83`, `DeviceStatusPublisher.cpp:38`); it is never read. Its definition goes in step 3;
  - `ProjectConfig::webhookEventName()` (deleted with `ProjectConfig.h`; it is not the runtime fallback, which is `Cloud::getWebhookName()`);
  - `CONNECT_BUDGET_DEFAULT_SEC` (`ConnectivityPolicy.h:64`, never read);
  - ~~`BLUE_LED`~~ **Correction (Claude Code, 2026-09-30, on re-checking against v28): `BLUE_LED` is live, not dead.** `device_pinout.cpp:86,98,119,142` uses it: it's the on-module LED that `signalLED()` drives. The inventory missed those reads. **Do not remove it.** Step 1 removes four dead settings, not five.
- **Delete three files:** `src/Settings.h`, `src/ProjectConfig.h`, and the empty `src/cloud/ConfigMerge.cpp` (it contains only an `#include`).
  - `Settings.h` is how `Config.h` reaches `BuildProfile.h` (and `ProjectConfig.h`). Any file that relied on that chain must include `BuildProfile.h` directly.
  - Remove the `#include "ProjectConfig.h"` in `Generalized-Core-Counter.cpp:79`.
  - Move any useful catalog text from `Settings.h` into documentation, not into code.
- **Include path:** `ConnectivityPolicy.h` includes `BuildProfile.h` itself, so when `Config.h` includes `ConnectivityPolicy.h` in place of `Settings.h`, the build profile is still reached.
- **One source for connection defaults:** `Config.h` uses `ConnectivityPolicy`'s connection defaults instead of repeating them. These are the 300 s connect budget (`Config.h:35` vs `ConnectivityPolicy.h:62`) and the 15 s / 30 s disconnect budgets (`Config.h:36–37` vs `ConnectivityPolicy.h:194–195`).
- **Check:** the release binary is **byte-identical** to before.

## Step 2: the build profile

- **Logging policy into `BuildProfile.h`:** move `SERIAL_LOG_LEVEL` from `Particle_Functions.cpp:12`, wrapped in `#ifndef` so a build flag can override it, together with the eight category log filters (`Particle_Functions.cpp:22–31`: `mux`, `system.nm`, `system`, `comm.dtls`, `comm.protocol`, `comm.protocol.handshake`, `net.pppncp` and `app.ab1805`, all WARN). `Particle_Functions` keeps the system mode, reset-info and function registration. Add a validation `#error` for an unsupported level; today the level switch has no final `#else`.
- **One build-flags word:** define the build-flags word once. Today `Generalized-Core-Counter.cpp:1477–1503` and `DeviceStatusPublisher.cpp:219–246` build the same bitmask independently.
- **Retire three misleading switches:** `ENABLE_PMIC_TRACE` and `ENABLE_PMIC_CHARGE_CYCLE_TEST` (their features are gone; each only sets a bit in the build-flags word), and the `ENABLE_PMIC_REGISTER_DUMP` mention in `BuildProfile.h:29`, which is a comment only.
- **`MUON_HAS_TMP112` tests its value** (`#if MUON_HAS_TMP112`), not just whether it's defined. Today `-DMUON_HAS_TMP112=0` still enables the path (`SensorManager.cpp:996`).
- **Add a clear release/bench profile selector.**
  - Release is the default.
  - Bench turns on the bench diagnostics (at least `ENABLE_DIAGNOSTICS_PUBLISH_MODE`).
  - **The bench profile must not turn on the RTC-skew hook** (`ENABLE_RTC_SKEW_TEST`); it stays an explicit, separate flag.
  - Individual flags can still be overridden with `-D`.
- **Fix `project.properties`:** correct the stale `platform=p2` (`project.properties:4`). Leave the Boron/M-SoM build selection to Step 6.
- **Tests:** two tests pin the build-flags word and will change with the retired bits: `tests/build_flags_witness_test.sh` and `tests/power_source_override_test.cpp:294`. Update them to the new values; no other test changes.
- **Check:** the release binary is identical **except the build-flags word**, and a bench build still turns on its diagnostics (for example, `pdiag`/`flushDiagBatch` present). Retiring the two switches frees two bits:
  - `ENABLE_PMIC_TRACE`: its bit is `0` in the current release word, so the word's value is unchanged unless the bits are renumbered.
  - `ENABLE_PMIC_CHARGE_CYCLE_TEST`: its bit is `0` in the current release word, with the same effect.

  Stage 6 must report the exact release-word values before and after (v27 release: `0x4008` on Boron), and whether any bit was renumbered. **Renumbering is out of scope:** retired bits stay reserved, so the fleet's existing readers of the word keep working.

## Step 3: the version in one place

- `FirmwareVersion.h` holds both the number (`FIRMWARE_PRODUCT_VERSION`) and the string (`FIRMWARE_VERSION`).
- Release notes move to `CHANGELOG.md`, out of the binary. Carry the current text over as the v27 entry.
- Delete `src/Version.h` and `src/Version.cpp`. Point their consumers (`Generalized-Core-Counter.cpp`, `DeviceStatusPublisher.cpp`, `StartupSnapshotRuntime.cpp`) at `FirmwareVersion.h`, and remove the extra `extern` declarations.
- **Fix `release.sh`:** it stages `Version.cpp` (`release.sh:167`); it must stage `FirmwareVersion.h` instead.
- **Fix `bump_version.sh`:** it edits five locations (`:70–82`) and can't parse names like `v27-SmallFixes` into the number 27 (`:26`); it must handle that naming format.
- Update `README.md:11` and `Doxyfile:5`, which still say `20.1-PowerMgt`.
- **Check:** the release binary is identical **except that the release-notes text is gone**; the version string and number are unchanged.

## Step 4: one behavior fix (the only intended behavior change)

**The webhook timeout accepts one range, defined once, and used by both `ConfigApply.cpp` and the main file.**

- Today the Ledger path accepts 1000–60000 ms (`ConfigApply.cpp:575`), while the main file accepts only 5000–120000 ms, with a 20000 ms fallback (`Generalized-Core-Counter.cpp:1695–1700`). So a configured value can be accepted and then silently replaced.
- The single range is defined once, with its owner, and used by both.
- **Budget: at most about 5 lines of `src/`**, plus **a test that a value that's accepted is also the value used**.
- **Range decided (Chip, 2026-09-30): 5000–120000 ms**, the range the device actually uses, so nothing deployed changes meaning. **Read-only check (Claude Code, 2026-09-30):** no device has a `reporting.webhook.timeoutMs` override; every device inherits the product default of 20000 ms, which is inside the range. Tightening the configuration-time check therefore affects no deployed setting.

## Binary checks: method (validated 2026-09-30)

Ordinary builds are not byte-identical: Codex's v27 Stage 7 found differing random LTO identifiers. The validated method:
1. **Build from scratch** with the Device OS 6.4.1 toolchain directly (`cd ~/.particle/toolchains/deviceOS/6.4.1/main`, then `make -s PLATFORM=boron APPDIR=<tree> TARGET_DIR=<out> DEVICE_OS_PATH=... BUILD_PATH_BASE=<fresh obj dir>`), with **`'EXTRA_CFLAGS=-frandom-seed=$$@'`**. The escaped `$$@` gives every object file its own fixed seed; an unescaped `$@` expands too early and fails.
2. **Compare the loadable sections only:** `.module_info`, `.dynalib`, `.text`, `.ARM.exidx`, `.data`, `.backup` and `.module_info_product`, extracted with `arm-none-eabi-objcopy -O binary -j <section>` and concatenated in that order. Three fields always differ between builds, because they are hashes over the whole image and its debug info, which includes the build directory's path: `.note.gnu.build-id`, the SHA-256 in `.module_info_suffix`, and `.module_info_crc`. Exclude them.

**Validation:** two from-scratch v28 builds of `6e7a3e1`, in different directories, gave byte-identical loadable sections: SHA-256 `ad322c5dd8517b78092fdaff95a581b8326382acac4c5183dc03817a4625bf3d`, 151322 bytes. The whole `.bin` differed in exactly those three hash fields (55 bytes). That's the baseline: `build-tmp/wo-2026-09-29-002-baseline/` (`v28-baseline-content.bin`, `.sha256`, `v28-baseline.elf`).

**Per-step expectations:**
- **Step 1:** loadable sections byte-identical to the baseline.
- **Step 2:** byte-identical, or different only in the build-flags word. The retired bits are 0 in the release word, and the word is a compile-time constant, so a single definition should compile to the same constant.
- **Step 3:** the release-notes string is in the v28 binary today (`strings` finds it, though `nm` shows no `FIRMWARE_RELEASE_NOTES` symbol). Removing it shrinks read-only data and shifts later addresses, so a byte comparison can't pass. The check is: the `nm` symbol names and sizes are identical apart from the notes; the size difference equals the string length plus alignment; and the disassembly is identical apart from addresses. The version string and number are unchanged.
- **Step 4:** the only intended code change; identify its differences.

## Out of scope (recorded here)

- **Duplicated power-source codes** (six codes copied in `PowerManager.cpp`, `PowerPlatform.cpp` and `PowerDiagnostics.cpp`) **and PMIC fault masks** (`SensorManager.cpp` and `PmicFaultMonitor.cpp`): Step 6, the power-owner split.
- **Boron versus M-SoM build selection:** Step 6. See the 2026-09-29 M-SoM portability check: it compiles cleanly, but PMIC, battery and pins need M-SoM implementations.
- **The seven Ledger fields with no operational consumer** (`messaging.verboseTimeoutMin`, `sensor.type`, `sensor.setting3`, `sensor.setting4`, `modes.reportingMode`, `modes.samplingMode`, `reporting.webhook.enabled`): a separate decision, because they are part of the cloud schema.
- **Other inventory items not in these four steps:** listed in Claude Code's report back, 2026-09-29, for a later decision.

## Routing (tomorrow)

- **Stage 6:** one Copilot round (`claude-opus-5`, reasoning medium) covering all four steps as separate commits' worth of changes, kept file-separable, with one report per step: line counts (net negative), the binary check result, and any stop-and-report.
- **Stage 7:** one narrow Codex review (`gpt-6-astra`, reasoning high) that checks **each step against its own binary check**, plus step 4's test, and the suite (sh via zsh, py via python3) with the `WITH_ACK` structural test.
- **Two rounds without VERIFIED means stop and restate the goal** (§12, guardrail 4).

## Approval record

- [x] Scope (Stage 5): Chip, 2026-09-29, in the opening instruction (four steps, one commit each, net-negative lines per step, the binary checks, the out-of-scope list, and the routing). 2026-09-30: step 4 keeps **5000–120000 ms** (no device overrides the 20000 ms default); branch rebased onto v28; `BLUE_LED` removed from the dead list (it's live); binary-check method validated.
- [x] Implementation (Stage 6) — 2026-09-30, Copilot `claude-opus-5`, reasoning medium. **Step 1** −96 net lines, loadable sections byte-identical to the baseline (`ad322c5d…`). **Step 2** −7 (first measured +6 and trimmed before acceptance), byte-identical; a bench-profile build has `pdiag` and no RTC-skew hook. **Step 3** −51; the image is 144 bytes smaller, exactly the release-notes string; `nm` names and sizes identical; no instruction differs apart from addresses. **Step 4** +6 `src/` lines (the `validateRange` call wraps to two lines) plus `tests/webhook_timeout_range_test.py`, mutation-checked; its differences are confined to the `validateRange` site. Final release build 150164 / 1090 / 2196 (−144 text), `v28-CloseBeforeSleep`, no `pdiag`. Suite **46/46 (sh via zsh, py via python3)**, re-run by Claude Code. Six deviations reported and accepted as reasonable: the CHANGELOG entry is v28 (the dispatch's instruction); `DeviceStatusPublisher.cpp:38` is a `FIRMWARE_VERSION` extern, handled in step 3; no `Settings.h` catalog text needed moving; a 1-line `device_pinout.h` comment fix so it doesn't point at the deleted `Settings.h`; step 2's trim; step 4 at 6 lines. Cumulative patches `build-tmp/wo-2026-09-29-002/step-{1..4}.cumulative.patch`, each applying cleanly to `c9c43b0`. Report: `WO-2026-09-29-002-stage6-copilot-report.md`.
- [x] Codex verification, narrow (Stage 7) — 2026-09-30, `gpt-6-astra`, reasoning high: **VERIFIED WITH NOTES**; every step passes. Patches apply to clean exports of `c9c43b0`, and step 4 reproduces the working tree exactly. Loadable-section SHA-256 (fresh builds, the WO's seeded method): step 1 `ad322c5d…bf3d` and step 2 `ad322c5d…bf3d`, both identical to the v28 baseline (the release flags word stays `0x4008`); step 3 `a37a5d07…738b`, where the `nm` names and sizes are identical, all 40,972 instructions match after address normalization, and the reduction is 148 bytes of string minus 4 bytes of padding = 144 bytes; step 4 `1673f72e…e2b6`, with changes only in `Cloud::applyReportingConfig` (symbol 0x1e0 → 0x1e4). Bench build: `pdiag` and `flushDiagBatch` present, RTC skew absent (151272 / 1090 / 2444). `-DSERIAL_LOG_LEVEL=0..4` selects the handler; −1 and 5 hit `#error`. `bump_version.sh` extraction returns 28. Step 4's test catches the old range. Suite 46/46 (sh via zsh, py via python3); `WITH_ACK` and build-flags witness tests unchanged and green; the only existing-test edits remove duplicate `FIRMWARE_VERSION` definitions in two stubs. Release 150164 / 1090 / 2196. Notes: Copilot's report misattributes some changes between steps, though the patches follow the specified order; a parallel build hit a make dependency-order error, so all results are from serial builds. Working tree byte-identical before and after. Verdict: `WO-2026-09-29-002-stage7-verdict.md`. **At USER GATE 2.**
- [ ] Chip final gate / commits (Stage 8)
