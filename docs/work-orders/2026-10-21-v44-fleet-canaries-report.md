# v44 fleet canaries: report (Court2 and PCKL1)

**Graded:** 21 Oct 2026, Court2 at about 11:55Z and PCKL1 with the final S3 refresh at 12:14:44Z, per `docs/work-orders/2026-10-16-v44-deviceos-6.6.0-release-and-soak.md` §3 (that WO lives on `release/v44`, PR #108, unmerged; as for the bench soak (PRs #120, #121) the result is a separate file on `main`, and the WO's Result section is filled when #108 merges).
**Graders:** Sonnet 5.5 subagent (`claude-sonnet-5-5`, medium), read-only, per the WO; Claude Code spot-checked the headline numbers in S3 and the Particle API (below). **Windows** (48 h from each device's first v44 boot, reset 70, by S3 `published_at`): Court2 19 Oct 11:25:37Z to 21 Oct 11:25:37Z; PCKL1 19 Oct 12:04:34Z to 21 Oct 12:04:34Z. **Baselines:** each device's own v43 days (first v43 boots 16 Oct: Court2 13:01:02Z, PCKL1 12:29:06Z).
**Rules applied:** the v42 health-check checks and §2 checks 2-6; canary rule: a breadcrumb-28 rate per 1,000 sleep returns above the device's own v43 rate is a FINDING, and so is any reset type not seen on v43; alert 40: one is reported, two or more on a device are a FINDING. US devices have no serial, so `slow_teardown` trips and `ConnSummary` `cellMs`/`cloudMs` are not determinable (alert 15 and warm-to-cold steps in `connecttime` stand in).

## Verdicts

| Device | Grade | Basis |
|---|---|---|
| ToM-MCP-Court2 | **PASS** | no resets, no watchdogs, flat heap, no alert 40/41/15; the one open item (the 11:00Z report) arrived 1 h late, was never lost |
| ToM-MCP-PCKL1 | **PASS** | breadcrumb-28 rate 0 per 1,000 sleeps on v44 against 0.20 on v43; no new reset type; one alert 40 (reported, not a finding) |

## Claude Code's spot-checks (S3 `particle-events/<day>/<event>/<id>/`, Particle API, 21 Oct 11:56-12:17Z)

- **Court2 11:00Z report:** at 11:56Z the other five US devices (PCKL1, Court1, Court3, MAFC-1, MAFC-2) all had their 11:00-11:03Z reports in the hook folder; Court2's latest was `2026-10-21T10-02-25-820Z` (API `online=false`, `last_heard 10:02:52Z`): a Court2-only gap. At 12:17Z the folder holds `...T12-00-52-857Z` and `...T12-00-54-217Z` (the 11:00Z and 12:00Z slots, 1.4 s apart): **delayed, not lost.** No Court2 `watchdog` event in S3 on 16-21 Oct.
- **PCKL1 watchdogs:** S3 `watchdog/e00fce6865fe7770b80a014e/`: 14 Oct `23-00-48-491Z` (v42), 18 Oct `13-14-01-772Z` (v43), **none on 19-21 Oct (the v44 window).**
- **PCKL1 alert 40:** hook reports 16-21 Oct (220 files): exactly one with `alerts` 40, published `2026-10-20T13-56-53-398Z`; none in the v43 window.
- **Release PR #108 binary (§13 (j)):** see the last section.

## Court2: ToM-MCP-Court2 (`e00fce686c3f94ad871d6479`), first grading (about 11:55Z)

Court2 (e00fce686c3f94ad871d6479) v44-DeviceOS660 canary grading. Model: Sonnet 5.5 (claude-sonnet-5-5). Read-only. Run at 2026-10-21 ~11:55Z.
Windows (by published_at; hook `timestamp` used for gaps/slots): v44 = 19 Oct 11:25:37Z to 21 Oct 11:25:37Z. v43 baseline = 16 Oct 13:01:02Z to 19 Oct 11:25:37Z (~70 h, all used; "v43-48h" = last 48 h of it). Sources: S3 Ubidots-Sensor-Hook-v1 (v43 131 reports, v44 85), status, watchdog, hibernate_wake; Particle API device record + device-status ledger.
A1 PASS. status 19 Oct 11:25:37.431Z: version v44-DeviceOS660, resetReason 70, data 0, alert 0; API firmware_version 44, system 6.6.0, desired 44. v43 first boot: 16 Oct 13:01:02.305Z reset 70.
A2 PASS. Only status event in the v44 window is the OTA boot (reset 70). watchdog and hibernate_wake folders are empty for 13-21 Oct. Hook cyc never decreases in either window (no reboot); hook `resets` = 0 throughout. Ledger firmware.resetCount 0.
A3 PASS. watchdogResetCount 2 before (v43 boot) and 2 after (v44 boot); lastWatchdogBreadcrumb 18, lastWatchdogUptimeMs 64760677 unchanged (a pre-v42 event).
A4 FINDING (provisional, open). All 06:00-22:00 local hourly slots delivered in v44 up to 21 Oct 10:00Z (0 lost; v43: 0 lost). But the 21 Oct 11:00Z slot (inside the window) has NO report: last hook 2026-10-21T10:02:25.820Z; API last_heard 2026-10-21T10:02:52.261Z, online false at ~11:50Z; polled S3 until 11:55Z, nothing newer (~55 min late). Ledger (updated 10:02:24Z): reporting.nextReportEpoch 1792580400 (=11:00:00Z), windowOpen true, lastReportEpoch 10:00:14Z, connection.elapsedMs 98421 lastResult ok. A late report would downgrade this; a stuck device is a failure. Recheck.
A5 PASS. `fh` (hook) v44: min 75488 / median 80752 / max 85064; v43: 76504 / 77296 / 81616. Level is ~+3.4 KB higher on 6.6.0 (OS effect, not a decline). Daily first report is the high point (85064 on 20 Oct, 85056 on 21 Oct; v43 81.6k). Two duplicate same-cyc events dip (75880 at 19 Oct 20:55, 75488 at 19 Oct 21:54); next report recovers to 80760. Status freeHeap at boot 89664 (v44) vs 90232 (v43).
A6 PASS (v44 has none). Hook `alerts`: v44 85 x 0. v43: 129 x 0 and 2 x alert 21 (17 Oct 20:00Z, 20:03Z, not in v42 list; v43 only). Alert 40: 0 in v44 and 0 in v43 (hook + status `alert`; v44 boot status alert 0). Alert 41: none. Alert 15: none either window.
A7 PARTIAL. Ledger device-status 21 Oct 10:02:24Z: battery tier HEALTHY, socTrust Trusted, soc 89.3, vcell 4.04, chargeState NOT, profile Solar, source BATTERY, clock trusted (syncAgeSec 79545). connectionMode and sensorMode: not in the ledger schema (not determinable). device-settings instance: not found.
A8 PASS. status failsafeStage 0, failsafeCount 0, failsafeLastAction 0 at v44 boot (same as v43 boot). No later status events.
A9 No change seen. Hook connecttime is one report behind (first report after the daily wake shows 1; the cold value appears in the next report). v43 cold values 38 (17 Oct), 38 (18 Oct), 15 (19 Oct), 43, 86, 111. v44: 11, 44 (20 Oct 11:00Z). Cadence unchanged (hourly + PIR-driven extras).
B2 PASS. Heap per wake: fh 80840 (cyc 2) to 80752-80760 (cyc 732), flat across 730 wakes; no steady fall (regression slope is positive only because of the daily high first report).
B3 (no pass/fail) connecttime s, min/median/p90/max: v44 1/1/1/44 (85 reports); v43-all 0/1/2/111 (131); v43-48h 0/1/6/111 (87). Reports with ct>=8: v44 2, v43-all 8, v43-48h 6. ConnSummary cellMs/cloudMs: serial only, not determinable. Ledger connection.elapsedMs 98421 for the 21 Oct 10:00 connect (single value, no v43 ledger comparison).
B4 Alert 15: 0 in both windows. Warm-to-cold steps (ct<5 then ct>=8): v44 1 (20 Oct 10:01Z 1 to 11:00Z 44); v43-all 6, v43-48h 5. slow_teardown trips: not determinable (no serial).
B5 Longest silence in open hours (consecutive reports, closed night excluded): by published_at v44 60.02 min (19 Oct 19:00Z to 20:00Z), v43 60.11 min (17 Oct 12:00Z to 13:00Z); by timestamp v44 60.02 min, v43 60.02 min. All gaps equal one hourly interval. Exception: the open 11:00Z gap above (now >110 min since the last report/heard).
C Canary rule. (a) bc-28 sleep watchdogs from S3 `watchdog`: v44 0 / (730 sleep returns/1000) = 0.00 per 1000; v43-all 0 / (1215/1000) = 0.00; v43-48h 0 / (1131/1000) = 0.00. Sleep returns = sum of positive hook cyc deltas (lower bound). v44 is not higher (equal, zero). (b) Reset list: v43 window status resetReason 70 x1 (16 Oct 13:01:02Z; earlier v42 OTA 14 Oct 10:01:46Z reset 70); v44 window reset 70 x1 (19 Oct 11:25:37Z); watchdog osReason events 0 and hibernate_wake 0 in all windows. No new reset type. Alert 40 rule: 0 in v44 (not a finding).
Inference: v44 behaviour matches v43 on every other check (no resets, no watchdogs, flat heap, same cadence); the only open item is the missing 11:00Z report, which can be explained by lateness or by a stall on the 07:00 local wake. Do not call it a loss until it is seen to never arrive; v43 had 0 such misses.
Files: raw downloads /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/3aa500de-487b-4335-ad8d-861927a20849/scratchpad/v44canary/court2/ ; script .../v44canary/scripts/parse.py
VERDICT: FINDING (provisional: 21 Oct 11:00Z hourly report missing at 11:55Z, last heard 10:02:52Z; all other checks PASS)
Model: Sonnet 5.5 (claude-sonnet-5-5)

### Court2 re-check (after 12:14:30Z)

Court2 re-check of A4 (read-only), 2026-10-21 12:14:44Z. Model: Sonnet 5.5.
Result: the 11:00Z report DID arrive, one hour late. A4 downgrades from FINDING to a delayed report (not a loss).
- 11:00Z slot: hook published_at 2026-10-21T12:00:52.857Z, timestamp 11:00:02Z (lag 3651 s); cyc 733, connecttime 98, fh 80744, alerts 0.
- 12:00Z slot: published_at 12:00:54.217Z, timestamp 12:00:01Z (lag 53 s); cyc 734, connecttime 98, fh 85048. Both arrived in the same connect, 1.4 s apart.
- Silence: last hook 10:02:25.820Z; API last_heard 10:02:52Z then; now online, last_heard 2026-10-21T12:01:43.778Z. So the cloud was silent 118 min (by published_at) while the device kept its clock (cyc 732 -> 733 -> 734, no cyc decrease).
- status / watchdog / hibernate_wake in 2026-10-21 for Court2: none. No reset; ledger startup epoch unchanged (1792409128, reason update), resetCount 0.
- Ledger device-status updated 12:00:57.494Z: connection.elapsedMs 17883 lastResult ok; tier HEALTHY, socTrust Trusted, soc 89.1, source BATTERY. (At 11:50Z the earlier value was elapsedMs 98421 for the 10:00 connect; hook connecttime 98 at 12:00 is the 11:00 attempt.)
- Inference: the 07:00 local wake could not register/connect for about an hour (a connection of ~98 s, which the hook connecttime shows as 98), the report was held and sent at the next wake. This is a delayed report, and the device was never stuck. It is a registration-silence datum for B5: longest open-hour silence by published_at is now 118 min (10:02:25Z to 12:00:52Z), against v43 60.1 min; by timestamp it is still 60 min. No such silence in v43 (0 missing slots, max 60.1 min).
- Grade effect: A4 passes (nothing lost). B5 should be reported as a long registration silence of 118 min on 21 Oct, outside v43's range; it is a single event, with no reset or watchdog.

**Court2 final grade: PASS.** The agent's first-pass A4 FINDING (the 11:00Z report missing for about 55 min) is withdrawn: the report was queued by the device and delivered at 12:00:52Z. **Observation for the record:** a 118-minute cloud silence by `published_at` (last hook 10:02:25Z, next 12:00:52Z) against v43's longest 60.1 min; by generation `timestamp` the longest gap is 60 min in both windows. Inference (the agent's): the 07:00 local wake could not connect for about an hour (a 98 s attempt) and sent its held report at the next wake; the device was never stuck. This is the registration-silence item (check 5) to watch, not a failure.

## PCKL1: ToM-MCP-PCKL1 (`e00fce6865fe7770b80a014e`)

PCKL1 (ToM-MCP-PCKL1, e00fce6865fe7770b80a014e, product 42131) v44-DeviceOS660 canary grading. Model: Sonnet 5.5 (claude-sonnet-5-5). Read-only. S3 refreshed 2026-10-21 12:14:44Z.
Windows (published_at; hook `timestamp` for slots/gaps): v44 = 19 Oct 12:04:34Z to 21 Oct 12:04:34Z. v43 = 16 Oct 12:29:06Z to 19 Oct 12:04:34Z (~71 h, all used; "v43-48h" = its last 48 h, from 17 Oct 12:04Z). Sources: S3 Ubidots-Sensor-Hook-v1 (v43 128 reports, v43-48h 61, v44 79), status, watchdog, hibernate_wake; Particle API device record and device-status ledger.
A1 PASS. status 19 Oct 12:04:34.456Z: v44-DeviceOS660, resetReason 70, data 0, alert 0. API: firmware 44, system 6.6.0, desired 44, online, last_heard 12:02:10Z.
A2 PASS (no resets in v44). Only status in the v44 window is the OTA boot (70). watchdog and hibernate_wake: 0 events in v44. Hook cyc never decreases in v44; `resets` 0; ledger resetCount 0.
A3 PASS. watchdogResetCount 29 at the v43 end (18 Oct 13:14:03Z) and 29 at the v44 boot; no later status, so 29 after. lastWatchdogBreadcrumb 28, uptime 175250753 (the v43 event).
A4 PASS. Hourly slots in open hours (06-22 local), by timestamp: v44 0 missing; v43 0 missing. Delayed only: v43 18 Oct 10:02 report to the next took 81.4 min by published_at (lag 1419 s); v44 max publish lag 137 s. Last v44 slot 12:00Z (published 12:02:06Z).
A5 PASS. `fh` min/median/max: v44 75240/80800/85080 (first 80800, last 80632); v43 76440/77264/89848 (89848 = first report after the 18 Oct watchdog boot). +3.5 KB level on 6.6.0, no decline (see B2). Status freeHeap at boot 89552 v44 vs 90216 v43.
A6 REPORT. Alert 40: exactly 1 in v44 (hook, 20 Oct 13:56:53.398Z published, ts 13:56:47Z: alerts 40, cyc 1586, occupancy 1; next report 14:00Z alerts 0). One alert 40 is reported, not a finding (2 or more would be). v44 hook alerts: 78 x 0, 1 x 40. v43: 126 x 0, alert 19 (18 Oct 13:14Z, post-watchdog), alert 44 (18 Oct 14:00Z, boot clock step; known). Alert 41: none. Alert 15: none in either window. v44 boot status alert 0 (so the v44 alert 40 is not in status).
A7 PARTIAL. Ledger device-status 21 Oct 12:02:04Z: tier HEALTHY, socTrust Trusted, soc 87.6, vcell 3.99, chargeState FAST, profile Solar, source VIN, connection ok. connectionMode/sensorMode not in the ledger (not determinable).
A8 PASS. failsafeStage 0, failsafeCount 0, failsafeLastAction 0 at the v44 boot (same as v43).
A9 No latch evidence change. Cold connects (hook connecttime >=8 s; shows in the report after the connect): v44 7 of 79 (values 8, 8, 10, 12, 15, 17, 17); v43 13 of 128 (9-22 s, plus 659 after the watchdog boot).
B2 PASS. Per-wake heap: fh 80800 (cyc 2) to 80632 (cyc ~3470 later), flat (v44 slope -0.5 B/report); no steady fall.
B3 (no pass/fail) connecttime s min/median/p90/max: v44 0/2/7/17; v43-all 0/1/9/659; v43-48h 0/2/15/659. ConnSummary cellMs/cloudMs: serial only, not determinable.
B4 Alert 15: 0 and 0. Warm-to-cold steps (ct<5 then >=8): v44 4, v43-all 6, v43-48h 5. slow_teardown trips: not determinable (no serial).
B5 Longest open-hour silence between consecutive reports: by published_at v44 60.38 min, v43 81.37 min (delayed 18 Oct 10:02Z report); by timestamp 60.02 min in both. Night gaps excluded.
C Canary rule. (a) bc-28 sleep watchdog events (S3 `watchdog`, stage sleep): v44 0 / (3470/1000) = 0.00 per 1000; v43-all 1 / (4922/1000) = 0.20 per 1000; v43-48h 1 / (4235/1000) = 0.24 per 1000. Sleep returns = sum of positive hook cyc deltas (lower bound; counts lost across the v43 watchdog reset). v44 rate is not higher.
  v43 event: 2026-10-18T13:14:01.772Z (published) {"reset":"watchdog","osReason":60,"bc":28,"stage":"sleep","elapsed":39,"queue":1,"state":3,"connAge":691068}; status 13:14:03.055Z resetReason 60, watchdogResetCount 28 -> 29, alert 19 (published 1.3 s apart, no queue lag). Context only, v42: 14 Oct 23:00:48.491Z osReason 60 bc 28 sleep elapsed 62, count 27 -> 28.
  (b) Reset types: v43 window: status 70 (16 Oct 12:29:06.832Z OTA), status 60 / watchdog osReason 60 (18 Oct 13:14); hibernate_wake 0. v44 window: status 70 (OTA 19 Oct 12:04:34Z) only. No reset type new in v44.
Inference: v44 behaviour matches v43 or better on watchdogs (0 vs 1); the one alert 40 is the known queued item (recovery plan item 7). Cold-connect frequency is not worse than v43.
Files: raw .../scratchpad/v44canary/pckl1/ ; scripts .../scratchpad/v44canary/scripts/parse2.py, slots2.py
VERDICT: PASS
Model: Sonnet 5.5 (claude-sonnet-5-5)

## Release PR #108 `.bin` against product v44 (both canaries pass; §13 (j))

- **PR body (`release/v44`, head `ed879987b6`):** `app.bin` **152726 B**, sha256 **`06cd51a982f36e31779c0df583c97787bd59634782b7b5ebd39cd1c4835715ed`**.
- **Product v44 (Particle API, product 42131, firmware 44, `v44-DeviceOS660-boron-app.bin`, uploaded 17 Oct 08:26:25Z), downloaded read-only:** 152726 B, sha256 `06cd51a982f36e31779c0df583c97787bd59634782b7b5ebd39cd1c4835715ed`. **Size and sha256 match the PR.** (The API's `app_hash` field is Particle's own hash, not the file sha256.)
- **Independent rebuild of the PR head** (`ed87998`, Device OS 6.6.0 toolchain, Boron): 152726 B, text/data/bss 151628/1094/2200, `strings` shows `v44-DeviceOS660`; its sha256 is `35ed712e3a440dfaa9082b26ebabd3cb10048ad81a48fec5b6abe46b1e1489e8`, which differs from the product binary in **56 bytes in three runs**: the 20-byte GNU build-ID note (path-dependent) and the 36 bytes of module SHA-256 and CRC that Particle derives from it. Every code and data byte is identical. Not a defect; a rebuild on another path is not byte-identical.
- **Current default:** product default is v43 (`Release Candidate - v43`, `product_default` true); v44 is "Development Candidate - v44", `product_default` false.

## Result for the WO's Result section (to be pasted when #108 merges)

**Fleet canaries (graded 21 Oct): Court2 PASS, PCKL1 PASS** (Sonnet 5.5 graders; this file). No breadcrumb-28 watchdog and no new reset type on either (PCKL1 0 against 0.20 per 1,000 sleeps on v43); heap flat on both (level about 3.4-3.5 KB higher on 6.6.0, as on the bench); one alert 40 on PCKL1 (reported; two or more is a finding), none on Court2; Court2's one delayed report (1 h late, delivered) noted under registration. #108's binary matches product v44 by size and sha256. **Chip can set v44 as the product default and merge #108** (§4), then run the health check about 24 h later.
