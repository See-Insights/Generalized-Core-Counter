AGENT: Copilot · MODEL: claude-opus-5 · REASONING: high
AUTHORIZATION SCOPE: edit firmware source, library, test, and the two named reference docs in this repository's working tree; run builds and tests / Not authorized: commits, pushes, merges, branch changes, stash, reset, flashing, device settings, AWS access, any edit outside this repository, any change to persisted struct layouts (SysData, CurrentData, SensorData), and any change beyond decisions 6 and 7.

# Stage 6 round 3 dispatch — WO-2026-09-25-001, Stage 5 decisions 6 and 7

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-09-25-001-publish-with-ack`. Do not switch branches. The working tree holds rounds 1 and 2 (uncommitted); build on them.
**Binding spec:** `docs/work-orders/WO-2026-09-25-001-report-webhook-loss.md`: Stage 5 decisions 1–8 and acceptance criteria 1–12. The findings this round addresses are in `docs/work-orders/WO-2026-09-25-001-stage7-codex/` (`VERDICT.md`, `REVIEW.md`, `delivery-review.md`, `counters-review.md`, and the `iv-remove-on-failure.patch` mutation).

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Work only from the WO. Any correction to the spec, even an obviously right one, is reported as a deviation. Do not commit, push, merge, or release; leave an uncommitted working-tree diff. Temporary artifacts: visible, descriptive names under a gitignored path, removed when you finish (including empty directories). Do not touch `docs/work-orders/` except to read it.

## Decision 6 (Stage 7 finding P1): bounded delivery wait

Today, a connected device with a queue that never drains (repeated ACK failures) never reaches the sleep gate's timeout: `State_Idle.cpp:233` requires the queue to be drained before entering `SLEEPING_STATE`, and the Idle ceiling (lines 278–310) also requires `queueCanSleep`. Implement:

- **Sleep is allowed when the queue is empty OR (the delivery budget has expired AND no publish is in flight).** Apply this consistently wherever the queue condition gates sleep or disconnect today (the Idle sleep gate, the Idle ceiling, and the `State_Sleep.cpp` cloud-sync gate). Do not simply delete the queue conditions: a publish in flight must never be abandoned by a disconnect.
- **The budget is 90 s by default and configurable.** Define precisely when the budget starts (for example: when the device is cloud-connected with a non-empty queue) and document it in code. For "configurable": use one named constant in the existing policy location (e.g. `ConnectivityPolicy`) unless an existing configuration path can carry it **without** a persisted layout change. If making it runtime-configurable would require a persisted layout change, **do not do it**: use the constant and report it.
- **"No publish in flight"** must come from the queue's actual state (for example a new read-only accessor on `PublishQueuePosix` reporting whether it is waiting on a publish Future), not an inference.
- **Before sleeping with events queued, move RAM-queue events to flash** (`writeQueueToFiles()`), so nothing is lost across hibernate.
- **Add a `sleptWithQueued` counter** to the retained delivery counters and to the `status` event's `"d"` object (use a one-letter key; re-check the event against the 1024-byte limit and update the budget test).
- **Log each budget expiry** at INFO: queued count, elapsed, and whether a publish was in flight.

## Decision 7 (Stage 7 findings P2 and P3)

- **Log the `q` snapshot and `CycleDelivery` before `System.sleep()`**, so a successful HIBERNATE (which resets and never returns) still records them. Keep the ULP path's output unchanged in content.
- **Add an `abandoned` counter, computed at boot from attempts without an outcome**: record the outstanding attempt in retained RAM when it is dispatched, clear it when its result arrives, and at boot count any still-outstanding attempt as abandoned. The invariant is `a = k + f + abandoned` (outside an in-flight attempt, and below saturation). Add it to the `status` event's `"d"` object (one-letter key; re-check the size).
- **Add a test that catches mutation (iv)**: an extra dequeue in `statePublishWait()`'s Future-failure/ACK-timeout branch must make a test fail. The existing structural check only positions the success-branch removal; cover the failure branch directly (the mutation is `docs/work-orders/WO-2026-09-25-001-stage7-codex/iv-remove-on-failure.patch`).
- **Correct `docs/FIELD_MEANINGS_REFERENCE.md`** for `GateFail q`: in the code, `q=1` means the queue is blocking sleep (`queuePending = queueEmpty ? 0 : 1`), and `qn` is the count.

## Tests

Mutations (i)–(iv) from Stage 7 must all make at least one test fail. Add tests for decision 6 (a host reproduction or structural check that a stuck queue leads to sleep once the budget expires with nothing in flight, never while a publish is in flight, and that RAM events are flushed first), and for the new counters and the invariant across a simulated reset during an attempt. Update the `status` event budget test for the new keys.

## Verification (run all; report results)

1. Full host suite: every `tests/*.sh` with **zsh** (shebang or `zsh <script>`, never bash) plus every bare `tests/*.py` with python3. Report `N/N (sh via zsh, py via python3)`. Round-2 result: 48/48.
2. Local ARM-toolchain build (boron), README command. Report text/data/bss against round 2's 152580 / 1110 / 2468.
3. Cloud compile: `particle compile boron . --target 6.4.1`. Report Flash/RAM against round 2's 153782 / 3570. Remove the downloaded binary.
4. The `status` event worst-case size with the new keys, against 1024.
5. The retained block's new size and placement (`nm`), since it grows.

## Implementation Report (required)

Files changed; behavior changed; the budget's start condition, value, and how it is configured; the in-flight accessor; the counter definitions (including `sleptWithQueued` and `abandoned`) and the invariant; tests and the mutations they catch; commands and results with interpreters, sizes, event size, and retained placement; deviations (or "none"); the model and reasoning level actually used.
