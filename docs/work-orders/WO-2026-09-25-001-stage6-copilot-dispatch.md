AGENT: Copilot · MODEL: claude-opus-5 · REASONING: high
AUTHORIZATION SCOPE: edit firmware source, library, and test files in this repository's working tree; run builds and tests; read (only) the fleet-ops repository and public Ubidots documentation / Not authorized: commits, pushes, merges, branch changes, stash, reset, flashing, device settings, AWS access, any edit outside this repository, and any change to persisted struct layouts (SysData, CurrentData, SensorData).

# Stage 6 dispatch — WO-2026-09-25-001, fix B (explicit WITH_ACK) plus delivery counters

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-09-25-001-publish-with-ack` (stacked on the committed WO-2026-09-24-001 branch at `599038e`). Do not switch branches.
**Binding spec:** `docs/work-orders/WO-2026-09-25-001-report-webhook-loss.md`: Stage 5 decisions 1–4 and acceptance criteria 1–8. The Stage 4 report is `docs/work-orders/WO-2026-09-25-001-stage4-codex/REPORT-webhook-loss-investigation.md`; read its sections 1–3 and 7.
**Starting point:** Codex's draft `docs/work-orders/WO-2026-09-25-001-stage4-codex/BENCH-B-explicit-ack.diff`. It applies cleanly to this branch (`git apply --check` passes on `599038e`). Apply it, then build on it. It already includes bench A's tracing and the version string `v24-Pubq-Ack-B`.

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Work only from the WO. Do not expand scope or change the architecture; if the WO cannot be implemented as written, stop and report back. Any correction to the spec, even an obviously right one, is reported as a deviation. Do not commit, push, merge, or release; leave an uncommitted working-tree diff. Temporary artifacts must have visible, descriptive names under a gitignored path, and be removed when you finish (including empty directories).

Do not touch `docs/work-orders/` except to read it. The WO-2026-09-25-002 file there is out of scope.

## What to implement

1. **Fix B (acceptance criteria 1, 2, 5, 8).** Apply the draft. Every queued send goes out with explicit `WITH_ACK` (clearing `NO_ACK`), including events already persisted in the queue by an older build. A queue entry is removed only after its publish's Future succeeds, which with `WITH_ACK` means the cloud acknowledged it. A failed or unacknowledged publish leaves the event queued for retry. Keep the existing 2 s / 1 s / 30 s pacing.

2. **Sleep and teardown with unacknowledged events (criterion 3).** Confirm, with file and line evidence, that the device does not sleep or tear down the cloud connection while an unacknowledged event is still queued, except when the existing cloud-sync gate times out. If the gate times out with events still queued, that must be logged (event count and elapsed time). If the current app code does not meet this, make the smallest change that does. The Stage 4 report warns that `setPausePublishing()` makes the queue report itself sleep-safe with events pending: do not use it.

3. **Delivery counters in the status payload (criterion 4, decision 2).** Add counters for publishes attempted, acknowledged, failed, and retried, plus the count of events still queued at sleep, to the device-status ledger payload (`src/cloud/DeviceStatusPublisher.cpp`). Constraints:
   - The payload is capped at 896 bytes and the last observed size was 839/896. Use compact keys. If the counters cannot fit without dropping or shortening existing fields, **stop and report** with the byte budget. Do not remove existing fields.
   - Do not change `SysData`, `CurrentData`, or `SensorData` layouts. If the counters must persist across hibernate, use retained RAM or another non-layout mechanism, and state exactly what survives a hibernate, a reset, and a power loss. If that isn't possible without a layout change, stop and report.
   - State in the report what "attempted", "acknowledged", "failed", "retried" and "queued at sleep" each count, and where each is incremented.

4. **Awake time (criterion 6).** Make sure the awake time per cycle can be read from a USB capture, using an existing log line if one already gives it (for example `Sleep: td=…`) or one new INFO line if not. Say which.

5. **Duplicates (criterion 7).** Investigate, read-only:
   - The fleet-ops ingest Lambda's deduplication. Source is in `/Users/chipmc/Documents/Maker/AWS/particle-fleet-operations` (read only; make no changes there). Does a resent event with the same payload (same payload timestamp) create a second DynamoDB/S3 record?
   - How Ubidots handles two dots with the same timestamp for the same variable (public Ubidots documentation).
   - Report the findings with evidence. If either cannot tolerate duplicates, **do not change the fleet-ops repository**. Propose the fix in your report, and implement only a firmware-side change if one is needed and within this WO's scope.

## Tests

Add or update host tests so that each of these Stage 7 mutations makes at least one test fail:
- (i) drop `WITH_ACK` (queued sends go out PRIVATE-only)
- (ii) delete the queue entry before the acknowledgment (remove on the publish call returning, or on dispatch)
- (iii) allow sleep with unacknowledged events queued

Also cover the counter increments. Follow the repository's existing test patterns (source-shape checks and host mirrors where the code can't compile on the host).

## Verification (run all; report results)

1. Full host suite: every `tests/*.sh` with **zsh** (shebang or `zsh <script>`, never bash) plus every bare `tests/*.py` with python3. Report `N/N (sh via zsh, py via python3)`. Current baseline: 43/43.
2. Local ARM-toolchain build (boron), README command. Reference: 150576 / 1090 / 2444.
3. Cloud compile: `particle compile boron . --target 6.4.1`. Reference: 151754 / 3530. Remove the downloaded binary.
4. Report the device-status ledger payload size with the counters added (worst case), against the 896-byte cap.

## Implementation Report (required)

- Files changed
- Behavior changed
- Counter definitions, increment sites, and what survives hibernate, reset, and power loss
- Criterion 3 evidence (sleep/teardown gating), and any change made
- How to read awake time per cycle from a capture
- Duplicate findings (ingest Lambda and Ubidots), with evidence, and any proposed fix
- Tests added or updated, and which mutations they catch
- Commands run and results, with interpreters, build sizes, and payload size
- Known limitations
- Deviations from the WO, including any spec correction (or "none")
- The model and reasoning level actually used
