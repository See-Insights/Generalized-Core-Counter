AGENT: Codex · MODEL: gpt-6-astra · REASONING: ultra
AUTHORIZATION SCOPE: review the uncommitted diff; temporarily mutate source/library/test files for the mutation checks, restoring each byte-identically; run host tests, the local ARM build, and `particle compile` (network enabled for the Particle compile service only); inspect built binaries / Not authorized: lasting edits, commits, pushes, stash, reset, checkout, flashing, device settings, AWS access.

# Stage 7 — WO-2026-09-25-001 (full review)

Per AI_DEVELOPMENT_WORKFLOW.md. Dispatched via `codex exec` (not `codex review --uncommitted`, which rejects custom instructions in CLI 0.154.0).

**Review target:** the uncommitted working-tree diff on `wo/2026-09-25-001-publish-with-ack`, whose base is the committed WO-2026-09-24-001 branch at `599038e` (`git diff 599038e` plus untracked files under `src/` and `tests/`).

**Binding spec:** `docs/work-orders/WO-2026-09-25-001-report-webhook-loss.md`: Stage 5 decisions 1–5 and acceptance criteria 1–10. Context: the Stage 4 report `WO-2026-09-25-001-stage4-codex/REPORT-webhook-loss-investigation.md`, the Stage 6 dispatches, and Copilot's reports `WO-2026-09-25-001-stage6-copilot-report.md` (round 1) and `WO-2026-09-25-001-stage6-round2-copilot-report.md` (round 2). Decision 5 accepted three round-1 deviations: the `project.properties` dependency removal, the overflow guard, and the application-thread hooks.

Out of scope, do not review or modify: `docs/work-orders/` (read only), `AI_DEVELOPMENT_WORKFLOW.md`, WO-2026-09-25-002 and WO-2026-09-25-003. File anything outside scope separately; it does not block this review.

Shell tests are zsh: run via shebang or `zsh <script>`, never bash. Report results as N/N (sh via zsh, py via python3).

## Verify

1. **Criteria 1–2:** every queued send carries explicit `WITH_ACK` (with `NO_ACK` cleared), including events persisted by an older build; a queue entry is removed only after the publish Future succeeds (a cloud ACK); a failed or unacknowledged publish stays queued for retry. Quote the exact lines in `lib/PublishQueuePosixRK` and `lib/BackgroundPublishRK`, and tie them to Device OS 6.4.1 `communication/src/publisher.cpp` (`~/.particle/toolchains/deviceOS/6.4.1`).
2. **Criterion 3 and the bounded wait:** the device does not sleep or tear down the cloud connection while an unacknowledged event is queued, except when the existing cloud-sync gate times out. **On that timeout the device sleeps and nothing is removed from the queue**, and the timeout is logged (event count, elapsed). Trace the path through `State_Sleep.cpp`, `State_Idle.cpp`, and the queue's `canSleep`/`getNumEvents()`, and what happens to RAM-queued events at disconnect (`writeQueueToFiles()` on `cloud_status_disconnecting`).
3. **Criterion 4 (decision 5):** the counters are in both variants of the `status` event; the device-status ledger payload format is unchanged from `599038e` (no `"d"`); the overflow guard is present. Check the counter definitions and increment sites for correctness (`a == k + f` outside an in-flight attempt; `r` attribution; `q` snapshot), and the event size against the 1024-byte `MAX_EVENT_DATA_LENGTH`.
4. **Criteria 5, 6, 8:** the tracing is present; awake time per cycle is readable from `CycleDelivery: awake=`; the version string is `v24-Pubq-Ack-B`.
5. **Criterion 7:** Copilot's duplicate analysis (fleet-ops ingest keys; Ubidots undocumented). Decision 5 settles the Ubidots side on the bench; confirm only that the analysis of the firmware side is sound.
6. **The `project.properties` change:** confirm the vendored library is what both the local and the cloud builds compile. Check that the cloud binary contains the `WITH_ACK` normalization, not the registry 0.0.7 code.
7. **Criterion 9 (binary verification):** build both candidate binaries: the local ARM build (README command) and `particle compile boron . --target 6.4.1`. For each, confirm by disassembly or strings/symbols that the `WITH_ACK` dispatch path and the two new hooks (`withPublishAttemptUserCallback` / `withPublishResultUserCallback` or their call sites) are present. Record each binary's SHA-256 and size, and keep both binaries in your scratch output (outside the repository), so Chip flashes a binary whose hash you verified.
8. **Criterion 10:** confirm the retained counters' placement from the linker map (`.map`) of the local build: the `retainedPublishDelivery` symbol, its address and size, and that it lies in the retained/backup section. Quote the map lines.

## Mutations (each must make at least one test fail; restore byte-identically after each)

- (i) Drop `WITH_ACK`: queued sends go out with the stored flags only.
- (ii) Delete the queue entry before the ACK: remove on the publish call returning, or on dispatch.
- (iii) Allow sleep with unacknowledged events queued: remove the queue condition from the sleep gate.
- (iv) On an ack-wait timeout, the event is removed from the queue: make the Future-failure/timeout path dequeue the event.

Report any surviving mutation as a finding.

## Builds

- Local Boron ARM build: Copilot's round-2 reference is 152580 / 1110 / 2468.
- Cloud compile: reference 153782 / 3570.

## Output

VERIFIED, VERIFIED WITH CONCERNS, or NOT VERIFIED, with file and line evidence; a criterion table (1–10); the mutation table; the bounded-wait finding; both binaries' SHA-256; the suite result with interpreters; and the model and reasoning level actually used. Write binaries and scratch files only outside the repository. Restore every mutated file byte-identically and remove temporary artifacts before finishing. Do not commit, push, stash, reset, or check out anything.
