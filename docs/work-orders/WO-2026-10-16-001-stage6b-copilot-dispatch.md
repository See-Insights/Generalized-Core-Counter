AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-18, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- **Edit:** `src/observability/LoopForensics.h` and `LoopForensics.cpp` (the adaptation below), and `src/Generalized-Core-Counter.cpp` **only** if the build proves a change there is unavoidable (report why). Tests: re-point or fix only what the adaptation requires, without weakening any check; report each.
- **Run** the host suite and local ARM release builds.
- **Scratch directory:** **`build-tmp/wo20261016-001-stage6b/`**. Remove **only it** when you finish.

**Not authorized:**
- Any change to a moved function body (byte-for-byte), to the location of the `retained` definitions (`retainedBoot` stays defined in `Generalized-Core-Counter.cpp:355`), or to anything P-0 added.
- Any other `src/` change, `docs/`, `lib/`, `project.properties` or Device OS.
- Commits, pushes, merges, branch changes, stash, reset, checkout or restore; the working tree already holds the P-1 diff (staged and unstaged) and **must be kept**.
- Device or network access beyond the Particle compile. Deleting anything you didn't create, including `build-tmp/` itself.

**SIZE BUDGET:**
- **`src/`:** **≤ +20 net non-move lines, HARD** (the first round measured +16; the adaptation adds about +2). Moved lines count 0, reported separately as gross. Over means STOP and report (§13 (b)).
- **Tests:** guideline +100 (+175 for a new harness); record the actual.

# Stage 6 dispatch, round 2: WO-2026-10-16-001 (Step 6 P-1: re-apply on P-0 main)

**Goal, in plain language:** the P-1 move (about 346 lines of loop forensics and awake-watchdog code out of `Generalized-Core-Counter.cpp` into `src/observability/`, unchanged) was stopped on 17 Oct by a retained-layout check. P-0 (merged) pinned the layout. The saved Stage 6 diff is now applied to main `363b064`; finish it.

**Repository:** this worktree, branch `wo/2026-10-16-001-step6-p1-observability` (main `363b064` merged in). The P-1 diff is applied and **uncommitted**. Main is Device OS 6.4.1; build with the 6.4.1 toolchain.

**Binding spec:** `docs/work-orders/WO-2026-10-16-001-step6-p1-observability-move.md`, including the 17 Oct ruling (the **four accepted deviations**: enums in the header, the `= true` default in the declaration, external linkage for the strategy variable and both handlers, +280 B text explained), the narrowed retained-layout rule (18 Oct) and the "Re-apply on P-0 main" section, which names the one adaptation needed. Where this dispatch and the WO differ, the WO wins; report the difference.

## Do

1. Make the adaptation in the WO's re-apply section: in `LoopForensics.h` replace the two `extern retained … appBreadcrumb / appBreadcrumbMs` lines with `#include "../persist/RetainedBoot.h"` and `extern retained RetainedBoot retainedBoot;`; in `LoopForensics.cpp`, after the includes, add the two `static` references into `retainedBoot` (the technique `Generalized-Core-Counter.cpp:357–361` uses), so the moved bodies stay byte-identical.
2. Build (6.4.1 Boron, fresh `BUILD_PATH_BASE`). Fix anything else the P-0 merge breaks, within the scope above, and report each fix.
3. **Retained check under the narrowed rule:** `nm` the whole `.backup` section before (build `HEAD` via `git archive HEAD`) and after. `retainedBoot` must stay at `0x2003f420` (size 0x20); the signature word must not move; no decision-bearing retained symbol may move. List every moved symbol and classify it (report-only counter, sentinel-checked, guarded, or other). A move of anything not report-only is a STOP.

## Verify (all required)

- **Byte-identical bodies:** every moved range at `HEAD` against the new files; the only differences are the linkage keyword, the `= true` default and the adaptation.
- **Golden payload test** unchanged; **full suite** before and after, `N/N (sh via zsh, py via python3)`; the baseline at `HEAD` is 83. Run shell tests with `zsh`. No copy of `src/` under the worktree while the suite runs.
- **Re-pointed anchors:** list each test you changed and why it is not weaker; `tests/retained_boot_*` and `serial_settle_test.py` (P-0) must still pass unchanged.
- **Structural:** no moved function still defined in main; no duplicate definitions.
- **Local ARM release build:** text/data/bss before and after; the first round measured +280 B text (explained by the strategy variable becoming external); explain any difference from that by symbol with `nm -C -S`. **Main `wc -l`** before (2720) and after.

## Implementation Report (required, as your final message)

- **Files:** new and changed, with net and gross `src/` lines and the arithmetic against +20; test lines added and removed.
- **Adaptation and any other fix** to make it build, with file:line.
- **Results:** the byte-diff, the suite before and after, the structural checks, the full `.backup` `nm` table with classification, the builds and `nm` differences.
- **Confirmations:** you removed only your scratch directory; the working tree still holds the P-1 diff.
- **Deviations** (or "none"), and **the model used**.
