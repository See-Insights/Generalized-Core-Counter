# Alert codes

The device holds **one** alert code at a time. It's the current code, not a count. The report's `alerts` field and the status event's `alert` field both carry it (report: `cloud/ReportPublisher.cpp:76` and the payload at `:106–119`; status event: `Generalized-Core-Counter.cpp:1296`, written at `:1355`/`:1384`). Status also carries `lastAlert` (`:1297`, written at `:1360`/`:1389`), the time the code last rose in severity. **The report does not carry it.** `lastAlertTime` is also reused as the alert-40 escalation timestamp (R:149), so it is not purely an onset time.

The code is persisted in `/usr/current.dat` (`MyPersistentData.h:665–666`), so it survives reboots and hibernate. `raiseAlert()` replaces it only with a **strictly more severe** code (`MyPersistentData.cpp:811–830`; the comparison is at `:817`) and ignores values ≤ 0 (`:812`). It stamps `lastAlertTime` only when it replaces the code (`:829`), so a same-severity re-raise does not move the time. A code that never clears therefore hides every lower one.

**Source:** firmware `main` at `f00aa04` (PR #222 merged, 11 Nov 2026), **with WO-2026-11-11-001 applied** (the `RP:`, `P:` and `C:` citations below were re-cited on that change; the historical section is not). Every `file:line` below was re-cited on 11 Nov 2026 against that commit, each by statement content, as part of the alert-reporting intake (`docs/handover/2026-11-11-alert-reporting-handover.md`). The `P:` citations (`power/PmicFaultMonitor.cpp`) were checked at the same commit and did not move. Earlier citation commits: `a3e012d` (P:), `2c477e8` (v38), `e78d59d` (v37), `7a2b026` (v36). Raise sites that moved files since v38: the sleep-teardown raises of 15 are now in `state/SleepTeardown.cpp` (TD), and the connectivity-failsafe clear is in `power/ConnectivityFailsafe.cpp` (F).

**Abbreviations:**
- **G** `Generalized-Core-Counter.cpp`;
- **S** `state/State_Sleep.cpp`;
- **TD** `state/SleepTeardown.cpp`;
- **RP** `cloud/ReportPublisher.cpp`;
- **F** `power/ConnectivityFailsafe.cpp`;
- **C** `state/State_Connect.cpp`;
- **E** `state/State_Error.cpp`;
- **R** `state/State_Report.cpp`;
- **P** `power/PmicFaultMonitor.cpp`;
- **T** `ThrashGuard.cpp`;
- **M** `MyPersistentData.cpp`.

**Report** means cleared after the first report that carries it is queued: `isAutoClearAfterReportAlert()` (RP:25–39; the list is 15, 17, 18, 19, 31, 41, 42, 43, 44), applied at RP:145–154 only when the report was accepted by the publish queue and the code is still the one reported.

**Type:**
- **event:** report it once, then clear;
- **condition:** clear when the condition resolves.

## Table

| Code | Meaning | Raised when (file:line) | Severity (M) | First raised | Type | Clears |
|---|---|---|---|---|---|---|
| 0 | no alert | — | 0 (:769) | — | — | — |
| 14 | out-of-memory | **Not raised by current firmware.** Raised v3.09 (`8aa2643`) to v33; the raise was removed in v34 (`1c74ac8`). A device can still hold a legacy 14. | 3 (:775) | v3.09 | event (legacy) | On boot after a reset-driven recovery (G:614–618) |
| 15 | modem / disconnect failure | Sleep disconnect/modem-off exceeded its budget (TD:201); sleep precondition (teardown) exceeded its budget (TD:321) | 3 (:776) | v3.09 | event | Report (RP:145–154); ERROR_STATE in non-CONNECTED modes (E:136–140) |
| 16 | repeated sleep failures | `ULTRA_LOW_POWER` sleep returned an error (S:860) | 3 (:777) | v3.09 | condition | On boot (G:591–595); E:136–140 |
| 17 | boot storm | Boot-storm holdoff pending at boot (G:554; the pending flag is set at G:398). Written with `set_alertCode(17)`, **bypassing severity**: it overwrites any active code, including 19. | 3 (:778) | v3.29 (`8843d41`, via `raiseAlert`); direct write since v4.05 (`98d98d8`) | event | **Report once, then clear** (`isAutoClearAfterReportAlert()`, WO-2026-11-11-001). The clear at connect (formerly at `f00aa04` C:560–563) was removed: it could run before any scheduled report carried the code. A re-raise at a later boot reports again. |
| 18 | state-machine thrash | ThrashGuard tier 2 (T:141); tier 3, then an immediate reset (T:150) | 3 (:779) | v4.05 (`98d98d8`) | event | **Report once, then clear** (`isAutoClearAfterReportAlert()`, WO-2026-11-11-001; tier 2 included). Before that change: never. It's raised just before the tier-3 reset; **from v49 it is written to `/usr/current.dat` and flushed before that reset** (`flushAllBeforeReset()`, WO-2026-11-03-005), so the first report after the reboot carries it, then it clears. While set it masks every code at or below tier 3 until that report is queued. |
| 19 | watchdog reset | Boot classified as a Device OS watchdog or an AB1805-confirmed pin reset (G:840) | **4, the highest** (:792–793) | committed `75ad0a8`; first released in v22-Diag-Soak (`3b9b39f`) | event | **Report once, then clear, since v38** (`isAutoClearAfterReportAlert()`; raise-site comment G:832–839). **Before v38: never** (deliberately excluded), so a device on older firmware keeps 19 and it hides every lower alert. |
| 20 | PMIC thermal | NTC fault (P:201); charge fault 0x02, thermal shutdown (P:220) | 3 (:780) | v3.20 (`3f29873`) | condition | Charging resumed, codes 20–23 (P:376–378); **or no NTC fault on a no-charge-fault read (P:390–393, WO-2026-11-11-001)**, which clears a 20 raised by an NTC fault alone (P:201). |
| 21 | PMIC charge timeout / stuck | Input fault persisting after a reset (P:215); safety timer expired (P:225); other charge fault persisting after a reset (P:230); fast charging 6 h+ with no material gain, **unless `vcell` ≥ 4.05 V** (`kPmicFullPackVcell`, a full pack; P:543–555: the window restarts and a `Log.warn` says the alert was skipped) | 3 (:781) | v3.20 | condition | Charge fault cleared (P:178–180); P:376–378; charge status ≠ 2 after recovery (P:385–387) |
| 22 | PMIC charge latch: USB unit stuck at Not Charging | Charger reads Not Charging (REG08 `CHRG_STAT` 00) with no charge or BAT_FAULT bit for 2 h continuously on a USB-backed input (corrected source test and VBUS status 1 or 2), `powerGood`, battery under 4.05 V, no thermal regulation, `safeToCharge`; then one level-1 charging cycle (1 h remediation cooldown applies) and one `Log.warn` with REG01/REG08/REG09; at most one attempt per 24 h, no escalation (P:396–413). Since the WO-2026-11-02-001 firmware. | 3 (`MyPersistentData.cpp:782`) | WO-2026-11-02-001 (unreleased) | condition | Charging resumed, codes 20–23 (P:376–378), once the charger leaves Not Charging: the line at P:342 sets `consecutiveFaults` to 1, only while no remediation cycle is running, so the clear runs and resets the counter to 0 in the same call. It stays raised while the charger reads Not Charging and through the latch's own cycle. **Known limitations (architect-accepted, WO-2026-11-02-001):** (a) it may clear early if a real charge fault occurs during a latch episode (the fault path's "charging healthy" clear at P:363–379 wipes 20–23 whatever the charger status); the latch then re-raises after a new 2 h window, and the 24 h attempt limit applies (P:401–413); (b) while raised, it can mask alert 20 (equal severity 3; a raise replaces only a strictly higher severity, `MyPersistentData.cpp:817`); both point to the charger. The toggle is the fix; the alert is a heads-up. |
| 23 | PMIC battery fault | Other charge fault (P:233); BAT_FAULT overvoltage (P:420) | 2 (:795) | v3.20 | condition | P:178–180; P:376–378; **or BAT_FAULT absent on a no-charge-fault read (P:390–393, WO-2026-11-11-001)**, which clears a 23 raised by BAT_FAULT alone (P:420). |
| 30 | connectivity timeout with radio up | Not raised | 2 (:796) | — | — | — |
| 31 | failed to connect to cloud | Connection attempt exceeded its budget (C:633) | 2 (:797) | v3.09 | event | Report (RP:145–154); a successful connection (C:518–520, which leaves `lastAlertTime` stale); E:136–140 |
| 32 | connect taking too long | Not raised | 2 (:798) | — | — | — |
| 40 | repeated webhook failures | Webhook response timeout (G:1215); no successful response for over 3 h in open hours (R:126, evaluated at R:112–126); no webhook response before sleep (S:284) | 2 (:799) | v3.09 | condition | Any non-empty webhook response (RP:301–304) |
| 41 | config / ledger apply failure | Configuration apply failed at connect (C:545). Common causes from v40: `reportingIntervalSec` above 65535; `sensor.type` other than 1. | 2 (:800) | v3.09 | event | Report (RP:145–154) |
| 42 | data ledger publish failure, **or** OTA updates pending | Report's data-ledger publish failed (RP:181); ConnectState's data-ledger publish failed (C:552); OTA updates pending at the sleep gate (S:281) | 2 (:801) | v3.09 | event | **Report once, then clear, since v38** (`isAutoClearAfterReportAlert()`). **Before v38: never** (no clear site), so a device on older firmware keeps 42. For "OTA updates pending", the sleep gate raises it again on each timeout while the update is pending (S:262–281). |
| 43 | publish queue not drained before sleep | S:272 | 2 (:802) | v3.09 | event | Report (RP:145–154) |
| 44 | ledger sync timeout before sleep | S:278 | 1 (:804–805) | v3.12 (`15db55c`) | event | Report (RP:145–154); after the boot status event (G:929–933) |
| 45 | connectivity failsafe (`ConnectivityPolicy.h:119`) | Not raised by current firmware | 1 (default) | — | condition (legacy) | On a successful connection (C:493 → F:134–152) |

