AGENT: Copilot · MODEL: claude-sonnet-5.5 (probed 2026-10-25, §5; the Sonnet tier) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- **Edit in `src/`:** `src/state/State_Sleep.cpp` **only**: the occupied branch of `sleepPlan()` and `logWakeSummary()` (both described below).
- **Tests:** the sleep-plan harness `tests/sleep_plan_equivalence_test.{cpp,sh}` (new scenarios; do not re-implement the plan, the harness extracts it by marker), the regenerated fixture `tests/fixtures/sleep_plan_transcript.txt`, `tests/sleep_plan_structural_test.py` and any other test that copy-checks the two changed blocks byte-for-byte, and one new small test for `logWakeSummary()`'s `led=` value. Report every test file touched.
- **Run** the host suite (serially, `.sh` via zsh, `.py` via python3) and the local ARM release build (Boron only, Device OS 6.6.0).
- **Scratch directory:** `build-tmp/wo20261025-001-stage6/`. Remove **only it** when you finish.

**Not authorized:**
- Any change to any other `src/` file; any new file under `src/`; the awake path (`updateOccupancyState()`, `State_Modes.cpp`), `closeOccupancySessionSafely()`, crediting, the COUNTING branch, `signalLED*`, `device_pinout.cpp`, a log line other than the one `/ 1000UL` fix, a breadcrumb, a transition, a persisted field, a new timer, state, flag or config.
- `docs/`, `lib/`, `project.properties`, Device OS.
- Commits, pushes, merges, branch changes, stash, reset, checkout or restore. Device or network access beyond the Particle compile. Deleting anything you didn't create, including `build-tmp/`.

**SIZE BUDGET:**
- **`src/`: expected +5 to +7 net, HARD limit +20,** counted over all lines (additions minus deletions); comments you add count. Anything beyond the design (a helper, a cap, a new constant) is a STOP: report, do not compress (§12.3).
- **Tests:** guideline +100; record the actual and flag it only if it reaches about twice that. **Record ruling 9's numbers** at the base (main `bf4e628`, plus the WO commits): test lines excluding `tests/fixtures/`, fixture lines, suite count, `src/` lines, before and after.

# Stage 6 dispatch: WO-2026-10-25-001 (the occupied sleep wakes at the time left in the debounce; recovery item 20)

**Goal, in plain language:** today an occupied device sleeps the **full** debounce after any wake that isn't a PIR pulse (for example the hourly report), so a session's end is noticed late and the late close time is credited. After this change it sleeps only the **time left** before the LED deadline (the debounce expiry), plus 1 s. Nothing else changes.

**Repository:** this worktree, branch `wo/2026-10-25-001-debounce-sleep-remaining` (from main `bf4e628`). HEAD is the commit that adds this dispatch. Don't switch branches. Main is on Device OS 6.6.0. Compute the `src/` tree hash (`find src -type f | LC_ALL=C sort | xargs shasum -a 256 | shasum -a 256`) before you start and report it, and again at the end.

**Binding spec:** `docs/work-orders/WO-2026-10-25-001-debounce-sleep-uses-remaining-time.md` and `WO-2026-10-25-001-step0-report.md` (the citations, the cases, the fixture expectations). Where this dispatch and the WO differ, the WO wins; report the difference.

## Do

1. **The occupied branch of `sleepPlan()`** (`State_Sleep.cpp`, the `if (CurrentReadings::get_occupied())` block inside the OCCUPANCY branch). Keep `debounceSeconds` as is. Then:
```
const uint32_t remaining = signalLEDTimeRemaining();   // 0 when expired or no valid time
if (remaining > 0) {
  const int untilDeadline = (int)remaining + 1;        // wake just after the deadline
  if (untilDeadline < wakeInSeconds) wakeInSeconds = untilDeadline;
} else if (debounceSeconds < (uint32_t)wakeInSeconds) {
  wakeInSeconds = (int)debounceSeconds;                // today's behavior; never worse
}
```
   replacing the existing `if (debounceSeconds < (uint32_t)wakeInSeconds) { ... }` block; `sleepReason = "debounce"` stays. This is the sketch from Step 0 §3: keep it unless the compiler forces a change, and report any difference. **No cap on `remaining`** (Step 0 §2, Low edge, left for Stage 7).
2. **`logWakeSummary()`:** `(unsigned long)(signalLEDTimeRemaining() / 1000UL)` becomes `(unsigned long)signalLEDTimeRemaining()`. It is already in seconds.
3. **No other `src/` edit.**

## Verify (all required)

- **Plan harness:** add scenarios for an occupied device in OCCUPANCY mode (`mode=1`, `occ=1`) with the LED remaining at **300, 120, 5, 1 and 0 s**, each with the reporting boundary **nearer** (the boundary wake below remaining + 1) and **further** (above it); expected wake = `min(remaining + 1, boundary wake)`, or `min(debounceSeconds, boundary wake)` when the remaining time is 0. The harness extracts the real plan block by marker (never re-implement the formula); keep `COPY_MISMATCH` behavior.
- **Regenerated fixture** (`tests/fixtures/sleep_plan_transcript.txt`): regenerate once with the harness's `--regenerate` mode. **Expected diff, nothing else (Step 0 §4): the 13 scenarios S55-S67 each gain exactly one `CALL signalLEDTimeRemaining` line and no plan value changes; the other 75 scenarios are byte-identical; plus the new appended scenarios.** Show the diff summary (`git diff --stat` and the per-scenario list). Any other difference is a defect: stop and report it.
- **`logWakeSummary()` test:** a small host test that drives the real function (by marker extraction with byte-for-byte copy check, as the other tests do) with the LED remaining at, say, 150 s and asserts the log shows `led=150s`; it must fail on the old `/ 1000UL` code.
- **Mutations (each in a scratch copy made with `git worktree add --detach`, not a copy without `.git`; each must be caught by the check aimed at it, not a compile failure):** (1) restore the full debounce (the old block); (2) drop the `+ 1`; (3) use the remaining time when it is 0 (drop the `remaining > 0` guard); (4) restore the `/ 1000UL`.
- **Full suite** before and after, `N/N` (sh via zsh, py via python3), serial; then the golden payload test unchanged.
- **ARM (Boron, Device OS 6.6.0, fresh `BUILD_PATH_BASE`, `-frandom-seed=wo1025`; "before" from `git archive HEAD`):** the release build; text/data/bss before and after; explain any change over ±64 B by symbol with `nm -C -S`; **retained check:** the `.backup` section before and after (no `retained` added; `ledOffTime` already is). Build command: AI_DEVELOPMENT_WORKFLOW.md §3 Stage 7 ("make -s PLATFORM=boron ... DEVICE_OS_PATH=~/.particle/toolchains/deviceOS/6.6.0 BUILD_PATH_BASE=<fresh dir>").
- `git diff --check` clean on new lines; `wc -l src/state/State_Sleep.cpp` before and after.

## Implementation Report (≤ 150 lines, your final message)

Files changed with net and gross `src/` lines and the arithmetic against the budget; test lines added and removed, the fixture diff list against the Step 0 list, and ruling 9's numbers; the new harness scenarios and their expected values; the `logWakeSummary()` test and the old-code failure; the four mutations and what caught each; the suite before and after; the ARM build with symbol deltas and the `.backup` table; the `src/` hash before and after; scratch removal; deviations from this dispatch (none expected).
