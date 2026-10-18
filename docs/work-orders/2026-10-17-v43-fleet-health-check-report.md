# v43 fleet health check: report

Run: Sonnet 5.5 subagent (medium), read-only, 2026-10-18 02:25Z

## Sources
- S3 raw archive, `particle-events/` days 14-18 Oct (to about 02:03Z on 18 Oct): `status`, `watchdog`, `hibernate_wake`, `Ubidots-Sensor-Hook-v1` (reports). Counts below are by payload `timestamp`.
- Particle API (`particle product device list 42131`, read-only): all 11 devices online at 02:1xZ. Product default: not read.
- Device-status/device-settings ledgers: no read-only route found (no CLI ledger command), so tier, socTrust, connectionMode, sensorMode are inferred from cadence/battery only, and failsafe from `status` events.

## Window
- Per device from first v43 boot (reset 70, S3 `status`, version v43-SensorShape) to 18 Oct 02:03Z (about 37 h for 8 US devices, about 36 h Trail02).
- Baseline: same device's 48 h on v42 before its OTA (S3 reports).
- Not in the check: Dev-09 and Dev-14 (OTA'd to v44-DeviceOS660 on 17 Oct 09:00-09:03Z, status events; v44 soak).
- All 9 fleet devices are on v43; none pending.

## Per device
How to read it: OTA = S3 `status` reset 70 (Z, 16 Oct). Resets = S3 `status`/`watchdog` since OTA. WD = `watchdogResetCount` OTA-boot -> latest status (no `watchdog` event on any device after its OTA). Reports = v43 report count / missing hourly slots in open hours (a slot's report may be late; none lost). fh = report `fh` range (B). Connect = report `connecttime` median and share >=10 s, v42 48 h -> v43.

| Device | OTA | Resets | WD | Reports (missing) | fh | Alerts (reports) | Battery % | Connect v42 -> v43 | Grade |
|---|---|---|---|---|---|---|---|---|---|
| Court1 | 13:00:59 | none | 20 -> 20 | 67 (0) | 77224-81624 | **21 x7** (17 Oct 20:13-22:08Z) | 82.5-85.6 | 1 s 10% -> 1 s 18% | **FINDING** |
| Court2 | 13:01:02 | none | 2 -> 2 | 65 (0) | 76504-81600 | **21 x2** (17 Oct 20:00-20:03Z) | 83.2-85.8 | 1 s 3% -> 1 s 5% | **FINDING** |
| Court3 | 13:02:35 | none | 77 -> 77 | 97 (0) | 76064-81656 | none | 83.9-89.6 | 10 s 61% -> 10 s 59% | PASS |
| PCKL1 | 12:29:06 | none | 28 -> 28 | 88 (0) | 76440-81560 | none | 84.3-86.6 | 12 s 73% -> 1 s 5% | PASS |
| PCKL2 | 12:35:13 | none | 10 -> 10 | 91 (0) | 76488-81608 | none | 83.7-87.4 | 1 s 6% -> 1 s 3% | PASS |
| PCKL3 | 13:21:07 | **reason 0 x2**: 16 Oct 23:43:08Z, 17 Oct 18:26:31Z | 6 -> 6 | 97 (0) | 76496-89824 | none | 83.2-86.3 | 1 s 3% -> 1 s 6% | **FINDING** |
| MAFC1 | 13:00:42 | none | 82 -> 82 | 44 (0) | 76624-81552 | none | 85.2-86.5 | 1 s 6% -> 1 s 9% | **FINDING** (data) |
| MAFC2 | 13:00:49 | none | 35 -> 35 | 43 (0) | 77128-81600 | none | 83.0-85.1 | 1 s 5% -> 1 s 14% | **FINDING** (data) |
| Trail02 | 14:01:01 | reason 30 x1 (17 Oct 10:01:10Z, expected) | 37 -> 37 | 43 (0) | 76656-81792 | none | 64.7-78.3 | 14 s 92% -> 14 s 95% | PASS |

Notes by device (evidence):
- **Court1.** Alert 21 (PMIC charge timeout/stuck, `docs/reference/alert-codes.md` row 21; cause site not distinguishable from reports) in 7 reports 17 Oct 20:13:39Z-22:08:02Z, absent from the 22:56:50Z report onward. Not on the accepted list. Also 17 Oct 11-20Z: reports arrived as 2-hourly pairs (e.g. ts 11:00 and 12:00 published 12:00:36-37Z, cold connect 10-13 s), all slots present, so late-delivery only; the tier/mode cannot be read (CONSERVING-like cadence, battery 84-85%, so the known downgrade gap may be visible again; not determinable without the ledger). 16 Oct and 18 Oct after 00Z hourly.
- **Court2.** Alert 21 in 2 reports (17 Oct 20:00:06Z and 20:03:49Z); clear by 20:53Z. One 111 s connect at 22:00Z 17 Oct (report was on time).
- **Court3.** Cold connects (10-18 s) on 59% of v43 reports, identical to v42 (61%); not a v43 change. 150-190 s publish lag on 16 Oct 22Z-17 Oct 10Z, then about 20 s (queued, no loss).
- **PCKL1.** Warm again: 1 s median on v43 versus 12 s on v42 (the v42 report's re-tripped latch). Longest v43 connect: 18 s (17 Oct 20:00Z).
- **PCKL2.** Its two v42 breadcrumb-28 watchdogs (15 Oct 12:02Z, 23:00Z; count 8 -> 10) are before the v43 OTA; none on v43.
- **PCKL3.** Two reason-0 status events on v43, both inside open hours (19:43 and 14:26 EDT), with `resetReasonData` 0, no watchdog event, `lastWatchdogBreadcrumb` unchanged at 28 (old). Pre-OTA: reason-0 restarts at 14 Oct 00:01/01:31/02:04Z, 15 Oct 01:01Z, 16 Oct 13:21Z (v42 report's pre-existing pattern), so rate is similar, but these two are not at the 02Z close.
- **MAFC1 and MAFC2.** Zero `occupancy` and `dailyoccupancy` for the whole operating day of 17 Oct (10Z-01:59:59Z; 17 reports each, all hourly slots present, battery/fh normal). Their 14-16 Oct days were 300/268/268 and 217/319/257. Other sites had counts (Court1 86, PCKL1 277) the same day. Cause unknown from the data (closure or weather versus a sensor or counting fault); not attributable to v43 by any evidence read. 18 Oct reports stop at 02Z, so 18 Oct is not yet judgeable.
- **Trail02.** Hibernate wake events (`hibernate_wake`) show `result fail, gateArm wake_reason, wakeReason UNKNOWN` on every morning, 14-17 Oct, the same on v42 and v43: unchanged. Battery rose 50.8 -> 78.3% (48 h v42 -> v43) and its cadence moved from 2 h to hourly in the afternoon of 17 Oct (about 17Z), consistent with a tier change to HEALTHY (inferred).

## Checks across the fleet
- **Check 2/3, watchdogs:** no `watchdog` event from any fleet device on 16-18 Oct (S3 `watchdog`), so no breadcrumb-28 watchdog and no reset 140 data 2; every WD count unchanged. Only reset 70 and PCKL3 reason 0 and Trail02 reason 30 exist in S3 `status` after the OTAs.
- **Check 4, reports:** every device has every hourly slot (06-22 local, Trail02 06-23 local; the 02Z close slot is stamped 01:59:59Z). Extra reports are count-driven wakes. Delayed deliveries: MAFC2 ts 13:00 and 14:00 published together 14:01:40Z (3698 s lag, connecttime 134 s), MAFC1 15:00 (331 s lag). Not failures.
- **Check 5, heap:** per-day `fh` minimum is flat on all devices (about 76-77.3 KB on 16, 17, 18 Oct; `lfb` min 70.4-77.2 KB). No steady fall. The 89.8 KB maxima (PCKL3) are just after boots.
- **Check 6, alerts:** only alert 21 (Court1, Court2) in reports. No 31, 40, 41 or 44 in any report, and none at the 17 Oct open. All v43 OTA-boot statuses show `alert 0`.
- **Check 7, battery/modes:** tier, socTrust, connectionMode, sensorMode not read (ledgers). All batteries 83-90% (Trail02 65-78%), `key1` is Not Charging. Court1's 2-hourly pairing 17 Oct is the only cadence irregularity (see above).
- **Check 8, failsafe:** `failsafeStage` 0 and `failsafeCount` 0 on every latest S3 `status` (OTA boots, PCKL3 17 Oct 18:26Z, Trail02 17 Oct 10:01Z).
- **Check 9, payload:** all 601 v43 reports (the 9 fleet devices) have the same key order as v42 (occupancy, dailyoccupancy, battery, vc, key1, temp, alerts, resets, connecttime, fh, lfb, cyc, slp, timestamp) and the same number-format shape as the 933 v42 reports (digit-masked comparison identical).
- **cyc - slp = 1** on every v43 report of every device.
- **Daily close:** every US device has a report stamped 01:59:59Z on 17 and 18 Oct (close - 1 s, e.g. Court1 245 and 86, PCKL1 518 and 277); the next 17 Oct 10Z report is daily 0 for all nine (Trail02: 17 Oct 10:01:07Z 0, its close report 17 Oct 02:59:59Z). No report was seen between close and open, so a carried dailyoccupancy (item 10) did not occur. The 18 Oct 10Z report is not yet available.
- **Connect time v43 vs v42:** unchanged on Court2, Court3, PCKL2, PCKL3, MAFC1/2, Trail02 (table); PCKL1 improved (12 s -> 1 s median).

## Grade
- PASS: Court3, PCKL1, PCKL2, Trail02.
- FINDING: Court1 and Court2 (alert 21 on 17 Oct afternoon), PCKL3 (two reason-0 restarts), MAFC1 and MAFC2 (zero counts on all of 17 Oct, cause unknown).

v43 is healthy as firmware (no watchdog, no failsafe, no payload change, no heap decline, no lost report). Needs a look: alert 21 on Court1/Court2, PCKL3's reason-0 restarts in open hours, and whether MAFC-1/2 had a closure or a counting fault on 17 Oct.

Not read: keepalive environment variables (`particle config env`); product default firmware; device-status/device-settings ledgers (tier, socTrust, connectionMode, sensorMode, failsafe live values); serial logs (US devices have none); S3 data before 14 Oct.
Model: Sonnet 5.5 (claude-sonnet-5-5), medium reasoning.

Controller's checks (Claude Code, 2026-10-18, S3 `Ubidots-Sensor-Hook-v1`, 17 Oct UTC day): alert 21 confirmed in 7 Court1 reports (20:13–22:07Z) and 2 Court2 reports (20:00, 20:03Z); MAFC-1 and MAFC-2 end the day at `dailyoccupancy` 0 (earlier UTC-day maxima 268 and 257 belong to 16 Oct local). Power code between v42 (`ff1715c`) and v43 (`71e346f`) changed only by the 2a dead-code removal (`git diff --stat ff1715c origin/main -- src/power src/ThrashGuard.cpp`: 1 insertion, 99 deletions), so no v43 link to alert 21 was found; alert 21 has existed since v3.20 (`docs/reference/alert-codes.md` row 21).
