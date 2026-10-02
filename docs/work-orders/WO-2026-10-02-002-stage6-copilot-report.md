<!-- Copilot claude-opus-5 (medium) Stage 6 Implementation Report, extracted verbatim from its --share transcript by Claude Code, 2026-10-02. Claude Code checked: archive unchanged (120584); only build-tmp/wo20261002-002-stage6/ created and removed. Deviation 3's docs edit was Claude Code's v32 bench-results update (on Chip's instruction), not an outside change. -->

# Implementation Report — WO-2026-10-02-002 (v33-HourlyWhileOccupied)

## The change — **8 net `src/` code lines** (budget: about 8)
`git diff -U0 -- src/`: 11 code lines added, 3 removed, **net 8**.

- **`src/state/State_Common.h` (+7):** two includes (`../Config.h`, `persist/SystemConfig.h`) and the one shared due test, 4 lines:
  ```cpp
  inline bool reportDueThisInterval() {
      const time_t interval = Config::reportingIntervalSecForRuntime(), lastReport = SystemConfig::get_lastReport();
      return lastReport == 0 || Time.now() / interval != lastReport / interval;
  }
  ```
  `reportingIntervalSecForRuntime()` never returns 0 (`Config.cpp:144`), so no zero guard is needed.
- **Site 1** (`State_Sleep.cpp:1749–1750`, +0): `&& !reportDueThisInterval()` appended to the existing suppression guard, so an occupied timer wake falls through to `sleep-timer-report` when due.
- **Site 2** (`State_Sleep.cpp:1770`, +1): the `// Skip overdue-report check` placeholder became `if (reportDueThisInterval()) { transitionTo(REPORTING_STATE, "sleep-pir-overdue-report"); return; }`.
- **Site 3** (`State_Idle.cpp:190`, +1): same shape with `"report interval"`.
- Unoccupied `else` branches, occupancy-change reports, the daily close, close-before-sleep and all of v32 are untouched.
- **Version:** `./bump_version.sh v33-HourlyWhileOccupied "…"` → `FIRMWARE_VERSION = "v33-HourlyWhileOccupied"`, `FIRMWARE_PRODUCT_VERSION 33`.

## Tests
**New: `tests/hourly_while_occupied_test.sh` + `.cpp`** — behavioral against the real source, using the existing `connectivity_failsafe_open_hours_test` harness pattern (the only pattern that fits: `handleSleepingState()`/`handleIdleState()` cannot be linked on the host). The real `reportDueThisInterval()` body is lifted verbatim from `State_Common.h` and compiled in; the three sites' real occupied branches are parsed out and injected as `-D` flags, so the simulations run the production rule through the production structure. Honors `HOURLY_WHILE_OCCUPIED_SRC_ROOT` for mutation runs on a copy.

All green: due rule is the clock interval; timer wake at the hour while occupied reports with `occupancy: 1` and `occupancyStartTime`/accumulation unchanged; no second report inside an interval; 4 h continuous occupancy with PIR wakes every 5 s and debounce wakes every 300 s → **exactly 4 scheduled reports, one per hour boundary**; v32 failsafe (real 10800 s threshold) never acts, plus a sanity check that it *does* act when the reports are suppressed; sites 2 and 3 each carry the hour alone; unoccupied unchanged at all three sites (incl. the deliberate elapsed-vs-clock-hour discriminator). Part 2 pins one shared due test with exactly three call sites, the protected labels, and that the report path closes a session only at `"daily-cleanup"`.

**Mutations** (run on copies in the scratch dir; production tree untouched, verified by `diff -r`):
| Mutation | Result |
|---|---|
| Restore suppression at site 1 | **FAILED** — "a timer wake at the report time while occupied must report" |
| Restore suppression at site 2 | **FAILED** — "a PIR wake just after the hour boundary while occupied must report" |

**Existing test updated (1):** `tests/loop_stage_sleep_prep_exclusion_test.py` pinned `EXPECTED_TRANSITION_CALLS = 16` in `handleSleepingState()`; site 2 adds a 16th real exit point → **17**, with the comment extended to name this WO. `tests/publish_with_ack_structural_test.py` passes **unchanged**.

## Commands and results
- Host suite, **zsh** for `tests/*.sh` + **python3** for bare `tests/*.py`: before **56/56** (v32), after **57/57**.
- ARM release build, boron / Device OS 6.4.1, after `make clean-user` (buildscripts 1.17.2, gcc-arm 10.2.1): **COMPILED SUCCESSFULLY**, `strings` shows `v33-HourlyWhileOccupied`.
  | | text | data | bss |
  |---|---|---|---|
  | v32 | 150628 | 1090 | 2204 |
  | v33 | **150780** (+152) | 1090 (+0) | 2204 (+0) |
- `target/` restored byte-for-byte to the pre-existing v32 outputs (verified: 150628/1090/2204, `v32-RecoveryVisibility`).
- `git diff --stat`: `CHANGELOG.md 6+`, `Doxyfile 1±`, `README.md 1±`, `src/FirmwareVersion.h 2±`, `src/state/State_Common.h 25+`, `src/state/State_Idle.cpp 4±`, `src/state/State_Sleep.cpp 7+/5-`, `tests/loop_stage_sleep_prep_exclusion_test.py 7±`. Untracked: the two new test files. Nothing committed, pushed, merged or flashed.

## Scratch cleanup
Deleted **only** `build-tmp/wo20261002-002-stage6/`. `build-tmp/connectivity-archive/` and `build-tmp/WO-2026-10-01-001-stage6-copilot-transcript.md` are intact.

## Deviations
1. **`#include "../Config.h"`, not `"Config.h"`.** The plain form compiles on the host but silently resolves to a Device OS header from `src/state/`, failing the ARM build; `../Config.h` is the existing convention (`State_Sleep.cpp`, `State_Idle.cpp`, `State_Connect.cpp`). No line-count effect.
2. **Helper declares both locals on one line** to land exactly on the 8-line budget; separate lines would be 9.
3. **`docs/work-orders/WO-2026-10-02-001-recovery-visibility.md` is modified in the working tree and I did not touch it.** It was clean when I started. `bump_version.sh` writes only `FirmwareVersion.h`, `Doxyfile`, `README.md`, `CHANGELOG.md`, so this is a concurrent external edit (Dev-09 bench results). Left as found.
4. Dispatch vs WO: no differences found.

**Model/reasoning actually used:** `claude-opus-5`, medium reasoning.

---

