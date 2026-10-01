AGENT: Copilot · MODEL: claude-opus-5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/Generalized-Core-Counter.cpp`, `src/state/State_Connect.cpp`, `src/ThrashGuard.cpp`, `src/state/State_Sleep.cpp` and `src/state/State_Error.cpp` (item C reset sites only), `src/power/ConnectivityPolicy.h`; create one small header for the reset-cause enum (e.g. `src/ResetCause.h`); run `./bump_version.sh` for the version; add or update tests under `tests/`; run the host suite and a local ARM build / Not authorized: commits, pushes, merges, branch changes, stash, reset, checkout, flashing, device settings, AWS or network access, any edit to `lib/`, `project.properties` or `docs/`, and any change beyond this WO.
**SIZE BUDGET: at most about 65 lines of `src/` in total: A ≤ 20, B ≤ 5, C ≤ 25 (expected about 10), D ≤ 12 (expected about 3). Tests are separate. Going over any item's budget means STOP and report, not continue.**

# Stage 6 dispatch — WO-2026-10-01-001 (v31, connectivity and diagnosability fixes)

**Goal, in plain language:** a device doesn't go to sleep in the middle of a firmware download that's still making progress; its long connection attempt comes round when it should, even if the previous attempts failed; after any reset the firmware issues itself, the next status names the code that issued it; and the signal reads "not available" when Device OS has no reading, instead of 0/0.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-10-01-001-connectivity-fixes`, `HEAD` `9e10778` (v30-LedOffAtNight). Do not switch branches. Files under `docs/` are records: do not touch them (several untracked `docs/work-orders/2026-10-01-*` and `WO-2026-10-01-001-*` files are present).
**Binding spec:** `docs/work-orders/WO-2026-10-01-001-connectivity-fixes.md`, including its "Stage 5 decisions" section. Where this dispatch and the WO differ, the WO wins. Report the difference.

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Work only from the WO. Any correction to the spec, even an obviously right one, is reported as a deviation. Leave an uncommitted working-tree diff. Temporary artifacts go under `build-tmp/` with descriptive names, and are removed when you finish. If the plan can't be implemented as approved, stop and report; don't improvise a new design.

## What to implement (nothing else)

### A. Don't sleep while a firmware download is making progress (≤ 20 lines)

1. Register a `firmware_update` system-event handler next to `System.on(out_of_memory, …)` (`Generalized-Core-Counter.cpp:887`). The handler **only records**:
   - that a `begin` (param 0) arrived (a flag);
   - the last event (begin 0, progress 2, complete 1, failed −1);
   - `millis()` of the latest begin or progress.

   No transitions and no other work in the handler. Use `volatile` file-scope state, and make it readable from `State_Connect.cpp` the way the file already shares state (e.g. via `session` or an existing shared header). Values: Device OS 6.4.1 `system/inc/system_event.h:52, 77–80`.
2. **Loop-level transition:** with the other loop-level transitions (after the user-switch block at `Generalized-Core-Counter.cpp:1684–1689`), when a `begin` has been recorded and `state != FIRMWARE_UPDATE_STATE`, clear the flag and `transitionTo(FIRMWARE_UPDATE_STATE, "firmware update begin")`.
3. **`handleFirmwareUpdateState()`** (`State_Connect.cpp:722–777`). Keep the entry block and the "Connected … loading configuration" block. Exits are **only**:
   - recorded `complete` → `IDLE_STATE`;
   - recorded `failed` → `IDLE_STATE`;
   - no `progress` event for 5 minutes → `SLEEPING_STATE`. The timer starts at state entry and restarts on each recorded progress (use the later of the entry time and the last progress `millis()`). Reuse `FIRMWARE_UPDATE_MAX_MS` (`ConnectivityPolicy.h:94`, 5 min) as this no-progress window, with its comment updated; or rename it if that's clearer. Report which;
   - the existing button override (`:760–765`), unchanged.

   **Remove** the `!System.updatesPending()` exit (`:752–757`) and the fixed absolute cap (`:767–774`). Clear the recorded complete/failed state when acting on it.
4. **ThrashGuard (Stage 5 decision 1):**
   - in `handleFirmwareUpdateState()`, call `thrashGuard.markProgress(...)` (use the existing `markProgress` signature) whenever a new progress event has been recorded since the last loop pass;
   - raise `ThrashGuard::timeoutForStateSec(FIRMWARE_UPDATE_STATE)` (`ThrashGuard.cpp:66–67`) from 180 to **330** s.
5. **Webhook-ack timeout:** pause it while in `FIRMWARE_UPDATE_STATE` **only if that takes ≤ 3 lines** (e.g. hold `session.webhookAwaitStartMs` at `millis()` while in the state). If it would take more, leave it and say so.
6. Leave untouched: entry on `System.updatesPending()` at `State_Connect.cpp:651–652`, the sleep gate (`State_Sleep.cpp:490`), Idle's checks (`State_Idle.cpp:225`, `:269`), the out-of-memory transition (`Generalized-Core-Counter.cpp:1680`), the user switch (`:1688`), and ThrashGuard's tiers.

