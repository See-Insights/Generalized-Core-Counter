# WO-2026-11-11-001 Stage 6 report (Copilot)

**Model:** claude-sonnet-5.5. **Reasoning level:** dispatched at medium; I could not observe the level actually run.
**Interpreter line:** `zsh 5.9 (arm64-apple-darwin26.0)`, `Python 3.14.6`, `Apple clang version 14.0.3`; Device OS 6.6.0 (`project.properties`), `arm-none-eabi-gcc` 10.2.1.
**Base:** branch HEAD `d27d090`; `src/` identical to `f00aa04`. **Nothing committed.**

## Measured baseline (before editing)
CI discovery in this worktree: **118/118 (sh via zsh, py via python3)**: 68 `.sh` + 50 `.py`, counted with `p=$((p+1))`.

## Step 0 (read-only, on `f00aa04` lines, before my edits)
- **(a) Re-cite.**
  1. `case 15:` list in `isAutoClearAfterReportAlert()`: `cloud/ReportPublisher.cpp:25–37` (switch at :26–36; 18 and 17 absent).
  2. Connect clear: `state/State_Connect.cpp:558–565` (the `if (...get_alertCode() == 17 && configOk && ledgerPublishOk)` block at :560–564, comment at :558–559). The WO's `:560–564` is the block, not the comment.
  3. PMIC no-charge-fault branch: the "charging healthy" clear is `PmicFaultMonitor.cpp:363–379`, gated by `consecutiveFaults > 0` (`:363`); 21's narrow clear is `:383–389`; the new predicates go after it (after the 21 clear, still inside the no-charge-fault `else`). `ntcFaultCode` (`:198`) and `regs.faultReg` are in scope. 20 is raised at `:201` (NTC), `:220`; 23 at `:233` and `:416` (BAT_FAULT).
  4. 6 h trigger: `trackAndReport()` `:449`; `raiseAlert(21)` at `:549`; ambiguous-load branch `:539–545`.
- **(b) `origin/integration/v50-ab1805` (`1d1d5af`):** `git diff f00aa04 origin/integration/v50-ab1805 -- <the three files>` is **empty**. It changes none of those lines.
- **(c) Readers of 17 and `lastAlertTime`.**
  - Alert 17: written `Generalized-Core-Counter.cpp:554` (`set_alertCode(17)`, with `set_lastAlertTime(Time.now())` at :560); severity table `MyPersistentData.cpp:778`; `State_Error.cpp:99` (log only, "no additional action"); the connect clear `State_Connect.cpp:560`. **No other code, comment or test reads "17 cleared at connect".** Docs: only `docs/reference/alert-codes.md` (rows 17, masking), `docs/html/*` (generated, stale), `CHANGELOG.md:458` (history).
  - `lastAlertTime`: reader `State_Report.cpp:144` (`lastEscalation`, alert 40 only, not 17); status event `Generalized-Core-Counter.cpp:1297`; `raiseAlert` writes it (`MyPersistentData.cpp:829`). Nothing assumes 17 is gone before the report.
  - `configOk` is still read (`State_Connect.cpp:536–537`, alert 41 branch). **`ledgerPublishOk` was read only by the deleted block**, so it became write-only; I removed it (2 lines, see deviations).
- **(d) `vcell` at the stuck-charging check:** it is a parameter of `trackAndReport(const Registers&, int, float soc, float vcell, float finalAcceptedSoc)` (`:449`), `float`, **volts** (same variable is logged `%.3f` and compared with `4.05f` at `:396` in the latch). In scope at `:549`. **No answer breaks the plan.**

## Failing-before evidence (new tests against `f00aa04` `src/`)
| Test | Result on `f00aa04` src |
|---|---|
| `alert19_clears_after_report_test.sh` `testThrashReportedOnceThenClears` (18 stays until queued, then clears; re-raise reports again) | `Assertion failed: g_alertCode == 0`, line 182 |
| same file, 17 / tier-3-18 tests (`testBootStormReportedOnceThenClears`, `testTierThreeThrashDeliveredAfterReboot`) | same `case` omission; not reached (18 test aborts first). The new connect-clear grep (`get_alertCode() == 17` in `State_Connect.cpp`) matches on `f00aa04`, so the script fails there too |
| `pmic_fault_monitor_test` scenario 11 (20 from NTC only) | `Assertion failed ... testAlert20FromNtcAloneClearsWhenNtcClears` line 533 |
| scenario 12 (23 from BAT_FAULT only) | `Assertion failed ... testAlert23FromBatFaultAloneClearsWhenBatFaultClears` line 552 |
| scenario 13 (21 at 4.04 fires, 4.05 and 4.16 do not) | `Assertion failed: !stuckFastChargeFires(4.05f)` line 573 (4.04 fires on both) |
| `report_payload_golden_test` (4 new `key1_bs{1,3,4,5}` vectors) | `FAIL: 190 vectors, fixture has 186` (before the fixture was extended); the strings themselves are recorded from the unchanged source |
| `watchdog_alert_code_test.sh` new fidelity check (mirror lists vs source tables) | `FIDELITY CHECK FAILED: mirror auto-clear list differs` (mirror has 17/18, src lacked them). The tier-3 list check was added with 22 in the mirror |

