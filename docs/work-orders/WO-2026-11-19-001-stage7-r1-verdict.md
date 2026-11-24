UNDER REVIEW: wo/2026-11-19-001-connect-decision @ 230cce7f71379bbcafe86c069d4130987d67542c
0 findings (src), 0 findings (tests), 1 Low (report)

**VERIFIED WITH FIXES**

The implementation passes the functional, equivalence, mutation and linkage checks. The build succeeds, but its two inherited application warnings contradict the Stage 6 report’s warning-free claim.

| # | Result | Evidence |
|---|---|---|
| 1. Equivalence | PASS | Read the old chain at `eb398b8:src/state/State_Report.cpp:220` against `src/power/ConnectDecision.cpp:15`. Independently compiled the exact old chain: 768 no-failure comparisons passed. Row results below. |
| 2. Order | PASS | Preserved blocks are byte-identical: R0 at `State_Report.cpp:145`, R1 at `:164`, connected guard at `:168`, battery commit at `:184`, sleep-return flag at `:202`, and R1b at `:223`. No computation crosses a kept exit. Policy resolution remains before the battery commit; effective mode is read afterward. |
| 3. Hold | PASS | `ConnectDecision.cpp:6` produces 1/2/4/6 hours, then 6 hours through count 255. At most three shifts occur; the largest intermediate is 28,800,000 ms. `State_Report.cpp:214` uses unsigned subtraction. Independent zero-timestamp and wrap-boundary checks passed. |
| 4. Failure facts | PASS | Sole runtime stamp write: `State_Connect.cpp:642`, immediately after the sole timeout caller at `:641`. Sole counter increment: `ModemHealth.cpp:57`, saturating at 255. Reset details below. |
| 5. Daily clear | PASS | `ModemHealth.cpp:72` clears the count unconditionally. Remaining function behavior and callers are unchanged; Report still calls it at `State_Report.cpp:82`, before deciding. |
| 6. Retirements | PASS | No deleted-field or defer-constant references remain in `src/` or `lib/`. Remaining test references are deliberate compatibility, absence-check and mutation code. `SleepTeardown.cpp:240` retains the slow-teardown latch; standby suppression is unchanged. |
| 7. Test edits | PASS | All ten edits are limited to the approved changes. Fixture transformations independently verified. All five Sleep structural tests rejected both unintended-edit probes; details below. |
| 8. Budget/build | FAIL — warning-free clause | Budget passes: **+88/−88, net 0**. Fresh Boron build succeeds; sizes and linkage pass. Two unchanged application files emit warnings, so literal “no `src/` warning” does not pass. |
| 9. Suite/mutations | PASS | **125/125**; no historical comparisons skipped. All ten requested decision/wiring mutations and ten Sleep structural mutations caught. No compilation or extraction failure was counted as detection. |

Row equivalence, with `failureCount = 0`:

| Row | Result and reason | Check |
|---|---|---|
| R2 | CONNECT `service request`, first priority | Reading and independent old-chain comparison; `ConnectDecision.cpp:16`. |
| R3 | CONNECT `occupancy change`, after R2 | Same; `:17`. |
| R4 | CONNECT `daily close`, before webhook/keep-alive/cadence | Same; `:18`. |
| R5 | CONNECT `webhook health check` | Same; `:19`. |
| R6 | Former IDLE `modem unstable` removed intentionally | Diff inspection; approved deletion. |
| R7 | CONNECT `keep alive mode`, before cadence | Reading and independent comparison; `:20`. |
| R8 | Former IDLE `modem unstable aligned` removed intentionally | Diff inspection; approved deletion. |
| R9 | CONNECT `boundary aligned` | Reading and independent comparison; `:21`. |
| R10 | WAIT → IDLE `not aligned` | Reading and independent comparison; `:32`. |

The independent harness also passed **195,840 cases** covering every nonzero `uint8_t` failure count, trigger combination, mode and selected boundary age under UBSan. Within `decide()`, only `serviceRequest` bypasses the hold. Daily close clears the counter before calling it; `closeDue` itself is not exempt.

With a nonzero count and `lastConnectFailMs == 0`, elapsed time is the current unsigned `millis()` value: the hold remains active until its boundary. Zero is not a sentinel. Normal production flow cannot reach a reporting decision with a positive count that has never been stamped: timeout increments and stamps consecutively. A timestamp legitimately equal to zero at wrap is handled correctly.

The count clears on success (`ModemHealth.cpp:67`), qualifying successful teardown recovery (`:47`), every daily close (`:72`), and boot/reset initialization (`StateMachine.h:64`; global session at `Generalized-Core-Counter.cpp:152`). Internal recovery stages do not increment it.

Whole-tree searches run:

```text
rg -n 'lastConnectFailMs|modemConnectTimeoutCount|noteConnectTimeout|noteConnectSuccess|clearAtDailyClose|maybeClearModemUnstable' src
rg -n 'lastTeardownEndMs|lastCloudDisconnectElapsedMs|lastModemOffElapsedMs|lastTotalTeardownElapsedMs|MODEM_UNSTABLE_RECONNECT_DEFER_MS' src lib tests
git log --all --oneline -S MODEM_UNSTABLE_RECONNECT_DEFER_MS -- src
```

History places the defer’s introduction at `c254467`, as modem-instability containment. Its only decision consumers were R6/R8. No other application path depended on that delay; slow-teardown detection, modem-off checks and standby suppression remain.

The ten existing test edits:

