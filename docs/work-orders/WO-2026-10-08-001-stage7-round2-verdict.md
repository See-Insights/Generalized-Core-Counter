# NOT VERIFIED

Model: `gpt-5.6-sol` · Reasoning: high  
Tests: **72/72 (sh via zsh, py via python3)**

The implementation, tests, mutations, and ARM linkage pass. Verification stops because the mandated move introduces a new medium-severity heap-churn edge: the cadence return now occurs after a device-ID `String` allocation/free on every eligible loop pass.

| Check | Result | Evidence |
|---|---|---|
| 1.1 Linkage | **PASS** | Release ELF supervisor calls `resolveRuntime()` at `0xb6622`; Report calls `Time.now()` at `0xc2420` then `set_lastConnection()` at `0xc2426`; `applyTimingConfig()` loads `65535` at `0xbb762`. |
| 1.2 Builds | **PASS** | Fresh Boron/6.4.1 release: **151028/1090/2196**, versus round 1 **151020/1090/2196**: **+8/0/0**. Test mode: **152548/1090/2196**. |
| 1.3 Tests | **PASS** | Full host suite **72/72**, run before creating any `src/` scratch copy. |
| 1.4 Binary | **PASS** | Release supervisor is `0x298` bytes. Test-mode supervisor is `0x224` and contains zero `resolveRuntime()` calls. |
| 1.5 Alerts | **PASS** | Alert ranking and clearing are unchanged. Alert 40’s widened reach remains architect-approved. Alert 41 is invoked on each connect-time apply while an over-range interval remains; deferred `Cloud::loop()` apply still only records failure. The CHANGELOG accurately describes this. `docs/reference/alert-codes.md` needs no update. |
| 1.6 Cadence rule | **CONCERN** | Formula is exactly 3 h for cadence below 3 h, otherwise cadence + 3 h: 4 h escalates at 7 h. Both sites use `resolveRuntime()`. However, cadence history is not retained; see Finding 2. |
| 1.7 CONNECTED fix | **CONCERN** | Refresh is confined to Report’s `Particle.connected()` branch. Normal KEEP_ALIVE/INTERMITTENT wake behavior is unchanged because the sleep gate waits for disconnection. Rare Report re-entry while still online can refresh an already-recent timestamp, as reported in round 1. |
| 1.8 Interval cap | **PASS** | 65535 is accepted; 65536/86400 are rejected. The rejected field retains its prior value—no default and no half-applied interval field. Other valid fields applying independently is the architect-accepted existing behavior. |
| 1.9 Tests/mutations | **PASS** | Cadence test dynamically extracts the production block and contains the requested loud `COPY_MISMATCH` check; it does not reimplement `STALE + cadence`. All mutations were caught. |
| 1.10 Budget | **PASS** | WO total is **+9 net non-comment `src/` lines**, below +20. |
| 2 Finding 3 move | **FAIL** | Block SHA-256 matches round 1 exactly: `9052329…ee19`. Position is after stage/cooldown and before `activeConnectAttemptWithinBudget()`. Release order is stage getter `0xb656e`, cooldown branch `0xb6610`, `resolveRuntime` `0xb6622`, connect-attempt check `0xb6644`. The crossed jitter computation has a resource side effect; see Finding 1. |
| 3 Finding 2 test | **PASS** | Formula mutation makes the 4 h behavior fail at 5 h, 6 h 59 m, and one second before 7 h. The retyped stale gate weakens direct behavioral coupling only for that gate; exact source-location/order checks and the added above-stale mutation cover this WO’s interaction. |
| 4 Mutations | **PASS** | Formula, cadence-condition, rule/body removal, whole-block removal, guard removal, move above stage/cooldown, refresh removal, 86400 restoration, and added move-above-stale mutation were all detected. **No survivor.** |
| 5 Builds | **PASS** | Both builds used distinct fresh `BUILD_PATH_BASE` paths. Release contains the cadence rule; test mode does not. |
| 6 Budget | **PASS** | Round 2 changes zero net `src/` lines and moves 13 physical lines—8 code/directive lines. |

`resolveRuntime()` now runs once per supervisor pass after valid-time/open/stale checks, provided stage is below 3 and no nonzero-stage cooldown remains. It is skipped for stage 3 and active cooldowns, but still runs before the active-connect-attempt and low-battery returns.

`lastConnection` reader effects remain as predicted:

- Failsafe and status age now use the latest online report.
- Alert-40 force-connect is suppressed for up to 30 minutes after that refresh.
- Alert-40 “connected recently” can reach the approved six-hour escalation.
- `LedgerClient.cpp:114` can restart its input window on a changed epoch.
- Connect/Sleep observability and test diagnostics see the newer timestamp.

## Findings

1. **Medium — new edge; architect must accept or reject:** `src/Generalized-Core-Counter.cpp:386`, `:2648`.  
   **Observation:** the move crosses `connectivityFailsafeJitterSec()`. Release disassembly calls `spark_deviceID` at `0xb65a6` and `String::~String()` at `0xb65d6`, before `resolveRuntime()` at `0xb6622`. Device OS 6.4.1 implements `spark_deviceID()` through `bytes2hex()`, whose `String::concat()` grows storage via `realloc()`.  
   **Inference:** decision values remain deterministic, but runtime behavior is not side-effect-free. Long-cadence devices between 3 h and their extended threshold now allocate/grow/free a device-ID string on every loop pass. This creates previously absent high-frequency heap traffic and unmeasured loop cost.

2. **Medium — cadence-transition edge; architect must accept or reject:** `src/Generalized-Core-Counter.cpp:2660`, `src/state/State_Report.cpp:171`.  
   **Observation:** the threshold uses only the currently resolved multiplier, which can range from 1× to 12×; it retains no record of the cadence applicable since the last connection. The supervisor runs before Report.  
   **Inference:** if battery recovery shortens cadence—for example 12 h to 1 h—when connection age already exceeds 3 h, the supervisor can reset immediately before Report gets an opportunity under the newly shortened schedule. The specific current-cadence formula is implemented correctly, but this remains an edge against the plain-language “expected connection overdue” goal.

3. **Low — residual test scope:** `tests/failsafe_cadence_rule_test.cpp:88`.  
   **Observation:** the stale-return statement is retyped; the shell driver checks its exact source location and ordering rather than compiling that statement from production.  
   **Inference:** this does not weaken coverage of the cadence formula or block position, but it is not full behavioral execution of the supervisor’s stale gate.

## Budget versus actual

| Record | Budget/expected | Net `src/` code | Physical `src/` | Moved | Tests |
|---|---:|---:|---:|---:|---:|
| Round 1 | WO cap +20 | +9 | +15 | 0 | 688 lines |
| Round 2 | 0 net expected | 0 | 0 net | 13 physical / 8 code-directive | +40 lines |
| WO total | +20 cap; +9 expected | **+9** | **+15** | **13 physical** | **728 lines** |

## Closing integrity

- Branch remains `wo/2026-10-08-001-failsafe-overdue`; HEAD remains `fb9a0fa39c92f78c525e6dec1cbf6ca5e4a8bd6d`.
- Final status exactly matches the start: three modified `src/` files and the same six untracked test files.
- `src/` diff SHA-256 remains `bd6e053265ae9db6a403a34f799d8d5992fc3db4d84795ff59c77b68b516caed`; all six test hashes also match the start.
- Deleted only `build-tmp/wo20261008-001-stage7r2/`; `build-tmp/` and all pre-existing contents remain.