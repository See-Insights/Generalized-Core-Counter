# v47 fleet health check: report (graded 30 Oct 2026 23:34Z, the machine clock; 31 Oct in Singapore)

**Format:** the v44 health-check WO checks 1-9, v46 report as the baseline. **Grader:** Sonnet 5.5 main session (read-only). **Sources:** S3 raw archive, UTC days 25-30 Oct 2026 (the 31 Oct folder was empty at 23:34Z): `Ubidots-Sensor-Hook-v1` (3,518 reports), `status` (63), `watchdog` (3 files), `hibernate_wake` (31), all for the 11 devices; Particle API `GET /v1/products/42131/devices` and the `device-status`, `device-settings` and `device-data` ledger instances, all read at about 23:33Z 30 Oct. Nothing was changed anywhere.
**v47 window** per device: its first v47 boot (status reset 70) to 30 Oct 23:34Z, about 47 h (US) and 73 h (Dev-09, Dev-14). **v46 baseline** per device: the 48 h before its OTA, capped at its first v46 boot (27 Oct 00:28-02:00Z for the US devices, so 45-47 h; Dev-09 from 25 Oct 23:01Z, Dev-14 from 25 Oct 22:41Z).

## Top line

- **Rollback triggers: none.** (a) No device dark 4 h or more in open hours: the only 4 h+ gaps in hook `published_at` are the closed-hours night gaps (US 02:00Z to about 10:00Z, Trail02 03:00Z to 10:00Z, bench 14:00Z to 22:00Z). (b) One breadcrumb-28 on v47 fleet-wide (PCKL1), not more than one; no reset 140 on any device. (c) No missing hourlies on any device.
- **All 11 devices are on v47.** Particle API `firmware_version` is 47 for all 11 and each status shows `v47-PersistFreeze`; all 11 were last heard between 23:00Z and 23:33Z 30 Oct. OTA times in the table (observation).
- **Grades: PCKL1 FINDING** (one breadcrumb-28 sleep watchdog 59 min after its v47 boot, new against its own v46 rate of 0). **The other eight US devices PASS.** Dev-09 and Dev-14 PASS (completeness only).
- Separate **WATCH, not v47's**: PCKL1's connect time rose from 2 s to 12-13 s at 28 Oct 10:00Z, on v46, 13 h before the v47 OTA (detail below).

## Table 1: resets, watchdogs, reports (checks 1-4)

| Device | First v47 boot (reset 70, UTC) | Resets since OTA (reason, data) | `watchdogResetCount` pre > post | bc28 per 1,000 sleep returns, v47 vs v46 | Missing hourly, open hours (v47) | Verdict |
|---|---|---|---|---|---|---|
| Court1 | 28 Oct 23:15:45 | one reset 0 data 0 (30 Oct 11:12:43Z) | 20 > 20 | 0 (0/1,323) vs 0 (0/1,499) | 0 of 34 | PASS |
| Court2 | 29 Oct 00:05:06 | none | 2 > 2 | 0 (0/693) vs 0 (0/784) | 0 of 33 | PASS |
| Court3 | 28 Oct 23:02:31 | none | 77 > 77 | 0 (0/5,773) vs 0 (0/4,736) | 0 of 34 | PASS |
| **PCKL1** | 28 Oct 23:01:50 | **reset 60 (watchdog) data 0, 29 Oct 00:00:40Z** | **30 > 31** | **0.39 (1/2,591)** vs 0 (0/2,468) | 0 of 34 | **FINDING** |
| PCKL2 | 28 Oct 23:06:18 | none | 12 > 12 | 0 (0/3,271) vs 0.37 (1/2,668) | 0 of 34 | PASS |
| PCKL3 | 28 Oct 23:08:52 | reset 0 data 0 at 30 Oct 00:01:16Z, 00:01:19Z, 18:04:20Z | 8 > 8 | 0 (0/1,433) vs 0 (0/834) | 0 of 34 | PASS |
| MAFC-1 | 29 Oct 00:01:43 | none | 83 > 83 | 0 (0/986) vs 0 (0/2,297) | 0 of 33 | PASS |
| MAFC-2 | 28 Oct 23:02:18 | none | 37 > 37 | 0 (0/1,020) vs 0 (0/2,150) | 0 of 34 | PASS |
| Trail02 | 29 Oct 00:00:44 | reset 30 data 0 at 29 Oct 10:00:26Z and 30 Oct 10:00:40Z (expected wakes); **no reset 40** | 37 > 37 | 0 (0/34) vs 0 (0/29) | 0 of 35 | PASS |
| Dev-09 (bench) | 27 Oct 22:37:31 | reset 30 data 0 nightly at 22:00Z (28, 29, 30 Oct), expected | 46 > 46 | 0 (0/1,609) vs 0 (0/1,027) | 0 of 52 | PASS |
| Dev-14 (bench) | 27 Oct 22:41:56 | reset 30 data 0 nightly at 22:00Z, expected | 127 > 127 | 0 (0/1,344) vs 0 (0/798) | 0 of 52 | PASS |

