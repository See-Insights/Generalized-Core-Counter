_Codex Stage 7 (narrow) verdict for decision 11, verbatim. Codex CLI 0.154.0, `gpt-6-astra`, reasoning high (confirmed in the run header). Dispatch: `WO-2026-09-25-001-stage7-decision11-codex-dispatch.md`._

**NOT VERIFIED** — decision 11’s edits are correct, but the work order’s queue-total claim is false. Model: **gpt-6-astra**, reasoning: **high**.

1. **Queue log arguments confirmed, with existing exceptions.** All paths below derive their counts from `getNumEvents()`:
   - `LoopStage`: `queueDepth` at `src/Generalized-Core-Counter.cpp:675,683,691`; sampled at `:704,2502`.
   - `BootWDT`: retained `startupPreviousQueueDepth` at `src/Generalized-Core-Counter.cpp:1526`, originating at `:649,797,805`.
   - `ConnDiag`: `queueDepth` at `src/state/State_Connect.cpp:462,478`, sampled at `:437`.
   - `ConnSummary` ok/fail: `summaryQueueDepth` at `src/state/State_Connect.cpp:517,528,691,702`, sampled at `:504,678`.
   - `Connect: ok`: `pending` at `src/state/State_Connect.cpp:616,626,632`, sampled at `:608`.
   - `CYCLE end`: three historical depth snapshots at `src/state/State_Sleep.cpp:1254,1255,1256`, sampled at `State_Connect.cpp:306,540` and `State_Sleep.cpp:1194`; unavailable snapshots print `-1`.
   - `GateBlock`, `Gate`, `GateFail`: `(int)queueDepth` at `src/state/State_Sleep.cpp:562,575,598`, sampled at `:491`.
   - New Idle: `(unsigned)pending` at `src/state/State_Idle.cpp:251`, sampled at `:245`.
   
   Separately, unchanged logging exceptions remain: release-side `GateBlock` hardcodes `q=0` (`State_Sleep.cpp:634`); diagnostic-skip depth uses `queue depth=`, not `q=` (`Generalized-Core-Counter.cpp:2466`).

2. **FAIL — total-count claim.** `publishCommon()` flushes queued RAM events when flash files exist (`lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:82,89`). However, `getNumEvents()` adds `curEvent` **only when `ramQueue` is empty** (`:247,251`). With two RAM events, sending one (`:313,314`) reports **1 instead of 2: −1**. After a failed RAM send, flushing leaves stale `curEvent` (`:381,385`), producing **+1** until the next selection. Both errors were reproduced using the unchanged method body. An in-flight RAM event can also coexist with flash events; that combination counts correctly when `ramQueue` is empty.

3. **PASS — Report.** Both variants use `ok=` and unchanged `queued ? 1 : 0` (`src/Generalized-Core-Counter.cpp:2129,2133,2136,2140`); `queued` remains `publish()`’s return (`:2061`).

4. **PASS — Idle.** Exact requested message and queue-count argument at `src/state/State_Idle.cpp:250,251`; offline message unchanged at `:247,248`.

5. **PASS — gate behavior.** Only three logging arguments changed. `queuePending` still drives `blockerMask` (`src/state/State_Sleep.cpp:515,520`); `gateFailReason` still uses equivalent `!queueEmpty` (`:593`). Control flow is unchanged.

6. **PASS against this dispatch — version.** `FIRMWARE_VERSION` is `"v25-WithAck"` (`src/Version.cpp:6`); numeric product version remains **24** (`src/FirmwareVersion.h:28`). During verification, an external WO edit added a requirement for **25**; that was outside this dispatch’s unchanged-version criterion.

7. **PASS — 44/44 (sh via zsh, py via python3)**: 22 shell scripts and 22 Python scripts, including `tests/publish_with_ack_structural_test.py`.

No repository writes performed. Source and test hashes remained unchanged; the WO changed externally. All temporary artifacts removed.