### B. Count failed attempts (≤ 5 lines)

At the connection-timeout path (`State_Connect.cpp:704–718`, before `transitionTo(SLEEPING_STATE, "connect-timeout")`), increment `connectionAttemptCounter` with the same `< DEEP_ATTEMPT_COUNTER_THRESHOLD` guard as the success increment (`:554–556`). Change nothing else: the success increment, the reset at `:276–281`, the thresholds, and the "above 50%" rule all stay.

### C. Name the code behind each firmware-issued reset (≤ 25 lines, expected about 10; Stage 5 decision 2)

Add one enum of distinct, **non-zero** reset-cause codes in a small header. Change each of the six `System.reset()` calls in `src/` to `System.reset(<its code>)` (Device OS 6.4.1 `System.reset(uint32_t data)`):
- `Generalized-Core-Counter.cpp:1813` (appWatchdogHandler, `#else` branch);
- `Generalized-Core-Counter.cpp:2632` (failsafe stage 2);
- `ThrashGuard.cpp:151` (tier 3);
- `State_Sleep.cpp:1171`;
- `State_Sleep.cpp:1469`;
- `State_Error.cpp:161`.

Don't touch `ab1805.deepPowerDown()` calls or `lib/`. The startup status already publishes `resetReasonData` (`Generalized-Core-Counter.cpp:2129`, `:2189`, `:2218`): don't change the payload.

### D. Signal "not available" instead of 0/0 (≤ 12 lines, expected about 3)

In `sampleConnectionSignal()` (`State_Connect.cpp:63–76`), set `valid` only when Device OS's percentages are valid (strength ≥ 0 and quality ≥ 0). Otherwise leave −1 and `valid = false`. Apply the same rule to the WiFi branch. The `sig=na` log branches (`:469`, `:519`, `:693`) already exist: don't change any log line.

### Version

Run `./bump_version.sh v31-ConnectivityFixes "<one-line release note>"`. It must give `FIRMWARE_VERSION` `"v31-ConnectivityFixes"` and `FIRMWARE_PRODUCT_VERSION` 31. Report the files it changed.

## Tests (each item against its own goal)

Prefer behavioral host tests where an existing harness pattern in `tests/` fits; structural (source-shape) checks are acceptable where a behavioral harness would need new mechanism. Say which you used for each.

- **A:**
  - `begin` enters `FIRMWARE_UPDATE_STATE` (from Sleep, Idle, Connecting).
  - Progress events keep it there past 5 minutes in total.
  - 5 minutes without progress exits to Sleep.
  - `complete` and `failed` exit to Idle.
  - No sleep transition happens while in the state.
  - The handler makes no `transitionTo` calls.
  - **Mutations** (each must fail a test): restoring the fixed cap; restoring the `updatesPending()` exit; removing the progress `markProgress`; setting the update-state ThrashGuard timeout below 300.
- **B:**
  - A failed attempt increments the counter.
  - After 3 failures at charge ≤ 50%, the next attempt uses the deep budget.
  - **Mutation:** removing the failure increment fails a test.
- **C:**
  - A structural test: every `System.reset(` in `src/` passes a reset-cause code, and the codes are distinct and non-zero. Adding a bare `System.reset()` or reusing a code must fail it.
  - The startup status payload includes `resetReasonData`.
- **D:**
  - With an invalid reading, `valid` is false and −1 is kept.
  - **Mutation:** restoring the unconditional `valid = true` fails a test.

Run each mutation, then restore the file byte-identically by rewriting it in place. Update any other test **only** if it pins text this WO changes, and report each one. `tests/publish_with_ack_structural_test.py` must pass unchanged. If anything else fails, stop and report.

## Verification (run all; report results)

1. Full host suite: every `tests/*.sh` with **zsh** (never bash), plus every bare `tests/*.py` with python3. Report the baseline before your changes and the result after, as `N/N (sh via zsh, py via python3)`.
2. Local ARM build (boron), release, after `make clean-user`. Report text/data/bss against v30's 150164 / 1090 / 2196. `strings` finds `v31-ConnectivityFixes` and `firmware update begin`.
3. Linkage: on the symbolised ELF, `nm` shows the `firmware_update` handler, and it has a production call site (the `System.on` registration).
4. `git diff --numstat`, and the `src/` line count **per item (A/B/C/D)** against its budget.

## Implementation Report (required, as your final message)

- Files and lines changed per item.
- Tests added or changed, and which were behavioral or structural, and why.
- Each mutation and its result.
- Commands and results, with interpreters and sizes.
- Deviations (or "none").
- The model and reasoning level actually used.