| Test group | Result | Scope audit |
|---|---|---|
| `modem_health_equivalence_test.sh` + fixture | PASS | `:65`: adjusted reset mutation anchors and added conditional-clear detection. Exactly six fixture outcomes changed, solely `count=2` → `count=0`. |
| `daily_cleanup_boundary_test.py` | PASS | `:237`: follows `due` into `closeDue` and checks relocated priority. Real CONNECT behavior is exercised by the decision test. |
| `occupancy_report_by_mode_test.{sh,cpp}` | PASS | Shell `:280`: extracts and links the real decision instead of modelling the chain tail. Expectations unchanged; all 23 mutations caught. |
| `sleep_cloud_gate_structural_test.py` | PASS | `:205`: only the two new files and four deleted writes allowed; both stray probes rejected. |
| `sleep_execution_structural_test.py` | PASS | `:145`: same narrow allowance; both probes rejected. |
| `sleep_plan_structural_test.py` | PASS | `:213`: same; both probes rejected. |
| `sleep_preflight_structural_test.py` | PASS | `:249`: same; both probes rejected. |
| `sleep_wake_structural_test.py` | PASS | `:178`: same; both probes rejected. |
| `sleep_teardown_equivalence_test.{sh,cpp}` + fixture/stub | PASS | C++ `:54`: base-only compatibility fields remove precisely the retired observations. Independent comparison confirms 1,065 removed `sess=` columns and one fewer `millis()` read on 288 completion passes; everything else matches. |
| `standby_latch_daily_clear_test.{sh,cpp}` | PASS | C++ `:120`: unlatched count 1 must now clear. Removed-field assertions alone were deleted; conditional-clear regression is caught. |

`connect_hold_base_delta.py:13` normalizes exactly the four contiguous deleted writes, once, in **base text only**. Its separate `:11` allowlist contains exactly the two new decision files. No assertion was weakened beyond the WO’s deltas.

Fresh build: Boron, Device OS 6.6.0, ARM GCC 10.2.1; new `BUILD_PATH_BASE=<worktree>/build-tmp/wo-2026-11-19-001-stage7-r1/device-os-build`.

| Image | text | data | bss |
|---|---:|---:|---:|
| Recorded base build | 151124 | 1114 | 2264 |
| Independently built candidate | 151116 | 1114 | 2248 |
| Delta | **−8** | **0** | **−16** |

`nm` finds `ConnectDecision::decide` at `0x000bdfe8` and `holdMs` at `0x000bdfb4`. Disassembly confirms the call from `handleReportingState()` at `0x000c34b6`. All 303 tracked build inputs matched their copies.

**Finding — Low, report:** `docs/work-orders/WO-2026-11-19-001-stage6-copilot-report.md:74`.

The warning-free evidence claim is not reproducible with the prescribed build. It emits `CONNECTIVITY_FAILSAFE_TEST_MODE=0` at `src/diagnostics/ConnectivityFailsafeTest.cpp:18` and undefined `MUON_HAS_TMP112` at `src/sensors/SensorManager.cpp:978`. Both files are byte-identical to the base. No modified application file emits a warning.

Smallest fix: correct the report to enumerate these inherited warnings and explicitly resolve the baseline exception to item 8’s zero-warning clause. This is an evidence correction, not a connect-logic defect.

| Independent mutation | Classification | Detection |
|---|---|---|
| Hold removed | Accidental deletion | `connect_decision_test.sh`: held occupancy checks. |
| Doubling removed | Accidental arithmetic regression | Same: failure 2 must hold 2 h. |
| Cap removed | Accidental arithmetic regression | Same: failure 4 must hold 6 h. |
| Capped calculation wraps through a `uint8_t` multiplier | Accidental narrowing regression | Same: failure 9 and subsequent sequence checks. |
| `occupancyChange` exempted | Accidental exemption | Same: occupancy must wait. |
| `closeDue` exempted | Accidental exemption | Same: daily close must wait. |
| Failure stamp also written on success | Accidental extra writer | `connect_hold_wiring_test.sh`: structural single-writer check. |
| Daily clear conditional again | Accidental reset regression | `standby_latch_daily_clear_test.sh`: `unlatched:countCleared`. |
| Keep-alive above daily close | Accidental rule reorder | `connect_decision_test.sh`: R4 KEEP_ALIVE characterization. |
| R1b moved before decision as an early exit | Accidental refactor | `connect_hold_wiring_test.sh`: structural order check. |
| Stray teardown threshold edit | Accidental scope drift | All five Sleep structural tests. |
| Stray new `src/Stage7Stray.cpp` | Accidental scope drift | All five Sleep structural tests. |

**Documented limits:** Nothing was flashed or bench-tested. RAM/reset behavior, Device OS registration backoff exclusions and constructed tooling evasions remain as specified. Step 0’s accepted ULP clock evidence was not remeasured.

**Known gaps:** Only the candidate was rebuilt, within authorization. Base sizes come from the recorded builds. `eb398b8:src` and the previously built `99ece4e…:src` both equal `e3b0b86edb5d94c4b54b9f990694bfde2cd12eab`; their `lib/` and `project.properties` also match.

Model/reasoning: `gpt-6-astra` / `xhigh`, as dispatched; no subagents.
Interpreters: **125/125 (sh via zsh 5.9, py via python3 3.15.2, without `-I`)**; host Apple Clang 14.0.3.

HEAD and tracked files remain unchanged. Task-owned temporary copies and objects were removed; evidence and the symbolized ELF remain in `build-tmp/wo-2026-11-19-001-stage7-r1/`.