UNDER REVIEW: wo/2026-11-11-001-alert-reporting-p @ 007504cb1330fd5e693e1f9acd1658196ebca6c6

# NOT VERIFIED

HEAD matched the full SHA before and after review. The tracked worktree remained unchanged.

| # | Result | Evidence and judgment |
|---|---|---|
| 1 | PASS | `ReportPublisher.cpp:25-39` adds only cases 17/18. `State_Connect.cpp:547-555` removes the connect clear and now-unused `ledgerPublishOk`; `configOk` remains used at `:536-537`. No live source still clears 17 at connect. |
| 2 | FAIL | Two reachable alert-20 lifecycle paths clear while the condition/recovery is still active; findings below. NTC/BAT-only clear paths themselves work. |
| 3 | PASS | `vcell` is volts. `PmicFaultMonitor.cpp:543-551` uses the specified `>= 4.05f`, warns, and restarts the six-hour window. Thus 4.04 fires; 4.05 and 4.16 do not. A pack held high warns at most once per six hours. A genuine high-voltage stuck charge can be hidden, as implied by the binding ruling. |
| 4 | FAIL | Required tests and mutations pass, but scenarios 11–13 omit both failing mixed-origin/remediation paths. The PMIC harness links the real implementation; the report-alert test extracts real functions; watchdog mirrors are source-compared; golden extraction requires unique markers. |
| 5 | PASS | Rows 17, 18, 20, 21 and 23, masking, and ERROR_STATE descriptions accurately cite the implemented behavior. The underlying alert-20 behavior is defective, but row 20 does disclose the broad “charging resumed” clear. |
| 6 | PASS | `src/` net −1, within ≤+10. Fresh Device OS 6.6.0 Boron builds reproduced 153108/1114/2240 → 153164/1114/2240. |
| 7 | PASS | v50 changes none of the reviewed source files relative to `f00aa04`; no textual conflict. Its later severity-4 inferred watchdog 19 supersedes an earlier boot-storm 17 as intended. |
| 8 | PASS | No additional regression beyond item 2. Repeated tier-2 18 or later boot-storm 17 incidents can appear on successive reports after each accepted clear; that is the bound “real information” behavior. |

## Findings

1. **High — `src/power/PmicFaultMonitor.cpp:343-393`.**  
   Path: three thermal-shutdown `CHRG_FAULT=0x02` polls raise 20, arm level-1 remediation, and execute phase 0 (`disableCharging`). On the first clear-CHRG poll before re-enable, `remediationInProgress` remains true, yet the new `currentAlert == 20 && ntcFaultCode == 0` predicate clears 20. This contradicts `:356-360`, which requires completing remediation and one further clean poll. A scheduled report can therefore miss the thermal alert.

   Smallest fix: require `consecutiveFaults == 0` for the new origin-specific 20/23 predicate. Add a real-function scenario covering thermal 20 through an incomplete and completed remediation cycle.

2. **Medium — `src/power/PmicFaultMonitor.cpp:198-201,363-379`.**  
   Path: a combined thermal CHRG+NTC poll raises 20 and increments `consecutiveFaults`; the following NTC-only poll still raises/holds 20 at `:201`, but the existing broad 20–23 “charging resumed” block clears it later in the same call. The poll ends with alert 0 while NTC is active, so that report can miss it.

   Smallest fix: prevent the broad range clear from clearing 20 while `ntcFaultCode != 0`. Add a combined-CHRG+NTC → NTC-only scenario. The two proposed scratch guards passed all 15 PMIC scenarios.

`ntcFaultCode` is derived from the current poll’s OR-combined `regs.faultReg` at `PmicFaultMonitor.cpp:198`. Active BAT_FAULT prevents the new 23 predicate and is raised again at `:418-420`. Alert 22 does not match the new predicate. `persistentAfterImmediateReset` remains in the CHRG-fault branch, where the new predicate does not run. The `P:233` default path is currently unreachable because a nonzero two-bit charge-fault field yields 1, 2, or 3, all explicitly handled.

