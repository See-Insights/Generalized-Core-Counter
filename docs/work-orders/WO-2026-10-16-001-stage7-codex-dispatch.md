AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-18, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-16-001-step6-p1-observability`; main `363b064` is merged in; the diff is staged plus a few unstaged edits).
- Run the host suite and the tests; run local ARM builds in scratch copies.
- Write temporary files under **`build-tmp/wo20261016-001-stage7/`** or `$TMPDIR`, and remove **only** what you create.
- Mutate only in scratch copies.
- Write your verdict to `docs/work-orders/WO-2026-10-16-001-stage7-verdict.md` (the only file you may add in the repo).

**Not authorized:**
- Changes outside your scratch locations and the verdict file.
- Commits, pushes, stash, reset, checkout or restore.
- Device, network or AWS access.

# Stage 7 dispatch: WO-2026-10-16-001 (Step 6 P-1: move loop forensics and the awake watchdog out of main)

**Goal, in plain language:** about 346 lines of loop-forensics and awake-watchdog code move out of `Generalized-Core-Counter.cpp` into `src/observability/LoopForensics.{h,cpp}`, unchanged. No behaviour change.

**Binding spec:** `docs/work-orders/WO-2026-10-16-001-step6-p1-observability-move.md`: the design, Claude Code's citation re-check (17 Oct), the architect's ruling of 17 Oct (the **four accepted deviations**: enums in the header; the `= true` default in the declaration; external linkage for `awakeWatchdogSleepStrategy` and both handlers; text +280 B explained), the **narrowed retained-layout rule** (decision-bearing retained values validated or fixed; report-only diagnostics may shift) and the "Re-apply on P-0 main" section (the **fifth adaptation**, forced by P-0).

**Inputs:** `docs/work-orders/WO-2026-10-16-001-stage6b-copilot-dispatch.md`, and Copilot's two reports (not in the repo; the figures below are Claude Code's reproduction). Treat every claim as unverified.

**What changed** (uncommitted):
- **`src/`:** `Generalized-Core-Counter.cpp` +1/−441 (2720 → 2280 lines); new `src/observability/LoopForensics.cpp` +400 and `LoopForensics.h` +58. Net **+18** (limit +20, hard).
- **Tests:** new `loop_forensics_move_structural_test.py` (+65); `retained_boot_structural_test.py` +20/−1 (P-0's test, now exempting exactly `LoopForensics.h` and `.cpp` and pinning what they may use); `loop_stage_sleep_prep_exclusion_test.py` +9/−6 and `connectivity_failsafe_stage1_retired_test.py` +2/−1, re-pointed.
- **Claude Code's reproduction (6.4.1 Boron, `HEAD` = main `363b064` against the working tree, fresh `BUILD_PATH_BASE`s):** text 151468 → 151756 (+288), data 1094, bss 2212 → 2220; the whole `.backup` section (24 symbols) is identical before and after: `retainedBoot` at `0x2003f420` (32 B), signature word at `0x2003f494`, no symbol moved.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

1. **Moved bodies are byte-identical.** Diff every function and variable removed from `Generalized-Core-Counter.cpp` against the same text in the new files (the WO's re-check lists the ranges; 441 lines removed, 346 of them function lines). The only permitted differences: `static` dropped on the two handlers, `awakeWatchdogSleepStrategy`/`Initialized`/`Started` and the two name functions moved out of the anonymous namespace, the `= true` default moved into the header declaration, the enums moved into the header, and the fifth adaptation. Anything else is a FAIL.
2. **The fifth adaptation is exactly what the WO says.** `LoopForensics.h` includes `../persist/RetainedBoot.h` and declares `extern retained RetainedBoot retainedBoot;`; `LoopForensics.cpp` binds `appBreadcrumb` and `appBreadcrumbMs` as `static` references into `retainedBoot`; `retainedBoot` stays defined in `Generalized-Core-Counter.cpp:355` and nothing else retained moved.
3. **Retained layout under the narrowed rule.** Reproduce the whole-`.backup` comparison. Confirm `retainedBoot` and the signature word do not move, that no decision-bearing retained symbol moved, and list any report-only move (expected: none). State the outcome: (A) nothing moved, (B) report-only moved, (C) anything else (STOP).
4. **Linkage and symbols.** Every symbol that became external: is it declared in the header, defined once, and free of a name collision (including with Device OS and library symbols)? No moved function is still defined in main; no duplicate definitions; the `#if Wiring_Watchdog` conditionals around the watchdog state are preserved (P2 and Argon don't build for an unrelated reason, so read that by eye).
5. **Initialization order.** The moved globals are constant-initialized; confirm no static-initialization-order dependence is introduced (the strategy variable's `#if PLATFORM_ID` initializer, `lastLoopForensicsSnapshotMs`, the retained references).
6. **The +288 B text and +8 B bss are explained by symbol** (`nm -C -S`, both builds). Copilot's list: `pauseAwakeWatchdogForSleep` +92, `hal_watchdog_stop`/`WatchdogClass::stop` +20, `appBreadcrumbName` and `awakeWatchdogSleepStrategyName` no longer inlined +52, `restoreAwakeWatchdogAfterWake` +16, `serviceAwakeWatchdog` +12, `setup` +12. Confirm there are no new functions beyond the moved ones and no logic change.
7. **The tests hold up.** In scratch copies: (a) a third file using `appBreadcrumb` fails `retained_boot_structural_test.py`; (b) `LoopForensics.cpp` using `retainedBoot.bootStormCount` fails it; (c) removing one of the four pinned lines fails it; (d) a moved function re-defined in main fails `loop_forensics_move_structural_test.py`. Diff the re-pointed tests: each assertion is unchanged apart from the file it reads. The exemption names the two files, with no pattern.
8. **Golden, suite and budget.** The golden payload test passes with the fixture unchanged; the full suite `N/N (sh via zsh, py via python3)` (baseline at `HEAD` is 83; expect 84); `git diff --check` clean; net `src/` ≤ +20 (hard); tests about +96/−8 against the +100 guideline.

## Verdict (write it to the file above, then end with a short summary as your final message)

- **Header:** one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED; the model and reasoning used; the test interpreter line.
- **A table of checks 1–8,** and, on its own line, **the retained outcome from check 3 (A, B or C)**.
- **Findings,** with file:line and severity, separating observation from inference.
- **Budget versus actual,** as one row.
- **Confirmations:** the tree matches the start, and you removed only your own scratch files.
