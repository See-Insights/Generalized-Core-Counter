# WO-2026-10-16-001: Step 6 P-1 — move loop forensics and the awake watchdog out of main (pure move)

**Author:** architect, 2026-10-16 · **Status:** ready (citation re-check done 2026-10-17, below)
**Branch:** `wo/2026-10-16-001-step6-p1-observability`, from main
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.2–12.4; §13 (b), (c), (g), (j)
**Spec source:** `WO-2026-10-15-003-step0-report.md` Q1 (group "(c) new `observability/LoopForensics` and `AwakeWatchdog`"), Q3 row "Observability", closing table WO 1. Cited at `71e346f`.

## Plain goal

Move the loop-forensics and awake-watchdog code (about 346 lines) from `Generalized-Core-Counter.cpp` into `src/observability/`, **unchanged**. No behavior change; main gets about 346 lines shorter. This is the first of six power-split moves; the others depend on it.

## What moves (Codex Q1; re-check every range)

- `awakeWatchdogSleepStrategyName` (`:185`), `appBreadcrumbName` (`:232`), `updateRetainedValue` (`:560`);
- the loop-stage names, init, sample, log and set functions (`:568, :590, :612, :620, :627, :666, :696, :735, :741`);
- the watchdog service, pause, restore and handlers (`:1728, :1743, :1788, :1825, :1830`);
- their file-static state (`awakeWatchdog*`, `lastLoopForensicsSnapshotMs`).

Two files, e.g. `observability/LoopForensics.{h,cpp}` and `observability/AwakeWatchdog.{h,cpp}`; one file if that is simpler. Your call, stated in the report.

## Rules for a pure move

1. Function bodies move **byte-for-byte**. Allowed edits: `static` → internal namespace or `static` in the new file, the declarations in the new header, includes. Anything else is a STOP.
2. **`retained` variables stay defined where they are** (`Generalized-Core-Counter.cpp:474–500`, incl. `appBreadcrumb`, `appBreadcrumbMs`, `retainedLoopForensics`, the boot-storm block). Moving a `retained` definition can change its address in retained RAM, and the first boot after an OTA would then read garbage forensics. The new files reach them through an `extern` in the header.
3. The breadcrumb enum stays where `State_Sleep.cpp` and the other users find it today.
4. `publishStartupStatus` / `publishWatchdogForensics` and the startup globals stay in main (Codex Q3: moving them would expose new shared globals).

## Budget

- **`src/`: ≤ +20 net non-move lines, hard.** Moved lines count 0; report gross moved lines separately, and count every new header line, include, `extern` and namespace line as non-move.
- Expected main shrink about 346 lines (report the actual `wc -l` before and after).
- Tests: guideline +100 (re-pointed anchors and any new structural test).

## Tests and checks

- Golden payload test unchanged; suite `N/N`.
- Tests that anchor on `Generalized-Core-Counter.cpp` text for the moved functions are re-pointed, not weakened.
- **ELF:** text change about 0 (inlining across files may shift a little; explain anything over ±64 B by symbol with `nm`).
- **Retained layout identical:** `nm` addresses of every `retained` symbol (the `.retained*` section) equal before and after. A difference is a STOP.
- Structural: no moved function still defined in main; no duplicate definitions.

## Agents and models

- **Stage 6:** Copilot, Sonnet tier, **medium**. A mechanical move.
- **Controller (before Stage 7, §13 (g)):** reproduce the build, the `nm` retained-address comparison and the suite.
- **Stage 7:** Codex `gpt-5.6-sol`, **medium**. Checks: byte-identical bodies (diff of moved ranges), retained addresses, ELF delta by symbol, no behavior-bearing edit. Baseline ELF per the §13 (g) reuse rule.

## Bench

None in this WO. The six moves ship together as one release with a bench soak.

## Closing record

Budget versus actual (non-move `src/`, moved lines, main `wc -l`, tests), ELF delta by symbol, retained-address check.

## Claude Code's citation re-check (2026-10-17, main `5a28885`; the `src/` tree equals `71e346f`'s)

**All 17 start lines hold** in `src/Generalized-Core-Counter.cpp` (`:185, :232, :560, :568, :590, :612, :620, :627, :666, :696, :735, :741, :1728, :1743, :1788, :1825, :1830`), each at the named function. The ranges, start to closing brace, are 9, 60, 7, 21, 21, 7, 6, 33, 29, 38, 5, 10, 14, 44, 35, 3 and 4 lines: **346 in all, matching the WO.** The `retained` block holds at `:474–500` (`appBreadcrumb` `:494`, `appBreadcrumbMs`, `retainedLoopForensics`, the boot-storm block); `main` is 2719 lines.

