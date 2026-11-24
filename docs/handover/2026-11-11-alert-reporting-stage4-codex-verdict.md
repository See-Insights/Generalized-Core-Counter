UNDER REVIEW: detached worktree @ aa418d18c2b9915fd3da292c86907e8f5239b7a1 (docs-only on top of `origin/main` f00aa04; `src/` is identical to f00aa04)

1. **Clear paths — FAIL.** `git rev-parse HEAD` matched the full SHA. Source audit:

| Code | Raise site(s) | Clear path |
|---|---|---|
| 14 | None current | Qualifying reset boot, `Generalized-Core-Counter.cpp:599-617` |
| 15 | `SleepTeardown.cpp:201,321` | Accepted report, `ReportPublisher.cpp:143-152`; non-CONNECTED ERROR, `State_Error.cpp:133-140` |
| 16 | `State_Sleep.cpp:860` | Next boot, `Generalized-Core-Counter.cpp:590-595`; non-CONNECTED ERROR |
| 17 | Direct write, `Generalized-Core-Counter.cpp:550-560` | Clean connect with config/ledger OK, `State_Connect.cpp:558-563` |
| 18 | `ThrashGuard.cpp:141,150` | **None** |
| 19 | Watchdog-classified boot, `Generalized-Core-Counter.cpp:822-840` | Accepted report |
| 20 | NTC/thermal, `PmicFaultMonitor.cpp:198-201,218-220` | Charge-fault recovery at `:363-379`; **NTC-only raise cannot reach it** |
| 21 | `PmicFaultMonitor.cpp:215,225,230,549` | `:178-181,363-388` |
| 22 | `PmicFaultMonitor.cpp:392-406` | Charger leaves Not Charging via `:342,363-379` |
| 23 | Generic/BAT fault, `PmicFaultMonitor.cpp:233,411-416` | `:178-181,363-379`; **BAT_FAULT-only raise cannot reach either** |
| 30,32 | Never raised | No code-specific clear |
| 31 | `State_Connect.cpp:643` | Accepted report; successful connect `:518-520`; non-CONNECTED ERROR |
| 40 | `Generalized-Core-Counter.cpp:1211-1215`; `State_Report.cpp:121-126`; `State_Sleep.cpp:282-284` | Any non-empty response, `ReportPublisher.cpp:273-301` |
| 41 | `State_Connect.cpp:536-545` | Accepted report |
| 42 | `ReportPublisher.cpp:176-179`; `State_Connect.cpp:550-553`; `State_Sleep.cpp:279-281` | Accepted report |
| 43 | `State_Sleep.cpp:269-272` | Accepted report |
| 44 | `State_Sleep.cpp:273-278` | Accepted report; startup status, `Generalized-Core-Counter.cpp:920-932` |
| 45 | Never raised | Successful connect through `State_Connect.cpp:493` → `ConnectivityFailsafe.cpp:134-152` |

CONNECTED and low-power modes differ only at `State_Error.cpp:133-145`: INTERMITTENT/DISCONNECTED clear 15/16/31. `/usr/current.dat` persists code/time (`MyPersistentData.cpp:613,625-635`; `MyPersistentData.h:665-666`). `MyPersistentData.h:257`’s `set_alertCodeNode()` is declaration-only and dead; no definition/caller. The only non-clear direct code write is 17. `State_Connect.cpp:518-520` clears 31 but, uniquely, leaves `lastAlertTime` stale.

2. **Stick/mask — FAIL for Stage 2’s conclusion.** Let T3={14–18,20–22}, T2={23,30–32,40–43}, T1={44,45}. A held T3 masks T3+T2+T1; T2 masks T2+T1; T1 masks T1. Code 19 masks every severity-gated raise. Direct code 17 bypasses all masking (`MyPersistentData.cpp:765-817`; `Generalized-Core-Counter.cpp:554`).

| Code | Can stick while running? | Masks while held |
|---|---|---|
| 14 | Yes; legacy, boot-only clear | T3/T2/T1 |
| 15 | No; report/non-CONNECTED clear | T3/T2/T1 |
| 16 | Until recovery boot; no live CONNECTED clear | T3/T2/T1 |
| 17 | Until clean service pass—correct condition | T3/T2/T1 |
| 18 | **Yes, defect** | T3/T2/T1 |
| 19 | No; report-once | All except direct 17 |
| 20 | **Yes on NTC-only path, defect** | T3/T2/T1 |
| 21 | Until charge recovery—correct condition | T3/T2/T1 |
| 22 | Until accepted charger recovery | T3/T2/T1 |
| 23 | **Yes on BAT_FAULT-only path, defect** | T2/T1 |
| 30,32 | Hypothetically yes, but never raised | T2/T1 |
| 31 | No | T2/T1 |
| 40 | Until response—correct condition | T2/T1 |
| 41,42,43 | No; report-once | T2/T1 |
| 44 | No; report/startup-status clear | T1 |
| 45 | Until connect; never raised | T1 |

