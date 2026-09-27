AGENT: Codex · MODEL: gpt-6-astra · REASONING: ultra
AUTHORIZATION SCOPE: review the uncommitted diff; temporarily mutate source/library/test files for the mutation checks, restoring each byte-identically; run host tests, host reproductions, the local ARM build, and `particle compile` (network enabled for the Particle compile service only); inspect built binaries / Not authorized: lasting edits, commits, pushes, stash, reset, checkout, flashing, device settings, AWS access.

# Stage 7 round 2 — WO-2026-09-25-001 (review of Stage 6 round 3)

Per AI_DEVELOPMENT_WORKFLOW.md. Dispatched via `codex exec`.

**Review target:** the uncommitted working-tree diff on `wo/2026-09-25-001-publish-with-ack` against its base `599038e` (`git diff 599038e` plus untracked files under `src/` and `tests/`). This round adds Stage 6 round 3 (decisions 6 and 7) on top of what Stage 7 round 1 reviewed.

**Binding spec:** `docs/work-orders/WO-2026-09-25-001-report-webhook-loss.md`: Stage 5 decisions 1–8 and acceptance criteria 1–12 (criterion 7 as revised by decision 8). Your round-1 findings are in `docs/work-orders/WO-2026-09-25-001-stage7-codex/` (`VERDICT.md`, `REVIEW.md`, and the supporting reviews and mutation patches). Copilot's round-3 dispatch and report: `WO-2026-09-25-001-stage6-round3-copilot-dispatch.md`, `WO-2026-09-25-001-stage6-round3-copilot-report.md`.

Out of scope, do not review or modify: `docs/work-orders/` (read only), `AI_DEVELOPMENT_WORKFLOW.md`, WO-2026-09-25-002 through -005. File anything outside scope separately; it does not block this review.

Shell tests are zsh: run via shebang or `zsh <script>`, never bash. Report results as N/N (sh via zsh, py via python3).

## Required checks

1. **Mutations (i)–(iv) are all caught** (each must make at least one test fail; restore byte-identically after each):
   - (i) drop `WITH_ACK`;
   - (ii) delete the queue entry before the ACK;
   - (iii) allow sleep with unacknowledged events queued (remove the queue condition from the sleep gate; note the round-3 rename to `queuePermitsSleep`);
   - (iv) on an ack-wait timeout / Future failure, the event is removed from the queue (your round-1 `iv-remove-on-failure.patch`, adapted if needed).
2. **Host reproduction of ACK failures that never recover** (your round-1 P1 scenario): a connected device whose queued event's publishes fail every time. The device must sleep within the delivery budget (90 s default, plus at most the in-flight hold), **never while a publish is in flight**, and the event must be retained in flash (RAM events moved to flash before sleep). Also confirm `sleptWithQueued` increments and the budget expiry is logged.
3. **Counter invariant across a reset in the middle of a publish:** reproduce with the real `PublishDeliveryCounters` module (your round-1 probe: attempt, reset/`begin()`, attempt, success). `a = k + f + abandoned` must hold afterwards, and the retry must be attributed.
4. **Hibernate cycles emit `CycleDelivery`** and record the `q` snapshot before `System.sleep()`, on both the HIBERNATE and ULTRA_LOW_POWER paths.
5. **Binary and linker-map checks on the new candidate:** build both candidate binaries (local ARM, README command; and `particle compile boron . --target 6.4.1`). For each, confirm the `WITH_ACK` dispatch path, the hooks, the in-flight accessor, and the delivery-gate code are present; record SHA-256 and size; keep both binaries in your scratch output (outside the repository). Confirm the retained block (`retainedPublishDelivery`, now 24 bytes, retained version 2) and its placement in the backup/retained section from the local build's `.map`, quoting the lines.
6. **Criteria 1–12:** a pass/fail table with file and line evidence, including criterion 3 (bounded wait), criteria 11 and 12 (decisions 6 and 7), and criterion 7 as revised (duplicates are possible and counted; consumer tolerance tracked in WO-2026-09-25-004).
7. **Round-1 findings:** confirm each of P1, P2 (hibernate telemetry), P2 (reset-unbalanced counters), P2 (mutation iv), and P3 (`GateFail q` doc) is resolved, or say what remains.
8. **Test hygiene:** Claude Code found an empty `build-tmp/` directory left in the repository root after running the full suite. Identify which test creates it and whether it cleans up (the workflow requires temporary artifacts, including directories, to be removed on completion).

## Builds

References from Copilot round 3: local ARM 153516 / 1114 / 2484; cloud 154718 / 3586.

## Output

VERIFIED, VERIFIED WITH CONCERNS, or NOT VERIFIED, with file and line evidence; the mutation table; the reproduction results; the criterion table (1–12); the round-1 finding table; both binaries' SHA-256; the suite result with interpreters; and the model and reasoning level actually used. Write binaries and scratch files only outside the repository. Restore every mutated file byte-identically and remove temporary artifacts before finishing. Do not commit, push, stash, reset, or check out anything.
