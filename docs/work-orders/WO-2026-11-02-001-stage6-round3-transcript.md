# Copilot CLI Session

> [!NOTE]
> - **Session ID:** `943efac7-0489-4ec7-bea2-9cb704e99611`  
> - **Started:** 11/2/2026, 4:06:36 PM  
> - **Duration:** 12m 27s  
> - **Exported:** 11/2/2026, 4:19:03 PM  

---

<sub>6s</sub>

### User

Implement per the dispatch in docs/work-orders/WO-2026-11-02-001-stage6-round3-copilot-dispatch.md. The WO is the binding spec wherever the dispatch refers to it. Do not commit, push, merge, or release.

---

<sub>9s</sub>

### Copilot

I'll start by reading the dispatch document.

---

<sub>9s</sub>

### `bash`

**Read dispatch doc**

$ cat docs/work-orders/WO-2026-11-02-001-stage6-round3-copilot-dispatch.md; ls docs/work-orders/ | head -50; git --no-pager status --short | head; git --no-pager log --oneline -3

<details>
<summary>76 lines</summary>

```
AGENT: Copilot · MODEL: claude-opus-5.5 · REASONING: high (still escalated after failed rounds, §5)
AUTHORIZATION SCOPE:

**Authorized:** edit `src/power/PmicFaultMonitor.cpp` (the single line at ~342 only) and the tests the change requires (`tests/pmic_fault_monitor_test.cpp`, its `.sh` only if a scenario is added to its list); run the host suite and the local ARM release build. **Scratch directory:** `build-tmp/wo20261102-001-stage6r3/`; remove only it.
**Not authorized:** anything else in `src/`, `docs/`, `lib/`; commits, pushes, merges, branch changes, stash, reset or checkout; deleting anything you didn't create, including `build-tmp/` itself.
**SIZE BUDGET:** `src/` total for the WO stays **net +30**; this round must add **0** net `src/` lines (a change to the existing line, no new line).

# Stage 6 round 3: WO-2026-11-02-001, Stage 7 round 2 blocker (`WO-2026-11-02-001-stage7-round2-verdict.md`, items 2, 3, 5)

Same worktree and branch; the round-2 diff is uncommitted and is your starting point. Binding spec: the WO incl. Amendment 1, and the architect rulings: P:342 is accepted **on the condition that the counter is set and reset to 0 within the same `pollAndRemediate` call and can never carry into the fault-escalation path.**

**The defect (verify it yourself first):** `PmicFaultMonitor.cpp:342` sets `consecutiveFaults = 1` when alert 22 is current and `chargeStatus != 0`. If `remediationInProgress` is true at that moment (the latch's own level-1 cycle is running), the `if (remediationInProgress)` branch (~343-362) runs and the clearing branch (`else if (consecutiveFaults > 0)`, ~363-380) is skipped, so the synthetic 1 survives the call. Consequences: (a) a later first real charge fault does `consecutiveFaults++` to 2 and requests a level-1 cycle on its first sighting; (b) a later fault-free poll with `chargeStatus == 0` enters the clearing branch and clears alert 22 while the charger still reads Not Charging.

**Fix (minimal, same line, 0 net lines):** set the counter only when no remediation cycle is running, i.e. add `!remediationInProgress` to the condition at ~342 (or the equivalent that guarantees the counter is set only on a call where the clearing branch at ~363 will run and reset it to 0). Then alert 22 simply stays raised until the cycle completes and a later poll with `chargeStatus != 0` sets and resets the counter in one call. Keep the same-line comment (update its wording if needed, still one line). If you find a path where the counter can still carry (a fault bit on the same call, `immediateResetConsumed`, anything), say which and stop rather than add lines.

**Tests (the gap Codex named):** add to scenario 7 (or a new scenario 10, your choice, kept small): with alert 22 current and the latch cycle running (`remediationInProgress`), a poll with `chargeStatus == 1` must **not** clear alert 22 and must not leave a carried counter: then (i) a fault-free `chargeStatus == 0` poll after the cycle completes does **not** clear alert 22, and (ii) a first real charge fault on the next fault poll does **not** request a level-1 cycle on its first sighting (it needs the usual 2 consecutive faults). Mutation (copy in `$TMPDIR`, never `src/`): remove the `!remediationInProgress` term; the new test must fail. Re-run the earlier mutation set that still applies.

**Suite and build:** full suite as one sequential loop, output to `/dev/null`, `N/N (sh via zsh, py via python3)` (round 2: 108/108); local ARM release build per §3 with a fresh `BUILD_PATH_BASE`; sizes; `git diff --numstat -- src` total (must stay +30 net).

**Report (final message):** the verified defect and the exact edit (file:line); test and mutation results; suite and build; `src/` arithmetic; model and reasoning level actually used; deviations or "none".
2026-09-29-config-inventory-codex-dispatch.md
2026-09-29-config-inventory-codex-report.md
2026-10-01-connectivity-investigation-codex-dispatch.md
2026-10-01-connectivity-investigation-codex-report.md
2026-10-02-wake-heap-loss-codex-dispatch.md
2026-10-02-wake-heap-loss-codex-report.md
2026-10-04-pre-step6-known-issues-codex-dispatch.md
2026-10-04-pre-step6-known-issues-codex-report.md
2026-10-06-step6-ownership-codex-dispatch.md
2026-10-06-step6-ownership-codex-report.md
2026-10-07-step6-extensibility-evidence.md
2026-10-09-connected-idle-sleep-inventory-dispatch.md
2026-10-09-connected-idle-sleep-inventory.md
2026-10-10-v40-fleet-health-check-report.md
2026-10-10-v40-fleet-health-check.md
2026-10-12-v41-release-and-soak.md
2026-10-15-v42-fleet-health-check-report.md
2026-10-15-v42-fleet-health-check.md
2026-10-15-v43-release-and-soak.md
2026-10-16-v44-deviceos-6.6.0-release-and-soak.md
2026-10-17-v43-fleet-health-check-report.md
2026-10-17-v43-fleet-health-check.md
2026-10-19-sleep-watchdog-investigation-report.md
2026-10-19-v44-bench-soak-dev09-report.md
2026-10-19-v44-bench-soak-dev14-report.md
2026-10-20-v45-release-and-soak.md
2026-10-21-v44-fleet-canaries-report.md
2026-10-22-v44-fleet-health-check-report.md
2026-10-22-v44-fleet-health-check.md
2026-10-22-v45-bench-soak-result.md
2026-10-24-v45-baseline-day1-result.md
2026-10-25-item-20-hour-straddler-step0-codex-dispatch.md
2026-10-25-item-20-hour-straddler-step0-report.md
2026-10-25-v45-baseline-day2-result.md
2026-10-25-v46-release-and-soak.md
2026-10-26-v46-bench-soak-result.md
2026-10-26-v46-day1-result.md
2026-10-27-v46-day2-result.md
2026-10-27-v47-release-and-soak.md
2026-10-28-v46-fleet-health-check-report.md
2026-10-28-v47-day1-and-24h-result.md
2026-10-29-item-10-post-close-credit-step0-codex-dispatch.md
2026-10-29-item-10-post-close-credit-step0-report.md
2026-10-31-v47-fleet-health-check-report.md
WO-2026-08-05-002-battery-authority-delta-guard.md
WO-2026-08-05-003-ledger-write-ordering.md
WO-2026-08-06-001-powermanager-phase1.md
WO-2026-08-07-001-state-report-merge-reconciliation.md
WO-2026-08-10-001-watchdog-instrumentation.md
WO-2026-08-11-001-sleep-prep-threshold-exclusion.md
 M docs/reference/alert-codes.md
 M src/MyPersistentData.cpp
 M src/power/PmicFaultMonitor.cpp
 M src/power/PmicFaultMonitor.h
 M src/sensors/SensorManager.cpp
 M tests/pmic_fault_monitor_test.cpp
 M tests/pmic_fault_monitor_test.sh
 M tests/stubs/pmic_fault_monitor_overrides/Particle.h
?? docs/work-orders/WO-2026-11-01-002-pmic-charger-reapply-step0.md
?? docs/work-orders/WO-2026-11-01-002-step0-report.md
a7b69ce Merge pull request #170 from See-Insights/docs/2026-10-31-occupancy-pilot-reorder
1aa6d12 Pilot record and wave checks: tier-aware reporting rule; LAB-01 low-battery case and panel question
2ee6a41 Occupancy migration: waves confirmed by reachability (Chip, 1 Nov); pending flags removed
<shellId: 0 completed with exit code 0>
```

</details>


---

<sub>11s</sub>

### `bash`

**Find WO files and diff stats**

$ ls docs/work-orders/ | grep -i 2026-11; git --no-pager diff --stat; git --no-pager diff --numstat -- src

<details>
<summary>27 lines</summary>

```
WO-2026-11-01-001-ab1805-native-driver-spike.md
WO-2026-11-01-002-pmic-charger-reapply-step0.md
WO-2026-11-01-002-step0-report.md
WO-2026-11-02-001-stage6-copilot-dispatch.md
WO-2026-11-02-001-stage6-round2-copilot-dispatch.md
WO-2026-11-02-001-stage6-round2-transcript.md
WO-2026-11-02-001-stage6-round3-copilot-dispatch.md
WO-2026-11-02-001-stage6-transcript.md
WO-2026-11-02-001-stage7-round1-codex-dispatch.md
WO-2026-11-02-001-stage7-round1-verdict.md
WO-2026-11-02-001-stage7-round2-codex-dispatch.md
WO-2026-11-02-001-stage7-round2-verdict.md
WO-2026-11-02-001-usb-charge-latch-recovery.md
 docs/reference/alert-codes.md                      |  10 +-
 src/MyPersistentData.cpp                           |   1 +
 src/power/PmicFaultMonitor.cpp                     |  53 +++-
 src/power/PmicFaultMonitor.h                       |   6 +-
 src/sensors/SensorManager.cpp                      |   2 +-
 tests/pmic_fault_monitor_test.cpp                  | 280 ++++++++++++++++++++-
 tests/pmic_fault_monitor_test.sh                   |   4 +
 .../stubs/pmic_fault_monitor_overrides/Particle.h  |   5 +
 8 files changed, 328 insertions(+), 33 deletions(-)
1	0	src/MyPersistentData.cpp
41	12	src/power/PmicFaultMonitor.cpp
3	3	src/power/PmicFaultMonitor.h
1	1	src/sensors/SensorManager.cpp
<shellId: 1 completed with exit code 0>
```

</details>


---

<sub>14s</sub>

### Copilot

Net `src/` is currently +30 (46−16). Reading the WO, the round-2 verdict, and the round-2 dispatch for build details.

---

<sub>14s</sub>

### `bash`

**Read WO and verdict**

$ cd docs/work-orders; cat WO-2026-11-02-001-usb-charge-latch-recovery.md; echo ======; cat WO-2026-11-02-001-stage7-round2-verdict.md

<details>
<summary>54 lines</summary>

```
# WO-2026-11-02-001 — USB charge-latch recovery (recovery item 22, USB scope)

**Status:** APPROVED for Stage 6 by the architect; Chip authorizes the dispatch.
**Governing sections:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 5 (architecture approval gate), Stage 6 (implementation), Stage 7 (independent verification); §5 model routing (power logic is safety-relevant).
**Inputs:** `WO-2026-11-01-002-pmic-charger-reapply-step0.md` and its report (#168, 958fd35); recovery item 22 (LAB-01 USB account).

## Goal (plain)
If a USB-powered unit has good power but its charger sits at "Not Charging" for two hours on a battery that isn't full, toggle charging once (the existing level-1 cycle) and raise alert 21. Log the charger registers when it fires, so the next case shows us the cause.

LAB-01, 1 Nov: "Not Charging" at 3.71–3.75 V from 12:00 to 15:00Z on USB, no charge-fault bit, so the existing fault-driven toggle (`PmicFaultMonitor.cpp`, the `kPmicChargeFaultMask` path) never ran. It charged only after the 15:30Z failsafe reset.

## Rulings on the Step 0's five questions
1. **Scope:** USB-only first; N = 2 h. **Exclusion uses voltage only: skip at ≥ 4.05 V.** Drop the 80% SoC condition: item 23 showed the gauge can be stale, and a stale-high reading would block the recovery.
2. **Weak supply:** `powerGood` false → no action. If the cycle doesn't restore charging: at most one attempt per 24 h, alert 21 stays raised, no escalation to resets. No loop.
3. **Seam:** yes, expose the smallest "request level-1 cycle" seam from `PmicFaultMonitor`; reuse its existing machinery and cooldown. No new state machine.
4. **Gauge quick-start: out of this WO.** Not safe while charging (Step 0), MAX17043 datasheet not in the repo, and `SocTrust` already keeps the tiers correct (item 23). Revisit under item 23.
5. **Solar:** out until Trail02 register evidence exists.
6. **Added:** when the check fires, log REG01, REG08 and REG09 once (one `Log.warn` line). This is the pre-reset snapshot we lacked.

## Conditions (all must hold for N = 2 h continuously)
USB input present and `powerGood`; charge status "Not Charging"; no charge-fault or BAT_FAULT bit; battery voltage < 4.05 V; not thermally regulated; `safeToCharge`. Any condition breaking resets the timer (millis-based).

## Budget
- `src/`: **+20 lines hard** (net). Without the quick-start, expect under the Step 0's +18. Record budget vs actual at close.
- Tests: host tests for the condition (each clause breaks it; 2 h boundary; 24 h attempt limit; ≥ 4.05 V exclusion). Record total test lines, fixture lines and suite count against `0c565fa` (ruling 9).

## Agents
- **Stage 6 — Copilot:** standard model, medium reasoning (single-file, small).
- **Stage 7 — Codex:** top model (`gpt-5.6-sol`), highest reasoning, stdin closed, new output file per round (power logic, §5 escalation). Mandatory linkage, local toolchain build, `nm` and tests per Stage 7.

## Release vehicle
Not decided. Candidate: v49 alongside the binding-delete guard, after the AB1805 spike. Chip decides.

## Out of scope
Gauge quick-start; solar units; changes to the fault-driven toggle path; failsafe changes.

## Amendment 1 (2026-11-02, after Stage 7 round 1 NOT VERIFIED)
**Goal restated:** a USB unit that has good power but is stuck at "Not Charging" for 2 h gets one charging toggle per 24 h, an alert, and one register log line. Nothing more.

1. **Raise alert 22, not 21.** 22 is unused, and it already sits in the "charging resumed, codes 20–23" clear (`alert-codes.md` row 22; P:369–371), so it clears when charging resumes with no new clear code. Add its severity entry (3, matching 20 and 21). **Restore the alert-21 clear to the original `regs.chargeStatus != 2` exactly.** Stage 7 must confirm the 20–23 clear fires on Pre-charge, Fast and Done.
2. **USB gating:** use the existing corrected-source test (the `usbBackedFaultContext` expression: `activeInputProfile` / `isUsbBackedSource(...)`), hoisted so both paths share it, AND VBUS status. Raw VBUS alone is not enough (VIN can read as USB).
3. **Request the level-1 cycle through the existing path and honour its 1 h cooldown;** don't write the remediation fields directly.
4. **Drop** the `vcell = 99.0f` default parameter (update the test calls) and the `vcell > 2.5f` bound.
5. **Normal style:** no comma statements, no one-line struct.
6. **Docs in the same PR:** `docs/reference/alert-codes.md` row 22 (meaning, raise site, severity, clear) and row 21 unchanged apart from line references.

**Budget raised: `src/` +30 hard (was +20).** Reason: the reformat and the severity entry. If round 2 can't fit +30, stop and report rather than trimming style.
======
# NOT VERIFIED

Model: `gpt-5.6-sol`; reasoning: `xhigh`.

1. **Round-1 findings:** Alert 22 is raised at `PmicFaultMonitor.cpp:406`; severity 3 is at `MyPersistentData.cpp:782`. The alert-21 predicate at `PmicFaultMonitor.cpp:385` is byte-identical to `main`. USB gating combines the shared corrected-source value (`:134–135`) with VBUS 1/2 (`:394`). The cycle uses `requestRemediationCycle()` (`:121–128,403`) and its 1 h cooldown. `vcell` is required (`PmicFaultMonitor.h:49–50`), with only `<4.05f` at `:396`. Normal style is restored; the architect-approved statics and same-line comment remain.

2. **Counter — High blocker:** `consecutiveFaults` is **not always reset in the same call**. When alert 22 is current, charging has resumed, and `remediationInProgress` is true, line 342 sets it to 1; lines 343–362 take the remediation branch and skip the clearing branch at `:363–380`, even when that call completes the cycle. The synthetic 1 therefore survives.

3. **Concrete premature-escalation path:** Fire the latch, run phase 0, then poll after 500 ms with no fault, `chargeStatus=1`, `safeToCharge=true`. Line 342 sets 1 and `:346–354` completes the cycle without clearing it. With no intervening poll, wait over 1 h and report a first real thermal charge fault (`faultReg=0x20`). Immediate reset is excluded by `:150`; `:208` increments 1→2, and `:302–311` immediately requests level 1. A first real fault has therefore escalated one level early.

4. A CHRG_FAULT already present on the line-342 call bypasses line 342; a successful immediate reset clears the counter at `:168–176`. BAT_FAULT alone still enters the no-CHRG-fault branch. `immediateResetConsumed` does not repair a synthetic value carried from an earlier call.

5. **20–23 clear — High blocker:** Nominally, with no remediation running, statuses 1/2/3 set 1 at `:342` and clear it plus alert 22 at `:363–379`; status 0 does not set it. But after the carry path above, a following fault-free status-0 poll enters `consecutiveFaults > 0` and clears alert 22 while still Not Charging. Thus “not on 0” and the row-22 claim are false.

6. Scenario 7 does expect clearing for Pre/Fast/Done (`pmic_fault_monitor_test.cpp:472–485`), but it finishes the cycle and adds another Not-Charging poll at `:478–480` before resumption. It does not exercise resumption while remediation is active. Alert 21 remains unchanged: Fast retains it and Off/Pre/Done clear it (`:492–512`).

7. **Isolation:** Each top-level scenario gets fresh statics through a separate process (`pmic_fault_monitor_test.sh:15–27`; test `:552–585`). Scenario-6 helper cases do not literally reset statics; `freshWindow()` breaks the timer, completes any cycle, and advances 25 h (`:337–347`). This is order-independent under the actual process-per-scenario harness.

8. A fresh compile was blocked: Clang could not create `/tmp/xcrun_db-*` or temporary files. The structural test passed. A pre-existing binary newer than the reviewed inputs exposed the current signature/latch symbols and passed 28 shuffled/repeated process runs, but that is not a fresh-build result. No PMIC-specific `/tmp` flake note is present in `tests/README.md`.

9. **Condition/cycle:** All clauses are present at `PmicFaultMonitor.cpp:394–396`; `:397–403` resets/restarts the 2 h timer, enforces 24 h, and consumes the attempt only after an accepted request. A running cycle/cooldown merely defers firing. Unsigned subtraction provides normal wrap behavior; the host test uses 64-bit wrap (`test:441–447`). The log is once per accepted request with all three `(unsigned)` casts (`:404–405`).

10. The phase machine retains ownership while unsafe and therefore does not strand charging ownerlessly (`:65–79,343–362`). A fault arriving mid-cycle follows the existing fault branch, except that the carried synthetic counter can corrupt its escalation level as described above.

11. **Fault-path equivalence:** Hoisting is Boolean-equivalent to `main`: current `usbBackedSource || usbInputPresent` (`:134–143`) equals the original profile/source/source/VBUS OR. At fault request sites `:311,314`, the outer `!remediationInProgress` and `>1 h` checks make the helper’s refusal conditions equivalent. No ordinary fault-path decision changed.

12. **Mutations:** By inspection, all eight required mutations are caught: 2 h and 4.05 V at test `:350–388`; power-good/timer reset/shared USB at `:410–438`; 24 h at `:360–374`; alert 21/22 and legacy clear at `:463–512` plus structural test `:30–44`. The material gap is no test/mutation for line 342 while remediation is active, followed by status 0 or a later real fault.

13. Controller must freshly run: `export TMPDIR="$(mktemp -d)"; zsh tests/pmic_fault_monitor_test.sh; python3 tests/pmic_latch_call_site_structural_test.py`, then invoke `$TMPDIR/pmic_fault_monitor_test` in shuffled/repeated 1–9 orders. In eight independent `$TMPDIR` copies it must apply the listed boundary, voltage, power-good, 24 h, timer-reset, USB, alert-code, and `& 1` mutations and rerun scenarios 1–9 plus the structural test.

14. **Size/style:** `src/` is `+46/−16 = +30 net` (`MyPersistentData +1`, monitor `.cpp +29`, header and caller 0), exactly the hard limit. `git diff --check` passes.

15. **Docs:** Rows 20/21/23 accurately re-cite, among others, NTC `P:201`, thermal `P:220`, alert-21 sites `P:215/225/230/549`, clears `P:178–180/376–378/385–387`, and alert-23 sites `P:233/416`. Row 21’s meaning is unchanged. Row 22 is not accurate: its claim that line 342 resets the counter in the same call and remains raised while Not Charging is disproved above.
<shellId: 2 completed with exit code 0>
```