The reference’s severity/masking sets are correct. “Only 18 has no clear site” is literally true among raised codes, but the implied P5 result is false: 20 and 23 each have a raise path that cannot reach their nominal clear (`PmicFaultMonitor.cpp:198-208,363-416`).

3. **Code 18 — FAIL currently.**  
(a) Tier 2 raises at `ThrashGuard.cpp:139-146`, requests full disconnect/radio-off, transitions to sleep, and remains alive. Tier 3 raises at `:149-153`, increments `thrashResetCount`, calls `flushAllBeforeReset()`, then `System.reset(RESET_CAUSE_THRASH_TIER3)`. The flush synchronously writes CurrentReadings at `State_Error.cpp:115-120`.  
(b) Yes, normally it survives and reaches startup status and the first scheduled report. Device OS 6.6.0 implements `System.reset(data)` as `RESET_REASON_USER` (`wiring/src/spark_wiring_system.cpp:54`); that is neither watchdog classification (`Generalized-Core-Counter.cpp:322,822`) nor counted by the boot-storm switch (`:346-363`). Thrash runs after setup has cleared `bootInProgress` (`:1121-1123`). No boot clear handles 18. Only an independently already-pending boot-storm 17 could directly overwrite it before reporting (`:550-560`).  
(c) Smallest fix: add `case 18:` to `isAutoClearAfterReportAlert()`, `ReportPublisher.cpp:25-37`: **+1 `src/` line**. Tier 2 then reports on the next accepted queued report and clears; recurrence after that re-raises a fresh incident.  
(d) Clearing 18 allows 15/16/20/21/22 and lower 23/31/40–44 to be raised from zero; none remains permanently hidden by 18.  
(e) Reuse `alert19_clears_after_report_test.sh`: replace `testThrashStaysSetAfterReport()` with `raiseAlert(18); assert(publishReport(true)==18); assert(code==0); raiseAlert(44); assert(code==44);`. It extracts the real severity, auto-clear switch, and clear block (`tests/alert19_clears_after_report_test.sh:71-88,136-147`).

4. **Other P5 candidates — FAIL because of 20/23, not 17/16/22/40.** Code 17 has a condition clear; 16 has non-CONNECTED and recovery-boot clears; 22’s accepted limitations remain as documented; 40 correctly remains until a non-empty response and may re-raise every report while down. No changes are needed for those four. Add two predicate lines to the existing no-fault check at `PmicFaultMonitor.cpp:383-389`: clear 20 when `ntcFaultCode==0`, 23 when `BAT_FAULT` is absent, and retain 21’s `chargeStatus!=2` rule. This is **+2 `src/` lines** and covers the missed NTC-only/BAT-only paths.

5. **P4 — PASS.** GCC preserves or strengthens legacy recovery: 15/31/44 soft-reset then hard-cycle (`State_Error.cpp:45-55`), 16 soft then hard-cycle (`:81-94`), and 40 soft-resets after trusted >3 h webhook failure (`:57-79`). Codes 17/18 need no duplicate ERROR action because boot-storm holdoff and ThrashGuard already act (`:96-106`). Thus legacy 15/31 recovery remains, legacy 30’s reset equivalent is covered by 40, and legacy 13’s excessive-reset role is split across 17/18/19 without losing the initiating recovery.

6. **Payload capacity — PASS only with a short key.** The real worst cases from `report_payload_fields_test.py:29-63,115-187` are: occupancy **237 rendered / 238 including NUL**, leaving 18 content bytes; counting **226 / 227**, leaving 29. A field costs `key_length + 14` bytes at a 10-digit `unsigned long`. Therefore keys ≤4 characters fit both. Proposed `,"ats":4294967295` costs 17 bytes; `alertAt` costs 21 and does not fit occupancy. `ReportPublisher.cpp:57,103-117`.

7. **Raw `lastAlertTime` — FAIL as onset time.**

| Path/codes | What it currently means |
|---|---|
| 15,16,18–23,31,40–44 | Raise time only when the code replaces a lower severity, `MyPersistentData.cpp:811-829` |
| Same-severity re-raise | Original held-code time; no update |
| 17 | Unconditional direct-write time, `Generalized-Core-Counter.cpp:554-560` |
| 14/30/32/45 | No current raise; persisted value is legacy/unknown |
| 40 after escalation | Last ERROR escalation, not onset, `State_Report.cpp:141-149` |
| Untrusted clock | Ungated `Time.now()`—possibly zero or a plausible but wrong epoch |
| Auto-clear report | Payload would capture code/time, then both clear at `ReportPublisher.cpp:143-151` |
| Code 31 connect clear | `alerts=0` with stale time unless payload explicitly emits zero |

