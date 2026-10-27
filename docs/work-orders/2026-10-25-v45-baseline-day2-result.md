# v45 baseline, day 2 (Sunday 25 Oct 2026): result

**WO:** `2026-10-25-v46-release-and-soak.md` §3, §5 (job `53704073`, "Sun v45 day 2"). **Run:** 25 Oct 14:33Z, on time (not a catch-up), from S3. **Agents:** a Sonnet subagent (read-only; fresh S3 pull of 23-25 Oct, five event types, both devices); Claude Code spot-checked the headline numbers in S3 with its own pull (§13 (j)). `tools/exerciser_baseline.py` byte-identical to main's (sha256 `d7503dfd…8e70b`). SGT day 25 Oct = 24 Oct 16:00Z to 25 Oct 16:00Z. Saturday's result: `2026-10-24-v45-baseline-day1-result.md`.

**Verdict: Dev-09's v45 baseline holds: Sunday error +2 min over 48 sessions, no flags. Sunday had no hour-straddlers, so it is a control day and does not test item 20. No FINDING by the WO's letter.** Nothing was changed on any device or in AWS.

## Dev-09 (v45), hooks with payload timestamp >= 2026-10-24T01:13:04Z

Script: `--dates 2026-10-25 --tz SGT-8 --open 6 --close 22 --debounce-ms 300000` (202 hooks kept, 76 excluded as pre-cut): **measured 458 min, expected 456, error +2.0 (+0.44%), 48 sessions, no flags.** Saturday re-run on the same footing reproduces day 1 exactly (397 / 361 / +36.0 / 38 sessions by the script; +26.5 against the 39 sessions that credited).

**Spot-check (Claude Code, own pull of Dev-09 hooks, 24-25 Oct):** 113 hooks in the SGT day, all fw 45; first 22:01:01Z (daily 0), last 13:59:59Z (daily **458**); **48 increments summing to 458, each 9 or 10**; no session has an occ=1 report in the :00-:01 minutes between its start and its close (no straddlers); `resets` 0 and `alerts` 0 on every hook. S3 `watchdog` folders for 24-25 Oct: Dev-14 one file (24 Oct, the known event), Dev-09 none.

**Per session category (Dev-09, hooks; credit = the daily increment at the occ=0 report; error against 9.5 min per session):**

| Category | Sat n | Sat credit | Sat mean | Sat error | Sun n | Sun credit | Sun mean | Sun error |
|---|---|---|---|---|---|---|---|---|
| normal | 26 | 249 | 9.58 | +2.0 | 48 | 458 | 9.54 | +2.0 |
| hour-straddler | 11 (10 credit >= 12) | 133 | 12.09 | +28.5 | 0 | 0 | | |
| close-cut | 1 | 6 | 6.0 | -3.5 | 0 | 0 | | |
| orphan occ=0 | 1 | 9 | 9.0 | -0.5 | 0 | 0 | | |
| **Total** | **39** | **397** | | **+26.5** | **48** | **458** | | **+2.0** |

Sunday's 48 normal sessions: 24 credit 9 and 24 credit 10 (mean 9.54: whole-minute quantization). Sunday has no close-cut: the last session is 21:41:13 SGT (13:41:13Z) to 13:50:42Z, and none starts after 21:50 SGT; the 13:59:59Z hook is a plain occ=0 report with +0.

**Reports delayed by failed or slow connects, listed separately (also counted above in their shape category).** Saturday: seven sessions with failed connects (alert 31, `connecttime` 660), credit 70, mean 10.0, error +3.5 (the five pure-outage sessions alone: credit 48, mean 9.6, error +0.5); two slow-but-successful connects (+12 straddler 09:53:27 to 10:05:45, +10 normal 10:13:28 to 10:22:57). Sunday: one session, 11:40:45 to 11:50:47Z, +10, error +0.5; its occ=1 report was published 175 s late (serial: DNS -170, CloudRecover stage 1 at 11:42:17Z, stage 2 at 11:43:17Z, PPP error 5, `ConnSummary ok` at 11:43:38Z, `elapsed=140367`, no fail line, alerts 0, no reset).

**Session lengths.** Hooks, first occ=1 report to occ=0 report: Sunday's 47 undelayed sessions run 556-584 s (mean 572); the delayed one is 602 s (the only one more than 30 s off 570). Saturday's normal undelayed sessions 569-595 s (mean 572.8). Serial `session=` (the Pi captured 13 of 48 Sunday sessions): 584, 576, 581, 580, 579, 580, 579, 574, 584, 571, 584, 579, 580 (mean 579.3, sd 3.7); none off 570 by more than 30 s. `sig=` on Sunday: all ok, 77-80 / 19-37, typically 78/29 or 78/33; no `sig=na`, no `sig=0/0`.

**Accuracy and precision over Sat+Sun (n = 2):** accuracy (mean daily error) **+19.0 min**, precision (sample sd) **24.0 min**. **These mix two regimes** (Saturday with ten straddlers, Sunday with none), so they describe neither. For the v46 comparison use the per-category table, normal sessions against normal sessions (Sat 26: mean 9.58; Sun 48: mean 9.54, both +2.0 against 9.5 per session). Per-window error on Sunday (script default phase): 112 windows, mean +0.02, sd 8.28; the sd is a **phase artifact** of the script, not firmware: T0 is the first occ=1 report (22:01:14Z), which carries about 14 s of lead, so each expected close lands just after the real occ=0 report and credit moves one window; with `--phase-offset-s -40` Sunday's windows are mean +0.02, sd 0.33, and day totals are identical with any offset.