</details>


---

<sub>17s</sub>

### `bash`

**Read round 2 dispatch and build info**

$ cd docs/work-orders; cat WO-2026-11-02-001-stage6-round2-copilot-dispatch.md; echo =====; tail -60 WO-2026-11-02-001-stage6-round2-transcript.md; grep -n "§3\|BUILD_PATH_BASE\|compile-all\|make " ../../AI_DEVELOPMENT_WORKFLOW.md 2>/dev/null | head -30; ls ../..

<details>
<summary>101 lines</summary>

````
AGENT: Copilot · MODEL: claude-opus-5.5 · REASONING: high (escalated after a failed round, §5)
AUTHORIZATION SCOPE:

**Authorized:** edit `src/power/PmicFaultMonitor.cpp`, `src/power/PmicFaultMonitor.h`, `src/MyPersistentData.cpp` (the severity entry for alert 22 only), `src/sensors/SensorManager.cpp` (the one call site, only if the signature needs it), and the tests the change requires (`tests/pmic_fault_monitor_test.cpp`, its `.sh`, the stub under `tests/stubs/pmic_fault_monitor_overrides/`, `tests/pmic_latch_call_site_structural_test.py`); run the host suite and the local ARM release build. **Scratch directory:** `build-tmp/wo20261102-001-stage6r2/`; remove only it.
**Not authorized:** anything else in `src/`, `docs/`, `lib/`; commits, pushes, merges, branch changes, stash, reset or checkout; deleting anything you didn't create, including `build-tmp/` itself.
**SIZE BUDGET:** `src/` total for the WO (round 1 plus this round) **net <= +30, HARD.** If it cannot be done in normal style within +30, **STOP and report; do not trim style, drop a ruling, or compress code.**

# Stage 6 round 2: WO-2026-11-02-001, Amendment 1 (architect rulings after Stage 7 round 1 NOT VERIFIED)

Binding spec: `docs/work-orders/WO-2026-11-02-001-usb-charge-latch-recovery.md`, **including "Amendment 1"** (read it first). The Stage 7 verdict is `WO-2026-11-02-001-stage7-round1-verdict.md`. The round-1 diff is uncommitted and is your starting point; same worktree and branch. Rework it as follows (the amendment overrides round 1 wherever they differ):

1. **Alert 22, not 21.** Raise `RecoveryState::raiseAlert(22)` when the latch fires. Add `case 22:` with the PMIC group in the severity function in `src/MyPersistentData.cpp` (~line 781, severity 3, same as 20 and 21). The existing "charging resumed" clear (`PmicFaultMonitor.cpp` ~367, codes 20..23) must stay untouched and must clear 22 with no new clear code. **Restore the alert-21 clear to the original `regs.chargeStatus != 2` exactly** (revert the round-1 `& 1` edit and its comment).
2. **USB gating:** hoist the existing `usbBackedFaultContext` expression (profile `UsbBench` / `isUsbBackedSource(...)` x2 / `usbInputPresent`, currently inside the fault branch ~line 128) so **both** the fault path and the latch use the same value, **and** require VBUS status 1 or 2 for the latch. Do not change what the fault path decides.
3. **Level-1 cycle through the existing path, honouring the 1 h cooldown.** Do not assign `remediationInProgress`/`remediationActiveLevel`/`remediationPhase` directly. Find how the fault branch starts a level-1 cycle (`remediationLevel`, `lastRemediationAttempt`, `kRemediationCooldownMs`, ~lines 285-310) and request it the same way from the latch trigger, so the cooldown applies and the cycle runs on the existing machinery. If the existing structure makes that impossible within +30 in normal style, stop and report.
4. **Drop** the `float vcell = 99.0f` default (make `vcell` a plain required parameter and update every test call) and the `vcell > 2.5f` bound. The voltage clause is only `< 4.05 V`.
5. **Normal style:** no comma statements, no one-line struct, braces and line breaks as in the surrounding file. Reformat the pure function and struct accordingly.
6. Keep: 2 h continuous, 24 h per attempt, `powerGood`, `chargeStatus == 0`, no fault bits, not thermal regulation, `safeToCharge`, the single `Log.warn` with REG01/REG08/REG09 (cast to `(unsigned)` for `%x`).
7. **Tests:** update scenario 6 (clauses, boundaries, 24 h; add the USB-gating and VBUS clauses, and a millis wrap-around case if the host type can model it) and **scenario 7 must expect alert 22 to clear when charging resumes** (Pre-charge, Fast and Done each, via the existing 20-23 clear). Add: alert 21 clear unchanged (`chargeStatus != 2` semantics, alert 21 is not touched by the latch), and a cooldown case (a latch request within 1 h of a remediation attempt does not start a second cycle). Mutations (copies in `$TMPDIR`, never `src/`): the earlier set (2 h `>=`/`>`, 4.05 V `<`/`<=`, drop `powerGood`, drop the 24 h limit, drop the timer reset), plus drop the shared USB-context clause, raise 21 instead of 22, and restore the `& 1` clear; each must fail a test. Update the structural test for the new signature and the alert code.
8. **Suite and build:** the full suite as one sequential loop, output to `/dev/null`, `N/N (sh via zsh, py via python3)` (baseline 107, round 1 was 108); local ARM release build per §3 recipe with a fresh `BUILD_PATH_BASE`; report sizes. Then `nm` on the ELF for the new symbols.

**Report (final message):** per file net and gross `src/` lines with the arithmetic and the total against +30; test lines; each mutation and its catching check; how the cooldown/seam is wired (file:line); the model and reasoning level actually used; deviations or "none".
=====
```


---

<sub>28m 31s</sub>

### Copilot

Round 2 is implemented and lands exactly at the +30 `src/` limit. The full suite passes **108/108 (sh via zsh, py via python3)** and the local ARM release build succeeds. Nothing was committed or pushed. The `alert-codes.md` update the WO requires is still outstanding because this dispatch doesn't authorise `docs/`. One added line (P:342) needs your ruling (deviation 3).

**`src/` lines (round 1 + round 2, against HEAD)**

| File | Gross | Net |
|---|---|---|
| `src/power/PmicFaultMonitor.cpp` | +41 / −12 | +29 |
| `src/power/PmicFaultMonitor.h` | +3 / −3 | 0 |
| `src/MyPersistentData.cpp` | +1 / −0 | +1 |
| `src/sensors/SensorManager.cpp` | +1 / −1 | 0 |
| **Total** | **+46 / −16** | **+30** |

29 + 0 + 1 + 0 = **+30, at the limit**. `git diff --check` is clean.

**Test lines:** `pmic_fault_monitor_test.cpp` +268/−12, `pmic_fault_monitor_test.sh` +4, stub `Particle.h` +5, `pmic_latch_call_site_structural_test.py` 45 lines (untracked file).

**Build:** text 152068, data 1114, bss 2216, `.bin` 153186 bytes. `nm` on the ELF and on `PmicFaultMonitor.o` shows `pollAndRemediate(PMIC&, bool, unsigned char, int, bool, float)` (no other version), the four `latch*` statics, the request lambda, and `PMIC::readPowerONRegister()`. The scratch directory has been removed.

**Mutations** (copies in `$TMPDIR`): all 18 fail at least one check. "Scenario 6 boundary" means the assert that the latch fires at exactly 2 h.

| Mutation | Caught by |
|---|---|
| 2 h `>=` → `>` | scenario 6 boundary, scenario 7 |
| 4.05 V `<` → `<=` | scenario 6 voltage case |
| drop `powerGood` | scenario 6 clause-break case |
| drop 24 h limit | scenario 6 (no second cycle before 24 h) |
| drop timer reset | scenario 6 first-poll assert, scenario 7 |
| drop shared USB-context clause | scenario 6 USB-gating case (VIN / battery / unknown source) |
| raise 21 instead of 22 | scenarios 6, 7, 9 and the structural test |
| restore the `& 1` clear | scenario 8 (Not Charging clears 21) and the structural test |

The other 10 are extras: drop the VBUS clause, allow OTG (VBUS 3), drop each of the remaining clauses (`chargeStatus == 0`, fault bits, thermal, `safeToCharge`), drop the cooldown (scenario 9), drop the alert-22 route (scenario 7), drop `case 22` and re-add the `vcell` default (both structural test).

**Cooldown and seam wiring**
- **Shared request:** `requestRemediationCycle` (lambda, P:121–128). It refuses while a cycle is running or while `now - lastRemediationAttempt <= kRemediationCooldownMs`.
- **Fault path:** P:311 and P:314 now call it instead of setting the fields; behaviour is the same.
- **Latch:** P:401–403 calls it. If refused, firing is put off; the timer keeps running and the 24 h attempt is not used up.
- **Cooldown start points:** P:163 (immediate reset) and P:354 (cycle finished).

**Model:** Claude Opus 5.5 (`claude-opus-5.5`). The reasoning level isn't visible to me; the dispatch specified high.

**Deviations**
1. **No struct or pure function.** In normal style they came to about +38, so the latch state is four `static`s inside `pollAndRemediate`. Tests go through the production function.
2. **USB gating split.** Only profile/`isUsbBackedSource` ×2 is shared (P:134). The fault path still adds raw VBUS (P:143), so its decisions don't change. Sharing the whole expression would make the latch gate raw VBUS only, which WO Amendment 1 rule 2 forbids, and the "drop USB-context" mutation could never be caught.
3. **Alert 22 would never clear without help.** The 20–23 clear only runs when `consecutiveFaults > 0` (P:363), and the latch never sets it. I added one line (P:342): if alert 22 is current and the charger is not at Not Charging, set `consecutiveFaults = 1`. The untouched clear at P:376 then does the clearing, so 22 stays while Not Charging and clears on Pre-charge, Fast or Done. This needs your ruling.
4. **Running cycle or cooldown** now puts off firing instead of resetting the timer.
5. **Comments shortened** to fit +30; no code was compressed.

---

<sub>Generated by [GitHub Copilot CLI](https://github.com/features/copilot/cli)</sub>
106:  - **Agents:** use a fresh `BUILD_PATH_BASE` for every build type, in a
109:  - **Reusing a path** (for example, Chip in a terminal): run `make clean` in
111:    `BUILD_PATH_BASE` arguments as the build.
113:  Workbench's `make clean-user` (from `buildscripts/<ver>/Makefile`) only runs
114:  `make clean` in `deviceOS/<ver>/main` with Workbench's own arguments, so it
117:  §3 command and its own `BUILD_PATH_BASE`. (Corrected 2026-10-07: this rule
118:  used to say `make clean-user`, which has no effect on agents' scratch
225:- Run the mutation checks the Stage 7 dispatch lists. Each mutation must make at least one test fail; a surviving mutation is a finding.
236:- Must not make lasting edits to source code. Every mutated file is restored byte-identically before the verdict is issued, and temporary artifacts are removed.
433:    make -s PLATFORM=boron APPDIR=<copy-of-tree> TARGET_DIR=<copy>/localbuild \
435:         BUILD_PATH_BASE=<fresh scratch dir that doesn't exist yet>
440:Use a new `BUILD_PATH_BASE` for each build type (see §2, "Verifying compile-time
441:flags"); `make clean-user` does not clean this path.
468:  `BUILD_PATH_BASE`, and the linkage check (`nm` on the symbolised ELF). A WO's
545:`codex review --uncommitted` is not used: it rejects custom instructions (CLI 0.154.0). Codex's sandbox has no network, so the Particle cloud compile (a release-head check, once per release; see "Release gate" in §3) is run by Claude Code, not in Stage 7.
740:5. **Verify the binary, not the source, for anything involving vendored libraries.** A library that is both vendored in `lib/` and listed in `project.properties` may be replaced by the registry copy in a cloud build. Confirm the change is present in the binary that will be flashed (disassembly, symbols, or strings), not only in the source tree. Local and cloud builds may use different library copies. Bench-test the build type the fleet will receive. The cloud compile that checks this runs once per release, on the release branch head (§3, "Release gate").
767:- **Scope:** Chip or the architect may pre-authorize Claude Code to make a one-line edit or deletion of the same kind as existing code, for example resetting a timer the way a neighbouring exit already does, or deleting a function the WO left uncalled.
790:  - **The candidate:** it always gets a fresh `BUILD_PATH_BASE`, with no object reuse between the two builds.
791:  - **Keeping the ELF:** to make this possible, each Stage 7 keeps its verified candidate ELF with its `src` tree hash, for example `build-tmp/verified-elf/<src-tree-hash>.elf`. (The case: WO-2026-10-15-002's Stage 7 spent 44 of its 65 minutes on two fresh builds.)
795:- the local ARM toolchain build (with its `BUILD_PATH_BASE`);
796:- the Particle cloud compile (once per release, on the release branch head; §3);
807:- **Builds:** results come from a fresh `BUILD_PATH_BASE` (§2, "Verifying compile-time flags").
AI_DEVELOPMENT_WORKFLOW.md
ARCHITECTURE_OVERVIEW.md
CHANGELOG.md
Doxyfile
LICENSE
PROJECT_STATUS.md
README.md
STYLE.md
build-tmp
bump_version.sh
default-settings-v3.23.json
docs
lib
project.properties
release
release.sh
src
tests
tools
<shellId: 3 completed with exit code 0>
````

</details>


---

<sub>20s</sub>

### `bash`

**Read build recipe**

$ sed -n 420,450p AI_DEVELOPMENT_WORKFLOW.md; grep -n "BUILD_PATH_BASE\|make -s\|compile\|toolchain\|PATH=" docs/work-orders/WO-2026-11-02-001-stage6-round2-transcript.md | head -30

<details>
<summary>56 lines</summary>

```
2026-10-25, Chip): it runs once per release, on the release branch head (see
"Release gate: the Particle cloud compile", below).

