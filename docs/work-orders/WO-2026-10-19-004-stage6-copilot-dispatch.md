AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-19, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- **Edit:**
  - **new** `src/power/ModemHealth.{h,cpp}`;
  - `src/state/State_Sleep.cpp`: remove the thresholds, the enum and the two latch functions (they move), and include the new header; **leave** `connectionModeLabel`, `batteryTierLabel` and `logTeardownContext` where they are;
  - `src/state/State_Connect.cpp`: remove `MODEM_UNSTABLE_CONNECT_TIMEOUT_THRESHOLD` and `markModemUnstableFromConnectTimeout()`, replace the call at `:664` with `noteConnectTimeout()` and the success reset at `:611–613` with `noteConnectSuccess()`;
  - `src/state/State_Report.cpp`: replace the daily-close block at `:85–91` with `clearAtDailyClose()` (keep the surrounding code and its comment);
  - `src/state/StateMachine.h` **only** if a declaration must move (the four fields stay declared there).
- **Tests:** re-point `standby_latch_daily_clear_test.{sh,cpp}`; add the new equivalence harness and the structural test; check `conn_phase_modem_off_test.sh` still finds its markers; report each change.
- **Run** the host suite and local ARM release builds.
- **Scratch directory:** **`build-tmp/wo20261019-004-stage6/`**. Remove **only it** when you finish.

**Not authorized:**
- Any change to a log line's text, level or condition, to a threshold's value (10000 ms, 5000 ms, 2), or to the order in which two log lines fire.
- Moving the labels or `logTeardownContext`; any read of the four fields (reads stay where they are); `session`'s definition; the `retained` definitions (including `retainedBoot`).
- Any other `src/` change, `docs/`, `lib/`, `project.properties` or Device OS.
- Commits, pushes, merges, branch changes, stash, reset, checkout or restore. Device or network access beyond the Particle compile. Deleting anything you didn't create, including `build-tmp/` itself.

**SIZE BUDGET:**
- **`src/`:** **≤ +20 net non-move lines, HARD**: lines moved byte-for-byte count 0 (reported as gross); new header, include, namespace and wrapper lines count, and so do the lines in the three call sites that change. Over means STOP and report (§13 (b)). Removing the Connect duplicate should make the total small.
- **Tests:** guideline **+175** (the equivalence harness is new); record the actual.

# Stage 6 dispatch: WO-2026-10-19-004 (Step 6 P-5: one owner for modem health and the standby latch)

**Goal, in plain language:** three files write the standby-latch fields (`session.modemUnstable`, `modemUnstableReason`, `modemStandbySuppressed`, `modemConnectTimeoutCount`), and Connect duplicates Sleep's logic with a magic `2`. Give those writes one owner, `power/ModemHealth`, so the planned latch fix has one place to change. **No behaviour change: the same decisions and the same log lines, character for character, under the same conditions.** This is not a pure move: three call sites change, so equivalence is proven by a host harness, not only by a byte diff.

**Repository:** this worktree, branch `wo/2026-10-19-004-step6-p5-modem-health` (from main `db9e9e1`, P-0 to P-4 merged). HEAD is at the commit that adds the WO and this dispatch. Don't switch branches. Main is Device OS 6.4.1; build with the 6.4.1 toolchain.

**Binding spec:** `docs/work-orders/WO-2026-10-19-004-step6-p5-modem-health-owner.md`, including **Claude Code's citation re-check at `db9e9e1`** (every site, the equivalence reading, and the findings). Where this dispatch and the WO differ, the WO wins; report the difference.

## Do

1. `power/ModemHealth.{h,cpp}`: the thresholds (10000 ms, 5000 ms, 2), the `ModemUnstableReason` enum, `markModemUnstable(reason, elapsedMs)` and `maybeClearModemUnstable(teardownMs)` moved **byte-for-byte** from Sleep, and three new functions: `noteConnectTimeout()` (increment the count saturating at 255; at `>=` the threshold call `markModemUnstable(CONNECT_TIMEOUT, 0)`), `noteConnectSuccess()` (the Connect `if (count != 0) count = 0;` form, kept as written), and `clearAtDailyClose()` (today's `State_Report.cpp:85–91` block including its log line, unchanged: it has no connect-result or teardown condition and does not log `recovered`). The header exposes what the callers need; `session` stays in `StateMachine.h`.
2. The call sites use the owner; no write to the four fields remains outside `ModemHealth.cpp`.
3. **No `retained` keyword in either new file** (standard negative check since P-2).

## Verify (all required)

- **Equivalence harness (new, host):** extract the old code from `git show HEAD:` (the Connect duplicate, the success reset, the Report block, and Sleep's two functions) and run it against the new owner on the same inputs, comparing the final state of the four fields and the log output **byte-for-byte**: `noteConnectTimeout()` for starting counts 0, 1, 2, 3 and 255, each with the latch not set, set with reason `CONNECT_TIMEOUT`, set with reason `SLOW_TEARDOWN`, and with `modemStandbySuppressed` set and not set; `noteConnectSuccess()` for counts 0, 1 and 255; `clearAtDailyClose()` with the latch set and not set; and the two moved Sleep functions through every branch (`reasonChanged` true/false, standby suppressed true/false, connect result SUCCESS/other, teardown time at 4999, 5000, 5001 and 10000/10001 ms).
- **Structural test:** no write to the four fields outside `ModemHealth.cpp` (bare-name regex over `src/`, excluding reads); no `retained` in the two new files.
- **Byte-identical moved bodies** (the two latch functions, thresholds, enum) against `HEAD`; `git diff --check` clean on new lines (hits on moved lines are waived only where the original has the same whitespace).
- **Mutations, each in a scratch copy, each must fail a named test (or be recorded as already unpinned at `HEAD`):** (1) drop the daily-close clear; (2) change the connect-timeout threshold from 2 to 3; (3) change the recovery teardown limit from 5000 to 6000; (4) add a write to a latch field in `State_Connect.cpp`; (5) a `retained` probe in each new file.
- **Golden payload test** unchanged; **full suite** before and after, `N/N (sh via zsh, py via python3)`, **run serially, one suite at a time**; the baseline at `HEAD` is 87.
- **Local ARM release build** (6.4.1 Boron; fresh `BUILD_PATH_BASE`): text/data/bss before (from `git archive HEAD`) and after; explain any text change over ±64 B by symbol with `nm -C -S`; list any new static-initialization stub. **Retained check under the narrowed rule:** the whole `.backup` section before and after; `retainedBoot` at `0x2003f420`, the hibernate fields at `0x2003f480/488/48c/490`, the signature word at `0x2003f494`.
- **`wc -l`** for `State_Sleep.cpp` (before 1725), `State_Connect.cpp` (754) and `State_Report.cpp` (333).

## Implementation Report (required, as your final message)

- **Files:** new and changed, with net and gross `src/` lines and the arithmetic against +20; test lines added and removed; each re-pointed test and why it is not weaker.
- **The equivalence table:** every case, old output versus new, and whether they matched.
- **Results:** the byte-diff, the suite before and after, the mutations and the tests that caught them, the build sizes and symbol deltas, the full `.backup` table.
- **Confirmations:** you removed only your scratch directory; no log line's text or condition changed.
- **Deviations** (or "none"), and **the model used**.
