# WO-2026-11-16-001 Stage 6 report (Copilot)

Model/level: claude-sonnet-5.5 (low effort, as dispatched). Interpreter: Python 3.15.2 (`python3`), zsh for `.sh`, host `c++`.
Changes are **uncommitted**, on HEAD `7ac0536` (base code `1d1d5af`).

## Baseline (before editing)
121/121 (sh via zsh, py via python3). CI discovery: `tests/*.sh` via zsh, `tests/*.py` via python3, no skips.

## `src/` diff (only `Generalized-Core-Counter.cpp`, two spots)
`git diff --numstat -- src`: 6 added, 3 removed = **+3 net** (budget +4).
- `setup()`: `.watchdogPin(WATCHDOG_DONE_PIN)` removed; trailing comment on the following chain line. No new line.
- `loop()`: `static int lastRefreshRc`, `const int refreshRc = ExternalWatchdog.refresh();` (comment kept), change-detecting `Log.warn("ExternalWatchdog: refresh rc=%d")`, latch update. Refresh still appears once, before `checkClockResync()`.

## Tests
| file | added/removed |
|---|---|
| tests/external_rtc_structural_test.py | +3/-2 (config check; "pin-restored" mutation in `smut`) |
| tests/external_rtc_test.cpp | +4/-4 (`fleet()` without pin; `wdtPin == -1`; stored `wdtPin = 5` re-bound once to -1) |
| tests/external_watchdog_feed_test.py (new) | +115 |
Total added: 122 lines (guideline +175).

New test: model of Device OS 6.6.0 `am18x5.cpp:934-946` and a 124 s chip counter; 200 s window, feed every 30 s; checks one register write per feed, no expiry, plus loop structure.

## Mutations (all caught)
| mutation | test | result |
|---|---|---|
| pin restored | external_rtc_structural_test | caught |
| pin restored (counter expires at 124 s) | external_watchdog_feed_test | caught |
| refresh removed | external_watchdog_feed_test | caught |
| rc discarded | external_watchdog_feed_test | caught |
| rc logging removed | external_watchdog_feed_test | caught |
| rc logged every loop (extra) | external_watchdog_feed_test | caught |
Mutations were in-memory copies of the source; the real files were never modified by them (no restore needed). `s2_alarm_watchdog_wake_structural_test` stays green.

## Suite after
**122/122 (sh via zsh, py via python3).**

## Local build (Device OS 6.6.0, boron, fresh `BUILD_PATH_BASE` each, copies of trees)
| | text | data | bss |
|---|---|---|---|
| base 1d1d5af | 151084 | 1114 | 2256 |
| new | 151124 | 1114 | 2264 |
| delta | +40 | 0 | +8 |
Both builds exit 0.

## Deviations / notes
- My first post-edit full-suite run was 121/122: `thermal_coupling_structural_test.py` scans the repo tree and picked up my scratch source copies under `build-tmp/`. I deleted those copies (and the build dirs I created; logs `new.log`, `base.log` kept) and re-ran: 122/122. Not a code issue.
- No other deviation.

## Seen outside the WO (not fixed)
- `WATCHDOG_DONE_PIN` is now declared but unused (expected per Step 0).
- `thermal_coupling_structural_test.py` is sensitive to source copies under `build-tmp/`.
