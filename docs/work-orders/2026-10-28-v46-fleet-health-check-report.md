# v46 fleet health check: report (graded 28 Oct 2026 about 02:05Z)

**WO / format:** `2026-10-22-v44-fleet-health-check.md` and its report, with v46 (`v46-SleepSplit`, Device OS 6.6.0) in place of v44; Chip's ruling of 26 Oct (job `9b8a67aa`). **Grader:** a Sonnet subagent (read-only; S3 `Ubidots-Sensor-Hook-v1`, `status`, `watchdog`, `hibernate_wake` for 23-28 Oct, hooks to 02:03:42Z 28 Oct, plus the Particle CLI). Claude Code spot-checked the headline numbers in S3 with its own pull (§13 (j)): the PCKL2 watchdog file and status, Trail02's four status events, and MAFC-2's first v46 boot. Nothing was changed in AWS, on a device, or in the product.

**Verdict: two FINDINGS by the letter, neither a loss of data. PCKL2: one breadcrumb-28 sleep watchdog on v46 (27 Oct 15:00:50Z). Trail02: two reset 40 (power down) on v46, new in this window. The other seven US devices PASS. v46 is not called healthy on the fleet until Chip and the architect have graded the two.** Chip decides what follows.

## Windows

- **Default time:** the Particle CLI (3.52.3) has no product-default or firmware-list query, so the exact time is **not determined**. The earliest v46 boot is MAFC-2 at 27 Oct 00:27:57Z (spot-checked), so the default was set at or before then; consistent with Chip's ~00:10Z. The US OTAs run 00:28Z to 02:01Z 27 Oct, so the window is **about 25 h** (the 24 h condition is met). Every US device was on v45 (`v45-PowerSplit`) before its OTA, from its 23 Oct boot, so the v45 baseline is the 48 h before each device's OTA.
- A 28 Oct 02:00Z report arrived on every US device by 02:04Z. The CLI lists all 11 devices online.

## Table (per US device)

| Device | First v46 boot (reset 70) | Resets since OTA | bc28 per 1,000 sleep returns, v46 vs v45 | `watchdogResetCount` before > after | Missing hourly | Item 18 gaps | Alerts (v46) | Verdict |
|---|---|---|---|---|---|---|---|---|
| Court1 | 27 Oct 00:31:59Z | none | 0 (0/820) vs 0 (0/1,717) | 20 > 20 | 0 of 19 | none | none | PASS |
| Court2 | 01:00:55Z | none | 0 (0/310) vs 0 (0/916) | 2 > 2 | 0 of 18 | none | none | PASS |
| Court3 | 00:59:35Z | none | 0 (0/3,083) vs 0 (0/6,455) | 77 > 77 | 0 of 19 | none | none | PASS |
| PCKL1 | 01:01:56Z | none | 0 (0/1,272) vs 0 (0/4,441) | 30 > 30 | 0 of 18 | none | none | PASS |
| **PCKL2** | 01:01:40Z | **reset 60 (watchdog), 15:00:50Z** | **0.62 (1/1,608)** vs 0 (0/4,058) | **11 > 12** | 0 of 18 | none | 19 once (15:00:53Z, the boot after the watchdog) | **FINDING** |
| PCKL3 | 01:01:23Z | reset 0 at 16:26:31Z and 20:42:44Z (known pattern) | 0 (0/439) vs 0.50 (1/2,016) | 8 > 8 | 0 of 18 | none | none | PASS, WATCH |
| MAFC-1 | 01:02:13Z | none | 0 (0/1,016) vs 0 (0/1,735) | 83 > 83 | 0 of 18 | none | none | PASS |
| MAFC-2 | 00:27:57Z | none | 0 (0/1,848) vs 0 (0/1,093) | 37 > 37 | 0 of 19 | none | none | PASS |
| **Trail02** | 02:00:53Z | **reset 40 (power down) at 19:26:40Z and 23:12:16Z**; reset 30 at 10:01:00Z (expected) | 0 (0/14) vs 0 (0/33) | 37 > 37 | 0 of 18 | none | none | **FINDING** |

Rules: sleep returns = the sum of positive hook `cyc` deltas per device; bc28 events come from the S3 `watchdog` folder (bc 28) per window; missing = hourly slots (10:00Z to 02:00Z, Trail02 to 03:00Z) with no report, a slot counts as late only after 1 h and none did; item 18 = hook `published_at` gaps of 4 h or more since 25 Oct: every one is the 7-8 h night gap ending in a normal report. **Fleet bc28 context:** v46 US 1 event in 10,410 returns (0.096 per 1,000); v45 US 1 in 22,464 (0.045); earlier background about 0.05-0.09.

## Findings

