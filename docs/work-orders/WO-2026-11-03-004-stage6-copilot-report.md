# WO-2026-11-03-004 Stage 6 report (Copilot CLI): item 19 C1, one millis-based debounce expiry

Model: Claude Sonnet 5.5 (`claude-sonnet-5.5`). Reasoning effort: the dispatch header says "medium", but the actual setting was low/default, not medium. Nothing committed, pushed, flashed or merged; all changes are uncommitted on `wo/2026-11-03-004-item19-c1-millis-expiry`.

## Result

- Host suite: **108/109 on the full run (sh via zsh, py via python3)**. The one failure, `alert19_clears_after_report_test.sh`, is flaky: its log shows PASS and it passed 3 of 3 reruns alone. Counting it as passing, 109/109. Its failure on the full run was not diagnosed (it also failed once earlier, before any test edit).
- No pre-change baseline run was possible (`git checkout` / `git archive` were denied), so "108 before" is assumed, not confirmed.
- Local build: Device OS 6.6.0, boron, fresh `BUILD_PATH_BASE=build-tmp/wo-2026-11-03-004-stage6/obj-release`, rc=0, text 151692 / data 1114 / bss 2200. Log: `build.log` in that directory.

## src/ change (net -4 lines; budget +16)

| file | added | removed |
|---|---|---|
| State_Common.h | 18 | 0 |
| State_Idle.cpp | 1 | 10 |
| State_Modes.cpp | 3 | 16 |
| State_Sleep.cpp | 2 | 2 |

`debounceRemainingMs()` (State_Common.h) is millis-only: `lastOccupancyEvent == 0` re-arms to `millis()`; returns 0 when `elapsed >= occupancyDebounceMs()`. Used by:
- `handleIdleState` (awake expiry),
- `updateOccupancyState` (awake expiry),
- `processSleepWakeEvent` OCCUPANCY wake close,
- `sleepPlan` occupied branch (`remaining/1000 + 1`, 1 s margin per Amendment 1; the full-debounce fallback is kept).

Persisted/retained layout, `closeOccupancySessionSafely` B2/B3 and the credit arithmetic are untouched. `ledOffTime` and `signalLED*` are untouched; `signalLEDTimeRemaining` is still used by COUNTING paths.

## Tests

New: `tests/debounce_expiry_c1_test.sh`. It extracts the real predicate, checks fresh, 1 ms left, boundary, past expiry, runtime default, sentinel and millis wrap, then checks call sites.

Adapted: `occupancy_report_by_mode_test.sh`, `occupancy_session_restart_test.sh`, `idle_timediag_on_change_test.cpp`, `sleep_wake_equivalence_test.{cpp,sh}`, `sleep_wake_structural_test.py`, `sleep_plan_structural_test.py`, `sleep_preflight_structural_test.py`, `sleep_plan_equivalence_test.{cpp,sh}`, and the fixture `tests/fixtures/sleep_plan_transcript.txt`.

## Mutation results (all caught)

- `debounce_expiry_c1_test`: no expiry clamp; off by the margin (+1000); sentinel not handled; wake close on wall clock (`signalLEDTimeRemaining`); awake idle on subtraction; plan on RTC deadline; plan margin dropped. A "`>` instead of `>=`" mutation was tried and is an equivalent mutant (remaining is 0 either way), so it was dropped.
- `sleep_plan_equivalence_test`: the earlier mutations, plus deadline margin dropped, RTC deadline restored, predicate off by the margin, sentinel not handled. All caught.
- `sleep_wake_equivalence_test` and `occupancy_report_by_mode_test`: the existing mutations, updated to the new wake-close text, all caught.

## nm / disassembly evidence (`tree.elf`; the objdump excerpt is saved as `callsites.txt` in the scratch dir)

- `W debounceRemainingMs()` at c245c and `W occupancyDebounceMs()` at c1ce0 are linked.
- Direct `bl debounceRemainingMs()` calls: `updateOccupancyState()` (c2502) and `processSleepWakeEvent(bool)` (c26c0).
- `handleIdleState()` and `handleSleepingState()` (sleepPlan is inlined there) inline the predicate: each calls `CurrentReadings::get_lastOccupancyEvent`, and handleIdleState calls `occupancyDebounceMs`. Their sites show no `bl debounceRemainingMs`, so linkage is by inlining. An unflagged build has no flag-gated difference.
- Object-file `nm` was not done (the objects live under the `BUILD_PATH_BASE`; I did not repeat it per object).

## Step 0 findings

- **Mismatch:** `WO-2026-11-03-001-step0-report.md` is not in the repo. I worked from the WO, its Amendment 1 and the code.
- The cited line ranges roughly matched the code.
- The only OCCUPANCY wake-close site is `processSleepWakeEvent` (once, from `postWakeRouting`). The CONNECTING reconnect early return skips it, but the awake Idle path then closes on the same predicate.
- LED close criterion on the wake paths: I kept the `signalLEDStatus()` gate and replaced only the wall `signalLEDTimeRemaining()==0` clause. Dropping the gate would change the frozen S-6 fixture and write `lastOccupancyEvent` while unoccupied. **This means the hole where the LED is off while occupied remains**, as in v48 (open question).
- Boundary change: `updateOccupancyState` used `>`, now `>=` (1 ms), matching Idle. It is not visible in any test.
- The old criterion also returned 0 whenever `!Time.isValid()`; that wall-clock coupling is what is removed.
- S-4 fixture: against the old fixture, exactly 13 pre-existing scenarios changed. Each lost `CALL signalLEDTimeRemaining` and gained one `CALL get_sensorSetting1`, with OUT lines identical:
  - occupied set1 in {0, 999, 1000, 5000, 3589000, 3590000, 3591000, 3592000, 7200000};
  - closed set1 in {0, 5000, 50000000};
  - unknown-untrusted.
  The 10 old `debounce-left ledRem=` scenarios were replaced by 33 millis-based ones (98 → 121 scenarios). The old fixture is at `build-tmp/wo-2026-11-03-004-stage6/old_fixture.txt`.
- `lastOccupancyEvent` is zeroed at boot and re-seeded by `MyPersistentData.cpp:696`, so the sentinel path is real.
- `tests/sleep_wake_transcript.txt` (S-6) was not touched. The fake in `sleep_wake_equivalence_test.cpp` keeps it frozen.

## Deviations to flag

- The WO says the `occupancy_report_by_mode` cpp is unchanged. The cpp is, but its `.sh` needed one extraction tweak and updated mutation strings.
- One test file (`sleep_plan_equivalence_test.sh`) was partly rewritten via a `python3` heredoc instead of the edit tool. All other edits used the edit tool.
- Scratch: `build-tmp/wo-2026-11-03-004-stage6/` (git-ignored); the copied `src`/`lib` trees were removed afterwards.

## Open questions

1. Is the retained LED-off-while-occupied gap acceptable, or should the wake close drop the LED gate (which changes S-6)?
2. Should the missing Step 0 report be restored to the repo?

## Pending (not claimed)

The :52 straddle bench test is Chip's later step.
