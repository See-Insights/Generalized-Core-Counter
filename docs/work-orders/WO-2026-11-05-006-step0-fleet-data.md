# Item 21 Step 0: fleet correctionSec data (read-only pull)

Model: Claude Sonnet 5.5 (claude-sonnet-5-5). Date: 2026-11-05. Window: 22 Oct to 5 Nov 2026 UTC (15 archive days).
AWS: profile particle-admin worked (no token error). Nothing in any repo was modified or committed.

## 1. Where correctionSec lives (observation)
- Firmware: src/time/Clock.cpp:407 logs `ClockResync: sync advanced, rtcUpdated=%d epoch=%ld correctionSec=%ld`. src/cloud/DeviceStatusPublisher.cpp:304 also writes `clock.correctionSec` (with `clock.trusted`, `syncAgeSec`, `lastSyncEpoch`) into the status payload.
- Archive: in the 14-day window NONE of the 162 `status` event files (13 devices; versions v44 to v49.1) contain `correctionSec`, `syncAgeSec` or a `clock` object. They are the older flat status payload (version, resetReason, alert, freeHeap, watchdog counters, failsafe fields). Other event dirs: Ubidots-Sensor-Hook-v1, hibernate_wake, watchdog (not searched for the field; no reason to carry it).
- The only source is therefore the `serialLog` event: key `particle-events/<day>/serialLog/<device id>/<ts>.json`, JSON field `particle.logLine`, time field `particle.timestamp` (collector time, UTC). Only two devices have serialLog: Dev-09 (e00fce68399ee6244a963935) and Dev-14 (e00fce688e592afaf23ac4fb), both Singapore bench units forwarded by serial-forwarder-pi-01. The other 11 devices (including ChurchSt-2 e00fce686fd4bda70a18c449 and LAB-01 e00fce683d329595679c374e) have status files only, so NO correctionSec data exists for them. Coverage: 2 of 13 devices, 15 of 15 days each.
- Serial logs contain no `Boot:` line (0 hits on Dev-09/14), so "first sync after a reboot" was inferred: the 10-digit uptime-ms prefix of the log line dropped (>1 s backwards) since the previous line, or the line `ClockResync: requesting sync (no confirmed sync yet this boot)` preceded it. A reboot during a serial gap would be missed (a "later" label can hide a first sync).

## 2. Per-device results, Dev-09 and Dev-14 (correctionSec is signed; thresholds below use absolute value)
| Device | ClockResync records | median | p95 | min / max (signed) | >5 s | >60 s | >600 s |
|---|---|---|---|---|---|---|---|
| Dev-09 | 27 | 0 | 0 | -5 / 1 | 0 (one at -5, not over) | 0 | 0 |
| Dev-14 | 115 | 0 | 1 | -11 / 10 | 4 | 0 | 0 |
| 11 other devices | no data | | | | | | |
(p95 is signed linear interpolation. Dev-14 histogram: 0 x59, 1 x25, -1 x12, -2 x5, -4 x5, 2 x2, then single 4, -10, 10, -11, 6, -3. Dev-09: 0 x19, -1 x5, -5, -2, 1.)
Inferred first-sync-after-boot count: Dev-14 17 (max |c| 4), Dev-09 16 (max |c| 0). Later syncs: Dev-14 98 (max |c| 11), Dev-09 11 (max |c| 5).

Every value over 5 s (all four are Dev-14, all "later" sync, none first-after-boot; the uptime prefix is ms since boot):
| UTC time | correctionSec | class | uptime at sync |
|---|---|---|---|
| 2026-10-28T04:53:35Z | -10 | later | 22,298,151 ms (6.2 h) |
| 2026-10-28T05:12:53Z | +10 | later | 23,457,516 ms (6.5 h) |
| 2026-10-29T06:57:42Z | -11 | later | 32,234,071 ms (9.0 h) |
| 2026-10-31T09:17:52Z | +6 | later | 40,620,489 ms (11.3 h) |
The first two are 19 minutes apart and equal and opposite (-10 then +10); no value exceeds 11 s in the window.

## 3. Dev-11 (e00fce683f6063bf254283dd), searched by id from the docs
- No data in the 14-day window (no status or serialLog keys for 22 Oct to 5 Nov); the retired Dev-11 archive ends 2026-09-29 (serialLog) / 2026-09-28 (status).
- I therefore pulled its serialLog 25 Aug to 29 Sep (scratch: /private/tmp/gcc-step0/i21-out/d11/serial; the download of the last days was still finishing, so a few late files may be missing; 13 correctionSec lines found). `correctionSec=` first appears 2026-09-21 (older builds log `rtcUpdated=1 epoch=N` without it). All 13 values, in time order:
  - 2026-09-21T09:17Z 0; 12:14Z 0; 15:03Z 0; 09-22T10:11Z 0; 09-23T02:51Z 0
  - 2026-09-23T12:09:51Z **596**, uptime 10.9 s: first sync after a boot (preceded by the "no confirmed sync yet this boot" line at 12:09:48Z)
  - 2026-09-24T00:20Z 0; 2026-09-26T03:01Z -3 (uptime 4.1 h)
  - 2026-09-26T12:31:52Z **1372**, uptime 13.6 h: later sync
  - 2026-09-27T11:32:58Z **7570**, uptime 12.6 h: classifier says first-after-boot (an uptime drop earlier in the stream), but the uptime says 12.6 h since boot, so it is ambiguous (a late first sync, or a later sync)
  - 2026-09-28T01:09:11Z **8557**, uptime 5.5 s: first sync after a boot; then 01:10:12Z 0 (the next sync, 61 s later, corrected nothing); 2026-09-29T08:58Z 0 (uptime 1 h).
- Summary: 9 of 13 are 0, one -3, four are over 5 s and all four are over 500 s (596, 1372, 7570, 8557). Two of the four occurred right at boot sync, one at 13.6 h of uptime.

## 4. Inference (not choosing a threshold)
- Healthy RTCs: across 142 syncs on two healthy bench units the distribution is tight: 95th percentile 0 to 1 s, maximum 11 s, nothing near 60 s. A threshold anywhere from about 15 s up to the Dev-11 values would have produced zero false withdrawals in this window; 5 s would have withdrawn trust 4 times in 15 days on Dev-14 (all unrelated to reboots, all at 6 to 11 h uptime, two of them cancelling each other 19 min apart), so a small threshold risks distrusting a good clock.
- Faulty RTC (Dev-11): 596, 1372, 7570 and 8557 s are all above 600 except 596 (just under 600). A 60 s threshold would have caught all four; 600 s would have missed the 596 (and with it the first visible sign of the fault on 23 Sep), caught the other three.
- The gap between the largest healthy value (11 s) and the smallest faulty value (596 s) is about 50x; any threshold in that gap separates the two groups on this evidence.
- Caveats: only 2 healthy devices (both Singapore bench, good connectivity) are covered; the US field fleet (11 devices) has no correctionSec in the archive, so its distribution is unknown (the clock object is not in the archived status payloads). Only one faulty device (Dev-11), and its serialLog coverage of correctionSec starts 21 Sep. Boot classification is inferred from uptime resets, not a Boot line. Evidence of drift exposure for a trusted-across-restart clock (long unsynced spans) is not in this data because every record is a sync that already happened.
