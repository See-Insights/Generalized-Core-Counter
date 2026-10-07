**Overall: NOT VERIFIED.** One round-2 regression affects alert 18 after a ThrashGuard tier-3 reset. No fix was applied. Model/reasoning used: **gpt-6-astra, high**.

**Finding — alert 18 can disappear before the first post-reset report.**

The new clear path allows this sequence:

1. ThrashGuard tier 2 raises 18.
2. A queued report carries 18, clears it, and the cleared state reaches `/usr/current.dat`.
3. Tier 3 subsequently raises 18 and immediately resets.
4. The newly raised 18 has not reached persistent storage, so the first post-reset report carries **0**.

`currentStatusData::setup()` configures a **250 ms deferred save** (`src/MyPersistentData.cpp:681`). Tier 3 raises and resets at `src/ThrashGuard.cpp:150–152`, before normal persistence servicing at `src/Generalized-Core-Counter.cpp:1647`. Progress updates do not reset ThrashGuard’s recent-trip count, so an intervening report does not prevent tier 3.

An extracted-code harness using the production ThrashGuard, reporting, severity and deferred-flush logic reproduced:

| Configuration | Persisted code at reset | First post-reset report | Result |
|---|---:|---:|---|
| Round-1 clear list | 18 | 18 | PASS |
| Round-2 clear list | 0 | 0 | FAIL |
| Proposed forced flush | 18 | 18 | PASS |

The ordinary tier-3 case with 18 already persisted also passes.

**Smallest proposed fix, unapplied:** use the existing forced-flush operation when raising 18. This requires no new persistence mechanism.

```diff
diff --git a/src/MyPersistentData.cpp b/src/MyPersistentData.cpp
--- a/src/MyPersistentData.cpp
+++ b/src/MyPersistentData.cpp
@@ -913,6 +913,9 @@ void currentStatusData::raiseAlert(int8_t value) {
         // untrusted preserves the existing cooldown behavior; see the
         // Implementation Report for the Chief Engineer's review.
         set_lastAlertTime(Time.now());
     }
+    if (value == 18) {
+        flush(true);
+    }
 }
```

| Check | Verdict | Evidence |
|---|---|---|
| 1. Complete audit | **PASS — VERIFIED WITH NOTES** | All 19 named current, reserved and legacy values accounted for below. Current writers, variable forwarding, severity cases and reachable source history checked. No missing active code, active misclassification, or condition code without a clear path. |
| 2. Added codes report then clear | **FAIL — NOT VERIFIED** | Full `publishData()` checks pass in counting and occupancy modes for queued/rejected reports, code/time clearing, later lower alerts, all three 42 sites and tier-2 18. Tier-3 regression above fails. |
| 3. Conditions and ERROR_STATE | **PASS — VERIFIED** | 16, 17, 20/21/23 and 40 remain excluded. ERROR_STATE returns no action for 18 and defaults to no action for 42. Recovery actions for 15, 16, 31, 40 and 44 are unchanged. |
| 4. Tests and mutations | **PASS — VERIFIED WITH NOTES** | Both new cases exercise extracted production code. Removing 18, 42 and 19 separately fails its corresponding named test, each exit **134**. Byte-identical restoration and baseline rerun pass. Watchdog test’s predicate matches production apart from `static`. Existing tests do not model tier-3 persistence. |
| 5. Suite and build | **PASS — VERIFIED** | **63/63:** 34 scripts through zsh, 29 through python3. Protected tests unchanged from both round 1 and the base, and green. Clean scratch Boron 6.4.1 release build passes. |
| 6. Budget | **PASS — VERIFIED** | Round 2 **+2 / +2 budget**; total executable-source net **+3 / approximately +3 budget**. No compression. |

Clearing 18 or 42 permits later alert-40 escalation to become reachable. A newly accepted 40 receives a fresh timestamp, preserving the three-hour cooldown. Neither added clear suppresses an ERROR_STATE recovery action.

**Audit evidence.** Abbreviations below refer to `src/Generalized-Core-Counter.cpp` (**G**), `src/state/State_Sleep.cpp` (**S**), `State_Connect.cpp` (**C**), `State_Error.cpp` (**E**), `State_Report.cpp` (**R**), `src/power/PmicFaultMonitor.cpp` (**P**) and `src/ThrashGuard.cpp` (**T**). **Report clear** means G:2062–2071, after successful queueing.

| Code | Raise sites | Type | Clear path |
|---|---|---|---|
| 14 | Historical OOM writer; none current | Retired event | Reset-driven boot recovery, G:1081–1084 |
| 15 | S:829,949 | Event | Report clear; low-power ERROR_STATE, E:128–131 |
| 16 | S:1442 | Condition | Boot, G:1058–1061; E:128–131 |
| 17 | Direct assignment, G:1021 | Condition | Successful service/config/ledger checks, C:618–621 |
| 18 | T:141,150 | Event | Report clear; tier-3 persistence defect above |
| 19 | G:1307 | Event | Report clear |
| 20 | P:192,211 | Condition | PMIC recovery, P:369–372 |
| 21 | P:206,216,221,523 | Condition | P:170–172,369–381 |
| 22 | No writer found | Reserved; WO “—” | Included in P:369–372 range |
| 23 | P:224,390 | Condition | P:170–172,369–372 |
| 30,32 | Severity entries; no writer found in reachable history | Reserved; WO “—” | No dedicated clear |
| 31 | C:731 | Event | Report clear; successful connect C:576–577; E:128–131 |
| 40 | G:1682; R:116; S:640 | Condition | Webhook response, G:2398–2400 |
| 41 | C:603 | Event | Report clear |
| 42 | G:2099; C:611; S:637 | Event | Report clear |
| 43 | S:628 | Event | Report clear |
| 44 | S:634 | Event | Report clear; startup status, G:1396–1399 |
| 45 | Historical `CONNECTIVITY_FAILSAFE_ALERT`; constant at `ConnectivityPolicy.h:119` | Retired condition | Successful connect C:551 → G:2516,2527–2529 |

The WO’s “—” entries describe inactive codes. No additional historical writer value was found.

The raise inside `publishData()` is now at **G:2099**. It occurs **after** snapshotting, queueing and clearing the current report. Its 42 remains for the **next** report. A continuing ledger failure can raise a fresh 42 after the previous one is reported and cleared.

| Local release build | text | data | bss |
|---|---:|---:|---:|
| Round 1 | 150748 | 1090 | 2180 |
| Round 2 | **150748** | **1090** | **2180** |
| Delta | **0** | **0** | **0** |

`strings` finds **`v38-AllAlertsClear`**. The ELF product-version field is **38 (`0x0026`)**. Disassembly uses mask **`0x3c010019`**, offset by 15 and bounded through 44: exactly **15, 18, 19, 31, 41, 42, 43, 44**. Claude’s cloud result remains supplied evidence: flash **151974**, RAM **3282**; no cloud operation was performed.

For budget precision, raw textual `src/` growth is **+2 this round, +5 overall**; the extra two overall lines are round 1’s comment expansion. The proposed fix is excluded from these counts.

**Preservation:** all **1,479 original entries** retain identical bytes, paths and modes; Git status is identical. Temporary mutations were restored by rewriting in place, and only `build-tmp/wo20261006-001-stage7r2/` was removed. No files outside it were created or deleted. The sole directory exception is the required **`build-tmp/` parent**, which was initially absent and remains empty for Claude Code to remove. No commits, pushes, network access, flashing or device changes occurred.