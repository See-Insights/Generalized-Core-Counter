# VERIFIED WITH CONCERNS

**Reviewer:** Codex · **Model:** `gpt-5.6-sol` (probed 2026-10-20, §5) · **Reasoning:** high

**Test interpreters:** shell suites via `zsh`; bare Python suites via `python3`; all 97 suites serial, one at a time. ARM builds used Device OS 6.4.1 Boron, GCC ARM 10.2.1, deterministic per-object `-frandom-seed`, and distinct fresh `BUILD_PATH_BASE` directories. Argon was not built, as dispatched.

Review target: implementation commit `58c59c0ef2cd2b1fbf59c37e1de614a5a9361d4d` against parent `f7de3127a992cddcf555c8312dc328cef3b7befc`; current tip `b5e67dc` adds only Stage 6/result/dispatch documentation.

| Check | Result | Evidence |
|---|---|---|
| 1. Transcript provenance and coverage | **PASS** | Fixture base `9578709` has the same `src` tree (`d39f73f…`) and `State_Sleep.cpp` blob (`9d160b3…`) as the parent and S-2 tip `8d9695e`. Regeneration in a Git-backed detached worktree was byte-identical: 5,471 lines, SHA-256 `6e6d9401dd4b3c2827b91c53b7bf0b42c566937d3ff232b9caa32bd13a35eeff`; body SHA-256 `bf143623…`. The table runs 78 scenarios/476 passes with trace on and the same 78/476 with trace off: every one of the 15 queue/config-ledger/webhook/update masks; separate output-ledger combinations; under/at/over base and raised budgets; depths 1/10/30/60/200; growth/drain and a non-shrinking budget; all three ledger-floor relationships; release with/without/after blocking; not-connected/disconnect/reconnect; firmware, occupancy, CONNECTED+OPEN and entry resets. Per pass it freezes alerts, exact logs, all ten fields, progress, service-end, transitions, process and watchdog calls. `git show` supplies the base; unique marker spans supply the base helpers/inline handler region and current helpers/struct/function/handler region; compiled copies are re-extracted and compared. A tampered include produced `COPY_MISMATCH`. Own update/webhook-priority and not-connected-release-reset mutants each produced `EQUIVALENCE_FAILED`. |
| 2. Moved block byte identity | **PASS** | Independent extraction found 217 old and 217 current core lines. Normalization is byte-identical after removing 57 `cloudGate.` prefixes—`cloudSyncStartMs` 7, `cloudSyncBudgetMs` 11, `cloudSyncMaxQueueDepth` 6, `lastQueueDepth` 3, `operationsCompleteLogged` 2, `gateWasBlocking` 5, `lastGateStatusLogMs` 4, `lastGateBlockerMask` 7, `lastGateBlockerBeforeRelease` 6, `gateReleaseReasonKnown` 6—and changing the one blocking `return;` to `return true;`. The only appended core line is `return false;`; the function signature/braces wrap it. Blank lines are 11 before and 11 after: **zero blank-line differences**. |
| 3. One owner | **PASS** | `State_Sleep.cpp:121-132` declares the ten fields once in one `cloudGate` struct inside the anonymous namespace. Writes occur only in `cloudGatePass` (`:134-353`) and the exact handler reset sites: entry `:721-726,785-787`, CONNECTED+OPEN `:752`, firmware `:804`, occupancy `:815`. An extra handler write was rejected with the expected count mismatch. `State_Idle.cpp:171-179` owns a separate function-local `lastQueueDepth`. No `retained` token was added; only `State_Sleep.cpp` changed in `src/`; `SleepTeardown.*` and the S-2 functions are byte-identical to the parent. |
| 4. Returns, order and effects | **PASS** | The handler order remains lost-timestamp guard, firmware exit, occupancy exit, gate, teardown (`State_Sleep.cpp:796-822`) and returns exactly when `cloudGatePass(...)` is true (`:820`). The sole old blocking return maps to true; every other path reaches false. Byte identity plus the transcript preserves alert priority 43→44→42→40 and conditions, `logLedgerSleepTimeoutState`, `markServiceEnd`, one-shot `operationsCompleteLogged`, and the not-connected resets (`:343-352`). |
| 5. Storage change | **PASS** | The ten local statics became the 24-byte `(anonymous namespace)::cloudGate` at `0x2003d558`, wholly in `.data`. Its linked bytes show only `lastGateBlockerMask=0xff` and `lastQueueDepth=0xffff` nonzero; every other initial value remains zero. All are constant scalar initializers with static storage duration; neither side has a guard symbol, the same 22 static-init stubs remain with no name-set change, and the existing `State_Sleep` init stub stays 2 bytes. Old gate storage occupied 4 bytes of `.data` (the two nonzero locals plus padding) and 24 bytes of `.bss` (zero locals plus padding); the struct gives data +20 and bss −24. Consolidated addressing lets the inlined gate shrink `handleSleepingState` `0x1580→0x14f4` (−140); no other named text symbol changes size, and +4 alignment padding makes total text −136. |
| 6. Retained and ELF | **PASS** | Fresh Boron builds: text/data/bss `151484/1094/2236 → 151348/1114/2212` (−136/+20/−24; total RAM −4). No standalone `cloudGatePass` symbol remains (inlined). `.backup` is 0x98 bytes at VMA `0x2003f400` on both sides and byte-identical, SHA-256 `adfe530f…`; 25 retained-range symbols keep every address. Only its flash LMA follows text, `0xd9340→0xd92cc`. No `retained` keyword was added. |
| 7. Re-anchored tests | **CONCERN** | All four diffs retain their earlier assertions and bind to the new position. `config_window_hold_test.sh` (+9/−6) binds the struct and abort reset; dropping it fails. `firmware_update_dwell_test.sh` (+60/−20) derives guard→gate→teardown and moving the guard fails. `occupancy_report_by_mode_test.{sh,cpp}` (+26/−10) validates the callee guard and handler order; moving occupancy below the gate fails seven targeted checks. `sleep_teardown_structural_test.py` (+9/−3) scopes readers to the handler and its internal guard-below-gate probe fires. A block-comment replacement of the real gate and a trailing-comment replacement of the real teardown call are both ignored as tokens and rejected (`found: guard,teardown` / `guard,gate`). See C1 for a false-positive comment case. A token/log/alert scan found no other test reading moved gate text; broader breadcrumb/transition tests still bind to unchanged properties. |
| 8. Budget, complexity and suite | **CONCERN** | Hard rule: additions minus deletions across all tracked `src/` lines, so intra-`src` moves cancel. `State_Sleep.cpp` 1,348→1,356, `+247/−239 = +8` net (nonblank +6), within +20. Against main `0c565fa`: `src` 22,306→22,351 (+45); tests excluding fixtures 28,893→31,545 (+2,652); fixtures 834→16,868 (+16,034); suites 91→97 (+6). S-3 alone is tests `+981/−42 = +939`, fixture +5,471, suites +2. Full target suite **97/97** (56 zsh + 41 Python), serial; golden payload SHA-256 `07de1e19…` unchanged; S-1/S-2 fixtures unchanged; `git diff --check` clean. See C2. |

