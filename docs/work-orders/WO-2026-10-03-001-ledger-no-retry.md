# WO-2026-10-03-001: a ledger write that's waiting is never retried (v35)

**Goal, in plain language:** a ledger write that's already waiting is never retried; the latest content is written once, after the previous write completes.

**Status:** **APPROVED at USER GATE 2** (Chip, 2026-10-03), after Stage 7 (Codex `gpt-6-astra` high) found no production defect. 61/61; 14 net `src/` lines against a budget raised from 10 to 14. Both Stage 7 notes are applied. Merged (PR #58). **Dev-09 bench PASS** (2026-10-03); the OTA-download case is to be confirmed at the next OTA to a v35 device.

**Workflow:** Per `AI_DEVELOPMENT_WORKFLOW.md`: Stage 5 decisions (Chip and the architect, in the opening dispatch) → Stage 6 (Copilot) → Stage 7 (Codex); §5 model routing; §12 guardrails (plain goal, size budget with no compressed code, two-round rule, facts checked against their source, budget vs actual in the closing record).

**Branch:** `wo/2026-10-03-001-ledger-no-retry`, from `main` at `307581c` (v34 merged, PR #57). All file:line below were checked against `307581c` on 2026-10-02. Before branching, the v34 release image rebuilt from `307581c` matched the deployed image exactly (SHA-256 `043831b5…5bc9`, 151,650 bytes, text/data/bss 150556 / 1090 / 2180).

**Version:** `v35-LedgerNoRetry`, product version 35. It includes v34.

## Background

Source: Claude Code's read-only investigation, 2026-10-02 (`claude-opus-5-5`). Its findings are summarized here.

**The flood.** On Dev-09, during the v34 OTA on 2 Oct, the device logged about 700 `LedgerDuplicateStillInflight` warnings in 16 s (`kind=STATUS`, `new=ClockResync`, about every 22 ms). They continued until the original STATUS write's callback arrived, about 300 s after it was sent (`LedgerCb … STATUS seq=1 … age=302665`).

**The mechanism:**
- ClockResync asks for a status republish **once** per confirmed new time sync. It calls `requestStatusPublish("ClockResync")` at `src/time/Clock.cpp:457`, guarded by `ClockTrust::shouldAttemptRtcWriteNow` at `:415`.
- `Cloud::loop()` (`src/cloud/Cloud.cpp:693–697`) then calls `writeDeviceStatusToCloud(pendingStatusPublishSource)` on **every** pass while `pendingStatusPublish` is set and the device is connected.
- While a STATUS write is in flight, `noteLedgerSyncRequest()` refuses the new one (`Cloud.cpp:280`, return 0 at `:325`). The writer then returns `false` (`src/cloud/DeviceStatusPublisher.cpp:404–419`), so the flag stays set and the next pass tries again.
- The warning appears only once the in-flight write is more than 30 s old (`Cloud.cpp:310`).
- It happens on every boot, because the first time sync lands while ConnectState's STATUS write (`src/state/State_Connect.cpp:582`) is still in flight.

**What each retry costs:** each pass rebuilds the status JSON and allocates a `LedgerData` on the heap (`DeviceStatusPublisher.cpp:374`) before the refusal. Nothing goes to the cloud: `set()` at `:422` is never reached.

**Refused writes today:**

| Write | Outcome |
|---|---|
| STATUS, through the `Cloud::loop()` drain | written later, by the busy retry |
| STATUS from ConnectState (`State_Connect.cpp:582`) | **dropped**; no flag is set |
| DATA from ReportState (`src/Generalized-Core-Counter.cpp:2086`) or ConnectState (`State_Connect.cpp:610`) | **dropped silently**: the refusal returns `true` (`DeviceStatusPublisher.cpp:568–581`) |

**History** (`git log -S "LedgerDuplicateStillInflight"`):
- `a95e284` (2026-06-06) added the duplicate check, the over-30-s release warning, and the retry-on-refusal comment;
- `293f4f7` (2026-08-31, WO-2026-08-29-002) added `requestStatusPublish()` and the ClockResync call, which made the retry happen on every boot.

## This WO closes

1. **WO-2026-09-15-001 Amendment B** (LedgerPayloadStatus, root-caused 2026-09-21, never authorized). **A correction to its record:** Amendment B says each repeated line is "a genuine repeated `deviceStatusLedger.set()` attempt … a real publish-class cost". That's wrong when checked against the source. The refusal returns at `DeviceStatusPublisher.cpp:404–419`, before `set()` at `:422`. The cost is on the device (CPU, heap churn and logging), not cloud publishes. Its root cause, the busy retry in `Cloud::loop()`, is right.
2. **The unwritten "DATA-ledger staleness" item:** refused DATA writes are dropped (`DeviceStatusPublisher.cpp:568–581`). Dev-14's serial log on 30 Sep (13Z) shows this. A DATA write had been in flight for 1,808 s (seq 22) when a newer ReportState write was refused, followed 15 s later by a ConnectState write (`globalSeq=23`). Another ReportState write was refused against seq 35 after 1,134 s.

## Change (`src/cloud/Cloud.h`, `src/cloud/Cloud.cpp`, `src/cloud/DeviceStatusPublisher.cpp`). Budget: about 10 net `src/` lines, raised to 14 (see the approval record)

- **(a1)** In `Cloud::loop()`, drain `pendingStatusPublish` only when no STATUS write is in flight. Use the same test `noteLedgerSyncRequest()` uses: the ledger is tracked and `ledgerHasUnsyncedWriteForDiag()` is true.
- **(a2)** On a STATUS refusal (including from ConnectState), set `pendingStatusPublish` and its source instead of dropping the write.
- **(a3)** On a DATA refusal, set a new `pendingDataPublish` instead of the silent `return true`. The caller still sees success; ReportState and alert 42 are unchanged.
- **(b)** Add a DATA drain in `Cloud::loop()`, gated the same way, after the status drain, with at most one operation per pass. Both payloads are rebuilt when written, so they carry the latest content.
  - **Never write from the sync callback** (system context). The write happens on the next `Cloud::loop()` pass.

Don't compress code to meet the budget. Going over means STOP and report.

## Stage 5 decisions (Chip and the architect, 2026-10-02)

1. **No "log once" latch.** With the gate, the loop never reaches the duplicate check during a wait. The remaining warnings come only from direct callers, at most once per connect or report.
2. **No change to the sleep gate** (`hasPendingOutputLedgerSync()`, `src/cloud/LedgerClient.cpp:89`, used at `src/state/State_Sleep.cpp:500`). `pendingDataPublish` isn't added to it, so a deferred DATA write can wait for the next wake.
   - **Note (Claude Code, fact check):** the sleep gate already counts `pendingStatusPublish` (`LedgerClient.cpp:89`). So with (a2), a refused ConnectState STATUS write now keeps the gate waiting until it drains. Today a refused ClockResync request already behaves this way. While the earlier write is in flight, the gate is already waiting for it through `ledgerHasUnsyncedWrite()`. The extra wait is the one deferred write's round trip. The gate's code is unchanged.
3. **Versioning:** `v35-LedgerNoRetry`, product 35.

## Tests (outside the budget)

- **Update:**
  - the `noteLedgerSyncRequest()` stub in `tests/stubs/power_source_override_overrides/cloud/CloudTestShim.cpp:48`, which always returns 0. `tests/power_source_override_test.cpp:297` describes it. **Fact-check correction:** the dispatch cited `:297`, which is a comment, not the stub;
  - `tests/clock_status_republish_test.cpp:173`'s pinned drain line (`Cloud.cpp:694`).
- **Add one behavioral test** against the real `Cloud.cpp` and `DeviceStatusPublisher.cpp`:
  - **The wait:** a simulated 300 s in-flight wait with the loop running gives zero write attempts and zero log lines during the wait. On the first pass after the completion callback, there is exactly one write, with the latest content;
  - **No drops:** a refused DATA write is written after completion. So is a refused ConnectState STATUS write;
  - **No heap churn:** no heap allocation per pass during the wait (a counting allocator);
  - **Mutation:** removing the in-flight gate makes the test fail.

## Acceptance (Stage 7, narrow)

1. Everything in Tests above, verified, with each mutation failing.
2. The sleep gate is unchanged (`hasPendingOutputLedgerSync()`, `State_Sleep.cpp:500`).
3. No write is made from callback context (`onDeviceStatusLedgerSync`, `onDeviceDataLedgerSync`).
4. One deferred operation per `Cloud::loop()` pass.
5. **The suite passes:** every `tests/*.sh` with zsh plus every bare `tests/*.py` with python3, as N/N. `tests/publish_with_ack_structural_test.py` and `tests/sleep_config_ownership_structural_test.py` pass unchanged.
6. **The build:**
   - a local boron release build, clean;
   - `strings` shows `v35-LedgerNoRetry`, product 35;
   - text/data/bss reported against v34's 150556 / 1090 / 2180.
7. **The budget:** about 10 net `src/` lines, with no compressed code.

## Bench (after the flash, Dev-09; Chip)

The flood happens at boot, when the first time sync lands while ConnectState's STATUS write is still in flight, and during an OTA.

**Pass:** watch serial across a boot and its first connection:
- no run of `LedgerDuplicateStillInflight` warnings;
- the in-flight STATUS write completes (`LedgerCb … STATUS`);
- then exactly one deferred STATUS write.

If an OTA is convenient, do one too, with the same pass criterion.

### Bench result: PASS (Dev-09, 2026-10-03)

**Result: PASS.** The OTA-download case is to be confirmed at the next OTA to a v35 device.

- **Device and source:** Dev-09, status `v35-LedgerNoRetry` at 06:18:47Z (reset reason 20, USB flash).
- **Who checked what:** Chip captured the serial log of the boot and first connection. Claude Code checked each claim against it.

Times are milliseconds since boot.

| Claim | Log line | Holds |
|---|---|---|
| One warning, not a flood; the refused ConnectState DATA write was marked for later | `108047 LedgerDuplicateStillInflight … kind=DATA orig=ReportState new=ConnectState age=106974 … pendingData=1`, the only one in the log | ✓ |
| The in-flight DATA write completed, then the deferred DATA write went out once | `108411 LedgerCb: kind=DATA seq=1` → `108416 LedgerPayloadData` | ✓ |
| STATUS completed, then the deferred clock-resync STATUS went out once | `108083 ClockResync` set it while STATUS seq 2 (ConnectState's, issued at 107592) was in flight → `109179 LedgerCb STATUS seq=2` → `109186 LedgerPayloadStatus` | ✓ |
| One operation per pass | the DATA write (108416) and the STATUS write (109186) went out on separate passes | ✓ |
| Both follow-ups completed, the gate released in 6.9 s, and the device slept | `109828 LedgerCb DATA seq=3`, `115247 LedgerCb STATUS seq=4`, `115253 GateRelease: wait=6854`, then `Sleep: ULP … dur=300s` | ✓ |

**Notes:**
- **The 107 s wait was the connection, not the ledger.** ReportState's DATA write was queued about 1 s after boot. The connection then took 106 s: a DNS failure (`-170`), then cloud recovery at stage 1 (`CloudRecover: success stage=1`, `sig=65/25`). That's why ConnectState's DATA write found it in flight.
- **The ConnectState snapshot that v34 would have dropped was written 5 ms after the first write completed** (108411 → 108416).

## Out of scope

- Device OS's slow callback during an OTA.
- Why Dev-09's warnings stopped after 16 s.

## Approval record

- [x] Stage 5: Chip and the architect, 2026-10-02, in the opening dispatch: goal, background, change, budget about 10, decisions 1–3, tests, Stage 7 checks, version v35 / product 35, bench. Routing: one Copilot round (`claude-opus-5`, medium) and one narrow Stage 7 (Codex, `gpt-6-astra`, high). Not authorized: commits, merging, flashing.
- [x] Fact check (Claude Code, 2026-10-02, against `307581c`): every file:line in the dispatch was verified. There are two corrections, recorded above:
  - the test stub's location;
  - the effect of (a2) on the sleep gate through the existing `pendingStatusPublish` term.
- [x] Stage 6: Copilot (`claude-opus-5`, medium), 2026-10-02. Report: `WO-2026-10-03-001-stage6-copilot-report.md`.
  - **Result:** 13 net `src/` lines against about 10, reported as a deviation. Nothing was compressed.
  - **Tests:** 60/60 before, 61/61 after. The new `ledger_no_retry_test` covers the wait, no drops and no heap churn; its 4 mutations each fail.
  - **Build:** 150676 / 1090 / 2180.
- [x] **Budget 10 → 14 (Chip, 2026-10-02):** the one-per-pass `return` (required by the WO, missed by the estimate), a2's two assignments, a wrapped condition, and the pre-authorized one-line clear of `pendingDataPublish` on a successful direct DATA `set()`.
- [x] **Narrow edit (Claude Code, pre-authorized by Chip, 2026-10-02):**
  - **The edit:** `pendingDataPublish = false;` in `publishDataToLedger()`'s success branch (`DeviceStatusPublisher.cpp`, after `pendingDeviceDataSync = true;`).
  - **Why:** without it, a direct DATA write that succeeds after a deferral leaves the flag set. DATA has no unchanged-payload check, so one redundant DATA write follows once that write completes.
  - **Check:** `ledger_no_retry_test` still passes. Stage 7 checks the edit, including a test that a direct write after a deferral doesn't produce a second write.
- [x] Stage 7: Codex (`gpt-6-astra`, high), 2026-10-02. Verdict: `WO-2026-10-03-001-stage7-verdict.md`. **VERIFIED WITH NOTES.**
  - **Checks:** all 9 pass. All six mutations fail as they should; the clear-flag regression and its mutation were checked in a scratch copy.
  - **Suite and build:** 61/61; build 150684 / 1090 / 2180 (+128 text over v34).
  - **Working tree:** byte-identical afterwards (125,214 entries).
  - **Note 1:** a permanent `testDirectDataWriteClearsDeferral` was proposed as a diff in the verdict, not applied.
  - **Note 2:** the comment at `power_source_override_test.cpp:297` should describe the retry as historical.
- [x] **Note 1 applied (Claude Code, pre-authorized by Chip, 2026-10-03):** `testDirectDataWriteClearsDeferral` added to `tests/ledger_no_retry_test.cpp`, exactly as Stage 7 proposed and tested.
  - **Check:** it passes; removing the clear-flag line makes it fail (checked, source restored byte-identically).
  - **Suite:** 61/61 (sh via zsh, py via python3).
  - **No new src change.** Stage 7 had already verified the clear-flag line and this exact test, so no further Stage 7 round was run.
- [x] **Note 2 applied (Claude Code, authorized by Chip, 2026-10-03):** in `tests/power_source_override_test.cpp`, the comment's last sentence now reads "This mirrors the historical production retry pattern fixed by WO-2026-10-03-001." It is a comment only; the test still passes.
- [x] **USER GATE 2:** Chip, 2026-10-03. Approved without a second Stage 7. Commit v35 as one commit; the recovery-plan edits go in a separate commit; push and open the PR; don't merge.
- [ ] USER GATE 2: Chip.

## Budget versus actual (closing record)

| Item | Budget | Actual net `src/` lines | Tests |
|---|---|---|---|
| (a1)–(b) plus the clear-flag edit | about 10, raised to 14 (Chip: the one-per-pass return, a2's two assignments, a wrapped condition, the clear-flag line) | **14** (Stage 7's count: Cloud.cpp +9, Cloud.h +1, DeviceStatusPublisher.cpp +4) | `ledger_no_retry_test` (+1); the stub and pinned line updated |
