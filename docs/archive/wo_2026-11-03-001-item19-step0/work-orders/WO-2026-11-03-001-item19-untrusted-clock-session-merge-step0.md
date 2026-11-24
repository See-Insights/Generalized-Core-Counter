# WO-2026-11-03-001: an untrusted clock merges separate sessions and credits the whole span (recovery item 19): Step 0 (read-only)

**Author:** Claude Code, 3 Nov 2026, DRAFT for the architect's quick review · **Status:** APPROVED to dispatch by the architect (3 Nov) with the amendments below folded in
**Branch:** `wo/2026-11-03-001-item19-step0`, from `main` (`21804e3`, v48)
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 4 (independent investigation); §5; §12.1, §12.5; §13 (c), (j)
**Agent:** Codex `gpt-5.6-sol`, **high**, read-only (`-s read-only`), stdin closed (`</dev/null`), new output file, report to `docs/work-orders/WO-2026-11-03-001-step0-report.md`, **at most 150 lines** (do not flag overages).

## Plain goal
**Goal of the fix this Step 0 scopes:** measure a session's duration with `millis()`, so clock corrections (an untrusted clock becoming trusted, an NTP/cloud sync jump, an RTC reset) can neither merge separate sessions nor stretch one. The wall clock only assigns the day.

While the clock is untrusted, an open occupancy session is kept open, nothing is credited and the debounce is re-armed, so the separate sessions of a pulsed site merge into one. When trust returns, the first trusted close credits the whole span, gaps included. Dev-09 (v45, 23 Oct) credited 205 min where about 97 were real. This Step 0 finds out **what a fix can safely do**, with no code change. Item 18 (the failsafe can act sooner) and #151 (Unknown counts as open) are already on `main`; this item is what remains.

## Evidence already in hand (recovery item 19, read it first)
- Dev-09 S3 `serialLog`, 23-24 Oct: session opened 10:35:21Z, stayed open through the 10:40:55Z restart until the first trusted Report at 01:00:07Z on 24 Oct, closed at the boundary, credited 205 min into the 23 Oct total (`dailyoccupancy` 345 → 550).
- The boot cap (B3: earliest of the boundary, the boot time, last report + debounce) did not bind because the boundary was the earliest.
- Mechanism: `closeOccupancySessionSafely` keeps the session open and re-arms the debounce while untrusted (`State_Common.h:341-349`, item B2 of WO-2026-10-04-001); the credit is `close time - start time` (`State_Common.h:350-395`).
- Since then: WO-2026-10-29-002 (occupancy ends at close; Unknown openness counts as today, "item 19's fix stays separate"), #151, and v48 are on `main`. **Re-read every cited line on `21804e3`;** line numbers in the plan were taken on older commits.

## Questions (observation and inference marked separately; cite file:line)
1. **Current behavior.** Walk `closeOccupancySessionSafely`, the B2/B3 rules, the boundary close and the reset-time paths on `main`. For an untrusted clock, which values are wall-clock and which are `millis()`? Where is the session start stored (retained? persisted?) and what happens to it across a reset, a hibernate and a power cycle?
2. **Has anything since 23 Oct changed the failure?** Show whether item 18's failsafe, #151 or WO-2026-10-29-002 shortens, removes or shifts it. Reproduce it on paper with the Dev-09 timeline.
3. **What can `millis()` vouch for?** Within a boot, it is monotonic and independent of clock trust. Across a restart it resets. Design question: measure a session's duration with `millis()` within a boot and use the clock only to assign the day. What does a session that **straddles a reboot** get credited (on-time only? the debounce window? nothing)?
4. **Can the debounce expiry run on `millis()`** so sessions are separated while untrusted? What does that change in the sleep plan (`State_Sleep.cpp`), the wake-source logic and the report path? Note the interaction with the debounce-uses-remaining-time fix (WO-2026-10-25-001).
5. **History.** Why B2 chose "keep it open" (WO-2026-10-04-001): which loss it avoided (discarding a real session), and whether a `millis()`-based credit preserves that guarantee.
6. **Candidate designs**, each with: behavior on the Dev-09 timeline, behavior on a clean (trusted) day (must be unchanged), what is lost or over-credited in the worst case, estimated `src/` lines, tests that exist and would need changing (B2/B3 tests, report-path tests), and the `retained`/persisted-layout impact (Step 7 rules: none unless the architect says so). Include at least: (a) credit measured on-time only; (b) credit capped at a bound derived from `millis()` and the debounce; (c) split the session at each untrusted debounce expiry and credit on-time per piece.
7. **Interaction with item 21** (clock trust surviving a restart, withdrawn on evidence) and **item 10/15**: which of those must land first, and which design choice they constrain.
8. **Bench plan.** The exerciser pattern (570 s session every 20 min, ~47.5 % duty) can reproduce it: what the bench needs (untrusted clock via no cloud + RTC state, a reset mid-session) and the pass numbers (real minutes vs credited).
9. **Stop decision:** STOP if no design can credit on-time without possibly discarding a real session; say why. Otherwise PROCEED, ranking the designs and naming the smallest WO shape (≤ +20 `src/` per WO, removals count).
10. **Source map (file:line, observation).** Every place the session **start** time and **end** (close) time are written, read and persisted (`occupancyStartTime`, last-PIR time, debounce deadline, close time), and for each: wall-clock or `millis()`, retained or persisted, and what resets it.
11. **The untrusted-clock-to-merge path.** Trace, step by step with file:line, how an untrusted clock leads to separate sessions merging (B2 re-arm) and the whole span being credited at the first trusted close; and the exact line where the credit `close - start` is computed.
12. **Does `millis()` hold across each sleep mode a session spans?** For each of STOP/ULP sleep (the debounce sleep with an open session), network standby, hibernate, `System.reset` (failsafe, planned, watchdog), and deep power-down: does `millis()` keep counting, stop or restart (cite Device OS source or docs and GCC's own notes), and what does the session start use afterward?
13. **Close and reset cases.** The normal debounce close, the boundary close (hour/day), a close at an untrusted clock, a reset mid-session (start retained or lost), and a reset between the close and night sleep: what is credited today and under each candidate design.
14. **Does item 15 (occupancy lost across a reset) affect the counts?** State whether a lost or retained occupancy changes the credit under the current code and under each design, and whether item 15 must be fixed first or can ride with the item-19 fix.
15. **Fix budget.** Each candidate design is sized against **≤ +20 net `src/` lines per WO** (removals count); if a design cannot fit, propose the split.


## Constraints
Read-only: no code, no device, no commit, no network. Report states the model and reasoning level actually used and the interpreter line if any tests are run (`sh via zsh`, `py via python3`). Observation (source read, with file:line) and inference are kept apart; every design claim about the Dev-09 numbers is recomputed from the plan's figures, not assumed.

## Out of scope
Any change to `src/`, the persisted layout or `retained`; the failsafe (item 18); the clock-trust rule itself (item 21); the report format.

## Approval record
Architect review: approved 3 Nov, subject to the amendments in questions 10-15 (goal, source map, merge path, `millis()` across sleep modes, close and reset cases, item 15, ≤ +20 budget) · Chip authorizes dispatch: per the relay of 3 Nov
