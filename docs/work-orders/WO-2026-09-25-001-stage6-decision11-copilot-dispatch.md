AGENT: Copilot · MODEL: claude-opus-5 · REASONING: medium
AUTHORIZATION SCOPE: edit the `src/` lines named below; update existing tests only where they pin the exact text of a line this changes; run the host suite and builds / Not authorized: commits, pushes, merges, branch changes, stash, reset, flashing, device settings, AWS access, any edit to `lib/`, `project.properties`, or `docs/`, and any change beyond decision 11.
**SIZE BUDGET: about 10 lines of `src/` changed. Going over the budget means STOP and report, not continue.**

# Stage 6 dispatch — WO-2026-09-25-001, Stage 5 decision 11 (logging accuracy and the build name)

**Goal, in plain language:** the logs accurately say what the queue is doing.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-09-25-001-publish-with-ack`, `HEAD` `599038e` with decision 10 as an uncommitted working-tree diff (Stage 7 VERIFIED). Build on it; do not switch branches. Files under `docs/work-orders/` are records: do not touch them.
**Binding spec:** `docs/work-orders/WO-2026-09-25-001-report-webhook-loss.md`, Stage 5 decision 11, including the table of what each `q=` counts today.

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Work only from decision 11. Any correction to the spec, even an obviously right one, is reported as a deviation. Do not commit, push, merge, or release; leave an uncommitted working-tree diff. Temporary artifacts: visible, descriptive names under a gitignored path, removed when you finish (including empty directories).

## What to implement (decision 11, nothing else)

`q=` means total events waiting, which is `PublishQueuePosix::instance().getNumEvents()` (the library keeps events in RAM only while there are no flash files, so this is RAM plus flash; see the WO). **Do not change `lib/`.**

1. **`GateBlock`, `Gate`, `GateFail`** (`src/state/State_Sleep.cpp`, around lines 559, 574, 595): their `q=` is currently the 0/1 flag `queuePending`. Make it the count (the `queueDepth` already computed there, from `getNumEvents()`). Keep `queuePending` for any non-log logic that uses it.
2. **`Report:`** (`src/Generalized-Core-Counter.cpp`, both variants around lines 2129 and 2136): rename `q=` to `ok=`. The value (`queued ? 1 : 0`, `publish()`'s return) is unchanged.
3. **The Idle message** (`src/state/State_Idle.cpp:250`): replace `Low-power idle: queue drained and no updates pending - entering SLEEPING_STATE` with `Low-power idle: no updates pending - handing queue (q=%u) to sleep gate`, passing `getNumEvents()` (the `pending` value already computed just above it may be reused). Leave the offline message at `:247` as it is.
4. **Version string** (`src/Version.cpp`): `FIRMWARE_VERSION` becomes `"v25-WithAck"`, with a matching one-line `FIRMWARE_RELEASE_NOTES`. Do not change the numeric product version in `FirmwareVersion.h`.

The lines that already report `getNumEvents()` (`LoopStage`, `BootWDT`, `ConnDiag`, `ConnSummary`, `Connect: ok`, `CYCLE end`) need no change. `GateRelease` and `LedgerSleepState` have no `q=`; do not add one.

## Tests

Update an existing test only if it pins the exact text of a line changed here, and report each one. `tests/publish_with_ack_structural_test.py` must still pass unchanged.

## Verification (run all; report results)

1. Full host suite: every `tests/*.sh` with **zsh** (shebang or `zsh <script>`, never bash) plus every bare `tests/*.py` with python3. Report `N/N (sh via zsh, py via python3)`. Current: 44/44.
2. Local ARM build (boron), README command: text/data/bss against decision 10's 150624 / 1090 / 2444.
3. **The exact line count:** `git diff --numstat -- src/` for this round only (the change on top of decision 10), against the ~10-line budget.

## Implementation Report (required)

Files and lines changed; the exact `src/` line count for this round; the final text of each changed log line; tests updated and why; commands and results with interpreters and sizes; deviations (or "none"); the model and reasoning level actually used.
