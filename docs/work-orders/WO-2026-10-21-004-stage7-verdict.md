# VERIFIED WITH CONCERNS

**Reviewer:** Codex · **Model:** `gpt-5.6-sol` (probed 2026-10-20, §5) · **Reasoning:** high

**Test interpreters:** **101/101** — 58 `.sh` suites via `zsh`, then 43 bare `.py` suites via `python3`, serial and one suite at a time. ARM builds used Device OS 6.4.1 Boron, GCC ARM 10.2.1, `-frandom-seed=p6`, and distinct fresh `BUILD_PATH_BASE` directories; the flag pair added `EXTRA_CFLAGS=-DDEV_BUILD=1`. Argon was not built.

Review target: implementation commit `a06b09a97226b5caeecd0ab1ded2b7029894c63f` against parent `cc3d157c5e84ed8ffe7652345c6f1932194d928c`; the parent's `src/` tree (`4a67dad9…`) and `State_Sleep.cpp` blob (`1caeb8c4…`) are identical to S-4 tip `e9475d7`. Current worktree tip `de4120a` adds only the Stage 6 result/controller reproduction and Stage 7 dispatch documentation.

| Check | Result | Evidence |
|---|---|---|
| 1. Transcript provenance and coverage | **PASS** | The fixture names `e9475d7`; detached Git-backed regeneration was byte-identical (927 lines, file SHA-256 `aa6ecd1e…`, body `e65eabcb…`): 86 scenarios/122 passes — release 59/59, DEV 7/7, Wi-Fi 20 scenarios/56 passes (42 WAIT, 14 CONTINUE). Release covers standby × cloud × radio, quiet/verbose, awake below/at/above the ceiling and around the 30 s slack, connect below/at/above budget+1000 with NONE/NORMAL/DEEP, SoC bounds/sentinel/rounding, and charge sentinel/mapping. DEV pins ceiling suppression while retaining other warnings. Wi-Fi covers already-off, first-on pass, retry just under/at/over 400 ms, limit/2001 ms give-up and reset, 253/254/255 counter behavior, entry reset, verbose/quiet, cloud on, standby, and multi-pass sequences. The script extracts base blocks/statics/reset by unique markers from `git show`, current functions/calls/reset from the current file, re-extracts compiled copies, and reports `COPY_MISMATCH`; forced copy drift did so. Its eleven mutations catch the five formerly unpinned properties. My independent valid mutants (`soc <= 100`→`< 100`; standby invariant tests radio instead of cloud) both produced specific `EQUIVALENCE_FAILED` deltas. |
| 2. Moved-block byte identity | **PASS** | Independent normalized diffs are empty. Summary: base outer `  {`/`  }` become the function signature/braces; all 68 interior lines, including all 6 blank lines, are byte-identical. Guard: the 50 original body lines, including all 4 blank lines, are identical after exactly 22 `wifiGuard.` prefixes and the sole early `return;`→`return true;`; the function appends `return false;`. The old `#if/#endif` now encloses the struct and function, and the handler retains a paired `#if/#endif` around the call. Whitespace differences outside the bodies are fully accounted for: one extra empty separator after `cloudGatePass`, one between summary and the Wi-Fi wrapper, one between struct and guard function, and no separator between the wrapper's closing `#endif` and `struct SleepPlan`. |
| 3. One owner and placement | **PASS** | `State_Sleep.cpp:356-488` places both internal-linkage functions after `cloudGatePass` and before `SleepPlan`; `wifiGuard` is one `Wiring_WiFi`-guarded struct at `:427-433`. Its only field writes are in `wifiOffGuardPass` (`:446-483`) and the entry reset (`:1085-1088`). Handler calls are at `:1149` and `:1155`; breadcrumb 24 (`:1134`), stale comments (`:1136-1139`) and alias (`:1144`) remain inline. Only `State_Sleep.cpp` changed in `src/`; no new source file, `SleepTeardown.*`, S-2–S-4 functions, includes, or `retained` token changed. |
| 4. Wi-Fi path | **PASS** | Both Boron build pairs contain zero Wi-Fi-guard strings/symbols, confirming the guarded code compiles out. The host Wi-Fi build compiles and matches the base for all 20 scenarios/56 passes. Base line 1125's early `return;` is current `return true;` at `:465`, and the handler returns exactly on true at `:1155`; false/give-up continues. Directive inspection plus the structural test confirms no unrelated preprocessor pairing moved. |
| 5. ELF and retained | **PASS** | Release is `151348/1114/2212` before and after; DEV is `151276/1114/2212` before and after. Extracted `.text` is byte-identical in each pair (release SHA-256 `53d7f40a…`; DEV `382de053…`), full defined-symbol listings are identical, the new helpers leave no out-of-line symbols, and static-init stubs are 22/22 in both pairs. Release and DEV `.backup` are each 0x98 bytes at VMA `0x2003f400` and byte-identical (`adfe530f…`); all retained addresses remain fixed (`retainedBoot` `0x2003f420`, hibernate fields `0x2003f480/488/48c/490`, signature `0x2003f494`). No added source line contains `retained`. |
| 6. Re-anchored tests (ruling 8) | **CONCERN** | The three diffs retain the earlier assertions and make the intended bindings: occupancy now anchors teardown at the handler's `SleepTeardown::run` and its new below-teardown mutant fails; the cloud-gate and plan tests add the exact S-5 markers/normalizations, while a gate-reset mutation and ordinary unrelated handler mutation (`breadcrumb 24→25`) fail. However, their global `.replace("wifiGuard.", "")` is broader than the four reset lines: prefixing the unrelated `"sleep-abort-open-hours"` string with `wifiGuard.` passes both re-anchored structural tests and the new S-5 structural test. Unchanged `config_window_hold_test.sh` rejects that mutant, so shipped behavior remains covered. Reader scan found no vacuity: `power_consumer_regression_test.sh` still extracts the SoC/`isCharging` encoding and `modem_teardown_confirmation_logging_test.py` still binds the standby=0 invariant text. |
| 7. Budget, complexity and suite | **CONCERN** | Hard rule is tracked `src/` additions minus deletions, so moves cancel: `+140/-130 = +10` net (1,373→1,383), with nonblank 1,246→1,253 (**+7**), within ≤+20. Per-file logical-line counts (including unterminated final lines) against main `0c565fa`: tests excluding fixtures 28,893→33,083 (**+4,190**), fixtures 834→18,890 (**+18,056**), suites 91→101 (**+10**), `src/` 22,306→22,378 (**+72**). S-5 alone: **+871 / +927 / +2 / +10**. The 86-scenario/927-line fixture is proportionate despite being 1.32× the estimate's 700-line ceiling: it uses one broad release matrix plus only 7 DEV and 20 multi-pass Wi-Fi cases, not duplicate full matrices. Non-fixture test growth is nevertheless 8.71× the +100 guideline. Full suite passed 101/101; golden payload is unchanged (`07de1e19…`), S-1–S-4 fixtures are unchanged, and `git diff --check a06b09a^ a06b09a` is clean. |

