AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-19, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-19-004-step6-p5-modem-health`; main `db9e9e1`).
- Run the host suite and the tests (**one suite at a time**: parallel suites collide on shared temp files); run local ARM builds in scratch copies.
- Write temporary files under **`build-tmp/wo20261019-004-stage7/`** or `$TMPDIR`, and remove **only** what you create.
- Mutate only in scratch copies (use `git worktree add --detach` for copies, because the equivalence test reads a commit from the repository).
- Write your verdict to `docs/work-orders/WO-2026-10-19-004-stage7-verdict.md` (the only file you may add in the repo).

**Not authorized:**
- Changes outside your scratch locations and the verdict file.
- Commits, pushes, stash, reset, checkout or restore in the review worktree.
- Device, network or AWS access.

# Stage 7 dispatch: WO-2026-10-19-004 (Step 6 P-5: one owner for modem health and the standby latch)

**Goal, in plain language:** three files wrote the standby-latch fields (`session.modemUnstable`, `modemUnstableReason`, `modemStandbySuppressed`, `modemConnectTimeoutCount`), and Connect duplicated Sleep's logic with a magic `2`. They now have one owner, `src/power/ModemHealth.{h,cpp}`. **No behaviour change: the same decisions and the same log lines, character for character, under the same conditions.** This is not a pure move: three call sites changed, so equivalence is the question.

**Binding spec:** `docs/work-orders/WO-2026-10-19-004-step6-p5-modem-health-owner.md` (design, budget, Claude Code's citation re-check at `db9e9e1`, including the line-by-line equivalence reading and that `clearAtDailyClose()` must reproduce the Report block, which is not `maybeClearModemUnstable`). Architect's ruling (19 Oct): the equivalence harness also pins the connect-timeout path and the 5000 ms recovery limit, which have no tests today; test guideline +175. Standing: no `retained` keyword in the new files; `git diff --check` hits on moved lines are waived only where the original has the same whitespace.

**Inputs:** `docs/work-orders/WO-2026-10-19-004-stage6-copilot-dispatch.md`, and Copilot's report (not in the repo; the figures below are Claude Code's reproduction). Treat every claim as unverified.

**What changed** (uncommitted):
- **`src/`:** new `power/ModemHealth.h` (16 lines) and `.cpp` (79); `State_Sleep.cpp` 1725 → 1670 (thresholds, enum and the two latch functions moved; the three labels and `logTeardownContext` stay); `State_Connect.cpp` 754 → 731 (the duplicate and its threshold removed; calls `noteConnectTimeout()` and `noteConnectSuccess()`); `State_Report.cpp` 333 → 328 (calls `clearAtDailyClose()`). **+101 added, −89 removed: net +12** (limit +20, hard).
- **Tests:** new `modem_health_equivalence_test.{sh,cpp}`, `modem_health_owner_structural_test.py`, stubs under `tests/stubs/modem_health/` (+346 lines), and `standby_latch_daily_clear_test.{sh,cpp}` re-pointed (+41/−27): about **+360 against the +175 guideline**. The equivalence test reads the old code from commit `db9e9e1` (override `MODEM_HEALTH_BASE`) rather than `HEAD`.
- **Claude Code's reproduction (6.4.1 Boron, fresh `BUILD_PATH_BASE`; the "before" is the P-4 tree, whose `src/` equals main `db9e9e1`):** suite **89/89** (baseline 87; sh via zsh, py via python3; serial); text/data/bss `151764/1094/2228 → 151748/1094/2228` (**−16 text**); the whole `.backup` section (24 symbols) identical (`retainedBoot` `0x2003f420`, hibernate fields `0x2003f480/488/48c/490`, signature `0x2003f494`); static-init stubs 22 before and after. Symbols: new `clearAtDailyClose()` 36 B, `markModemUnstable(ModemUnstableReason, unsigned long)` 92, `noteConnectSuccess()` 16, `noteConnectTimeout()` 32, `maybeClearModemUnstable(unsigned long)` 72; gone: the anonymous-namespace `markModemUnstable … [clone .constprop.0]` 72; `handleSleepingState` 7048 → 6968, `handleConnectingState` 2412 → 2320, `handleReportingState` 992 → 976; `__tcf_1` 2 → 24, `__tcf_3` 12 → 2, a `Buffer::operator=` clone 44 → 42.
- **Mutations I ran serially (new tree, with the right test sets):** drop the daily-close clear (caught by the structural test and `standby_latch_daily_clear_test.sh`); threshold 2 → 3 and recovery limit 5000 → 6000 (each caught by the equivalence test and the structural test); a latch write in `State_Connect.cpp` (structural); a `retained` probe in the `.cpp` and in the `.h` (equivalence and structural). At `HEAD` (unmoved code) the threshold and the recovery-limit mutations pass the two existing latch tests: both were unpinned before.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

1. **The moved bodies are byte-identical,** against `db9e9e1`: the two thresholds (10000 and 5000 ms), the enum, `markModemUnstable` and `maybeClearModemUnstable`. Anything beyond linkage, declarations and includes is a FAIL.
2. **Equivalence of the three new functions,** by your own reading and not only the harness. `noteConnectTimeout()` against the old `markModemUnstableFromConnectTimeout()`: the saturating increment, the `>=` threshold, the state writes, the `reasonChanged` condition (the old code used the literal `2`), both log lines and their order, the `count=%u` value. `noteConnectSuccess()` against `State_Connect.cpp:611–613`. `clearAtDailyClose()` against `State_Report.cpp:85–91`, including that it neither logs `MODEM_HEALTH: recovered` nor depends on the connect result or the teardown time. List every difference you find, including field-write order, even if unobservable.
3. **The six log lines are byte-identical and fire under the same conditions:** `MODEM_HEALTH: unstable reason=slow_teardown elapsed=%lu`, `MODEM_HEALTH: unstable reason=connect_timeout count=%u`, `MODEM_POLICY: standby temporarily disabled reason=unstable_modem`, `MODEM_HEALTH: recovered`, `MODEM_POLICY: standby restored`, `MODEM_POLICY: standby re-allowed at daily close; night modem-off decides`. Search the rest of `src/` for any other write to a latch field or any dependent log.
4. **The equivalence harness is real, not tautological.** Does it extract the old code from the base commit and compare it with the new owner on identical inputs, byte-for-byte on state and log output? Do its 176 cases cover what the dispatch lists (connect-timeout counts 0, 1, 2, 3 and 255 with the latch unset, set with each reason, and `modemStandbySuppressed` set and unset; `noteConnectSuccess()` counts 0, 1, 255; `clearAtDailyClose()` set and unset; both Sleep functions through every branch with teardown times at 4999, 5000, 5001, 10000 and 10001 ms)? Does it fail when the new code differs (run two mutations of your own)? **State whether its dependence on commit `db9e9e1` being present in the repository is a problem** (a shallow clone or an exported source tree would lack it): is a frozen fixture of the old code the more robust design, and what would it cost? Rate it CONCERN or PASS and explain.
5. **One owner.** No write to the four fields outside `ModemHealth.cpp` (bare-name search over `src/`, excluding `StateMachine.h` declarations and tests); the reads stay where they were (`State_Connect.cpp:444`, `State_Report.cpp:205`); `session` is still defined in `StateMachine.h`.
6. **Linkage and symbols.** `markModemUnstable` and `maybeClearModemUnstable` were internal (anonymous namespace in `State_Sleep.cpp`); they and the three new functions are now external. Check for name collisions (including Device OS and libraries), ODR (the thresholds and enum in the header), that the final link succeeded, and that `MODEM_UNSTABLE_SLOW_TEARDOWN_MS` in the header is the only constant Sleep needs.
7. **The −16 B text is itemized by symbol** (`nm -C -S`, both builds; the figures above are mine): account for the whole delta including alignment; confirm no new static-initialization stub.
8. **Retained layout under the narrowed rule:** the whole `.backup` comparison, outcome A/B/C; no `retained` keyword in the two new files (a probe in each fails with a named message).
9. **Tests and budget.** Diff the re-pointed `standby_latch_daily_clear_test.{sh,cpp}` (Copilot says it keeps all six old mutants and adds one); `conn_phase_modem_off_test.sh` passes unchanged. Non-move `src/` ≤ +20 (hard): count independently and state the rule. Test volume about +360 against the +175 guideline: flag, don't fail. `git diff --check`. Full suite `N/N (sh via zsh, py via python3)`, run serially; the golden payload test passes with the fixture unchanged (`07de1e1980e4a44e03420540e6c721a93a61b5d6392539219928420c048f0637`).

## Verdict (write it to the file above, then end with a short summary as your final message)

- **Header:** one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED; the model and reasoning used; the test interpreter line.
- **A table of checks 1–9,** and, on its own line, **the retained outcome from check 8 (A, B or C)**.
- **Findings,** with file:line and severity, separating observation from inference.
- **Budget versus actual,** as one row.
- **Confirmations:** the tree matches the start, and you removed only your own scratch files.