Method (observation): `watchdogResetCount` is the pre-OTA status value against the last status in the window. Sleep returns = sum of positive hook `cyc` deltas per device over the window. bc28 events = S3 `watchdog` files with `bc` 28 in the window. A hourly slot is 10:00Z to 02:00Z (Trail02 to 03:00Z; bench 22:00Z to 14:00Z) and counts as present if a hook report has a device `timestamp` within 30 min of the hour. **Fleet bc28 context:** US devices v47 1 event in 17,124 returns (0.058 per 1,000) against v46 1 in 17,465 (0.057; the PCKL2 event of 27 Oct). Both within the earlier background of 0.05-0.09. Reset 140: none on any device, so no failsafe finding and no heap-guard 140.

**Lateness (observation, not a miss):** a late report is one published more than 5 min after its device `timestamp`. v47: Court1 one (30 Oct 14:12Z, 12.9 min), Court3 two (29 Oct 23:08Z 8 min; 30 Oct 16:10Z 10 min), MAFC-1 one (30 Oct 11:06Z, 6 min), PCKL3 one (the 23:00Z 29 Oct hourly arrived 00:01Z 30 Oct, 61 min, then the reset-0 boots at 00:01Z). All arrived; none lost. Whether v46 had the same is not determined (not measured).

## Findings

1. **PCKL1: one breadcrumb-28 sleep watchdog on v47** (observation). `watchdog/` file 2026-10-29T00:00:40.479Z: osReason 60, bc 28, stage sleep, elapsed 51, state 3, connAge 689,218. Status 00:00:41.773Z: reset 60 data 0, `lastWatchdogBreadcrumb` 28, `watchdogResetCount` 30 to 31, alert 19, failsafe 0/0. It came 58 min 50 s after PCKL1's first v47 boot (28 Oct 23:01:50Z), at the 00:00Z wake. **Nothing lost:** the 00:00Z report arrived (hook alerts 19, `connecttime` 2 s, `cyc` 121), then 01:00Z, 02:00Z on. By the rule (higher than its own v46 rate of 0 in 2,468 returns) a FINDING. Context (inference): one event in 2,591 returns, in line with the fleet background; same signature (stage sleep, bc 28) as PCKL2's 27 Oct event on v46 and PCKL3's on v45 (25 Oct 22:01Z), the known in-`System.sleep()` stall (recovery plan item 14). Whether v47 changed the rate is not determined from one event. The Particle ledger shows PCKL1 `startup.reason` watchdog, `resetCount` 1.
2. **Gauge/label note, no grade (observation):** hook `key1` read "Fault" once each on Court3 (29 Oct 12:00:53Z, battery 88.07, vc 3.97) and Court1 (30 Oct 12:00:21Z, 85.34, vc 4.04), never on v46 or on 25-26 Oct (v45). SoC and voltage normal, ledger `chargeState` NOT at 23:33Z, no alert. Cause not determined; not one of the nine checks.

## Table 2: heap, alerts, battery, connect time (checks 5, 6, 7, 9)