The two use different include-search orders. A source file can compile cleanly in
the cloud and fail locally — or the reverse — so a cloud build passing is **not**
evidence that the firmware builds. The most common form is a header name that
collides with one Device OS ships (`Config.h` is the known case; Device OS has
`services/inc/Config.h`). Within `src/`, subdirectory files include app headers
by relative path (`"../Config.h"`) precisely to avoid this; a bare include is
resolution-order dependent and may work only by accident.

    export PATH=~/.particle/toolchains/buildtools/1.1.1/bin:~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH
    cd ~/.particle/toolchains/deviceOS/6.6.0/main
    make -s PLATFORM=boron APPDIR=<copy-of-tree> TARGET_DIR=<copy>/localbuild \
         DEVICE_OS_PATH=~/.particle/toolchains/deviceOS/6.6.0 \
         BUILD_PATH_BASE=<fresh scratch dir that doesn't exist yet>

This is also the build that produces the symbolised ELF the linkage check above
requires, so the two checks share the work.

Use a new `BUILD_PATH_BASE` for each build type (see §2, "Verifying compile-time
flags"); `make clean-user` does not clean this path.

Added 2026-08-26 after WO-2026-08-25-001. An include rewritten from
`"../Config.h"` to `"Config.h"` broke the local build and survived **seven
implementation rounds and three independent Stage 7 reviews** — because every
acceptance test, on every side, used the cloud compiler. Codex had flagged the
same collision class in round 3 and it was recorded as an environment quirk
rather than a defect class. It was found only when the engineer tried to flash a
device.

57:8. **Suite and build:** the full suite as one sequential loop, output to `/dev/null`, `N/N (sh via zsh, py via python3)` (baseline 107, round 1 was 108); local ARM release build per §3 recipe with a fresh `BUILD_PATH_BASE`; report sizes. Then `nm` on the ELF for the new symbols.
168:- **Stage 7 — Codex:** top model (`gpt-5.6-sol`), highest reasoning, stdin closed, new output file per round (power logic, §5 escalation). Mandatory linkage, local toolchain build, `nm` and tests per Stage 7.
212:6. **Default parameter — Medium:** The sole production caller passes `vcell` (`SensorManager.cpp:716–719`), and the structural test passed with exit 0. Nevertheless, `float vcell=99.0f` at `PmicFaultMonitor.h:49–50` lets any future caller compile while silently disabling latch recovery. The structural test scans only current `src/*.cpp` calls. Remove the default and update older host calls to pass an explicit value.
2082:912-  // sites in this file compiles for any given platform.
3046:    if c.returncode: fails.append("compile")
3169:$ grep -n "BUILD_PATH_BASE" -B3 -A12 AI_DEVELOPMENT_WORKFLOW.md docs/*.md 2>/dev/null | head -60
3178:AI_DEVELOPMENT_WORKFLOW.md:106:  - **Agents:** use a fresh `BUILD_PATH_BASE` for every build type, in a
3183:AI_DEVELOPMENT_WORKFLOW.md:111:    `BUILD_PATH_BASE` arguments as the build.
3189:AI_DEVELOPMENT_WORKFLOW.md:117:  §3 command and its own `BUILD_PATH_BASE`. (Corrected 2026-10-07: this rule
3203:AI_DEVELOPMENT_WORKFLOW.md-432-    cd ~/.particle/toolchains/deviceOS/6.6.0/main
3204:AI_DEVELOPMENT_WORKFLOW.md-433-    make -s PLATFORM=boron APPDIR=<copy-of-tree> TARGET_DIR=<copy>/localbuild \
3205:AI_DEVELOPMENT_WORKFLOW.md-434-         DEVICE_OS_PATH=~/.particle/toolchains/deviceOS/6.6.0 \
3206:AI_DEVELOPMENT_WORKFLOW.md:435:         BUILD_PATH_BASE=<fresh scratch dir that doesn't exist yet>
3211:AI_DEVELOPMENT_WORKFLOW.md:440:Use a new `BUILD_PATH_BASE` for each build type (see §2, "Verifying compile-time
3217:AI_DEVELOPMENT_WORKFLOW.md-446-acceptance test, on every side, used the cloud compiler. Codex had flagged the
3226:AI_DEVELOPMENT_WORKFLOW.md-466-  (j). The compile must be of the head that is uploaded, not an earlier commit.
3228:AI_DEVELOPMENT_WORKFLOW.md:468:  `BUILD_PATH_BASE`, and the linkage check (`nm` on the symbolised ELF). A WO's
3269:#### Mandatory: local toolchain build
3271:Every change must be built with the **local Device OS toolchain** before Stage 7
3272:can pass. The Particle cloud compile is **not** a per-change check (changed
3274:"Release gate: the Particle cloud compile", below).
3276:The two use different include-search orders. A source file can compile cleanly in
3284:    export PATH=~/.particle/toolchains/buildtools/1.1.1/bin:~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH
3285:    cd ~/.particle/toolchains/deviceOS/6.6.0/main
3286:    make -s PLATFORM=boron APPDIR=<copy-of-tree> TARGET_DIR=<copy>/localbuild \
3287:         DEVICE_OS_PATH=~/.particle/toolchains/deviceOS/6.6.0 \
3288:         BUILD_PATH_BASE=<fresh scratch dir that doesn't exist yet>
3375:Suite: **108/108**. Now the local ARM release build in the scratch dir, using a fresh `BUILD_PATH_BASE`.
3386:export PATH=~/.particle/toolchains/buildtools/1.1.1/bin:~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH
3387:cd ~/.particle/toolchains/deviceOS/6.6.0/main
<shellId: 4 completed with exit code 0>
```

</details>


---

<sub>20s</sub>

### `view`

**view the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/src/power/PmicFaultMonitor.cpp.**

/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/src/power/PmicFaultMonitor.cpp (lines 100-420)

<details>
<summary>298 lines</summary>

```
} // namespace

namespace PmicFaultMonitor {

Registers pollAndRemediate(PMIC &pmic, bool safeToCharge, uint8_t battState, int powerSource,
                            bool chargeDisableConfigVerified, float vcell) {
  static unsigned long lastRemediationAttempt = 0;
  static uint8_t remediationLevel = 0;       // 0=none, 1=soft reset, 2=power cycle
  static uint8_t consecutiveFaults = 0;
  static bool remediationInProgress = false;
  static uint8_t remediationActiveLevel = 0; // 0=none, 1=soft, 2=aggressive
  static uint8_t remediationPhase = 0;       // 0=start, 1=wait
  static unsigned long remediationPhaseStartMs = 0;
  static bool immediateResetConsumed = false;
  static uint8_t immediateResetFaultReg = 0;
  static unsigned long latchSince = 0;       // USB Not Charging latch (WO-2026-11-02-001)
  static unsigned long latchLastAttempt = 0;
  static bool latchTiming = false;
  static bool latchTried = false;
  constexpr unsigned long kRemediationCooldownMs = 3600000UL; // 1 hour

  // Shared level-1/2 cycle request (fault path, charge latch); refused while one runs or in the 1 h cooldown.
  auto requestRemediationCycle = [&](uint8_t level, unsigned long now) {
    if (remediationInProgress || now - lastRemediationAttempt <= kRemediationCooldownMs) return false;
    remediationInProgress = true;
    remediationActiveLevel = level;
    remediationPhase = 0;
    return true;
  };

  Registers regs;
  readRegisters(pmic, regs);

  const PowerReport &powerReport = PowerManager::instance().latestReport();
  const bool usbBackedSource = powerReport.activeInputProfile == PowerInputProfile::UsbBench ||
                               isUsbBackedSource(powerReport.reading.powerSource) || isUsbBackedSource(powerSource);

  // Immediate one-shot reset attempt: only when safe, USB-backed (a solar dip
  // is not fixed by toggling charging), not already tried for this exact
  // fault, not thermally regulated, and not already in a severe battery state.
  bool persistentAfterImmediateReset = false;
  if (regs.faultReg & kPmicChargeFaultMask) {
    const bool usbInputPresent = (regs.vbusStatus == 1 || regs.vbusStatus == 2 || regs.vbusStatus == 3);
    const bool usbBackedFaultContext = usbBackedSource || usbInputPresent;
    const bool severeBatteryState = (battState == 5 || battState == 6);
    const uint8_t initialChargeFault = pmicChargeFaultCode(regs.faultReg);
    const bool alreadyAttempted = immediateResetConsumed && immediateResetFaultReg == regs.faultReg;
    const bool canAttemptImmediateReset =
        !alreadyAttempted && safeToCharge && regs.powerGood && usbInputPresent &&
        usbBackedFaultContext && !regs.thermalRegulation && !severeBatteryState &&
        initialChargeFault != 0x02;

    if (canAttemptImmediateReset) {
      Log.warn("PMIC: charge fault active; attempting charging reset");
      pmic.disableCharging();
      boundedBatterySettleDelay(500UL);
      pmic.enableCharging();
      boundedBatterySettleDelay(500UL);

      readRegisters(pmic, regs);
      immediateResetConsumed = true;
      immediateResetFaultReg =
          (regs.faultReg & kPmicChargeFaultMask) ? regs.faultReg : (uint8_t)(initialChargeFault << 4);
      lastRemediationAttempt = millis();

      Log.info("PMIC: fault reset result faultReg=0x%02x charge=%s", regs.faultReg,
                chargeLabel(regs.chargeStatus, false));

      if (!(regs.faultReg & kPmicChargeFaultMask)) {
        Log.info("PMIC: charge fault cleared");
        consecutiveFaults = 0;
        remediationLevel = 0;
        remediationInProgress = false;
        remediationActiveLevel = 0;
        remediationPhase = 0;
        immediateResetConsumed = false;
        immediateResetFaultReg = 0;

        const int8_t currentAlert = RecoveryState::get_alertCode();
        if (currentAlert == 21 || currentAlert == 23) {
          RecoveryState::set_alertCode(0);
          RecoveryState::set_lastAlertTime(0);
        }
      } else {
        persistentAfterImmediateReset = true;
      }
    }
  }

  // NTC_FAULT (bits 2:0): the PMIC's own TS/thermal-status classification of
  // the *charge-path* thermistor network, independent of CHRG_FAULT (bits
  // 5:4) and BAT_FAULT (bit 3) above. WO-2026-08-25-001 Amendment B, Blocker
  // 4: this pack has no NTC, but the carrier/TS network can still produce
  // this fault class, and it was previously neither classified nor alerted -
  // the monitor only ever checked CHRG_FAULT/BAT_FAULT. It is a thermal-status
  // fault by definition, so it is one of the two classes for which "recovers
  // naturally, don't fight it" is a defensible remediation-suppression
  // rationale (see thermallyCorrelatedFault below).
  const uint8_t ntcFaultCode = regs.faultReg & kPmicNtcFaultMask;
  if (ntcFaultCode != 0) {
    Log.warn("PMIC: NTC fault active (code=0x%02x) - charge-path thermistor/TS network fault", ntcFaultCode);
    RecoveryState::raiseAlert(20); // Reuse the thermal alert code - this is a thermal-status class fault.
  }

  // CHRG_FAULT (bits 5:4). BAT_FAULT is separate, handled below, and does not
  // enter this remediation path (a hardware condition, not fixable by toggle).
  if (regs.faultReg & kPmicChargeFaultMask) {
    const uint8_t chargeFault = pmicChargeFaultCode(regs.faultReg);
    consecutiveFaults++;

    switch (chargeFault) {
      case 0x01: // Input fault: usually a transient solar dip - log only.
        Log.info("PMIC: Input fault - VBUS out of range (likely solar variation)");
        if (persistentAfterImmediateReset) {
          Log.error("PMIC: charge fault persists after reset - alert 21");
          RecoveryState::raiseAlert(21);
        }
        break;
      case 0x02: // Thermal shutdown.
        Log.error("PMIC: Thermal shutdown - charging stopped due to temperature");
        RecoveryState::raiseAlert(20);
        break;
      case 0x03: // Charge safety timer expired - common stuck-charging indicator.
        Log.error(persistentAfterImmediateReset ? "PMIC: charge fault persists after reset - alert 21"
                                                 : "PMIC: Charge safety timer expired - charging timeout");
        RecoveryState::raiseAlert(21);
        break;
      default:
        if (persistentAfterImmediateReset) {
          Log.error("PMIC: charge fault persists after reset - alert 21");
          RecoveryState::raiseAlert(21);
        } else {
          Log.warn("PMIC: Charge fault detected (code=0x%02x)", chargeFault);
          RecoveryState::raiseAlert(23);
        }
        break;
    }

    // Per-fault-class remediation suppression (WO-2026-08-25-001 Amendment B,
    // Blocker 4; corrected under Amendment C, Decision C3 point 1). "It
    // recovers naturally" is true ONLY for a fault directly explained by the
    // same enclosure-temperature condition ChargeInhibit is managing -
    // thermal shutdown (CHRG_FAULT==0x02). It is FALSE for an input fault
    // (independent cause: e.g. solar variation, cabling) and for
    // safety-timer expiry (needs an explicit charge-cycle/timer
    // intervention) and the general/unclassified case - blanket-suppressing
    // those for the entire hot interval would silently drop the only
    // attempted recovery for those fault classes.
    //
    // Classified strictly per the CHRG_FAULT code being handled in THIS
    // switch - NOT by whether NTC_FAULT (bits 2:0, a physically separate
    // register field, already alerted independently above) happens to also
    // be nonzero on the same register read. NTC_FAULT and CHRG_FAULT can be
    // simultaneously nonzero without being the same event (e.g. a TS-network
    // fault occurring at the same poll as an unrelated VBUS input dip); OR-ing
    // ntcFaultCode into this flag previously suppressed remediation for that
    // independent, non-thermal CHRG_FAULT too.
    const bool thermallyCorrelatedFault = (chargeFault == 0x02);

    // Even for a non-thermal-correlated fault, if ChargeInhibit's DCT config
    // disable was READ BACK as set this cycle (System.getPowerConfiguration()
    // confirmed it, not merely the software safeToCharge intent), a raw PMIC
    // toggle here would likely fight a disable that is already in effect, or
    // about to be applied by Device OS's asynchronous reconciliation, at the
    // hardware level. If it was NOT read back as set, proceed anyway - this
    // fault may be exactly why the disable never took, and this is the one
    // path that could recover it.
    const bool configDisableBlocksToggle =
        !safeToCharge && !thermallyCorrelatedFault && chargeDisableConfigVerified;

    if ((!safeToCharge && thermallyCorrelatedFault) || configDisableBlocksToggle) {
      if (thermallyCorrelatedFault) {
        Log.info("PMIC: Fault detected but charging disabled due to temperature (%.1fC) - fault class is "
                 "thermally correlated, skipping remediation (recovers naturally)",
                 (double)CurrentReadings::get_internalTempC());
      } else {
        Log.info("PMIC: Fault detected, thermal disable config is read-back verified - deferring "
                 "remediation for this non-thermal fault class until charging is re-enabled");
      }
      // Freeze remediation state rather than abandoning it (ALSO FIX): if a
      // level-1/2 sequence already disabled charging in phase 0
      // (remediationPhase==1, i.e. pmic.disableCharging() already ran and it
      // is waiting to resume/re-enable) and this branch zeroed
      // remediationInProgress/ActiveLevel/Phase here, charging would be left
      // disabled at the PMIC with no software owner left to re-enable it.
      // Leaving the state untouched lets it resume - and finish, including
      // the re-enable step - the moment it is safe/no-longer-hardware-blocked
      // again, instead of stranding it disabled.
    } else {
      const unsigned long now = millis();
      if (remediationInProgress) {
        const unsigned long waitMs = (remediationActiveLevel == 2) ? 1000UL : 500UL;
        if (advanceChargeCyclePhase(pmic, remediationPhase, remediationPhaseStartMs, waitMs,
                                    remediationActiveLevel == 2, safeToCharge)) {
          Log.info(remediationActiveLevel == 2 ? "PMIC: Charging re-enabled with watchdog supervision"
                                                : "PMIC: Charging re-enabled after soft reset");
          if (remediationActiveLevel == 2) remediationLevel = 0; // Reset level after power cycle attempt.
          remediationInProgress = false;
          remediationActiveLevel = 0;
          remediationPhase = 0;
          lastRemediationAttempt = now;
        }
      } else if (now - lastRemediationAttempt > kRemediationCooldownMs) {
        if (consecutiveFaults >= 3 && remediationLevel < 2) {
          remediationLevel = 2; // Escalate to power cycle reset.
        } else if (consecutiveFaults >= 2 && remediationLevel < 1) {
          remediationLevel = 1; // Escalate to disable/enable charging.
        }

        if (remediationLevel == 1) {
          Log.warn("PMIC: Attempting soft remediation - cycle charging (level 1)");
          requestRemediationCycle(1, now);
        } else if (remediationLevel == 2) {
          Log.error("PMIC: Attempting aggressive remediation - power cycle reset (level 2)");
          requestRemediationCycle(2, now);
        } else {
          Log.info("PMIC: Fault detected but remediation level 0 - monitoring only");
        }
      } else {
        Log.info("PMIC: Fault detected but in cooldown period (%lu min remaining)",
                  (kRemediationCooldownMs - (now - lastRemediationAttempt)) / 60000UL);
      }
    }
  } else {
    // No faults on this read. WO-2026-08-25-001 Amendment C, Decision C3
    // point 2: this previously zeroed remediationInProgress / ActiveLevel /
    // Phase unconditionally whenever CHRG_FAULT read back clear - including
    // the case where a level-1/2 cycle had already run phase 0
    // (pmic.disableCharging()) and was waiting in phase 1 to re-enable.
    // CHRG_FAULT can read clear WHILE charging is disabled (disabling
    // charging is not itself a fault condition), so that unconditional clear
    // could strand charging disabled at the PMIC with no software owner left
    // to finish the re-enable step. Finish any in-progress remediation cycle
    // FIRST; only clear counters once it has actually completed.
    //
    // NOTE: a clear CHRG_FAULT read here does NOT by itself mean it is safe
    // to re-enable charging - the thermal ChargeInhibit path can independently
    // hold safeToCharge==false (e.g. a hot pack) with no PMIC fault at all.
    // advanceChargeCyclePhase() is passed safeToCharge and freezes the
    // sequence (no enableCharging(), state untouched) rather than advancing
    // it while unsafe; see that function's contract above. Alert 22 resolves
    // once the charger leaves Not Charging: route it to the 20-23 clear below.
    if (RecoveryState::get_alertCode() == 22 && regs.chargeStatus != 0 && consecutiveFaults == 0) consecutiveFaults = 1; // latch alert 22: let the 20-23 clear below run, which resets this to 0
    if (remediationInProgress) {
      const unsigned long now = millis();
      const unsigned long waitMs = (remediationActiveLevel == 2) ? 1000UL : 500UL;
      if (advanceChargeCyclePhase(pmic, remediationPhase, remediationPhaseStartMs, waitMs,
                                  remediationActiveLevel == 2, safeToCharge)) {
        Log.info(remediationActiveLevel == 2 ? "PMIC: Charging re-enabled with watchdog supervision"
                                              : "PMIC: Charging re-enabled after soft reset");
        if (remediationActiveLevel == 2) remediationLevel = 0; // Reset level after power cycle attempt.
        remediationInProgress = false;
        remediationActiveLevel = 0;
        remediationPhase = 0;
        lastRemediationAttempt = now;
      }
      // Whether or not this phase completed, do not fall through to the
      // fault-free counter clear below yet - a completed cycle still needs
      // one more no-fault poll to confirm charging stayed healthy before we
      // treat it as recovered, and an incomplete cycle must not have its
      // state clobbered mid-sequence.
      immediateResetConsumed = false;
      immediateResetFaultReg = 0;
    } else if (consecutiveFaults > 0) {
      // No faults, and any remediation cycle already finished cleanly -
      // clear counters if charging is healthy.
      immediateResetConsumed = false;
      immediateResetFaultReg = 0;
      Log.info("PMIC: Charging healthy - clearing fault counters");
      consecutiveFaults = 0;
      remediationLevel = 0;
      remediationInProgress = false;
      remediationActiveLevel = 0;
      remediationPhase = 0;

      const int8_t currentAlert = RecoveryState::get_alertCode();
      if (currentAlert >= 20 && currentAlert <= 23) {
        Log.info("PMIC: Clearing battery/charging alert %d - charging resumed", currentAlert);
        RecoveryState::set_alertCode(0);
        RecoveryState::set_lastAlertTime(0);
      }
    }

    // Narrow recovery path for alert 21 (charge timeout/stuck charging).
    const int8_t currentAlert = RecoveryState::get_alertCode();
    if (currentAlert == 21 && regs.chargeStatus != 2) {
      Log.info("PMIC: clearing alert 21 after charge recovery");
      RecoveryState::set_alertCode(0);
      RecoveryState::set_lastAlertTime(0);
    }
  }

  // USB latch: corrected USB source + VBUS, stuck 2 h below 4.05 V -> one level-1 cycle per 24 h.
  const unsigned long now = millis();
  const bool chargeStuck = usbBackedSource && (regs.vbusStatus == 1 || regs.vbusStatus == 2) && regs.powerGood &&
                           regs.chargeStatus == 0 && !(regs.faultReg & (kPmicChargeFaultMask | kPmicBatFaultMask)) &&
                           vcell < 4.05f && !regs.thermalRegulation && safeToCharge;
  if (!chargeStuck) {
    latchTiming = false;
  } else if (!latchTiming) {
    latchTiming = true;
    latchSince = now;
  } else if (now - latchSince >= 7200000UL && (!latchTried || now - latchLastAttempt >= 86400000UL) &&
             requestRemediationCycle(1, now)) {
    Log.warn("PMIC: Not Charging 2h on USB REG01=0x%02x REG08=0x%02x REG09=0x%02x - cycling charging, alert 22",
             (unsigned)pmic.readPowerONRegister(), (unsigned)regs.systemStatus, (unsigned)regs.faultReg);
    RecoveryState::raiseAlert(22);
    latchTried = true;
    latchLastAttempt = now;
  }

  // BAT_FAULT (bit 3): battery overvoltage protection - a hardware condition,
  // separate from CHRG_FAULT, not resolvable by toggling charging, so it does
  // not increment consecutiveFaults or enter remediation.
  if (regs.faultReg & kPmicBatFaultMask) {
    Log.error("PMIC: Battery overvoltage protection active (BAT_FAULT)");
    RecoveryState::raiseAlert(23);
  }

  return regs;
}
```

</details>


---

<sub>28s</sub>

### Copilot

Defect confirmed by reading: line 342 runs before the `if (remediationInProgress)` branch, so the synthetic 1 survives whenever a cycle is running. Next I'll look at the tests and the harness.

---

<sub>28s</sub>

### `bash`

**Inspect test harness**

$ grep -n "latch alert 22" src/power/PmicFaultMonitor.cpp; cat tests/pmic_fault_monitor_test.sh; wc -l tests/pmic_fault_monitor_test.cpp; ls tests | head -80; ls build-tmp

<details>
<summary>108 lines</summary>

```
342:    if (RecoveryState::get_alertCode() == 22 && regs.chargeStatus != 0 && consecutiveFaults == 0) consecutiveFaults = 1; // latch alert 22: let the 20-23 clear below run, which resets this to 0
#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
binary="$TMPDIR/pmic_fault_monitor_test"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$repo_root/tests/stubs/pmic_fault_monitor_overrides" -I"$repo_root/src" \
  "$repo_root/tests/pmic_fault_monitor_test.cpp" \
  "$repo_root/src/power/PmicFaultMonitor.cpp" \
  "$repo_root/src/power/PowerManager.cpp" \
  "$repo_root/src/power/BatteryAuthorityPolicy.cpp" \
  -o "$binary"

# Each scenario runs as its own process - pollAndRemediate()'s remediation
# state is function-local `static` and persists across calls WITHIN one
# process (by design, modeling escalation across uptime), so scenarios must
# not share a process or they would observe each other's accumulated state.
"$binary" 1
"$binary" 2
"$binary" 3
"$binary" 4
"$binary" 5
"$binary" 6
"$binary" 7
"$binary" 8
"$binary" 9
     593 tests/pmic_fault_monitor_test.cpp
README.md
SensorManagerBatteryAuthorityTests.cpp
alert19_clears_after_report_test.sh
awake_cycle_counters_test.sh
battery_authority_commit_sites_structural_test.py
battery_authority_seam_structural_test.py
battery_authority_structural_test.py
battery_health_test.cpp
battery_health_test.sh
battery_tier_guard_test.cpp
battery_tier_guard_test.sh
boron_soc_commit_test.py
build_flags_witness_test.sh
charge_inhibit_thermal_test.cpp
charge_inhibit_thermal_test.sh
clock_hibernate_helpers_move_structural_test.py
clock_openness_test.cpp
clock_openness_test.sh
clock_owner_structural_test.py
clock_resync_wiring_test.py
clock_rtc_writeback_test.cpp
clock_status_republish_test.cpp
clock_status_republish_test.sh
clock_trust_standard_structural_test.py
clock_trust_test.cpp
close_before_night_sleep_test.py
config_window_hold_test.cpp
config_window_hold_test.sh
conn_phase_modem_off_test.sh
connect_attempt_counter_test.sh
connect_no_modem_wait_structural_test.py
connected_online_last_connection_test.cpp
connected_online_last_connection_test.sh
connection_mode_downgrade_structural_test.py
connection_mode_downgrade_test.cpp
connection_mode_downgrade_test.sh
connection_signal_validity_test.sh
connectivity_failsafe_move_structural_test.py
connectivity_failsafe_open_hours_test.cpp
connectivity_failsafe_open_hours_test.sh
connectivity_failsafe_stage1_retired_test.py
connectivity_failsafe_unknown_openness_test.cpp
connectivity_failsafe_unknown_openness_test.sh
daily_cleanup_boundary_test.py
exerciser_baseline_test.py
facade_header_isolation_test.py
failsafe_cadence_rule_test.cpp
failsafe_cadence_rule_test.sh
firmware_update_dwell_test.sh
firmware_update_wiring_structural_test.py
fixtures
hibernate_cycle_single_owner_structural_test.py
hibernate_wake_diagnostics_test.cpp
hibernate_wake_diagnostics_test.sh
hour_rules_test.cpp
hour_rules_test.sh
hourly_while_occupied_test.cpp
hourly_while_occupied_test.sh
idle_timediag_on_change_test.cpp
idle_timediag_on_change_test.sh
ledger_no_retry_test.cpp
ledger_no_retry_test.sh
loop_forensics_move_structural_test.py
loop_stage_sleep_prep_exclusion_test.py
modem_health_equivalence_test.cpp
modem_health_equivalence_test.sh
modem_health_owner_structural_test.py
modem_teardown_confirmation_logging_test.py
nightly_heap_guard_flush_test.sh
occupancy_ends_at_close_structural_test.py
occupancy_report_by_mode_test.cpp
occupancy_report_by_mode_test.sh
occupancy_session_restart_test.sh
occupancy_start_time_clamp_test.cpp
occupancy_start_time_clamp_test.sh
oom_immediate_reset_test.sh
persisted_layout_preservation_test.py
persistence_facade_behavior_test.cpp
persistence_facade_behavior_test.sh
persistence_facade_fanin_structural_test.py
wo20261102-001-stage6r3
<shellId: 5 completed with exit code 0>
```

</details>


---

<sub>34s</sub>

### `bash`

**Check scratch dir**

$ ls -la build-tmp/wo20261102-001-stage6r3; find build-tmp/wo20261102-001-stage6r3 | head

```
total 0
drwxr-xr-x  2 chipmc  wheel  64 Nov  2 16:06 .
drwxr-xr-x  3 chipmc  wheel  96 Nov  2 16:06 ..
build-tmp/wo20261102-001-stage6r3
<shellId: 6 completed with exit code 0>
```


---

<sub>34s</sub>

### `view`

**view the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/tests/pmic_fault_monitor_test.cpp.**

/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/tests/pmic_fault_monitor_test.cpp (lines 1-130)

<details>
<summary>115 lines</summary>

```
// WO-2026-08-25-001 Amendment C, Decision C3 (Blocker E) host test: exercises
// the actual PRODUCTION src/power/PmicFaultMonitor.cpp::pollAndRemediate(),
// not a reimplementation, against a test-controllable PMIC stub. Covers all
// three C3 fixes:
//   1. thermallyCorrelatedFault must be classified per CHRG_FAULT code, not
//      by whether NTC_FAULT happens to also be nonzero on the same read - a
//      simultaneous independent input fault (0x01) must still be logged/
//      alerted/counted toward remediation even when NTC_FAULT is nonzero.
//   2. The no-fault branch must not abandon (zero out) an in-progress
//      remediation cycle that already disabled charging in phase 0 - it must
//      let that cycle finish (re-enable charging) before clearing counters.
//   3. The fault register is read TWICE and OR'd - a fault bit present on
//      only the first (or only the second) read must still be observed.
//
// Stage 7 R5a fix (this file's scenarios 4-5): the no-fault branch's
// advanceChargeCyclePhase() call previously had NO safeToCharge gate, so it
// could call pmic.enableCharging() on a genuinely unsafe (hot) pack once the
// phase-1 wait elapsed - contradicting PmicFaultMonitor.h's "remediation here
// always yields to safeToCharge == false" contract. The fix freezes (does not
// advance, does not abandon) an in-progress cycle while unsafe, and
// resumes/completes it once safe again. Scenarios 4 (level 1) and 5 (level 2)
// are the permanent regression tests for this - scenario 4 is Codex's
// decisive Stage 7 reproduction made permanent.

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "Particle.h"
#include "power/PmicFaultMonitor.h"
#include "power/PowerPlatform.h"
#include "MyPersistentData.h"

// Definition for the extern declared in tests/stubs/.../power/PowerPlatform.h.
PowerPlatform::TestPowerPlatformState PowerPlatform::testPowerPlatformState;

// Definitions for the two functions PmicFaultMonitor.cpp declares extern
// (real definitions live in src/sensors/SensorManager.cpp, which this host
// test does not link - it is out of scope for the PMIC fault-handling fixes
// under test here).
void boundedBatterySettleDelay(unsigned long) {}
unsigned long currentWakeAwakeMs() { return 0UL; }

namespace {

void resetPmic(PMIC &pmic) {
  pmic.faultRegQueue[0] = 0;
  pmic.faultRegQueue[1] = 0;
  pmic.faultCallCount = 0;
  pmic.systemStatusValue = 0;
  pmic.disableChargingCallCount = 0;
  pmic.enableChargingCallCount = 0;
  pmic.setWatchdogCallCount = 0;
  pmic.readPowerOnCallCount = 0;
}

void resetCurrent() {
  testCurrent.alertCodeValue = 0;
  testCurrent.lastAlertTimeValue = 0;
  testCurrent.internalTempCValue = 20.0f;
  testCurrent.batteryStateValue = 0;
  testCurrent.raiseAlertCallCount = 0;
  testCurrent.lastRaisedAlertCode = -1;
}

// --- Fix 1: per-fault-class classification, not per-register-read. ---

void testIndependentInputFaultIsNotSuppressedByCoincidentNtcFault() {
  PMIC pmic;
  resetPmic(pmic);
  resetCurrent();
  g_testMillis = 4000000UL; // bypass the 1h remediation cooldown gate

  // REG09 fault byte: NTC_FAULT bits(2:0) = 0x01 (nonzero, e.g. TS-network
  // condition), CHRG_FAULT bits(5:4) = 0x01 (Input fault) -> 0b00010001 = 0x11.
  // These are two independent conditions on the same register read.
  //
  // safeToCharge=false (as if ChargeInhibit had it disabled for an unrelated
  // reason) is the exact condition under which the old code's blanket
  // thermallyCorrelatedFault (true whenever NTC != 0) would have suppressed
  // remediation for this independent Input fault, freezing
  // consecutiveFaults/remediation state forever regardless of how many times
  // it recurs. With the fix, an Input fault (chargeFault==0x01) is NOT
  // thermally correlated merely because NTC_FAULT is also nonzero, so
  // remediation proceeds and escalates normally.
  for (int i = 0; i < 3; i++) {
    pmic.faultRegQueue[0] = pmic.faultRegQueue[1] = 0x11;
    pmic.faultCallCount = 0;
    PmicFaultMonitor::pollAndRemediate(pmic, /*safeToCharge=*/false, /*battState=*/0,
                                        /*powerSource=*/0, /*chargeDisableConfigVerified=*/false, /*vcell=*/3.9f);
    g_testMillis += 10;
  }

  // NTC_FAULT alone still raises alert 20 unconditionally either way (that
  // part is unaffected by this fix) - the OBSERVABLE effect this test must
  // catch is whether the independent, non-thermal Input fault was still
  // allowed to escalate remediation. Under the pre-fix blanket
  // classification this never reaches phase 0 (pmic.disableCharging() is
  // never called, no matter how many consecutive faults occur) because
  // `(!safeToCharge && thermallyCorrelatedFault)` is unconditionally true
  // whenever NTC_FAULT != 0, freezing remediation state forever.
  assert(testCurrent.lastRaisedAlertCode == 20);
  assert(pmic.disableChargingCallCount >= 1);
}

