> **Record note (Claude Code, 2026-10-20):** Codex's `-o` flag overwrote this file with its closing summary. The full verdict below was restored verbatim from the final `apply_patch` of Codex's session log (the 89-line file Codex wrote); only this note was added.

# VERIFIED WITH CONCERNS

**Reviewer:** Codex · **Model:** `gpt-5.6-sol` (standard tier) · **Reasoning:** medium

**Test interpreters:** shell tests via `zsh`; bare Python tests via `python3`; all suites run serially, one suite at a time.

Review target: the uncommitted working-tree implementation on `wo/2026-10-20-001-step6-p6-power-constants`, at documentation `HEAD` `4182a8b`, whose implementation base is main `db98ba6`. Clean `HEAD` and candidate copies were detached worktrees from a local bare clone under the authorized Stage 7 scratch directory, preserving real Git history for the structural tests. Local builds used Device OS 6.4.1, Boron, GCC ARM 10.2.1, fixed per-object random seeds, and a distinct fresh `BUILD_PATH_BASE` for every build.

| Check | Result | Evidence |
|---|---|---|
| 1. Values identical | **PASS** | `src/power/PowerPlatform.h:6-11` defines the six codes as `constexpr int` 0-5, byte-for-byte equal in type/value to all three `db98ba6` sets. `src/power/PmicFaultMonitor.h:7-9` defines the masks as `constexpr uint8_t` 0x30/0x08/0x07, equal to both old sets. Every code comparison remains `int` against `int`; `uint8_t` masks undergo the same integer promotion in `PmicFaultMonitor.cpp` and `SensorManager.cpp:149,166`. Source diff reading found no live log-string or call-order change; the only use-site edits are the two mask-name substitutions. |
| 2. One definition each | **PASS** | Independent source/macro scans over `src/` and `lib/` find exactly one textual definition of every one of the nine names, only at the two headers above; no legacy `PMIC_CHRG_FAULT_MASK` or `PMIC_BAT_FAULT_MASK`, qualified duplicate, or macro remains. Device OS 6.4.1 and repository-library scans found no collision. Namespace-scope `constexpr` has internal linkage under this C++ mode, so there is no linker ODR collision; unqualified lookup, not ADL, finds these globals. A same-translation-unit global with the same name could still be a compile-time redefinition, but none exists. The real/stub `power/PowerPlatform.h` files are alternative include-path resolutions; no translation unit includes both. |
| 3. Dead items | **CONCERN** | Bare, qualified, address-of, macro and constructed-name searches with `git grep`/`rg` find no caller or definition of `availabilityLabel`, `batteryContextLabel` or `hasFuelGauge` in tracked `src/`, `tests/` or `lib/`; the untracked structural test mentions the names only as forbidden tokens. All three are absent from the release ELF. The tracked generated Doxygen pages under `docs/html/` still advertise them, however; see F3. No generated build input or Device OS/library reference was found. Unknown out-of-tree consumers cannot be ruled out, but all six complete local links succeeded. |
| 4. Flag builds | **PASS** | Release (already `ENABLE_PMIC_FORENSICS=1`), `-DENABLE_DIAGNOSTICS_PUBLISH_MODE=1`, and `-DENABLE_PMIC_FORENSICS=0` compiled before and after. The guarded diagnostics code obtains source constants through `PowerDiagnostics.cpp`'s `power/PowerPlatform.h`; PMIC code obtains masks through its own header; `SensorManager.cpp` directly includes both headers. Sizes are tabulated below. |
| 5. Tests pin their claims | **CONCERN** | `tests/power_constants_values_test.cpp:3-17` includes and statically asserts the real headers; its runner puts `src/` before `tests/stubs` (`power_constants_values_test.sh:8-10`). The structural test pins one production home and absence of retired names. But the separate mirror at `tests/stubs/power/PowerSourceCodes.h:3-11` is not cross-checked: my mutation `kPowerSourceUsbOtg = 44` there survived the entire **91/91** suite. Its line 4 also names nonexistent `power_source_codes_values_test.cpp`; the real filename is `power_constants_values_test.cpp`. Removing the three local constants from `power_source_override_test.cpp` did not weaken its production-behavior assertion: my independent mutation changed the production override assignment from `UsbHost` to `UsbAdapter`, and `power_source_override_test.sh` failed at line 97 with exit 134. |
| 6. ELF and retained | **PASS** | Release text/data/bss is `151748/1094/2228 -> 151724/1094/2228` (−24 text). Static-init counts are unchanged: release 22/22, diagnostics 23/23, PMIC-off 22/22. The whole `.backup` section is byte-identical, and no added source line contains `retained`; details below. |
| 7. Budget and suite | **PASS** | `src/` is +14/−75, **net −61** (limit net <=0; hard ceiling +20). Tests are +104/−14, **net +90**, within the +100 guideline. `git diff --check` plus explicit no-index checks for all four untracked files are clean. Full candidate suite: **91/91** (53 shell via `zsh`, then 38 bare Python via `python3`, serial). `report_payload_golden_test.sh` passes; fixture SHA-256 remains `07de1e1980e4a44e03420540e6c721a93a61b5d6392539219928420c048f0637` and the fixture has no diff. |

**Retained outcome: A**

## Build and ELF details

| Profile | Before text/data/bss | After text/data/bss | Delta |
|---|---:|---:|---:|
| Release (`ENABLE_PMIC_FORENSICS=1`) | 151748 / 1094 / 2228 | 151724 / 1094 / 2228 | −24 / 0 / 0 |
| `ENABLE_DIAGNOSTICS_PUBLISH_MODE=1` | 152864 / 1094 / 2476 | 152848 / 1094 / 2476 | −16 / 0 / 0 |
| `ENABLE_PMIC_FORENSICS=0` | 150844 / 1062 / 2228 | 150812 / 1062 / 2228 | −32 / 0 / 0 |

