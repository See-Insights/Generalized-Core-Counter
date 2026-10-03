AGENT: Copilot · MODEL: claude-opus-5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/cloud/Cloud.h`, `src/cloud/Cloud.cpp` and `src/cloud/DeviceStatusPublisher.cpp` (items a1, a2, a3 and b only); run `./bump_version.sh v35-LedgerNoRetry "<one-line note>"`; add or update tests under `tests/` (including `tests/stubs/`); run the host suite and a local ARM release build. Your scratch directory is **`build-tmp/wo20261003-001-stage6/`**: keep every temporary file in it and remove **only it** when done / Not authorized: commits, pushes, merges, branch changes, stash, reset, checkout, flashing, device settings, AWS or network access, any edit to `lib/`, `project.properties`, `docs/` or Device OS, any change beyond this WO (including the sleep gate, `LedgerClient.cpp`, and any caller in `src/state/` or `Generalized-Core-Counter.cpp`), and **deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`**.
**SIZE BUDGET: about 10 net `src/` code lines (nonblank, non-comment, including braces, declarations and includes). Don't compress code to meet it (`AI_DEVELOPMENT_WORKFLOW.md` §12 guardrail 3). Going over means STOP and report.**

# Stage 6 dispatch — WO-2026-10-03-001 (v35-LedgerNoRetry)

**Goal, in plain language:** a ledger write that's already waiting is never retried; the latest content is written once, after the previous write completes.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-10-03-001-ledger-no-retry`, `HEAD` `307581c` (v34 on main). Do not switch branches. Files under `docs/` are records: don't touch them, including the unrelated uncommitted `docs/RECOVERY_PLAN_2026-09-26.md` edit.
**Binding spec:** `docs/work-orders/WO-2026-10-03-001-ledger-no-retry.md`, including its Stage 5 decisions and fact-check corrections. Where this dispatch and the WO differ, the WO wins; report the difference.

## Your role

You are the Implementer under `AI_DEVELOPMENT_WORKFLOW.md`. Work only from the WO. Any correction to the spec is reported as a deviation. If it can't be done as approved within the budget, stop and report it. Leave an uncommitted working-tree diff.

## What to implement (all file:line checked against `307581c`)

| Item | Where | Change |
|---|---|---|
| a1 | `Cloud::loop()`, `Cloud.cpp:693–697` | Drain `pendingStatusPublish` only when no STATUS write is in flight, using the same test as `noteLedgerSyncRequest()` (`Cloud.cpp:270–280`): the ledger is tracked **and** `ledgerHasUnsyncedWriteForDiag()` is true. |
| a2 | STATUS refusal, `DeviceStatusPublisher.cpp:404–419` | Set `pendingStatusPublish` and `pendingStatusPublishSource` (the caller's source) instead of dropping the write. This covers ConnectState (`State_Connect.cpp:582`). |
| a3 | DATA refusal, `DeviceStatusPublisher.cpp:568–581` | Set a new `pendingDataPublish` (declared in `Cloud.h` next to `pendingStatusPublish`, initialized `false` in the constructor) instead of the silent `return true`. Still return `true`: ReportState and alert 42 are unchanged. |
| b | `Cloud::loop()`, after the status drain | A DATA drain, gated the same way: `publishDataToLedger(...)`, clearing `pendingDataPublish` when it succeeds. **At most one deferred operation per pass**: if the status drain ran this pass, the DATA drain waits for the next one. |

- **Never write from the sync callbacks** (`onDeviceStatusLedgerSync` / `onDeviceDataLedgerSync` in `LedgerClient.cpp`, system context). Don't edit them.
- **Both payloads are rebuilt when written**, so they carry the latest content. Don't cache payloads.
- **No "log once" latch** (decision 1). **No change to the sleep gate** (decision 2): `hasPendingOutputLedgerSync()` is unchanged, and `pendingDataPublish` isn't added to it.
- **Version:** `./bump_version.sh v35-LedgerNoRetry "<one-line note>"`, giving product 35.

## Tests (outside the budget)

1. **Update** the `noteLedgerSyncRequest()` stub at `tests/stubs/power_source_override_overrides/cloud/CloudTestShim.cpp:48`, only as needed for that harness to keep testing what it tests. `tests/power_source_override_test.cpp:297` describes the stub. Also update the pinned drain line in `tests/clock_status_republish_test.cpp:173` (`Cloud.cpp:694`). Report each change.
2. **Add one behavioral test** against the real `Cloud.cpp` / `DeviceStatusPublisher.cpp`. Reuse an existing harness pattern, such as `clock_status_republish_test`, where it fits. It must show:
   - a simulated 300 s in-flight STATUS wait with `Cloud::loop()` running gives **zero write attempts** (`set()` calls and payload builds) and **zero log lines** during the wait, then **exactly one write with the latest content** on the first pass after the completion callback;
   - a refused DATA write is written after completion (not dropped), and so is a refused ConnectState STATUS write;
   - **no heap allocation per pass during the wait** (a counting allocator);
   - **mutation:** removing the in-flight gate makes the test fail. Also say whether removing the one-operation-per-pass rule fails it.
3. Run each mutation on a copy or restore byte-identically. Update an existing test only if it pins text this WO changes, and report each one. `tests/publish_with_ack_structural_test.py` and `tests/sleep_config_ownership_structural_test.py` must pass unchanged.

## Verification (run all; report results)

1. Full host suite: every `tests/*.sh` with **zsh**, plus every bare `tests/*.py` with python3, as `N/N`, before (v34: 60/60) and after.
2. Local ARM release build (boron, Device OS 6.4.1, after `make clean-user`, in your scratch directory or with outputs restored): text/data/bss against v34's 150556 / 1090 / 2180; `strings` shows `v35-LedgerNoRetry`.
3. Net `src/` lines against about 10, and `git diff --stat`.

## Implementation Report (required, as your final message)

- The change per item, with line counts.
- Tests and mutations, with results.
- Commands and results, with interpreters and sizes.
- Confirmation that you deleted only `build-tmp/wo20261003-001-stage6/`.
- Deviations (or "none").
- The model and reasoning level actually used.