// --- Fix 2: no-fault branch must not abandon in-progress remediation. ---

void testNoFaultBranchCompletesInProgressRemediationBeforeClearing() {
  PMIC pmic;
  resetPmic(pmic);
  resetCurrent();

  // Bypass the 1h remediation cooldown gate (`now - lastRemediationAttempt >
  // kRemediationCooldownMs`) on the very first fault - lastRemediationAttempt
  // starts at 0, so millis() must already exceed the cooldown window.
  g_testMillis = 4000000UL;

  // Drive 3 consecutive CHRG_FAULT=0x03 (safety timer) reads with
  // safeToCharge=true so remediation is free to run (not suppressed) and
  // escalates: fault 2 arms level 1 (sets remediationInProgress/phase=0 for
  // the NEXT call); fault 3 is the call that actually runs phase 0
  // (pmic.disableCharging()) via advanceChargeCyclePhase().
  pmic.faultRegQueue[0] = pmic.faultRegQueue[1] = 0x30; // CHRG_FAULT=0x03 << 4
  for (int i = 0; i < 3; i++) {
    PmicFaultMonitor::pollAndRemediate(pmic, /*safeToCharge=*/true, /*battState=*/0,
                                        /*powerSource=*/0, /*chargeDisableConfigVerified=*/false, /*vcell=*/3.9f);
    // Only reset the fault-read cursor for the next call (so getFault() is
    // read fresh) - NOT the cumulative disable/enable call counters, which
    // this test needs to observe across the whole sequence.
    pmic.faultCallCount = 0;
```

</details>


---

<sub>34s</sub>

### `view`

**view the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/tests/pmic_fault_monitor_test.cpp.**

/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/tests/pmic_fault_monitor_test.cpp (lines 300-593)

<details>
<summary>274 lines</summary>

```
  PmicFaultMonitor::pollAndRemediate(pmic, /*safeToCharge=*/true, /*battState=*/0,
                                      /*powerSource=*/0, /*chargeDisableConfigVerified=*/false, /*vcell=*/3.9f);
  assert(pmic.enableChargingCallCount == enableBefore + 1); // resumed, completed exactly once
  assert(pmic.setWatchdogCallCount == 1); // watchdog armed on level-2 resume
  assert(pmic.disableChargingCallCount == 1);
}

// --- WO-2026-11-02-001 (Amendment 1): fault-free USB "Not Charging" latch.
// Driven through the production pollAndRemediate(); its latch state is
// function-local static, so each scenario walks fresh windows sequentially. ---

constexpr unsigned long kH = 3600000UL;
constexpr unsigned long kSettleGap = 25UL * kH; // > the 24 h attempt limit and the 1 h cooldown
constexpr uint8_t kUsbNotCharging = 0x44;       // REG08: VBUS=USB host, Not Charging, power good

struct LatchInput {
  uint8_t status = kUsbNotCharging;
  uint8_t fault = 0;
  float vcell = 3.75f;
  bool safe = true;
  int source = 2; // USB host
};

// One pollAndRemediate() at time t. True if this poll fired the latch
// (raised alert 22 and requested a cycle).
bool latchPoll(PMIC &pmic, unsigned long t, const LatchInput &in = LatchInput()) {
  g_testMillis = t;
  pmic.faultRegQueue[0] = in.fault;
  pmic.faultRegQueue[1] = in.fault;
  pmic.faultCallCount = 0;
  pmic.systemStatusValue = in.status;
  const int raisedBefore = testCurrent.raiseAlertCallCount;
  PmicFaultMonitor::pollAndRemediate(pmic, in.safe, /*battState=*/0, in.source,
                                      /*chargeDisableConfigVerified=*/false, in.vcell);
  return testCurrent.raiseAlertCallCount > raisedBefore && testCurrent.lastRaisedAlertCode == 22;
}

// Breaks the latch timer, lets any requested cycle run to completion
// (phase 0, then re-enable after 500 ms) and returns a start time past the
// 24 h attempt limit and the 1 h cooldown: a fresh latch window.
unsigned long freshWindow(PMIC &pmic, unsigned long t) {
  LatchInput off;
  off.status = 0x00; // no input
  latchPoll(pmic, t, off);
  latchPoll(pmic, t + 600UL, off);
  latchPoll(pmic, t + 1200UL, off);
  return t + kSettleGap;
}

// Stuck from t0: no action before 2 h, exactly one at 2 h.
void assertFiresAtTwoHours(PMIC &pmic, unsigned long t0, const LatchInput &in) {
  assert(!latchPoll(pmic, t0, in));
  assert(!latchPoll(pmic, t0 + 2 * kH - 1, in));
  assert(latchPoll(pmic, t0 + 2 * kH, in));
}

void assertNeverFires(PMIC &pmic, unsigned long t0, const LatchInput &in) {
  for (unsigned long t = t0; t <= t0 + 3 * kH; t += kH / 2) assert(!latchPoll(pmic, t, in));
}

// 2 h boundary (>=), one cycle, then the 24 h per-attempt limit while the
// condition stays continuously true.
void testLatchBoundaryAndAttemptLimit(PMIC &pmic, unsigned long t0) {
  const LatchInput stuck;
  assertFiresAtTwoHours(pmic, t0, stuck);
  const unsigned long fired = t0 + 2 * kH;
  assert(pmic.readPowerOnCallCount == 1); // REG01 read once, for the one Log.warn
  assert(!latchPoll(pmic, fired + 1, stuck)); // cycle phase 0
  assert(pmic.disableChargingCallCount == 1);
  assert(!latchPoll(pmic, fired + 600UL, stuck)); // re-enable completes the cycle
  assert(pmic.enableChargingCallCount == 1);
  assert(!latchPoll(pmic, fired + 2 * kH, stuck)); // cooldown over, 24 h limit holds
  assert(!latchPoll(pmic, fired + 24 * kH - 1, stuck));
  assert(latchPoll(pmic, fired + 24 * kH, stuck));
  assert(pmic.readPowerOnCallCount == 2);
}

// Voltage clause is only < 4.05 V: no lower bound (a deeply discharged cell
// still qualifies), 4.05 V excluded.
void testLatchVoltageThreshold(PMIC &pmic, unsigned long &t) {
  LatchInput in;
  in.vcell = 4.049f;
  assertFiresAtTwoHours(pmic, t, in);
  t = freshWindow(pmic, t + 2 * kH + 1);
  in.vcell = 2.0f;
  assertFiresAtTwoHours(pmic, t, in);
  t = freshWindow(pmic, t + 2 * kH + 1);
  in.vcell = 4.05f;
  assertNeverFires(pmic, t, in);
  t = freshWindow(pmic, t + 3 * kH + 1);
}

// USB gating: corrected USB source AND VBUS 1/2. VBUS=2 (adapter) qualifies;
// raw VBUS=USB with a non-USB corrected source (VIN, battery) never does.
void testLatchUsbGating(PMIC &pmic, unsigned long &t) {
  LatchInput in;
  in.status = 0x84; // VBUS=adapter
  assertFiresAtTwoHours(pmic, t, in);
  t = freshWindow(pmic, t + 2 * kH + 1);
  in.status = kUsbNotCharging;
  in.source = 3; // USB adapter
  assertFiresAtTwoHours(pmic, t, in);
  t = freshWindow(pmic, t + 2 * kH + 1);
  for (int source : {1, 5, -1}) { // VIN, battery, unknown
    in.source = source;
    assertNeverFires(pmic, t, in);
    t = freshWindow(pmic, t + 3 * kH + 1);
  }
}

// Each clause breaking once (at +1 h) restarts the timer: no action at the
// original 2 h mark, one action 2 h after the clause is restored.
void testLatchEachClauseBreakResetsTimer(PMIC &pmic, unsigned long &t) {
  for (int clause = 0; clause < 13; clause++) {
    LatchInput bad;
    switch (clause) {
      case 0: bad.status = 0x04; break;  // no VBUS
      case 1: bad.status = 0xC4; break;  // OTG is not input
      case 2: bad.status = 0x40; break;  // power not good
      case 3: bad.status = 0x54; break;  // Pre-charge
      case 4: bad.status = 0x64; break;  // Fast
      case 5: bad.status = 0x74; break;  // Done
      case 6: bad.status = 0x46; break;  // thermal regulation
      case 7: bad.fault = 0x10; break;   // CHRG_FAULT
      case 8: bad.fault = 0x08; break;   // BAT_FAULT
      case 9: bad.vcell = 4.05f; break;
      case 10: bad.safe = false; break;
      case 11: bad.source = 1; break;    // corrected source VIN
      case 12: bad.status = 0x84; bad.source = 5; break; // VBUS adapter, source battery
    }
    const LatchInput good;
    assert(!latchPoll(pmic, t, good));
    assert(!latchPoll(pmic, t + kH, bad));
    assert(!latchPoll(pmic, t + kH + 1, good));
    assert(!latchPoll(pmic, t + 2 * kH, good));
    assert(!latchPoll(pmic, t + 3 * kH, good));
    assert(latchPoll(pmic, t + 3 * kH + 1, good));
    t = freshWindow(pmic, t + 3 * kH + 2);
  }
}

// millis() wrap-around: the timer straddles the unsigned long wrap (host
// unsigned long is 64-bit, so this wraps at 2^64; the device wraps at 2^32 -
// both rely on the same modular unsigned subtraction).
void testLatchAcrossMillisWrap(PMIC &pmic) {
  const unsigned long t0 = ~0UL - kH / 2; // 30 min before the wrap
  assertFiresAtTwoHours(pmic, t0, LatchInput());
  assert(t0 + 2 * kH < t0); // the firing time really is past the wrap
}

void testLatchScenario() {
  PMIC pmic;
  resetPmic(pmic);
  resetCurrent();
  unsigned long t = 1000UL;
  testLatchBoundaryAndAttemptLimit(pmic, t);
  t = freshWindow(pmic, t + 26 * kH + 1);
  testLatchVoltageThreshold(pmic, t);
  testLatchUsbGating(pmic, t);
  testLatchEachClauseBreakResetsTimer(pmic, t);
  testLatchAcrossMillisWrap(pmic);
}

// Alert 22 (not 21) is raised; it stays while Not Charging persists and is
// cleared by the existing "charging resumed, codes 20-23" clear once the
// charger reports Pre-charge, Fast or Done.
void testLatchAlert22ClearsWhenChargingResumes() {
  PMIC pmic;
  resetPmic(pmic);
  resetCurrent();
  unsigned long t = 1000UL;
  const LatchInput stuck;
  for (uint8_t resumed : {0x54, 0x64, 0x74}) { // Pre-charge, Fast, Done
    const int raisedBefore = testCurrent.raiseAlertCallCount;
    assertFiresAtTwoHours(pmic, t, stuck);
    assert(testCurrent.alertCodeValue == 22 && testCurrent.lastRaisedAlertCode == 22);
    assert(testCurrent.raiseAlertCallCount == raisedBefore + 1);
    const unsigned long fired = t + 2 * kH;
    latchPoll(pmic, fired + 1, stuck);      // phase 0
    latchPoll(pmic, fired + 600UL, stuck);  // re-enable
    latchPoll(pmic, fired + 1200UL, stuck); // still Not Charging
    assert(testCurrent.alertCodeValue == 22);
    LatchInput charging;
    charging.status = resumed;
    latchPoll(pmic, fired + 1800UL, charging);
    assert(testCurrent.alertCodeValue == 0);
    assert(testCurrent.raiseAlertCallCount == raisedBefore + 1);
    t = fired + kSettleGap;
  }
  assert(pmic.disableChargingCallCount == 3 && pmic.enableChargingCallCount == 3);
}

// The legacy alert-21 clear is unchanged: cleared on any no-fault poll
// whose charge status is not Fast (2) - including Not Charging - and the
// latch never raises or holds 21.
void testAlert21ClearUnchanged() {
  PMIC pmic;
  resetPmic(pmic);
  resetCurrent();
  LatchInput in;
  in.source = 1; // keep the latch out of it
  unsigned long t = 1000UL;
  in.status = 0x64; // Fast keeps 21
  testCurrent.alertCodeValue = 21;
  latchPoll(pmic, t++, in);
  assert(testCurrent.alertCodeValue == 21);
  for (uint8_t status : {0x44, 0x54, 0x74}) { // Not Charging, Pre-charge, Done clear 21
    testCurrent.alertCodeValue = 21;
    in.status = status;
    latchPoll(pmic, t++, in);
    assert(testCurrent.alertCodeValue == 0);
  }
  assert(testCurrent.raiseAlertCallCount == 0);
}

// A latch request inside the 1 h cooldown after a remediation attempt does
// not start a second cycle: an immediate charge-fault reset that reads back
// clear leaves the mature latch timer running, so without the cooldown the
// latch would cycle charging on that same poll.
void testLatchHonoursRemediationCooldown() {
  PMIC pmic;
  resetPmic(pmic);
  resetCurrent();
  const unsigned long t0 = 1000UL;
  const LatchInput stuck;
  assert(!latchPoll(pmic, t0, stuck));
  assert(!latchPoll(pmic, t0 + kH, stuck));

  // At the 2 h mark: CHRG_FAULT on the first read pair, clear after the
  // immediate reset's re-read (stub returns faultRegQueue[1] from then on).
  const unsigned long reset = t0 + 2 * kH;
  g_testMillis = reset;
  pmic.faultRegQueue[0] = 0x10;
  pmic.faultRegQueue[1] = 0x00;
  pmic.faultCallCount = 0;
  pmic.systemStatusValue = kUsbNotCharging;
  PmicFaultMonitor::pollAndRemediate(pmic, true, 0, 2, false, 3.75f);
  assert(pmic.faultCallCount == 4); // immediate reset ran and re-read
  assert(pmic.disableChargingCallCount == 1 && pmic.enableChargingCallCount == 1);
  assert(testCurrent.raiseAlertCallCount == 0);

  assert(!latchPoll(pmic, reset + 1, stuck));
  assert(!latchPoll(pmic, reset + kH / 2, stuck));
  assert(!latchPoll(pmic, reset + kH, stuck)); // cooldown is "more than 1 h"
  assert(pmic.disableChargingCallCount == 1); // no second cycle started
  assert(latchPoll(pmic, reset + kH + 1, stuck));
  assert(!latchPoll(pmic, reset + kH + 2, stuck));
  assert(pmic.disableChargingCallCount == 2);
}

} // namespace

