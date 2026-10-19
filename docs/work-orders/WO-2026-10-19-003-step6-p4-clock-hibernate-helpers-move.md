# WO-2026-10-19-003: Step 6 P-4 — move the clock and hibernate helpers to `time/` (pure move)

**Author:** architect, 2026-10-19 · **Status:** ready (citation re-check done 2026-10-19, below)
**Branch:** `wo/2026-10-19-003-step6-p4-time-helpers`, from main **after #116 (P-3) merges**
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.2–12.4; §13 (b), (c), (g), (j)
**Spec source:** `WO-2026-10-15-003-step0-report.md` Q1 groups "(b) `time/`" (main) and "(b) `time/HibernateCycle`" (Sleep), Q3 row "Time", closing table WO 4. Cited at `71e346f`; main has shifted since, so re-cite everything.

## Plain goal

Move the clock and hibernate helper functions out of main (about 93 lines) and out of `State_Sleep.cpp` (about 85 lines) into `time/`, **unchanged**. No behavior change. This is the first move that shrinks `State_Sleep.cpp`.

## What moves (Codex Q1 at `71e346f`; re-check every range)

- **To a new `time/ClockDiagnostics.{h,cpp}`, from main:** `ab1805WakeReasonNameLocal` (old `:301`), its wrapper `ab1805WakeReasonName` (old `:464`), and `logTimeDiag` (old `:1846`; the v41 TimeDiag-on-change logic moves byte-for-byte).
- **To a new `time/HibernateEligibility.{h,cpp}`, from `State_Sleep.cpp`:** `isDeviceOsAtLeast6400` (old `:221`), `clearAb1805StaleAlarmInterrupts` (old `:239`), `shouldUseBoronRtcAlarmHibernate` (old `:269`).
- `time/HibernateCycle.cpp` already calls the wake-reason wrapper; it gains an include only.

## Why new files and not `HibernateCycle.cpp` (architect)

`HibernateCycle.cpp` defines retained hibernate fields (`:22–24`, plus the pending flag). Adding code to that file can change the order in which its retained variables are laid out, the way P-1's first attempt did. New files keep that file's retained layout untouched. Don't put moved code into `HibernateCycle.cpp`.

## Rules (as P-1 to P-3)

1. Bodies move **byte-for-byte**. Allowed: linkage, declarations, includes. Anything else is a STOP. Function-local `static` constants (e.g. `kRegStatus`) move inside their functions unchanged.
2. Retained: the P-0 struct stays at `0x2003f420`; **no decision-bearing retained symbol moves, including the hibernate fields and their pending flag**; report-only shifts are listed. **No `retained` keyword in the new files** (negative checks for each new `.cpp` and `.h`).
3. Whitespace carried byte-for-byte from the original is waived (P-3 rule); whitespace on new lines fails.
4. `ab1805` stays the existing global; the new files reach it by the existing `extern`.

## Budget

- **`src/`: ≤ +20 net non-move lines, hard.** Moved lines count 0; new header, include and namespace lines count. Two new file pairs make this tight; if it can't fit, stop and report the count rather than merging the files into `HibernateCycle.cpp`.
- Expected shrink: main about 93 lines, Sleep about 85 (report `wc -l` before and after for both).
- Tests: guideline +100 (re-pointed anchors, including the TimeDiag-on-change and hibernate tests).

## Tests and checks

- Golden payload test unchanged; suite `N/N`.
- Byte-identical bodies (diff of each moved range against its new home).
- ELF: text change about 0; explain anything over ±64 B by symbol. Note any new static-init stubs and rule them by the P-3 test (per-file header objects only).
- `nm` retained check under the narrowed rule; the hibernate fields' addresses are listed before and after.
- Mutations the controller re-runs: make `logTimeDiag` log on every call (the v41 flood) and remove the AB1805 stale-alarm clear; each must fail a test. If either passes at HEAD too, record it as a pre-existing gap (like items 12 and 12a), not a P-4 failure.

## Agents and models

