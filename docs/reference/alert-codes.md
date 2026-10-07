# Alert codes

The device holds **one** alert code at a time. It's the current code, not a count. The report's `alerts` field and the status event's `alert` field both carry it (`Generalized-Core-Counter.cpp:1991`, written at `:2026`/`:2046`; status `:2170`). Status also carries `lastAlert`, the time the code last rose in severity.

The code is persisted in `/usr/current.dat` (`MyPersistentData.h:684`), so it survives reboots and hibernate. `raiseAlert()` replaces it only with a **more severe** code (`MyPersistentData.cpp:895–902`) and ignores values ≤ 0 (`:896`). A code that never clears therefore hides every lower one.

**Source:** firmware `main` at `2c477e8` (v38 merged). Every file:line below is at that commit and was checked against the source on 2026-10-07. Earlier citations: `e78d59d` (v37) and `7a2b026` (v36).

**Abbreviations:**
- **G** `Generalized-Core-Counter.cpp`;
- **S** `state/State_Sleep.cpp`;
- **C** `state/State_Connect.cpp`;
- **E** `state/State_Error.cpp`;
- **R** `state/State_Report.cpp`;
- **P** `power/PmicFaultMonitor.cpp`;
- **T** `ThrashGuard.cpp`;
- **M** `MyPersistentData.cpp`.

**Report** means cleared after the first report that carries it is queued: `isAutoClearAfterReportAlert()` (G:1942–1955), applied at G:2078–2087.

**Type:**
- **event:** report it once, then clear;
- **condition:** clear when the condition resolves.

## Table

| Code | Meaning | Raised when (file:line) | Severity (M) | First raised | Type | Clears |
|---|---|---|---|---|---|---|
| 0 | no alert | — | 0 (:853–854) | — | — | — |
| 14 | out-of-memory | **Not raised by current firmware.** Raised v3.09 (`8aa2643`) to v33; the raise was removed in v34 (`1c74ac8`). A device can still hold a legacy 14. | 3 (:860) | v3.09 | event (legacy) | On boot after a reset-driven recovery (G:1091–1093) |
| 15 | modem / disconnect failure | Disconnect/modem-off exceeded budget (S:833); pre-sleep teardown exceeded budget (S:953) | 3 (:861) | v3.09 | event | Report; ERROR_STATE in non-CONNECTED modes (E:128–130) |
| 16 | repeated sleep failures | `ULTRA_LOW_POWER` sleep returned an error (S:1446) | 3 (:862) | v3.09 | condition | On boot (G:1068–1070); E:128–130 |
| 17 | boot storm | Boot-storm holdoff pending at boot (G:1031). Written with `set_alertCode(17)`, **bypassing severity**: it overwrites any active code, including 19. | 3 (:863) | v3.29 (`8843d41`, via `raiseAlert`); direct write since v4.05 (`98d98d8`) | condition | After a connection with config applied and ledger published OK (C:581–583) |
| 18 | state-machine thrash | ThrashGuard tier 2 (T:141); tier 3, then an immediate reset (T:150) | 3 (:864) | v4.05 (`98d98d8`) | event | **Clears: never (planned fix with ThrashGuard persistence).** No clear site, in any version. It's raised just before the tier-3 reset, so it may not be saved. Once the ThrashGuard/persistence work saves state before a deliberate reset, 18 becomes report-once like 19 and 42. A stuck 18 is a known limitation, not an ongoing fault. |
| 19 | watchdog reset | Boot classified as a Device OS watchdog or an AB1805-confirmed pin reset (G:1317) | **4, the highest** (:876–877) | committed `75ad0a8`; first released in v22-Diag-Soak (`3b9b39f`) | event | **Report once, then clear, since v38** (`isAutoClearAfterReportAlert()`; raise-site comment G:1310–1316). **Before v38: never** (deliberately excluded), so a device on older firmware keeps 19 and it hides every lower alert. |
| 20 | PMIC thermal | NTC fault (P:192); charge fault 0x02, thermal shutdown (P:211) | 3 (:865) | v3.20 (`3f29873`) | condition | Charging resumed, codes 20–23 (P:369–371) |
| 21 | PMIC charge timeout / stuck | Input fault persisting after a reset (P:206); safety timer expired (P:216); other charge fault persisting after a reset (P:221); fast charging 6 h+ with no material gain (P:523) | 3 (:866) | v3.20 | condition | Charge fault cleared (P:169–171); P:369–371; charge status ≠ 2 after recovery (P:378–380) |
| 22 | — | Not raised; not in the severity table | 1 (default, :890–891) | — | — | Within P:369–371's 20–23 range |
| 23 | PMIC battery fault | Other charge fault (P:224); BAT_FAULT overvoltage (P:390) | 2 (:879) | v3.20 | condition | P:169–171; P:369–371 |
| 30 | connectivity timeout with radio up | Not raised | 2 (:880) | — | — | — |
| 31 | failed to connect to cloud | Connection attempt exceeded its budget (C:666) | 2 (:881) | v3.09 | event | Report; a successful connection (C:539–540); E:128–130 |
| 32 | connect taking too long | Not raised | 2 (:882) | — | — | — |
| 40 | repeated webhook failures | Webhook response timeout (G:1692); no successful response for over 3 h in open hours (R:116); no webhook response before sleep (S:640) | 2 (:883) | v3.09 | condition | Any non-empty webhook response (G:2412–2414) |
| 41 | config / ledger apply failure | Configuration apply failed at connect (C:566) | 2 (:884) | v3.09 | event | Report |
| 42 | data ledger publish failure, **or** OTA updates pending | Report's data-ledger publish failed (G:2114); ConnectState's data-ledger publish failed (C:574); OTA updates pending at the sleep gate (S:637) | 2 (:885) | v3.09 | event | **Report once, then clear, since v38** (`isAutoClearAfterReportAlert()`). **Before v38: never** (no clear site), so a device on older firmware keeps 42. For "OTA updates pending", the sleep gate raises it again on each timeout while the update is pending (S:506, S:635–637). |
| 43 | publish queue not drained before sleep | S:628 | 2 (:886) | v3.09 | event | Report |
| 44 | ledger sync timeout before sleep | S:634 | 1 (:888–889) | v3.12 (`15db55c`) | event | Report; after the boot status event (G:1406–1408) |
| 45 | connectivity failsafe (`ConnectivityPolicy.h:119`) | Not raised by current firmware | 1 (default) | — | condition (legacy) | On a successful connection (C:514 → G:2531–2543) |

