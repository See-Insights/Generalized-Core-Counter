Done. Working tree left uncommitted. **Implementation Report** follows.

## Files changed

| File | Δ | Notes |
|---|---|---|
| `src/time/DailyBoundary.h` (new) | +35 | `struct Result { bool due; time_t boundary; uint8_t close; }; Result check(time_t now);` |
| `src/time/DailyBoundary.cpp` (new) | +54 | `localTodayAt()` + the due-test, moved verbatim |
| `src/state/State_Report.cpp` | +13/−40 | inline block → `DailyBoundary::check(now)`; log line kept |
| `src/state/State_Sleep.cpp` | +7 | close-due guard at the `Closed` branch, before `onEnterSleep()` |
| `src/Version.cpp` | +2/−2 | `v28-CloseBeforeSleep`, notes (no `pdiag`) |
| `src/FirmwareVersion.h` | +1/−1 | 27 → 28 |

**Moved/new split (`src/`, non-blank):** moved **45**, new **47** raw → **26 new non-comment code lines** excluding the 3 version-bump lines (13 header declarations/boilerplate, 5 `.cpp` scaffolding, 5 in `State_Report.cpp`, 3 in `State_Sleep.cpp`). Within the ~25-line budget. The 45 moved lines are byte-identical (verified by token-pool matching against `HEAD:src/state/State_Report.cpp`).

## Tests

- **`tests/daily_cleanup_boundary_test.py`** (+59/−15) — moved-logic checks (`localTodayAt`, trusted gate, normalization, `-= 86400`, due comparison, no `LocalTimeCache`/`lastReport`/Y-M-D) now run against `DailyBoundary::check()`; added a `DailyBoundary.h` shape check, a "pure query" check, and a negative control that `localTodayAt` is gone from `State_Report.cpp`. `handleReportingState()` now pins the call plus the unchanged order (publish at `boundary − 1` → `dailyCleanup()` → `set_lastDailyCleanup(now)`). Acceptance model and the item-A `else if (due)` assertion untouched.
- **`tests/close_before_night_sleep_test.py`** (new) — the `Closed` branch must call `DailyBoundary::check(`, guard-and-return with `transitionTo(REPORTING_STATE, "close due before night sleep")`, before `onEnterSleep()` and `secondsUntilNextOpen()`; plus include-the-owner / no-local-copy checks.
- **Other tests pinning text this WO changed (each reported):**
  1. `tests/clock_trust_standard_structural_test.py` — the "daily-cleanup day-boundary gate" `if (Clock::isTrusted()) {` site now lives in `DailyBoundary.cpp`; path retargeted, still 14 sites.
  2. `tests/sleep_duration_consumes_openness_test.py` — invariant 2's brace-free text window (`[^{}]*$`) rejected the new braced guard. Replaced with an equivalent, stronger structural check: the innermost unclosed `{` before `nightSleepSec = secondsUntilNextOpen();` must belong to `if (parkOpenness == Clock::Openness::Closed)`. Intent preserved.
  3. `tests/loop_stage_sleep_prep_exclusion_test.py` — pinned `transitionTo(` count in `handleSleepingState()` 14 → 15 (13 → 14 real exit points).
- `tests/publish_with_ack_structural_test.py`: unchanged, passing.

**Mutation:** removed the 6-line guard from `State_Sleep.cpp` → `close_before_night_sleep_test.py` fails (`FAIL: the parkOpenness == Clock::Openness::Closed branch must call DailyBoundary::check(...)`, rc=1). Restored in place; `shasum -a 256 -c` OK (`1b0ab06c…d791f`).

## Verification

1. **Host suite: 45/45 (sh via zsh, py via python3)** — 22 `.sh` via zsh, 23 `.py` via python3 (44 → 45 with the new test).
2. **Local ARM build** (boron, release, Device OS 6.4.1, fresh `BUILD_PATH_BASE`): **text 150308 / data 1090 / bss 2196** vs v27's 150220 / 1090 / 2196 → **+88 text**, data/bss unchanged. `strings firmware.bin`: `v28-CloseBeforeSleep` = 1, `close due before night sleep` = 1, `pdiag` = 0, `v27-SmallFixes` = 0.
3. `git diff --numstat` as tabled; untracked: the two `src/time/DailyBoundary.*` files and the new test. No `docs/`, `lib/`, or `project.properties` change.

## Deviations

1. **`handleReportingState()` re-reads `SystemConfig::get_lastDailyCleanup()` for the log line.** `Result` carries only `{due, boundary, close}` per the WO, but the "Daily boundary reached …" `Log.info` prints `last=`. The read is before any `set_lastDailyCleanup()`, so the log output is byte-identical to v27.
2. **`make clean-user` not used** — no such target in `deviceOS/6.4.1/main` (same as the v27 round); equivalent achieved with a deleted, fresh `BUILD_PATH_BASE`/`TARGET_DIR`.
3. **`#include "LocalTimeRK.h"` left in `State_Report.cpp`**, now unused there — removing it was not authorized.
4. **Three tests beyond `daily_cleanup_boundary_test.py` were updated** (listed above), under the WO's "only if it pins text this WO changes, and report each one".

**Model / reasoning actually used:** claude-opus-5, medium.

Nothing committed, pushed, merged, or flashed; branch unchanged (`wo/2026-09-30-001-close-before-night-sleep`). `build-tmp/` artifacts removed.