// pollAndRemediate() uses function-local `static` remediation state that
// persists across every call within one process (by design - it models
// escalation across a device's uptime). Each scenario below therefore runs
// in its OWN process (selected by argv[1]) so its statics start fresh,
// rather than sharing accumulated state with the other scenarios.
int main(int argc, char **argv) {
  const int scenario = (argc > 1) ? atoi(argv[1]) : 0;
  switch (scenario) {
    case 1:
      testIndependentInputFaultIsNotSuppressedByCoincidentNtcFault();
      break;
    case 2:
      testNoFaultBranchCompletesInProgressRemediationBeforeClearing();
      break;
    case 3:
      testFaultRegisterIsReadTwiceAndOred();
      break;
    case 4:
      testNoFaultBranchFreezesLevel1WhileUnsafeThenResumes();
      break;
    case 5:
      testNoFaultBranchFreezesLevel2WhileUnsafeThenResumesWithWatchdog();
      break;
    case 6:
      testLatchScenario();
      break;
    case 7:
      testLatchAlert22ClearsWhenChargingResumes();
      break;
    case 8:
      testAlert21ClearUnchanged();
      break;
    case 9:
      testLatchHonoursRemediationCooldown();
      break;
    default:
      fprintf(stderr, "usage: %s <1-9>\n", argv[0]);
      return 2;
  }
  printf("pmic_fault_monitor_test: scenario %d passed\n", scenario);
  return 0;
}
```

</details>


---

<sub>1m 8s</sub>

### Copilot

Now checking the test stub's `raiseAlert` semantics before writing scenario 10.

---

<sub>1m 8s</sub>

### `bash`

**Inspect raiseAlert stub**

$ grep -n "raiseAlert" -A12 tests/stubs/pmic_fault_monitor_overrides/*.h | head -40; ls tests/stubs/pmic_fault_monitor_overrides/

<details>
<summary>34 lines</summary>

```
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h:16:  int raiseAlertCallCount = 0;
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-17-  int lastRaisedAlertCode = -1;
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-18-
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-19-  int8_t get_alertCode() const { return alertCodeValue; }
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-20-  void set_alertCode(int8_t v) { alertCodeValue = v; }
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-21-  void set_lastAlertTime(time_t v) { lastAlertTimeValue = v; }
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-22-  float get_internalTempC() const { return internalTempCValue; }
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-23-  void set_batteryState(uint8_t v) { batteryStateValue = v; }
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-24-  float get_stateOfCharge() const { return 50.0f; }
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-25-  uint8_t get_batteryState() const { return batteryStateValue; }
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-26-
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h:27:  void raiseAlert(int8_t code) {
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-28-    alertCodeValue = code;
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h:29:    raiseAlertCallCount++;
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-30-    lastRaisedAlertCode = code;
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-31-  }
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-32-};
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-33-
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-34-struct TestSystemStatus {
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-35-  bool verboseMode = false;
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-36-  bool solarPowerMode = false;
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-37-  uint8_t sensorMode = 0;
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-38-  uint8_t connectionMode = 0;
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-39-  bool lowBatteryMode = false;
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-40-
tests/stubs/pmic_fault_monitor_overrides/MyPersistentData.h-41-  bool get_verboseMode() const { return verboseMode; }
Config.h
MyPersistentData.h
Particle.h
device_pinout.h
observability
persist
power
<shellId: 7 completed with exit code 0>
```

</details>


---

<sub>1m 13s</sub>

### Copilot

Applying the fix to line 342. It stays one line.

---

<sub>1m 13s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/src/power/PmicFaultMonitor.cpp.**

/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/src/power/PmicFaultMonitor.cpp

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/src/power/PmicFaultMonitor.cpp updated with changes.
```


---

<sub>1m 29s</sub>

### Copilot

Fix applied. Now adding scenario 10.

---

<sub>1m 29s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/tests/pmic_fault_monitor_test.cpp.**

/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/tests/pmic_fault_monitor_test.cpp

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/tests/pmic_fault_monitor_test.cpp updated with changes.
```


---

<sub>1m 29s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/tests/pmic_fault_monitor_test.cpp.**

/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/tests/pmic_fault_monitor_test.cpp

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/tests/pmic_fault_monitor_test.cpp updated with changes.
```


---

<sub>1m 29s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/tests/pmic_fault_monitor_test.sh.**

/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/tests/pmic_fault_monitor_test.sh

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/tests/pmic_fault_monitor_test.sh updated with changes.
```


---

<sub>1m 39s</sub>

### Copilot

Building and running the PMIC test with scratch `TMPDIR`.

---

<sub>1m 41s</sub>

### `bash`

**Run PMIC tests**

$ S=$PWD/build-tmp/wo20261102-001-stage6r3; mkdir -p $S/tmp && export TMPDIR=$S/tmp && zsh tests/pmic_fault_monitor_test.sh && python3 tests/pmic_latch_call_site_structural_test.py; echo rc=$?

<details>
<summary>13 lines</summary>

```
pmic_fault_monitor_test: scenario 1 passed
pmic_fault_monitor_test: scenario 2 passed
pmic_fault_monitor_test: scenario 3 passed
pmic_fault_monitor_test: scenario 4 passed
pmic_fault_monitor_test: scenario 5 passed
pmic_fault_monitor_test: scenario 6 passed
pmic_fault_monitor_test: scenario 7 passed
pmic_fault_monitor_test: scenario 8 passed
pmic_fault_monitor_test: scenario 9 passed
pmic_fault_monitor_test: scenario 10 passed
pmic_latch_call_site_structural_test: passed
rc=0
<shellId: 8 completed with exit code 0>
```

</details>


---

<sub>1m 49s</sub>

### Copilot

All 10 scenarios and the structural test pass. Next I'll pull the round-2 mutation definitions from the transcript so I can re-run them.

---

<sub>1m 49s</sub>

### `bash`

**Find round-2 mutation script**

$ grep -n "mutat\|& 1\|sed \|perl " docs/work-orders/WO-2026-11-02-001-stage6-round2-transcript.md | head -60; head -30 tests/pmic_latch_call_site_structural_test.py

<details>
<summary>82 lines</summary>

```
50:1. **Alert 22, not 21.** Raise `RecoveryState::raiseAlert(22)` when the latch fires. Add `case 22:` with the PMIC group in the severity function in `src/MyPersistentData.cpp` (~line 781, severity 3, same as 20 and 21). The existing "charging resumed" clear (`PmicFaultMonitor.cpp` ~367, codes 20..23) must stay untouched and must clear 22 with no new clear code. **Restore the alert-21 clear to the original `regs.chargeStatus != 2` exactly** (revert the round-1 `& 1` edit and its comment).
56:7. **Tests:** update scenario 6 (clauses, boundaries, 24 h; add the USB-gating and VBUS clauses, and a millis wrap-around case if the host type can model it) and **scenario 7 must expect alert 22 to clear when charging resumes** (Pre-charge, Fast and Done each, via the existing 20-23 clear). Add: alert 21 clear unchanged (`chargeStatus != 2` semantics, alert 21 is not touched by the latch), and a cooldown case (a latch request within 1 h of a remediation attempt does not start a second cycle). Mutations (copies in `$TMPDIR`, never `src/`): the earlier set (2 h `>=`/`>`, 4.05 V `<`/`<=`, drop `powerGood`, drop the 24 h limit, drop the timer reset), plus drop the shared USB-context clause, raise 21 instead of 22, and restore the `& 1` clear; each must fail a test. Update the structural test for the new signature and the alert code.
59:**Report (final message):** per file net and gross `src/` lines with the arithmetic and the total against +30; test lines; each mutation and its catching check; how the cooldown/seam is wired (file:line); the model and reasoning level actually used; deviations or "none".
202:   Directly assigning the three remediation fields at `:387` is not the requested exposed seam and bypasses the explicit 1 h cooldown at `:291`. Usually the 2 h qualification subsumes it, but an immediate fault reset can refresh `regs` to clear without resetting an already-mature latch timer, allowing a latch cycle less than 1 h later. `!remediationInProgress` and the 24 h timestamp stop ordinary refires. The latch itself does not escalate; a genuine charge fault appearing mid-cycle enters the existing fault path and may later escalate independently.
206:   Changing `chargeStatus != 2` to `(chargeStatus & 1)` at `PmicFaultMonitor.cpp:374–379` is an out-of-scope change to the existing fault/stuck-fast path. It now clears only PRE/DONE (1/3), while retaining both Not Charging (0) and Fast (2). Fast was already retained; Not Charging is the regression. A persisted or stuck-fast-origin alert can now remain forever after transitioning to Not Charging and mask lower alerts. A latch recovery that goes directly to Fast also remains raised despite charging having resumed.
212:6. **Default parameter — Medium:** The sole production caller passes `vcell` (`SensorManager.cpp:716–719`), and the structural test passed with exit 0. Nevertheless, `float vcell=99.0f` at `PmicFaultMonitor.h:49–50` lets any future caller compile while silently disabling latch recovery. The structural test scans only current `src/*.cpp` calls. Remove the default and update older host calls to pass an explicit value.
214:7. **Tests:** Scenario 6 pins the exact 2 h boundary, exact 24 h boundary, `<4.05`, power-good, and timer-reset behavior (`pmic_fault_monitor_test.cpp:323–371`); by inspection, each requested mutation would violate an assertion. Thermal and `safeToCharge` timer breaks are covered at `:361–362`. Gaps include wrap-around, positive VBUS=2, corrected USB-source/profile gating, cooldown interleaving, fault appearance mid-cycle, and all legacy alert-clear transitions. Scenario 7 currently enshrines the global sticky-Not-Charging behavior.
216:   The requested host command exited 1 because the read-only sandbox prevented Clang from creating `/tmp` cache/temporary files. `$TMPDIR` mutation-copy creation was likewise denied, so mutation failures and Copilot’s ARM build claim could not be independently executed.
275:+    if (currentAlert == 21 && (regs.chargeStatus & 1)) { // Not Charging keeps the latch alert
411:// (Device OS applies this asynchronously - see ChargeInhibit.h). Used as a
547:  // a fault bit that is only briefly latched is not silently missed by a
705:    // ntcFaultCode into this flag previously suppressed remediation for that
792:    // advanceChargeCyclePhase() is passed safeToCharge and freezes the
837:    if (currentAlert == 21 && (regs.chargeStatus & 1)) { // Not Charging keeps the latch alert
1095:$ sed -n 760,830p src/MyPersistentData.cpp; wc -l tests/pmic_fault_monitor_test.cpp
1163:        // alert were raised while the clock is untrusted and this wrote 0,
1283:// phase-1 wait elapsed - contradicting PmicFaultMonitor.h's "remediation here
1471:// call pmic.enableCharging() unconditionally once the phase-1 wait elapsed -
1660:// Through pollAndRemediate(): one soft cycle, alert 21 raised and kept while
1778:$ for f in src/power/ConnectivityFailsafe.cpp:152 src/state/State_Connect.cpp:519 src/state/State_Connect.cpp:562 src/state/State_Error.cpp:131 src/Generalized-Core-Counter.cpp:582 src/Generalized-Core-Counter.cpp:605 src/Generalized-Core-Counter.cpp:920 src/cloud/ReportPublisher.cpp:150 src/cloud/ReportPublisher.cpp:300; do file=${f%%:*}; ln=${f##*:}; echo "== $f"; sed -n "$((ln-8)),$((ln+1))p" $file; done; grep -n "Clear\|clear" docs/reference/alert-codes.md | head -20
1853:  // Alert 44 is raised late in the previous wake cycle, after the normal
1862:  // If the underlying condition persists, it will be raised again.
1887:27:| Code | Meaning | Raised when (file:line) | Severity (M) | First raised | Type | Clears |
1888:34:| 18 | state-machine thrash | ThrashGuard tier 2 (T:141); tier 3, then an immediate reset (T:150) | 3 (:864) | v4.05 (`98d98d8`) | event | **Clears: never (planned fix with ThrashGuard persistence).** No clear site, in any version. It's raised just before the tier-3 reset, so it may not be saved. Once the ThrashGuard/persistence work saves state before a deliberate reset, 18 becomes report-once like 19 and 42. A stuck 18 is a known limitation, not an ongoing fault. |
1889:35:| 19 | watchdog reset | Boot classified as a Device OS watchdog or an AB1805-confirmed pin reset (G:1317) | **4, the highest** (:876–877) | committed `75ad0a8`; first released in v22-Diag-Soak (`3b9b39f`) | event | **Report once, then clear, since v38** (`isAutoClearAfterReportAlert()`; raise-site comment G:1310–1316). **Before v38: never** (deliberately excluded), so a device on older firmware keeps 19 and it hides every lower alert. |
1894:66:5. **22:** missing from the fleet-ops table. **Added:** never raised and not in the severity table, but inside the PMIC clear range.
2220:$ grep -n "advanceChargeCyclePhase() is passed safeToCharge" -A6 src/power/PmicFaultMonitor.cpp
2226:340:    // advanceChargeCyclePhase() is passed safeToCharge and freezes the
2406:+  // Shared level-1/2 cycle request (fault path, charge latch); refused while one runs or in the 1 h cooldown.
2455:     // advanceChargeCyclePhase() is passed safeToCharge and freezes the
2586:// (raised alert 22 and requested a cycle).
2858:sed -i '' 's/  pmic.setWatchdogCallCount = 0;\n}/X/' $f
2976:    sys.exit("FAIL: alert 22 must be raised only by the charge latch")
3000:Structural test passes. Now running the mutation suite on copies under `$TMPDIR`. `src/` is never touched.
3008:**Run mutation tests in TMPDIR copies**
3010:$ cat > "$TMPDIR/mutate.py" <<'EOF'
3023: "restore & 1 clear": (P, "currentAlert == 21 && regs.chargeStatus != 2)", "currentAlert == 21 && (regs.chargeStatus & 1))"),
3060:python3 "$TMPDIR/mutate.py"; echo "exit=$?"
3073:CAUGHT   restore & 1 clear: scenario 8 (line 510); structural: FAIL: the legacy alert-21 clear must stay `regs.chargeStatus != 2`
3097:All 18 mutations are caught, including the 8 required ones. Mapping the line numbers to their asserts for the report:
3107:$ for l in 351 353 357 371 434 435 485 510 539 545; do printf "%s: " $l; sed -n "${l}p" tests/pmic_fault_monitor_test.cpp; done
3176:AI_DEVELOPMENT_WORKFLOW.md-104-  with `-DENABLE_DIAGNOSTICS_PUBLISH_MODE=1` reused the release objects. In
3190:AI_DEVELOPMENT_WORKFLOW.md-118-  used to say `make clean-user`, which has no effect on agents' scratch
3208:AI_DEVELOPMENT_WORKFLOW.md-437-This is also the build that produces the symbolised ELF the linkage check above
3217:AI_DEVELOPMENT_WORKFLOW.md-446-acceptance test, on every side, used the cloud compiler. Codex had flagged the
3228:AI_DEVELOPMENT_WORKFLOW.md:468:  `BUILD_PATH_BASE`, and the linkage check (`nm` on the symbolised ELF). A WO's
3257:$ sed -n 410,436p AI_DEVELOPMENT_WORKFLOW.md; ls build-tmp 2>/dev/null | head
3266:two of its four functions absent from the artifact, having passed every other
3404:/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/build-tmp/wo20261102-001-stage6r2/tree/lib/LocalTimeRK/src/LocalTimeRK.cpp:1244:12: warning: unused variable 'origTime' [-Wunused-variable]
3408:/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/build-tmp/wo20261102-001-stage6r2/tree/lib/LocalTimeRK/src/LocalTimeRK.cpp:1255:12: warning: unused variable 'origTime' [-Wunused-variable]
3514:rm -rf "$S" && rm -f "$TMPDIR/mutate.py"; ls -la build-tmp; git status --short
3610:121:  // Shared level-1/2 cycle request (fault path, charge latch); refused while one runs or in the 1 h cooldown.
3651:| restore the `& 1` clear | scenario 8 (Not Charging clears 21) and the structural test |
3658:- **Latch:** P:401–403 calls it. If refused, firing is put off; the timer keeps running and the 24 h attempt is not used up.
3665:2. **USB gating split.** Only profile/`isUsbBackedSource` ×2 is shared (P:134). The fault path still adds raw VBUS (P:143), so its decisions don't change. Sharing the whole expression would make the latch gate raw VBUS only, which WO Amendment 1 rule 2 forbids, and the "drop USB-context" mutation could never be caught.
#!/usr/bin/env python3
"""WO-2026-11-02-001 (Amendment 1): pollAndRemediate() takes vcell as a required parameter (no
default that could silently disable the latch recovery), the one production call passes vcell, the
latch raises alert 22 (not 21), and alert 22 ranks at severity 3 with 20 and 21."""
import os
import re
import sys
from pathlib import Path

ROOT = Path(os.environ.get("PMIC_LATCH_SRC_ROOT") or Path(__file__).resolve().parent.parent)
SRC = ROOT / "src"

header = (SRC / "power/PmicFaultMonitor.h").read_text(errors="replace")
decl = re.search(r"Registers pollAndRemediate\(([^)]*)\);", header)
if not decl:
    sys.exit("FAIL: pollAndRemediate declaration not found")
if not re.search(r",\s*float vcell\s*$", decl.group(1)) or "=" in decl.group(1):
    sys.exit("FAIL: pollAndRemediate must end with a required `float vcell` parameter (no defaults)")

calls = []
for p in sorted(SRC.rglob("*.cpp")):
    if p.name == "PmicFaultMonitor.cpp":
        continue
    calls += re.findall(r"PmicFaultMonitor::pollAndRemediate\((pmic,[^;]*?)\);", p.read_text(errors="replace"), re.S)
if len(calls) != 1:
    sys.exit(f"FAIL: expected 1 production pollAndRemediate call, found {len(calls)}")
if not re.search(r",\s*vcell\s*$", calls[0].strip()):
    sys.exit("FAIL: pollAndRemediate call must pass vcell as its last argument")

monitor = (SRC / "power/PmicFaultMonitor.cpp").read_text(errors="replace")
<shellId: 9 completed with exit code 0>
```

</details>


---

<sub>1m 50s</sub>

### `bash`

**Read round-2 mutation script**

$ sed -n 3008,3100p docs/work-orders/WO-2026-11-02-001-stage6-round2-transcript.md

````
**Run mutation tests in TMPDIR copies**

$ cat > "$TMPDIR/mutate.py" <<'EOF'
import os, re, shutil, subprocess, sys, tempfile
from pathlib import Path
repo = Path.cwd()
P = "src/power/PmicFaultMonitor.cpp"
muts = {
 "2h >= -> >": (P, "now - latchSince >= 7200000UL", "now - latchSince > 7200000UL"),
 "4.05 < -> <=": (P, "vcell < 4.05f", "vcell <= 4.05f"),
 "drop powerGood": (P, "regs.vbusStatus == 2) && regs.powerGood &&", "regs.vbusStatus == 2) &&"),
 "drop 24h limit": (P, "(!latchTried || now - latchLastAttempt >= 86400000UL) &&\n             ", ""),
 "drop timer reset": (P, "  if (!chargeStuck) {\n    latchTiming = false;\n", "  if (!chargeStuck) {\n"),
 "drop shared USB context": (P, "usbBackedSource && (regs.vbusStatus == 1", "(regs.vbusStatus == 1"),
 "raise 21 not 22": (P, "RecoveryState::raiseAlert(22);", "RecoveryState::raiseAlert(21);"),
 "restore & 1 clear": (P, "currentAlert == 21 && regs.chargeStatus != 2)", "currentAlert == 21 && (regs.chargeStatus & 1))"),
 "drop VBUS 1/2 clause": (P, "(regs.vbusStatus == 1 || regs.vbusStatus == 2) && ", ""),
 "VBUS 1/2/3 (allow OTG)": (P, "(regs.vbusStatus == 1 || regs.vbusStatus == 2) &&", "(regs.vbusStatus != 0) &&"),
 "drop chargeStatus==0": (P, "regs.chargeStatus == 0 && !(regs.faultReg", "!(regs.faultReg"),
 "drop fault-bit clause": (P, " && !(regs.faultReg & (kPmicChargeFaultMask | kPmicBatFaultMask)) &&", " &&"),
 "drop thermal clause": (P, "vcell < 4.05f && !regs.thermalRegulation && safeToCharge", "vcell < 4.05f && safeToCharge"),
 "drop safeToCharge": (P, "!regs.thermalRegulation && safeToCharge;", "!regs.thermalRegulation;"),
 "drop cooldown in request": (P, "if (remediationInProgress || now - lastRemediationAttempt <= kRemediationCooldownMs) return false;", "if (remediationInProgress) return false;"),
 "drop alert-22 resolve route": (P, "    if (RecoveryState::get_alertCode() == 22 && regs.chargeStatus != 0 && consecutiveFaults == 0) consecutiveFaults = 1;\n", ""),
 "drop severity case 22": ("src/MyPersistentData.cpp", "        case 22: // PMIC USB Not Charging 2 h (charge latch)\n", ""),
 "re-add vcell default": ("src/power/PmicFaultMonitor.h", "float vcell);", "float vcell = 99.0f);"),
}
base = Path(tempfile.mkdtemp(prefix="pmicmut-", dir=os.environ["TMPDIR"]))
results = []
for name, (f, old, new) in muts.items():
    d = base / re.sub(r"\W+", "_", name)
    shutil.copytree(repo / "src", d / "src"); shutil.copytree(repo / "tests", d / "tests")
    t = (d / f).read_text(); assert t.count(old) == 1, (name, t.count(old)); (d / f).write_text(t.replace(old, new))
    env = dict(os.environ, TMPDIR=str(d)); fails = []
    b = d / "bin"
    c = subprocess.run(["clang++","-std=c++17","-I",str(d/"tests/stubs/pmic_fault_monitor_overrides"),"-I",str(d/"src"),
        str(d/"tests/pmic_fault_monitor_test.cpp"),str(d/"src/power/PmicFaultMonitor.cpp"),str(d/"src/power/PowerManager.cpp"),
        str(d/"src/power/BatteryAuthorityPolicy.cpp"),"-o",str(b)],capture_output=True,text=True)
    if c.returncode: fails.append("compile")
    else:
        for s in range(1,10):
            r = subprocess.run([str(b), str(s)], capture_output=True, text=True)
            if r.returncode:
                m = re.search(r"pmic_fault_monitor_test\.cpp, line (\d+)|pmic_fault_monitor_test\.cpp:(\d+)", r.stderr)
                fails.append(f"scenario {s} (line {(m.group(1) or m.group(2)) if m else '?'})")
    r = subprocess.run([sys.executable, str(d/"tests/pmic_latch_call_site_structural_test.py")], env=dict(env, PMIC_LATCH_SRC_ROOT=str(d)), capture_output=True, text=True)
    if r.returncode: fails.append("structural: " + (r.stdout + r.stderr).strip().splitlines()[-1])
    results.append((name, fails))
    print(f"{'CAUGHT' if fails else 'SURVIVED':8} {name}: {'; '.join(fails) or '-'}", flush=True)
shutil.rmtree(base)
sys.exit(1 if any(not f for _, f in results) else 0)
EOF
python3 "$TMPDIR/mutate.py"; echo "exit=$?"

<details>
<summary>20 lines</summary>

```
CAUGHT   2h >= -> >: scenario 6 (line 353); scenario 7 (line 353)
CAUGHT   4.05 < -> <=: scenario 6 (line 357)
CAUGHT   drop powerGood: scenario 6 (line 434)
CAUGHT   drop 24h limit: scenario 6 (line 371)
CAUGHT   drop timer reset: scenario 6 (line 351); scenario 7 (line 351)
CAUGHT   drop shared USB context: scenario 6 (line 357)
CAUGHT   raise 21 not 22: scenario 6 (line 353); scenario 7 (line 353); scenario 9 (line 545); structural: FAIL: the charge latch must raise alert 22 (and only 22)
CAUGHT   restore & 1 clear: scenario 8 (line 510); structural: FAIL: the legacy alert-21 clear must stay `regs.chargeStatus != 2`
CAUGHT   drop VBUS 1/2 clause: scenario 6 (line 434)
CAUGHT   VBUS 1/2/3 (allow OTG): scenario 6 (line 434)
CAUGHT   drop chargeStatus==0: scenario 6 (line 434); scenario 7 (line 351)
CAUGHT   drop fault-bit clause: scenario 6 (line 435)
CAUGHT   drop thermal clause: scenario 6 (line 434)
CAUGHT   drop safeToCharge: scenario 6 (line 434)
CAUGHT   drop cooldown in request: scenario 9 (line 539)
CAUGHT   drop alert-22 resolve route: scenario 7 (line 485)
CAUGHT   drop severity case 22: structural: FAIL: alert 22 must share severity 3 with alerts 20 and 21
CAUGHT   re-add vcell default: structural: FAIL: pollAndRemediate must end with a required `float vcell` parameter (no defaults)
exit=0
<shellId: 21 completed with exit code 0>
```

</details>


---

<sub>10m 16s</sub>

### Copilot

All 18 mutations are caught, including the 8 required ones. Mapping the line numbers to their asserts for the report:

---
<shellId: 10 completed with exit code 0>
````


---

<sub>2m 10s</sub>

### Copilot

Running the mutation set (round 2's 18, updated for scenario 10, plus the new required mutation and an isolation variant for check (ii)).

---

<sub>2m 44s</sub>

### `bash`

**Run mutation suite**

$ S=$PWD/build-tmp/wo20261102-001-stage6r3; export TMPDIR=$S/tmp; cat > "$S/mutate.py" <<'EOF'
import os, re, shutil, subprocess, sys, tempfile
from pathlib import Path
repo = Path.cwd()
P = "src/power/PmicFaultMonitor.cpp"
T = "tests/pmic_fault_monitor_test.cpp"
L342 = "    if (RecoveryState::get_alertCode() == 22 && regs.chargeStatus != 0 && consecutiveFaults == 0 && !remediationInProgress) consecutiveFaults = 1;"
muts = {
 "drop !remediationInProgress (REQUIRED)": [(P, " && !remediationInProgress) consecutiveFaults = 1;", ") consecutiveFaults = 1;")],
 "drop !remediationInProgress, check (i) disabled": [(P, " && !remediationInProgress) consecutiveFaults = 1;", ") consecutiveFaults = 1;"),
     (T, "  assert(testCurrent.alertCodeValue == 22); // (i)\n", "")],
 "2h >= -> >": [(P, "now - latchSince >= 7200000UL", "now - latchSince > 7200000UL")],
 "4.05 < -> <=": [(P, "vcell < 4.05f", "vcell <= 4.05f")],
 "drop powerGood": [(P, "regs.vbusStatus == 2) && regs.powerGood &&", "regs.vbusStatus == 2) &&")],
 "drop 24h limit": [(P, "(!latchTried || now - latchLastAttempt >= 86400000UL) &&\n             ", "")],
 "drop timer reset": [(P, "  if (!chargeStuck) {\n    latchTiming = false;\n", "  if (!chargeStuck) {\n")],
 "drop shared USB context": [(P, "usbBackedSource && (regs.vbusStatus == 1", "(regs.vbusStatus == 1")],
 "raise 21 not 22": [(P, "RecoveryState::raiseAlert(22);", "RecoveryState::raiseAlert(21);")],
 "restore & 1 clear": [(P, "currentAlert == 21 && regs.chargeStatus != 2)", "currentAlert == 21 && (regs.chargeStatus & 1))")],
 "drop VBUS 1/2 clause": [(P, "(regs.vbusStatus == 1 || regs.vbusStatus == 2) && ", "")],
 "VBUS 1/2/3 (allow OTG)": [(P, "(regs.vbusStatus == 1 || regs.vbusStatus == 2) &&", "(regs.vbusStatus != 0) &&")],
 "drop chargeStatus==0": [(P, "regs.chargeStatus == 0 && !(regs.faultReg", "!(regs.faultReg")],
 "drop fault-bit clause": [(P, " && !(regs.faultReg & (kPmicChargeFaultMask | kPmicBatFaultMask)) &&", " &&")],
 "drop thermal clause": [(P, "vcell < 4.05f && !regs.thermalRegulation && safeToCharge", "vcell < 4.05f && safeToCharge")],
 "drop safeToCharge": [(P, "!regs.thermalRegulation && safeToCharge;", "!regs.thermalRegulation;")],
 "drop cooldown in request": [(P, "if (remediationInProgress || now - lastRemediationAttempt <= kRemediationCooldownMs) return false;", "if (remediationInProgress) return false;")],
 "drop alert-22 resolve route": [(P, L342, "    if (false) consecutiveFaults = 1;")],
 "drop severity case 22": [("src/MyPersistentData.cpp", "        case 22: // PMIC USB Not Charging 2 h (charge latch)\n", "")],
 "re-add vcell default": [("src/power/PmicFaultMonitor.h", "float vcell);", "float vcell = 99.0f);")],
}
base = Path(tempfile.mkdtemp(prefix="pmicmut-", dir=os.environ["TMPDIR"]))
results = []
for name, edits in muts.items():
    d = base / re.sub(r"\W+", "_", name)
    shutil.copytree(repo / "src", d / "src"); shutil.copytree(repo / "tests", d / "tests")
    for f, old, new in edits:
        t = (d / f).read_text(); assert t.count(old) == 1, (name, t.count(old)); (d / f).write_text(t.replace(old, new))
    env = dict(os.environ, TMPDIR=str(d)); fails = []
    b = d / "bin"
    c = subprocess.run(["clang++","-std=c++17","-I",str(d/"tests/stubs/pmic_fault_monitor_overrides"),"-I",str(d/"src"),
        str(d/"tests/pmic_fault_monitor_test.cpp"),str(d/"src/power/PmicFaultMonitor.cpp"),str(d/"src/power/PowerManager.cpp"),
        str(d/"src/power/BatteryAuthorityPolicy.cpp"),"-o",str(b)],capture_output=True,text=True,env=env)
    if c.returncode: fails.append("compile")
    else:
        for s in range(1,11):
            r = subprocess.run([str(b), str(s)], capture_output=True, text=True)
            if r.returncode:
                m = re.search(r"pmic_fault_monitor_test\.cpp, line (\d+)|pmic_fault_monitor_test\.cpp:(\d+)", r.stderr)
                fails.append(f"scenario {s} (line {(m.group(1) or m.group(2)) if m else '?'})")
    r = subprocess.run([sys.executable, str(d/"tests/pmic_latch_call_site_structural_test.py")], env=dict(env, PMIC_LATCH_SRC_ROOT=str(d)), capture_output=True, text=True)
    if r.returncode: fails.append("structural: " + (r.stdout + r.stderr).strip().splitlines()[-1])
    results.append((name, fails))
    print(f"{'CAUGHT' if fails else 'SURVIVED':8} {name}: {'; '.join(fails) or '-'}", flush=True)
shutil.rmtree(base)
sys.exit(1 if any(not f for _, f in results) else 0)
EOF
python3 "$S/mutate.py"; echo "exit=$?"

<details>
<summary>22 lines</summary>

```
CAUGHT   drop !remediationInProgress (REQUIRED): scenario 10 (line 579)
CAUGHT   drop !remediationInProgress, check (i) disabled: scenario 10 (line 590)
CAUGHT   2h >= -> >: scenario 6 (line 353); scenario 7 (line 353); scenario 10 (line 353)
CAUGHT   4.05 < -> <=: scenario 6 (line 357)
CAUGHT   drop powerGood: scenario 6 (line 434)
CAUGHT   drop 24h limit: scenario 6 (line 371)
CAUGHT   drop timer reset: scenario 6 (line 351); scenario 7 (line 351); scenario 10 (line 351)
CAUGHT   drop shared USB context: scenario 6 (line 357)
CAUGHT   raise 21 not 22: scenario 6 (line 353); scenario 7 (line 353); scenario 9 (line 545); scenario 10 (line 353); structural: FAIL: the charge latch must raise alert 22 (and only 22)
CAUGHT   restore & 1 clear: scenario 8 (line 510); structural: FAIL: the legacy alert-21 clear must stay `regs.chargeStatus != 2`
CAUGHT   drop VBUS 1/2 clause: scenario 6 (line 434)
CAUGHT   VBUS 1/2/3 (allow OTG): scenario 6 (line 434)
CAUGHT   drop chargeStatus==0: scenario 6 (line 434); scenario 7 (line 351)
CAUGHT   drop fault-bit clause: scenario 6 (line 435)
CAUGHT   drop thermal clause: scenario 6 (line 434)
CAUGHT   drop safeToCharge: scenario 6 (line 434)
CAUGHT   drop cooldown in request: scenario 9 (line 539)
CAUGHT   drop alert-22 resolve route: scenario 7 (line 485); scenario 10 (line 583)
CAUGHT   drop severity case 22: structural: FAIL: alert 22 must share severity 3 with alerts 20 and 21
CAUGHT   re-add vcell default: structural: FAIL: pollAndRemediate must end with a required `float vcell` parameter (no defaults)
exit=0
<shellId: 11 completed with exit code 0>
```

</details>


---

<sub>2m 48s</sub>

### Copilot

All 20 mutations are caught. Checking which asserts the new lines map to, then running the full suite.

---

<sub>2m 49s</sub>

### `bash`

**Map lines and check suite runner**

$ for l in 579 583 590; do printf "%s: " $l; sed -n "${l}p" tests/pmic_fault_monitor_test.cpp; done; cat tests/README.md | grep -n -i "loop\|sequential\|N/N\|zsh\|python3" | head -20

<details>
<summary>24 lines</summary>

```
579:   assert(testCurrent.alertCodeValue == 22); // (i)
583:   assert(testCurrent.alertCodeValue == 0); // no cycle running: cleared on the same call
590:   latchPoll(pmic, done + kH + 2, fault); // second: requests level 1
45:zsh tests/watchdog_ab1805_classification_test.sh   # item 1: AB1805 wake-reason classification
46:zsh tests/watchdog_alert_code_test.sh              # item 3: new alert code severity/auto-clear (payload bytes: report_payload_golden_test.sh)
47:zsh tests/report_payload_golden_test.sh             # WO-2026-10-14-001: golden webhook payload bytes, event name and flags (175 vectors)
48:python3 tests/sleep_breadcrumb_sequence_test.py    # item 2: sleep-entry breadcrumb ordering (traces real source)
49:python3 tests/serial_settle_test.py                # item 4: always-on serial-settle placement/gating (traces real source)
54:`noteLoopStageDuration()`/`setLoopStage()`/`transitionTo()`
60:- `LOOP_STAGE_SLEEP_PREP` is still excluded from the
61:  `kLoopStageWarnThresholdMs`/`kLoopStageErrorThresholdMs` WARN/ERROR
62:  escalation inside `noteLoopStageDuration()` (a bare early return, before
64:- `setLoopStage()` is fully reverted to its simple original form (same-stage
66:  `lastLoopStage`/`stageStartMillis`), with no SLEEP_PREP-specific logic of
68:  here, gated on `setLoopStage()`'s own same-stage guard - broken, because
69:  `loop()` retags the loop stage through a fixed sequence every iteration
73:- **The `INFO`-level `LoopStage:` line for `SLEEP_PREP` is logged from a
80:  currently open (`retainedLoopForensics.sleepPrepSpanStartMillis != 0`); if
82:  (`logLoopStageLine()`) before proceeding with the state change and its own
92:  user-switch-press handling in `loop()` (both call `transitionTo()`
119:- A dedicated `RetainedLoopForensics.sleepPrepSpanStartMillis` field (new;
120:  `kLoopForensicsVersion` bumped 1 -> 2 since this is `retained` RAM) tracks
122:  `stageStartMillis`, which stays contaminated by loop-stage tag churn. It
<shellId: 12 completed with exit code 0>
```

</details>


---

<sub>2m 53s</sub>

### Copilot

Line 590 in the variant is line 591 here (one line was removed), i.e. the `(ii)` assert. Next I'll check how round 2 ran the full suite so the count is comparable.

---

<sub>2m 53s</sub>

### `bash`

**Find suite loop**

$ grep -n "for f in tests\|/dev/null" docs/work-orders/WO-2026-11-02-001-stage6-round2-transcript.md | head; 

<details>
<summary>11 lines</summary>

```
57:8. **Suite and build:** the full suite as one sequential loop, output to `/dev/null`, `N/N (sh via zsh, py via python3)` (baseline 107, round 1 was 108); local ARM release build per §3 recipe with a fresh `BUILD_PATH_BASE`; report sizes. Then `nm` on the ELF for the new symbols.
896:$ cat tests/pmic_fault_monitor_test.sh; echo =====; cat tests/stubs/pmic_fault_monitor_overrides/Particle.h; echo ====; ls tests/stubs/pmic_fault_monitor_overrides/; echo ====; cat tests/pmic_latch_call_site_structural_test.py 2>/dev/null || echo "NO STRUCTURAL TEST"; git --no-pager diff -- tests/pmic_fault_monitor_test.sh tests/stubs
1221:$ git status --short | tail -8; git --no-pager diff --numstat 0c565fa -- tests 2>/dev/null | head; git --no-pager log --oneline -1 0c565fa
1972:$ cd tests/stubs/pmic_fault_monitor_overrides; find . -type f | sort; grep -n "raiseAlert\|alertCode\|struct TestCurrent\|lastRaised" -n MyPersistentData.h persist/*.h 2>/dev/null | head -30
2016:$ grep -n "latestReport\|activeInputProfile\|PowerInputProfile\b" src/power/PowerManager.h | head; grep -n "enum class PowerInputProfile" -A8 -r src/power/*.h; grep -n "struct PowerReport" -A25 src/power/*.h | grep -n "activeInputProfile\|powerSource\|reading" ; cat .clang-format 2>/dev/null | head
2052:$ grep -n "struct PowerReading" -A15 src/power/PowerManager.h | grep -n "powerSource"; grep -n "pollAndRemediate" -B3 -A3 src/sensors/SensorManager.cpp | head -30; grep -rn "\.clang-format\|ColumnLimit" --include=.clang-format . 2>/dev/null | head -3; awk 'length > 110' src/power/PmicFaultMonitor.cpp | wc -l; awk '{ if (length > m) m = length } END { print m }' src/power/PmicFaultMonitor.cpp
3135:Now the full host suite as one sequential loop: `.sh` files via zsh, `.py` via python3, output to `/dev/null`.
3145:$ ls tests/*.sh tests/*.py | wc -l; pass=0; total=0; failed=""; for t in tests/*.sh tests/*.py; do total=$((total+1)); case $t in *.sh) zsh "$t" >/dev/null 2>&1;; *.py) python3 "$t" >/dev/null 2>&1;; esac && pass=$((pass+1)) || failed="$failed $t"; done; echo "$pass/$total (sh via zsh, py via python3)"; echo "failed:$failed"
3169:$ grep -n "BUILD_PATH_BASE" -B3 -A12 AI_DEVELOPMENT_WORKFLOW.md docs/*.md 2>/dev/null | head -60
3257:$ sed -n 410,436p AI_DEVELOPMENT_WORKFLOW.md; ls build-tmp 2>/dev/null | head
<shellId: 13 completed with exit code 0>
```

</details>


---

<sub>8m 28s</sub>

### `bash`

**Run full host suite**

$ sed -n 3136,3145p docs/work-orders/WO-2026-11-02-001-stage6-round2-transcript.md | grep -i tmpdir; S=$PWD/build-tmp/wo20261102-001-stage6r3; export TMPDIR=$S/tmp; ls tests/*.sh tests/*.py | wc -l; pass=0; total=0; failed=""; for t in tests/*.sh tests/*.py; do total=$((total+1)); case $t in *.sh) zsh "$t" >/dev/null 2>&1;; *.py) python3 "$t" >/dev/null 2>&1;; esac && pass=$((pass+1)) || failed="$failed $t"; done; echo "$pass/$total (sh via zsh, py via python3)"; echo "failed:$failed"

```
108
108/108 (sh via zsh, py via python3)
failed:
<shellId: 14 completed with exit code 0>
```


---

<sub>8m 32s</sub>

### Copilot

Suite passes 108/108. Starting the local ARM release build; first I'll check how round 2 copied the tree.

---

<sub>8m 33s</sub>

### `bash`

**Read round 2 build steps**

$ sed -n 3380,3400p docs/work-orders/WO-2026-11-02-001-stage6-round2-transcript.md; sed -n 3480,3515p docs/work-orders/WO-2026-11-02-001-stage6-round2-transcript.md

````
### `bash`

**Local ARM release build**

$ set -e; R="$PWD"; S="$R/build-tmp/wo20261102-001-stage6r2"; mkdir -p "$S/tree"; cp -R src lib project.properties "$S/tree/" 2>/dev/null || cp -R src project.properties "$S/tree/"; ls "$S/tree"; test ! -e "$S/obj-release" && echo fresh
export PATH=~/.particle/toolchains/buildtools/1.1.1/bin:~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH
cd ~/.particle/toolchains/deviceOS/6.6.0/main
make -s PLATFORM=boron APPDIR="$S/tree" TARGET_DIR="$S/tree/localbuild" DEVICE_OS_PATH=$HOME/.particle/toolchains/deviceOS/6.6.0 BUILD_PATH_BASE="$S/obj-release" > "$S/build.log" 2>&1; echo "make exit=$?"; tail -15 "$S/build.log"

<details>
<summary>21 lines</summary>

```
lib
project.properties
src
fresh
make exit=0
/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/build-tmp/wo20261102-001-stage6r2/tree/lib/LocalTimeRK/src/LocalTimeRK.cpp: In member function 'bool LocalTimeSchedule::getNextScheduledTime(LocalTimeConvert&, std::function<bool(LocalTimeScheduleItem&)>) const':
/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/build-tmp/wo20261102-001-stage6r2/tree/lib/LocalTimeRK/src/LocalTimeRK.cpp:924:25: warning: suggest parentheses around '&&' within '||' [-Wparentheses]
00000000 b PmicFaultMonitor::pollAndRemediate(PMIC&, bool, unsigned char, int, bool, float)::lastRemediationAttempt
00000000 b PmicFaultMonitor::pollAndRemediate(PMIC&, bool, unsigned char, int, bool, float)::remediationActiveLevel
00000000 b PmicFaultMonitor::pollAndRemediate(PMIC&, bool, unsigned char, int, bool, float)::remediationPhaseStartMs
00000000 t PmicFaultMonitor::pollAndRemediate(PMIC&, bool, unsigned char, int, bool, float)::{lambda(unsigned char, unsigned long)#1}::operator()(unsigned char, unsigned long) const [clone .constprop.0]
000bdf90 T _ZN16PmicFaultMonitor16pollAndRemediateER4PMICbhibf
2003dafc b _ZZN16PmicFaultMonitor16pollAndRemediateER4PMICbhibfE10latchSince
2003db00 b _ZZN16PmicFaultMonitor16pollAndRemediateER4PMICbhibfE10latchTried
2003db01 b _ZZN16PmicFaultMonitor16pollAndRemediateER4PMICbhibfE11latchTiming
2003db04 b _ZZN16PmicFaultMonitor16pollAndRemediateER4PMICbhibfE16latchLastAttempt
2003db08 b _ZZN16PmicFaultMonitor16pollAndRemediateER4PMICbhibfE16remediationLevel
2003db09 b _ZZN16PmicFaultMonitor16pollAndRemediateER4PMICbhibfE16remediationPhase
2003db0a b _ZZN16PmicFaultMonitor16pollAndRemediateER4PMICbhibfE17consecutiveFaults
2003db0b b _ZZN16PmicFaultMonitor16pollAndRemediateER4PMICbhibfE21remediationInProgress
2003db0c b _ZZN16PmicFaultMonitor16pollAndRemediateER4PMICbhibfE22immediateResetConsumed
2003db0d b _ZZN16PmicFaultMonitor16pollAndRemediateER4PMICbhibfE22immediateResetFaultReg
2003db10 b _ZZN16PmicFaultMonitor16pollAndRemediateER4PMICbhibfE22lastRemediationAttempt
2003db14 b _ZZN16PmicFaultMonitor16pollAndRemediateER4PMICbhibfE22remediationActiveLevel
2003db18 b _ZZN16PmicFaultMonitor16pollAndRemediateER4PMICbhibfE23remediationPhaseStartMs
000bdf50 t _ZZN16PmicFaultMonitor16pollAndRemediateER4PMICbhibfENKUlhmE_clEhm.constprop.0
<shellId: 28 completed with exit code 0>
```

</details>


---

<sub>27m 58s</sub>

### `bash`

**Record bin size and clean scratch**

$ S="build-tmp/wo20261102-001-stage6r2"; ls -l $S/tree/localbuild/*.bin | awk '{print $5, $9}'; shasum -a 256 $S/tree/localbuild/tree.bin | cut -c1-16
rm -rf "$S" && rm -f "$TMPDIR/mutate.py"; ls -la build-tmp; git status --short
<shellId: 15 completed with exit code 0>
````


---

<sub>12m 9s</sub>

### `bash`

**Local ARM release build**

$ set -e; R="$PWD"; S="$R/build-tmp/wo20261102-001-stage6r3"; mkdir -p "$S/tree"; cp -R src lib project.properties "$S/tree/"; test ! -e "$S/obj-release" && echo fresh
export PATH=~/.particle/toolchains/buildtools/1.1.1/bin:~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH
cd ~/.particle/toolchains/deviceOS/6.6.0/main
set +e; make -s PLATFORM=boron APPDIR="$S/tree" TARGET_DIR="$S/tree/localbuild" DEVICE_OS_PATH=$HOME/.particle/toolchains/deviceOS/6.6.0 BUILD_PATH_BASE="$S/obj-release" > "$S/build.log" 2>&1; echo "make exit=$?"; grep -iE "error|text\s+data" -A1 "$S/build.log" | head; ls -l $S/tree/localbuild/

<details>
<summary>16 lines</summary>

```
fresh
make exit=0
   text	   data	    bss	    dec	    hex	filename
 152084	   1114	   2216	 155414	  25f16	/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/a96315bb-4804-4652-8240-2203b9045bd8/scratchpad/wt5/build-tmp/wo20261102-001-stage6r3/tree/localbuild/tree.elf
total 112032
drwxr-xr-x  3 chipmc  wheel        96 Nov  2 16:18 obj
-rwxr-xr-x  1 chipmc  wheel    153202 Nov  2 16:18 tree.bin
-rw-r--r--  1 chipmc  wheel         4 Nov  2 16:18 tree.bin.crc_updated
-rw-r--r--  1 chipmc  wheel    153198 Nov  2 16:18 tree.bin.no_crc
-rw-r--r--  1 chipmc  wheel         6 Nov  2 16:18 tree.bin.product
-rw-r--r--  1 chipmc  wheel        36 Nov  2 16:18 tree.bin.suffix_updated
-rwxr-xr-x  1 chipmc  wheel  14424124 Nov  2 16:18 tree.elf
-rw-r--r--  1 chipmc  wheel    431034 Nov  2 16:18 tree.hex
-rw-r--r--  1 chipmc  wheel   3309623 Nov  2 16:18 tree.lst
-rw-r--r--  1 chipmc  wheel  38475589 Nov  2 16:18 tree.map
<shellId: 16 completed with exit code 0>
```

</details>


---

<sub>12m 16s</sub>

### Copilot

Build succeeded (text 152084, +16 vs round 2). Now the `nm` check, the `src/` arithmetic, `git diff --check`, and scratch cleanup.

---

<sub>12m 16s</sub>

### `bash`

**nm, numstat, cleanup**

$ S="build-tmp/wo20261102-001-stage6r3"; ~/.particle/toolchains/gcc-arm/10.2.1/bin/arm-none-eabi-nm -C $S/tree/localbuild/tree.elf | grep -c "PmicFaultMonitor::pollAndRemediate(PMIC&, bool, unsigned char, int, bool, float)"; git --no-pager diff --numstat -- src; git --no-pager diff --numstat -- src | awk '{a+=$1;d+=$2} END {print "total +"a" -"d" net "a-d}'; git --no-pager diff --numstat -- tests; git --no-pager diff --check && echo check-clean; rm -rf "$S"; ls -la build-tmp; git status --short

<details>
<summary>35 lines</summary>

```
15
1	0	src/MyPersistentData.cpp
41	12	src/power/PmicFaultMonitor.cpp
3	3	src/power/PmicFaultMonitor.h
1	1	src/sensors/SensorManager.cpp
total +46 -16 net 30
317	12	tests/pmic_fault_monitor_test.cpp
5	0	tests/pmic_fault_monitor_test.sh
5	0	tests/stubs/pmic_fault_monitor_overrides/Particle.h
check-clean
total 0
drwxr-xr-x   2 chipmc  wheel   64 Nov  2 16:18 .
drwxr-xr-x  23 chipmc  wheel  736 Nov  2 12:00 ..
 M docs/reference/alert-codes.md
 M src/MyPersistentData.cpp
 M src/power/PmicFaultMonitor.cpp
 M src/power/PmicFaultMonitor.h
 M src/sensors/SensorManager.cpp
 M tests/pmic_fault_monitor_test.cpp
 M tests/pmic_fault_monitor_test.sh
 M tests/stubs/pmic_fault_monitor_overrides/Particle.h
?? docs/work-orders/WO-2026-11-01-002-pmic-charger-reapply-step0.md
?? docs/work-orders/WO-2026-11-01-002-step0-report.md
?? docs/work-orders/WO-2026-11-02-001-stage6-copilot-dispatch.md
?? docs/work-orders/WO-2026-11-02-001-stage6-round2-copilot-dispatch.md
?? docs/work-orders/WO-2026-11-02-001-stage6-round2-transcript.md
?? docs/work-orders/WO-2026-11-02-001-stage6-round3-copilot-dispatch.md
?? docs/work-orders/WO-2026-11-02-001-stage6-transcript.md
?? docs/work-orders/WO-2026-11-02-001-stage7-round1-codex-dispatch.md
?? docs/work-orders/WO-2026-11-02-001-stage7-round1-verdict.md
?? docs/work-orders/WO-2026-11-02-001-stage7-round2-codex-dispatch.md
?? docs/work-orders/WO-2026-11-02-001-stage7-round2-verdict.md
?? docs/work-orders/WO-2026-11-02-001-usb-charge-latch-recovery.md
?? tests/pmic_latch_call_site_structural_test.py
<shellId: 17 completed with exit code 0>
```

</details>


---

<sub>12m 27s</sub>

### Copilot

I fixed the line-342 defect with no new lines. The full suite passes 108/108 and the local ARM release build succeeds. Nothing was committed or pushed.

**Defect:** confirmed by reading the code. At `PmicFaultMonitor.cpp:342` the counter was set to 1 even while a cycle was running. The `if (remediationInProgress)` branch then ran instead of the clearing branch at ~363, so the 1 carried into later calls.

**Edit (`PmicFaultMonitor.cpp:342`, same line):** added `&& !remediationInProgress` to the condition and reworded the end-of-line comment. Nothing changes `remediationInProgress` between 342 and 343, so the clearing branch always runs and resets the counter in that call. A fault on the same call can't reach 342, which only runs on fault-free polls. `immediateResetConsumed` doesn't affect it. I found no other carry path.

**Tests:** new scenario 10 (`pmic_fault_monitor_test.cpp`, +49 lines; `10` added to the `.sh` list). The latch fires, and the cycle completes on a Pre-charge (`chargeStatus 1`) poll; alert 22 must remain.
- **(i):** a later Not Charging poll keeps alert 22. A Pre-charge poll with no cycle running then clears it.
- **(ii):** a first thermal charge fault doesn't start a cycle. The second consecutive fault requests one, and the third poll runs it.

**Mutations** (copies under the scratch dir, never `src/`):
- **Required** (remove `!remediationInProgress`): scenario 10 fails at check (i), line 579.
- **Same mutation with check (i) removed:** scenario 10 still fails at check (ii), line 591, so each check catches the bug on its own.
- **Round 2's 18:** all still caught. "Drop the alert-22 resolve route" is now also caught by scenario 10.

**Build:** text 152084, data 1114, bss 2216, `.bin` 153202 bytes (text +16 vs round 2). `nm` shows the `pollAndRemediate(..., float)` symbols. `git diff --check` is clean. The scratch dir is removed; `build-tmp/` is kept.

**`src/` lines:** +46/−16 = **+30 net**. This round added 0 net (one line changed).

**Model:** Claude Opus 5.5 (`claude-opus-5.5`). I can't see the reasoning level; the dispatch specified high.

**Deviations:** none. `docs/reference/alert-codes.md` row 22 is outside this round's authorisation, so I didn't touch it. Its counter wording now needs updating to match this fix.

---

<sub>Generated by [GitHub Copilot CLI](https://github.com/features/copilot/cli)</sub>
