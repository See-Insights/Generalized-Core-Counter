# NOT VERIFIED

**Reviewer:** Codex · **Model:** `gpt-5.6-sol` (standard tier; probed 2026-10-20) · **Reasoning:** high

**Test interpreters:** shell tests via `zsh`; bare Python tests via `python3`; all suites serial, one suite at a time.

Review target: implementation commit `658b7cf018d6680141b2a239b01b71ee42950d43` against parent `7176048`; the branch tip `3648dd3` changes only documentation after the implementation. Detached scratch worktrees from a local bare clone preserved real history. The fixture's base `0c565fa0bec00f5e91f62ec30f10b15d5b9b074d` has a byte-identical `src/` tree to `7176048`.

| Check | Result | Evidence |
|---|---|---|
| 1. Transcript provenance and coverage | **CONCERN** | Regeneration from the fixture header's full base hash produced a byte-identical 4,753-line fixture, SHA-256 `d3b97f9d7de3a49273f8dcd4feafc08425f344d5ec781fce45bef5125275fa1e` (body `803dbede…`). The runner uses `git show` plus unique marker spans for the real base labels, safety guard and block, and directly includes the real candidate `.cpp`; it is not a mirrored implementation. Its 174 scenarios/1,065 passes cover cellular 120 and Wi-Fi 54, cloud online/offline, cellular on/in-between/off, standby requested/effective `(0/0,1/0,1/1)`, all budget configurations, delayed modem-off, force-off pending, and executed pass offsets 4999/5000/5001 and 9999/10000/10001 (Wi-Fi finishes at 10000). The exact 5000-ms ModemHealth boundary is separately pinned at 4999/5000/5001 by `modem_health_equivalence_test.cpp:70`; the transcript fakes ModemHealth. The final precondition request arms run, but its wait/timeout arm does not and cannot; see check 6. |
| 2. Moved ranges byte-identical | **PASS** | Independent marker comparison found all three helpers/labels, both lambdas, the safety reset and the base teardown block byte-identical after only canonicalizing the call boundary (`return;` to five `WAIT` and two `ERROR` results, plus final `READY`) and removing the new method brace. `logWakeSummary` remains in `State_Sleep.cpp`. Every other source difference is listed below. |
| 3. One owner / latch ownership | **PASS** | `SleepTeardown.cpp:59-62,346` contains one file-static 28-byte `Teardown s` owning all nine fields. No other `src/` definition/write exists. `disconnectRequested()` at `:350` is read-only and is used by the three former readers at `State_Sleep.cpp:269,280,287`. All writes to `session.modemUnstable`, `modemStandbySuppressed`, `modemUnstableReason`, and `modemConnectTimeoutCount` remain only in `power/ModemHealth.cpp`. Neither new file contains `retained`. |
| 4. Returns and transitions | **PASS** | `State_Sleep.cpp:504` returns for every non-READY result, exactly matching all old early returns. `SleepTeardown.cpp:202,322` retains both ERROR transitions and reasons; `:222-224,340-342` retains both watchdog/process/WAIT paths. The transcript observes WAIT/READY/ERROR, both collaborators, and the reachable timeout. |
| 5. Entry-reset STOP condition | **PASS** | `SleepTeardown::enter()` runs at `State_Sleep.cpp:169`; the CONNECTED+OPEN abort is `:213-217`. None of the seven newly early-reset fields is read between those points, and the next state entry resets them again. The exhaustive read audit is below. The consolidation is behavior-neutral on this abort path. |
| 6. Dead precondition timeout | **PASS** | `SleepTeardown.cpp:309-342` is unreachable without memory corruption/an unauthorized writer: a true latch with `stillOn` returns at `:224`; a true latch with `!stillOn` is cleared at `:250`; a false latch at the final gate is set by the request arm and returns at `:306`. Radio/cloud changes between reads can reach that request arm, not the timeout arm. The move preserves this base dead branch; its alert-15 mutant is explicitly reported unpinned. |
| 7. ELF and retained | **PASS** | Fresh Device OS 6.4.1 Boron builds: text/data/bss `151724/1094/2228 → 151532/1094/2228` (text −192). `handleSleepingState` −1416; new API symbols +1248; removed `CloudClass::process()` clone −24; helper/switch-table moves net zero. Static-init stubs are 22/22. `.backup` is `0x98`, byte-identical SHA-256 `adfe530f…`; all 25 symbols retain addresses, while only flash LMA moves `0xd9430 → 0xd9370`. |
| 8. Re-pointed tests not weakened | **FAIL** | A source mutation moving the firmware-update guard from `State_Sleep.cpp:269` to below the teardown call at `:504` survived the entire **93/93** suite. `firmware_update_wiring_structural_test.py:41-45,143-153` still orders the guard against later direct `Connectivity::request*` calls in `State_Sleep.cpp`, not `SleepTeardown::run`; `firmware_update_dwell_test.sh:149-170` extracts the guard and synthetically places it first. The candidate implementation is correctly ordered, but its required regression assertion is weakened. |
| 9. Budget and suite | **PASS** | Independent all-line rule: sum additions minus deletions over `src/`, so intra-`src` moves cancel. `+368−348 = +20`, exactly the hard ceiling; nonblank total is +18. New harness/test/stub files are +793 versus +175 guideline, plus the accepted 4,753-line fixture; all test changes excluding the fixture are +845/−36 = net +809. `git diff --check` is clean. Candidate suite **93/93**: 54 shell + 39 Python. Golden payload fixture is unchanged at SHA-256 `07de1e19…` and its test passes. |

**Retained outcome: A**

## Complete moved-source difference list

