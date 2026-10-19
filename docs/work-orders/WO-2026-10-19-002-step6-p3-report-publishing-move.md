# WO-2026-10-19-002: Step 6 P-3 — move report publishing and the daily cleanup out of main (pure move)

**Author:** architect, 2026-10-19 · **Status:** ready (citation re-check done 2026-10-19, below)
**Branch:** `wo/2026-10-19-002-step6-p3-report-publishing`, from main **after #114 (P-2) merges**
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.2–12.4; §13 (b), (c), (g), (j)
**Spec source:** `WO-2026-10-15-003-step0-report.md` Q1 groups "(b) `cloud/`" and "(b) `state/State_Report.cpp`", Q3 row "Cloud report", closing table WO 3; architect ruling 17 Oct (`dailyCleanup` moves with P-3). Cited at `71e346f`; P-1 and P-2 shifted main, so re-cite everything.

## Plain goal

Move the hourly report's publishing code and the daily cleanup out of `Generalized-Core-Counter.cpp`, **unchanged** (about 285 lines). No behavior change, and the webhook payload stays byte-for-byte identical. This is the product's core output, so the golden payload test is the main guard.

## What moves (Codex Q1 at `71e346f`; re-check every range)

- To a new `cloud/ReportPublisher.{h,cpp}`: `isAutoClearAfterReportAlert` (old `:1940`), `publishData` (old `:1964`), `publishHibernateWakeForensics` (old `:2328`), `UbidotsHandler` (old `:2361`), `publishDiagnosticSafe` (old `:2435`).
- To `state/State_Report.cpp`, its only caller: `dailyCleanup` (old `:2697`). The publish → `dailyCleanup()` → restart order at `State_Report.cpp:69–82` is unchanged.
- The breadcrumb constants these functions use move with them, or are reached through the existing header (Codex Q3).
- **Stays in main:** `setup()`'s registration of `UbidotsHandler` (only the declaration it needs changes), `publishStartupStatus` and `publishWatchdogForensics` (Codex Q3: they'd expose new shared globals).

## Rules (as P-1 and P-2)

1. Bodies move **byte-for-byte**. Allowed: linkage, declarations, includes. Anything else is a STOP.
2. Retained: the P-0 struct stays at `0x2003f420`; no decision-bearing retained symbol moves; report-only shifts are listed. **No `retained` keyword in the new files**, with negative checks for both the `.cpp` and the `.h` (standard from P-2, recovery-plan item 13).
3. **The golden payload test** is re-pointed to the new file **without touching its fixture** (`tests/fixtures/report_payload_golden.txt`) or its markers (`static const char *batteryContext[7] = {` … the `PublishQueuePosix::instance().publish(webhookName, data, PRIVATE | WITH_ACK)` line). The fixture's sha256 is recorded before and after and must match.
4. **`WITH_ACK` stays proven:** `publish_with_ack_structural_test.py` must still find `WITH_ACK` on the report, status and diagnostic publishes in their new home, and must still fail if any is removed (mutation).

## Budget

- **`src/`: ≤ +20 net non-move lines, hard.** Moved lines count 0; new header, include and namespace lines count.
- Expected main shrink about 285 lines (report `wc -l` before and after).
- Tests: guideline **+150**. About 15 test files anchor on these functions in main (golden, alert19, daily-cleanup boundary, reset wiring, standby-latch clear, WITH_ACK, payload fields, occupancy-by-mode, watchdog alert code, clock-resync wiring, …); re-point them, don't weaken them.

## Tests and checks

- Golden payload test passes with the same fixture hash; suite `N/N`.
- Byte-identical bodies (diff of each moved range against its new home).
- ELF: text change about 0; explain anything over ±64 B by symbol. `nm` retained check under the narrowed rule.
- Mutations Codex re-runs: drop `WITH_ACK` from the report publish; swap the alerts/resets order in one layout; move the `dailyCleanup()` call ahead of the publish. Each must fail a test.
- Structural: none of the moved functions remains in main; no duplicates.

## Agents and models

- **Stage 6:** Copilot, Sonnet tier, **medium**. A mechanical move with many test re-points.
- **Controller (before Stage 7, §13 (g)):** reproduce the build, the fixture hash, the `nm` check, the mutations and the suite.
- **Stage 7:** Codex `gpt-5.6-sol`, **high**. It's the report path: check byte-identical bodies, the fixture hash, the WITH_ACK proof, the close order and the handler registration.

## Bench

None in this WO. Ships in v45 with P-0 to P-6; the v45 soak's payload check covers it on hardware.

## Closing record

Budget versus actual (non-move `src/`, moved lines, main `wc -l`, tests), fixture hash, ELF by symbol, retained outcome.

## Claude Code's citation re-check (2026-10-19, main `dd90842`, P-0, P-1 and P-2 merged; `Generalized-Core-Counter.cpp` is 1978 lines)

