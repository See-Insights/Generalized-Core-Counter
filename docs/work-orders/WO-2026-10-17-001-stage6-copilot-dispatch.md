AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-17, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- **Edit:**
  - `src/Generalized-Core-Counter.cpp`: define the retained struct in place of the nine variables at `:491–499`, keep every existing name working, and add the magic and version check as the first statement of `setup()`;
  - **new** `src/persist/RetainedBoot.h` (or similar, your call, stated in the report) **only** if the check and the reset need to be host-testable; it counts against the budget.
- **Tests:** add the host test and the structural test the WO lists; update any test that anchors on the old `retained uint8_t bootStormCount = 0;` style text, without weakening it (report each); adjust only what this change requires elsewhere, reporting each.
- **Run** the host suite and local ARM release builds.
- **Scratch directory:** **`build-tmp/wo20261017-001-stage6/`**. Remove **only it** when you finish.

**Not authorized:**
- Any change to a boot-storm decision, to what the nine variables mean, to `retainedLoopForensics`, the hibernate fields, `lastTrend*`, or the report-only counters in other files.
- Any other `src/` change, `docs/`, `lib/`, `project.properties` or Device OS.
- Commits, pushes, merges, branch changes, stash, reset or checkout. Device or network access beyond the Particle compile.
- Deleting anything you didn't create, including `build-tmp/` itself.

**SIZE BUDGET:**
- **`src/`:** net **≤ +20, HARD** (estimate about +18): over means STOP and report (§13 (b)). One commit's worth of change.
- **Tests:** a **guideline** of +100 added lines. Record the actual; don't stop for it unless it passes about twice the guideline.

# Stage 6 dispatch: WO-2026-10-17-001 (Step 6 P-0: pin the retained layout)

**Goal, in plain language:** put today's unvalidated retained variables in `Generalized-Core-Counter.cpp` into one `retained` struct with a magic and a version, checked before anything reads them, so that no later code move can shift them and a layout change resets them cleanly. No other behaviour change.

**Repository:** this worktree, branch `wo/2026-10-17-001-step6-p0-retained-layout` (from main `5a28885`). HEAD is at the commit that adds the WO and this dispatch. Don't switch branches. Main is Device OS 6.4.1; build with the 6.4.1 toolchain.

**Binding spec:** `docs/work-orders/WO-2026-10-17-001-step6-p0-pin-retained-layout.md`, including **Claude Code's citation re-check at `5a28885`** and its Stage 6 observations. Where this dispatch and the WO differ, the WO wins; report the difference.

## Design (from the WO; do not widen)

1. One `retained` struct holding the nine variables (`bootStormCount`, `bootInProgress`, `bootStormWindowStart`, `bootStormLastResetReason`, `bootStormLastResetReasonData`, `bootStormTripCount`, `bootStormAlertPending`, `appBreadcrumb`, `appBreadcrumbMs`), `magic` (32-bit) and `version` (8-bit) first. Member order fixes the layout.
2. The first statement of `setup()` (ahead of `ensureRetainedLoopForensicsInitialized()` at `:788`, which can read `appBreadcrumb` through `:616` and `:634`) checks magic and version. If either is wrong, set every member to today's initial value (`0`, `false`, `BREADCRUMB_NONE`), then write the magic and version.
3. Keep every existing name (references or another smaller technique), so the other uses don't change.
4. **Mind the post-OTA boot:** after the reset the boot-storm counters are zero and `bootInProgress` is false, so the first boot after the OTA **must not** raise a boot-storm alert or take the holdoff sleep at `:873–882`. The reset reason on that boot is 70 (`RESET_REASON_UPDATE`, not one that counts toward a storm), but test the zero-state path directly.

## Verify (all required)

- **Host test:** magic wrong or version wrong → every member at its initial value, magic and version written; both right → values kept. A test that, from the reset state, the `:873` condition is false for any reset reason.
- **Structural test:** the check precedes every read of a member in `setup()` (the first reads are listed in the WO's re-check).
- **`nm`:** the struct is one `.backup` symbol; record its address and size. List **every** `.backup` symbol before and after with its address; classify each one that moved (report-only counter, sentinel-checked, guarded, or other) and say whether `__backup_sram_signature` moved. The WO's re-check expects later symbols may shift; report, don't hide.
- **Golden payload test** unchanged; **full suite** before and after, `N/N (sh via zsh, py via python3)`. The baseline is the current count at `HEAD` (81). Run shell tests with `zsh`. No copy of `src/` under the worktree while the suite runs.
- **Local ARM release build** with the same release recipe as WO-2026-10-16-001's Stage 6 (not `compile-all`) and a fresh `BUILD_PATH_BASE`: text/data/bss before (from `git show HEAD:` sources) and after; explain any text change over ±64 B by symbol with `nm -C -S`.

## Implementation Report (required, as your final message)

- **Files:** new and changed, with net and gross `src/` lines and the arithmetic against the +20 budget; test lines added and removed.
- **Design choices:** the struct, the member order, how names are kept, where the check sits.
- **Results:** the host and structural tests; the suite before and after; the full `nm` table with classification; the build sizes and the symbol-level text delta.
- **Confirmations:** you removed only your scratch directory.
- **Deviations** (or "none"), and **the model used**.