RETAINED OUTCOME: A

## Retained addresses (identical before and after)

`0x2003f400`: `link_global_retained_start`, `platform_system_flags_ram_end`, `platform_user_backup_ram_start`, `retainedLoopForensics`; `0x2003f420`: `retainedBoot`; `0x2003f440`: `thrashTripCount`; `0x2003f442`: `lastThrashReason`; `0x2003f444`: `thrashResetCount`; `0x2003f448`: `ledOffTime`; `0x2003f450/454/458`: `lastTrendSoc/Vcell/Epoch`; `0x2003f460`: `pmicAnomalyCount`; `0x2003f468`: `lastPmicAnomalyEpoch`; `0x2003f470/474/478/479/47a`: `lastPmicAnomalyUptimeMs/Soc/ChargeStatus/PowerSource/VbusStatus`; `0x2003f480/488/48c/490`: hibernate RTC-before/requested-sleep/count/pending; `0x2003f494`: `__backup_sram_signature`; `0x2003f498`: `link_global_retained_end`.

## Findings

### C1 — Low — the dwell test's comment handling has a false-positive path

- **Observation:** `tests/firmware_update_dwell_test.sh:41-64` correctly strips block and trailing comments for order discovery, and the two comment-spoof removal mutations are rejected. However, `extract_braced_block` at `:22-35` still takes the first raw marker. Adding an otherwise harmless earlier block comment containing the exact firmware-guard marker makes it extract from the comment and fail compilation, rather than skip the comment and pass.
- **Inference:** This cannot hide a guard-order regression—the clean test, its own movement mutant, both removal/spoof mutants, and `firmware_update_wiring_structural_test.py` cover that property. It is a brittle false failure on a future comment-only edit, so the re-anchor is behaviorally sound but does not fully satisfy the stated comment-skipping robustness.

### C2 — Low — S-3 test volume and trace-off duplication are disproportionate

- **Observation:** S-3 adds 939 non-fixture test lines against the +100 guideline and a 5,471-line fixture against the 1,500–2,500 estimate. Trace-on has 2,984 body lines; trace-off 2,480. After removing compile-time gate-trace log lines, the trace selector, and the macro-sensitive event catalog/count, their 2,415-line projections are identical.
- **Inference:** Building trace-off is justified to prove that the `#if`-compiled path builds and trace logs disappear, but replaying and freezing every one of the same 476 behavioral passes adds no state/branch coverage beyond those log lines. A small trace-off matrix or a projected comparison could retain that assurance with less maintenance cost.

## Budget versus actual

| Item | Budget/guideline | Actual |
|---|---:|---:|
| `src/` net | ≤ +20 hard | **+8** all lines; **+6** nonblank |
| S-3 tests excluding fixture | +100 guideline | **+939 net** (`+981/−42`) |
| S-3 fixture | 1,500–2,500 estimate | **5,471 lines** |
| S-3 suites | baseline 95 | **97** (+2) |

## Confirmations

- The original worktree was clean before review. After review, the authorized verdict file is its only change.
- Fixture regeneration left the exact detached target worktree clean. All builds, mutations, generated files and logs were confined to review-created Git-backed detached worktrees or paths under `build-tmp/wo20261021-002-stage7/`; that scratch tree was removed.
- No commit, push, stash, reset, checkout/restore, device, network or AWS access occurred. Only Boron was built.