| Device | `fh` min / median / max, v47 (v46) | Alerts (v47) | Battery first > last > min, v47 | Ledger tier / socTrust | Connect time median / p90 / at 8 s or more, v47 | Same, v46 |
|---|---|---|---|---|---|---|
| Court1 | 76,016 / 80,708 / 89,128 (74,320 / 80,720 / 89,104) | none | 88.50 > 87.93 > 85.05 | HEALTHY / Trusted | 2 / 14 / 36 of 92 (39%) | 7 / 15 / 38 of 80 (48%) |
| Court2 | 75,496 / 80,752 / 85,064 (75,800 / 80,728 / 85,144) | none | 86.78 > 86.63 > 85.63 | HEALTHY / Trusted | 1 / 5.8 / 4 of 83 (5%) | 1 / 1 / 4 of 84 (5%) |
| Court3 | 75,352 / 85,032 / 85,048 (75,072 / 85,024 / 85,040) | 40 once (29 Oct 10:01Z, the open) | 91.02 > 91.49 > 86.92 | HEALTHY / Trusted | 10 / 14 / 79 of 99 (80%) | 10 / 14 / 90 of 99 (91%) |
| PCKL1 | 75,424 / 85,088 / 89,096 (75,360 / 85,008 / 85,072) | 19 once (29 Oct 00:00Z, after the watchdog) | 89.91 > 90.22 > 88.07 | HEALTHY / Trusted | 13 / 16 / 96 of 97 (99%) | 12 / 15 / 35 of 63 (56%) |
| PCKL2 | 75,808 / 80,696 / 85,040 (75,912 / 80,808 / 89,112) | none | 88.21 > 87.64 > 85.91 | HEALTHY / Trusted | 1 / 1 / 5 of 85 (6%) | 1 / 1 / 6 of 72 (8%) |
| PCKL3 | 75,600 / 80,688 / 89,136 (80,616 / 80,712 / 89,088) | 44 once (30 Oct 18:10Z, after the 18:04Z boot) | 88.36 > 87.78 > 86.34 | HEALTHY / Trusted | 1 / 3 / 8 of 121 (7%) | 1 / 4.3 / 8 of 118 (7%) |
| MAFC-1 | 75,816 / 80,640 / 84,960 (80,608 / 80,680 / 85,080) | none | 88.96 > 86.49 > 86.06 | HEALTHY / Trusted | 2 / 7.2 / 7 of 69 (10%) | 2 / 7 / 5 of 54 (9%) |
| MAFC-2 | 80,448 / 80,656 / 85,008 (80,584 / 80,672 / 85,072) | none | 86.06 > 86.92 > 85.63 | HEALTHY / Trusted | 1 / 1.8 / 4 of 99 (4%) | 1 / 1.7 / 4 of 64 (6%) |
| Trail02 | 75,936 / 80,832 / 80,880 (75,416 / 80,896 / 85,168) | none | 90.07 > 90.38 > 90.07 | HEALTHY / Trusted | 1 / 9 / 5 of 37 (14%) | 6 / 18.7 / 14 of 34 (41%) |
| Dev-09 | 74,232 / 80,456 / 89,088 (74,272 / 80,432 / 89,088) | none | 79.77 > 79.77 > 74.05 | HEALTHY / Trusted | 1 / 5 / 29 of 349 (8%) | 1 / 5.5 / 17 of 226 (8%) |
| Dev-14 | 74,424 / 80,704 / 89,080 (74,240 / 80,736 / 89,080) | 40 x4, 31 x4, 44 x2 (poor link) | 80.42 > 80.59 > 79.77 | HEALTHY / Trusted | 3 / 231 / 123 of 349 (35%) | 11.5 / 488 / 118 of 226 (52%) |

Source and method: hook payload fields `fh`, `alerts`, `battery`, `connecttime` over each window; p90 by linear interpolation; ledger tier and trust from `device-status` `battery.tier` and `battery.socTrust` at 23:33Z 30 Oct (all `lowBatteryMode` false).

- **Heap (check 5):** the same clusters (about 80.7 k and 85.0 k, boot-time dips to about 75 k, 89 k after a boot) on every device as on v46. Per-day medians are flat, for example Court1 80,720 / 80,704 / 85,136 (28, 29, 30 Oct) and PCKL2 80,744 / 80,712 / 80,696; Court3 85,008 / 85,032. **No steady per-wake fall** on any device.
- **Alerts (check 6):** **no alert 41 and no alert 15 on any device** (hook `alerts` and status `alert` fields). Alert 40: Court3 once at the open (known, recovery item 7; v46 also once, 28 Oct 10:04Z); no US device has two or more. Alert 19: PCKL1, after its watchdog. Alert 44: PCKL3 once after a boot (known). Dev-14's alerts are the Singapore link (v46 had 15).
- **Connect time (check 9):** no regression on any device except the one that is not v47's: **PCKL1** 99% at 8 s or more against 56%. Its connect times by UTC day: 25-27 Oct median 2 s (2, 2, 2 at 8 s or more of 37, 34, 28); **28 Oct 12 s (34 of 38)**, 29 Oct 12 s (39 of 40), 30 Oct 13 s (56 of 56). The first slow report was 28 Oct 10:00Z (13 s) after 25-27 Oct at 1-7 s, 13 h before the v47 OTA at 23:01Z. So the shift is on v46, not v47 (observation); cause not determined (no serial). Court3 and Court1 already ran 10-11 s medians on v45 and v46 (Court1 varies by day: 29 Oct median 1 s). Trail02 improved against v46 (the 41% included its wake-day reconnects). Largest single connects on v47: Court3 588 s, MAFC-1 376 s, Court1 90 s, PCKL3 100 s, MAFC-2 84 s; v46 had 372, 16, 317, 36, 287 s respectively (different devices spike each window); none repeated in a pattern. Warm-to-cold steps and alert 15: not determinable beyond the hook (no serial on US devices).

