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

## LAB-01 events, 1 Nov (moved)
The 1 Nov timeline (the 15:30:04Z failsafe stage-2 reset, the three reset-40 power cycles at 18:55Z, the SoC re-estimate to 92.28%) and the charge reading are recorded in `WO-2026-11-01-002-step0-report.md` ("LAB-01 events, 1 Nov") and recovery plan item 22 (PMIC charge-fault latch, extended to USB units). Not repeated here. What this record keeps: the 13:00Z report was queued and delivered at 15:59:50Z, so the "4x tier" reading above is **not established**, and the stage-2 reset fired correctly (field evidence for the failsafe ladder).

## 24 h checks (2 Nov; S3 hooks and `status`, read 10:10Z and 11:02Z)
- **ChurchSt-2: FINDING (the schedule), the rest PASS.** Hourly reports for every open hour on 1 Nov (11:00 to 22:00Z), start and end reports present, the 22:00:07Z close report read the day total (95 min), and the first report after the open (06:01:23Z) read `dailyoccupancy` 0. `cyc - slp` = 1 on every report, alerts 0, `resets` 0, one boot status (reset 70, 10:02Z) and no later status event, battery 86.0% to 85.9% (86.9% peak; "Not Charging" overnight at 4.05-4.10 V, normal on solar), `fh` flat (80.6k to 80.9k, one 84.9k at the morning wake), one slow connect (65 s at 07:00Z 2 Nov).
  - **The finding:** the device runs a **06:00-22:00 UTC** day. The default is 06:00-22:00 `EST5EDT`; after the 1 Nov fall-back that is reports from **11:00Z** and a close at **03:00Z**. ChurchSt-2's last report on 1 Nov was **22:00Z**, then nothing until **06:01Z**, then hourly from 07:00Z through 11:00Z (01:00-06:00 EST, closed hours under EST). At **09:29Z** (04:29 EST) it counted a 5-minute session (`dailyoccupancy` 5 at 09:34Z), which an Eastern closed-hours schedule would have ignored.
  - **LAB-01 on the same product defaults shows the Eastern schedule:** reports through **03:00:09Z** (the close, day total 10), silence 04:00-10:00Z, first report of the day **11:00:44Z** with `dailyoccupancy` 0.
  - **Cause:** the timezone from the ledger reached the stored setting but not the running clock until the next boot. Derivation (both devices on `config.generation` `EF590054`, `LocalTime::withConfig` called once in `setup()`, why LAB-01 differed) is recorded in recovery plan item 25 and `WO-2026-11-02-004-timezone-applies-to-running-clock.md`. The fix (the Eastern default from first boot) is #176, in v49.1.
  - **Status of the test (added on rebase, 6 Nov):** the reset of ChurchSt-2 on 2 Nov and the later confirmation that the schedule follows Eastern time over two days are Chip's (6 Nov: item 25 confirmed over two days); the evidence is not repeated in this record.
  - **Effect:** counts and resets are internally consistent, but the "day" is the UTC day, so overnight Eastern time is counted as open and the daily minutes will not match the old unit's local day. **Hold the 72 h verdict and the waves on ChurchSt-2 until the schedule is confirmed after the reset** (architect to rule).
- **OCCUPANCY-LAB-01: PASS.** Hourly reports through the evening (19:00 to 03:00Z), the close at 03:00:09Z (day total 10), the open at 11:00:44Z reads 0; `cyc - slp` = 1 (11 and 10 at the open), alerts 0 since the one alert 44 at 16:04Z 1 Nov, `watchdogResetCount` 0, no status event since the 18:55Z power cycles; one slow connect (312 s at 00:00Z 2 Nov, one report). **Battery 92.6% at 4.12 V, "Charged", holding since the 19:00Z gauge re-estimate** (92.28% to 92.44% to 92.6%), so the tier is back to hourly with no touch. `fh` flat (80.8k; 85.1k at the open). Only 10 occupancy minutes in the day (two sessions), so the daily-minutes magnitude comparison waits for 72 h.

## Next
- As of 2 Nov: 72 h per §10 (ChurchSt-2 on hold pending the schedule question above); the waves wait for v48 and the architect's ruling. Later: item 25 is fixed (#176) and confirmed (Chip, 6 Nov); wave 1 is next (see `docs/handover/2026-11-06-handoff.md`).