**The ranges, start to closing brace** (old WO numbers in brackets): `isAutoClearAfterReportAlert` `:1380–1393` (`static`) [`:1940`]; `publishData` `:1404–1563` [`:1964`]; `publishHibernateWakeForensics` `:1768–1779` [`:2328`]; `UbidotsHandler` `:1801–1859` [`:2361`]; `publishDiagnosticSafe` `:1875–1891` [`:2435`]; `dailyCleanup` `:1956–1978` [`:2697`]. **That is 14 + 160 + 12 + 59 + 17 + 23 = 285 function lines, as the WO says.** The golden markers hold: `static const char *batteryContext[7] = {` at `:1406` and `PublishQueuePosix::instance().publish(webhookName, data, PRIVATE | WITH_ACK)` at `:1479`. The close order in `state/State_Report.cpp` holds: `publishData(due ? boundary - 1 : 0);` at `:69`, then `if (due) { dailyCleanup(); …` at `:76–77`. The golden fixture `tests/fixtures/report_payload_golden.txt` has sha256 **`07de1e1980e4a44e03420540e6c721a93a61b5d6392539219928420c048f0637`** (the value Codex recorded on P-2 as well).

**Findings the implementer must plan for (not WO changes):**
- **Main-defined things the moved code uses:** `state` and `session` (declared in `state/StateMachine.h:25, :78`), and two constants defined in main, `REPORT_FORENSICS_SLOW_LOG_THRESHOLD_MS` and `REPORT_FORENSICS_ABNORMAL_WARN_THRESHOLD_MS` (`:162–163`), whose only uses are inside `publishData` (`:1485–1540`); they move with `publishData`. The breadcrumb constants (`BREADCRUMB_REPORT_QUEUE_START` … `BREADCRUMB_REPORT_EXIT`) come from `observability/LoopForensics.h` since P-1, and `setAppBreadcrumb` is declared in `state/State_Common.h:178`; the new file includes both.
- **What stays in main and needs a declaration:** `setup()` registers `UbidotsHandler` (`:507`) and calls `publishHibernateWakeForensics` (`:935`); both are only forward-declared in main today (`:115`, `:118`), so the new header declares them (the latter needs `HibernateWakeDiagnostics::EventFields`). Main's `:94` is only a comment and `:112–119` are the forward declarations to remove.
- **Already declared elsewhere:** `publishData` (`state/StateMachine.h:134` and `State_Common.h:158`, with the default argument), `dailyCleanup` (`StateMachine.h:129`, `State_Common.h:151`), `publishDiagnosticSafe` (`State_Common.h:146`, plus its own redeclaration in `MyPersistentData.cpp:51`, which stays). Callers outside main: `State_Report.cpp:69` and `:77`, `State_Connect.cpp:552`, `MyPersistentData.cpp:128`.
- **`dailyCleanup` goes into `state/State_Report.cpp`** (its only caller, `:77`); it uses `publishDiagnosticSafe` (declared in `State_Common.h`).
- **No `retained` keyword and no `appBreadcrumb` variable appear in the moved ranges** (only `setAppBreadcrumb(BREADCRUMB_…)` calls), so the retained check is expected to show nothing moving. The standard negative check applies (recovery plan item 13): the new `.cpp` and `.h` must not contain the `retained` keyword.
- **Tests that anchor on these functions in main** and must be re-pointed, not weakened: `alert19_clears_after_report_test.sh`, `occupancy_report_by_mode_test.sh`, `hibernate_wake_diagnostics_test.sh`, `reset_wiring_test.sh`, `report_payload_golden_test.sh`, `standby_latch_daily_clear_test.sh`, `watchdog_alert_code_test.sh`, `clock_resync_wiring_test.py`, `daily_cleanup_boundary_test.py`, `publish_with_ack_structural_test.py`, `report_payload_fields_test.py`, and the `.cpp` mirrors their drivers compile (`occupancy_report_by_mode_test.cpp`, `report_payload_golden_test.cpp`, `ledger_no_retry_test.cpp`, `persistence_facade_behavior_test.cpp`, `reset_wiring_test.cpp`, `standby_latch_daily_clear_test.cpp`, `watchdog_alert_code_test.cpp`, `connection_mode_downgrade_test.cpp`, `power_manager_accessors_test.cpp`); a bare-name search will find any others.


## Stage 6/7 result, architect's rulings and closing record (2026-10-19)

**Stage 6** (Copilot, Sonnet 5.5, medium): one round. **Controller reproduction** (Claude Code, 6.4.1 Boron, fresh `BUILD_PATH_BASE`s; the "before" is the P-2 tree, whose `src/` equals main `dd90842`): suite 86/86 (sh via zsh, py via python3); fixture sha256 `07de1e1980e4a44e03420540e6c721a93a61b5d6392539219928420c048f0637` unchanged; text/data/bss `151756/1094/2220 → 151820/1094/2228`; the whole `.backup` section identical; the four mutations caught. **Stage 7** (Codex `gpt-5.6-sol`, high): **NOT VERIFIED solely on `git diff --check`**; checks 1–7 PASS, check 8 CONCERN, check 9 FAIL only on whitespace (`WO-2026-10-19-002-stage7-verdict.md`).

