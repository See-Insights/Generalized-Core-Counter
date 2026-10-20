# WO-2026-10-20-003: Sleep S-1 — extract the teardown from `handleSleepingState` (behavior-neutral)

**Author:** architect, 2026-10-20 · **Status:** ready (citation re-check first)
**Branch:** `wo/2026-10-20-003-sleep-s1-teardown`, from main **after the v45 release branch is cut** (v45 = P-0 to P-6; the sleep moves ship later, as v46)
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.2–12.4; §13 (b), (c), (g), (j)
**Spec source:** `WO-2026-10-20-002-step0-report.md` (round 1) §Q2–Q3 and sequence item 1; `WO-2026-10-20-002-step0-addendum-report.md` (round 2) Q9 row 3 and sequence item 1; architect's rulings of 20 Oct (sequence accepted; no NAP/NIGHT; no behavior fixes in the split). Cited at `a3e012d`; re-check every range.

## Plain goal

Move the cloud-disconnect and modem-teardown phase out of `handleSleepingState()` into its own function and file, `state/SleepTeardown.{h,cpp}`, that owns its state. **No behavior change:** the same requests, waits, budgets, alert 15, `ModemHealth` calls, timings, log lines and transitions, pass by pass. This is the first of six sleep WOs; it isolates the field-risk teardown first.

## What moves (round 1, at `a3e012d`; re-check every range)

- The teardown logic `State_Sleep.cpp:577–843` (disconnect request and wait, modem-off and final precondition, budgets, alert 15, the `ModemHealth` calls).
- The labels and context helpers `:33–86` (`connectionModeLabel`, `batteryTierLabel`, `logTeardownContext`).
- The teardown state, nine statics: `cloudDisconnectCompleteLogged`, `radioOffCompleteLogged`, `disconnectRequested`, the four request and elapsed timestamps, `lastDisconnectProgressMs`, `lastGateProgressMs` (`:197`, `:204`, `:293–299`), with their resets on entry (`:216–219`, `:308–312`, `:316–317`) and the related lines `:326–335`. About 349 lines leave the sleep file.

## Design (do not widen)

