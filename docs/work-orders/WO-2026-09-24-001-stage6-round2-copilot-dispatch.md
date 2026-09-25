AGENT: Copilot · MODEL: gpt-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit the source and test files named below in the working tree, run builds and tests / Not authorized: commits, pushes, merges, branch changes, stash, reset, flashing, device settings, or any change outside the files named below.

# Stage 6 round 2 dispatch — WO-2026-09-24-001, Stage 5 decision 8

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-09-24-001-daily-cleanup-boundary-fix`. Do not switch branches.
**Binding spec:** `docs/work-orders/WO-2026-09-24-001-daily-cleanup-boundary-fix.md`, including Stage 5 decisions 1–8. Read the Fix section, the Requirements, and acceptance criteria 8 and 9 before changing anything.
**Starting point:** the working tree already contains this WO's Stage 6/7-verified implementation (decisions 1–7) as staged changes. Build on it. Other untracked files under `docs/work-orders/` (WO-2026-09-25-*) belong to other work orders: do not touch them.

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Work only from the WO. Do not expand scope or change the architecture; if the WO cannot be implemented as written, stop and report back. Any correction to the spec, even an obviously right one, is reported as a deviation. Do not commit, push, merge, or release; leave an uncommitted working-tree diff. Temporary artifacts must have visible, descriptive names under a gitignored path and be removed when you finish.

## What to implement: decision 8 only

1. **Remove the in-block snapshot publish.** In `src/state/State_Report.cpp`, the cleanup block currently calls `publishData()` before `dailyCleanup()`. Remove it. Each report has exactly one `publishData()` call.
2. **Restructure `handleReportingState()` to the Fix section's order:**
   - Evaluate the boundary test first, with no writes: trusted gate, `close` normalization, `localTodayAt(close)`, the `now < boundary` adjustment, and `due = (lastDailyCleanup < boundary || lastDailyCleanup > now)`. Untrusted means not due and nothing stamped.
   - If due: capture `wasOccupied` and the original session start, then close the session at the boundary (`closeOccupancySessionSafely("daily-cleanup", boundary)`).
   - The existing sensor reads (`measure.loop()`, `measure.batteryState()`) then the single report publish: `publishData(due ? boundary - 1 : 0)`.
   - If due, in this order: `dailyCleanup()`, the always-open restart (`close == 24 && wasOccupied` → start at `std::max(boundary, originalSessionStart)`), then `SystemConfig::set_lastDailyCleanup(now)`.
   - Everything after the publish that exists today (`set_lastReport(now)`, the counting-mode `hourlyCount` reset, webhook supervision, the connectivity decision) stays as it is.
3. **Add the stamp override to `publishData()`:** `void publishData(time_t stampOverride = 0)`. In the occupancy-mode payload, a nonzero override replaces `timestampValue` (today `nowStampSec`); zero keeps today's value. The counting-mode payload is unchanged. `publishData()` is declared in three places (`src/Generalized-Core-Counter.cpp` near line 120, `src/state/StateMachine.h:134`, `src/state/State_Common.h:111`). Give the default argument in exactly one declaration that every caller sees; C++ forbids repeating a default argument in the same scope. Report which declaration carries it.
4. Keep everything decisions 1–7 established: the live `localTodayAt()` (never `LocalTimeCache`), crediting the session only up to the boundary, the `max(boundary, sessionStart)` restart, the `last > now` guard, and no `sensorOff()`.

## Tests: update `tests/daily_cleanup_boundary_test.py`

Keep every existing check that still applies, including all of mutations (a)–(h) from the prior Stage 7. Add coverage so that each of these fails if broken:

- (a) The report is published **before** the reset: in `handleReportingState()`, the single `publishData(` call precedes `dailyCleanup();`, and the boundary test (`due` computation) precedes the publish.
- (b) **No second publish** at cleanup: exactly one `publishData(` call in `handleReportingState()`.
- (c) The payload is stamped at `boundary - 1` **only** for the report that triggers the cleanup: the call passes `due ? boundary - 1 : 0` (or an exactly equivalent form), and `publishData()` applies a nonzero override only to the occupancy payload's timestamp.

Stage 7 will run these mutations, and each must make at least one test fail: (i) restore the in-block publish, (ii) move the report after the reset, (iii) stamp at `now` instead of the boundary, (iv) apply `boundary - 1` to every report.

## Verification (run all; report results)

1. Full host suite: every `tests/*.sh` run with **zsh** (via shebang or `zsh <script>`, never bash) plus every bare `tests/*.py` with python3. Report as `N/N (sh via zsh, py via python3)`. Current baseline: 43/43.
2. Local ARM-toolchain build (boron), per the README command. Report text/data/bss against the current reference 150520 / 1090 / 2444.
3. Cloud compile: `particle compile boron . --target 6.4.1`. Report Flash/RAM against the current reference 151698 / 3530. Remove the downloaded binary afterward.

## Implementation Report (required)

- Files changed
- Behavior changed
- Tests added or updated, and why
- Commands run and results, with interpreters and build sizes
- Which `publishData()` declaration carries the default argument
- Known limitations
- Deviations from the WO, including any spec correction (or "none")
- The model and reasoning level actually used