## The phase note (why Sunday had no straddlers)

Saturday's exerciser sessions started at :13, :33 and :53, so the :53 sessions spanned the hourly :00 report (item 20). Sunday's started at :01, :21 and :41 (first occ=1 at 22:01:14Z = 06:01:14 SGT), so the :00 report falls between sessions and item 20 was not exercised. **Chip's note (25 Oct):** in normal operation the exerciser's phase is set by the 06:00 power-on (:01, :21, :41), so no hourly straddlers occur. Consistent with the data: Saturday's first normal session after the 00:53Z power cycle was 01:13:04Z (the :13 phase); Sunday's first was 22:01:14Z after the 22:00Z wake. There is no exerciser log, so this is consistent with the data, not independently proven. **Consequence for the v46 comparison:** Monday and Tuesday on the :01/:21/:41 phase will have no straddlers either, so the Dev-09 v46 against v45 comparison is **normal sessions against normal sessions** and cannot show item 20's fix (WO-2026-10-25-001).

## Note for the v47 soak WO (Chip, 25 Oct; there is no v47 WO yet)

To test item 20's fix on the bench, **set the test device's `reportingIntervalSec` to 1800** (reports at :00 and :30), so each :21 session (:21 to about :30:30) spans the :30 report. Expected on v45/v46: 12-13 min credited per spanning session (the sleep after the report is a full debounce); with WO-2026-10-25-001 (PR #155): about 570 s, 9 or 10 min, as normal sessions. Grade per session category (normal, hour-straddler = spans a report, close-cut), with the baseline script's per-window and per-category report as the pass test. Restore the device's interval afterwards. This should replace the "hour-straddling sessions should read 570 s" line in WO-2026-10-25-001's Bench section (PR #155), which assumed the :53 phase; the architect amends it.

## Dev-14 (v46), reported separately (connect metrics excluded after 03:59Z 24 Oct)

Script, all Dev-14 hooks: Saturday 417 vs 456, **-39.0** (flags: missing reports, reset: the 24 Oct watchdog); **Sunday 459 vs 456, +3.0, 48 sessions, no flags** (window sd 3.82, the same phase artifact). One Sunday session credits 12 (13:07:23 occ=1 to 13:12:31 occ=0) after a 13:00:02 report with `connecttime` 336-438 s and 5-8 min publish lags; whether that is a hidden straddler is not determined. Serial `session=` (18 captured, 24 Oct 00:00Z to 25 Oct 16:00Z): none off 570 by more than 30 s (Sunday SGT: 586, 580, 580, 585, 576, 585, 568; mean 580). `sig=` is the bad antenna (77/12, 75/6, 77/6, 75/12, 75/0), ungraded.
- **Breadcrumb-28 watch (recovery item 14):** one event, the 24 Oct watchdog (estimated reset about 11:24:35.6Z, reproduced from the fresh pull); none since. By the subagent's definition (per boot segment, the final `slp` value) v46 had 752 sleeps to 25 Oct 01:00Z plus 384 after, **1,136** to 25 Oct 14:00Z (the earlier count was 765 to 01:00Z; the 13-sleep difference is a method difference, not determined): 1 in 1,136-1,149 (0.87-0.88 per 1,000), still confounded by the antenna. v44 + v45: 0 in 914 (earlier count, not re-derived).
- **Item 18:** no gap of 3 h or more outside the overnight close; the only gaps of 30 min or more are the 40 and 58 min ones around the watchdog and the close. Alerts on Sunday: 31 at 08:21:45Z (antenna, excluded). No reset on Sunday; no sleep-path warning.

## Health, 25 Oct (S3 to 14:00Z, the 22:00 SGT close)

- **Dev-09:** online, last event 14:00:15Z (hook payload 13:59:59Z), fw v45-PowerSplit, battery 79.6%, alerts 0, no reset since 25 Oct 00:00Z; the 14:00:28Z serial line shows it going to hibernate. `watchdogResetCount` 46 (unchanged), `fh` Sunday median 80,456 (min 79,352, flat), `cyc` - `slp` = 1 on all 278 hooks of 23-25 Oct, no GateFail in the window.
- **Dev-14:** online, last event 14:00:08Z, fw v46-SleepSplit, battery 79.8%, no reset since 25 Oct 00:00Z; `watchdogResetCount` 127; `cyc` - `slp` = 1 on all 314 hooks.

## Not determined

Why the exerciser phase moved between Saturday and Sunday beyond the power-cycle observation above (no exerciser log); the cause of Saturday's orphan occ=0 at 02:44:03Z (a dropped hook) and of `cyc` standing at 36 during Saturday's outage; the Pi's 35 missing Sunday `session=` lines; whether Dev-14's 13:07 +12 session is a hidden straddler; the 13-sleep difference in Dev-14's v46 count; Dev-14's reset trail beyond the 111 ms watchdog record (its serial carries no breadcrumb lines).

## Next

Dev-09 is locked to v46 by Chip after this run (the OTA lands at the 22:00Z wake = 06:00 SGT Monday 26 Oct); the first v46 boot is read from S3 `status` (reset 70, fw 46) before grading. Then Monday 14:30Z (v46 day 1), Monday 22:20Z (24 h grading) and Tuesday 14:30Z (Dev-09 v46 normal sessions against v45's).
