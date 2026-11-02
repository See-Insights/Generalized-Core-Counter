# WO-2026-10-29-001: occupancy migration pilot record

**Author:** Claude Code, 1 Nov 2026 · **Plan:** `WO-2026-10-29-001-step0-report.md` §10 (PR #165, revised in #170) · **Devices:** ChurchSt-2 `e00fce686fd4bda70a18c449` (Ubidots `6195a59f1d84727eb18f7cd6`), OCCUPANCY-LAB-01 `e00fce683d329595679c374e` (Ubidots `6abbcf5ddf5a0479db154a94`)
Sources: S3 `particle-events` (`status`, `Ubidots-Sensor-Hook-v1`), read 1 Nov 13:49Z and 14:04Z; Ubidots read by the architect.

## Moves (Chip, 1 Nov about 07:05Z)
Both moved from product 14220 to 42131 and locked to v47; no `device-settings` ledger. Particle API (13:49Z): both listed under product 42131, offline between reports.

## First boots (S3 `status`, reset 70, `v47-PersistFreeze`)
| | ChurchSt-2 | OCCUPANCY-LAB-01 |
|---|---|---|
| First v47 boot | 10:02:49Z | 11:01:49Z |
| Old-format reports flushed first | 10:02:46Z (1; stamped 09:00Z) | 11:01:43-48Z (4; stamped 01:00-10:00Z) |
| First v47-format report | 11:00:07Z | 12:00:08Z |
| `watchdogResetCount`, alerts, other resets | 0, 0, none | 0, 0, none |

The first v47 report of each is at or after the 06:00 EST open (11:00Z after the 1 Nov clock change). **OTA size and time, and Device OS 6.6.0, are not in S3;** they need the Particle console or API (not recorded).

## Gate 2: PASS (architect, from Ubidots, 1 Nov)
- **ChurchSt-2:** variables since 18 Nov 2021 continue through 13:00Z; `vc`, `fh`, `lfb`, `cyc`, `slp` were created at 10:02Z; 10 to 15 variables.
- **OCCUPANCY-LAB-01:** variables since 29 Sep 2026 continue through 12:00Z; the new five were created at 11:01Z; 7 to 12 variables.
Same Ubidots device, history unbroken, the v47 payload keys present (same keys and order as Dev-09's).

## 2 h checks (1 Nov)
- **ChurchSt-2: PASS.** One reset 70, no loop; hourly reports at 11, 12 and 13Z; `cyc - slp` = 1 each; battery 85.6%, 4.03 V, Charging at 13:00Z; `connecttime` 25 s then 1 s.
- **OCCUPANCY-LAB-01: open.** Reset 70 once, no loop, one report (12:00Z, `cyc` 2, `slp` 1, `connecttime` 10 s). **No 13:00Z or 14:00Z report (checked 14:04Z).** Battery **36.14%**, 3.71 V, "Not Charging" at 12:00Z, below §10's bar of about 40%.
  - **Probable cause (INF, from source): a battery-tier interval change, not a miss.** `BatteryBackoff::calculateTier` gives `TIER_CRITICAL` for 35% <= SoC < 50% (`src/cloud/BatteryBackoffPolicy.h:32-34`) and `intervalMultiplier(TIER_CRITICAL)` is 4 (`:45-57`), so the 3600 s base becomes 14,400 s on epoch-aligned boundaries (`State_Report.cpp:255-259`). 12:00Z is such a boundary (1793534402 s / 14,400 = 124,551.0); the next are **16:00Z and 20:00Z**. The tier is not in the payload or `status`, so this stays INF until the 16:00Z report. If a report is missing at 16:00Z, it is a genuine miss.
  - **Accepted (architect, 1 Nov): judge LAB-01 against its tier's interval, 4x at 35-50% (every 4 h on the epoch boundaries), not hourly. A missing 16:00Z report would be a real miss.**
  - **Tracking:** record `battery` and `key1` at each daylight report (16:00Z, 20:00Z; then 00:00Z and 04:00Z) to see whether it charges. **If it climbs out of the tier (to 50% or more it is CONSERVING, 2x; to 75% or more, with the hysteresis at 70-75%, HEALTHY, hourly), confirm the reports resume at the shorter interval by themselves, with no touch** (`BatteryBackoffPolicy.h:19-41,45-57`).
  - **A useful case:** LAB-01 is **exercising the low-battery reporting path in the field**, with a real device on a real panel. Record it as such; do not treat the 4 h gap as a pilot failure.
  - **Correction (Chip, 1 Nov): LAB-01 is USB powered, not solar.** The earlier "all 13 movers are solar" ruling (report, controller check) does not hold for it, and there is no panel question. **The open question to Chip is now why a USB-powered device sits at 36% and "Not Charging"** (12:00Z; the old firmware's reports said "Discharging"): the supply, the cable or port, or a charge fault. Chip is checking its charging status and will update.
  - **Reading until then (INF):** a USB-fed battery that does not charge fits a weak or absent supply (`vc` 3.71 V is the battery voltage, not VBUS), or a PMIC charge-fault state like recovery item 22. Neither is established; the next reports' `key1` and `battery` are the evidence. If it does not climb with a good supply, the 4 h interval stays and the pilot judges the low-battery path on a device that cannot recover by itself.

## LAB-01 events, 1 Nov (S3 `status` and hooks, read 23:44Z; no serial for this device)
| Time (Z) | Event |
|---|---|
| 12:00:08 | v47 report: 36.14%, 3.71 V, "Not Charging"; the last connect for 3.5 h |
| 15:30:04 | **Failsafe stage-2 reset** (status reset 140, data 2, `failsafeStage` 2, count 1, breadcrumb 10 = `CONNECTIVITY_FAILSAFE_HARD`; `failsafeLastAction` 1793547004 = 15:30:04Z; uptime 4.48 h) |
| 15:59:50-52 | **The 13:00, 14:00 and 15:00Z reports arrive** (payload stamps 13:00:02, 14:00:02, 15:00:02), still "Not Charging" at 3.75 V; then the reset's status at 15:59:54 |
| 15:59:55 | first post-reset report: **"Charging", 4.18 V**, SoC 36.14%; alert 44 (ledger sync timeout, minor) at 16:04 |
| 16:04 to 18:48 | hourly reports; SoC 39.02, 39.38, 40.11, 40.46%; "Charging" then "Charged" at 4.13 V |
| 18:55:18 to 18:57:01 | **three reset-40 boots** (power cycles; Chip checking the charge status) |
| 19:00 on | **SoC 92.28%**, "Charged", 4.13 V, hourly reports, `cyc - slp` = 1, alerts 0 |

- **Correction to the tier explanation above:** the 13:00Z report was **not missing**: it was queued and delivered at 15:59:50. The "4x tier" reading was an inference from the 12:00Z SoC alone and is **not established**: the tier also depends on the cached cell voltage and SoC trust (`BatteryAuthorityCommand.cpp:45-50`), and after the reset the device reported hourly at SoC 39-40%. The evidence does not show which tier it was in. Do not treat the 4 h gap as a tier result.
- **The failsafe reset fired correctly (architect, 1 Nov):** the 15:30:04Z stage 2 came after about 3 h of failed connects (the 13:00-15:00Z reports were queued, not sent) and **recovered both the connection and the charging** (item 22, USB case). That is **field evidence for the failsafe ladder.** (S3 cannot show the failed attempts themselves; the queued 13, 14 and 15Z reports and the 3.5 h since the 12:00Z connect are what we see. My earlier note questioning the 7 h cadence rule is withdrawn: the supervisor's view of the cadence is not visible in S3.)
- **The charge story (INF):** USB-fed, the unit showed "Not Charging" at 3.71-3.75 V until a reset, and "Charging" at 4.18 V immediately after it. The SoC then read low (36-40%) at 4.13 V until the 18:55Z power cycle, and 92.28% after it. That fits a charge that was off until the reset and a fuel gauge that was re-estimated by the power cycle (as for Trail02, "a gauge re-estimate, not a flat battery"), and is a candidate data point for recovery item 22 (PMIC charge-fault latch; item 22 says solar devices only, LAB-01 is USB). Not established: Chip's check of the supply is pending.

## Next
- 24 h checks: ChurchSt-2 about 10:02Z and LAB-01 about 11:01Z on 2 Nov; 72 h per §10.
