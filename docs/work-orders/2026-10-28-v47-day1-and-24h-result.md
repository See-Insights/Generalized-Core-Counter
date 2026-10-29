# v47 bench soak: day 1 (Wed 28 Oct SGT) and the 24 h grading (graded 28 Oct 14:40Z and 22:45Z)

**WO:** `2026-10-27-v47-release-and-soak.md` (AMENDED banner: item 20 is not a pass condition) §3. **Graders:** Sonnet subagents (read-only; fresh S3 pulls, both devices, five event types); Claude Code spot-checked the headline numbers in S3 with its own pulls (§13 (j)). `tools/exerciser_baseline.py` sha256 `d7503dfd…8e70b` (main at the time); its day totals were wrong on 28 Oct (post-close carry-in), so hook increments are the evidence; the fix is PR #163. Nothing was changed on any device, in AWS or in the product.

**Verdict: v47 passes day 1 and the 24 h grading on Dev-09 and (sleep path only) Dev-14. No FINDING by the ruling's letter. Per Chip (29 Oct) v47 is NOT cleared for default today; the default decision is Saturday 31 Oct.**

## What each device ran

Both took v47 by OTA, not at the nightly wake (serial `comm.ota UpdateStart` 22:36:53Z and 22:40:38Z): **Dev-09 first v47 boot 27 Oct 22:37:31.0Z, Dev-14 22:41:56.6Z** (status resetReason 70, `v47-PersistFreeze`, spot-checked). The 22:00Z wake before it was reset 30 on v46. The OTA-interrupted session: Dev-09 none (the 22:20 session closed 22:29:54Z on v46, credit 9); Dev-14 22:40:21Z, credited 2 (item 15's boot cap), excluded. `appBreadcrumb` 18 at the OTA boot is normal (OTA during a report connect; architect, 29 Oct).

## Dev-09, day 1 (28 Oct SGT; v46 part: two sessions at 22:00 and 22:20Z of 27 Oct, credit 9 each)

| Category | n | Credit | Mean | Error vs 9.5 | Hook duration |
|---|---|---|---|---|---|
| normal | 43 | 411 (19 ×9, 24 ×10) | 9.558 | +2.5 | 570-596 s, mean 572.9 |
| delayed-connect (occ hook late 100 s or more) | 2 | 20 | 10.0 | +1.0 | 589, 580 s |
| late-start (occ=1 hook built late, 02:02:40Z, serial PIR 02:00:31Z) | 1 | 9 | | -0.5 | 441 s (a hook artifact) |
| post-close (item 10) | 1 | 7 | | | 317 s |
| report-spanning / close-cut / orphan | 0 | | | | |

**Comparison with v45/v46 normal sessions** (v45 Sat 9.58 n=26, Sun 9.54 n=48; v46 Mon 9.575 n=40, Tue 9.535 n=43, combined 9.554; SE about 0.07): **v47 9.558, no change.** Spot-check: 46 sessions from 22:40Z, credit 440, none with a hook between its occ=1 and occ=0, longest credit 10. Serial `session=` (14 captured) 568-596 s, none off 570 s by more than 30 s. **Report-spanning sessions: 0** (3600 s in force; the :30 reports ended on 27 Oct between 09:30:03Z and 11:29:57Z, before the OTA): item 20 is not tested here, as ruled.

## The 24 h grading (Dev-09 27 Oct 22:37:31Z to 28 Oct 22:37:31Z; Dev-14 to 22:41:56Z)

The data ran to 22:30Z (Dev-09) and 22:29Z (Dev-14): the last 7 and 12 min and the 22:40Z session are not in these numbers.

| Check | Dev-09 | Dev-14 |
|---|---|---|
| hooks in window | 115 | 113 |
| resets | only 70 (OTA) and 30 (22:00Z wake); `resets` 0 on every hook; no 140 | same |
| `watchdogResetCount` / `lastWatchdogUptimeMs` | 46 / 6,696,704 unchanged | 127 / 48,215,552 unchanged |
| `watchdog` files 27-29 Oct | 0 (**no breadcrumb-28**) | 0 (0 in about 450 v47 sleeps) |
| failsafe (status, serial) | stage 0 count 0; `Failsafe:` lines 0 | same |
| `cyc` - `slp` = 1 | all hooks (the only drops: the wake and the OTA boot) | all |
| alerts | 0 | 40 ×2 (12:50Z, 14:05Z; connect-driven, excluded) |
| payload key tuple | one 16-key tuple, unchanged vs v46 | same |
| missing reports | none (all hourlies, 46 session slots, close and wake reports present) | none; largest gap outside the close 19.8 min (**no item 18**) |
| `fh` | median 80,512, min 74,280 (occ=1 mid-connect), max 89,088 (wake); flat | median 80,688, min 74,424, max 89,080; flat |
| GateFail, MODEM_, Sleep->Error, `sig=na` (serial) | 0 | 0 |

**Normal sessions in the window, Dev-09:** 44, credit 420, mean **9.545** against 9.554 (hook 561-596 s, serial 570-596 s; 4 delayed-connect, 39 credit; 0 spanning). **Dev-14:** 47 sessions, credit 448, mean 9.53, none spanning, all credits 9 or 10; 21 sessions have a late-published hook (153-623 s, the antenna); three late-start artifacts (04:04, 06:02, 13:06Z).

**The 28 Oct 22:00Z wake (both devices, reset 30 on v47):** `hibernate_wake` ok, ALARM, osReason 30, Dev-09 req 28260 / actual 28261 / err 1 / count 5, Dev-14 req 28320 / 28321 / 1 / 6 (shorter than 28799 because each slept at about 14:09Z after its post-close hook). **The post-close credit carried across the wake: Dev-09's first report (payload 22:00:29Z) reads `dailyoccupancy` 7, then 7, 16, 16, 26; Dev-14's reads 5, 5, 14, 14, 24 (spot-checked).** Neither started the new day from 0: recovery item 10, already recorded, not a v47 failure.

## Observations (none a pass condition)

- **Post-close sessions (item 10, 28 Oct):** Dev-09's close hook (payload 13:59:59Z, daily 458) was published 14:02:37Z after a 151 s close connect with CloudRecover stage 2; the exerciser's 22:00 SGT slot started a PIR session at 14:00:44Z (serial `Occ: state=1 reason=pir`); occ=1 daily 0 at payload 14:02:35Z, occ=0 daily 7 at 14:07:52Z (317 s); `Idle->Sleep` 14:09:22Z. Dev-14: close hook daily 449, PIR at 14:00:50Z, an orphan occ=0 hook (payload 14:07:47Z, daily 5, alert 40: webhook timeout 14:00:28Z) with no occ=1 hook. Both are in the Step 0 dispatch for item 10 (`2026-10-29-item-10-post-close-credit-step0-*`).
- **Dev-09 battery dip (28 Oct):** 79.77% to 74.05% at 07:40:43Z, 74.22% at 08:10Z, 74.38% at 09:00Z, back to 79.93% at 09:50Z and 79.77% from 11:20Z, with no alerts or resets. **ChargeDiag (serial, Claude Code, as Chip asked): `vbus=NONE src=VIN` from 08:00Z to 10:30Z (the lines in the dip hours), `soc` 74.1 to 74.4; the charger read `chg=OFF` from 06:30Z (with `vbus=ADAPT`) until `chg=DONE` at 09:43Z; `vbus=NONE` also read once at 01:10Z (soc 79.8). So USB or adapter power was lost during the dip (the device ran on VIN), and the gauge sagged; the cause of the power loss and of the earlier `chg=OFF` is not determined.** A note only.
- **Dev-14:** `connecttime` median 3 s, p90 195 s, max 464 s (30 hooks at 100 s or more) and 188 "Failed to determine server address": the bad antenna, not judged. "Failed to power off" ×3.
- **Baseline script:** the 28 Oct script day totals were invalid (Dev-09 7 vs 456, Dev-14 5): the last value picks up the post-close credit. PR #163 fixes it; with it Dev-09 reads 458 vs 456 (+2.0) and Dev-14 449 vs 456 (-7.0).

## Not determined

The last minutes of each 24 h window; the exact time Dev-09's interval returned to 3600 (between 09:30:03Z and 11:29:57Z on 27 Oct, from hook timing); the cause of the Dev-09 delayed connects and of the battery dip's power loss; the Dev-14 serial `session=` values for the v47 day (none captured); a breadcrumb-28 rate for Dev-14 on v47 better than "under about 7 per 1,000 at 95%" (about 450 sleeps).

## What follows (Chip, 29 Oct)

v47 is not cleared for default today (Chip is travelling to China; restricted access until Sat 31 Oct). The bench keeps soaking v47: day runs Thu 29 and Fri 30 Oct 14:33Z and a summary for Saturday morning. The default decision and the v47 fleet health check (about 24 h after the default; the v46 format plus each device's failsafe stage and any reset 140 or deep power-down) follow on Saturday. Item 10 is the next fix after v47 (Step 0 below). No firmware PR merges are needed before Saturday.
