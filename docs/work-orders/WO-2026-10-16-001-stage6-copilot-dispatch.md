AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-17, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- **Edit:**
  - `src/Generalized-Core-Counter.cpp`: remove the moved functions and their file-static state, add the includes the file then needs, and nothing else;
  - **new** `src/observability/LoopForensics.{h,cpp}` and `src/observability/AwakeWatchdog.{h,cpp}` (or one file; your call, stated in the report);
  - `src/state/State_Common.h` and `src/state/StateMachine.h` **only** to keep the existing declarations the moved functions already have there (`setAppBreadcrumb`, `setLoopStage`, `serviceAwakeWatchdog`, `pauseAwakeWatchdogForSleep`, `restoreAwakeWatchdogAfterWake`), and only if an include move forces it.
- **Tests:** re-point any test that anchors on `Generalized-Core-Counter.cpp` text for a moved function so it reads the new file (never weaken it); add the structural tests the WO lists; adjust only what this change requires elsewhere, reporting each.
- **Run** the host suite and local ARM release builds.
- **Scratch directory:** **`build-tmp/wo20261016-001-stage6/`**. Remove **only it** when you finish.

**Not authorized:**
- Any change to a function body (WO rule 1: byte-for-byte), to the breadcrumb enum's location, to where the `retained` variables are defined, or to `publishStartupStatus`, `publishWatchdogForensics` and the startup globals.
- Any other `src/` change, `docs/`, `lib/`, `project.properties` or Device OS.
- Commits, pushes, merges, branch changes, stash, reset or checkout. Device or network access beyond the Particle compile.
- Deleting anything you didn't create, including `build-tmp/` itself.

**SIZE BUDGET:**
- **`src/`:** **≤ +20 net non-move lines, HARD**: over means STOP and report (§13 (b)). Moved lines count 0, reported separately as gross. Every new header line, include, `extern` and namespace line counts as non-move.
- **Tests:** a **guideline** of +100 added lines. Record the actual; don't stop for it unless it passes about twice the guideline.

# Stage 6 dispatch: WO-2026-10-16-001 (Step 6 P-1: move loop forensics and the awake watchdog out of main)

**Goal, in plain language:** move about 346 lines of loop-forensics and awake-watchdog code out of `Generalized-Core-Counter.cpp` into `src/observability/`, **unchanged**. No behaviour change.

**Repository:** this worktree, branch `wo/2026-10-16-001-step6-p1-observability` (from main `5a28885`). HEAD is at the commit that adds the WO. Don't switch branches. Main is Device OS 6.4.1; build with the 6.4.1 toolchain.

**Binding spec:** `docs/work-orders/WO-2026-10-16-001-step6-p1-observability-move.md`, including **Claude Code's citation re-check at `5a28885`** (all 17 start lines hold; the ranges total 346). Where this dispatch and the WO differ, the WO wins; report the difference.

## Do

1. Move the functions and file-static state the WO lists, **byte-for-byte**. The re-check shows main still uses several of them (`setup()` uses the watchdog statics and both handlers; `publishStartupStatus` uses `appBreadcrumbName` and `loopStageName`; `loopStageForensicName`, `logLoopStageLine`, `ensureRetainedLoopForensicsInitialized` and `noteLoopStageDuration` are called from main), so those need external linkage and a declaration in the new header. That is the only edit to a moved line the WO allows beyond includes (`static` handling), and each one is reported.
2. **`retained` variables stay defined in main** (`:474–500`); the new files reach them through `extern`.
3. Keep the breadcrumb enum and `RetainedLoopForensics` where `State_Sleep.cpp` and the others find them.

## Verify (all required)

- **Byte-identical bodies:** diff every moved range at `HEAD` against the new files; the only differences are the `static` or linkage keyword and indentation if a namespace is used (state which).
- **Golden payload test** unchanged; **full suite** before and after, `N/N (sh via zsh, py via python3)`. The baseline is the current count at `HEAD`. Run shell tests with `zsh`. No copy of `src/` under the worktree while the suite runs.
- **Re-pointed anchors:** list each test you changed and why it is not weaker.
- **Structural:** no moved function still defined in main; no duplicate definitions.
- **Retained layout identical:** `nm` addresses of every `retained` symbol (the `.retained*` section) equal before and after. A difference is a STOP.
- **Local ARM release build** with the same release recipe as WO-2026-10-15-002's Stage 6 (not `compile-all`) and a fresh `BUILD_PATH_BASE`: text/data/bss before (from `git show HEAD:` sources) and after; explain any text change over ±64 B by symbol with `nm -C -S`.
- **Main `wc -l`** before and after (expect about 346 fewer).

## Implementation Report (required, as your final message)

- **Files:** new and changed, with net and gross `src/` lines and the arithmetic; the non-move line count against the +20 budget; test lines added and removed. Whether you used one new file or two, and why.
- **Linkage edits:** every moved symbol whose linkage changed, and each header declaration.
- **Results:** the byte-diff, the suite before and after, the structural checks, the retained-address comparison, the builds and the `nm` differences, the `wc -l` figures.
- **Confirmations:** you removed only your scratch directory.
- **Deviations** (or "none"), and **the model used**.
