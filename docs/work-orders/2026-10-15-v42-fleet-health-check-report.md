# v42 fleet health check: report

**Run:** Claude Code, read-only, 2026-10-15 at about 02:03Z. A read-only subsession can't reach S3 or the Particle API, so this ran in the main session (Opus), not at the Sonnet tier.
- **Sources:** Particle API (device records; `device-status` and `device-settings` per instance; firmware list) read at 02:03Z; S3 raw events for 12–15 Oct (`status`, `watchdog`, `Ubidots-Sensor-Hook-v1`, `hibernate_wake`).
- **Window:** each device's first v42 boot (reset 70, 14 Oct 10:00:57–10:03:36Z) to 15 Oct 02:03Z, about 16 h. That covers one open period and one close.
- **v40 baseline:** the 48 h before each device's OTA. The US devices have no serial logs.

## Plain goal

Confirm every fleet device runs v42 (`v42-ConfigWindowLatch`) cleanly since the 14 Oct rollout, and find anything v42 made worse.

## Per device

How to read the columns:
- **Resets:** from S3 `status` and `watchdog` events.
- **Watchdog count:** pre-OTA (the OTA-boot status) → now.
- **Reports:** missing reports in open hours (06–22 EDT, Trail02 to 23).
- **fh:** the range on v42.
- **Connect time:** from report `connecttime`; median and share ≥ 10 s, v40 48 h → v42.

All devices run OCCUPANCY in KEEP_ALIVE (`default-settings`); `failsafeStage`/`failsafeCount` are 0/0 everywhere, and no report carried alert 41.

| Device | OTA (reset 70) | Resets since OTA | Watchdog count | Reports (missing) | fh (B) | Alerts | Tier / trust | Connect time v40 → v42 | Grade |
|---|---|---|---|---|---|---|---|---|---|
| Court1 | 10:01:43Z | none | 20 → 20 | 43 (0) | 75704–77448 | none | HEALTHY / Trusted | 11 s, 100 % → 1 s, 7 % | PASS |
| Court2 | 10:01:46Z | none | 2 → 2 | 56 (0) | 77264–77432 | none | HEALTHY / Trusted | 1 s, 3 % → 1 s, 0 % | PASS |
| Court3 | 10:03:36Z | none | 77 → 77 | 55 (0) | 76664–77336 | none | HEALTHY / Trusted | 11 s, 100 % → 1 s, 0 % | PASS |
| PCKL1 | 10:02:17Z | **watchdog, reason 60**, breadcrumb 28 (`stage: sleep`) | **27 → 28** | 48 (0) | 76232–89816 | 19, 44 after the reset | HEALTHY / Trusted | 1 s, 5 % → 2 s, 27 % | **FINDING** (pre-existing pattern) |
| PCKL2 | 10:02:31Z | none | 8 → 8 | 50 (0) | 75688–77448 | none | HEALTHY / Trusted | 1 s, 6 % → 1 s, 0 % | PASS |
| PCKL3 | 10:01:15Z | **reason 0**, at 01:01:04Z on 15 Oct (status) | 6 → 6 | 64 (0) | 76328–89816 | none | HEALTHY / Trusted | 1 s, 10 % → 1 s, 3 % | **FINDING** (pre-existing pattern) |
| MAFC-1 | 10:03:04Z | none | 82 → 82 | 40 (0) | 76608–77400 | none | HEALTHY / Trusted | 10 s, 56 % → 1 s, 10 % | PASS |
| MAFC-2 | 10:02:40Z | none | 35 → 35 | 26 (0) | 75720–77392 | none | HEALTHY / Trusted | 1 s, 3 % → 1 s, 4 % | PASS |
| Trail02 | 10:01:16Z | none since the OTA (its hibernate wake came just before the OTA) | 37 → 37 | 16 (0) | 77256–81600 | none | **CONSERVING** / Trusted (60.1 %, 3.95 V) | 9.5 s, 50 % → 12 s, 69 % | PASS (see note) |
| Dev-09 / Dev-14 | bench, on v42 | hibernate wakes (reason 30) | — | — | — | — | HEALTHY / Trusted | — | listed only |

