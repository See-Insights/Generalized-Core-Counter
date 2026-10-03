AGENT: Codex · MODEL: gpt-6-astra · REASONING: high
AUTHORIZATION SCOPE: review the uncommitted diff; run the host suite and a local ARM build in a scratch copy (or with outputs restored); write temporary harnesses under your scratch directory **`build-tmp/wo20261003-001-stage7/`** and remove **only that directory** when done; temporarily mutate files for checks, restoring each byte-identically by rewriting in place / Not authorized: lasting edits, commits, pushes, stash, reset, checkout, flashing, device settings, network or AWS access, and deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`.

# Stage 7 dispatch (narrow) — WO-2026-10-03-001 (v35-LedgerNoRetry)

**Goal, in plain language:** a ledger write that's already waiting is never retried; the latest content is written once, after the previous write completes.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-10-03-001-ledger-no-retry`, base `307581c` (v34), with the uncommitted diff. Ignore the unrelated uncommitted edit to `docs/RECOVERY_PLAN_2026-09-26.md`.
**Binding spec:** `docs/work-orders/WO-2026-10-03-001-ledger-no-retry.md`, including its Stage 5 decisions, the fact-check notes, the budget raise to 14, and the narrow edit in the approval record.
**Stage 6 report:** `docs/work-orders/WO-2026-10-03-001-stage6-copilot-report.md`.

Narrow review: check against the WO's acceptance criteria only. Don't widen the fault model or propose new mechanisms. For any defect, give the smallest fix within the approved design.

## Checks (PASS/FAIL with evidence for each)

1. **No retry while in flight (a1, b):**
   - both drains in `Cloud::loop()` are gated by exactly the condition under which `noteLedgerSyncRequest()` refuses (`Cloud.cpp`, `existing && inflightForPointer`);
   - during a simulated 300 s wait with the loop running: zero `set()` calls, zero payload builds, zero log lines, zero heap allocations per pass;
   - exactly one write, with the latest content, on the first pass after the completion callback.
2. **No drops (a2, a3):**
   - a refused DATA write (ReportState, ConnectState) is written after completion;
   - a refused ConnectState STATUS write is too;
   - ReportState's return value and alert 42 are unchanged.
3. **The narrow edit (clear-flag):** `pendingDataPublish = false;` in `publishDataToLedger()`'s success branch.
   - **Test (required):** after a deferral, a direct DATA write that succeeds (for example, the next ReportState) produces **no second write** after its completion;
   - **Mutation:** removing that line fails the test.

   The test may extend `tests/ledger_no_retry_test.cpp` in a scratch copy. If it belongs in the suite permanently, give the exact addition as a proposed diff, but don't apply it.
4. **One deferred operation per pass**, and the mutation that removes it fails a test.
5. **Nothing written from callback context:** `onDeviceStatusLedgerSync` / `onDeviceDataLedgerSync` are unchanged and only clear flags.
6. **The sleep gate is unchanged:** `hasPendingOutputLedgerSync()` (`LedgerClient.cpp:89`) and `State_Sleep.cpp:500`, and `pendingDataPublish` isn't part of it. Confirm the WO's fact-check note on (a2): a refused ConnectState STATUS write now keeps the gate waiting through the existing `pendingStatusPublish` term, and nothing else changes.
7. **Test changes keep their intent:** the `CloudTestShim.cpp` constructor addition, the pinned line `Cloud.cpp:694` → `:703` in `clock_status_republish_test.cpp`, and Copilot's deviation 3 (the stale comment at `power_source_override_test.cpp:297`). Say whether that comment needs a change.
8. **Suite and build:**
   - every `tests/*.sh` with **zsh**, plus every bare `tests/*.py` with python3, as `N/N` (Copilot: 61/61);
   - `tests/publish_with_ack_structural_test.py` and `tests/sleep_config_ownership_structural_test.py` unchanged and green;
   - a local boron release build, clean: text/data/bss against v34's 150556 / 1090 / 2180 (Copilot, before the narrow edit: 150676 / 1090 / 2180); `strings` shows `v35-LedgerNoRetry`, product 35.
9. **The budget:** net `src/` lines (nonblank, non-comment) against 14, by your counting rule; nothing compressed.

## Verdict (required)

VERIFIED, NOT VERIFIED, or VERIFIED WITH NOTES, per check and overall, with evidence; binary sizes; the model and reasoning level actually used. Confirm that the working tree is byte-identical to how you found it, and that nothing outside `build-tmp/wo20261003-001-stage7/` was created or deleted.
