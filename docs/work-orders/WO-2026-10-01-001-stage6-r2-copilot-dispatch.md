AGENT: Copilot · MODEL: claude-opus-5 · REASONING: medium
AUTHORIZATION SCOPE: item A only. Edit `src/Generalized-Core-Counter.cpp`, `src/state/State_Connect.cpp` and `src/state/State_Sleep.cpp` (item A's code only), `src/ThrashGuard.cpp` and `src/power/ConnectivityPolicy.h` only if item A requires it; update item A's tests (`tests/firmware_update_dwell_test.sh`, `tests/firmware_update_wiring_structural_test.py`) or add item A tests; run the host suite and a local ARM build. Your scratch directory is **`build-tmp/wo20261001-stage6-r2/`**: create it, keep all temporary files in it, and remove **only it** when done / Not authorized: commits, pushes, merges, branch changes, stash, reset, checkout, flashing, device settings, AWS or network access, any edit to `lib/`, `project.properties` or `docs/`, any change to items B, C or D or their tests, re-running `bump_version.sh`, and **deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`**.
**SIZE BUDGET: item A about 30 net `src/` code lines measured against v30 (`9e10778`), likely fewer. Count as Codex did in round 1: nonblank, non-comment physical lines, including braces, declarations and includes. Going over means STOP and report, not continue. This is round 2 of 2 (two-round rule): there's no round 3.**

# Stage 6 round 2 dispatch — WO-2026-10-01-001 item A (Particle's reference pattern)

**Goal, in plain language:** a device doesn't go to sleep in the middle of a firmware download that's still making progress.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-10-01-001-connectivity-fixes`, base `9e10778` (v30). The working tree holds round 1's uncommitted diff. Items B, C, D and the version bump in it are **verified and frozen**: leave their lines byte-identical. Files under `docs/` are records: do not touch them.
**Binding spec:** `docs/work-orders/WO-2026-10-01-001-connectivity-fixes.md`, section "A — round 2". Round 1's Stage 7 verdict, for context: `docs/work-orders/WO-2026-10-01-001-stage7-verdict.md`.
**Reference design:** https://docs.particle.io/firmware/low-power/wake-publish-sleep-cellular/. A saved copy is at `build-tmp/connectivity-archive/particle-docs/firmware_low-power_wake-publish-sleep-cellular.txt`, lines 216–413: `firmwareUpdateInProgress`, `STATE_SLEEP`'s first check, `STATE_FIRMWARE_UPDATE`, `firmwareUpdateHandler`. Read-only. Follow its flag-and-check pattern, with our three deliberate differences listed below.

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Work only from the WO. Any correction to the spec, even an obviously right one, is reported as a deviation. Leave an uncommitted working-tree diff. If the plan can't be implemented as approved within the budget, stop and report.

## What to implement (item A, replacing round 1's item A code)

1. **Handler, record only** (`Generalized-Core-Counter.cpp`, keeping the existing `System.on(firmware_update, firmwareUpdateHandler)` registration):
   - `firmware_update_begin` (0) sets `firmwareUpdateInProgress = true`;
   - `firmware_update_complete` (1) and `firmware_update_failed` (−1) clear it;
   - begin and progress (2) record `millis()` as the last activity (**deliberate difference 1**).

   Use `volatile` file-scope state, shared with the state files the way round 1 did (`extern`).
2. **Remove from round 1:**
   - the loop-level begin block (`if (firmwareUpdateBeginPending && …) transitionTo(FIRMWARE_UPDATE_STATE, "firmware update begin")`) and `firmwareUpdateBeginPending`;
   - `firmwareUpdateLastEvent`;
   - the complete/failed → Idle exits in `handleFirmwareUpdateState()`.
3. **Check before sleeping** (`State_Sleep.cpp`, `handleSleepingState()`): if `firmwareUpdateInProgress`, `transitionTo(FIRMWARE_UPDATE_STATE, "firmware update in progress")` and `return`.
   - **Placement:** before the first teardown request (cloud disconnect / radio off at `State_Sleep.cpp:755`, `:773`, `:903`), on every pass while no disconnect has been requested (`!disconnectRequested`), mirroring the reference's check at the top of `STATE_SLEEP`.
   - **Not** at the night-sleep commitment (`:973`) or the sleep calls: those run after teardown, when the download would already be lost.
   - If leaving the handler mid-gate needs any existing per-cycle static reset so that a later return to sleep starts its gate cleanly, use the handler's existing reset pattern and report it.
4. **`handleFirmwareUpdateState()`** (`State_Connect.cpp`):
   - exits to `SLEEPING_STATE` when `firmwareUpdateInProgress` is false;
   - exits to `SLEEPING_STATE` after 5 minutes (`FIRMWARE_UPDATE_MAX_MS`) with no new activity. The timer starts at entry and restarts on each recorded progress (**deliberate difference 2**);
   - keeps the button override, the configuration-load block, the webhook-ack hold (3 lines), and ThrashGuard `markProgress` on each new recorded activity (**deliberate difference 3**, with the 330 s update-state timeout unchanged).

   Nothing else exits. There is no special handling for `complete`: it clears the flag, and Device OS resets the device.
5. Leave unchanged: entry on `System.updatesPending()` at `State_Connect.cpp:651–652`. With the flag clear, that entry now leaves straight for `SLEEPING_STATE`.

## Tests (item A only; B, C, D tests untouched)

Update `tests/firmware_update_dwell_test.sh` (behavioral, compiled extraction) and `tests/firmware_update_wiring_structural_test.py` to the round-2 design.

They must cover:
- begin → the flag is set; complete/failed → cleared; progress records activity; the handler makes no transitions;
- in `SLEEPING_STATE` with the flag set, the handler goes to `FIRMWARE_UPDATE_STATE` **before** any teardown request (assert that no disconnect or radio-off call is made in that pass);
- in the update state: progress keeps it there past 5 minutes in total; 5 minutes without progress → `SLEEPING_STATE`; flag cleared → `SLEEPING_STATE`; the button still exits;
- **Stage 7 round 1's three reproductions, which must now pass:**
  1. a begin delivered between loop passes while the device is in the sleep gate never reaches teardown;
  2. no re-entry into the update state after the download ends;
  3. a stale complete/failed doesn't cut short a later download's stay.

**Mutations** (each must fail a test; restore byte-identically):
- moving the sleep-handler check after the teardown request;
- removing it;
- a fixed (non-restarting) 5-minute timer;
- removing `markProgress`;
- an update-state ThrashGuard timeout below 300.

`tests/publish_with_ack_structural_test.py` must pass unchanged. B, C and D's tests must pass unchanged.

## Verification (run all; report results)

1. Full host suite: every `tests/*.sh` with **zsh** (never bash), plus every bare `tests/*.py` with python3, as `N/N`. The round 1 result was 51/51.
2. Local ARM build (boron), release, after `make clean-user`, **inside your scratch directory or with outputs restored**. Report text/data/bss against v30's 150164 / 1090 / 2196 and round 1's 150444 / 1090 / 2212. `strings` finds `v31-ConnectivityFixes` and `firmware update in progress`.
3. Linkage: `nm` shows `firmwareUpdateHandler`, registered via `System.on`, and the sleep-handler check is in the image.
4. Item A's net `src/` lines against v30, by the counting rule above, against about 30. Also confirm B, C and D's lines are byte-identical to round 1.

## Implementation Report (required, as your final message)

- Files and lines changed.
- Item A's line count.
- Tests changed, and each mutation and its result.
- Commands and results, with interpreters and sizes.
- Confirmation that you deleted only `build-tmp/wo20261001-stage6-r2/`.
- Deviations (or "none").
- The model and reasoning level actually used.
