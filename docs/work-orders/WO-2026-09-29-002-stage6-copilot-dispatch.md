AGENT: Copilot · MODEL: claude-opus-5 · REASONING: medium
AUTHORIZATION SCOPE: edit and delete the `src/` files named in the WO's four steps, `project.properties` (the platform line only), `release.sh`, `bump_version.sh`, `README.md`, `Doxyfile` and `CHANGELOG.md`; update tests only where they pin text this WO changes, and add step 4's test; run the host suite and the reproducible builds described below / Not authorized: commits, pushes, merges, branch changes, stash, reset, flashing, device settings, AWS access, any edit to `lib/` or `docs/`, and any change beyond this WO.
**SIZE RULE: every step reduces the net line count (step 4: at most about 5 `src/` lines plus its test). A step that would add net lines means STOP and report, not continue.**

# Stage 6 dispatch — WO-2026-09-29-002 (configuration cleanup)

**Goal, in plain language:** configuration lives in a few clear places, and the clutter is gone. The firmware behaves exactly as before, except for the one intentional fix in step 4.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-09-29-002-config-cleanup`, `HEAD` `c9c43b0` (the WO documents on top of `main` at `6e7a3e1`, v28-CloseBeforeSleep). Do not switch branches.
**Binding spec:** `docs/work-orders/WO-2026-09-29-002-config-cleanup.md`, with Codex's inventory `docs/work-orders/2026-09-29-config-inventory-codex-report.md` as the reference for every setting. **Where they disagree, the WO wins.** In particular, **`BLUE_LED` is live: do not remove it.**

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Work only from the WO. Any correction to the spec, even an obviously right one, is reported as a deviation. Leave an uncommitted working-tree diff. Temporary artifacts: visible, descriptive names under `build-tmp/wo-2026-09-29-002/` (gitignored), removed at the end **except** the per-step records listed below.

## Doing the four steps (one commit each, which Chip makes)

Implement **step 1, then 2, then 3, then 4**, in order, in the same working tree. After finishing each step N:
1. Save the **cumulative** diff (`git diff` plus the new or deleted files, e.g. `git add -N` for new files, then `git diff`) to `build-tmp/wo-2026-09-29-002/step-N.cumulative.patch`.
2. Run that step's binary check (below) and the host suite, and record the results before starting the next step.

Chip will turn the four cumulative patches into four commits.

## The steps

Implement exactly what the WO says for each step:
- **Step 1, delete clutter:** four dead settings (`DEBUG_SERIAL`; the extra `FIRMWARE_RELEASE_NOTES` declarations; `ProjectConfig::webhookEventName()` via deleting `ProjectConfig.h`; `CONNECT_BUDGET_DEFAULT_SEC`). Delete `Settings.h`, `ProjectConfig.h` and `cloud/ConfigMerge.cpp`. `Config.h` uses `ConnectivityPolicy`'s connection defaults, including `ConnectivityPolicy.h` in place of `Settings.h`.
- **Step 2, build profile:** `SERIAL_LOG_LEVEL` (under `#ifndef`, with an `#error` for unsupported values) and the eight category filters move into `BuildProfile.h`. One build-flags word definition, used by both current sites. Retire `ENABLE_PMIC_TRACE`, `ENABLE_PMIC_CHARGE_CYCLE_TEST` and the `ENABLE_PMIC_REGISTER_DUMP` comment, **keeping their bit positions reserved (no renumbering)**. `MUON_HAS_TMP112` tests its value. A release/bench profile selector (release is the default; bench enables the bench diagnostics; **bench must not enable `ENABLE_RTC_SKEW_TEST`**). `project.properties` `platform=p2` fixed to `boron`. Tests pinning the build-flags expression or its location may be updated; report each.
- **Step 3, version in one place:** as the WO says. `FirmwareVersion.h` holds the number and the string. The current release-notes text becomes the v28 entry in `CHANGELOG.md`. Delete `Version.h` and `Version.cpp`. Fix `release.sh` and `bump_version.sh` (the latter must derive `28` from `v28-CloseBeforeSleep`). Update `README.md:11` and `Doxyfile:5` to the current version.
- **Step 4, one behavior fix:** a single webhook-timeout range, **5000–120000 ms** (Chip, 2026-09-30), defined once with its owner and used by both `ConfigApply.cpp` and `Generalized-Core-Counter.cpp`. At most about 5 `src/` lines, plus a test that a value that's accepted is also the value used (for example, that the configuration check and the runtime check agree at 4999, 5000, 120000 and 120001).

## Binary checks (the WO's validated method; run after each step)

Build from scratch in a **fresh** object directory each time:

```
export PATH=~/.particle/toolchains/buildtools/1.1.1/bin:~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH
cd ~/.particle/toolchains/deviceOS/6.4.1/main
make -s PLATFORM=boron APPDIR=<a copy of the working tree> TARGET_DIR=<out> \
  DEVICE_OS_PATH=/Users/chipmc/.particle/toolchains/deviceOS/6.4.1 BUILD_PATH_BASE=<fresh obj dir> \
  'EXTRA_CFLAGS=-frandom-seed=$$@'
```

Extract the loadable sections `.module_info .dynalib .text .ARM.exidx .data .backup .module_info_product` with `arm-none-eabi-objcopy -O binary -j <section>`, concatenate them in that order, and compare with the baseline `build-tmp/wo-2026-09-29-002-baseline/v28-baseline-content.bin` (SHA-256 `ad322c5d…bf3d`), or with the previous step's result.
- **Step 1:** byte-identical to the baseline.
- **Step 2:** byte-identical, or different only in the build-flags word. Explain any other difference, and stop if it isn't explainable.
- **Step 3:** the WO's step-3 check: `nm` names and sizes identical apart from the release notes; size difference = string length plus alignment; disassembly identical apart from addresses; version string and number unchanged.
- **Step 4:** identify the differences; they should be confined to the timeout range.

Also, **bench profile (step 2 onward):** a bench-profile build contains `pdiag` and `PowerDiagnostics::flushDiagBatch`, and not the RTC-skew hook.

## Verification (report all)

- Per step: the net line change (must be negative; step 4 ≤ about 5 `src/` lines plus its test), the binary-check result with SHA-256s, and suite `N/N (sh via zsh, py via python3)` (current 45/45). `tests/publish_with_ack_structural_test.py` must pass unchanged.
- At the end: a normal local release build (`make … clean-user` then `compile-user`, or equivalent) with text/data/bss against v28's 150308 / 1090 / 2196, and `strings` showing the unchanged version string, with no `pdiag`.

## Implementation Report (required)

Per step: files and lines changed, the net line count, the tests changed and why, the binary-check evidence, and the patch file. Then: files shared between steps, deviations (or "none"), and the model and reasoning level actually used.
