AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-19, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-19-003-step6-p4-time-helpers`; main `768c3c0`).
- Run the host suite and the tests; run local ARM builds in scratch copies.
- Write temporary files under **`build-tmp/wo20261019-003-stage7/`** or `$TMPDIR`, and remove **only** what you create.
- Mutate only in scratch copies. Run suites **one at a time** (parallel suites collide on shared temp files and give false failures).
- Write your verdict to `docs/work-orders/WO-2026-10-19-003-stage7-verdict.md` (the only file you may add in the repo).

**Not authorized:**
- Changes outside your scratch locations and the verdict file.
- Commits, pushes, stash, reset, checkout or restore.
- Device, network or AWS access.

# Stage 7 dispatch: WO-2026-10-19-003 (Step 6 P-4: move the clock and hibernate helpers to `time/`)

**Goal, in plain language:** 93 lines of clock helpers move out of `Generalized-Core-Counter.cpp` into a new `src/time/ClockDiagnostics.{h,cpp}` (`ab1805WakeReasonNameLocal`, its wrapper `ab1805WakeReasonName`, `logTimeDiag`), and 85 lines of hibernate-eligibility helpers move out of `src/state/State_Sleep.cpp` into a new `src/time/HibernateEligibility.{h,cpp}` (`isDeviceOsAtLeast6400`, `clearAb1805StaleAlarmInterrupts`, `shouldUseBoronRtcAlarmHibernate`, under the Boron guard), **unchanged**. No behaviour change. It is a helpers move with a mechanical retained check.

**Binding spec:** `docs/work-orders/WO-2026-10-19-003-step6-p4-clock-hibernate-helpers-move.md` (rules, budget, Claude Code's citation re-check at `768c3c0`). Standing since P-3: `git diff --check` hits on moved lines are waived when the original has the same whitespace; hits on new lines fail. Standing since P-2: no `retained` keyword in the new files.

**Inputs:** `docs/work-orders/WO-2026-10-19-003-stage6-copilot-dispatch.md`, and Copilot's report (not in the repo; the figures below are Claude Code's reproduction). Treat every claim as unverified.

**What changed** (uncommitted):
- **`src/`:** new `time/ClockDiagnostics.{h,cpp}` (109 + 3 lines) and `time/HibernateEligibility.{h,cpp}` (93 + 7 lines); main 1596 → 1492; `State_Sleep.cpp` 1814 → 1725 (one include added). `time/HibernateCycle.cpp` must be untouched.
- **Non-move `src/` lines: claimed +20, exactly the hard limit** (the new files' includes, namespace and guard-declaration lines, plus one include in `State_Sleep.cpp`). My own count: 18 non-blank non-move lines in the new files + 1 include in `State_Sleep.cpp` + the `} // namespace` closing line that closes the new anonymous namespace = 20. **Count it yourself and state your counting rule; a count of 21 or more is a FAIL.**
- **Tests:** new `clock_hibernate_helpers_move_structural_test.py` (+100); `clock_owner_structural_test.py` (the `isPlausibleEpoch` caller check now reads `HibernateEligibility.cpp` and also asserts `State_Sleep.cpp` has none, which is stricter; the `Time.isValid()` allowlist scans main plus `ClockDiagnostics.cpp`), `clock_resync_wiring_test.py` (`logTimeDiag` extracted from `ClockDiagnostics.cpp`), and path comments only in `rtc_skew_test.cpp` and `watchdog_ab1805_classification_test.cpp`: +16/−8.
- **Claude Code's reproduction (6.4.1 Boron, fresh `BUILD_PATH_BASE`; the "before" is the P-3 tree, whose `src/` equals main `768c3c0`):** suite **87/87** (baseline 86; sh via zsh, py via python3; run sequentially); text/data/bss `151820/1094/2228 → 151764/1094/2228` (**−56 text**); the whole `.backup` section (24 symbols) identical, with `retainedBoot` at `0x2003f420` and the hibernate fields at `0x2003f480`, `0x2003f488`, `0x2003f48c`, `0x2003f490`; no new static-initialization stub. Symbol-level: three new out-of-line symbols `isDeviceOsAtLeast6400()` 104 B, `clearAb1805StaleAlarmInterrupts()` 96 B, `shouldUseBoronRtcAlarmHibernate(unsigned long, long long&)` 184 B (previously inlined into `handleSleepingState`, 7340 → 7048, −292); `logTimeDiag(bool)` 464 → 504; `setup` 3096 → 3136; `__tcf_1` 24 → 2.
- **Two controller findings on the WO's mutations:** (1) the WO says to make `logTimeDiag` log on every call; but `logTimeDiag` has no on-change logic: the v41 on-change condition is `if (enteredIdle || timeDiagKey != lastTimeDiagKey)` at `State_Idle.cpp:132`, which this move does not touch (Copilot found the same). (2) Removing the AB1805 stale-alarm clear (an early `return true;` at the top of `clearAb1805StaleAlarmInterrupts()`) passes the whole suite both at `HEAD` (86/86, in `State_Sleep.cpp`) and at the new home (87/87): a pre-existing gap, to go to the recovery plan.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