Smallest correct form: reuse unused persisted `SysData::reservedTime0` (`MyPersistentData.h:140`) as `alertOnsetTime`; add two RecoveryState declarations, two one-line facade definitions, stamp it on severity replacement and direct 17 using `Clock::isTrusted()?Time.now():0`, and report `(reportedAlertCode>0 ? onset : 0)` as numeric key `ats`: **+7 `src/` lines**, no persistent-layout growth. Leave `lastAlertTime` serving alert-40 cooldown unchanged. Ubidots creates a new top-level `ats` variable; it sees 0 for no alert and also for unknown/untrusted onset, distinguished by `alerts`. `ats` avoids existing `alerts`, status `alert`, and `lastAlert`.

8. **Clear report — PASS with the scheduled model.** After any clear, the next scheduled payload already has `alerts:0`; after auto-clear, the report carrying the incident was captured before the clear (`ReportPublisher.cpp:74,103-117,143-152`). No separate immediate clear publish is needed. Otherwise it costs another queued report and potentially another radio connection. Condition codes still repeat until resolution; merely adding X1 time does not make those report-once.

9. The onset field supplies the original time in a delayed/replayed report; it does **not** stop transport backlog replay by itself.

10. **P1 contract — PASS for the combined suite.** Baseline tests passed. Scratch mutations produced:

| Mutation | Golden | Fields |
|---|---:|---:|
| Rename battery/key1/connecttime | caught | caught |
| Rename alerts/resets | caught | **missed** |
| Quote battery | caught | caught |
| Drop connecttime | caught | caught |

The golden fixture executes both modes and pins exact bytes (`report_payload_golden_test.sh:69-83`; fixture max-width vectors). The fields test reconstructs dynamic names as literal `alerts`/`resets` (`report_payload_fields_test.py:99-108`), explaining its miss; golden coverage closes it. Types and all five names are therefore covered jointly in both modes.

11. No P1 gap remains, so no additional five-key test is required. A smaller replacement for the golden protection would be weaker than the existing real-format/fixture test.

12. `key1` is exactly `Unknown`, `Not Charging`, `Charging`, `Charged`, `Discharging`, `Fault`, `Disconnected` (`ReportPublisher.cpp:50-55`). Yes, pin all seven: current golden vectors cover states 0,2,6 and out-of-range fallback, but omit 1,3,4,5 (`report_payload_golden_test.cpp:117,124`).

13. **Integration comparison — PASS with one relevant difference.** `origin/integration/v50-ab1805` resolved to `1d1d5af7a22dc47094a4c23b515b348bd8ae153d`. Formatter, auto-clear list, normal raise/clear sites, severity values, and `lastAlertTime` behavior are unchanged. At that head, PIN_RESET without persisted power-off evidence is inferred as watchdog and raises 19 (`Generalized-Core-Counter.cpp:760-797`), broader than f00aa04’s AB1805-confirmed path. P4’s hard recovery uses timed `powerOffForRecovery()` instead of `ab1805.deepPowerDown()` (`State_Error.cpp:173-183`; `ConnectivityFailsafe.cpp:135-146,329`), with the same intended power-cycle outcome.

14. Before the WO: update `alert-codes.md` for the 20/23 gaps and code-31 stale timestamp. **Inference:** code 17 can be cleared by an early successful service connection before any scheduled `alerts` report, although startup `status.alert` captured it (`Generalized-Core-Counter.cpp:920-923`; `State_Connect.cpp:558-563`); confirm the Ubidots Slack rule consumes a scheduled `alerts` value. Also update the existing test that deliberately requires 18 to remain sticky, and note `watchdog_alert_code_test.cpp:38-64` omits source-tier-3 code 22 from its mirror.

| Work item | Added `src/` lines | Test lines, separate |
|---|---:|---:|
| P5: auto-clear 18 | 1 | 0 net; rewrite existing 18 scenario |
| P5: NTC-20/BAT-23 clears | 2 | Add two PMIC scenarios, not budgeted |
| **P5 total / budget** | **3 / 40** | — |
| X1: persisted trusted onset + `ats` | 7 | Payload/onset cases, not budgeted |
| **X1–X3 budget remaining** | **73 / 80** | — |
| P1 five-key contract | 0 | 0; existing combined suite sufficient |
| Optional all-seven `key1` pin | 0 | About 4 new vectors |

**Recommendation: revise.** P5 must include the missed NTC-only 20 and BAT_FAULT-only 23 clears; otherwise proceed with the small 18 fix. X1 should not expose raw `lastAlertTime`; use the separate trusted onset described above and have the architect approve the short `ats` key/reserved-slot reuse. Full host suite: **118/118 passed**.  
**Model/reasoning:** gpt-5.6-sol, high.  
**Interpreter:** **118/118 (68 sh via zsh, 50 py via python3).**