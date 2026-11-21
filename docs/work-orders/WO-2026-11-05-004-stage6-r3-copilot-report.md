# WO-2026-11-05-004 (v50 S2) – Stage 6 round 3 – Copilot report

Nothing was committed, pushed, merged or released. All work is uncommitted on top of HEAD 1fa868b (S2 code at 86b11e0).
**No hardware claim is made.** Only host tests, a compile, and `nm`/objdump were run.

## Run details
- Model: claude-sonnet-5.5. Dispatched at medium reasoning; the level actually run could not be observed.
- Interpreters: zsh 5.9 (arm64-apple-darwin26.0), Python 3.14.6, Apple clang++.
- Build: Device OS 6.6.0, boron, buildscripts 1.17.2, gcc-arm 10.2.1, fresh `BUILD_PATH_BASE=build-tmp/wo-2026-11-05-004-stage6-r3/bp`. Compiled; text/data/bss = **151508 / 1114 / 2240** (86b11e0: 151468 / 1114 / 2240; +40 text). 6 warning lines (the dispatch did not give a baseline; r1 verdict saw five, none in S2 lines; not analysed further).

## src/ net (vs 3b5cfb8: **-133**, budget +20; vs 86b11e0: +7)
| file | +/- vs 3b5cfb8 | +/- vs 86b11e0 |
|---|---|---|
| Generalized-Core-Counter.cpp | +48/-70 | +4/-3 |
| persist/RecoveryState.h | +6/-1 | +1/-1 |
| power/ConnectivityFailsafe.cpp | +1/0 | +1/0 |
| state/State_Error.cpp | +3/-1 | +3/-1 |
| time/HibernateCycle.cpp | +7/-49 | +3/-3 (rename + comment) |
| time/HibernateWakeDiagnostics.h | +17/-4 | +8/-5 |
Other files unchanged since 86b11e0. `SysData`/`retained` layouts untouched; `lib/`, S1/S1b code untouched; the two `deepPowerDown()` calls unchanged.

## Fixes
1. **F1:** `HibernateWakeDiagnostics::kPowerOffSentinel = 0xA5`; `isInferredWatchdog()` now takes the raw byte and compares `== kPowerOffSentinel`. State_Error writes the constant. 0, legacy 2 and anything else = no record.
2. **F2:** `SystemConfig::flushNow()` right after the boot clear. After each `deepPowerDown()` (one line each): State_Error clears the byte and flushes; the failsafe caller clears the byte, sets stage to 2 (no longer stage-3 evidence) and flushes.
3. **F3:** the arm field is renamed `wakeReasonIsHibernateWake`; comments/tests say reason 30 = successful HIBERNATE wake (RTC or GPIO). The 1,800 s test is restated; a GPIO-wake case (100 s into 3600 s) was added. Mock label "ALARM" → "POWER_MGMT".
4. **F4:** stage 3 counts only with `timeValid && 0 <= age <= 600`.
5. **F5:** `"ab1805WakeReason":"N/A"` restored before `watchdogSource` (buffer 64 → 80 bytes; worst case ≈ 54 chars).
6. **F6** (see below).

## Tests
Host suite **118/118** (66 via zsh + 52 via python3, counted with `p=$((p+1))`). `thermal_coupling_structural_test.py` passes on an export of the tracked files (working tree src/tests overlaid, plus the two new untracked helpers).

F6 design:
- `tests/s2_hibernate_normalise.py` (new): normalises **the base side only** — a short explicit list of approved S2 substitutions (AB1805 pre-arm/clear and bypass else-branches, `.duration(...)`, ExternalWatchdog replacing ab1805 stop/resume, the reworded comment) plus indentation. Used by `sleep_plan`, `sleep_wake`, `sleep_cloud_gate`, `sleep_preflight` structural tests and `sleep_execution_structural_test.py`. Any other change to `hibernateAttempt()` fails.
- Equivalence fixture: base blocks (commit 1e6d34f, from the fixture header) run again as mode `a`; `tests/s2_transcript_normalise.py` (new) maps the base transcript to S2 form (drops stale-clear/pre-arm/bypass events, AB1805→EXTWDT events, adds `dur=wakeIn*1000`, recomputes DISTINCT_EVENTS). The current transcript must equal the fixture, and the normalised base transcript must equal it too. Fixture regenerated from the base (121 scenarios; was 137 — the 16 dropped are the retired stale-clear/alarm-failure scenarios, which can no longer exist).
- Fakes: base `AB1805Stub`/`clearAb1805StaleAlarmInterrupts` restored for the base blocks; a returned HIBERNATE advances the fake clock 7 ms regardless of duration (otherwise base and current differ in time only).
- New cases: GPIO wake; legacy marker 2 (and 0, 1, 0x5A, 0xA4, 0xA6, 0xFF); forced consumption (boot clear immediately followed by flush); returning `deepPowerDown()` (byte cleared + flush at State_Error; byte cleared, stage downgraded, flush at the failsafe); time-invalid stage 3 rejected.
- Updated for the new call-site lines: `deliberate_reset_flush_test.py` (failsafe terminator), `connectivity_failsafe_unknown_openness_test.cpp` (stub setters).

Mutations (real files, backed up in `build-tmp/.../bak/` and restored after each; all caught):
| mutation | caught by |
|---|---|
| F1 `== sentinel` → `!= 0` | watchdog_inferred_classification_test.sh |
| F1 `== sentinel` → `>= 2` (legacy 2) | same |
| F2 boot-clear flush removed | s2_alarm_watchdog_wake_structural_test.py |
| F2 clear/flush after error-path return removed | same |
| F2 failsafe stage downgrade/flush removed | same |
| F4 invalid time accepted | watchdog_inferred_classification_test.sh |
| F5 `ab1805WakeReason` removed | s2_alarm_watchdog_wake_structural_test.py |
| extra: hibernate duration ×1000 → ×100 | equivalence, sleep_wake, sleep_plan |
Also: 4 new in-memory probes in the S2 structural test (14 total) and 3 new mutants in the classification test (8 total).

## Build / nm
Linked and present: `AB1805::deepPowerDown(int)` is called at two sites (be1f6 failsafe, c2254 State_Error); each is immediately followed by `RecoveryState::set_powerOffRequested` (be1fc, c225a) and, for the failsafe, `set_connectivityRecoveryStage` (be202). Boot clear call at b664c area. `RecoveryState::{get,set}_powerOffRequested` and `set_connectivityRecoveryStage` are linked.

## Deviations / open questions
- `sleep_execution_structural_test.py` silently skipped its base-comparison group at 86b11e0 because the fixture header had no `Base commit` line. The restored header activates it; it needed the same normalisation (done). It is a fifth base-comparison test beyond the four named.
- Fixture is regenerated from the base blocks with the S2 table, not the original 137-scenario fixture (the retired failure scenarios cannot run on S2 code).
- Failsafe downgrade sets the stage to **2** (not 0), so the next failsafe action still escalates; confirm this is the intended "downgrade".
- Several S2 call-site lines put two or three statements on one line to keep the diff at one line per site.
- `sed -i` was used once to change a default base commit in a test script (should have been `edit`).
