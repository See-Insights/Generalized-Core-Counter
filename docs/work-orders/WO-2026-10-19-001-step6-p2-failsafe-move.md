# WO-2026-10-19-001: Step 6 P-2 — move the connectivity failsafe out of main (pure move)

**Author:** architect, 2026-10-19 · **Status:** ready (citation re-check done 2026-10-19, below)
**Branch:** `wo/2026-10-19-001-step6-p2-failsafe`, from main **after #112 (P-1) merges**
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.2–12.4; §13 (b), (c), (g), (j)
**Spec source:** `WO-2026-10-15-003-step0-report.md` Q1 group "(c) new `power/ConnectivityFailsafe`", Q3 row "Connectivity failsafe", closing table WO 2. Cited at `71e346f`; P-1 shifted main's lines, so re-cite everything.

## Plain goal

Move the connectivity failsafe (about 274 lines) from `Generalized-Core-Counter.cpp` into a new `power/ConnectivityFailsafe.{h,cpp}`, **unchanged**. No behavior change. This is the device's last-resort recovery (stage 2 system reset, stage 3 deep power-down), so "unchanged" is checked strictly.

## What moves (Codex Q1 at `71e346f`; re-check every range on the new base)

- Helpers: `currentBatteryTierForFailsafe`, `connectivityFailsafeHasExternalPower`, `connectivityFailsafeJitterSec`, `persistConnectivityFailsafeState` (old `:353, :367, :384, :402`, and the tier/source helper at `:338`);
- the defer-reason label and claim functions and their wrappers (old `:414, :433, :460, :468`), including `failsafeDeferReasonName`;
- `clearConnectivityFailsafeRecovery` (old `:2506`) and `connectivityFailsafeSupervisor` (old `:2546`).
- Their declarations move from `state/State_Common.h:192` and `:255` to the new header; `State_Connect.cpp:514` and `State_Sleep.cpp:1037` include it.

## Rules (as P-1, plus three)

1. Bodies move **byte-for-byte**. Allowed: linkage, declarations, includes. Anything else is a STOP.
2. Retained: the P-0 struct stays at `0x2003f420`; no decision-bearing retained symbol moves; report-only shifts are listed (narrowed rule, 18 Oct). The failsafe uses persisted file state, not retained RAM; confirm.
3. **Both builds compile and pass:** the release build and the `CONNECTIVITY_FAILSAFE_TEST_MODE=1` bench build (`BuildProfile.h:103–118`).
4. **Leave `diagnostics/ConnectivityFailsafeTest.cpp` alone,** including its duplicate `...Local()` copies of the tier and external-power helpers (`:61`, `:72`). Note them for a later clean-up, not here.
5. The stage-1-retired comment, the cadence rule and the low-battery block move with the code; no edits to their logic or text.

## Budget

- **`src/`: ≤ +20 net non-move lines, hard.** Moved lines count 0; every new header line, include and namespace line counts.
- Expected main shrink: about 274 lines (report `wc -l` before and after).
- Tests: guideline +100.

## Tests and checks

- Golden payload test unchanged; suite `N/N`; the existing failsafe tests (`failsafe*`, the `WO-2026-10-08-001` tests) re-pointed, not weakened.
- **Byte-identical bodies:** a diff of each moved range against its new home.
- ELF: text change about 0; explain anything over ±64 B by symbol.
- `nm` retained check under the narrowed rule.
- Structural: no failsafe function left in main; no duplicate definitions.

## Agents and models

- **Stage 6:** Copilot, Sonnet tier, **medium**. A mechanical move.
- **Controller (before Stage 7, §13 (g)):** reproduce both builds, the `nm` check and the suite.
- **Stage 7:** Codex `gpt-5.6-sol`, **high**. It's the last-resort recovery path, so check that every branch (open-hours gate, cadence rule, cooldown and jitter, low-battery block, stage 2/3 actions, defer logging) is byte-identical and still reached from the same call sites.

## Bench

None in this WO. Ships in v45 with P-0 to P-6.

## Closing record

Budget versus actual (non-move `src/`, moved lines, main `wc -l`, tests), ELF by symbol, retained outcome, both builds.

## Claude Code's citation re-check (2026-10-19, main `3975e9c`, P-0 and P-1 merged; `Generalized-Core-Counter.cpp` is 2280 lines)