Real functions used: severity table, `raiseAlert`, `isAutoClearAfterReportAlert` and the clear block are extracted verbatim from source (existing `alert19` pattern); PMIC tests drive the real `pollAndRemediate()` / `trackAndReport()`. The "tier-3 reset path" test models the persisted alert code (nothing at boot clears it; first report after reboot carries and clears 18).

## Tests changed (all under `tests/`)
- `alert19_clears_after_report_test.sh`: rewrote `testThrashStaysSetAfterReport`; added 17 and tier-3/after-reboot tests; added the connect-clear grep guard.
- `pmic_fault_monitor_test.cpp/.sh`: scenarios 11, 12, 13.
- `watchdog_alert_code_test.cpp/.sh`: mirror now has 22 in tier 3 and 17/18 in the auto-clear list; `.sh` now compares the mirror's tier-3 and auto-clear case lists to the source (the mirror was not checked before).
- `report_payload_golden_test.cpp` + `fixtures/report_payload_golden.txt` (+4 lines, recorded from `--dump`) + one `key1` mutant in the `.sh`: all seven `key1` strings are now pinned (0, 2, 6 existed; 1, 3, 4, 5 added).

## `src/` net lines against `f00aa04` (`git diff --numstat`)
| File | +/− | Net |
|---|---|---|
| `cloud/ReportPublisher.cpp` | +2 / −0 | **+2** |
| `power/PmicFaultMonitor.cpp` | +8 / −2 | **+6** |
| `power/PmicFaultMonitor.h` (`kPmicFullPackVcell`) | +1 / −0 | **+1** |
| `state/State_Connect.cpp` | +0 / −10 | **−10** |
| **Total** | | **−1** |

Budget ≤ +10: met. Estimate +2: beaten by 3 (the deleted block and unused flag are larger than the WO's "about −5").
Item 6 (P4): `git diff f00aa04 -- src/state/State_Error.cpp` is empty; `resolveErrorAction()` for 15, 16, 31, 40 untouched.

## Mutations (backed up to `build-tmp/.../bak/`, restored with `cp`, `git diff` byte-checked)
| Mutation | Result |
|---|---|
| drop `case 18` | CAUGHT (alert19 test) |
| drop `case 17` (connect clear deleted) | CAUGHT (alert19 test) |
| restore a connect clear for 17 | CAUGHT (alert19 grep guard) |
| drop the 20 predicate | CAUGHT (scenario 11) |
| drop the 23 predicate | CAUGHT (scenario 12) |
| `>` in place of `>=` at 4.05 V | CAUGHT (scenario 13) |
| drop one `key1` vector (fixture line removed) | CAUGHT (golden count). The in-script mutant `Discharging`→`Discharge` is also caught |

## Verification
- Suite after edits: **118/118 (sh via zsh, py via python3)** (same file count as baseline; no new helper files). The first run showed 117/118 only because my scratch `old/src` copy confused `thermal_coupling_structural_test.py`; I deleted the copy and it passes.
- Build (Device OS 6.6.0, boron, fresh `BUILD_PATH_BASE` and `TARGET_DIR` as make arguments, `make -C .../deviceOS/6.6.0/modules/boron/user-part`); base built from `git archive f00aa04`:

| | base `f00aa04` | this change | delta |
|---|---|---|---|
| text | 153108 | 153164 | **+56** |
| data | 1114 | 1114 | 0 |
| bss | 2240 | 2240 | 0 |

  `nm -S` moved: `trackAndReport` 0x498→0x4c8 (+48), `pollAndRemediate` 0x66c→0x68c (+32), `handleConnectingState` 0x910→0x8cc (−68), one `CSWTCH` renumbered. `strings` on the new `.bin`: "full pack - alert 21 skipped" ×1; "Boot-storm recovery" ×0. Nothing claimed about hardware.

## `docs/reference/alert-codes.md`
17 and 18 are report-once (type, Clears, list at "Report", ERROR_STATE note, masking, difference note 9); 20 and 23 clears replace the "Known gap" text; 21 carries the `vcell` ≥ 4.05 V exclusion. `RP:`, `P:` and `C:` citations re-cited for the shifts (RP +2 from :28, P +4 from :390 and +6 from :541, C −1/−2/−10); the "historical" section was left as is. Source header notes the WO is applied.

## Deviations
1. Removed `ledgerPublishOk` (declaration and one write): after deleting the block it was write-only (unused-but-set warning risk). The dispatch said to keep it only if other code uses it; none does.
2. Alert 21 skip: when `vcell >= kPmicFullPackVcell` the branch joins the existing "ambiguous load" path (a `Log.warn` saying the alert was skipped for a full pack, and the 6 h window restarts), so the warning does not repeat every sample. The WO said only "skip"; the restart is my choice.
3. The 20/23 predicates sit inside the no-CHRG-fault `else`, so they run only when CHRG_FAULT reads clear; a charge fault still goes through the existing clear.
4. The tests' "re-raise after clear" for 17 is a direct `set_alertCode(17)` (that is how G:554 raises it).

## Open questions
- `CHANGELOG.md:116` still says 18 "stays sticky" (a historical release entry, outside my authorization); Claude Code may want a new entry for this change.
- `docs/html/` generated pages still show the old connect clear (stale before this change too).
- The Ubidots "boot storm" event can now see 17 in a report; that is Chip's checklist half.