**Findings the implementer must plan for (not WO changes):**
- **Moved symbols that main still uses need external linkage and a header declaration**, so the "file-static" wording is too narrow:
  - **`setup()`** (`:906–949`) reads and writes `awakeWatchdogInitialized` (`:181`) and `awakeWatchdogStarted` (`:182`), reads `awakeWatchdogSleepStrategy` (`:175`) and `awakeWatchdogSleepStrategyName` (`:929`, `:940`), and passes `awakeWatchdogExpiredHandler` (`:109`, `:920`) and `appWatchdogHandler` (`:111`, `:949`) to the watchdog. Both handlers are `static` today and forward-declared at `:109` and `:111`.
  - **`publishStartupStatus` and friends** use `appBreadcrumbName` (`:1496`, `:1502`) and `loopStageName` (`:1503`); `loopStageForensicName` is used at `:2295`; `logLoopStageLine` at `:2483`; `ensureRetainedLoopForensicsInitialized` at `:788`; `noteLoopStageDuration` in `loop()` (`:1612–1722`).
  - Already declared elsewhere: `setAppBreadcrumb` and `setLoopStage` (`State_Common.h:178`, `:185`), and `serviceAwakeWatchdog`, `pauseAwakeWatchdogForSleep`, `restoreAwakeWatchdogAfterWake` (`StateMachine.h:83–97`).
  - Used only inside the moved ranges: `refreshRetainedLoopForensics`, `updateRetainedValue` (a template, so it lives in a header or stays with its one user), `resetRetainedLoopForensics`.
- **Budget:** each new `extern`, declaration, include and namespace line is non-move, against the hard ≤ +20. My count is about 15 declarations plus includes; one new header keeps it lowest. If it can't fit, STOP and report (§13 (b)).
- **External callers** of the moved functions are in `SensorManager.cpp:198`, `State_Connect.cpp`, `State_Idle.cpp` and `State_Sleep.cpp`, all through the existing declarations above.
- **Build base:** this WO builds on Device OS 6.4.1 (main today). The v44 release (6.6.0) isn't merged yet; the P-moves rebuild on 6.6.0 once it is (see the v44 WO §1).


## Stage 6 result and architect's ruling (2026-10-17)

**Stage 6 STOPPED on the retained-layout check, and the controller reproduced it** (two fresh 6.4.1 Boron builds, `HEAD` against the working tree): four retained symbols moved, `bootStormCount` `0x2003f424 → 0x2003f41c`, `bootStormWindowStart` `0x2003f428 → 0x2003f420`, `appBreadcrumb` `0x2003f41c → 0x2003f428`, `appBreadcrumbMs` `0x2003f420 → 0x2003f42c`; the signature word stayed at `0x2003f494`, so the shifted values would be read as valid on the first boot after an OTA. v42 and v43 have identical retained layouts, so no earlier OTA was affected.

**Ruling: STOP confirmed; pin the layout first.**
- **P-0** (WO-2026-10-17-001) puts the unvalidated retained variables in main in one struct with a magic and a version, checked at the start of `setup()`.
- **This branch is left as is.** Copilot's diff is saved as a patch, 6 files, 1104 lines (`src/Generalized-Core-Counter.cpp`, new `src/observability/LoopForensics.{h,cpp}`, three tests), outside the repository in the ignored `build-tmp/saved-patches/wo-2026-10-16-001-p1-observability-move.patch` (`diff -ruN before/ after/`, apply with `patch -p1` from the repo root after stripping the `before/`, `after/` prefixes), sha256 prefix `259a0d33a9dc6029`. The working tree on this branch still carries the same uncommitted diff.
- **After P-0 merges,** re-apply the patch on main under the new rule: **the P-0 struct and every unvalidated retained symbol stay put; validated ones may move.** Re-run the `nm` comparison for the whole `.backup` section.
- **Deviations accepted by the architect:** (1) `AppBreadcrumb` and `AwakeWatchdogSleepStrategy` live in `LoopForensics.h`; (2) the `= true` default of `noteLoopStageDuration` moved to the header declaration; (3) external linkage for `awakeWatchdogSleepStrategy` and both handlers, because `setup()` uses them (`:920`, `:929`, `:940`, `:949`); (4) text +280 B (151396 → 151676), caused by the strategy variable now being external, so the compiler can't fold away the manual-stop branch (`pauseAwakeWatchdogForSleep` +92 B).
- **Other figures kept from the Stage 6 report:** net non-move `src/` +16 against the +20 limit; moved lines about 436 gross (346 function lines); main 2719 → 2279 lines; suite 81/81 before and 82/82 after; one new file pair, not two.

## Narrowed retained-layout rule (architect, 2026-10-18)

P-0 round 1 is accepted (WO-2026-10-17-001). The rule for re-applying this patch is **narrowed**: *a decision-bearing retained value must be validated or stay put; report-only diagnostics (the ThrashGuard trio, the PMIC-anomaly fields) may shift,* with "first post-OTA values invalid" recorded for the release. This replaces the 17 Oct wording "every unvalidated retained symbol stays put". After P-0 merges, re-apply the saved patch on main and compare the whole `.backup` section by `nm`: the P-0 struct (`retainedBoot`, `0x2003f420`), `retainedLoopForensics`, `ledOffTime`, `lastTrend*` and the hibernate fields must not move unless validated; any other move is listed and classified in the closing record. The four deviations accepted on 17 Oct stand.