**ERROR_STATE actions** (E:`resolveErrorAction()`):
- **15, 31 and 44 (E:45–55):** soft reset, then a hard power cycle, then stop after 4 resets.
- **16 (E:81–95):** a soft reset, then a hard power cycle.
- **40 (E:57–79):** a soft reset if the clock is trusted and there's been no webhook response for over 3 h.
- **17 and 18 (E:96–106):** none (unchanged; they are report-once now, so the code is gone after the first report).
- **Every other code:** none.
- **In non-CONNECTED modes,** 15, 16 and 31 are cleared instead (E:136–140).

## Report payload field list (`RP:106–119`, at `f00aa04`)

One `snprintf` builds the scheduled report (256-byte buffer `RP:59`, format `RP:106`, arguments `RP:107–119`). Every field is an unquoted JSON number except `key1`, which is a context string (`batteryContext[]`, `RP:53–57`). The Ubidots webhook turns each top-level key into a variable, so a rename or a quoted value breaks the cloud rules that read it.

- **Leading sensor fields** (`activeSensor()->reportFields`, `RP:99`): occupancy mode `occupancy`, `dailyoccupancy` (`PIRSensor.cpp:27`); counting mode `hourly`, `daily` (`PIRSensor.cpp:31`).
- **Common fields, in order:** `battery` (%, 2 decimals, clamped to 0–100, `RP:80–84`), `vc` (cell voltage, 0.00 when none), `key1` (charge state), `temp`.
- **Alert fields, order depends on mode:** occupancy mode `alerts` then `resets`; counting mode `resets` then `alerts` (`RP:112–115`).
- **Trailing fields:** `connecttime`, `fh`, `lfb`, `cyc`, `slp`, `timestamp` (milliseconds, `…000`).
- **Not in the report:** `lastAlert` / `lastAlertTime` (status event only), and any per-period counter.

