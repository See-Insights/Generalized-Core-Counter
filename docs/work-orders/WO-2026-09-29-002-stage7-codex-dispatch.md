AGENT: Codex · MODEL: gpt-6-astra · REASONING: high
AUTHORIZATION SCOPE: review the uncommitted diff and the four cumulative step patches; build in scratch copies (apply each patch to a clean export of `c9c43b0`); run the host suite; temporarily mutate files for checks, restoring each byte-identically by rewriting in place / Not authorized: lasting edits, commits, pushes, stash, reset, checkout, flashing, device settings, network or AWS access.

# Stage 7 dispatch (narrow) — WO-2026-09-29-002 (configuration cleanup)

**Goal, in plain language:** configuration lives in a few clear places, and the clutter is gone. The firmware behaves exactly as before, except for the one intentional fix in step 4.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-09-29-002-config-cleanup`, base `c9c43b0` (the WO documents on top of `main` at `6e7a3e1`, v28), with the Stage 6 change as an uncommitted working-tree diff. **Binding spec:** `docs/work-orders/WO-2026-09-29-002-config-cleanup.md`, including its "Binary checks: method (validated 2026-09-30)" section, and with the `BLUE_LED` correction (it's live and must stay). Stage 6 report: `WO-2026-09-29-002-stage6-copilot-report.md`. Cumulative step patches: `build-tmp/wo-2026-09-29-002/step-{1,2,3,4}.cumulative.patch`. Baseline: `build-tmp/wo-2026-09-29-002-baseline/` (loadable-section SHA-256 `ad322c5d…bf3d`).

This is a narrow review: check **each step against its own binary check** and the WO's scope for that step, plus step 4's test. Do not widen the fault model or propose new mechanisms.

## Checks (PASS/FAIL with evidence for each)

1. **Patches:** each `step-N.cumulative.patch` applies cleanly to a clean export of `c9c43b0`, and `step-4` reproduces the working tree's `src/`, `tests/` and root-file changes exactly.
2. **Step 1 (delete clutter):** only the four dead settings are gone (`DEBUG_SERIAL`, the extra `FIRMWARE_RELEASE_NOTES` declarations, `ProjectConfig::webhookEventName()`, `CONNECT_BUDGET_DEFAULT_SEC`); `BLUE_LED` remains; `Settings.h`, `ProjectConfig.h` and `cloud/ConfigMerge.cpp` are deleted; `Config.h` uses `ConnectivityPolicy`'s connection defaults. Net lines negative. **Binary:** loadable sections byte-identical to the baseline, using the WO's method (from-scratch build, `'EXTRA_CFLAGS=-frandom-seed=$$@'`, the listed sections).
3. **Step 2 (build profile):**
   - `SERIAL_LOG_LEVEL` is under `#ifndef` in `BuildProfile.h`, with an `#error` for unsupported values, and the eight category filters are moved there.
   - There is one build-flags word definition, used by both sites; the retired bits stay reserved (not renumbered).
   - `ENABLE_PMIC_TRACE`, `ENABLE_PMIC_CHARGE_CYCLE_TEST` and the `ENABLE_PMIC_REGISTER_DUMP` comment are retired; `MUON_HAS_TMP112` tests its value.
   - The release/bench selector exists: release is the default, bench enables the bench diagnostics, and bench does **not** enable `ENABLE_RTC_SKEW_TEST`. `project.properties` platform is corrected.
   - Net lines negative. **Binary:** byte-identical to step 1, or different only in the build-flags word; explain any other difference. **Bench build:** `pdiag` and `PowerDiagnostics::flushDiagBatch` present, RTC-skew hook absent. Also show that `-DSERIAL_LOG_LEVEL=<n>` now overrides the level.
4. **Step 3 (version in one place):**
   - `FirmwareVersion.h` holds the number and the string; `Version.h` and `Version.cpp` are deleted.
   - The release notes are in `CHANGELOG.md`.
   - `release.sh` stages the right file, and `bump_version.sh` derives `28` from `v28-CloseBeforeSleep` (show a dry run or extraction); `README.md` and `Doxyfile` are updated.
   - Net lines negative. **Binary:** the WO's step-3 check: `nm` names and sizes identical apart from the notes; size difference = string length plus alignment; disassembly identical apart from addresses; version string and number unchanged.
5. **Step 4 (the one behavior fix):** one webhook-timeout range, 5000–120000 ms, defined once and used by both `ConfigApply.cpp` and the main file; about 5 `src/` lines (6 reported). The new test shows an accepted value is the value used (4999, 5000, 120000, 120001); mutating the range back to `1000, 60000` fails it. **Binary:** differences confined to the timeout-range sites.
6. **Everywhere:** the suite, every `tests/*.sh` with **zsh** (never bash) plus every bare `tests/*.py` with python3, `N/N` (expected 46/46); `tests/publish_with_ack_structural_test.py` unchanged and green; only tests that pin changed text were edited (list them); a normal local release build's text/data/bss (expected 150164 / 1090 / 2196) with `strings` showing `v28-CloseBeforeSleep` and no `pdiag`.

## Verdict (required)

VERIFIED, NOT VERIFIED, or VERIFIED WITH NOTES, per step and overall, with evidence (SHA-256s of each step's loadable sections); the model and reasoning level actually used. Confirm the working tree is byte-identical to how you found it.
