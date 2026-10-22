# v45 bench soak: result (Dev-09 and Dev-14, graded 22 Oct 2026 about 23:00Z)

**WO:** `2026-10-20-v45-release-and-soak.md` §2 (it lives on `release/v45`, PR #135, held for this soak; this file is its Result for the architect and Chip). **Grader:** Sonnet 5.5 subagent (`claude-sonnet-5-5`, medium), read-only; Claude Code spot-checked the headline numbers in S3. **Verdict: both PASS; nothing holds the rollout. Setting v45 as the product default and merging #135 are Chip's steps (§3 of the WO).** Windows: 24 h from each device's first v45 boot; baseline each device's own v44 days (17 Oct to the OTA).

## First v45 boot (serial vs S3 status)

| Device | Serial (OTA, uptime restart) | S3 `status` (v45-PowerSplit, reset 70) | Window |
|---|---|---|---|
| Dev-09 | 21 Oct 22:01:45Z (first post-OTA line 22:01:47.9Z at uptime 2.8 s; "Update time" 22:01:37Z) | 22:02:07.503Z | to 22 Oct 22:01:45Z |
| Dev-14 | about 21 Oct 22:01:04Z (first line 22:01:13.7Z at uptime 9.9 s) | 22:01:21.654Z | to 22 Oct 22:01:04Z |

The serial has no version banner, so the boot is identified by the uptime restart after the OTA finishes and confirmed by the v45 status (forwarder lag 8–16 s). Both OTAs landed on the nightly 22:00Z wake.

## Checks (WO §2)

| Check | Dev-09 | Dev-14 |
|---|---|---|
| 1 payload unchanged | PASS: same field order and keys as v44 (S3 hook; 155 v44 / 43 v45 reports) | PASS (210 v44 / 64 v45) |
| 1 missing reports | 0 of 16 hourly slots (v44 0 of 73) | 0 of 32 half-hour slots (v44 0 of 142) |
| 1 PIR and timer wakes | 16 timer + 27 PIR, 17 with occupancy, daily max 201 | 32 timer + 32 PIR, 18 with occupancy, daily max 140 |
| 1 daily reset | `dailyoccupancy` 201 → 0 at the 22:00:53Z report | 140 → 0 at the 22:00:35Z report |
| 1 standard | resets only 30 and the one 70; `watchdogResetCount` 46 before and after; watchdog folders empty; alerts 0 in all v45 reports (v44: alert 44 and 31 once each; GateFail 0); `fh` 80368–80840 (v44 median 80584) | resets only 30 and the one 70; `watchdogResetCount` 126 before and after; watchdog folders empty; alerts 0 (v44: 44, 31 ×3, 40, 44); `fh` 80560–80800 (v44 median 80848) |
| 2 P-0 one-time reset | PASS: first v45 status `appBreadcrumb 0`, `alert 0`, no boot-storm alert or sleep; second boot (22 Oct 22:00:51.840Z, reset 30) `appBreadcrumb 28`, hibernate count 13 → 14, values carried over | PASS: first status `appBreadcrumb 0`, `alert 0`; second boot (22:00:33.774Z) `appBreadcrumb 28`, hibernate count 7 → 8 |
| 3 `connecttime`, teardown, alert 15 | median 4 s (v44 4), p90 6 (v44 20), max 159 (v44 660); `slow_teardown` 0, alert 15 0 | median 2 s (v44 2), p90 38 (v44 55), max 254 (v44 660); `slow_teardown` 0, alert 15 0 |
| 3 `MODEM_HEALTH`/`MODEM_POLICY` wording | identical: all 8 format strings are byte-identical in `src/` between `release/v44` and `release/v45`; no line fired in v45 | identical in source; no `MODEM_*` line fired in v45 (v44 had one `MODEM_HEALTH unstable`) |

## Check 4: counts only (serial, a lower bound; v45 window; v44 window was the same, all 0)

STOP fallbacks (both) **0**, hibernate bypass **0**, nightly heap guard **0**, all-modes-failed reset **0** on both devices. The status and `hibernate_wake` data are the stronger evidence: no reset reason other than 30 and 70, `hibernate_wake` result ok every night. (The serial forwarder dropped lines: 96 and 79 `SERIAL_DISCONNECTED` in the v45 windows.)

## Claude Code's spot-checks (S3 `status`, `watchdog`, `Ubidots-Sensor-Hook-v1`, 21–22 Oct)

- First v45 status: Dev-09 `22:02:07.503Z` reset 70, `appBreadcrumb 0`, `watchdogResetCount 46`; Dev-14 `22:01:21.654Z` reset 70, `appBreadcrumb 0`, `watchdogResetCount 126` (the v44 statuses at 22:02:04.403Z and 22:01:18.582Z are the pre-OTA wakes). Second boots carry `appBreadcrumb 28` and the same `watchdogResetCount`.
- `watchdog` files on 21–22 Oct: **0** for both. v45 hooks (to 23:10Z): Dev-09 48 with every `alerts` 0, connect median 4 s, max 159 s; Dev-14 68 with every `alerts` 0, median 2 s, max 254 s (slightly more than the 24 h counts above because the window was extended).

## Open items for the architect (neither a finding)

1. **ThrashGuard diagnostic counters not read:** the Particle variables `thrashTrips`/`thrashResets` timed out on both devices, and they are not in any S3 event; the "reads wrong once on the first boot" expectation (P-0) is unverified. The serial has no `THRASH` trip line in either v45 window.
2. **`MODEM_*` wording is identical by source only:** no such line fired on v45.
Bench caveat: both devices are BRN404X on Singtel; the connect-time figures say nothing about the US fleet.

## Default

Both pass, so per WO §3: set v45 as the product default (recheck the size and sha256), merge #135, then the fleet health check about 24 h later; Dev-09 and Dev-14 stay locked for v46. Those are Chip's steps; nothing was done here.
