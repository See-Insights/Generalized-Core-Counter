AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-13, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- **Delete** the code listed below from `src/`. The only non-deletion edit allowed is ruling 1's single modified line, which only removes code.
- **Edit `ARCHITECTURE_OVERVIEW.md`:** lines `:11` and `:88` only (ruling 5).
- **Write one patch file,** `build-tmp/WO-2026-10-13-001-commit-a.patch` (see "Two commits").
- Add or update tests under `tests/` if a test breaks. None is expected (Step 0 Q3).
- Run the host suite and local ARM release builds.
- Your scratch directory is **`build-tmp/wo20261013-001-stage6/`**. Remove **only it** when you finish; keep the patch file.

**Not authorized:**
- Any `src/` addition or rewrite beyond ruling 1's line. No moves, no renames, and no persisted field: the layout doesn't change, and `samplingMode` stays.
- Anything in the "2b leftovers" (ruling 6). They stay.
- Any other `docs/` file, `lib/` or Device OS.
- Commits, pushes, merges, branch changes, stash, reset or checkout. Flashing, device or network access beyond the Particle compile.
- Deleting anything you didn't create, apart from the listed `src/` code, including `build-tmp/` itself or the files already in it.

**SIZE BUDGET:** net `src/` ≤ 0, gross `src/` additions 0 (ruling 1's modified line excepted). Step 0 estimates about −450 net; ruling 4 adds about −14. If anything would need an addition, STOP and report.

# Stage 6 dispatch: WO-2026-10-13-001 (Step 6 WO 2a, remove dead code)

**Goal, in plain language:** delete code that nothing in a release build calls, with no change in device behavior.

**Repository:** this worktree, branch `wo/2026-10-13-001-step6-2a-dead-code`, HEAD at the commit that adds this dispatch. Don't switch branches.

**Binding spec:** `docs/work-orders/WO-2026-10-13-001-step6-2a-dead-code.md`, including the architect's rulings 1–8. **The list and its line ranges:** `docs/work-orders/WO-2026-10-13-001-step0-report.md`, Q1 table and Q4. Re-check each range at HEAD before deleting, delete whole declarations and doc comments, and leave no orphaned blank-line runs or dangling `#include`s. Where this dispatch and the WO differ, the WO wins; report the difference.

## What to delete

**Commit (a), the charge-cycle test alone:**
- `runPmicChargeCycleTest`, with its banner comment block, in `SensorManager.cpp` (about `:1273–1418`);
- its declaration and doc in `SensorManager.h` (about `:198–209`).

**Commit (b), everything else** (Step 0 Q1 rows 2–5, plus rulings):
- `SensorManager` `setup`, `getSensorData` and `getSignalStrength`: definitions and declarations;
- the doc example at `SensorManager.h:43–49` (ruling 3);
- **all of `src/cloud/BatteryBackoff.cpp`** (ruling 2), plus the two wrapper declarations and docs in `Cloud.h` (about `:366–393`);
- `PowerManager`: `capabilities`, `hasPmic`, `hasFuelGauge`, `availabilityLabel`, `batteryContextLabel`, `profileSelectionReasonLabel` (ruling 4), `struct PowerInputs`, and `setupComplete_`;
  - **for `setupComplete_`:** delete its declaration (`PowerManager.h:287`) and its write (`PowerManager.cpp:99`), and change `PowerManager.cpp:90` from `: report_(), setupComplete_(false)` to `: report_()` (ruling 1);
- `PowerPlatform::hasFuelGauge`, definition and declaration;
- `PowerTier::label`, definition and declaration;
- `SensorFactory::getSensorTypeName`;
- `PIRSensor::_isrCount`: the declaration, the definition, the `++` in `pirISR`, and its comment (`PIRSensor.cpp:5–6`). **`pirISR` itself stays,** and so does `_motionDetected`;
- `sensorISR`: the declaration (`Generalized-Core-Counter.cpp:116`) and the definition (about `:2709–2717`). `sensorDetect` stays (ruling 6);
- the files `src/state/StateHandlers.cpp` and `src/sensors/SensorData.h`. `StateHandlers.h` is a different, live file and **stays**;
- `ARCHITECTURE_OVERVIEW.md:11` and `:88`: remove or correct the references to the two deleted stubs (ruling 5).

If a file's build list (for example a makefile or `project.properties`) names a deleted file, report it rather than editing it. Particle builds every file under `src/`.

## Two commits (you don't commit)

1. **Do commit (a)'s deletions first.** Run `git diff > build-tmp/WO-2026-10-13-001-commit-a.patch`. It must contain only the two (a) hunks.
2. **Then do commit (b).** Claude Code splits the working tree into the two commits at Stage 8.

Report the line count of each.

## Tests

- **Suite:** full host suite before and after, as `N/N (sh via zsh, py via python3)`. The baseline on this branch is **76/76**. No copy of `src/` under the worktree while it runs.
- **No new test is required,** because nothing changes behavior. If a structural test needs updating, report each change.

## Verification

1. **Local ARM release builds,** each with a fresh `BUILD_PATH_BASE` under your scratch directory:
   - the baseline, built from `git show HEAD:` sources into scratch; expected 151460 / 1094 / 2220;
   - after (a);
   - after (a)+(b).

   Give text/data/bss for each. **Text must not grow.**
2. **`nm` comparison:** `nm -C -S` before against after (a)+(b). List every symbol that disappeared or changed size.
   - **Expected:** only deletions, plus `PIRSensor::pirISR()` and `PowerManager::PowerManager()` shrinking (ruling 7), and `bss` −4 (`_isrCount`).
   - **Any other change:** report it.
3. **Size:** net `src/` lines for (a), (b) and the total; gross additions; `git diff --stat`.

## Implementation Report (required, as your final message)

- Commits (a) and (b): files, with net and gross lines for each.
- Any range that differed from Step 0, and why.
- Tests, with results.
- Builds and sizes, and the `nm` comparison.
- Confirmation that the patch file exists and holds only (a), and that you deleted only your scratch directory.
- Deviations (or "none").
- The model and reasoning level used.
