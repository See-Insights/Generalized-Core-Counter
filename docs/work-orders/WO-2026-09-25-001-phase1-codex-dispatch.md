AGENT: Codex · MODEL: gpt-6-astra · REASONING: high
AUTHORIZATION SCOPE: read-only code investigation, plus one `particle compile` cloud build written to your scratch folder / Not authorized: any edit to the repository, commits, pushes, stash, reset, checkout, flashing, device settings, AWS access.
Size budget: none (read-only). Answer each question in at most 5 lines, with file:line references.

# Phase 1 investigation — report webhook recovery (WO-2026-09-25-001)

Per AI_DEVELOPMENT_WORKFLOW.md. Dispatched via `codex exec`.

**Goal, in plain language:** every report the device makes should reach Ubidots, using the design the user already built: send with WITH_ACK, wait for Ubidots' reply on a specific response topic, and escalate through the error state if the reply doesn't come. We restore that design; we don't invent new mechanisms.

**Repository (READ ONLY):** /Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter, branch `wo/2026-09-25-001-publish-with-ack` at `599038e` (clean). "Today's code" means this tree. History: `git log`/`git show` are fine; do not check out or modify anything. Your working directory is a scratch folder: write all output there.

**Known history (verify, don't assume):** `WITH_ACK` was on the report publish from v3.07 (`4196c17`), on `status` from v3.09 (`8aa2643`), and on the diagnostic helper from v3.12 (`15db55c`); all three were removed in `eda6b7e` (2026-02-09, "v3.24 - Occupancy Mode", no stated reason). v3.23 is `eda6b7e^`.

## Questions (at most 5 lines each, with file:line references)

1. **Sleep with WITH_ACK.** If every queue publish uses `PRIVATE | WITH_ACK` and a report is never acknowledged, can `src/state/State_Idle.cpp:233` (wait for an empty queue) or the Idle safety ceiling keep the device awake indefinitely? Was that condition present in v3.23 (`eda6b7e^`)? If it can hang, what is the smallest change that hands off to the existing bounded gate in `src/state/State_Sleep.cpp`, and how many lines is it?
2. **Alerts.** Does today's `Alert_Handling::alertResolution()` (or its current equivalent) still have the overwrite bugs in cases 12, 13 and 40, where the conditional value is overwritten by the next line? Does alert 40 still route to the error state? Is the webhook supervision block in `src/state/State_Report.cpp` compensating for case 40 never escalating?
3. **Response topic.** What does today's code subscribe to for webhook replies, compared with the original `responseTopic` (find it in history)? What exact topic string should be restored?
4. **Library provenance.** For every library that is vendored in `lib/` and listed in `project.properties`, which copy does a cloud build actually use? Specifically: are PR #41's AB1805_RK constant fixes (commit `ab2e0fe`) present in the binary a cloud build produces? Check a real cloud-built binary (`particle compile boron . --target 6.4.1 --saveTo <your scratch folder>`), not the source: compare the relevant code (disassembly or constants) against the vendored and the registry versions.
5. **Every queue publish.** List every `PublishQueuePosix::instance().publish(` in `src/` with its flags (report, status, diagnostic, Ubidots_Alert_Hook, and any others).

## Output

Numbered answers 1–5, each at most 5 lines with file:line references; then the model and reasoning level actually used. Do not modify the repository; remove temporary artifacts from your scratch folder except the cloud-built binary and its SHA-256.
