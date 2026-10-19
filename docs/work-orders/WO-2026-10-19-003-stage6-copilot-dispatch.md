AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-19, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- **Edit:**
  - `src/Generalized-Core-Counter.cpp`: remove the three moved definitions (`ab1805WakeReasonNameLocal`, `ab1805WakeReasonName`, `logTimeDiag`) and add the one include it then needs, if any;
  - `src/state/State_Sleep.cpp`: remove the three moved definitions together with their `#if PLATFORM_ID == PLATFORM_BORON` / `#endif` guard lines and add the include it then needs;
  - **new** `src/time/ClockDiagnostics.{h,cpp}` (from main) and **new** `src/time/HibernateEligibility.{h,cpp}` (from `State_Sleep.cpp`); the Boron guard goes around the Sleep functions' definitions and their declarations;
  - `src/state/State_Common.h` only if a declaration must move (the `logTimeDiag` declaration at `:199` should stay).
- **Tests:** re-point every test that anchors on text in `Generalized-Core-Counter.cpp` or `State_Sleep.cpp` for a moved function so it reads the new file (never weaken it); add the structural test the WO asks for; report each change.
- **Run** the host suite and local ARM release builds.
- **Scratch directory:** **`build-tmp/wo20261019-003-stage6/`**. Remove **only it** when you finish.

**Not authorized:**
- Any change to a moved function body, comment, constant or guard condition.
- **`src/time/HibernateCycle.cpp`** (it already declares `ab1805WakeReasonName` at `:9`; it defines the hibernate retained fields, so it stays untouched), and the `retained` definitions (including `retainedBoot`).
- Any other `src/` change, `docs/`, `lib/`, `project.properties` or Device OS.
- Commits, pushes, merges, branch changes, stash, reset, checkout or restore. Device or network access beyond the Particle compile. Deleting anything you didn't create, including `build-tmp/` itself.

**SIZE BUDGET:**
- **`src/`:** **≤ +20 net non-move lines, HARD**: moved lines count 0, reported separately as gross; every new header line, include, namespace line and guard line counts. If two file pairs don't fit, STOP and report the count: do not put the code into `HibernateCycle.cpp`.
- **Tests:** guideline +100; record the actual.

# Stage 6 dispatch: WO-2026-10-19-003 (Step 6 P-4: move the clock and hibernate helpers to `time/`)

**Goal, in plain language:** move 93 lines of clock helpers out of `Generalized-Core-Counter.cpp` (the AB1805 wake-reason name lookup and its wrapper, and `logTimeDiag`, whose v41 log-on-change logic must move byte-for-byte) and 85 lines of hibernate-eligibility helpers out of `State_Sleep.cpp` into `time/`, **unchanged**. No behaviour change. This is the first move that shrinks `State_Sleep.cpp`.

**Repository:** this worktree, branch `wo/2026-10-19-003-step6-p4-time-helpers` (from main `768c3c0`, P-0 to P-3 merged). HEAD is at the commit that adds the WO and this dispatch. Don't switch branches. Main is Device OS 6.4.1; build with the 6.4.1 toolchain.

**Binding spec:** `docs/work-orders/WO-2026-10-19-003-step6-p4-clock-hibernate-helpers-move.md`, including **Claude Code's citation re-check at `768c3c0`** (the ranges, the Boron guard, the findings). Where this dispatch and the WO differ, the WO wins; report the difference.

## Do

1. Move the six functions byte-for-byte; allowed edits are linkage keywords (the three Sleep functions are internal today and become external, as does nothing else), declarations, includes, and the `#if PLATFORM_ID == PLATFORM_BORON` guard around the Sleep functions in the new `.cpp` and header. `ab1805WakeReasonNameLocal` is internal to main's anonymous namespace; in the new file it may stay internal with the external wrapper defined beside it, as before.
2. The new files include `state/StateMachine.h` for the `ab1805` extern (rule 4) and whatever the moved bodies already used.
3. **No `retained` keyword in any new file** (standard negative check since P-2): the new structural test, modelled on `tests/report_publisher_move_structural_test.py`, must fail if the keyword (after stripping comments and string literals) appears in either new `.cpp` or `.h`; it must also check no moved function is still defined in main or `State_Sleep.cpp`, each is defined exactly once, and `HibernateCycle.cpp` is unchanged.
4. Re-point the tests the WO's re-check lists, and any others a bare-name search finds, to the new homes with the same assertions.

## Verify (all required)

- **Byte-identical bodies:** every moved range at `HEAD` against its new home, including doc comments and the Boron guard lines. Whitespace carried byte-for-byte from the original is waived (P-3 rule); whitespace on a new line fails `git diff --check`.
- **Golden payload test** unchanged; **full suite** before and after, `N/N (sh via zsh, py via python3)`; the baseline at `HEAD` is 86. Run shell tests with `zsh`. No copy of `src/` under the worktree while the suite runs.
- **Mutations, each in a scratch copy, each must fail a named test (or be recorded as already unpinned at `HEAD` if it passes there too):** (1) make `logTimeDiag` log on every call (remove the v41 on-change condition); (2) remove the AB1805 stale-alarm clear (`clearAb1805StaleAlarmInterrupts` returning without clearing); (3) a `retained` probe in each new `.cpp` and each new `.h`.
- **Local ARM release build** (6.4.1 Boron; a fresh `BUILD_PATH_BASE`): text/data/bss before (from `git archive HEAD`) and after; explain any text change over ±64 B by symbol with `nm -C -S`; list any new static-initialization stub and what it constructs. **Retained check under the narrowed rule:** the whole `.backup` section before and after; `retainedBoot` stays at `0x2003f420`, the signature word at `0x2003f494`, **and the hibernate fields (`HibernateCycle::…::retainedHibernateRtcBefore/RequestedSleep/Count/Pending`) at `0x2003f480`, `0x2003f488`, `0x2003f48c`, `0x2003f490`**; list any move and classify it.
- **`wc -l`** for `Generalized-Core-Counter.cpp` (before 1596) and `State_Sleep.cpp` (before 1814).

## Implementation Report (required, as your final message)

- **Files:** new and changed, with net and gross `src/` lines and the arithmetic against +20; test lines added and removed; each re-pointed test and why it is not weaker.
- **Linkage, guard and placement choices**, with file:line.
- **Results:** the byte-diff, the suite before and after, the mutations and the tests that caught them, the build sizes and symbol-level text deltas, the full `.backup` table with the hibernate fields' addresses.
- **Confirmations:** you removed only your scratch directory; `HibernateCycle.cpp` is unchanged.
- **Deviations** (or "none"), and **the model used**.
