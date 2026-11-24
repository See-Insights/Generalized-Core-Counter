# Archive evidence for item 17 Step 0 (read by Claude Code from the S3 raw archive; UTC)

ChurchSt-1a (e00fce686f8cb2133cd3f8ed, Morrisville NC, EST5EDT since 1 Nov, open 06:00 local = 11:00Z, close 22:00 local = 03:00Z), fw 4901 (v49.1-DurableResets).
Archive holds this device's reports from 7 Nov only.

## The three stage-2 failsafes (status event, published right after the reset)
| Day | resetReason/data | alert | lastAlert (raised) | failsafeLastAction | failsafeCount | lastConnectionAgeSec |
| 10 Nov | 140/2 | 40 | 11:00:21Z | 14:00:01Z | 1 | -1 |
| 13 Nov | 140/2 | 40 | 11:00:32Z | 14:00:02Z | 1 | -1 |
| 16 Nov | 140/2 | 40 | 11:00:33Z | 14:00:02Z | 1 | -1 |
All three: appBreadcrumb 10, watchdogResetCount 0.

## Reports (Ubidots-Sensor-Hook-v1): stamp (device clock) -> published_at (cloud receipt)
10 Nov: stamp 11:00:21 alerts 0 conn 8   -> published 14:00:49
        stamp 12:00:02 alerts 40 conn 660 -> published 14:00:50
        stamp 13:00:02 alerts 40 conn 660 -> published 14:00:52
        stamp 14:00:07 alerts 40 conn 660 cyc 1 slp 0 resets 1 -> published 14:00:54   (post-reset boot report)
13 Nov: stamps 11:00:32 (conn 17), 12:00:02, 13:00:02 (conn 660), 14:00:07 (cyc 1) -> published 14:00:56 to 14:01:02
16 Nov: stamps 11:00:32 (conn 17), 12:00:02, 12:58:14 (occ 1), 13:09:23 (occ 1), 14:00:18 (cyc 1) -> published 14:01:01 to 14:01:08
No report was received between ~11:00Z and 14:00:49Z on any of the three days. connecttime 660 = the 660 s connect budget exhausted.
Normal days (8, 9, 11, 12, 14, 15, 17 Nov) show no delivery lag over 10 min and no alert 40. The three events are exactly 3 days apart (10, 13, 16).
Alert 40 is first raised 21-33 s after the 11:00Z open wake, i.e. at open (the "alert-40-at-open" item). The reports stamped 12:00 and 13:00 carry alerts=40.
The webhook responses (hook-response events) are NOT in the archive; only device-published events are.

## Fleet-wide stage-2 events (status resetReason 140 + data 2, or failsafeStage >= 2), 1-17 Nov
| Published | Device | fw | alert | failsafeLastAction | local time / hours since 06:00 open |
| 1 Nov 15:59Z | e00fce683d329595679c374e = OCCUPANCY-LAB-01 (product 42131, a lab unit; tz not known) | 47 | 0 | 15:30:04Z | tz unknown |
| 10 Nov 14:00Z | ChurchSt-1a | 4901 | 40 | 14:00:01Z | 09:00:01 EST = 3.0 h after open |
| 13 Nov 14:01Z | ChurchSt-1a | 4901 | 40 | 14:00:02Z | 09:00:02 EST = 3.0 h |
| 14 Nov 03:02Z | PCKL3 (e00fce6853367e34da7e905c) | 4901 | 0 | 03:02:24Z | 22:02:24 EST = 2 min after the 22:00 close (16 h after open) |
| 16 Nov 14:01Z | ChurchSt-1a | 4901 | 40 | 14:00:02Z | 09:00:02 EST = 3.0 h |
PCKL3 is therefore not a 3-h-after-open event; alert was 0.

## Court2 (e00fce686c3f94ad871d6479), 13 Nov 23:00:57Z, firmware v49.1
watchdog event: reset=watchdog osReason 60 bc 28 stage sleep elapsed 37 queue 0 state 3 (SLEEPING) connAge 625508 ms.
status event: rr 60/0 alert 19 bc 28 appBreadcrumbMs 644645716 (7.46 d uptime) watchdogResetCount 3.