**Architect's rulings (19 Oct):**
- **Whitespace waived:** six trailing-whitespace lines are carried byte-for-byte from the original bodies, which have the same whitespace at `dd90842`: `State_Report.cpp:314, :328, :330` (from `dailyCleanup`) and `ReportPublisher.cpp:346, :348, :353` (from `publishDiagnosticSafe`'s neighbourhood and its doc block). **Standing rule for P-4 to P-6:** `git diff --check` hits on moved lines are waived when the original has the same whitespace; hits on new lines still fail.
- **The static-init stubs are benign:** `_GLOBAL__sub_I__Z11publishDatax` (24 B) stores the constants `PRIVATE` and `WITH_ACK` into the new file's own copies, and `_GLOBAL__sub_I__Z20handleReportingStatev` (12 B) stores `PRIVATE` into `State_Report.cpp`'s copy. All run before `setup()`, none calls moved code, no other file reads them during static initialization, and the relative order of every pre-existing initializer is unchanged (Codex check 5).
- **Recovery plan:** `BREADCRUMB_REPORT_LEDGER_DONE` is unpinned (a wrong constant passes all 11 anchoring tests at `HEAD` and 86/86 now); pin it when the report path is next touched. Added as item 13a next to the failsafe item.

## Closing record

| Item | Budget | Actual |
|---|---|---|
| `src/` net | ≤ +20, hard | **+12**: `+395/−383` (main +1/−383; `State_Report.cpp` +32; new `ReportPublisher.cpp` 357 and `.h` 5) |
| Moved lines | about 285 | **285 function lines** (14 + 160 + 12 + 59 + 17 + 23), byte-identical with their doc comments; 340 lines into the new `.cpp` and 31 into `State_Report.cpp`, counted gross |
| Main `wc -l` | about 285 fewer | **1978 → 1596** (−382) |
| Tests | guideline +150 | **+34/−29** across ten re-pointed files plus the new 87-line `report_publisher_move_structural_test.py`: about **+92** |
| Suite | baseline 85 | **86/86** (51 sh via zsh, 35 py via python3); golden payload test 186 vectors, 16 built-in mutants caught |

- **Fixture hash:** `07de1e1980e4a44e03420540e6c721a93a61b5d6392539219928420c048f0637` before and after; the fixture and the golden test's markers are untouched (the markers now sit at `ReportPublisher.cpp:51–124`).
- **ELF (6.4.1 Boron):** text **151756 → 151820 (+64)**, data 1094, bss **2220 → 2228 (+8)**. Text, itemized by symbol: new static-init stubs +24 and +12; `publishStartupStatus` 540 → 548 (+8) and `publishWatchdogForensics` 156 → 164 (+8) (compiler inlining of the same queue wrapper call, no source change); two new 4-byte `.init_array` entries +8; linker fill 268 → 272 (+4); total +64. Bss: three one-byte file-local flag objects plus fill 89 → 94 (+5) = +8. `CSWTCH` tables are renumbered with an unchanged size multiset; the `LocalTimePosixTimezone::operator=` clones and `__tcf_1` are unchanged (an earlier pairing by duplicate names showed false changes).
- **Retained outcome A:** the whole `.backup` section (24 symbols) identical; `retainedBoot` at `0x2003f420`, the signature word at `0x2003f494`. No `retained` keyword in the new files or the moved `dailyCleanup` text; the negative check fails with a named message for a probe in the `.cpp` and in the `.h`.
- **Mutations:** `WITH_ACK` dropped from each of the three publishes (caught by `publish_with_ack_structural_test.py`, and the report one also by the golden test); alerts/resets names swapped (caught by the golden test, not by `report_payload_fields_test.py`); `dailyCleanup()` ahead of `publishData()` (three tests); a second `publishData` definition in main (the new structural test); alert case 19 removed (`alert19_clears_after_report_test.sh`, `watchdog_alert_code_test.sh`); the restart moved ahead of cleanup (`standby_latch_daily_clear_test.sh`, `daily_cleanup_boundary_test.py`).
- **Deviations:** the "Shared notes for report payload construction" doc block stays in main (it is not inside any function range); one declared test adjustment, `daily_cleanup_boundary_test.py` now checks that the `publishData` definition in `ReportPublisher.cpp` omits the default argument, while still requiring exactly one default declaration in `State_Common.h`.
- **Rounds:** Stage 6 one round; Stage 7 once (NOT VERIFIED on whitespace only, waived).