Release named-symbol size changes are: `PowerManager::PowerManager()` 92→84 (−8), `refreshInputProfile()` 572→564 (−8), `setup()` 46→40 (−6), `PowerPlatform::detectCapabilities()` 22→20 (−2), `hasPmic()` 4→18 (+14), and `readPowerSource()` 152→168 (+16). Unique named-symbol coverage therefore changes by +6 bytes. Compiler-local `CSWTCH` labels are renumbered but retain the same size multiset; unnamed literal/alignment/fill coverage falls by 30 bytes, accounting exactly for the net `.text` change of −24.

The complete release `.backup` section is `0x98` bytes at VMA `0x2003f400` before and after. Its extracted bytes have identical SHA-256 `adfe530fcec43aa43fd659f9c9e2e83760bd9a1d217dfb77e715f7ed22d3b0e7`. Every symbol is unchanged:

| Symbol | Address | Size |
|---|---:|---:|
| `link_global_retained_start`, `platform_system_flags_ram_end`, `platform_user_backup_ram_start` | `0x2003f400` | boundary |
| `retainedLoopForensics` | `0x2003f400` | `0x1c` |
| `retainedBoot` | `0x2003f420` | `0x20` |
| `thrashTripCount` | `0x2003f440` | `0x02` |
| `lastThrashReason` | `0x2003f442` | `0x01` |
| `thrashResetCount` | `0x2003f444` | `0x02` |
| `ledOffTime` | `0x2003f448` | `0x08` |
| `lastTrendSoc` | `0x2003f450` | `0x04` |
| `lastTrendVcell` | `0x2003f454` | `0x04` |
| `lastTrendEpoch` | `0x2003f458` | `0x08` |
| `pmicAnomalyCount` | `0x2003f460` | `0x02` |
| `lastPmicAnomalyEpoch` | `0x2003f468` | `0x08` |
| `lastPmicAnomalyUptimeMs` | `0x2003f470` | `0x04` |
| `lastPmicAnomalySoc` | `0x2003f474` | `0x04` |
| `lastPmicAnomalyChargeStatus` | `0x2003f478` | `0x01` |
| `lastPmicAnomalyPowerSource` | `0x2003f479` | `0x01` |
| `lastPmicAnomalyVbusStatus` | `0x2003f47a` | `0x01` |
| `retainedHibernateRtcBefore` | `0x2003f480` | `0x08` |
| `retainedHibernateRequestedSleep` | `0x2003f488` | `0x04` |
| `retainedHibernateCount` | `0x2003f48c` | `0x04` |
| `retainedHibernatePending` | `0x2003f490` | `0x01` |
| `__backup_sram_signature` | `0x2003f494` | `0x04` |
| `link_global_retained_end` | `0x2003f498` | boundary |

Only the `.backup` flash load address follows the text shrink, `0x000d9448 -> 0x000d9430`; its RAM address, contents, size and every symbol remain unchanged.

## Findings

### F1 — Low — test-side source-code mirror can drift silently

- **Observation:** `tests/stubs/power/PowerSourceCodes.h:3-11` duplicates all six values. `tests/power_constants_values_test.cpp:3-17` deliberately includes the real headers and never includes or compares that mirror. In a detached candidate worktree, changing only mirror `kPowerSourceUsbOtg` from 4 to 44 left all 91 tests passing.
- **Inference:** production values are protected, but host tests can silently exercise different values from production for source cases they do not assert. Generate the stub constants from the production header or add a compile-time cross-check that includes both under nonconflicting names.

### F2 — Advisory — stale test comment

- **Observation:** `tests/stubs/power/PowerSourceCodes.h:4` says `power_source_codes_values_test.cpp` pins the values; no such file exists. The actual test is `tests/power_constants_values_test.cpp`.
- **Inference:** this does not alter coverage, but it sends maintainers to the wrong test and obscures F1's distinction between real-header and mirror coverage.

### F3 — Low — generated API documentation is stale

- **Observation:** tracked generated pages such as `docs/html/_power_diagnostics_8h.html:105-106`, `docs/html/_power_platform_8cpp.html:112`, and `docs/html/struct_power_capabilities.html:106` still advertise the deleted labels/field. `Doxyfile:12,45` builds HTML from `src`, README, changelog and docs.
- **Inference:** these pages are not compiled callers and do not affect firmware behavior, but the checked-in API documentation remains misleading until regenerated. This review was not authorized to modify it.

## Budget versus actual

| Budget | Actual | Result |
|---|---|---|
| `src/` net <=0 (hard ceiling +20); tests +100 guideline | `src/` +14/−75 = **−61**; tests +104/−14 = **+90** | **PASS** — source 61 lines below zero; tests 10 lines inside guideline |

## Confirmations

- The implementation files in the review worktree were compared byte-for-byte with the detached candidate snapshot taken before review activity; they match. Status remained the original 18 modified and four untracked implementation/test files, plus only this authorized verdict.
- All builds, mutations, extracted sections, local Git clone/worktrees and logs were confined to `build-tmp/wo20261020-001-stage7/` or `$TMPDIR`. Only review-created scratch files are removed at handoff; no pre-existing scratch material is removed.
- No commit, push, stash, reset, checkout/restore, device access, network access or AWS access occurred.
- `PowerCapabilities::hasFuelGauge` was at `PowerManager.h:62` and written at `PowerPlatform.cpp:123,131` on `db98ba6`; per the binding note, it should return with its first reader in the P2/M-SoM portability work rather than remain write-only now.