- **Stage 6:** Copilot, Sonnet tier, **medium**.
- **Controller (before Stage 7, §13 (g)):** reproduce the build, `nm`, the mutations and the suite.
- **Stage 7:** Codex `gpt-5.6-sol`, **medium**. Helpers, byte-identical, with a mechanical retained check.

## Bench

None in this WO. Ships in v45; the v45 soak watches TimeDiag and hibernate wakes.

## Closing record

Budget versus actual (non-move `src/`, moved lines, main and Sleep `wc -l`, tests), ELF by symbol, retained outcome with the hibernate fields' addresses.

## Claude Code's citation re-check (2026-10-19, main `768c3c0`, P-0 to P-3 merged; `Generalized-Core-Counter.cpp` is 1596 lines, `State_Sleep.cpp` 1814)

**The ranges, start to closing brace** (old WO numbers in brackets). **Main:** `ab1805WakeReasonNameLocal` `:167–181` [`:301`] (its doc comment starts at `:158`; it sits inside main's anonymous namespace `:155–212`); the external wrapper `ab1805WakeReasonName` `:214–216` [`:464`]; `logTimeDiag` `:1280–1354` [`:1846`]: 15 + 3 + 75 = **93 lines, as the WO says**. **Sleep:** `isDeviceOsAtLeast6400` `:221–237` [`:221`], `clearAb1805StaleAlarmInterrupts` `:239–267` [`:239`], `shouldUseBoronRtcAlarmHibernate` `:269–307` [`:269`]: 17 + 29 + 39 = **85 lines, as the WO says**.

**Findings the implementer must plan for (not WO changes):**
- **The three Sleep functions are under `#if PLATFORM_ID == PLATFORM_BORON` (`State_Sleep.cpp:220–308`) and inside `State_Sleep.cpp`'s anonymous namespace (`:29–310`).** The `#if`/`#endif` guard moves with them (the new `.cpp` and any declaration in the new header need the same guard), and they gain external linkage; their callers are `State_Sleep.cpp:1125` (`shouldUseBoronRtcAlarmHibernate`) and `:1128` (`clearAb1805StaleAlarmInterrupts`). The function-local constants (`kRegStatus` `:240`, `kRegIntMask`, `kMaskAieTie`, `kMinHibernateSleepSec` `:283`) stay inside their functions.
- **`ab1805WakeReasonNameLocal` is internal (main's anonymous namespace); the wrapper `ab1805WakeReasonName` is the external symbol.** `time/HibernateCycle.cpp:9` already declares `extern const char *ab1805WakeReasonName(AB1805::WakeReason reason);` and uses it at `:66, :92`, so **no edit to `HibernateCycle.cpp` is needed. Leave that file untouched**: it defines the hibernate retained fields (rule 2 and the architect's reason above), and the WO's "gains an include" is not required.
- **`logTimeDiag`** is already declared in `state/State_Common.h:199` and called from `State_Idle.cpp:133` and `State_Sleep.cpp:1003`; the declaration stays where it is. The new `ClockDiagnostics.cpp` reaches `ab1805` through the existing `extern` in `state/StateMachine.h:31` (rule 4).
- **No `retained` keyword appears in any moved range** (checked `Generalized-Core-Counter.cpp:158–181, :214–216, :1280–1354` and `State_Sleep.cpp:220–308`), and the moved code uses no file-local symbol of either source file. The standard no-retained negative check applies to the new files.
- **Mentions only in comments:** `time/Clock.h:146` and `time/RtcSkewTest.h:29` name `shouldUseBoronRtcAlarmHibernate()`; leave them.
- **Tests that name the moved functions** (a bare-name search finds): `clock_owner_structural_test.py`, `clock_trust_standard_structural_test.py`, `sleep_duration_consumes_openness_test.py`, `clock_resync_wiring_test.py`, `idle_timediag_on_change_test.cpp`, `hibernate_wake_diagnostics_test.cpp`, `rtc_skew_test.cpp`, `watchdog_ab1805_classification_test.cpp` and the shell drivers that extract from or compile against them; re-point, don't weaken.
- **`src/` budget:** two new file pairs plus a guarded declaration block; if the non-move count passes +20, stop and report it.

