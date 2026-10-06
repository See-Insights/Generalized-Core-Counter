AGENT: Codex · MODEL: gpt-6-astra · REASONING: high (a rule applied across every alert code, so the review has to be thorough)
AUTHORIZATION SCOPE: review the uncommitted diff in this worktree (branch `wo/2026-10-06-001-alert19-clears`, on top of round 1's commit `77abd4f`); run the host suite and a local ARM build in a scratch copy (or with outputs restored); write temporary harnesses under your scratch directory **`build-tmp/wo20261006-001-stage7r2/`** and remove **only that directory** when done; temporarily mutate files for checks, restoring each byte-identically by rewriting in place / Not authorized: lasting edits, commits, pushes, stash, reset, checkout, flashing, device settings, network or AWS access, and deleting anything you didn't create (if `build-tmp/` didn't exist before, say so; Claude Code removes it if empty).

# Stage 7 dispatch (narrow, round 2) — WO-2026-10-06-001 (v38)

**Goal, in plain language:** event alerts are reported once and then cleared, so none can hide other alerts.

**Two-round rule:** this is v38's second round.

**Repository:** this worktree, branch `wo/2026-10-06-001-alert19-clears`. Round 1 is committed at `77abd4f` (base `7a2b026`, main). Round 2 is the uncommitted diff on top of it.
**Binding spec:** `docs/work-orders/WO-2026-10-06-001-alert19-clears.md`, including the round-2 audit table and its approval record.
**Round 1 verdict:** `docs/work-orders/WO-2026-10-06-001-stage7-verdict.md` (VERIFIED WITH NOTES). Round 1's checks are not repeated, except where round 2 could affect them.

Narrow review: check round 2 against the WO only. Don't widen the fault model or propose new mechanisms. For any defect, give the smallest fix as a proposed diff; don't apply it.

## Checks (PASS/FAIL with evidence for each)

1. **The audit is complete. Every code is accounted for.**
   - Independently enumerate every alert code the firmware can hold: every `raiseAlert(...)` and `set_alertCode(...)` with a nonzero value in `src/`, including through constants or variables, every code in `getAlertSeverity()`, and any legacy value that can persist in `/usr/current.dat`.
   - For each, confirm the WO table's type (event or condition) and its clear path, with file:line.
   - Report any code the table misses or misclassifies, and any condition-type code with no clear path.
2. **Each added code clears after one report:** 18 and 42.
   - The first queued report carries it, then it's cleared (code and `lastAlertTime` 0), and a later lower alert is visible.
   - A report that isn't queued doesn't clear it.
   - Check 42 at each of its raise sites, including the raise inside `publishData()` itself (`Generalized-Core-Counter.cpp:2097`): is it carried by the *next* report rather than cleared by the report that raised it?
   - Check 18 at ThrashGuard tier 2 (no reset) and tier 3 (reset, then the first report after the reboot).
3. **No condition-type alert clears early:**
   - 16, 17, 20/21/23 and 40 are not in `isAutoClearAfterReportAlert()`, and nothing else in round 2 changes their clearing;
   - no code that ERROR_STATE acts on (`State_Error.cpp` `resolveErrorAction()`) now clears before ERROR_STATE can act on it. Say whether clearing 18 or 42 after a report changes any ERROR_STATE or escalation behavior.
4. **Tests:**
   - the two new cases in `tests/alert19_clears_after_report_test.sh` drive the real extracted code;
   - run a mutation per code (remove `case 18:`, then `case 42:`, then `case 19:`): each must fail its own test;
   - restore byte-identically.
   - The update to the copy of the clear list in `tests/watchdog_alert_code_test.cpp` keeps it faithful to the source.
5. **Suite and build:**
   - every `tests/*.sh` with **zsh**, plus every bare `tests/*.py` with python3, as `N/N` (Claude Code: 63/63);
   - the protected tests (`publish_with_ack_structural_test.py`, `sleep_config_ownership_structural_test.py`, `ledger_no_retry_test`) unchanged and green;
   - a clean local boron 6.4.1 release build: text/data/bss against round 1's 150748 / 1090 / 2180; `strings` shows `v38-AllAlertsClear`, product 38;
   - the disassembled clear list covers exactly 15, 18, 19, 31, 41, 42, 43 and 44.
   - Claude Code already ran the cloud compile (succeeded; flash 151974 / RAM 3282).
6. **Budget:** round 2's net `src/` lines against +2 (+1 per added code), and v38's total against about +3; nothing compressed.

## Verdict (required)

VERIFIED, NOT VERIFIED, or VERIFIED WITH NOTES, per check and overall, with evidence. Include:
- the audit as a compact table (code, raise sites, type, clear path);
- binary sizes;
- budget versus actual;
- the model and reasoning level actually used.

Confirm that the working tree is byte-identical to how you found it, and that nothing outside your scratch directory was created or deleted.
