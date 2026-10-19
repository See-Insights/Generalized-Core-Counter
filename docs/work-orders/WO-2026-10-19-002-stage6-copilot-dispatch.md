AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-19, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- **Edit:**
  - `src/Generalized-Core-Counter.cpp`: remove the moved functions, the two `REPORT_FORENSICS_*` constants and the forward declarations of the moved functions (`:112–119` as applicable), and add the includes it then needs; nothing else;
  - **new** `src/cloud/ReportPublisher.{h,cpp}` (one pair);
  - `src/state/State_Report.cpp`: add `dailyCleanup()` (and the includes it needs) and nothing else;
  - `src/state/State_Common.h`, `StateMachine.h`, `MyPersistentData.cpp`: **only** if the build proves a change is unavoidable (say why); the existing declarations should be enough.
- **Tests:** re-point every test that anchors on text in `Generalized-Core-Counter.cpp` for a moved function so it reads the new home (never weaken it); add the structural test the WO asks for; report each change.
- **Run** the host suite and local ARM release builds.
- **Scratch directory:** **`build-tmp/wo20261019-002-stage6/`**. Remove **only it** when you finish.

**Not authorized:**
- Any change to a moved function body, comment or constant value.
- **`tests/fixtures/report_payload_golden.txt` and the golden test's markers** (`static const char *batteryContext[7] = {` … the `PublishQueuePosix::instance().publish(webhookName, data, PRIVATE | WITH_ACK)` line): the test is re-pointed to the new file without touching the fixture.
- The close order in `State_Report.cpp` (publish → `dailyCleanup()` → restart), `publishStartupStatus`, `publishWatchdogForensics`, `setup()`'s registration of `UbidotsHandler`, the `retained` definitions (including `retainedBoot`), `diagnostics/`.
- Any other `src/` change, `docs/`, `lib/`, `project.properties` or Device OS.
- Commits, pushes, merges, branch changes, stash, reset, checkout or restore. Device or network access beyond the Particle compile. Deleting anything you didn't create, including `build-tmp/` itself.

**SIZE BUDGET:**
- **`src/`:** **≤ +20 net non-move lines, HARD**: moved lines count 0, reported separately as gross; every new header line, include and namespace line counts. Over means STOP and report (§13 (b)).
- **Tests:** guideline **+150** (about 15 test files anchor on these functions); record the actual.

# Stage 6 dispatch: WO-2026-10-19-002 (Step 6 P-3: move report publishing and the daily cleanup out of main)

**Goal, in plain language:** move the hourly report's publishing code (285 function lines) out of `Generalized-Core-Counter.cpp`: five functions into a new `src/cloud/ReportPublisher.{h,cpp}`, and `dailyCleanup()` into `src/state/State_Report.cpp`, its only caller. **Unchanged.** The webhook payload stays byte-for-byte identical; the golden payload test is the main guard.

**Repository:** this worktree, branch `wo/2026-10-19-002-step6-p3-report-publishing` (from main `dd90842`, P-0, P-1 and P-2 merged). HEAD is at the commit that adds the WO and this dispatch. Don't switch branches. Main is Device OS 6.4.1; build with the 6.4.1 toolchain.

**Binding spec:** `docs/work-orders/WO-2026-10-19-002-step6-p3-report-publishing-move.md`, including **Claude Code's citation re-check at `dd90842`** (the ranges, the findings, the golden fixture hash). Where this dispatch and the WO differ, the WO wins; report the difference.

## Do

1. Move the six functions byte-for-byte (`isAutoClearAfterReportAlert` stays `static`, internal to the new `.cpp`); move `REPORT_FORENSICS_SLOW_LOG_THRESHOLD_MS` and `REPORT_FORENSICS_ABNORMAL_WARN_THRESHOLD_MS` with `publishData`, their only user. Allowed edits: linkage keywords, declarations, includes.
2. The new header declares `UbidotsHandler` and `publishHibernateWakeForensics` (main's `setup()` still uses them: `:507`, `:935`); `publishData`, `publishDiagnosticSafe` and `dailyCleanup` are already declared in `State_Common.h`/`StateMachine.h`. The new `.cpp` includes `state/StateMachine.h` (for `state`, `session`), `observability/LoopForensics.h` (the `BREADCRUMB_*` constants) and `state/State_Common.h` (`setAppBreadcrumb`).
3. **No `retained` keyword in the new `.cpp` or `.h`** (standard negative check since P-2): the new structural test, modelled on `tests/connectivity_failsafe_move_structural_test.py`, must fail if the `retained` keyword (after stripping comments and string literals) appears in either file, and it must also check that no moved function is still defined in main, that each is defined exactly once, and that `dailyCleanup` is defined once, in `State_Report.cpp`, without the `retained` keyword.
4. Re-point the tests the WO's re-check lists, and any others a bare-name search finds, to the new home with the same assertions. The golden payload test reads the new file; its fixture and markers are untouched (record the fixture's sha256 before and after: `07de1e1980e4a44e03420540e6c721a93a61b5d6392539219928420c048f0637`).

## Verify (all required)

- **Byte-identical bodies:** every moved range at `HEAD` against its new home.
- **Golden payload test** passes, with the fixture hash unchanged; **full suite** before and after, `N/N (sh via zsh, py via python3)`; the baseline at `HEAD` is 85. Run shell tests with `zsh`. No copy of `src/` under the worktree while the suite runs.
- **Mutations, each in a scratch copy, each must fail a named test:** (1) drop `WITH_ACK` from the report publish; (2) swap the `alerts`/`resets` order in one layout of the payload; (3) move the `dailyCleanup()` call ahead of `publishData(...)` in `State_Report.cpp`; (4) a `retained` probe in the new `.cpp` and in the new `.h`.
- **Structural:** no moved function still defined in main; no duplicate definitions.
- **Local ARM release build** (6.4.1 Boron; a fresh `BUILD_PATH_BASE`): text/data/bss before (from `git archive HEAD`) and after; explain any text change over ±64 B by symbol with `nm -C -S`. **Retained check under the narrowed rule:** the whole `.backup` section before and after; `retainedBoot` stays at `0x2003f420`, the signature word at `0x2003f494`; list and classify any move.
- **Main `wc -l`** before (1978) and after.

## Implementation Report (required, as your final message)

- **Files:** new and changed, with net and gross `src/` lines and the arithmetic against +20; test lines added and removed; each re-pointed test and why it is not weaker.
- **Linkage and placement choices**, with file:line.
- **Results:** the byte-diff, the fixture hash before and after, the suite before and after, the four mutations and the tests that caught them, the build sizes and symbol-level text deltas, the full `.backup` table.
- **Confirmations:** you removed only your scratch directory; the fixture is untouched.
- **Deviations** (or "none"), and **the model used**.
