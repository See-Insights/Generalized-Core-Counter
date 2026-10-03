## Implementation Report — WO-2026-10-03-001 (v35-LedgerNoRetry)

**Model/reasoning actually used:** `claude-opus-5`, medium.

### Changes (net `src/` lines, nonblank/non-comment)

| Item | File | Lines |
|---|---|---|
| a1 | `Cloud.cpp` `loop()` — status drain gated on `!(isLedgerPointerTracked(&deviceStatusLedger) && ledgerHasUnsyncedWriteForDiag(deviceStatusLedger))` | +1 (condition continuation), −1 (old condition) |
| a2 | `DeviceStatusPublisher.cpp` STATUS refusal — sets `pendingStatusPublish` + `pendingStatusPublishSource = issueSource`, still returns `false` | +2 |
| a3 | `DeviceStatusPublisher.cpp` DATA refusal — sets `pendingDataPublish`, still returns `true` | +1 |
| b | `Cloud.cpp` `loop()` — `return;` after status drain + DATA drain block (`publishDataToLedger("DeferredData")`) | +7 |
| decl | `Cloud.h` `bool pendingDataPublish;` (next to `pendingStatusPublish`, not in `hasPendingOutputLedgerSync()`) | +1 |
| ctor | `Cloud.cpp` `pendingDataPublish = false;` | +1 |

**Total: 14 added / 1 removed = 13 net** (version bump excluded). Sleep gate, `LedgerClient.cpp` callbacks, `src/state/`, `Generalized-Core-Counter.cpp` untouched.

### Tests
- **Updated:** `tests/stubs/power_source_override_overrides/cloud/CloudTestShim.cpp` — added `pendingDataPublish(false)` to the ctor init list (member-order correct); the `noteLedgerSyncRequest()` stub itself needed no change, since both refusal paths still run exactly as that harness documents. `tests/clock_status_republish_test.cpp:173` — pinned drain line `Cloud.cpp:694` → `:703`.
- **Added:** `tests/ledger_no_retry_test.cpp` + `.sh` + `tests/stubs/ledger_no_retry_overrides/Particle.h` (fork of the clock-republish stub, per its own fork precedent; adds opt-in in-flight simulation, test-controlled `millis()`, log-line counting). Links the real `Cloud.cpp`/`DeviceStatusPublisher.cpp`. Proves: 13,636 passes over a simulated 300 s wait → 0 `set()`, 0 payload builds, 0 log lines, 0 heap allocations (global `operator new` counter); exactly one write with `trusted=true` on the first pass after completion; refused DATA and refused ConnectState STATUS both written after completion; one deferred op per pass.

### Mutations (each applied in place, then restored byte-identically — `cmp` verified)
| # | Mutation | Result |
|---|---|---|
| M1 | remove the in-flight gate | **FAIL** (`g_allocCount == allocBefore`, line 173) |
| M2 | remove the one-op-per-pass `return;` | **FAIL** (line 294) — yes, it fails the test |
| M3 | revert a3 (`pendingDataPublish`) | **FAIL** (line 232) |
| M4 | revert a2 | **FAIL** (line 264) |

### Verification
- Host suite, `tests/*.sh` via **zsh** + bare `tests/*.py` via **python3**: **60/60 before**, **61/61 after** (+1 new test). `publish_with_ack_structural_test.py` and `sleep_config_ownership_structural_test.py` pass unmodified.
- `./bump_version.sh v35-LedgerNoRetry` → product 35, `FIRMWARE_VERSION = "v35-LedgerNoRetry"`.
- ARM release (boron, Device OS 6.4.1, after successful `make clean-user`): **150676 / 1090 / 2180** vs v34's 150556 / 1090 / 2180 → **+120 text, +0 data, +0 bss**. `strings` finds `v35-LedgerNoRetry`; no `pdiag`.
- `git diff --stat`: 10 modified files, 52 insertions / 9 deletions; 5 untracked (2 WO docs pre-existing, 3 new test artifacts). Working tree left uncommitted.

### Deletions
Deleted **only** `build-tmp/wo20261003-001-stage6/`. `build-tmp/`, `build-tmp/connectivity-archive/` and `build-tmp/WO-2026-10-01-001-stage6-copilot-transcript.md` are intact.

### Deviations
1. **Budget: 13 net `src/` lines vs "about 10"** — 30% over, reported per the guardrail. No code was compressed to reduce it; the overage is the two-line a2 assignment pair, the `return;` kept for consistency with the `pendingConfigApply` block above it, and the two-line gate conditions kept on separate lines for readability. Nothing optional was added.
2. `docs/RECOVERY_PLAN_2026-09-26.md` shows as modified — that is the **pre-existing** uncommitted edit, unchanged by me. `CHANGELOG.md`, `README.md` and `Doxyfile` were changed by `bump_version.sh` (authorized).
3. `tests/power_source_override_test.cpp:297`'s comment now describes a production shape this WO removes ("Cloud::loop() retrying an already-in-flight sync"). It pins no text and no assertion, so I left the file untouched rather than churn it; flagging for Stage 7.

