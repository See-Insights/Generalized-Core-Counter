AGENT: Copilot · MODEL: claude-opus-5 · REASONING: medium
AUTHORIZATION SCOPE: edit the files the WO's "Permitted files" lists, for the purposes it gives; run `./bump_version.sh v32-RecoveryVisibility "<one-line note>"`; add or update tests under `tests/`; run the host suite and local ARM builds (release, and the failsafe test-mode build). Your scratch directory is **`build-tmp/wo20261002-stage6/`**: keep every temporary file in it and remove **only it** when done / Not authorized: commits, pushes, merges, branch changes, stash, reset, checkout, flashing, device settings, AWS or network access, any edit to `lib/`, `project.properties` or `docs/`, any change beyond this WO, and **deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`**.
**SIZE BUDGET (net `src/` code lines; nonblank, non-comment, including braces, declarations and includes): A ≤ 8, B net negative (its 2-line test-build include fix counted separately), C ≤ 8, D ≤ 10, E ≤ 12; total about 45. Going over any item's budget means STOP and report, not continue.**

# Stage 6 dispatch — WO-2026-10-02-001 (v32-RecoveryVisibility)

**Goal, in plain language:** a device that can't reach the cloud gets back within about 3 hours instead of 18, and every report shows enough about memory and sleep to diagnose the next problem.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-10-02-001-recovery-visibility`, `HEAD` `71f955e` (v31 merged). Do not switch branches. Files under `docs/` are records: do not touch them.
**Binding spec:** `docs/work-orders/WO-2026-10-02-001-recovery-visibility.md`, including "Stage 5 decisions" and the "Ubidots payload rule". Every file:line there was checked against `71f955e`. Where this dispatch and the WO differ, the WO wins; report the difference.

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Work only from the WO. Any correction to the spec, even an obviously right one, is reported as a deviation. If an item can't be done as approved within its budget, stop and report it; don't redesign it. Leave an uncommitted working-tree diff.

## What to implement

Implement items A–E exactly as the WO's "Change" section specifies. The points most likely to go wrong:

- **A (open hours only):**
  - The supervisor acts only while `Clock::openness() == Open`.
  - Its age is `now − max(lastConnection, todayAt(openHour))`, using the existing `localTodayAt()` exported from `DailyBoundary`.
  - Production `STALE` = 3 h. `COOLDOWN`, `JITTER` and the test-mode values are unchanged.
  - Keep `ConnectivityFailsafeTest.cpp`'s prediction logic consistent.
- **B:**
  - Remove the `nextStage == 1` block. With no stage recorded, the first action is stage 2; a persisted 1 still progresses to 2.
  - Remove anything that becomes unused, and report each item.
  - Apply the 2-line test-build include fix.
- **C:** `MODEM_OFF` only when `!Cellular.ready() && !Cellular.isOn()`. Its time counts into `connPhaseCellMs`. Nothing branches on it.
- **D:** `fh` = `System.freeMemory()`. `lfb` = `HAL_Core_Runtime_Info()` with `info.size = sizeof(info)`, reading `largest_free_block_heap`.
- **E:**
  - `cyc` starts at 1 at boot and increments after every return from `System.sleep()` (all four sites in `State_Sleep.cpp`).
  - `slp` increments only when that call succeeded.
  - Both are RAM counters, not retained.
- **D and E payload:** add the four fields to **both** report formats, as **unquoted JSON numbers**. No new key may carry a string.

## Tests (each item against its goal)

Prefer behavioral host tests where an existing harness pattern fits; structural checks where it doesn't. Say which for each.
- **A:**
  - during open hours with no successful connection, the full reset (code 2) fires when the open-hours age reaches 3 h, and not before;
  - **a 06:00 wake after a 22:00 close does not reset**;
  - closed hours never act;
  - a successful connection clears it;
  - stage 3 stays `COOLDOWN` + jitter after stage 2.
  - **Mutations:** restoring 12 h; removing the open-hours base.
- **B:** a structural test that the supervisor has no radio-reset path and that its first action is stage 2.
- **C:** with isOn false and not ready → `MODEM_OFF`; with isOn true → `CELLULAR_ACQUIRE`. A structural check that no decision reads `MODEM_OFF`.
- **D and E:**
  - the serialized payload (both formats) contains `fh`, `lfb`, `cyc`, `slp` as unquoted numbers, with no new string-valued key;
  - the maximum payload stays under 256;
  - `cyc ≥ slp`, and a failed sleep increments `cyc` but not `slp`.

Run each mutation, then restore the file byte-identically. Update an existing test **only** if it pins text this WO changes, and report each one. `tests/publish_with_ack_structural_test.py` must pass unchanged.

## Verification (run all; report results)

1. Full host suite: every `tests/*.sh` with **zsh** (never bash), plus every bare `tests/*.py` with python3, as `N/N`, before and after. The current count is 51/51.
2. Local ARM builds (boron, Device OS 6.4.1, after `make clean-user`, in your scratch directory or with outputs restored):
   - **release:** text/data/bss against v31's 150420 / 1090 / 2204; `strings` shows `v32-RecoveryVisibility`.
   - **failsafe test mode** (`EXTRA_CFLAGS=-DCONNECTIVITY_FAILSAFE_TEST_MODE=1`): it must compile.
3. Linkage: `nm` shows the new code paths in the release ELF (e.g. the `HAL_Core_Runtime_Info` call and the counters).
4. The maximum payload size in each format, before and after.
5. `git diff --numstat` and the net `src/` lines per item against its budget.

## Implementation Report (required, as your final message)

- Files and lines per item, with line counts.
- Tests and mutations, with results.
- Commands and results, with interpreters and sizes.
- Payload sizes.
- Confirmation that you deleted only `build-tmp/wo20261002-stage6/`.
- Deviations (or "none").
- The model and reasoning level actually used.
