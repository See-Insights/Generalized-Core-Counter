AGENT: Copilot · MODEL: claude-opus-5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/state/State_Sleep.cpp` (sites 1 and 2), `src/state/State_Idle.cpp` (site 3), and at most one shared header for the due test (e.g. `src/state/State_Common.h`); run `./bump_version.sh v33-HourlyWhileOccupied "<one-line note>"`; add or update tests under `tests/`; run the host suite and a local ARM release build. Your scratch directory is **`build-tmp/wo20261002-002-stage6/`**: keep every temporary file in it and remove **only it** when done / Not authorized: commits, pushes, merges, branch changes, stash, reset, checkout, flashing, device settings, AWS or network access, any edit to `lib/`, `project.properties` or `docs/`, any change to v32's items (the failsafe, `MODEM_OFF`, payload fields) or beyond this WO, and **deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`**.
**SIZE BUDGET: about 8 net `src/` code lines (nonblank, non-comment, including braces, declarations and includes). Going over means STOP and report, not continue.**

# Stage 6 dispatch — WO-2026-10-02-002 (v33-HourlyWhileOccupied)

**Goal, in plain language:** the device makes its scheduled report every hour whether or not the site is occupied. That also means v32's "3 hours without a successful connection" only happens when the device actually can't connect.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-10-02-002-hourly-while-occupied`, `HEAD` `e8e33a7` (v32 committed, stacked). Do not switch branches. Files under `docs/` are records: don't touch them.
**Binding spec:** `docs/work-orders/WO-2026-10-02-002-hourly-while-occupied.md`, including the Stage 5 decision (the clock-hour due rule). Where this dispatch and the WO differ, the WO wins; report the difference.

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Work only from the WO. Any correction to the spec is reported as a deviation. If it can't be done as approved within the budget, stop and report it. Leave an uncommitted working-tree diff.

## What to implement

1. **One shared due test** for the occupied case:
   - due when `lastReport == 0` or `now / interval != lastReport / interval`;
   - `interval = Config::reportingIntervalSecForRuntime()`, `lastReport = SystemConfig::get_lastReport()`, `now = Time.now()`.
2. **The three sites,** each currently conditioned on occupancy mode + occupied + `INTERMITTENT_KEEP_ALIVE`:
   - **Site 1** (`State_Sleep.cpp:1746–1752`, timer wake): while occupied, `transitionTo(REPORTING_STATE, "sleep-timer-report")` if due; otherwise keep `transitionTo(SLEEPING_STATE, "sleep-timer-occupied-suppress-report")`.
   - **Site 2** (`State_Sleep.cpp:1762–1768`, PIR wake): while occupied, `transitionTo(REPORTING_STATE, "sleep-pir-overdue-report")` if due; otherwise fall through as today.
   - **Site 3** (`State_Idle.cpp:185–191`): while occupied, `transitionTo(REPORTING_STATE, "report interval")` if due; otherwise as today.
3. **Unoccupied branches stay exactly as they are.** Don't touch occupancy-change reports, the daily close, close-before-sleep, or the session (no close, no restart).
4. **Version:** `./bump_version.sh v33-HourlyWhileOccupied "<one-line note>"`, giving product 33.

## Tests

Prefer behavioral host tests against the real source where an existing harness pattern fits; say which.
- **Hourly while occupied:** a timer wake at the report time while occupied → `REPORTING_STATE` with occupancy 1, and the session's `occupancyStartTime` and accumulation are unchanged.
- **No more than due:** after a report in the current interval, debounce timer wakes and PIR wakes in the same interval → no report. Over a simulated 4-hour continuous occupancy with PIR wakes every few seconds and debounce wakes every 300 s, exactly one scheduled report per hour boundary.
- **The v32 failsafe:** a host check that with continuous occupancy for 4 hours and a working connection, the v32 failsafe never resets.
- **Unoccupied unchanged** at all three sites.
- **Mutations** (each must fail a test; restore byte-identically, or run on a copy): restoring the suppression at site 1; restoring it at site 2.
- Update an existing test only if it pins text this WO changes, and report each one. `tests/publish_with_ack_structural_test.py` must pass unchanged.

## Verification (run all; report results)

1. Full host suite: every `tests/*.sh` with **zsh**, plus every bare `tests/*.py` with python3, as `N/N`, before (v32: 56/56) and after.
2. Local ARM release build (boron, Device OS 6.4.1, after `make clean-user`, in your scratch directory or with outputs restored): text/data/bss against v32's 150628 / 1090 / 2204; `strings` shows `v33-HourlyWhileOccupied`.
3. Net `src/` lines against about 8, and `git diff --stat`.

## Implementation Report (required, as your final message)

- The change, with line counts.
- Tests and mutations, with results.
- Commands and results, with interpreters and sizes.
- Confirmation that you deleted only `build-tmp/wo20261002-002-stage6/`.
- Deviations (or "none").
- The model and reasoning level actually used.