- Added `SleepTeardown.h`, its include, namespace/anonymous namespace, `Teardown` owner, `s`, and four forwarding API functions; the nine former statics became initialized members with the same names.
- Moved the three helpers exactly; `logWakeSummary` did not move. Moved the safety guard exactly into `guardLostTimestamp()` and the teardown block exactly into `run()`.
- Replaced five old WAIT-path `return;` statements with `Result::WAIT`, two post-transition returns with `Result::ERROR`, added final `Result::READY`, and replaced the inline block with the non-READY return gate at `State_Sleep.cpp:504`.
- Consolidated the two entry-reset sites into `s = Teardown()` at `SleepTeardown.cpp:348`; replaced only the three external latch readers with the read-only query.
- Passed requested/effective standby by value and the two S-2 standby-return fields by reference. Added non-cellular dummy locals at `State_Sleep.cpp:156-158` solely to satisfy those references. No other moved-body, lambda, label, log, side effect or ordering difference exists.

## Seven-field read audit for the entry-reset ruling

- `disconnectRequested`: owner reads at `SleepTeardown.cpp:68` (lost-timestamp guard), `:125,150` (request predicates), `:172,182` (completion logging), `:192` (primary wait), `:228` (completion), `:275` (final request arm), and `:350` (query); query consumers are `State_Sleep.cpp:269,280,287`. All occur after the abort return.
- `disconnectRequestStartMs`: `SleepTeardown.cpp:68` (guard), `:144,166` (teardown-start calls), `:184` (modem elapsed), `:193,195,200` (primary timeout/log/latch), `:230` (total elapsed), `:305` (final-gate start call), `:309,313` (dead timeout setup/elapsed). All occur after the abort return.
- `cloudDisconnectStartMs`: `SleepTeardown.cpp:173` (cloud elapsed), after the abort return.
- `cloudDisconnectElapsedMs`: `SleepTeardown.cpp:176` (completion log), `:233` (summary), `:240` (session copy), after the abort return.
- `modemOffElapsedMs`: `SleepTeardown.cpp:186` (completion log), `:234` (summary), `:241` (session copy), after the abort return.
- `lastDisconnectProgressMs`: `SleepTeardown.cpp:218` (reachable primary-wait throttle), after the abort return.
- `lastGateProgressMs`: `SleepTeardown.cpp:336` (dead final-wait throttle), after the abort return.

## Re-pointed-test diff audit

- `connection_mode_downgrade_structural_test`: exact count split `State_Sleep 8 → 6` plus `SleepTeardown 2`; not weakened.
- `firmware_update_dwell_test`: query shim/marker only; runtime assertions unchanged, but it never preserves source ordering (part of F1).
- `firmware_update_wiring_structural_test`: query regex updated, teardown ordering anchor not updated; **weakened** (F1).
- `occupancy_report_by_mode_test`: query shim plus reset/run anchors; the new call boundary is included in decision ordering; not weakened.
- `sleep_gate_modem_off_test`: all three real decisions re-pointed; standby configuration remains checked in the handler; not weakened.
- `standby_latch_daily_clear_test`: trip-check extraction/copy/mutant source re-pointed only; not weakened.
- `loop_stage_sleep_prep_exclusion_test`: transition count split 16 handler + 2 teardown; the new structural test pins both reasons; no material weakening.
- `sleep_breadcrumb_sequence_test`: cross-file textual check alone is weaker, but the equivalence transcript pins executed breadcrumb 22 before READY/23; no suite-level gap reproduced.

## Findings

### F1 — High — required firmware pre-teardown ordering is no longer regression-pinned

- **Observation:** the guard-after-`SleepTeardown::run()` mutation survived all 93 tests. The moved call can return WAIT after issuing cloud/radio teardown, so the relocated guard is not reached on that pass.
- **Inference:** a future refactor can lose an in-flight firmware update before transitioning to `FIRMWARE_UPDATE_STATE`, while both re-pointed firmware tests report success. Make `SleepTeardown::run` the structural ordering boundary and have the behavior harness preserve/exercise the real source order. This failed required check makes the verdict NOT VERIFIED; it is a test defect, not an observed defect in commit `658b7cf`'s current ordering.

### F2 — Advisory — transcript cannot pin the dead timeout arm

- **Observation:** no transcript pass has nonzero `lastGateProgressMs` or transition reason `sleep-precondition-timeout`; dropping that arm's alert 15 survives as declared. The three final-gate request log variants do execute.
- **Inference:** retaining the dead base branch is behavior-neutral for S-1, but its internals have no runtime equivalence evidence. The architect has already designated it for post-split removal.

## Independent mutations

- Removed reachable primary-wait `serviceAwakeWatchdog()` at `SleepTeardown.cpp:222`: caught by transcript watchdog read-count differences.
- Moved the firmware guard below `SleepTeardown::run()`: survived 93/93 and establishes F1.

## Budget versus actual

`State_Sleep.cpp` is 1670 → 1331 lines (`+9/−348`); the new module/header are 352+7 lines. Across `src/`, moved lines cancel and the non-move net is **+20 all lines / +18 nonblank**, meeting but exhausting the hard budget. Tests add 793 lines of new harness/stubs (4.53× guideline), a 4,753-line fixture, and net +16 in existing tests; accepted concern, not a budget failure.

## Confirmations

- Main review worktree source/tests remain byte-identical to `658b7cf`; only this authorized verdict is changed. The fixture and golden payload remain unchanged.
- All builds, generated files, regeneration and mutations were confined to review-created scratch under `build-tmp/wo20261020-003-stage7/` or `$TMPDIR`; only those review-created paths were removed.
- No commit, push, stash, reset, checkout/restore, device, network or AWS access occurred.
