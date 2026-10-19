AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-19, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-19-001-step6-p2-failsafe`; main `3975e9c`).
- Run the host suite and the tests; run local ARM builds in scratch copies.
- Write temporary files under **`build-tmp/wo20261019-001-stage7/`** or `$TMPDIR`, and remove **only** what you create.
- Mutate only in scratch copies.
- Write your verdict to `docs/work-orders/WO-2026-10-19-001-stage7-verdict.md` (the only file you may add in the repo).

**Not authorized:**
- Changes outside your scratch locations and the verdict file.
- Commits, pushes, stash, reset, checkout or restore.
- Device, network or AWS access.

# Stage 7 dispatch: WO-2026-10-19-001 (Step 6 P-2: move the connectivity failsafe out of main)

**Goal, in plain language:** the connectivity failsafe (the device's last-resort recovery: stage 2 system reset, stage 3 deep power-down) moves out of `Generalized-Core-Counter.cpp` into `src/power/ConnectivityFailsafe.{h,cpp}`, **unchanged**. No behaviour change. Because it is the last-resort path, "unchanged" is checked strictly.

**Binding spec:** `docs/work-orders/WO-2026-10-19-001-step6-p2-failsafe-move.md`: the rules, the budget and Claude Code's citation re-check at `3975e9c` (the ranges and the findings, including `batteryTierShortName`).

**Inputs:** `docs/work-orders/WO-2026-10-19-001-stage6-copilot-dispatch.md`, and Copilot's report (not in the repo; the figures below are Claude Code's reproduction). Treat every claim as unverified.

**What changed** (uncommitted):
- **`src/`:** `Generalized-Core-Counter.cpp` +1/−303 (2280 → 1978 lines); new `src/power/ConnectivityFailsafe.h` (3 lines) and `.cpp`. Claimed non-move lines: +17 (header 3, new-file includes and namespace lines 13, one include in main) before crediting two removed forward declarations, so +15 net of them (limit +20, hard). `batteryTierShortName` moved with the failsafe (a deviation from the WO's list).
- **Tests:** 7 re-pointed (`connectivity_failsafe_open_hours_test.sh`, `failsafe_cadence_rule_test.sh`, `connectivity_failsafe_stage1_retired_test.py`, `power_consumer_regression_test.sh`, `connection_mode_downgrade_structural_test.py`, `connection_mode_downgrade_test.sh`, `clock_trust_standard_structural_test.py`) and a new `connectivity_failsafe_move_structural_test.py` (76 lines); about +96 in all.
- **Claude Code's reproduction (6.4.1 Boron, fresh `BUILD_PATH_BASE`s):** suite 85/85 (baseline 84; sh via zsh, py via python3); release build text/data/bss 151756/1094/2220 before and after; bench build (`EXTRA_CFLAGS=-DCONNECTIVITY_FAILSAFE_TEST_MODE=1`) 153260/1094/2220 before and after; no symbol appeared, vanished or changed size apart from compiler `CSWTCH` renumbering; the whole `.backup` section (24 symbols) is identical, `retainedBoot` at `0x2003f420`, the signature word at `0x2003f494`.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

1. **Every moved body is byte-identical, including both arms of each `#if CONNECTIVITY_FAILSAFE_TEST_MODE` block.** Compare each function removed from main (the WO's re-check lists 11 ranges: `currentBatteryTierForFailsafe`, `connectivityFailsafeHasExternalPower`, `connectivityFailsafeJitterSec`, `persistConnectivityFailsafeState`, `failsafeDeferReasonNameLocal`, `claimFailsafeDeferLogLocal`, the two wrappers, `clearConnectivityFailsafeRecovery`, `connectivityFailsafeSupervisor`, plus `batteryTierShortName`) against its text in the new file at brace-matched level, with comments. Anything beyond linkage, declarations and includes is a FAIL.
2. **The last-resort path is intact branch by branch.** Walk `connectivityFailsafeSupervisor()` in the new file against `HEAD`: the open-hours gate, the cadence rule, cooldown and jitter, the low-battery hard-stage block, the stage 2 and stage 3 actions, and the defer logging; and confirm the call sites reach it exactly as before: `loop()` in main (`:1280` at `HEAD`) still calls `connectivityFailsafeSupervisor()`, `State_Connect.cpp:514` still calls `clearConnectivityFailsafeRecovery("cloud-ok")`, `State_Sleep.cpp:1035–1037` still use `claimFailsafeDeferLog` and `failsafeDeferReasonName`. Check by bare name as well as qualified.
3. **Linkage, declarations and no collisions.** Which functions are external, which internal; each external one declared once and defined once; the helpers' internal linkage cannot change behaviour; no ODR/duplicate definition; both final links succeed (release and bench). `batteryTierShortName` has no other user (bare-name search over `src/` and `tests/`).
4. **Both builds.** Reproduce the release and the bench (`CONNECTIVITY_FAILSAFE_TEST_MODE=1`) builds, each before and after, with fresh `BUILD_PATH_BASE`s, and the text/data/bss figures above; explain any difference by symbol with `nm -C -S`.
5. **Retained layout under the narrowed rule.** Reproduce the whole-`.backup` comparison (release build); the failsafe uses persisted state (`RecoveryState`, `SystemConfig::flushNow()`), not retained RAM: confirm no `retained` variable is named in the moved code. State the outcome: (A) nothing moved, (B) report-only moved, (C) anything else (STOP).
6. **The tests were re-pointed, not weakened.** For each of the seven, diff it: the assertions are unchanged apart from the file they read, with one declared exception: `clock_trust_standard_structural_test.py` now scans the new file (Copilot says that is stricter). Then mutate in scratch copies: (a) change a threshold or reorder a branch in the new file's supervisor and confirm a named test fails; (b) re-define a moved function in main and confirm `connectivity_failsafe_move_structural_test.py` fails; (c) put a `retained` variable in the new file and confirm it fails; (d) edit one arm of an `#if CONNECTIVITY_FAILSAFE_TEST_MODE` block and see which test, if any, catches it (report honestly if none does).
7. **`src/diagnostics/ConnectivityFailsafeTest.cpp` is byte-identical to `HEAD`** (rule 4), and so are `State_Common.h`, `State_Connect.cpp`, `State_Sleep.cpp` and `BuildProfile.h`.
8. **Golden, suite and budget.** The golden payload test passes with the fixture unchanged; the full suite `N/N (sh via zsh, py via python3)`; `git diff --check` clean; non-move `src/` lines ≤ +20 (hard), counted as the WO says (every new header line, include and namespace line); main `wc -l` before (2280) and after.

## Verdict (write it to the file above, then end with a short summary as your final message)

- **Header:** one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED; the model and reasoning used; the test interpreter line.
- **A table of checks 1–8,** and, on its own line, **the retained outcome from check 5 (A, B or C)**.
- **Findings,** with file:line and severity, separating observation from inference.
- **Budget versus actual,** as one row.
- **Confirmations:** the tree matches the start, and you removed only your own scratch files.