1. **Every moved body is byte-identical,** with its doc comment and the Boron guard lines: `ab1805WakeReasonNameLocal` (with the doc block above it), `ab1805WakeReasonName`, `logTimeDiag` (with its leading comment), `isDeviceOsAtLeast6400`, `clearAb1805StaleAlarmInterrupts` (including `kRegStatus` and the other function-local constants), `shouldUseBoronRtcAlarmHibernate` (including `kMinHibernateSleepSec`). Anything beyond linkage, declarations, includes and the guard/namespace lines is a FAIL.
2. **Linkage, guard and declarations.** The three Sleep functions are external under `#if PLATFORM_ID == PLATFORM_BORON`, declared once in `HibernateEligibility.h` under the same guard, defined once; their callers (`State_Sleep.cpp` around `:1125`, `:1128`, themselves under the Boron guard) still resolve; `ab1805WakeReasonName` is still the one external symbol `HibernateCycle.cpp:9` declares; `logTimeDiag` is still declared in `State_Common.h:199` and called from `State_Idle.cpp` and `State_Sleep.cpp`. Non-Boron builds do not link in this repo for an unrelated reason, so read the guard placement by eye for those platforms.
3. **Retained layout under the narrowed rule.** Reproduce the whole `.backup` comparison and list the hibernate fields' addresses before and after; `time/HibernateCycle.cpp` is byte-identical to `HEAD`; no `retained` keyword in any of the four new files (after stripping comments and string literals), and the new structural test fails with a named message for a probe in each new `.cpp` and `.h`. State the outcome: (A) nothing moved, (B) report-only moved, (C) anything else (STOP).
4. **The −56 B text is itemized by symbol.** Account for the whole delta (`nm -C -S`, both builds), including the three new out-of-line symbols, `handleSleepingState` −292, `logTimeDiag` +40, `setup` +40, `__tcf_1`, `CSWTCH` tables and alignment/fill; confirm no new static-initialization stub and no new function beyond the three that are now out of line.
5. **The tests were re-pointed, not weakened.** Diff each changed test: assertions unchanged apart from the file they read, except the declared stricter changes in `clock_owner_structural_test.py`. Then, in scratch copies: (a) remove or invert the on-change condition at `State_Idle.cpp:132` and see which test fails (`idle_timediag_on_change_test.sh` is expected); (b) re-define a moved function in main or `State_Sleep.cpp` and confirm the new structural test fails; (c) a `retained` probe in each new file; (d) confirm the stale-alarm early-return mutation passes at both `HEAD` and the new home, as the controller found (report honestly if your result differs).
6. **Budget and hygiene.** Non-move `src/` lines ≤ +20, hard (count independently, state the rule); `git diff --check` clean on new lines (hits on moved lines are waived only where the original has the same whitespace; check each hit against `HEAD`); main `wc -l` 1596 → 1492 and `State_Sleep.cpp` 1814 → 1725.
7. **Golden, suite.** The golden payload test passes with the fixture unchanged (`07de1e1980e4a44e03420540e6c721a93a61b5d6392539219928420c048f0637`); the full suite `N/N (sh via zsh, py via python3)`, run sequentially.

## Verdict (write it to the file above, then end with a short summary as your final message)

- **Header:** one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED; the model and reasoning used; the test interpreter line.
- **A table of checks 1–7,** and, on its own line, **the retained outcome from check 3 (A, B or C)**.
- **Findings,** with file:line and severity, separating observation from inference.
- **Budget versus actual,** as one row.
- **Confirmations:** the tree matches the start, and you removed only your own scratch files.
