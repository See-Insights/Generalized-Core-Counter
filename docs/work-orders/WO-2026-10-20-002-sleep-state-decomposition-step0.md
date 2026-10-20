# WO-2026-10-20-002: decompose `handleSleepingState` (Step 0 only)

**Author:** architect, 2026-10-20 · **Status:** Step 0 (read-only); no implementation until the architect accepts a sequence
**Branch:** `wo/2026-10-20-002-sleep-decomposition-step0`, from main (`a3e012d` or later)
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 2–4; §12.1 (history first); §12.2–12.4; §13 (b), (c), (g), (j)
**Sources:** `docs/architecture-review-2026-09-03.md` (the sleep/time/persistence coupling); `WO-2026-10-15-003-step0-report.md` Q7 and open points (the gate and teardown share cross-pass statics); recovery plan items 14 (breadcrumb-28 sleep stall), the standby latch on PCKL1, and the Dev-14 post-failsafe sleep loop.

## Plain goal

`State_Sleep.cpp` is 1,670 lines, and `handleSleepingState()` alone runs from `:179` to `:1670`, about 1,490 lines, with about 28 function-local `static` variables (`:191–304`) carrying state from one loop pass to the next. Split it into named phases, each a function with an explicit, owned state, **without changing behavior**, so the fixes queued against sleep (the latch, the sleep stall, the post-failsafe loop) become small changes to one small function.

## Step 0 questions (report ≤ 150 lines on the WO branch, committed and pushed)

1. **Phase map.** Walk `handleSleepingState()` in execution order and name each phase with its line range and size. For example: wake summary; the cloud gate before sleep (queue, ledger, webhook, update); the cloud disconnect and modem teardown (budgets, alert 15, `ModemHealth` calls); choosing the sleep mode (ULP, STOP, hibernate, standby); the `System.sleep()` call (breadcrumb 28, watchdog pause and restore); wake handling (wake source, the PIR-wake block, timers); and the transition out. Correct this list to match the code.
2. **The implicit state machine.** The function is re-entered on every loop pass and returns early while waiting. Describe how: which conditions return, which statics say where it is, and what resets them. Is it effectively a sub-state machine, and if so, which states?
3. **State inventory.** For every function-local `static` (`:191–304` and any later ones): which phase reads and writes it, whether it must survive between passes, and where it's reset. Group them into per-phase state. Flag any shared between phases. Those are the hard part (Codex, 15 Oct: `:335–349, :397, :428–448` at `71e346f`).
4. **History first (§12.1).** Was this function ever split into phases or sub-states (`8aa2643`, the `2c19b08` split, earlier), and why was it merged back? Restoring an earlier split is the default if its reason no longer holds.
5. **Field-bug anchors.** Where exactly do these happen, by line: breadcrumb 28 and the `System.sleep()` calls; the watchdog pause and restore; the modem-off budget, alert 15 and the `ModemHealth` calls; the path behind the 15 Oct Dev-14 loop (modem-off over budget every 30 s after a failsafe reset); the hibernate decision. **Also: where would a hang leave no log line?** List the gaps that matter for recovery-plan item 14. This is an answer only, with no logging proposed.
6. **Sequence.** Up to **six** WOs, most risk-reducing first:
   - **behavior-neutral** throughout, with the golden payload test unchanged;
   - **≤ +20 net non-move `src/` lines each, hard;** a pure move counts 0;
   - each phase becomes a function that owns its state, as a file-static struct in the phase's own file, or a struct passed in. **No new globals and no framework;**
   - **equivalence proof** where code isn't a pure move: freeze the output, not the code (the P-5 method). Name the transcript each WO would record, the states and log lines for a fixed set of inputs;
   - the usual rules: no `retained` in new files, the narrowed retained rule, whitespace carried byte-for-byte is waived;
   - for each WO: the sleep-file lines removed and the expected ELF change.
7. **Testability.** Which existing host harnesses already reach parts of this function, and what's the smallest harness that would let each phase be tested alone?
8. **Stop and report** if the split can't be made behavior-neutral, needs a persisted field, or needs more than six WOs for a useful result. Say what the six-WO cut leaves.

## Standing rulings that carry over

- The PIR-wake block stays in `State_Sleep.cpp` (14 Oct) unless you show a clear reason to move it.
- No behavior fixes in this series: the latch, the sleep stall and the post-failsafe loop get their own WOs **after** the split, each with evidence.
- `ModemHealth` (P-5) stays the only writer of the latch fields.

## Agents and models

- **Step 0:** Codex `gpt-5.6-sol`, **high**, read-only. It's the largest and most stateful function in the firmware, and the sequence decides how safely we can change sleep later.
- **Controller:** Claude Code checks every file:line in the report against the tree before it reaches the architect (§13 (j)). No ARM build.

## Budget

Step 0: 0 `src/` lines. The eventual WOs: stated per WO when the architect accepts the sequence.
