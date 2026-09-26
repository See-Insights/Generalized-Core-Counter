AGENT: Copilot · MODEL: claude-opus-5 · REASONING: high
AUTHORIZATION SCOPE: edit firmware source and test files in this repository's working tree; run builds and tests / Not authorized: commits, pushes, merges, branch changes, stash, reset, flashing, device settings, AWS access, any edit outside this repository, and any change beyond the item below.

# Stage 6 round 2 dispatch — WO-2026-09-25-001, Stage 5 decision 5

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-09-25-001-publish-with-ack`. Do not switch branches. The working tree holds your round-1 implementation (uncommitted); build on it.
**Binding spec:** `docs/work-orders/WO-2026-09-25-001-report-webhook-loss.md`, Stage 5 decisions 1–5 and acceptance criteria 1–10. Your round-1 report is `docs/work-orders/WO-2026-09-25-001-stage6-copilot-report.md`.

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Work only from the WO. Any correction to the spec, even an obviously right one, is reported as a deviation. Do not commit, push, merge, or release; leave an uncommitted working-tree diff. Temporary artifacts: visible, descriptive names under a gitignored path, removed when you finish (including empty directories). Do not touch `docs/work-orders/` except to read it.

## What to implement: decision 5 only

**Move the delivery-counter block (`"d": {a, k, f, r, q}`) out of the device-status ledger payload (`src/cloud/DeviceStatusPublisher.cpp`) and into the `status` event only** (the event built and published through the queue in `src/Generalized-Core-Counter.cpp`, currently `PublishQueuePosix::instance().publish("status", status, PRIVATE)`).

- The device-status ledger payload must be byte-for-byte the same format as before WO-2026-09-25-001: no `"d"` object.
- **Keep the overflow guard** in `DeviceStatusPublisher.cpp` exactly as it is (decision 5).
- Keep everything else from round 1 unchanged: `WITH_ACK`, the hooks, the counters module, `CycleDelivery:`, `GateFail:`, tracing, `project.properties`, version string.
- **Size check (stop condition):** measure the `status` event's size with the counters added (observed and worst case) against both limits: the Particle event data limit for Device OS 6.4.1, and PublishQueuePosixRK's own event-size ceiling (a comment in `src/power/PowerDiagnostics.cpp` cites "~622-byte event ceiling"; confirm the real figure from the library source). If the counters don't fit in the worst case, **stop and report** with the byte budget. Do not shorten or remove existing `status` fields.
- Update the tests: the payload-budget and counter-wiring tests must now assert the counters are in the `status` event and **absent** from the ledger payload, and a mutation that puts them back in the ledger must fail a test. Update `docs/contracts/ledger-contracts.md` and `docs/FIELD_MEANINGS_REFERENCE.md` to match.

No other changes.

## Verification (run all; report results)

1. Full host suite: every `tests/*.sh` with **zsh** (shebang or `zsh <script>`, never bash) plus every bare `tests/*.py` with python3. Report `N/N (sh via zsh, py via python3)`. Round-1 result: 47/47.
2. Local ARM-toolchain build (boron), README command. Report text/data/bss against round 1's 152628 / 1110 / 2468.
3. Cloud compile: `particle compile boron . --target 6.4.1`. Report Flash/RAM against round 1's 153830 / 3570. Remove the downloaded binary.
4. The `status` event size (observed and worst case) against both limits, and confirmation the ledger payload no longer contains `"d"`.

## Implementation Report (required)

Files changed; behavior changed; the size check; tests changed and which mutations they catch; commands and results with interpreters and sizes; deviations (or "none"); the model and reasoning level actually used.
