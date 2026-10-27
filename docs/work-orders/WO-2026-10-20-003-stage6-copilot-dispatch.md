AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-20, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- **Edit:**
  - **new** `src/state/SleepTeardown.{h,cpp}`: the teardown phase and its owned state (file-static struct, no new globals, no `retained`);
  - `src/state/State_Sleep.cpp`: remove the teardown block, the nine statics with their resets and safety reset, and the three label helpers, and call the new module; **leave** `logWakeSummary`, the cloud-gate state and resets, the standby-return statics (`:199–201`) and everything else where they are;
  - `src/state/StateMachine.h` or another header **only** if a declaration must move (say why).
- **Tests:** add the equivalence harness, its scenario table and the committed transcript fixture `tests/fixtures/sleep_teardown_transcript.txt`; add the structural test; re-point every existing test that reads teardown text from `State_Sleep.cpp` (find them by running the suite) without weakening it; report each change.
- **Run** the host suite (serially) and the local ARM release build.
- **Scratch directory:** **`build-tmp/wo20261020-003-stage6/`**. Remove **only it** when you finish.

**Not authorized:**
- Any change to a request, wait, budget, alert, `ModemHealth` call, timing, log line (text, level or condition), result or transition, or to the order of any of them.
- Moving `logWakeSummary`, the standby-return fields, the cloud-gate state, or any `retained` definition; adding a `retained` keyword; any write to the latch fields outside `ModemHealth`.
- Any other `src/` change, `docs/` (except the fixture's header you write), `lib/`, `project.properties`, Device OS.
- Commits, pushes, merges, branch changes, stash, reset, checkout or restore. Device or network access beyond the Particle compile. Deleting anything you didn't create, including `build-tmp/` itself.

**SIZE BUDGET:**
- **`src/`: ≤ +20 net non-move lines, HARD** (round 1 estimated +14). Lines moved byte-for-byte count 0 (report them as gross); new header, include, namespace, wrapper, parameter-passing and query lines count, and so do the lines in the call site. Over means STOP and report (§13 (b)); do not widen the scope.
- **Tests:** guideline +175 (new harness); record the actual and flag it if it is about twice that.

# Stage 6 dispatch: WO-2026-10-20-003 (Sleep S-1: extract the teardown from `handleSleepingState`)

**Goal, in plain language:** the cloud-disconnect and modem-teardown phase of `handleSleepingState()` moves into its own module, `state/SleepTeardown.{h,cpp}`, which owns its nine statics. **No behaviour change:** the same requests, waits, budgets, alert 15, `ModemHealth` calls, timings, log lines and transitions, pass by pass. Not a pure move: statics become owned state with a query, and two standby-return fields are reached by reference, so equivalence is proven by a transcript, not only by a byte diff.

**Repository:** this worktree, branch `wo/2026-10-20-003-sleep-s1-teardown` (from main `0c565fa`; `State_Sleep.cpp` is identical to `a3e012d`). HEAD is the commit that adds the WO and this dispatch. Don't switch branches. Main is Device OS 6.4.1; build with the 6.4.1 toolchain.

**Binding spec:** `docs/work-orders/WO-2026-10-20-003-sleep-s1-teardown.md`, including **Claude Code's citation re-check at `0c565fa`** (every site, and five findings: do not move `logWakeSummary`; the block writes two standby-return fields that stay; the other readers of `disconnectRequested`; the tests that parse `State_Sleep.cpp`; the harness approach). Also `docs/work-orders/WO-2026-10-20-002-step0-report.md` (§1–§3, §6 item 1) and the addendum report (Q9 row 3). Where this dispatch and the WO differ, the WO wins; report the difference.

## Do

1. `SleepTeardown`: the nine statics as one file-static struct; a reset on state entry (today's resets, unchanged); **one pass function** returning today's three outcomes (WAIT, READY, ERROR) with the same side effects, taking `useNetworkStandbyRequested`/`useNetworkStandbyEffective` and references to `forceModemOffAfterStandbyFailure` and `standbySleepFailureError`; a **read-only query** for `disconnectRequested` (used at `:342`, `:353`, `:360`); the safety reset `:326–335` goes with it. Move the lambdas and the three label helpers byte-for-byte. Say which of its lines are non-move.
2. `handleSleepingState()` calls the pass where the block was and acts on the result exactly as the inline code did (the same `return`s and the same `transitionTo(...)`; ERROR keeps `transitionTo(ERROR_STATE, "sleep-disconnect-timeout")` / `"sleep-precondition-timeout"` semantics, wherever the call sits, with the same reasons).
3. **No other phase writes** the nine fields. No `retained` keyword in either new file (standard negative check since P-2: strip comments and string literals).

## Verify (all required)

- **Equivalence harness (new, host).** The full handler is not host-compilable (`tests/README.md:52–58`). Compile (a) the teardown block **extracted verbatim by marker text from the base** (`git show <base>:src/state/State_Sleep.cpp`, base = the commit recorded in the fixture header, default `0c565fa`; never by line number) inside a thin wrapper, and (b) the new `SleepTeardown.cpp`, both against the same fakes (Particle cloud, `Cellular`, `Connectivity`, the clock, the queue, the watchdog and `Particle.process`, `ModemHealth`, `RecoveryState`), driven by one scenario table. **Scenarios:** cellular and Wi-Fi × cloud connected or offline × modem on, off or in between × standby (requested and effective) or not × elapsed before and after each budget edge (the disconnect budget, the modem-off wait, the precondition wait, the slow-teardown thresholds around 5000 and 10000 ms) × a pending `forceModemOffAfterStandbyFailure`. **Freeze, per pass:** the Connectivity calls; the WAIT/READY/ERROR result; alert 15; transitions and reasons; log lines character for character; the session timings and the nine fields; `serviceAwakeWatchdog()`/`Particle.process()` calls. A generator mode records the transcript from (a); the fixture carries the base hash and the scenario count in its header; the run mode compares (b) to it and (a) to it too. Show the transcript is not trivial (the number of passes and distinct outcomes).
- **Structural test:** none of the nine fields is defined or written outside `SleepTeardown.cpp`; `State_Sleep.cpp` no longer contains the moved block or labels; no `retained` in the new files (a probe in each fails with a named message).
- **Byte-identical moved bodies** against the base (the lambdas, the labels, the block statements apart from field access and the call boundary); `git diff --check` clean on new lines (hits on moved lines are waived only where the original has the same whitespace).
- **Mutations (each in a scratch copy; each must fail the transcript or a named test, or be recorded as unpinned at the base):** (1) change the modem-off/disconnect budget by 1 s (`computeDisconnectBudgetMs`); (2) drop `RecoveryState::raiseAlert(15)` on the primary timeout (and separately on the final-precondition timeout); (3) skip the final `Cellular.isOff()` precondition (`sleepPreconditionsSatisfied`); (4) add a write to a teardown field in `State_Sleep.cpp`; (5) a `retained` probe in each new file. Scratch copies have no `.git`; judge them on the tests that run the teardown code, and note that a few structural tests need history.
- **Golden payload test** unchanged; **full suite** before and after, `N/N (sh via zsh, py via python3)`, **run serially, one suite at a time**; the baseline at `HEAD` is 91.
- **Local ARM release build** (6.4.1 Boron; fresh `BUILD_PATH_BASE`; before from `git archive HEAD`): text/data/bss before and after; explain any change over ±64 B by symbol with `nm -C -S`; list any new static-initialization stub. **Retained check under the narrowed rule:** the whole `.backup` section before and after; `retainedBoot` at `0x2003f420`, the hibernate fields `0x2003f480/488/48c/490`, the signature word `0x2003f494`.
- **`wc -l`** for `State_Sleep.cpp` (before 1670) and the two new files; the lines removed from Sleep (about 349 expected).

## Implementation Report (required, as your final message; ≤ 150 lines)

- **Files:** new and changed, with net and gross `src/` lines and the arithmetic against +20; test lines added and removed; each re-pointed test and why it is not weaker.
- **The transcript:** scenario and pass counts, distinct outcomes, the fixture header, and the old-versus-new comparison result.
- **Results:** the byte-diff, the suite before and after, the mutations and the tests that caught them, the build sizes and symbol deltas, the `.backup` table.
- **Confirmations:** you removed only your scratch directory; no request, wait, budget, alert, timing, log line or transition changed.
- **Deviations** (or "none"), and **the model used**.
