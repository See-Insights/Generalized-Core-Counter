**Overall: VERIFIED WITH NOTES.** No defect found within the WO’s acceptance criteria. Model/reasoning used: **gpt-6-astra, high**.

| Check | Verdict | Evidence |
|---|---|---|
| 1. Report once, then clear | **PASS — VERIFIED** | Extracted production logic and complete `publishData()` passed in both payload modes: queued report contains `alerts:19`, then code/time become 0; later 40 appears. Queue rejection preserves 19 and its timestamp. |
| 2. Other behavior | **PASS — VERIFIED** | Severity remains tier 4. Watchdog counters, fields and forensic event are unchanged. Every other code’s auto-clear behavior is unchanged. Boot ordering and escalation details below. |
| 3. Raise-site comment | **PASS — VERIFIED** | Describes reporting once, clearing afterward and retaining forensics; no sticky claim remains. |
| 4. Tests and mutation | **PASS — VERIFIED** | New test extracts real severity, raise, predicate and clear-block code. Removing `case 19:` fails at `g_alertCode == 0` (exit 134). Byte-identical restoration passes. Existing watchdog-test intent is preserved apart from the authorized sticky reversal. |
| 5. Suite and build | **PASS — VERIFIED WITH NOTES** | **63/63:** 34 scripts via zsh, 29 via python3. Protected publish-ACK, sleep-ownership and ledger-no-retry tests are unchanged and green. Clean local release build passed. |
| 6. Budget | **PASS — VERIFIED** | Budget **+1**, actual **+1** nonblank, non-comment `src/` line. Version edits net zero; no compression. |

No newly enabled unintended clear path was found. Watchdog classification and `raiseAlert(19)` precede startup status; `publishData()` runs later from the loop. Startup status does not clear 19. Alert 17 retains its existing boot assignment and successful-service clear behavior; reports do not auto-clear it.

Alert-40 escalation becomes reachable after 19 clears, as intended. Extracted supervision logic confirms that raising 40 writes a fresh timestamp before checking escalation, preserving the greater-than-three-hour cooldown.

| Build | text | data | bss |
|---|---:|---:|---:|
| v36 baseline | 150740 | 1090 | 2180 |
| Reviewed v38 | **150748** | **1090** | **2180** |
| Delta | **+8** | **0** | **0** |

`strings` finds `v38-Alert19Clears`; the product field contains **38** (`0x0026`). `nm` finds `publishData` and `setup`. Disassembly contains clear-list mask `0x34010011`, covering exactly 15, 19, 31, 41, 43 and 44. Claude’s cloud result remains recorded evidence, not independently rerun.

The suite initially had one scratch-copy collision: the thermal structural test scanned the copied application. It passed unchanged after scratch cleanup.

**Preservation:** all **1,498 original entries** match their initial bytes, paths and modes; Git status is unchanged. The designated scratch directory was removed. One qualification prevents confirming the strict “nothing outside” statement: `build-tmp/` was initially absent, so creating the required scratch path created that parent. It remains empty because deleting it was explicitly prohibited. No original files were deleted; no lasting file edits, commits, network access or device actions occurred.