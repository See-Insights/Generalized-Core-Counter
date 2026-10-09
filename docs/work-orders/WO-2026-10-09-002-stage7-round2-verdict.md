# VERIFIED WITH CONCERNS — Codex · gpt-5.6-sol (standard) · reasoning high

Test interpreters: **74/74 — 43 shell tests via `zsh`, 31 Python tests via `python3`.**

| Check | Verdict | Evidence |
|---|---|---|
| R1-1 Linkage/build | **PASS** | Fresh Boron/Device OS 6.4.1 build: **151316 / 1090 / 2220** text/data/bss; versus v40 **+304 / 0 / +24**. ELF and objects contain `areLedgersSynced`, both snapshot statics, `holdDoneEpoch`, `configHold`, and `cloudSyncStartMs`. Disassembly confirms the CONNECTED+open zero-store before transition. An initial build-command quoting error produced `-frandom-seed=`; the successful build used a second, unused `BUILD_PATH_BASE`. |
| R1-2 Tests | **PASS** | Full host suite **74/74** before creating any `src/` copy. |
| R1-3 Hold behavior | **PASS** | On a new edge, [LedgerClient.cpp:125](</private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/d85bec8a-671e-48d3-9139-ab71c8426e9b/scratchpad/wo-cw/src/cloud/LedgerClient.cpp:125>) selects the hold and snapshots both inputs. Output-pending calls re-anchor at line 148. Either changed snapshot or `>10 s` completes at lines 151–155; only that completion stamps the marker. Both equal-baseline 200/200→150 and unequal-baseline 200/100→50 scratch cases ended immediately with `onSync`. |
| R1-4 No cost elsewhere | **PASS** | Outside a hold, the original both-synced fast return remains at line 159. Warm, partial-with-content, empty-device, and cold cases passed. |
| R1-5 Ruling 4 | **PASS** | The zero-marker boot term is independent of trust; untrusted boot passed. `todayAt()` performs no trust check and returns the configured hour in the current converted day at [DailyBoundary.cpp:9](</private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/d85bec8a-671e-48d3-9139-ab71c8426e9b/scratchpad/wo-cw/src/time/DailyBoundary.cpp:9>); with an invalid clock it is an epoch-derived boundary. The daily term itself is trust-gated and does not normally repeat. |
| R1-6 Once-a-day edges | **CONCERN** | Cut-short holds do not stamp and retry; committed cases pass. Scratch 03:00 boot/open-at-06:00 case produced exactly two holds, then an immediate third connection. The zero-epoch repetition is fixed, but marker `1` has the bounded edge described below. |
| R1-7 Gate interactions | **PASS** | The hold feeds the ledger blocker at [State_Sleep.cpp:511](</private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/d85bec8a-671e-48d3-9139-ab71c8426e9b/scratchpad/wo-cw/src/state/State_Sleep.cpp:511>). Gate remains 30 s base and ratchets to 70 s while output is pending; accepted late-clear alert 44 remains possible. Firmware/occupancy exits reset the timer. The hold does not write `lastConnection`; teardown/standby starts only after release or GateFail. |
| R1-8 `:407` change | **PASS** | [State_Sleep.cpp:409](</private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/d85bec8a-671e-48d3-9139-ab71c8426e9b/scratchpad/wo-cw/src/state/State_Sleep.cpp:409>) resets `cloudSyncStartMs`. Its declaration move is behavior-neutral and matches the other mid-gate exits. Object disassembly proves the zero-store precedes `transitionTo`. |
| R1-9 Tests/mutations | **PASS** | The harness byte-extracts and compiles the real function at [config_window_hold_test.sh:32](</private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/d85bec8a-671e-48d3-9139-ab71c8426e9b/scratchpad/wo-cw/tests/config_window_hold_test.sh:32>). All nine committed mutations were caught by runtime/structure checks, not compilation errors. |
| R1-10 Budget | **PASS** | WO total is exactly **+20** net nonblank, non-comment `src/` lines. |
| R2-2 F1 | **PASS** | Separate snapshots at lines 133–134; either `!=` at line 151 releases. Both requested backward-step cases passed. Output writes use distinct `device-status`/`device-data` ledgers and cannot alter the two input timestamps; snapshot assignment only copies values. |
| R2-3 F2 | **CONCERN** | Line 155 stores `currentConnectionEpoch ? currentConnectionEpoch : 1`, never zero. The zero-epoch committed case passed. Marker `1` can cause one additional first-after-open hold after trust arrives; it is bounded under the normal connection path. |
| R2-4 F3 | **PASS** | The unequal-baseline backward case at [config_window_hold_test.cpp:256](</private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/d85bec8a-671e-48d3-9139-ab71c8426e9b/scratchpad/wo-cw/tests/config_window_hold_test.cpp:256>) catches `!=`→`>`. |
| R2-5 Mutations | **PASS** | All nine harness mutations passed. My additional “snapshot only default ledger” mutation failed `boot_trusted` and the device-ledger backward case as required. |
| R2-6 Budget/fallback | **PASS** | Round 2 added +2 code lines, bringing the WO to the exact +20 cap. **Fallback was not needed.** |

## Findings

- **P3 concern — Observation:** [LedgerClient.cpp:155](</private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/d85bec8a-671e-48d3-9139-ab71c8426e9b/scratchpad/wo-cw/src/cloud/LedgerClient.cpp:155>) converts a zero completion epoch to `1`. Once the clock becomes trusted, `1` is below today’s opening boundary, permitting a first-after-open hold. A scratch case reproduced boot completion at zero, then exactly one daily hold, then an immediate same-day connection.

  **Inference:** this is reachable if the initial successful connection stores zero before clock trust arrives. It is bounded to once in the normal state-machine path because [State_Connect.cpp:513](</private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/d85bec8a-671e-48d3-9139-ab71c8426e9b/scratchpad/wo-cw/src/state/State_Connect.cpp:513>) refreshes `lastConnection` from the now-trusted clock before the next gate, and completion then replaces marker `1` with that real epoch. This fails toward one extra delivery-safe hold, but the edge was not explicitly pre-accepted.

No P1/P2 findings remain.

## Budget versus actual

| Scope | Net `src/` code lines | Cap |
|---|---:|---:|
| Round 1 | +18 | — |
| Round 2 | +2 | — |
| WO total | **+20** | **+20** |

The final tree matches the starting tree exactly: the same two modified source files and two untracked test files; the tracked diff and untracked-file hashes matched. `git diff --check` is clean for the uncommitted changes. I explicitly removed only `build-tmp/wo20261009-002-stage7r2/`; all other `build-tmp/` contents remain untouched.