1. **PCKL2, a breadcrumb-28 sleep watchdog on v46** (spot-checked). `watchdog/` file 2026-10-27T15:00:50.4Z: osReason 60, bc 28, stage sleep, elapsed 43; status 15:00:51.9Z: reset 60, `lastWatchdogBreadcrumb` 28, `watchdogResetCount` 11 to 12, `appBreadcrumb` 28, failsafe 0/0, no 140. By the rule (higher than its own v45 rate of 0 in 4,058 returns) a FINDING. Context: one event in 1,608 returns; the fleet v46 rate is within the earlier background; **PCKL2 already read `lastWatchdogBreadcrumb` 28 at its first v46 boot (count 11), so it had an older breadcrumb-28 event before this window**. Nothing was lost: the 15:00Z report arrived at 15:00:53Z (`connecttime` 1 s); the following connects took 33 s (15:42Z) and 16 s (16:00Z), then normal. Whether the S-1 to S-6 split changed this rate is **not determined** (one event); the recovery item 14 watch applies.
2. **Trail02, reset 40 (power down) twice on v46** (spot-checked: statuses at 19:26:40.4Z and 23:12:16.0Z, both v46, reset 40 data 0, `appBreadcrumb` 0, `watchdogResetCount` 37 unchanged, failsafe 0/0). Its v45 days 23-26 Oct show only reset 30 and the 70, so reset 40 is new in this window (earlier than 23 Oct not checked). The hook `cyc` restarts at 2 after each; battery read 66.29% "Not Charging" before the first, 69.46% "Charging" after it, then 69.95% (23:00Z) to 89.75% (00:00Z) after the second (a gauge re-estimate, as the Trail02 discharge run found), 89.91% at 02:00Z. No report lost, no hook gap. **Cause not determined from S3** (no serial on US devices): a power loss or a deliberate power-down cannot be told apart. Its 10:01Z wake on 27 Oct is reset 30 labelled UNKNOWN in `hibernate_wake` (`result` fail, `gateArm` wake_reason), identical to its v45 wakes of 23-26 Oct. Hibernate is enabled. It took v46 at its first wake (02:00:53Z), so it was not left on an older version.

## Standing checks

- **Resets, others:** PCKL3's reset 0 events (bc 18 at 16:26Z, bc 15 at 20:42Z) match its v45 pattern (about 14 reset-0 statuses 23-27 Oct); not watchdogs, not failsafes, no lost report. MAFC-1, MAFC-2, Court1-3 and PCKL1: only the reset 70.
- **Failsafe:** `failsafeStage` and `failsafeCount` 0/0 in every US status since the OTA; no reset 140 on any US device.
- **Alerts:** none of 15, 40, 41, 44 on any US device in the v46 window; PCKL2's alert 19 above. v45 baseline: Court3 one alert 40 (25 Oct 10:03Z), PCKL3 one alert 19 at its 25 Oct 22:01Z watchdog.
- **Heap `fh`** (hook, v46 min / median / max vs v45): clusters at about 80.7 k and 85 k on every device, minima are post-boot dips, no steady fall (Court1 74,320 / 80,720 / 85,024 vs 74,920 / 85,040 / 85,064; PCKL2 80,608 / 80,904 / 89,112 vs 75,592 / 80,736 / 85,040; the rest in the same bands).
- **`connecttime`** (median / p90 / count at 8 s or more, v46 vs v45): Court1 1 / 14 / 9 of 45 vs 11 / 14 / 73 of 79; Court2 1 / 1 / 3 of 47 vs 1 / 1 / 2 of 75; Court3 10 / 14 / 39 of 48 vs 10 / 14 / 90 of 90; PCKL1 2 / 6 / 2 of 28 vs 2 / 7 / 4 of 71; PCKL2 1 / 16 / 5 of 32 vs 1 / 1 / 3 of 76 (the post-watchdog connects); PCKL3 1 / 6 / 7 of 72 vs 1 / 17 / 16 of 108; MAFC-1 2 / 7 / 3 of 30 vs 11 / 14 / 57 of 57; MAFC-2 1 / 6 / 3 of 32 vs 1 / 1 / 3 of 81; Trail02 14 / 18 / 12 of 18 vs 14 / 20 / 36 of 36. No regression; no outlier above 60 s in a US v46 window.
- **Battery** (first > last > min): Court1 88.96 > 87.06 > 86.92; Court2 86.92 > 86.78 > 85.77; Court3 89.91 > 89.91 > 84.62; PCKL1 88.64 > 88.50 > 87.49; PCKL2 87.93 > 88.07 > 86.63; PCKL3 87.06 > 88.80 > 86.20; MAFC-1 90.70 > 88.21 > 88.21; MAFC-2 87.64 > 84.91 > 84.91; Trail02 see finding 2.
- **Bench (completeness only):** Dev-09 v46 from 25 Oct 23:01:00Z, then `v47-PersistFreeze` reset 70 at 22:37:31Z 27 Oct (the lock); `watchdogResetCount` 46 unchanged; no bc28 on v46. Dev-14 v46 from 23 Oct 08:30:45Z, then v47 at 22:41:56Z; its one bc28 (24 Oct 12:01:43Z, count 126 to 127) is on the recovery item 14 watch; alerts 40 ×2, 44, 31s: the poor link.

## Not determined

The exact product-default time (no CLI query); Court1's CONSERVING state and whether it has a downgrade (tier, `socTrust`, `connectionMode` and `sensorMode` are ledger fields, not in S3; its battery is 87-89% with no alerts or resets); the cause of Trail02's two reset 40s; `slow_teardown` and `ConnSummary` (no serial on US devices); alert 15 beyond "none in the hooks".

**Action needed before v46 is called healthy on the fleet: grade PCKL2's breadcrumb-28 watchdog (a watch item, one event) and Trail02's two reset 40 power-downs.**
