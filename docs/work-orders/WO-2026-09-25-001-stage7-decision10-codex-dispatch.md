AGENT: Codex · MODEL: gpt-6-astra · REASONING: ultra
AUTHORIZATION SCOPE: read the repository; copy it to your scratch folder and mutate the copy for check (c); run the host suite and host reproductions; one `particle compile` cloud build of the working tree (network enabled for the Particle compile service only) / Not authorized: any edit to the repository itself, commits, pushes, stash, reset, checkout, flashing, device settings, AWS access.
**Size budget: none (verification only).**

# Stage 7 (narrow) — WO-2026-09-25-001, Stage 5 decision 10

Per AI_DEVELOPMENT_WORKFLOW.md. Dispatched via `codex exec`. Safety-relevant (sleep and power), so highest reasoning.

**Goal, in plain language:** every report the device makes should reach Ubidots. The design already existed: every queued publish was sent with `WITH_ACK`, so the queue only deleted an event once the cloud confirmed it. That flag was accidentally dropped in `eda6b7e` (2026-02-09). Decision 10 puts it back, and hands a never-acknowledged event's wait to Sleep's existing bounded gate so it can't keep the device awake.

**Repository (READ ONLY):** /Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter, branch `wo/2026-09-25-001-publish-with-ack` at `599038e`, with decision 10 as an uncommitted working-tree diff (`git diff -- src/ tests/` plus untracked `tests/publish_with_ack_structural_test.py`). Staged and untracked files under `docs/work-orders/` are records, out of scope. **Do not modify the repository.** For mutation checks, copy the tree into your scratch folder and mutate the copy.

**Binding spec:** `docs/work-orders/WO-2026-09-25-001-report-webhook-loss.md`, Stage 5 decision 10 only (decisions 1–3 and 5–8 are superseded; their code is archived and not in this tree). Copilot's report: `WO-2026-09-25-001-stage6-decision10-copilot-report.md`. Two test-harness deviations are already accepted. The `CONNECTED`-mode Idle ceiling is a **recorded known limit, out of scope**: do not report it as a finding.

**Fault model:** acknowledgments that never arrive, a connection that fails, a reset at any point, and hibernate. Anything outside that model is filed separately and does not block this review.

## Verify

- **(a)** All six queue publishes carry `WITH_ACK` (report, `status`, `watchdog`, `hibernate_wake`, the diagnostic helper, `pdiag`), and there are no other `PublishQueuePosix::instance().publish(` calls in `src/`.
- **(b)** The Idle handoff: with an event that is never acknowledged, in the intermittent modes (`INTERMITTENT` = 1 and `INTERMITTENT_KEEP_ALIVE` = 3), the device reaches Sleep's bounded gate and sleeps within its existing limit (at most 120 s), raising alert 43. **Demonstrate this with a host reproduction** built from the real source where practical. Include a failed connection, a reset at any point, and a hibernate cycle within the fault model: show that the unacknowledged event is retained (on flash, or re-queued) and retried on the next connection, not deleted.
- **(c)** The structural test (`tests/publish_with_ack_structural_test.py`) catches each of: removing `WITH_ACK` from any one site; restoring `canSleepGate`; adding a new `PublishQueuePosix::instance().publish(` without `WITH_ACK`. Run each mutation on your scratch copy.
- **(d)** The full suite passes: every `tests/*.sh` with **zsh** (shebang or `zsh <script>`, never bash), every bare `tests/*.py` with python3. Report `N/N (sh via zsh, py via python3)`.
- **(e)** `WITH_ACK` is present in the actual cloud-built binary: build the working tree with `particle compile boron . --target 6.4.1 --saveTo <your scratch folder>` **from a copy of the repository root** (not a library example), and confirm the flag value `0x08` combined with PRIVATE `0x01` (i.e. `0x09`) reaches the publish calls, by disassembly or equivalent. Note that cloud builds substitute registry libraries for `lib/`; decision 10 is in `src/`, so it should be unaffected, but confirm on the binary. Record the binary's SHA-256.

## Output

A short verdict, VERIFIED or NOT VERIFIED, with file and line references; a result line for each of (a)–(e); the reproduction results; the binary's SHA-256; and the model and reasoning level actually used. Keep binaries and scratch files outside the repository; remove temporary artifacts except the binary. Do not commit, push, stash, reset, or check out anything.