**ERROR_STATE actions** (E:`resolveErrorAction()`):
- **15, 31 and 44:** soft reset, then a hard power cycle, then stop after 4 resets.
- **16:** a soft reset, then a hard power cycle.
- **40:** a soft reset if the clock is trusted and there's been no webhook response for over 3 h.
- **17 and 18:** none.
- **Every other code:** none.
- **In non-CONNECTED modes,** 15, 16 and 31 are cleared instead (E:128–130).

## Differences from the fleet-ops table, resolved against the source

The fleet-ops agent's table was built from received payloads and used only as a cross-check.

1. **Line numbers:** most of fleet-ops' citations matched v37's branch (`85bd316`), not the pre-v37 main this reference was first cited at (`7a2b026`). Since v37 merged, they match main (`e78d59d`), and this reference is now cited there; examples: 15 at S:833/953, 31 at C:666, 41 at C:566.
2. **14:** fleet-ops says the raise is "gone since". **Resolved:** removed in v34 (`1c74ac8`). A legacy 14 clears only on a reset-driven boot.
3. **16:** fleet-ops says "Device OS rejects the ULP sleep config". **Resolved:** raised when `ULTRA_LOW_POWER` sleep returns *any* error. A rejected configuration is the source comment's example.
4. **17:** "first version v3.29 (`8843d41`)" is **confirmed**: it was first raised through `raiseAlert(17)`, and became the direct `set_alertCode(17)` in v4.05 (`98d98d8`). **Added:** because the write bypasses severity, 17 overwrites a 19.
5. **22:** missing from the fleet-ops table. **Added:** never raised and not in the severity table, but inside the PMIC clear range.
6. **31 clear:** fleet-ops says "next connect attempt". **Resolved:** a *successful* connection (C:539–540, in the post-connect path).
7. **40 clear:** fleet-ops says "any webhook response". **Resolved:** any **non-empty** response; an empty one is logged and doesn't clear it.
8. **42:** fleet-ops' raise sites are **confirmed**. **Added:** the code has two meanings (a ledger publish failure, and OTA updates pending).
9. **18, 19 and 42 "Never":** **confirmed** for firmware before v38. Since v38, 19 and 42 report once and then clear; 18 still never clears.
10. **Confirmed unchanged:** the severities (including 23 at 2, 44 at 1, 45 at the default 1), the first versions of 14/15/16/18/19/20/21/23/31/40–44, 19's release in v22-Diag-Soak, and the clear sites for 16, 17, 20, 21, 23, 44 and 45.

## Keeping this current

Any WO that adds, removes or changes how an alert is raised, ranked or cleared updates this file in the same PR, and Stage 7 checks it (`AI_DEVELOPMENT_WORKFLOW.md`, Stage 7).