RETAINED OUTCOME: A

## Findings

### C1 — Low — S-5 prefix normalization is broader than its stated allow-list

- **Observation:** `tests/sleep_cloud_gate_structural_test.py:139`, `tests/sleep_plan_structural_test.py:136`, and `tests/sleep_preflight_structural_test.py:170` remove every `wifiGuard.` substring across their compared handler span. A Git-backed mutant changing the unrelated transition reason to `"wifiGuard.sleep-abort-open-hours"` passed all three structural tests. The unchanged `config_window_hold_test.sh:79` rejected it; ordinary unrelated-code and gate-reset mutations also fail as intended.
- **Inference:** This is a narrow test-maintenance loophole, not an implementation or current behavior defect, and another unchanged suite closes the demonstrated case. Restricting prefix normalization to the four entry-reset lines would make the ruling-8 claim exact.

### C2 — Low — non-fixture test implementation exceeds the guideline

- **Observation:** S-5 adds `+880/-9 = +871` non-fixture test lines, 8.71× the +100 guideline; its 927-line fixture is 227 lines above the estimate's upper bound.
- **Inference:** The fixture itself is proportionate to the independently exercised branch/boundary surface and the one-build rule, but the harness adds material maintenance cost. This is test-only and does not weaken the equivalence evidence.

## Budget versus actual

| Item | Budget/guideline | Actual |
|---|---:|---:|
| `src/` net | ≤ +20 hard | **+10** all lines; **+7** nonblank |
| S-5 tests excluding fixture | +100 guideline | **+871 net** (`+880/-9`) |
| S-5 fixture | 450–700 estimate | **927 lines**, 86 scenarios/122 passes |
| S-5 suites | baseline 99 | **101** (+2) |

## Confirmations

- The original worktree was clean before review. Regeneration occurred in a detached Git-backed worktree and left its fixture byte-identical; the source/test tree was not changed. The authorized verdict file is the only final worktree change.
- The sandbox could not write this checkout's external administrative `.git/worktrees`; I therefore made local Git clones in authorized scratch and used `git worktree add --detach` from those clones, preserving full history for every mutant and regeneration.
- All review-created worktrees, clones, build outputs and logs were removed. The pre-existing empty `build-tmp/` directory remains. No commit, push, stash, reset, checkout/restore, device, network or AWS access occurred; only Boron was built.