- **Fleet-wide:**
  - every report has cyc − slp = 1;
  - no `GateFail` can be seen without serial;
  - the alert 40 on the OTA-boot statuses is the known open-hour case (queued item 7);
  - the heap shows no steady fall (the upper bound of 89816 is just after a boot).
- **On v42:** all 11 devices report `v42-ConfigWindowLatch`; product v42 has been the default since 14 Oct 06:45Z (firmware list).

## Findings (neither is attributable to v42)

1. **PCKL1: a breadcrumb-28 watchdog on v42.**
   - **The reset:** a `watchdog` event published at 23:00:48Z on 14 Oct: `osReason 60`, `bc 28`, `stage sleep`, `elapsed 62`, `connAge 276873`. The status at 23:00:49Z shows `watchdogResetCount 28`. `device-status` gives the boot as 22:30:13Z, so the status was queued until the next connection.
   - **Same pattern on v40, in the 48 h before the OTA:** PCKL2 at 13:03:00Z on 12 Oct (count 7 → 8) and MAFC-1 at 02:01:17Z on 13 Oct (81 → 82), both breadcrumb 28 in sleep.
   - **INF:** this is the known background rate of breadcrumb-28 watchdogs (about one per device per week, 9 Oct analysis), not a v42 regression. The release contains no code that runs between breadcrumb 28 and the return from `System.sleep()`.
2. **PCKL3: a reason-0 restart on v42.** One, at 01:01:04Z on 15 Oct. In the 48 h before, on v40, it had four (status events at 02:00Z on 13 Oct, and at 00:01Z, 01:31Z and 02:04Z on 14 Oct), all near the 02:00Z close. This is its known pattern, at an unchanged or lower rate.

## v42-specific checks (indirect)

- **The standby latch** (connect times are the only signal without serial):
  - **Court1, Court3 and MAFC-1:** connected cold (10 s or more) at 100 %, 100 % and 56 % of connections over their last 48 h on v40, then warm (about 1 s) **from straight after the OTA reboot**.
    - **INF:** they were latched on v40 and the OTA reboot (RAM reset) cleared it.
    - **The daily clear can't be determined yet:** none re-tripped and then crossed a close on v42 in the window.
  - **PCKL1:** warm on v40 and early on v42, then cold (12–21 s, and 211 s once) on every connection from 22:24Z on 14 Oct, around its watchdog reset. **INF:** a latch re-tripped after the reset. Its first connection after the 15 Oct open (about 10:00Z) is the first fleet test of the daily clear: warm means the clear worked.
  - **Trail02:** not determinable. It hibernates nightly, so every morning is a cold boot, and it reports on CONSERVING's 2 h cadence.
- **The config window:** no cadence change. The effective interval is 3600 s everywhere except Trail02, whose 7200 s is from CONSERVING. Every device has its 10Z (06:00 EDT) report. The only hold in the window is the OTA boot itself; a first-after-open hold on a non-boot morning isn't observable until 10:00Z on 15 Oct.
- **Trail02 note (battery and modes):** it is CONSERVING (Trusted 60.1 %, 3.95 V) in OCCUPANCY/KEEP_ALIVE with `lowBatteryMode` false. That is the known downgrade-check gap (queued item 5), seen before on Court1. Court1 is HEALTHY now, so it doesn't show the gap today.

## Grade

- **7 PASS:** Court1–3, PCKL2, MAFC-1/2, Trail02.
- **2 FINDING, both pre-existing patterns:** PCKL1 (a breadcrumb-28 watchdog) and PCKL3 (a reason-0 restart).

**v42 healthy on the fleet; no v42 regression.** Two follow-ups are worth a later look:
- PCKL1's first post-open connection on 15 Oct, the first test of the daily clear;
- whether Chip wants the breadcrumb-28 rate and PCKL3's close-time restarts tracked in the recovery plan.