## Alert 17/18 lifecycle walk

- Boot-storm pending sets 17 directly at `Generalized-Core-Counter.cpp:550-562`.
- Reporting builds and queues the payload at `State_Report.cpp:67-70`, before a following connect. `ReportPublisher.cpp:76` snapshots the code before queueing; `:145-154` clears only after acceptance and only if the held code still matches.
- Queue rejection retains 17/18.
- ERROR_STATE performs no action for 17/18 (`State_Error.cpp:96-106`); the low-power clear covers only 15/16/31.
- A second boot while 17 is held either preserves it or directly writes 17 again.
- 17 then severity-gated 19 leaves 19. Direct 17 after a held 19 overwrites 19, matching the documented direct-write semantics.
- Tier-3 18 persists across reset; the test models the persisted scalar, although it does not execute the real ThrashGuard/reset machinery.

## Mutation results

| Scratch mutation | Result |
|---|---|
| Drop `case 18` | CAUGHT — 18 remained set |
| Drop `case 17` | CAUGHT — 17 remained set |
| Restore connect clear for 17 | CAUGHT — grep guard |
| Drop alert-20 predicate | CAUGHT — scenario 11 |
| Drop alert-23 predicate | CAUGHT — scenario 12 |
| Change `>=` to `>` at 4.05 V | CAUGHT — scenario 13 |
| Drop one `key1` fixture vector | CAUGHT — 190 vectors versus 189 |
| Own: allow NTC predicate to clear 22 | CAUGHT — scenario 7 found 22 cleared while Not Charging |

Additional real-function reproductions:

- Thermal-20/in-progress-remediation: current source failed.
- Combined CHRG+NTC → NTC-only: current source failed.
- Scratch implementation of both smallest guards: scenarios 1–15 passed.

## Documentation spot-check

Statement-content checks passed for RP:25–39, RP:145–154, G:398, G:554, T:141, T:150, P:201, P:220, P:390–393, P:420, P:543–555, M:778–782, M:795, M:817, E:45–55, E:57–79, E:81–106 and E:136–140.

## Budget, build and binary inspection

`src/`: `ReportPublisher.cpp` +2; `PmicFaultMonitor.cpp` +8/−2; `PmicFaultMonitor.h` +1; `State_Connect.cpp` −10; total **+11/−12, net −1**.

| Build | text | data | bss |
|---|---:|---:|---:|
| `f00aa04` | 153108 | 1114 | 2240 |
| Reviewed SHA | 153164 | 1114 | 2240 |
| Delta | +56 | 0 | 0 |

`nm -C -S`:

- `trackAndReport`: `0x498 → 0x4c8` (+48)
- `pollAndRemediate`: `0x66c → 0x68c` (+32)
- `handleConnectingState`: `0x910 → 0x8cc` (−68)
- compiler-local `CSWTCH.108 → CSWTCH.106`, both `0x14`
- `.text`: `0x255a4 → 0x255dc`; changed log literals account for the remaining read-only-text movement.

Integration paragraph: `origin/integration/v50-ab1805` resolved to `1d1d5af7a22dc47094a4c23b515b348bd8ae153d`. Its diff from `f00aa04` in `ReportPublisher.cpp`, `PmicFaultMonitor.cpp/.h`, and `State_Connect.cpp` is empty. Its inferred PIN_RESET watchdog 19 runs after boot-storm 17 and supersedes it through severity 4; ChurchSt-style repeated 19 incidents remain digest-only cloud behavior.

**Model/reasoning:** gpt-5.6-sol, high.  
**Interpreter:** CI discovery inside this worktree, **118/118 (68 sh via zsh, 50 py via python3)**; zsh 5.9, Python 3.14.6.  
**Toolchain:** Device OS 6.6.0, Boron, ARM GCC 10.2.1, fresh separate `BUILD_PATH_BASE` directories.

Round 1 of at most 2; failure goes to the architect.