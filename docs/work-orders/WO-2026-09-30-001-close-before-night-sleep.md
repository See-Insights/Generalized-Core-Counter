# WO-2026-09-30-001: never sleep for the night with the close still due

**Goal, in plain language:** the device never commits to night sleep while the daily close is due. If it is due, the device runs the report first, and that report does the close.

**Status:** Opened 2026-09-30. Stage 5 decided by Chip and the architect (the opening dispatch). Stage 7 VERIFIED (2026-09-30; round 1's size finding accepted, budget raised to about 35 lines, reason recorded). Stage 8: commit, local release build, bench on Dev-09.

**Workflow:** Per `AI_DEVELOPMENT_WORKFLOW.md`, including §12: a plain goal, a size budget, and the two-round rule.

**Branch:** `wo/2026-09-30-001-close-before-night-sleep`, from `main` at `c2fb35c` (v27 merged, PR #51).

**Version:** `v28-CloseBeforeSleep`, product version 28.

## Problem (evidence, 2026-09-29/30)

Dev-14 on v27 missed its 22:00 SGT close:
- Its occupancy report at 21:50:57 started a connection attempt with no cellular registration.
- The 660 s budget ran out at about 22:02, and `connect-timeout` led straight to closed-hours hibernate at 22:01:59.
- No report ran after 22:00, so no closing report was made and the count didn't reset. The 06:00 report showed 39, not 0.

A read-only trace (Claude Code, 2026-09-30) found that the "is the close due" test exists only inside `handleReportingState()` (`src/state/State_Report.cpp:50–72`). The night-sleep commitment in `handleSleepingState()` (`src/state/State_Sleep.cpp:969–971`, `parkOpenness == Closed`) never asks it.

Every route into `SLEEPING_STATE` that doesn't pass through a report after the boundary therefore hibernates with the close pending:

| Route | Where |
|---|---|
| Connection timeout | `State_Connect.cpp:718` (`connect-timeout`, budget up to 660 s at `:123`) |
| Return to sleep after a report that finishes connecting after the boundary | `State_Connect.cpp:656` |
| Idle's park-closed and low-power idle | `State_Idle.cpp:124`, `:253` |
| A motion wake while closed | `State_Sleep.cpp:1735` skips the opportunistic report; `:1757` returns to sleep |
| Occupied at close, outside keep-alive mode | The session-end report at `State_Sleep.cpp:1596–1616` runs only in keep-alive mode (`reportNow`) |
| Thrash guard | `ThrashGuard.cpp:143` |

All of them reach the single night-sleep decision at `State_Sleep.cpp:971`.

## Change

**Size budget: about 25 lines of new `src/` code.** The moved due-test lines are counted separately and must move unchanged. Going over means stop and report.

**Budget raised to about 35 lines (Chip, 2026-09-30, after Stage 7 round 1):** "New owner file pair (DailyBoundary.h/.cpp) is planned Phase 3 structure; the behavioral change is the 5-line guard. The logic is moved, not new." Folding `DailyBoundary` into `time/Clock.cpp` to save lines was rejected: it would put a new responsibility into the file already flagged as the next "gravity well". A small, separately named owner is the structure Step 5.5 and Phase 3 aim for. Budgets catch mechanisms growing out of control; here the growth is scaffolding for a planned owner.

1. **New owner, `DailyBoundary`** (a small new file pair, e.g. `src/time/DailyBoundary.h` / `.cpp`). **Move, unchanged,** the existing due-test from `State_Report.cpp:50–72`:
   - the trusted-clock gate (`Clock::isTrusted()`);
   - `open == close` normalized to `close = 24`;
   - the live `localTodayAt()` (moved from `State_Report.cpp:27`);
   - `if (now < boundary) boundary -= 86400`;
   - `due = (lastDailyCleanup < boundary || lastDailyCleanup > now)`.

   It returns `{due, boundary, close}` from `DailyBoundary::check(now)`. The "Daily boundary reached ..." log line stays in `handleReportingState()`, so the report's log is unchanged. `handleReportingState()` calls `DailyBoundary::check(now)` and otherwise behaves exactly as today: same local names (`due`, `boundary`, `close`), same order, same writes.
2. **`State_Sleep.cpp:971`:** when `parkOpenness == Clock::Openness::Closed` **and** `DailyBoundary::check(Time.now()).due`, `transitionTo(REPORTING_STATE, "close due before night sleep")` and return, instead of committing to night sleep. Nothing else in the sleep path changes.

**Why there's no loop:** the report calls the same `DailyBoundary::check()`. When it's due, the report closes the day and stamps `lastDailyCleanup = now`, so the next pass through `:971` finds the close not due and sleeps for the night.

**Untrusted clock:** `Clock::openness()` returns `Unknown`, not `Closed`, and `DailyBoundary::check()` returns not-due, so the new branch can't fire.

## Tests

- **Update `tests/daily_cleanup_boundary_test.py`:** it pins the due-test's source text inside `handleReportingState()`. Point its source-shape checks at `DailyBoundary` for the moved logic, and at `handleReportingState()` for the call and the unchanged order (publish at `boundary - 1`, then `dailyCleanup()`, then `set_lastDailyCleanup(now)`). Keep its acceptance model and every existing assertion's intent. The item-A assertion (`else if (due)` → `CONNECTING_STATE` before the keep-alive and cadence branches) must still pass.
- **Add** a check that `handleSleepingState()`'s `Closed` branch calls `DailyBoundary::check(...)` and transitions to `REPORTING_STATE` before committing to night sleep. A structural test is acceptable. Removing the condition must make it fail.
- Update any other test **only** if it pins text this WO changes, and report each one. `tests/publish_with_ack_structural_test.py` must pass unchanged.

## Acceptance (Stage 7, narrow)

1. **Equivalence:** the `DailyBoundary` result (`due`, `boundary`) is identical to the old inline logic over the existing test cases, the acceptance model in `daily_cleanup_boundary_test.py`.
2. **Each traced route reaches the report before night sleep when the close is due:** a host check for each of connection timeout, return-to-sleep after a report, Idle's park-closed, a motion wake while closed, and occupied at close in intermittent mode.
3. **No loop:** after that report, the close isn't due, and the device goes to night sleep.
4. **Untrusted clock:** the close isn't due, and the device goes straight to night sleep.
5. **Mutation:** removing the `:971` condition makes a test fail.
6. **Suite:** passes (sh via zsh, py via python3); the `WITH_ACK` structural test is still green.
7. Version `v28-CloseBeforeSleep`, product 28; the `src/` size budget is met.

## Bench (after the flash, Chip)

On **Dev-09**, whose connectivity is better than Dev-14's:
1. Set `closeHour` to the next hour.
2. Trigger the sensor 2–3 minutes before the close, so the device is occupied at close.
3. Check that a closing report stamped `boundary − 1` arrives, followed by a report at 0.

## Approval record

- [x] Stage 5: Chip and the architect, 2026-09-30, in the opening dispatch (goal, the `DailyBoundary` owner plus the `:971` condition, the ~25-line budget, the Stage 7 checks, the version, the bench, and the routing: one Copilot round `claude-opus-5` medium, one narrow Stage 7 Codex `gpt-6-astra` high; not authorized: commits, flashing).
- [x] Implementation (Stage 6) — 2026-09-30, Copilot `claude-opus-5`, reasoning medium. New `src/time/DailyBoundary.h`/`.cpp` (the due-test and `localTodayAt()` moved from `State_Report.cpp`, 45 lines byte-identical); `handleReportingState()` calls `DailyBoundary::check(now)` with the same locals, order and log line (+13/−40); `State_Sleep.cpp` +7: the guard at the `Closed` branch, before `onEnterSleep()`. About 26 new code lines against ~25. Tests: `daily_cleanup_boundary_test.py` retargeted (+59/−15); new `close_before_night_sleep_test.py`; three more tests that pinned the changed text updated and reported (`clock_trust_standard_structural_test.py` site path, `sleep_duration_consumes_openness_test.py` brace-window check made structural, `loop_stage_sleep_prep_exclusion_test.py` transition count 14 → 15). Mutation: removing the guard fails the new test. Suite 45/45 (sh via zsh, py via python3). Local ARM release 150308 / 1090 / 2196 (+88 text); `v28-CloseBeforeSleep` present, no `pdiag`. Deviations reported: the log line re-reads `lastDailyCleanup` (output identical); the now-unused `LocalTimeRK.h` include left in `State_Report.cpp`. Report: `WO-2026-09-30-001-stage6-copilot-report.md`.
- [x] Codex verification, narrow (Stage 7) — round 1, 2026-09-30, `gpt-6-astra`, reasoning high: **NOT VERIFIED, on the size budget only.** Checks 1–6 pass. **(1)** Equivalence: all 8 acceptance-model calls give identical `due`/`boundary` to compiled v27, including an untrusted clock; the moved code is byte-identical; the report tail, log text and item-A trigger are unchanged. **(2)** All five routes, via a compiled extraction with hardware stubbed and sleep gates assumed released, reach `REPORTING_STATE` ("close due before night sleep") before `onEnterSleep()`, `secondsUntilNextOpen()` or HIBERNATE. **(3)** No loop: the report stamps `lastDailyCleanup = now`; the next pass is not-due and reaches HIBERNATE. **(4)** Untrusted: not-due, and `Unknown` openness takes the short-nap path. **(5)** Mutation caught. **(6)** 45/45 (sh via zsh, py via python3); `WITH_ACK` test byte-identical and green; the other test changes are permitted adaptations. **(7) FAIL, size:** excluding blanks, comments and the 3 version lines, the net `src/` code delta is **+35** (`State_Report.cpp` −22, `State_Sleep.cpp` +5, `DailyBoundary.cpp` +39, `DailyBoundary.h` +13) against about 25. Stage 6's figure of 26 counted 9 lines as moved whose originals remain. Identity and build pass: 150308 / 1090 / 2196, `v28-CloseBeforeSleep`, product 28, no `pdiag`; ELF shows both handlers call `DailyBoundary::check()`. Working tree byte-identical. Verdict: `WO-2026-09-30-001-stage7-verdict.md`.
- [x] Stage 7 closed (Chip, 2026-09-30): **VERIFIED.** The size finding is accepted and this WO's budget raised to about 35 lines, for the reason recorded under Change. Noted: Codex caught Copilot understating the size (it counted 9 lines as moved whose originals remain), which is the independent check working.
- [ ] Chip final gate / commit (Stage 8)
- [ ] Bench on Dev-09
