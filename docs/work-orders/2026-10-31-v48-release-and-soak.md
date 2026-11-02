# v48 release and bench soak (occupancy ends at the close)

**Author:** Claude Code, 31 Oct 2026, from the v47 WO (`2026-10-27-v47-release-and-soak.md`) and WO-2026-10-29-002; the architect may amend · **Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §2, §3 ("Release gate: the Particle cloud compile"), §13 (g), (h), (i), (j)
**Agents:** release PR, Claude Code itself; soak and baseline runs, a **Sonnet subagent, medium**, read-only. Report the model used.
**Budget:** 0 `src/` lines beyond the version string.
**Start:** v47 is the product default (23:01Z 28 Oct) and its fleet health check passed (PR #167). #166 (item 10) is merged.

## Plain goal

Ship #166 as **v48-OccupancyEndsAtClose** on Device OS 6.6.0. The one behavior change: on a normal-hours site, a session open at the close is credited to the close and ended by it, and nothing detected between the close and the next opening is counted. **What the soak must show:** the first report each morning reads `dailyoccupancy` **0** with no carry-in (v47 carried +5 to +7 min into the next day on Dev-09 and Dev-14, 28 Oct), and the day's last session before 22:00 is still credited to the close.

v48 is also the gate for the main waves of the legacy-occupancy migration (WO-2026-10-29-001, PR #165): the pilot devices stay on v47 and the waves wait for the v48 pass.

## 1. Release PR (branch `release/v48`)

- `src/` differs from v47 (`f624c15`) only by #166 (`State_Common.h`, `State_Modes.cpp`, net +16) and the version string. Release commit: `FirmwareVersion.h` (48, `v48-OccupancyEndsAtClose`), `CHANGELOG.md`, `README.md`, `Doxyfile`.
- **Suite:** run with zsh for the `.sh` tests and python3 for the `.py` tests, serial. Result: 107/107 on the release head.
- **Local build (§3, mandatory per change), 31 Oct:** Boron, Device OS 6.6.0, fresh `BUILD_PATH_BASE`, `EXTRA_CFLAGS=-frandom-seed=$$@`, built from a `git archive` of the release head (`release/v48`, before this WO edit). `app.bin` **152,722 B** (v47: 152,610 B, +112), **sha256 `2aa44eb2f303e6c07b13e040283a97c77bcd9e3b8d85a559dc9a21035fd60da8`**, text/data/bss 151604/1114/2200 (v47: 151492/1114/2200). `.backup` section **152 B**, as v47; `src/` has no `retained` change since v47 (`git diff f624c15 origin/main -- src` has no `retained` line). `strings`: `v48-OccupancyEndsAtClose` once. `particle binary inspect`: CRC ok (7e59bca4), compiled for boron, product version 48, depends on system module 1 at version 6600. The file: `/Users/chipmc/Downloads/v48-OccupancyEndsAtClose-boron.bin`. The sha256 is not reproducible across builds (v47 WO), so upload this file and compare Particle's size and sha256 with the numbers above, §13 (j).
- **Release gate, Particle cloud compile (§3), 31 Oct: not completed; the service failed.**
  - The full tree: `Compile failed: Compile input not allowed` (a different error from before; it listed 368 files including `tests/`, `docs/` and `tools/`).
  - Only `src/`, `lib/` and `project.properties`: `Compile failed: Compiler timed out or encountered an error` after 2 min with no diagnostics, as on the v46 and v47 heads. **No flash or RAM numbers.**
  - **Per the gate: recorded; release from the verified local build.** Retry once more before the upload and add the numbers here if it passes.

## 2. Roles and plan

- **Dev-09** (e00fce68399ee6244a963935) and **Dev-14** (e00fce688e592afaf23ac4fb) are the soak devices, locked to v48 by Chip. Dev-09's `timing.reportingIntervalSec` stays 3600. **Antenna swap (Chip, about 09:36Z 31 Oct, 17:36 SGT): Dev-14's bad antenna (Dev-09's old one) was replaced with a new one.** Dev-14 is no longer the bad-antenna stress case, so **from v48 its connect metrics count again** (`connecttime`, `ConnSummary`, alerts 31/40/15, connect failures), judged like Dev-09's; the v47-WO exclusions no longer apply. Anything before 09:36Z 31 Oct (all of v47) stays on the old antenna and is not compared with v48 on connect metrics.
  - **Antenna check (from Dev-14's next connects after 09:36Z; Claude Code, read-only):** `sig=` in the serial `ConnSummary` lines is back to Dev-09's level (about `/29` quality; Dev-09's recent values are the reference, read them the same day), and connect failures stop (no `ConnSummary: fail`, no 660 s `CELLULAR_ACQUIRE` timeouts, no failsafe stage 2/3 from a connectivity silence). If `sig=` stays near the old values or failures continue, record it and tell Chip (the antenna or its connector).
  - **Antenna check, first result (Claude Code, 1 Nov 00:06Z; serial `ConnSummary` lines, 09:36Z to 00:00Z; Dev-09 as the reference):** the swap worked. Dev-14 `sig=` quality 12 or lower fell from **23 of 156 lines (15%)** before the swap (30 Oct to 09:36Z 31 Oct) to **1 of 41** after; slow recoveries (over 30 s, `cloudRecoverStage` 1-2, 110-466 s) fell from **22 to 1** (09:52:53Z, 155 s, 16 min after the swap; none after). No `ConnSummary: fail`, 660 s timeout or failsafe event before or after. Quality median is 25 (Dev-09 33), strength 77 (Dev-09 82), so Dev-14 is better but not at Dev-09's level. The after sample is about 8 daytime hours; **the full SGT day of 1 Nov (22:00Z to 14:00Z) is the real check: any recurrence of 150 s or longer connects is recorded and Chip told.**
  - A failsafe stage-2/3 reset after the 22:00 SGT close remains the designed #151 behavior (record it; not a failure).
- **First v48 boot, status 1 Nov 00:06Z (S3):** **neither device has booted v48.** Both woke at 22:00Z 31 Oct on `v47-PersistFreeze` (status reset 30, hibernate wake, `fw_version` 47; Dev-14 22:00:46Z, Dev-09 22:00:57Z), and their 00:00Z serial lines read "no updates pending". So v48 is not yet uploaded and locked to them, or the OTA has not reached them. The anchors below stay empty until each device logs reset 70 with `v48-OccupancyEndsAtClose`; the 1 Nov 14:33Z day run, the 24 h grading and the 2 Nov run are not yet anchored and run only after it. Dev-14 first v48 boot: (pending). Dev-09 first v48 boot: (pending).
- **Anchor** every job on each device's actual first v48 boot (status reset 70, `v48-OccupancyEndsAtClose`; exclude the OTA-interrupted session).
- Pilot devices (Vaughan-002, OCCUPANCY-LAB-01, ChurchSt-2) are not part of this soak and stay on v47.
- Not set as the product default during the soak.
- Chip uploads v48 (checking size and sha256 against the release PR), then locks the two devices.

## 3. Checks, per device (as the v47 soak §3)

1. Payload unchanged (same keys and order as v47); the standard checks (resets, `watchdogResetCount`, `fh`, alerts); "missing" = a report v47 would have sent.
2. First boot: reset 70 once (the OTA) and nothing else; `dailyoccupancy` continuity across the OTA.
3. Sleep returns: every cycle sleeps (`cyc` − `slp` = 1); no breadcrumb-28 watchdog; reset types as v47.
4. Failsafe: `failsafeStage` and `failsafeCount`; counts only.

## 4. The item 10 test: pass criteria

Use the baseline script's per-category report (`--dates <SGT day> --tz SGT-8 --open 6 --close 22 --debounce-ms 300000`, #163) plus the hook increments.
- **PASS needs all of:**
  1. On each full v48 day, the **first report after the 06:00 open reads `dailyoccupancy` 0** on both devices, with no post-close carry-in in the script's per-report increments.
  2. The **last session before 22:00 is credited to the close** (about 9.5 min if it was a full session, less if cut by the close).
  3. **Normal sessions: mean within 9.5 ± 0.1 min over at least 40 sessions** (architect ruling, 2 Nov: the true session is 570 s = 9.50 min; the whole-minute `dailyoccupancy` quantization moves the mean by up to 0.02, so the old 9.54-9.58 spread was too tight).
  4. **The standard health checks (§3) are clean:** resets (only the one reset 70 at the OTA, and reset types as v47), breadcrumb-28 watchdogs (none on Dev-09; Dev-14 stays on the item 14 watch), item 18 gaps (a gap of 3 h or more outside the overnight close is recorded; hook gaps of 4 h or more are the normal 7-8 h night gap ending in a normal report), payload (same keys and order as v47), and missing reports (none not explained by a failed connect).
- **FINDING (holds the rollout):** any non-zero first-of-day `dailyoccupancy`; any session credited after the close; a normal-session mean outside 9.5 ± 0.1 min over at least 40 sessions; a payload difference; a breadcrumb-28 watchdog on Dev-09; an item 18 gap on Dev-09; a missing report not explained by a failed connect.
- **Not testable remotely:** the 0-24 and Unknown-openness cases (host tests cover them, Stage 7 checked them).

- **Item 20 hardware test (only if Chip resets Dev-09's exerciser to start at about :53; not a pass condition for v48 unless it runs):** report the sessions that start near :53 and span the hourly report (occ=1 before and occ=0 after a report; categorize by observed overlap, never by clock phase). **PASS: each spanning session is credited 9-10 min (about 570 s, the normal range 567-594 s), with at least 8 spanning sessions on v48 days.** Any spanning session credited 12 min or more (the v45/v46 behavior) is a FINDING against #155. If the exerciser is not reset, the record stays "verified in code; hardware test pending".

## 5. Default (if both pass)

Set v48 as the product default (recheck size and sha256), merge the release PR, run the fleet health check about 24 h later (the v47 report as the model). Then the migration's main waves may start (WO-2026-10-29-001 §10); items 19 and 15 follow in v49.

## 6. Schedule

Dates are set when Chip uploads v48. Chip has no stable connection until further notice, so every step is remote and anchored on the first v48 boot:

| When | Step | Who |
|---|---|---|
| on Chip's go | Upload v48; lock Dev-09 and Dev-14 | **Chip** |
| the next 22:00Z wake | OTA (reset 70) | automatic |
| first full v48 SGT day, 14:30Z | v48 day 1 and the first-boot check | Claude Code (Sonnet subagent) |
| 24 h after each first boot | 24 h grading | Claude Code (Sonnet subagent) |
| the day after | day 2 and the decision | Claude Code, then **Chip** |

All reads need `AWS_PROFILE=particle-admin`; an expired SSO token stops a job.

## Result

**Missed run (1 Nov): the scheduled 14:33Z day run did not fire.** It was run by hand at 23:44Z 1 Nov, and the 24 h grading is run by hand at about 02:52Z 2 Nov, not from the schedule. **Day run 1 (partial v48 day, SGT 1 Nov; Claude Code, from S3 hooks, status and serial).** First v48 boots (status reset 70): **Dev-09 02:51:37Z, Dev-14 02:52:18Z** on 1 Nov, so 1 Nov is a partial v48 day; the first full v48 day is 2 Nov. The baseline script's per-window table is not meaningful for 1 Nov (the exerciser phase changed at 04:52Z, and the day includes v47 hours).
- **Item 10:** first report after the 06:00 open (22:00Z) reads `dailyoccupancy` **0** on both (Dev-09 22:02:02Z, Dev-14 22:00:53Z). Dev-09's last session (13:53-14:00Z) was credited **+7 min to the close** (466 at 14:00:40Z); Dev-14's last session ended 13:50Z (457) and nothing counted after. **No report between 14:00Z and 22:00Z on either device.**
- **Normal sessions (hook increments from 02:55Z to the close):** Dev-09 **9.58 min** (33 sessions, 14 x 9 and 19 x 10, excluding the +7), Dev-14 **9.55** (33; 15 x 9, 18 x 10). Inside the v45-v47 9.54-9.58.
- **Health:** only the reset 70 at the OTA and the 22:00Z hibernate wake (reset 30); `watchdogResetCount` unchanged (Dev-09 46, Dev-14 127); alerts 0; failsafe stage 0; payload keys and order as v47.
- **Item 20 hardware test (Dev-09 exerciser reset to :52 at 04:52Z): PASS, no FINDING.** Nine :52 sessions (04:52Z to 12:52Z) each spanned the :00 report (occupied before it, unoccupied after, total held across it) and were credited **9, 10, 9, 9, 9, 10, 10, 9, 10 min**; the one with a surviving serial close line (07:02:36Z) read 577 s. No spanning session at 12 min or more. (The 13:52 session was cut by the 14:00Z close: +7.) Serial lines are patchy, so the exact seconds are mostly unavailable.
- **Dev-14 antenna, full SGT day (22:00Z 31 Oct to 14:00Z 1 Nov): confirmed.** 101 connects, 0 slow recoveries (22 before the swap), quality 12 or lower 1 of 101, median 29 (Dev-09 33), no `ConnSummary: fail`, no failsafe event. One slow connect at 23:04Z (223 s, stage 2, `sig=78/33`) and Dev-09 one at 22:01Z (76 s, stage 1): to watch on 2 Nov.
- **24 h grading (by hand, 2 Nov 02:53Z; window = each first v48 boot, Dev-09 02:51:37Z and Dev-14 02:52:18Z on 1 Nov, to 02:50Z 2 Nov; S3 hooks, status, serial; Claude Code):**
  - **Resets:** only the reset 70 at the OTA and the 22:00Z hibernate wake (reset 30) on both; `watchdogResetCount` unchanged (Dev-09 46, Dev-14 127), so no new breadcrumb-28 watchdog (the status field `lastWatchdogBreadcrumb` 28 is the old record); failsafe stage 0, count 0; `fw_version` 48 on every hook (114 and 113).
  - **Payload:** one key set on every hook, same keys and order as v47 (`occupancy, dailyoccupancy, battery, vc, key1, temp, alerts, resets, connecttime, fh, lfb, cyc, slp, timestamp`). `cyc - slp` = 1 on every hook of both devices.
  - **Item 18 gaps / missing reports:** the only gap over 1.5 h is the normal night gap 14:00Z to 22:00-22:02Z (8.0 h, ending in a normal report); no missing report.
  - **Item 10:** first report after the 06:00 open reads 0 on both (see day run 1); Dev-09's last session credited +7 to the close, Dev-14 none cut; nothing between the close and the open.
  - **Normal sessions over the full window:** Dev-09 **9.56 min** (48 sessions: 21 x 9, 27 x 10) in the 9.54-9.58 spread; Dev-14 **9.52** (48: 23 x 9, 25 x 10), **0.02 under the spread**. The whole-minute truncation of `dailyoccupancy` is worth up to 1 min over 48 sessions (0.02 min each), so this is at the quantization limit; the criterion as written (outside the 9.54-9.58 spread) is touched, so it is **recorded for the architect, not self-passed.** Day 2 shows whether it persists.
  - **Alerts:** none except Dev-14 alert 40 on one report (12:30:55Z 1 Nov): the serial line at 12:21:38Z is "Webhook response timeout after 20003 ms - raising alert 40" (the report at 12:21:40Z itself reached S3); cleared on the next report; not at the open.
  - **Connects:** Dev-14 one slow connect (223 s, 23:04Z, `sig=78/33`) and Dev-09 one (76 s, 22:01Z); battery flat (Dev-09 79.77 to 79.61, Dev-14 80.42 steady).
  - **Verdict inputs for the decision:** every health check clean; item 10 as designed; item 20 PASS; the one open point is Dev-14's 9.52. **The decision follows the full-day run at 14:33Z 2 Nov (the first full v48 SGT day, 22:00Z 1 Nov to 14:00Z 2 Nov).**
- **Architect ruling (2 Nov): the 24 h grading is a PASS.** Dev-14's 9.52 is minute-quantization noise around the true 570 s = 9.50 min. Session check replaced (criterion 3 and the FINDING line above) by "mean within 9.5 ± 0.1 min over at least 40 sessions", for this and future soak WOs. Dev-09 9.56 and Dev-14 9.52 (48 sessions each) both pass it. Dev-14 alert 40 (webhook response timeout, 12:21:38Z, cleared on the next report) and its 223 s connect go on the **watch list, not findings.**
- **Still to do:** the full-day run by hand at 14:33Z 2 Nov with the new session check; if clean, recommend v48 as the product default (Chip sets it).
