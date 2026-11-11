UNDER REVIEW: wo/2026-11-11-001-alert-reporting-p @ 467eea70e9e9ff9572c041305d9168fbc8e528e2

# PASS

HEAD matched the full SHA before and after review. Tracked files remained unchanged.

| # | Result | Evidence and judgment |
|---|---|---|
| 1 | PASS | Guard 2a at `src/power/PmicFaultMonitor.cpp:390-393` holds thermal alert 20 through the first clean poll, remediation completion, and until the following clean poll. Scenario 14 executes the real function and passes. A stale nonzero counter with no CHRG fault and no running cycle cannot stick 20: `:363-380` zeroes the counter, then either clears 20 or preserves it only while NTC remains active. |
| 2 | PASS | Guard 2b at `PmicFaultMonitor.cpp:376-380` preserves 20 on combined CHRG+NTC → NTC-only and clears it once NTC disappears. `ntcFaultCode` is from the current OR-combined register read at `:198`; after an immediate reset it reflects the refreshed read. Thermal 20 with coincident NTC clears correctly after NTC and any remediation state resolve. The new condition is restricted to `currentAlert == 20`; behavior for 21, 22 and 23 is unchanged. |
| 3 | PASS | Scenarios 14–15 at `tests/pmic_fault_monitor_test.cpp:559-605`, invoked at `tests/pmic_fault_monitor_test.sh:32-33`, link and execute production `pollAndRemediate()`. Their alert and PMIC-call assertions prevent wrong-reason passes. Both required guard-removal mutations fail the intended scenario; all required round-1 mutations are caught. A low coverage note is recorded below. |
| 4 | PASS | `docs/reference/alert-codes.md:39-42` matches the normal source lifecycle. Spot-checked P:201, P:220, P:233, P:376–378, P:390–393, P:420, P:543–555, M:780–782 and M:795. No cited source statement moved in round 2. Row 23 was untouched and remains accurate. |
| 5 | PASS | Round 2 is +2/−2, net 0, versus `007504c`; total versus `f00aa04` is +12/−13, net −1, within the ≤+10 net budget. Fresh Device OS 6.6.0 Boron build produced 153172/1114/2240. `nm -C -S` gives `pollAndRemediate` size `0x698`. |
| 6 | PASS | No further in-scope regression in the two guards. One long-duration lifecycle defect outside the two lines is recorded as a known gap under Amendment 1, not as a round-2 failure. |

## In-scope findings

None.

## Known gap for the architect’s closing record

**Low — `src/power/PmicFaultMonitor.cpp:108,208,343-393`.** `consecutiveFaults` is a `uint8_t`. A thermal cycle can start, become frozen while `safeToCharge == false`, and remain under a persistent thermal CHRG fault for another 253 polls, wrapping the counter from 3 to 0. If CHRG then clears while the cycle remains frozen, guard 2a sees zero and clears 20 before charging is re-enabled. A scratch real-function reproduction confirmed this path.

This is outside the two amended guard lines and therefore a known gap, not a failure. Smallest future fix: saturate the increment at `:208`, or additionally require `!remediationInProgress` in guard 2a. Record the limitation in `alert-codes.md` and the WO closing record.

**Low test-coverage note — `tests/pmic_fault_monitor_test.cpp:595-603`.** Scenario 15 uses only NTC code `0x01`; a mutant protecting only `ntcFaultCode == 1` survived all 15 scenarios. The required guard-removal mutation is caught, so this does not invalidate Amendment 1, but parameterizing representative nonzero NTC codes would close the gap.

## Mutation results

| Scratch mutation | Result |
|---|---|
| Remove guard 2a | CAUGHT — scenario 14, assertion at test line 577 |
| Remove guard 2b | CAUGHT — scenario 15, assertion at line 601 |
| Drop alert-20 predicate | CAUGHT — scenarios 11 and 15 |
| Drop alert-23 predicate | CAUGHT — scenario 12 |
| Change `>=` to `>` at 4.05 V | CAUGHT — scenario 13 |
| Own: narrow guard 2b from nonzero NTC to `== 1` | SURVIVED — coverage note above |
| Own lifecycle reproduction: wrap `consecutiveFaults` during a frozen cycle | REPRODUCED on current source |

## Build and execution record

| Revision | text | data | bss | Delta from reviewed SHA |
|---|---:|---:|---:|---:|
| `f00aa04` | 153108 | 1114 | 2240 | reviewed +64/0/0 |
| `007504c` | 153164 | 1114 | 2240 | reviewed +8/0/0 |
| `467eea70` fresh build | **153172** | **1114** | **2240** | — |

`pollAndRemediate`: `0x68c` at round 1 → **`0x698`** (+12 bytes).

**Interpreter:** CI discovery inside this worktree, **118/118 (68 sh via zsh, 50 py via python3)**.  
**Model/reasoning:** gpt-5.6-sol, high.  
**Toolchain:** Device OS 6.6.0, Boron, ARM GCC 10.2.1, fresh `BUILD_PATH_BASE`.

Round 2 of 2, the last.