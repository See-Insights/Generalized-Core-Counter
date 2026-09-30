**Overall: VERIFIED WITH NOTES.** All requested checks passed. Model used: **gpt-6-astra, reasoning high**.

| Check | Verdict | Evidence |
|---|---|---|
| Patches | **PASS — VERIFIED** | All four apply cleanly to separate clean exports of `c9c43b0`. Step 4 exactly reproduces the working tree’s source, tests, and tracked root files, including executable modes. |
| Step 1 | **PASS — VERIFIED** | Four dead settings removed; three specified files deleted; `BLUE_LED` preserved; connection defaults use `ConnectivityPolicy`. **−96 lines.** Loadable sections byte-identical to baseline. |
| Step 2 | **PASS — VERIFIED** | Logging guard, validation error, and eight filters centralized. Both flag emitters use one definition; release remains **`0x4008`**, with retired bits reserved. Switch retirement, value-based TMP112 guard, profiles, and platform correction pass. **−7 lines.** Release binary byte-identical to step 1. |
| Step 3 | **PASS — VERIFIED** | Version centralized; old files deleted; exact notes preserved in CHANGELOG. Release staging and documentation updated. Script’s actual extraction expression returns **28**. **−51 lines.** `nm` names/sizes identical; all **40,972 instructions** match after address normalization. Reduction: **148-byte string − 4 bytes additional padding = 144 bytes**. Version remains `v28-CloseBeforeSleep`, product version **28**. |
| Step 4 | **PASS — VERIFIED** | One **5000–120000 ms** range serves both sites. **+6 source lines**, plus the 204-line test. All four boundary cases pass; restoring `1000, 60000` fails the test. Mutation restored byte-identically in place. |
| Suite/release | **PASS — VERIFIED** | **46/46:** 22 shell tests via **zsh**, 24 Python tests via **python3**. Normal release text/data/bss: **150164 / 1090 / 2196**. `strings` finds `v28-CloseBeforeSleep` and no `pdiag`. |

Step 2’s bench build contains `pdiag` and `PowerDiagnostics::flushDiagBatch`; RTC-skew symbols are absent from both the ELF and relevant objects. Bench text/data/bss is **151272 / 1090 / 2444**. Preprocessing the real logging translation unit confirms `-DSERIAL_LOG_LEVEL=0..4` selects the requested handler; −1 and 5 trigger `#error`.

Step 4 changes decoded instructions only within `Cloud::applyReportingConfig`: bounds, the 120000 literal-pool entry, and resulting address/alignment adjustments. Its symbol grows **`0x1e0 → 0x1e4`**. Instructions elsewhere are unchanged, and every compared section except `.text` remains byte-identical.

Fresh Device OS 6.4.1 Boron builds used **`'EXTRA_CFLAGS=-frandom-seed=$$@'`** and the WO’s seven sections in the specified order:

| Step | Bytes | Loadable-section SHA-256 |
|---|---:|---|
| 1 | 151322 | `ad322c5dd8517b78092fdaff95a581b8326382acac4c5183dc03817a4625bf3d` |
| 2 | 151322 | `ad322c5dd8517b78092fdaff95a581b8326382acac4c5183dc03817a4625bf3d` |
| 3 | 151178 | `a37a5d07adc90774c0346ec572fa15abe9698c491340e246d7c520236614738b` |
| 4 | 151178 | `1673f72e35ddc2237fd435dc102ec128cf85f4dcd9a04c7a4a3ad3fab902e2b6` |

The only existing test-file edits remove duplicate `FIRMWARE_VERSION` definitions:

- `tests/power_source_override_test.cpp`
- `tests/stubs/clock_status_republish_overrides/CloudLinkStubs.cpp`

No assertions were weakened. `build_flags_witness_test.sh` and `publish_with_ack_structural_test.py` are unchanged and green.

Notes: the Stage 6 report misattributes some changes between steps; the **actual patches follow the specified ordering**. The accepted v28 CHANGELOG placement and six-line step-4 budget are confirmed. Parallel compilation attempts encountered a make dependency-order error; all reported results came from successful fresh serial builds.

**Preservation confirmed:** all **1,349 working-tree files** match the initial byte/mode manifest. Git status, index bytes, and HEAD are unchanged. Scratch artifacts were removed. No network or device access occurred.