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
  - **Tracking (architect):** `battery` and `key1` through today's daylight (does it charge?): reports expected at 16:00Z and 20:00Z, plus 00:00Z and 04:00Z; Chip will report on the panel.

## Next
- 24 h checks: ChurchSt-2 about 10:02Z and LAB-01 about 11:01Z on 2 Nov; 72 h per §10.
