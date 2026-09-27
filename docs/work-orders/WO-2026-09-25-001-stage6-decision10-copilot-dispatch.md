AGENT: Copilot · MODEL: claude-opus-5 · REASONING: medium
AUTHORIZATION SCOPE: edit the `src/` lines named below and add one test file; run the host suite and builds / Not authorized: commits, pushes, merges, branch changes, stash, reset, flashing, device settings, AWS access, any edit to `lib/`, `project.properties`, or `docs/`, and any change beyond decision 10.
**SIZE BUDGET: about 15 lines of `src/` changed (added + removed), plus one test file. Going over the budget means STOP and report, not continue.**

# Stage 6 dispatch — WO-2026-09-25-001, Stage 5 decision 10 (restoration)

**Goal, in plain language:** every report the device makes should reach Ubidots. The design already existed: every queued publish was sent with `WITH_ACK`, so the queue only deleted an event once the cloud confirmed it. That flag was accidentally dropped in `eda6b7e` (2026-02-09). Put it back, and make sure a never-acknowledged event can't keep the device awake. Restore; don't invent.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-09-25-001-publish-with-ack` at `599038e`. Do not switch branches. The staged and untracked files under `docs/work-orders/` are work-order records: do not touch them.
**Binding spec:** `docs/work-orders/WO-2026-09-25-001-report-webhook-loss.md`, Stage 5 decision 10 only. Decisions 1–3 and 5–8 are superseded; their code is archived and must **not** be reused.

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Work only from decision 10. Any correction to the spec, even an obviously right one, is reported as a deviation. Do not commit, push, merge, or release; leave an uncommitted working-tree diff. Temporary artifacts: visible, descriptive names under a gitignored path, removed when you finish (including empty directories).

## What to implement (decision 10, nothing else)

1. **`PRIVATE | WITH_ACK` on every queue publish.** The six sites, all `PublishQueuePosix::instance().publish(` in `src/`:
   - report: `src/Generalized-Core-Counter.cpp:2061`
   - `status`: `:2283`
   - `watchdog`: `:2323`
   - `hibernate_wake`: `:2358`
   - diagnostic helper `publishDiagnosticSafe()`: `:2471` (it passes its `flags` argument; make the queue call add `WITH_ACK`, as v3.12 did with `flags | WITH_ACK`)
   - `pdiag`: `src/power/PowerDiagnostics.cpp:439`
   Confirm with a search that there are no others. Do not change `lib/`.
2. **The Idle handoff.** `src/state/State_Idle.cpp:238` becomes `if (!updatesPending) {`, and the now-unused gate lines (`canSleepGate` at 233–236, and the comment above them if it no longer applies) are removed. This hands the queue wait to the existing bounded gate in `SLEEPING_STATE` (`State_Sleep.cpp`), which waits 30–120 s, then raises alert 43 and disconnects. Do not change `State_Sleep.cpp`.
3. **One structural test** (a new `tests/*.py`, following the repository's existing structural-test pattern): every `PublishQueuePosix::instance().publish(` call in `src/` includes `WITH_ACK`, and `handleIdleState()` no longer gates the low-power sleep entry on the queue. A mutation that removes `WITH_ACK` from any one site must fail the test.

If you find that the Idle safety ceiling (around `State_Idle.cpp:280` and `:303`) can still keep the device awake with a never-acknowledged event, **report it as a finding; do not change it**. It is outside decision 10.

## Verification (run all; report results)

1. Full host suite: every `tests/*.sh` with **zsh** (shebang or `zsh <script>`, never bash) plus every bare `tests/*.py` with python3. Report `N/N (sh via zsh, py via python3)`.
2. Local ARM-toolchain build (boron), README command: text/data/bss against `599038e`'s 150576 / 1090 / 2444.
3. Cloud compile: `particle compile boron . --target 6.4.1`: Flash/RAM against 151754 / 3530. Remove the downloaded binary.
4. **The exact line count:** `git diff --numstat -- src/` (added/removed per file and total), plus the test file's line count.

## Implementation Report (required)

Files changed; the exact `src/` line count against the ~15-line budget; the six sites with their final flags; any further publish sites found; the Idle change; the test and the mutation it catches; commands and results with interpreters and sizes; any Idle-ceiling finding; deviations (or "none"); the model and reasoning level actually used.