**The ranges, start to closing brace** (old WO numbers in brackets): `currentBatteryTierForFailsafe` `:226–238` [`:338`]; `connectivityFailsafeHasExternalPower` `:240–255` [`:353`/`:367`]; `connectivityFailsafeJitterSec` `:257–273` [`:384`]; `persistConnectivityFailsafeState` `:275–285` [`:402`]; `failsafeDeferReasonNameLocal` `:287–304` [`:414`]; `claimFailsafeDeferLogLocal` `:306–321` [`:433`]; the wrappers `failsafeDeferReasonName` `:333–335` and `claimFailsafeDeferLog` `:341–343` [`:460`, `:468`]; `clearConnectivityFailsafeRecovery` `:2067–2089` [`:2506`]; `connectivityFailsafeSupervisor` `:2107–2246` [`:2546`]. **That is 260 function lines** (the WO's about 274 includes their comments and blank lines). Forward declarations are at `:119–120`; the call from `loop()` stays at `:1280`.

**Outside citations hold:** `state/State_Common.h:192` (`clearConnectivityFailsafeRecovery`), `:247` (`claimFailsafeDeferLog`, not in the WO) and `:255` (`failsafeDeferReasonName`); `State_Connect.cpp:514`; `State_Sleep.cpp:1035` (claim) and `:1037` (name); `BuildProfile.h:103–118` (`CONNECTIVITY_FAILSAFE_TEST_MODE`, default 0, must be 0 or 1). The `FailsafeDeferReason` enum is `State_Common.h:63` and stays.

**Findings the implementer must plan for (not WO changes):**
- **Moved code uses three main globals and one main function.** `state`, `ab1805` and `session` are already declared in `state/StateMachine.h:25, :31, :78`, so the new file includes that. **`batteryTierShortName` (main `:211`) has no header declaration and its only user is the supervisor (`:2226`).** Either move it with the failsafe (11 lines, a helper of the moved code; state this in the report) or declare it; moving it is the smaller change and keeps the budget. Do not leave it defined in main without a declaration.
- **The helpers sit in main's anonymous namespace** (internal linkage), with the external wrappers below. In the new file the helpers may stay internal; only `clearConnectivityFailsafeRecovery`, `connectivityFailsafeSupervisor`, `failsafeDeferReasonName` and `claimFailsafeDeferLog` need external declarations (they already have two in `State_Common.h`, and main needs one for `connectivityFailsafeSupervisor()` at `:1280`).
- **The failsafe state is persisted, not retained:** `persistConnectivityFailsafeState` writes `RecoveryState` and flushes `SystemConfig` (`:275–285`), and none of the moved ranges names a `retained` variable. The retained check is therefore expected to show nothing moving.
- **`CONNECTIVITY_FAILSAFE_TEST_MODE` appears inside the moved ranges** (`#if` blocks around the timings, the open-hours/cadence constants and a defer path): the move must keep both arms, so both builds are required (rule 3).
- **Tests that anchor on text in `Generalized-Core-Counter.cpp`** and must be re-pointed, not weakened (a bare-name search finds): `connectivity_failsafe_open_hours_test.sh`, `failsafe_cadence_rule_test.sh`, `connectivity_failsafe_stage1_retired_test.py`, `power_consumer_regression_test.sh` (extracts `currentBatteryTierForFailsafe`), `battery_authority_commit_sites_structural_test.py`, `clock_trust_standard_structural_test.py`, `loop_stage_sleep_prep_exclusion_test.py`, and whatever else a search for the moved names turns up.
- `diagnostics/ConnectivityFailsafeTest.cpp` (rule 4) has its own `...Local()` helpers and `batteryTierShortNameLocal` (`:46`, `:61`, `:72`) and calls `failsafeDeferReasonName` and `claimFailsafeDeferLog` through `State_Common.h`; it stays untouched.


## Stage 6/7 result, architect's rulings and closing record (2026-10-19)

**Stage 6** (Copilot, Sonnet 5.5, medium): one implementation round, plus one narrow follow-up after Stage 7 (below). **Controller reproduction** (Claude Code, 6.4.1 Boron, fresh `BUILD_PATH_BASE`s; the release "before" is the P-1 tree, whose `src/` equals main `3975e9c`): suite 85/85 (sh via zsh, py via python3); release text/data/bss 151756/1094/2220 before and after; bench (`CONNECTIVITY_FAILSAFE_TEST_MODE=1`) 153260/1094/2220 before and after; no symbol appeared, vanished or changed size apart from compiler `CSWTCH` renumbering; the whole `.backup` section identical. **Stage 7** (Codex `gpt-5.6-sol`, high): **NOT VERIFIED on test strength only**; checks 1–5, 7 and 8 PASS (`WO-2026-10-19-001-stage7-verdict.md`): the 11 moved definitions byte-identical including every test-mode arm, the supervisor's branches and call sites intact, both builds and the retained layout identical (outcome A), protected files unchanged.

**The two Stage 7 findings, both P2:**
1. **A `retained` variable added to the new file was not rejected** by `connectivity_failsafe_move_structural_test.py` (its regex matched only `retainedBoot` and `retainedLoopForensics`). Fixed by a narrow test-only follow-up (+11 lines): after stripping comments and string literals, the test fails if the keyword `retained` appears at all in `ConnectivityFailsafe.h` or `.cpp`. **Checked by the controller:** a retained probe in the `.cpp` fails with "ConnectivityFailsafe.cpp must not use the 'retained' keyword", the same probe in the `.h` fails with the matching `.h` message, the unmodified test passes, and the suite is 85/85.
2. **The test-mode jitter arm is unpinned.** A one-second change to `jitterSec(stage)` in the `CONNECTIVITY_FAILSAFE_TEST_MODE` arm passes all seven re-pointed tests and the new one. **This is pre-existing:** the same mutation on the unmoved code at `HEAD` is caught by 0 of 8 related tests (`connectivity_failsafe_open_hours`, `failsafe_cadence_rule`, `power_consumer_regression`, `connection_mode_downgrade`, `connectivity_failsafe_stage1_retired`, `connection_mode_downgrade_structural`, `clock_trust_standard_structural`, `battery_authority_commit_sites_structural`). Not a P-2 defect; recorded in the recovery plan.

**Architect's rulings (19 Oct):** the controller's reproduction suffices (the follow-up is test-only and the `src/` tree matches Codex's candidate; checked: the `src/` hash is `045125728886ce35`, equal to the reviewed tree's). Stage 8 pre-approved on those conditions, all met. **The "no `retained` in the new files" negative check becomes standard in P-3 to P-6.** The unpinned test-mode arms go into the recovery plan; pin them when the failsafe is next touched.

## Closing record

| Item | Budget | Actual |
|---|---|---|
| `src/` non-move | ≤ +20, hard | **+15**: new header 3 + new-file include and namespace lines 13 + 1 include in main − 2 removed forward declarations (`+318/−303` physical) |
| Moved lines | about 274 | **260 function lines** (289 content lines, 301 physical with blanks), byte-identical |
| Main `wc -l` | about 274 fewer | **2280 → 1978** (−302) |
| Tests | guideline +100 | **+96/−11** (net +85), including the +11 follow-up |
| Suite | baseline 84 | **85/85** (51 sh via zsh, 34 py via python3); golden payload test 186 vectors, fixture unchanged |

- **Builds (6.4.1 Boron):** release text 151756 → 151756, bench 153260 → 153260, data 1094 and bss 2220 unchanged in both; zero text delta by symbol (only same-sized `CSWTCH` renames).
- **Retained outcome A:** the whole `.backup` section (24 symbols) identical; `retainedBoot` at `0x2003f420`, the signature word at `0x2003f494`. The failsafe's state is persisted (`RecoveryState`, `SystemConfig::flushNow()`), not retained.
- **Deviations from the WO:** `batteryTierShortName` moved with the failsafe (no header declaration, only user is the supervisor); two tests the WO did not name were re-pointed (`connection_mode_downgrade_structural_test.py`, `connection_mode_downgrade_test.sh`); `clock_trust_standard_structural_test.py` now scans the new file and is stricter than before. `diagnostics/ConnectivityFailsafeTest.cpp`, `State_Common.h`, `State_Connect.cpp`, `State_Sleep.cpp` and `BuildProfile.h` are byte-identical to `HEAD`.
- **Rounds:** Stage 6 round 1, Stage 7 (NOT VERIFIED on tests), one test-only follow-up, accepted by the architect without a second Codex run.
