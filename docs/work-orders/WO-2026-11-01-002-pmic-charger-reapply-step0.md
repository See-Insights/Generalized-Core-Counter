# WO-2026-11-01-002: can GCC re-apply the charger when power is present and the battery is not charging? Step 0 (read-only)

**Author:** Claude Code, 1 Nov 2026, from the architect's instruction on recovery item 22 (extended to USB units); the architect may amend · **Status:** QUEUED, not dispatched
**Branch:** `docs/2026-10-31-pmic-fault-latch-item` (PR #168), report added there
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 4 (independent investigation); §13 (c), (j)

## Plain goal

OCCUPANCY-LAB-01 (USB powered, v47) read "Not Charging" at 36.14%, `vc` 3.71-3.75 V, from 12:00 to 15:00Z on 1 Nov; after the 15:30:04Z failsafe stage-2 reset it read "Charging" at 4.18 V by 15:59:55Z (recovery item 22, USB case). Trail02 (solar) needed a power cycle. **This Step 0 answers whether firmware can clear such a state by itself:** what GCC configures in the PMIC and charger, what an MCU reset re-applies, what the charger's safety timer and fault states do, and whether a "power present, not charging, not full for N hours: re-initialize the charger" check is worth its lines. No code, no device change.

## Questions (OBS = observed in source, INF = inferred, UNKNOWN; cite file:line)

1. **What GCC sets up.** Every PMIC and charger call in `src/` (bare-name search per §13 (c): `PMIC`, `PowerManagement`, `System.enableFeature`, `setChargeVoltage`, `setChargeCurrent`, `setInputCurrentLimit`, `enableCharging`, `disableCharging`, `PmicFaultMonitor`, `ChargeInhibit`, `PowerPlatform`), with the values and where in boot or the loop each runs. Which are applied on every boot and which only on a trigger?
2. **What an MCU reset re-applies.** Trace Device OS 6.6.0's power-manager and PMIC init (`~/.particle/toolchains/deviceOS/6.6.0`, the `hal/` and `system/` power-manager sources): what happens to the charger's registers across an MCU reset, a `System.reset()`, a hibernate wake and a full power cycle; whether the PMIC watchdog resets registers to defaults; whether charging is re-enabled and the charge cycle and safety timer restarted at boot. Say what differs between a reset (LAB-01 cleared) and a power cycle (Trail02 needed one).
3. **The BQ24195 safety timer and fault states.** The fault register values and meanings the code reads (`0x01` input, `0x02` thermal, `0x03` safety timer; `BatteryState` 5 = Fault), how "Not Charging" arises (`BatteryState` mapping), and what clears each. **The BQ24195 datasheet is not in `docs/datasheets`:** state anything that depends on it as UNKNOWN unless Device OS's own driver shows the register and bit, and list what datasheet pages Chip should add. Does a safety-timer expiry on a USB unit with a drained cell leave "Not Charging" until a restart of the charge cycle?
4. **Thermal charge-inhibit.** The `ChargeInhibit` settings (thresholds, hold, hysteresis, when it disables charging and when it re-enables); LAB-01's `temp` was 24-26 C, so say whether it could have been inhibited and what the code would report.
5. **`PmicFaultMonitor` on a non-fault "Not Charging".** Which states it escalates on (alerts 20/21/23; the soft cycle after 2 faulty polls; the PMIC power cycle after 3; the 1 h cooldown; the immediate reset gated on a USB-backed input and `powerGood`), and whether "Not Charging" with power present and no fault bit is seen at all. LAB-01's `status` showed `pmicAnomalyCount` 0.
6. **The candidate check.** Draft: *power present (USB-backed input and `powerGood`), not charging, not full, for N hours: re-initialize the charger (the existing level-1 soft cycle, or a charger register reset)*. Give: the inputs it would read (all already sampled?), the state it needs (a retained counter would touch the persisted layout: say so and propose a RAM-only design), where it would run, the choice of N against LAB-01's 3.5 h, the false-positive cases (a full battery reads "Not Charging" too; a hot pack; a weak supply), the alert it would raise, and the **line cost in `src/`** with the arithmetic. Check the +20 net-lines budget per WO.
7. **Test shape.** What host test would pin it, and the bench step for a USB unit (Chip's hands) without breaking the battery.
8. **Stop decision.** STOP if firmware cannot re-apply the charger at all, or the check cannot be told from a full battery. Otherwise PROCEED to a design WO.

## Evidence (Claude Code, read-only, before dispatch)

- The `src/` call sites and files above, the Device OS 6.6.0 power sources, and `docs/` references.
- LAB-01's S3 data: the 1 Nov hook series (12:00:08Z, 15:59:50-52Z, 15:59:55Z, 16:04Z, 17:00Z, 18:00Z, 18:48Z, 19:00Z) and `status` events (reset 70 at 11:01:49Z, reset 140 data 2 at 15:59:54Z, reset 40 at 18:55:18Z, 18:55:19Z, 18:57:01Z). These are in `WO-2026-10-29-001-pilot-record.md` (PR #170).
- Court1, Court3 and Trail02 facts from recovery item 22.

## Agents and models

- **Evidence:** Claude Code, read-only.
- **Analysis:** Codex `gpt-5.6-sol`, **high**, read-only, **stdin closed**, run with `--skip-git-repo-check` from the scratchpad (it refuses outside a trusted git dir). Probe model availability first.
- **Controller check:** every citation and number against the sources (§13 (j)).

## Output and budget

- A report of at most 150 lines (`WO-2026-11-01-002-step0-report.md`), committed on the branch above, then stop for the architect.
- 0 `src/` lines. Standing rules: read-only; `retained` RAM and the persisted layout are untouched.
