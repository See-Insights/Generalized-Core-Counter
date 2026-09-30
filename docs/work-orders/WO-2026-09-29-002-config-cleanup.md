# WO-2026-09-29-002: configuration cleanup

**Goal, in plain language:** configuration lives in a few clear places, and the clutter is gone. The firmware behaves exactly as before, except for the one intentional fix in step 4.

**Status:** Opened 2026-09-29 (Chip). Stage 5 scope set in the opening instruction. Written tonight as a document only, left untracked; committed tomorrow on a new branch from `main` once v27 (WO-2026-09-29-001) has merged. Stage 6 not started.

**Workflow:** Per `AI_DEVELOPMENT_WORKFLOW.md`, including §12: history first, a size budget, the two-round rule, and verify the binary, not the source.

**Spec:** Codex's read-only inventory, `docs/work-orders/2026-09-29-config-inventory-codex-report.md` (`gpt-6-astra`, reasoning high, at `a6a283c`, v27). It lists every setting with file:line, value, consumers, build variation, kind and status. Where this WO names a setting, the inventory's line references are authoritative as of `a6a283c`; re-confirm them on the new base.

**Branch (tomorrow):** a new branch from `main` after v27 merges. **Four steps, one commit each, in order.**

**Size rule:** every step reduces the line count. **Any step that would add net lines is stop-and-report.**

## Step 1: delete clutter

- **Remove the 5 dead settings:**
  - `DEBUG_SERIAL` (`BuildProfile.h:253–256`; defined in DEV builds, never read);
  - the extra `FIRMWARE_RELEASE_NOTES` declarations (`Generalized-Core-Counter.cpp:82–83`, `DeviceStatusPublisher.cpp:38`); it is never read. Its definition goes in step 3;
  - `ProjectConfig::webhookEventName()` (deleted with `ProjectConfig.h`; it is not the runtime fallback, which is `Cloud::getWebhookName()`);
  - `CONNECT_BUDGET_DEFAULT_SEC` (`ConnectivityPolicy.h:64`, never read);
  - `BLUE_LED` (`device_pinout.cpp:43` and its declaration, never read).
- **Delete three files:** `src/Settings.h`, `src/ProjectConfig.h`, and the empty `src/cloud/ConfigMerge.cpp` (it contains only an `#include`).
  - `Settings.h` is how `Config.h` reaches `BuildProfile.h` (and `ProjectConfig.h`). Any file that relied on that chain must include `BuildProfile.h` directly.
  - Remove the `#include "ProjectConfig.h"` in `Generalized-Core-Counter.cpp:79`.
  - Move any useful catalog text from `Settings.h` into documentation, not into code.
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
- Which range to keep is a Stage 5 detail. The recommendation, to confirm with Chip before dispatch, is the runtime range 5000–120000 ms, because that is what the device actually uses today.

## Binary checks: method

Codex's v27 Stage 7 found that **ordinary builds contain differing random LTO identifiers**, so two builds of the same source are not byte-identical by default. Each step's binary check must therefore:
- use a method that removes that noise. Codex used fixed per-file GCC random seeds (`-frandom-seed`); alternatively, compare section contents excluding the LTO identifiers;
- apply the same method to the before and after builds;
- build after `make clean-user` (`AI_DEVELOPMENT_WORKFLOW.md` §2).

State the method in the Stage 6 report and the Stage 7 verdict.

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

- [x] Scope (Stage 5): Chip, 2026-09-29, in the opening instruction (four steps, one commit each, net-negative lines per step, the binary checks, the out-of-scope list, and the routing). Open before dispatch: which webhook-timeout range step 4 keeps.
- [ ] Implementation (Stage 6)
- [ ] Codex verification, narrow (Stage 7)
- [ ] Chip final gate / commits (Stage 8)