1. `SleepTeardown` owns those nine fields as one file-static struct in `SleepTeardown.cpp`. No new globals, no `retained`, no framework.
2. It exposes: a reset on state entry (today's resets, unchanged), one pass function returning today's three outcomes (WAIT, READY, ERROR) with the same side effects, and a read-only query for `disconnectRequested` for its other readers (the firmware and occupancy priority checks and the cloud-gate ordering, round 1 §Q3). **No other phase writes that field.**
3. `handleSleepingState()` calls the pass function where the block was and acts on its result exactly as the inline code did (the same returns and the same transitions).
4. The standby-return fields (`:199–201`) are **not** part of S-1; they move with S-2. S-1 reads them as today.
5. `ModemHealth` stays the only writer of the latch fields.

## Equivalence (freeze the output, not the code; the P-5 method)

- A host harness drives the real `handleSleepingState()` through the teardown scenarios with fake collaborators (Particle cloud, `Cellular`, `Connectivity`, the clock, the queue, the watchdog), round 1 §7.
- **Record the transcript once from the pre-move tree (`a3e012d` or the base you branch from), commit it as `tests/fixtures/sleep_teardown_transcript.txt` with that commit hash in its header,** then run the moved code against it.
- **Scenarios:** cellular and Wi-Fi × cloud connected or offline × modem on, off or in between × standby or not × elapsed before and after each budget. **Freeze, per pass:**
  - the Connectivity calls;
  - the WAIT/READY/ERROR result;
  - alert 15;
  - the transitions and reasons;
  - the log lines (character for character);
  - the session timings;
  - the watchdog and `Particle.process` calls.

## Budget

- **`src/`: ≤ +20 net non-move lines, hard** (round 1 estimated +14). If it goes over, stop and report; don't widen the scope.
- Tests: the harness is new, so the guideline is +175. Record the actual; flag it at about twice the guideline.

## Tests and checks

- The transcript matches exactly; golden payload test unchanged; suite `N/N`, serial.
- Existing teardown and latch tests pass, re-pointed, not weakened (`standby_latch_daily_clear_test`, `conn_phase_modem_off_test`, the modem-off and teardown logging tests).
- Structural: none of the nine fields is defined or written outside `SleepTeardown.cpp`; no `retained` in the new files (negative checks for both).
- **Mutations:** change the modem-off budget by 1 s; drop alert 15 on the timeout; skip the final `Cellular.isOff()` precondition. Each must fail the transcript or a named test.
- `nm` retained check (narrowed rule); ELF delta explained by symbol (round 1 expects a small call and layout change).
- Whitespace carried byte-for-byte is waived; new lines must be clean.

## Agents and models

- **Stage 6:** Copilot, Sonnet tier, **high**. Statics become owned state with a query, so it's not a pure move, and the harness is new.
- **Controller (before Stage 7, §13 (g)):** regenerate the transcript from the base and confirm it is byte-identical to the fixture; reproduce the build, `nm`, the mutations and the suite, serially.
- **Stage 7:** Codex `gpt-5.6-sol`, **high**. Checks: the transcript's provenance and coverage of every budget edge; every moved range byte-identical apart from field access; one owner for `disconnectRequested`; the same returns and transitions at the call site.

## Bench

None in this WO. Ships in v46 with S-2 to S-6; the v46 soak compares teardown timings and `MODEM_*` lines with v45.

## Closing record

Budget versus actual (non-move `src/`, moved lines, sleep `wc -l`, tests), the transcript hash and provenance, ELF by symbol, retained outcome.

## Claude Code's citation re-check (2026-10-20, main `0c565fa`; `State_Sleep.cpp` is byte-identical to `a3e012d`)

**Branching and merge (architect, 20 Oct):** the branch is cut now, from main `0c565fa`, and the PR is opened after Stage 8 marked **"do not merge until v45 is cut"** (the WO header's "after the v45 release branch is cut" is for the merge, not for starting). The sleep moves ship as v46.

**Every site holds.** The nine statics at `:197` (`cloudDisconnectCompleteLogged`), `:204` (`radioOffCompleteLogged`), `:293–299` (`disconnectRequested`, `disconnectRequestStartMs`, `cloudDisconnectStartMs`, `cloudDisconnectElapsedMs`, `modemOffElapsedMs`, `lastDisconnectProgressMs`, `lastGateProgressMs`); their entry resets at `:216–219` (the radio-off flag inside `#if Wiring_Cellular`), `:308–312` and `:316–317`; the safety reset `:326–335`; the teardown block `:577–843` (stillOn at `:579`/`:585`, the budget lambda `:593`, `computeDisconnectBudgetMs()` called at `:620`, request/start `:625–668`, the progress and primary timeout `:683–724` with `markModemUnstable` `:699` and `raiseAlert(15)` `:701`, `ModemHealth` `:745–747`, `sleepPreconditionsSatisfied` `:762–765` and its use `:771`, the final timeout `:820–821`, `serviceAwakeWatchdog()` + `Particle.process()` at `:722–723` and `:840–841`).

**Findings the implementer must plan for (not WO changes):**
1. **Do not move `logWakeSummary` (`:63–70`).** The WO's `:33–86` range includes it, but it belongs to S-6. Move only `connectionModeLabel` (`:33–46`), `batteryTierLabel` (`:48–61`) and `logTeardownContext` (`:72–86`); all three are used only inside the teardown block (`:640, 652, 658, 737–738, 779–798`) and by `logTeardownContext` itself. Moving them is a pure move (about 54 lines).
2. **The block writes two standby-return fields that stay in Sleep until S-2:** `forceModemOffAfterStandbyFailure` and `standbySleepFailureError` are read and cleared at `:632–639` (declared `:200–201`, also written at `:1290–1305`). The pass function must reach them without moving them: pass references or a small input/output struct from `handleSleepingState()`. Its inputs also include the locals `useNetworkStandbyRequested` and `useNetworkStandbyEffective` (`:228–250`) and the budget, which stay. These parameter lines are non-move lines; count them.
3. **Other readers of `disconnectRequested` outside the block:** `:342`, `:353`, `:360` (the firmware and occupancy priority checks and the gate ordering). They use the read-only query. The entry resets at `:308–312` and `:316–317` sit among the cloud-gate resets (`:313–315`, S-3): move only the teardown fields' lines and leave the gate's.
4. **Existing tests that parse `State_Sleep.cpp`:** 34 test files name it. The ones that read teardown text (for example `modem_teardown_confirmation_logging_test.py`, `sleep_gate_modem_off_test.sh`, `modem_health_owner_structural_test.py`, `standby_latch_daily_clear_test.{sh,cpp}`, `power_consumer_regression_test.sh`, `sleep_breadcrumb_sequence_test.py`) will need re-pointing to `SleepTeardown.cpp`; find them by running the suite, re-point, and do not weaken any.
5. **The harness cannot compile `handleSleepingState()`** (`tests/README.md:52–58`). Compile the block as it stands at the base, extracted verbatim by marker text from `git show <base>:src/state/State_Sleep.cpp` (not by line number) inside a thin wrapper with fakes, and the new `SleepTeardown.cpp` against the same fakes, driven by one scenario table. The recorded transcript (the P-5 method: freeze the output, not the code) is committed as the fixture with the base hash in its header; a generator mode regenerates it from the base so the controller can confirm it matches.

## Stage 6 result and controller reproduction (2026-10-20)

**Stage 6** (Copilot, Sonnet 5.5, high): one round. `SleepTeardown.{h,cpp}` (7 + 352 lines) owns the nine statics as one file-static struct; `State_Sleep.cpp` 1670 → 1331 (+9 / −348); the standby-return fields are passed by reference; `logWakeSummary` stays. **Net `src/` +20 counting blank lines (+18 non-blank): at the hard limit, no headroom.** Tests about +793 lines (4.5× the +175 guideline) plus a 4,753-line fixture (about 230 KB): flagged. **Controller reproduction** (Claude Code, 6.4.1 Boron, fresh `BUILD_PATH_BASE`, `-frandom-seed=p6`; "before" is `git archive HEAD`):
- **Transcript regenerated from the base (`0c565fa`) with `--regenerate` is byte-identical to the committed fixture** (174 scenarios, 1,065 passes); the normal run (a) base block and (b) new module both match it.
- **Suite 93/93** (sh via zsh, py via python3, serial; baseline 91); `git diff --check` clean.
- **Build:** text/data/bss `151724/1094/2228 → 151532/1094/2228` (−192); the nine function-static symbols are replaced by one 28-byte struct; `handleSleepingState` 6968 → 5552; static-init stubs 22 before and after; **`.backup` unchanged** (size `0x98`, 25 retained symbols at the same addresses; only the flash load address moved, `0xd9430 → 0xd9370`): retained outcome A.
- **Mutations I ran:** budget +1 s, alert 15 dropped on the primary timeout, and the final `Cellular.isOff()` skipped each fail the transcript; a teardown-field write in `State_Sleep.cpp` and a `retained` probe each fail the structural test. **Dropping alert 15 on the precondition timeout survives, and the harness says so ("unpinned (dead branch)")**: after the primary block and the completion reset, `disconnectRequested` is false on arrival at the precondition block, so its timeout branch (`SleepTeardown.cpp:321–322`) can only run if the radio state flips between two reads in one pass. This is dead code in the base, not a gap in the transcript.
- **Deviation to check in Stage 7:** the two old entry resets (`State_Sleep.cpp:216–219` before the CONNECTED+OPEN abort, and `:308–317` after it) became one `SleepTeardown::enter()`, so on the abort path all nine fields now reset where two did. Nothing reads them there and the next entry resets them again, but the transcript cannot see this path.

## Architect's rulings on Stage 6 (20 Oct)

- **Review from the WIP branch:** the implementation is pushed as `658b7cf` on `wip/s1-sleep-teardown` (no PR); Codex reviews it there. Stage 8 brings it to this branch by fast-forward.
- **The precondition-timeout branch stays in S-1** (alert 15, `sleep-precondition-timeout`; dead in practice, `SleepTeardown.cpp:321–322`). It is a **removal candidate for the post-split simplify series** (recovery plan item 17).
- **The consolidated entry reset is accepted if Codex confirms** that none of the seven newly reset fields is read on the CONNECTED+OPEN abort path before its next reset; **otherwise STOP.**
- **Test volume (about +793 lines and the 4,753-line fixture) is accepted for S-1.** **S-2 to S-6 extend this harness** (new scenarios and fixtures on the same runner) and do not build new ones; each later WO says so.
- **Stage 8 is pre-approved** on VERIFIED with net `src/` ≤ +20; the PR is marked **"do not merge until v45 is cut"**.


## Stage 7 result and rulings (20 Oct)

**Stage 7** (Codex `gpt-5.6-sol`, high, on `658b7cf`): **NOT VERIFIED** on F1 only (`WO-2026-10-20-003-stage7-verdict.md`): the firmware-update ordering check was anchored on the first direct `Connectivity::request*` call in `handleSleepingState()`; after the move that call is inside `SleepTeardown::run()`, so a guard moved below `run()` passed 93/93. Checks 2–7 and 9 PASS, check 1 CONCERN (the transcript fakes `ModemHealth`; the 4999/5000/5001 boundary is pinned at `modem_health_equivalence_test.cpp:70`: accepted), retained outcome A; the seven-field entry-reset check PASS. F2 (the dead precondition-timeout arm cannot be pinned) is a removal candidate (recovery plan item 17), no action.
**Rulings:** F1 gets a test-only Copilot follow-up (Sonnet, medium): re-anchor `firmware_update_wiring_structural_test.py` on `SleepTeardown::run(` and make `firmware_update_dwell_test.sh` keep the real source order; no `src/` change. Controller reproduction per §13 (g) (`src/` hash match, so no ARM build and the baseline ELF stands), then a Codex recheck limited to check 8 and the F1 mutation. Stage 8 stays pre-approved on VERIFIED with the `src/` hash unchanged and net `src/` ≤ +20; **no third round**. Each later Step 0 (S-2 to S-6) lists the tests anchored on text or line order in the file being moved (WO-2026-10-20-002 ruling 8).
**Follow-up result (commit `8de7ddc`):** `src/` tree hash `7fc57df7a302712ea3f26ff37758ce455aaa6203808a7092d11636f5d06d190a` unchanged; the structural test's boundary is the earliest of `SleepTeardown::run(` and the direct teardown calls; the dwell test derives the order of the guard, the cloud gate and `SleepTeardown::run(` from the real text (+47/−17 test lines in all). **Controller reproduction:** the guard moved below `run()`, and separately above the later `Connectivity::request` calls, fails both tests; the real code passes both; suite **93/93** (sh via zsh, py via python3, serial); `git diff --check` clean.
