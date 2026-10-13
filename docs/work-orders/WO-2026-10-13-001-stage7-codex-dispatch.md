AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-13, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-13-001-step6-2a-dead-code`).
- Run the host suite in place, and local ARM builds in scratch copies.
- Write temporary files under **`build-tmp/wo20261013-001-stage7/`**, and remove **only that directory**.
- Mutate only in scratch copies.

**Not authorized:**
- Changes outside your scratch directory.
- Commits, pushes, stash, reset or checkout.
- Device, network or AWS access.
- Deleting anything you didn't create, including `build-tmp/` itself or the files already in it (in particular `build-tmp/WO-2026-10-13-001-commit-a.patch`).

# Stage 7 dispatch: WO-2026-10-13-001 (Step 6 WO 2a, remove dead code)

**Goal, in plain language:** delete code that nothing in a release build calls, with no change in device behavior.

**Binding spec:** `docs/work-orders/WO-2026-10-13-001-step6-2a-dead-code.md`, including the architect's rulings 1–8.

**Inputs:**
- `docs/work-orders/WO-2026-10-13-001-step0-report.md` (the Q1 table);
- `docs/work-orders/WO-2026-10-13-001-stage6-copilot-dispatch.md`;
- Copilot's report (the end of `build-tmp/WO-2026-10-13-001-stage6-copilot-output.log`).

Treat every claim as unverified.

**What changed** (uncommitted, against HEAD):
- 17 files: `src/` has 1 insertion and 461 deletions, and `ARCHITECTURE_OVERVIEW.md` has −2.
- **Deleted whole files:** `src/cloud/BatteryBackoff.cpp`, `src/state/StateHandlers.cpp`, `src/sensors/SensorData.h`.
- **The one insertion** is ruling 1's `PowerManager::PowerManager() : report_() {`.
- **Commit (a)** is `build-tmp/WO-2026-10-13-001-commit-a.patch`: the charge-cycle test in `SensorManager.cpp`/`.h` only.
- **Copilot's deviation:** it did **not** delete `PowerManager::profileSelectionReasonLabel` (ruling 4). Copilot says, and Claude Code confirmed, that it is called unqualified at `PowerManager.cpp:206`, so deleting it would require an addition.

## Test-suite hygiene

`tests/thermal_coupling_structural_test.py` scans the whole worktree. Run the host suite before you create any copy of `src/`, or after removing it.

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

1. **Nothing deleted is used.** For every deleted symbol and file, search all of `src/` (and `lib/` if relevant), including every `#if` arm of `BuildProfile.h`'s live flags. Check unqualified calls inside member functions and calls through other names: this is the trap Step 0 fell into with ruling 4's function.
2. **Deletions only.** `git diff -U0 -- src` adds exactly the one ruling-1 line. No moved or renamed code, no persisted field, and the persisted layout is unchanged (`samplingMode` stays). `pirISR` and `_motionDetected`, `sensorDetect`, `StateHandlers.h`, and every "2b leftover" (ruling 6) are still present.
3. **The two-commit split.** `git apply --check` the commit-(a) patch against HEAD. It must contain only the charge-cycle test hunks. Applying it, then the rest of the working tree, must reproduce the working tree exactly. Give the net lines for (a), (b) and the total.
4. **Builds and size.** Build HEAD (`git archive HEAD` into scratch) and the working tree, each with a fresh `BUILD_PATH_BASE`. Give text/data/bss for both. Text must not grow; Copilot claims 151460 → 151372, and bss 2220 → 2212.
5. **`nm` before/after (ruling 7).** List every symbol that disappeared, appeared or changed size.
   - **Allowed by ruling 7:** `PIRSensor::pirISR()` and `PowerManager::PowerManager()` shrinking.
   - **Explain each one Copilot reports, and say whether it is a mechanical consequence of a deletion** (with evidence, for example disassembly or the deleted caller). If so, it's a CONCERN for the architect; if not, it's a FAIL.
     - `PowerManager::setup()` shrinking;
     - the `PowerManager::instance()` static object shrinking by 4 bytes;
     - the `SensorManager::batteryState` lambda clone growing 0x90 → 0x98;
     - a `SystemClass::powerSource()` `.isra` clone disappearing;
     - `CSWTCH.*` renumbering.
6. **Tests.** Report the full suite as `N/N (sh via zsh, py via python3)`; the baseline is 76/76.
7. **Docs.** `ARCHITECTURE_OVERVIEW.md` no longer names the two deleted stubs, and nothing else in it changed.
8. **The ruling-4 deviation.** Confirm or refute that `profileSelectionReasonLabel` is live, and that removing it would need an addition. This is an observation for the architect.
9. **Budget.** Net `src/` ≤ 0, and gross additions only ruling 1's line.

## Verdict (your final message; it is saved as the verdict file)

- **Header:** one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED; the model and reasoning used; the test interpreter line.
- **A table of checks 1–9.**
- **Findings,** with file:line and severity. Separate observation from inference.
- **Budget versus actual,** as one row.
- **Confirmations:** the tree matches the start, and you deleted only your scratch directory.

Limit: 120 lines.
