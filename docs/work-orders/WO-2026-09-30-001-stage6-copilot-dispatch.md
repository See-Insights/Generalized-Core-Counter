AGENT: Copilot · MODEL: claude-opus-5 · REASONING: medium
AUTHORIZATION SCOPE: create `src/time/DailyBoundary.h` and `src/time/DailyBoundary.cpp`; edit `src/state/State_Report.cpp` (move the due-test out, call `DailyBoundary::check`), `src/state/State_Sleep.cpp` (the one condition at the night-sleep commitment), and the version files; update `tests/daily_cleanup_boundary_test.py` and add one test as the WO says; run the host suite and a local ARM build / Not authorized: commits, pushes, merges, branch changes, stash, reset, flashing, device settings, AWS access, any edit to `lib/`, `project.properties`, or `docs/`, and any change beyond this WO.
**SIZE BUDGET: about 25 lines of new `src/` code; the moved due-test lines are counted separately and must move unchanged. Going over the budget means STOP and report, not continue.**

# Stage 6 dispatch — WO-2026-09-30-001 (never sleep for the night with the close still due)

**Goal, in plain language:** the device never commits to night sleep while the daily close is due. If it is due, the device runs the report first, and that report does the close.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-09-30-001-close-before-night-sleep`, `HEAD` `c2fb35c` (v27-SmallFixes). Do not switch branches. Files under `docs/` are records: do not touch them (including the three untracked `docs/work-orders/2026-09-29-*` and `WO-2026-09-29-002-*` files, which belong to another WO).
**Binding spec:** `docs/work-orders/WO-2026-09-30-001-close-before-night-sleep.md`.

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Work only from the WO. Any correction to the spec, even an obviously right one, is reported as a deviation. Leave an uncommitted working-tree diff. Temporary artifacts: visible, descriptive names under a gitignored path, removed when you finish.

## What to implement (nothing else)

1. **`DailyBoundary`** (`src/time/DailyBoundary.h`, `.cpp`): `namespace DailyBoundary { struct Result { bool due; time_t boundary; uint8_t close; }; Result check(time_t now); }`. Move into it, **unchanged in logic**, the due-test from `State_Report.cpp:50–72` (the `Clock::isTrusted()` gate; `close = (openHour == closeHour) ? 24 : closeHour`; `boundary = localTodayAt(close)`; `if (now < boundary) boundary -= 86400`; `due = (lastDailyCleanup < boundary || lastDailyCleanup > now)`) and the helper `localTodayAt()` from `State_Report.cpp:27`. Untrusted clock: `{false, 0, 0}`, exactly as the current initial values. Keep the "Daily boundary reached ..." `Log.info` in `handleReportingState()`.
2. **`handleReportingState()`**: replace the inline block with `const DailyBoundary::Result closeCheck = DailyBoundary::check(now);`, then the same locals `due`, `boundary`, `close` taken from it, then the unchanged log line. Everything after (session capture, `publishData(due ? boundary - 1 : 0)`, `dailyCleanup()`, `set_lastDailyCleanup(now)`, and the item-A `else if (due)` connect trigger) stays exactly as it is.
3. **`State_Sleep.cpp:971`**: at the night-sleep commitment, before any night-sleep work (before `SensorManager::instance().onEnterSleep()`), when `parkOpenness == Clock::Openness::Closed` and `DailyBoundary::check(Time.now()).due`, call `transitionTo(REPORTING_STATE, "close due before night sleep");` and `return;`. A one-line comment is allowed. Nothing else in the sleep path changes.
4. **Version**: `src/Version.cpp` `FIRMWARE_VERSION` → `"v28-CloseBeforeSleep"`, with a one-line `FIRMWARE_RELEASE_NOTES` that does **not** contain `pdiag`; `src/FirmwareVersion.h` `FIRMWARE_PRODUCT_VERSION` 27 → 28.

## Tests

- **`tests/daily_cleanup_boundary_test.py`**: it pins the due-test's source text inside `handleReportingState()`. Point its source-shape checks at `DailyBoundary` for the moved logic, and at `handleReportingState()` for the call and the unchanged order (publish at `boundary - 1`, then `dailyCleanup()`, then `set_lastDailyCleanup(now)`). Keep the acceptance model and every existing assertion's intent, including the item-A assertion.
- **Add** one check (structural is fine; a new `tests/*.py`, or an addition to the same file) that `handleSleepingState()`'s `Closed` branch calls `DailyBoundary::check(` and `transitionTo(REPORTING_STATE, "close due before night sleep")` before `onEnterSleep()` and `secondsUntilNextOpen()`. Removing the condition must make it fail. Show that (mutate, run, restore byte-identically by rewriting in place).
- Update any other test only if it pins text this WO changes; report each. `tests/publish_with_ack_structural_test.py` must pass unchanged. If anything else fails, stop and report.

## Verification (run all; report results)

1. Full host suite: every `tests/*.sh` with **zsh** (never bash) plus every bare `tests/*.py` with python3. Report `N/N (sh via zsh, py via python3)`. Current: 44/44.
2. Local ARM build (boron), release, after `make clean-user`: text/data/bss against v27's 150220 / 1090 / 2196; `strings` finds `v28-CloseBeforeSleep` and `close due before night sleep`, and not `pdiag`.
3. `git diff --numstat`, and the `src/` count split into **moved** lines (the due-test and `localTodayAt()`) and **new** lines, against the ~25-line budget.

## Implementation Report (required)

Files and lines changed; the moved/new line split; the tests changed or added and why; the mutation result; commands and results with interpreters and sizes; deviations (or "none"); the model and reasoning level actually used.