## Standing checks (checks 2, 7, 8)

- **Failsafe (check 8):** `failsafeStage` and `failsafeCount` 0/0 in every v47 status event (63 statuses, 11 devices).
- **Reset 0 boots (observation):** Court1 one on v47 (30 Oct 11:12:43Z; ledger `startup` 11:11:54Z, reason none, `watchdogResetCount` unchanged, no watchdog file) against five on v46 (28 Oct 13:00:48Z, 13:00:51Z, 13:00:52Z, 14:00:30Z, 22:00:26Z). PCKL3 three on v47 against two on v46 (27 Oct 16:26Z, 20:42Z); this is the known PCKL3 pattern (bc 18, once bc 15 on v46; seven reset-0 statuses on 25 Oct on v45). The reset type was seen on v46 on both, so not a new type, and no report was lost.
- **Mode fields (check 7):** `connectionMode` and `sensorMode` are not in the `device-status` ledger. The `device-settings` ledger has `connectionMode` 3 only for Dev-09 (also `verboseTimeoutMin` 59; Dev-14 58); the eight US devices other than Trail02 have no `device-settings` instance (404), and Trail02's holds only `closeHour` 23 and `enableHibernateSleep` true. So connectionMode and sensorMode are **not determined** for the US devices.
- **Power source (ledger):** US devices BATTERY (solar profile), Trail02 VIN, bench USB_HOST. `overrideActive` true only on Dev-14.
- **Trail02 (check 2 note):** the two reset 40 events of 27 Oct on v46 did not recur on v47; reset 40 is its recurring power-path history (v46 report). Its hibernate wakes read reset 30 `UNKNOWN` `gateArm` wake_reason in `hibernate_wake`, unchanged from v45 and v46 (6 of 6 in 25-30 Oct). Battery steady 90.07-90.38%.
- **Bench (completeness only):** Dev-09 and Dev-14 have been on v47 since the 27 Oct lock; their nightly 22:00Z wake is reset 30 `ALARM`, `result` ok; no bc28; `watchdogResetCount` unchanged. Their stale-cloud pattern is expected.

## Not determined

The exact time each US device's OTA was queued (only the first-boot time is observable); `connectionMode` and `sensorMode` for the US devices; `slow_teardown`, `ConnSummary` and alert 15 beyond the hook field (no serial on US devices); the cause of PCKL1's watchdog, of its connect-time shift on 28 Oct 10:00Z, and of the two "Fault" labels; whether v46 had late reports of the same size (not measured); the 31 Oct UTC day (not yet written to S3 at 23:34Z 30 Oct).

## Grade

PCKL1 FINDING (one sleep watchdog on v47, bc28, nothing lost; same signature as the known item 14 stall; WATCH the connect-time shift, which began on v46). Court1, Court2, Court3, PCKL2, PCKL3, MAFC-1, MAFC-2, Trail02, Dev-09 and Dev-14 PASS.

**v47 healthy on the fleet: all 11 devices on v47 with no missing hourly, no reset 140, no alert 15 or 41, no rollback trigger; the only item is PCKL1's single breadcrumb-28 watchdog (WATCH, one more on PCKL1 would make two).**

## Controller check (Claude Code, 31 Oct)

Spot-checked against S3: PCKL1's `watchdog` event at `2026-10-29T00:00:40.479Z` (fw 47, `bc` 28, stage `sleep`, `osReason` 60, `elapsed` 51 ms, `queue` 0, `connAge` 689218) is the **only** watchdog event in the fleet for 28-30 Oct (`particle-events/<day>/watchdog/`; PCKL1 on 29 Oct only), which supports "no more than one breadcrumb-28 fleet-wide". Not re-derived: the per-device tables and connect-time figures (taken from the grader's S3 pulls).

## Disposition (architect, 31 Oct)

Accepted: v47 healthy, pilot gate 1 passes. PCKL1 breadcrumb-28: **WATCH** (same escalation as PCKL2). PCKL1 connect time and the late reports: watch only. "Fault" key1 on Court1 and Court3: added to the Trail02 charge-fault-latch recovery item.
