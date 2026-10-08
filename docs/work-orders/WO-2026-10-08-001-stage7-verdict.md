# NOT VERIFIED

Model: `gpt-5.6-sol` · Reasoning: high  
Tests: **72/72 (sh via zsh, py via python3)**

The shipped implementation and ARM linkage are correct, but two round-2 findings block verification: configuration rejection leaves a partial apply, contrary to the architect’s new pre-ruling, and the 4-hour behavioral test does not detect the required formula mutation.

| # | Result | Evidence |
|---|---|---|
| 1 | **PASS** | Release ELF: supervisor `0xb64d0` calls `PowerManager::soc()` and `ReportingPolicyResolver::resolveRuntime()` at `0xb6582`; Report calls `Time.now()` then `SystemConfig::set_lastConnection()` at `0xc241e`. `applyTimingConfig()` loads `65535` and calls real `validateRange<int>()`. |
| 2 | **PASS** | Fresh local Boron/6.4.1 builds. Release: **151020/1090/2196**, versus v39 **150956/1090/2196**: **+64/0/0**. Test mode: **152548/1090/2196**; its supervisor has no `resolveRuntime()` call. |
| 3 | **PASS** | Full suite **72/72**, run before any `src/` copy existed under `build-tmp/`. |
| 4 | **PASS** | Release supervisor size `0x290`; disassembly contains cadence lookup and stale-plus-cadence addition. Test supervisor size `0x224`, with cadence call absent. Report persistence store and 65535 range bound are also present in the linked release ELF. |
| 5 | **CONCERN** | Alert ranking and clearing are unchanged. Alert 40 is only newly reachable as architect-approved. Alert 41 gains a new input path; synchronous Connect invokes it on every connection while the invalid ledger remains, though equal/higher alerts can mask storage. Deferred apply does not raise 41. `docs/reference/alert-codes.md` remains accurate and needs no update. |
| 6 | **CONCERN** | Threshold is exactly `cadence >= STALE ? STALE + cadence : STALE`, mode-independent: short cadence escalates at 3 h; 3 h at 6 h; 4 h at 7 h. Failsafe and Report use the same resolver and interval/tier formula. Report samples the battery after the preceding failsafe pass, so tier can differ by up to 1×–12×; clock skew between calls does not change `effectiveIntervalSec`. A shorter prior cadence already had a due opportunity, so this does not make escalation precede every historically due connection. Cost is bounded/read-only and likely under 100 ms, but was not timed on ARM. |
| 7 | **CONCERN** | Refresh occurs only in Report’s `Particle.connected()` branch. Initial post-wake behavior is unchanged because the sleep gate requires cloud disconnection. Rare re-entry before sleep can reach the branch after a successful Connect—for example, pending KEEP_ALIVE occupancy or a user-switch report—moving an already-recent timestamp slightly. |
| 8 | **FAIL** | 65535 accepted; 65536/86400 rejected. Previously 86400 wrapped to 20864. Other range failures also reject aggregate apply, satisfying pre-ruling condition 1. Condition 2 fails: all sections continue applying valid fields, then results are combined; only the rejected field retains its old value. This is a partial apply. Connect raises alert 41; deferred `Cloud::loop()` only records `lastApplySuccess=false`. **Medium, round 2 required.** |
| 9 | **FAIL** | Copilot’s five mutations were all caught. The independent `STALE + cadence` → `cadence` mutation returned failure only from the structural formula check; every advertised 4-hour behavioral assertion still passed because the host mirror hard-codes `STALE + cadence`. **Medium, round 2 required.** |
| 10 | **PASS** | Actual: **+9 net non-comment `src/` code lines** against +20; physical diff **+16/−1 = +15**; moved lines **0**. |

Reader effects for `lastConnection`:

- Failsafe: age is refreshed at each online report.
- Status: age becomes time since the latest report.
- Alert-40 force-connect: after a drop, suppressed for up to 30 minutes; it is only acted upon offline.
- Alert-40 “connected recently”: now true for CONNECTED-online webhook failures, enabling the accepted six-hour ERROR_STATE escalation.
- `LedgerClient.cpp:114`: each changed epoch can restart the input window; already-synced ledgers return immediately.
- Connect/Sleep observability and test-mode diagnostics see the newer timestamp only.

## Findings

1. **Medium — round 2 required:** [ConfigApply.cpp:122](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/d85bec8a-671e-48d3-9139-ab71c8426e9b/scratchpad/wo-1b/src/cloud/ConfigApply.cpp:122).  
   **Observation:** every section mutates independently before success is combined at lines 132–139; validation/status scheduling occurs only on total success.  
   **Inference:** an over-range interval leaves a partial configuration, failing the architect’s newly added acceptance condition. No v40 CHANGELOG entry is warranted until disposition.

2. **Medium — round 2 required:** [failsafe_cadence_rule_test.cpp:62](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/d85bec8a-671e-48d3-9139-ab71c8426e9b/scratchpad/wo-1b/tests/failsafe_cadence_rule_test.cpp:62).  
   **Observation:** `belowThreshold()` hard-codes `STALE + cadence`; the source parser checks the real formula only structurally. The required mutation left all 4-hour behavioral assertions green.  
   **Inference:** the claimed behavioral coverage of the production formula is overstated, although the structural check still catches this exact mutation.

3. **Low — round 2 recommended:** [Generalized-Core-Counter.cpp:2632](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/d85bec8a-671e-48d3-9139-ab71c8426e9b/scratchpad/wo-1b/src/Generalized-Core-Counter.cpp:2632).  
   **Observation:** once age reaches three hours, the resolver runs every loop before stage-3 cooldown and low-battery returns. It performs cached reads and bounded local-time calculations; no logging, persistence, or hardware sampling.  
   **Inference:** individual calls should fit the 100 ms budget, but sustained ARM cost at loop frequency remains unmeasured.

## Budget

| Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |
|---|---:|---|---:|---|
| WO-2026-10-08-001 | +20 | — | +9 code; +15 physical | 3 pairs, 688 lines |

## Closing integrity

- Branch stayed `wo/2026-10-08-001-failsafe-overdue`; HEAD stayed `fa78c85`. This is a documentation-only child of the dispatched `9f91aa4`.
- The `src/` diff stat still matches the start: **3 files, 16 insertions, 1 deletion**.
- Full status/stat does **not** match the start: during review, an external 10-line architect pre-ruling appeared in `docs/work-orders/WO-2026-10-08-001-failsafe-overdue.md`. I did not modify or revert it.
- Deleted exactly `build-tmp/wo20261008-001-stage7/`; `build-tmp/` and all pre-existing contents remain.