## Masking at `f00aa04`

A raise replaces the held code only if it is strictly more severe (`M:817`). The held code is therefore a mask for everything at or below its tier until it clears:

- **18 (tier 3, report-once)** blocks 15, 16, 20, 21 and 22 (equal tier) and every tier-2 and tier-1 code (23, 31, 40–44) until a report that carries it is queued. Only 19 (tier 4) and 17 (a direct `set_alertCode`, G:554) get past it.
- **17 (tier 3, report-once)** is written directly, so it overwrites any held code, but while held it masks the same codes as 18.
- **Tier-3 codes that clear** (15, 16, 17, 18, 20, 21, 22) block 23 and 31–44 while held.
- **Codes with no clear site in `src/`:** none among the codes the firmware raises (18 reports once since WO-2026-11-11-001). (14 clears on a reset-driven boot, but is not raised; 30 and 32 are not raised; 45 is not raised and clears on a connect.)

## Differences from the fleet-ops table, resolved against the source

*Historical: the line numbers quoted in this section were cited at `e78d59d` and have not been re-cited; the table above is current.*

The fleet-ops agent's table was built from received payloads and used only as a cross-check.

1. **Line numbers:** most of fleet-ops' citations matched v37's branch (`85bd316`), not the pre-v37 main this reference was first cited at (`7a2b026`). Since v37 merged, they match main (`e78d59d`), and this reference is now cited there; examples: 15 at S:833/953, 31 at C:666, 41 at C:566.
2. **14:** fleet-ops says the raise is "gone since". **Resolved:** removed in v34 (`1c74ac8`). A legacy 14 clears only on a reset-driven boot.
3. **16:** fleet-ops says "Device OS rejects the ULP sleep config". **Resolved:** raised when `ULTRA_LOW_POWER` sleep returns *any* error. A rejected configuration is the source comment's example.
4. **17:** "first version v3.29 (`8843d41`)" is **confirmed**: it was first raised through `raiseAlert(17)`, and became the direct `set_alertCode(17)` in v4.05 (`98d98d8`). **Added:** because the write bypasses severity, 17 overwrites a 19.
5. **22:** missing from the fleet-ops table. **Added:** never raised and not in the severity table, but inside the PMIC clear range.
6. **31 clear:** fleet-ops says "next connect attempt". **Resolved:** a *successful* connection (C:539–540, in the post-connect path).
7. **40 clear:** fleet-ops says "any webhook response". **Resolved:** any **non-empty** response; an empty one is logged and doesn't clear it.
8. **42:** fleet-ops' raise sites are **confirmed**. **Added:** the code has two meanings (a ledger publish failure, and OTA updates pending).
9. **18, 19 and 42 "Never":** **confirmed** for firmware before v38. Since v38, 19 and 42 report once and then clear; 18 did not until WO-2026-11-11-001, which also made 17 report-once.
10. **Confirmed unchanged:** the severities (including 23 at 2, 44 at 1, 45 at the default 1), the first versions of 14/15/16/18/19/20/21/23/31/40–44, 19's release in v22-Diag-Soak, and the clear sites for 16, 17, 20, 21, 23, 44 and 45.

## Keeping this current

Any WO that adds, removes or changes how an alert is raised, ranked or cleared updates this file in the same PR, and Stage 7 checks it (`AI_DEVELOPMENT_WORKFLOW.md`, Stage 7).
