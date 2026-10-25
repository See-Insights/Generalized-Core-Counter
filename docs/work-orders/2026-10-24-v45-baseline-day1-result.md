# v45 baseline, day 1 (Saturday 24 Oct 2026): result, and the Dev-14 v46 watchdog evidence

**WO:** `2026-10-25-v46-release-and-soak.md` §3b (catch-up 1; the 24 Oct 14:30Z session job did not fire) and §3. **Run:** 25 Oct about 01:08Z, from S3, over the original window (SGT 24 Oct = 23 Oct 16:00Z to 24 Oct 16:00Z); a catch-up, not a live run. **Agents:** Sonnet 5.5 subagents (`claude-sonnet-5-5`), read-only, three: the catch-up run, an independent spot-check (fresh S3 pull, §13 (j)), and the Dev-14 evidence run. `tools/exerciser_baseline.py` was byte-identical to main's (sha256 `d7503dfd…8e70b`). All times UTC unless marked SGT.

**Verdict: Dev-09's v45 day 1 is recorded as the baseline, with the categories below. Dev-14 has one FINDING by the WO's letter: a breadcrumb-28 sleep watchdog on v46 (the reset at about 11:24:36Z on 24 Oct). Per Chip's ruling it is added to #113 and holds v46's fleet default until judged; it does not stop the Dev-09 plan (lock to v46 after the Sunday 14:30Z run).** Nothing was changed on any device or in AWS.

## Firmware check

Dev-09: all 188 hooks of 23-25 Oct carry `fw_version` 45 (`v45-PowerSplit`). Dev-14: v46-SleepSplit since 23 Oct 08:30:43Z (status reset 70 at 08:30:45Z); all 102 hooks in the window are fw 46.

## Dev-09 (v45), hooks with payload timestamp >= 2026-10-24T01:13:04Z

Script: `--dates 2026-10-24 --tz SGT-8 --open 6 --close 22 --debounce-ms 300000`: **measured 397 min, expected 361, error +36 (+9.97%), 38 sessions, no flags.** One day, so accuracy is +36 and precision is not defined. A first run with the pre-cut hooks included gave a wrong phase (expected 456, error -59: the device sat at occupancy 1 and daily 345 until the counter dropped to 0 at 01:13:04Z) and is discarded.

**The spot-check confirms 397 and the +36 as the script's output, with two caveats.** (1) The script counts 38 sessions; an independent count finds **39 that credited minutes**: 38 occ=1 to occ=0 pairs plus one **orphan** occ=0 at 02:44:03Z (+9, daily 38 to 47) with no occ=1 report before it (a dropped or unsent hook; not determined). The script's expected count drops the close-cut 13:53 slot and does not see the orphan. Against 39 full sessions (370.5 min) the error is **+26.5**. (2) Either way the ten straddlers below are the largest term.

**Per session category (Chip's ruling: grade Sunday and later runs this way):**

| Category | Count | Credit (min) | Mean | Error vs 9.5 per session |
|---|---|---|---|---|
| normal (one occ=1 report; includes the 03:17:04 duplicate-timestamp pair) | 26 | 249 | 9.58 | +2.0 |
| hour-straddler (a session across the hourly :00 report) | 11 (10 credit >= 12) | 133 | 12.09 | +28.5 |
| close-cut (13:53:30 to 13:59:59, cut by the 22:00 SGT close) | 1 | 6 | 6 | -3.5 |
| orphan occ=0 (02:44:03Z, no occ=1 report) | 1 | 9 | 9 | -0.5 |
| **Total** | **39** | **397** | | **+26.5** |

**The ten straddlers that credit 12 or 13 min** (occ=1 reports, the :00 report in the middle, then the occ=0 report that carries the increment): 03:53:17 + 04:00:03 → 04:06:47 (**13**); 04:53:21 + 05:00:02 → 05:05:43 (13); 05:53:21 + 06:00:02 → 06:05:38 (12); 06:53:23 + 07:00:02 → 07:05:58 (13); 07:53:25 + 08:00:02 → 08:05:45 (12); 08:53:26 + 09:00:02 → 09:05:37 (12); 09:53:27 + 10:00:02 → 10:05:45 (12); 10:53:29 + 11:00:01 → 11:05:39 (12); 11:53:29 + 12:00:07 → 12:05:43 (12); 12:53:31 + 13:00:12 → 13:05:47 (13). Four at 13 and six at 12: **124 min against 95 at 9.5 each = +29.0**. An eleventh straddler (01:53:07 + 02:00:02 → 02:11:03) credits only 9; it sits in the Dev-09 outage described below.

**Session lengths.** Hooks (first occ=1 to occ=0 report): the straddlers run 730-810 s; the normal sessions 570 s; the close-cut 389 s; five sessions 02:22-03:50Z read 660 s because their reports were delayed by failed connects. Serial `session=` (captured for 15 of 38 sessions): 569, 570 ×3, 571 ×2, 575, 585 ×2, 595, 734, 736, 737, 740, 810 s; the five long ones (810 s at 04:06:54Z, 737 s at 06:05:47Z, 740 s at 08:05:59Z, 734 s at 12:06:04Z, 736 s at 13:06:10Z) are straddlers, so the device really stayed in session 165-240 s longer; it is not a reporting artifact on the hook side. **Mechanism found by the Codex Step 0 (recovery item 20, report `2026-10-25-item-20-hour-straddler-step0-report.md`; the central line checked by Claude Code): the :00 report does not touch the session, but after the report and connect the sleep plan wakes after the full debounce (300 s), not the time left on it (`State_Sleep.cpp:607-616`), so the expired debounce is noticed one full sleep late and the late close time is credited (`State_Common.h:377-395`). Credit is added once; the minutes are real elapsed time of a firmware-prolonged session, not 12-13 min of occupancy. No fix is proposed.**

**23 Oct, same device, is not a clean second day.** The spot-check finds 8 sessions crediting >= 12 min on 23 Oct (UTC day): 3 are clean straddlers (02:12:30 +13, 05:06:33 +12, 07:08:21 +14); the rest are in the unstable period (01:31 +44, 01:52 +20, 02:37 +25) or are the 205-min close of item 19 at 13:59:59. Straddlers at 04:05 (+11) and 06:05 (+10) on 23 Oct do not reach 12. Item 20 therefore rests on 24 Oct (ten of ten from 04:06Z to 13:05Z); 23 Oct is consistent but mixed.

**Standard checks, Dev-09:** `cyc` - `slp` = 1 on every report; no resets in the window; `watchdogResetCount` 46, unchanged; no watchdog events, no breadcrumb-28; GateFail 0; no gap over 11.5 min between reports. `sig=` 70-80 / quality 6-33, typically 78/29 (52 of the connects), 10 `sig=na` on failed connects, no `sig=0/0`.
- **Outage:** alert 31 on 10 consecutive reports from 02:11:03Z to 03:50:05Z (`connecttime` 660 s, `CELLULAR_ACQUIRE` failures, PPP errors, CloudRecover); self-recovered at 03:53:17Z with no reset; reports arrived late with payload timestamps intact (lag up to 6,798 s). `cyc` stayed at 36 on all reports during the 110-minute outage while payload timestamps advanced (cause not determined).
- **Just before the window (excluded, recovery item 18):** the failsafe stage-2 reset (140, data 2) of 23 Oct 10:40:55Z delivered at 00:56:42Z, and the 00:53Z power cycle.

## Dev-14 (v46), session lengths only, reported separately

Whole SGT day: measured 417, expected 456, **error -39.0** (flags: missing reports, reset); 101 windows, mean -0.39, sd 5.51. The -39 is the watchdog event below (about four sessions of credit not counted). Outside it, window errors are ±0.5 min. Hook start-to-end is meaningless after 04:00Z (connect delays 130-600 s inflate it). Serial `session=` (13 captured): 568, 570, 571 ×2, 574 ×2, 575 ×2, 579, 580, 581, 584 s (mean 575.6, sd 5.0); none off 570 by more than 30 s, 6 before the antenna change and 7 after. Dev-14's phase puts :00 between sessions, so it has no straddlers. `sig=` before 03:59Z 77-82 / 25-37 (17 ok connects); after, 75/6, 77/12, 77/6, 75/0 and similar: the bad antenna, as expected. Its post-03:59Z connect metrics (alerts 31 at 04:32Z and 11:11Z, slow connects 04:41-11:21Z) are antenna and not graded.

## Dev-14: the breadcrumb-28 sleep watchdog on v46 (FINDING by the WO's letter)

**Correction to the first report:** the `watchdog` event is published at 12:01:43Z, but the reset happened **earlier, at about 11:24:36Z**: the event was queued and delivered on the first cloud connect after the reboot. Evidence: the status `lastWatchdogUptimeMs` 48,215,552 against the serial prefix 48,036,357 at 11:21:36.41Z gives 179.2 s, so the reset was at 11:24:35.6Z ±1 s; the boot was at about 11:25:24Z (serial uptime 12,615 ms at 11:25:36.5Z); serial uptime is continuous from there to 13:56Z, so there was no second reset.

**Raw event** (S3 `watchdog`, published 12:01:43.080Z, fw 46): `{"reset":"watchdog","osReason":60,"bc":28,"stage":"sleep","elapsed":111,"queue":0,"state":3,"connAge":175651}`. Status 12:02:32.136Z and 12:02:34.880Z: `resetReason 60`, `resetReasonData 0`, `alert 19`, `appBreadcrumb 28`, `appBreadcrumbMs 48215552`, `watchdogResetCount 127` (126 on every 23 Oct status), `lastWatchdogBreadcrumb 28`, version `v46-SleepSplit`. The loop was in SLEEP_PREP, 111 ms into the stage; `connAge` 175.6 s puts the last cloud mark at about 11:21:40Z, 3.5 s after the Report-to-Connect request.

**The serial lines (the forwarder's `timestamp` is the Pi's clock; each line carries the device uptime ms, used to align):**
```
11:21:25.986 0048035868 PowerDiag[1537]: post-wake source=USB_HOST ...
11:21:29.134 ...035902 Occ: state=1 reason=pir-wake led=300s report=1
11:21:30.145 ...035903 LoopStage: stage=SLEEP_PREP elapsed=482926 state=3 q=0 connMs=483643
11:21:31.165 StateReq: Sleep->Report reason=sleep-pir-occupancy-report
11:21:35.411 ...036354 Report: occ=1 totalMin=394 alert=0 ok=1 ledger=req
11:21:36.410 ...036357 StateReq: Report->Connect reason=occupancy change   <- last device line
11:21:37.458 SERIAL_DISCONNECTED, then MISSING (no app lines until 11:25:36)
11:25:36.521 0000012615 [ncp.client] ERROR: Failed to power off            <- after the reboot
11:25:54.928 ...031018 WARN: SLEEP: disconnect/modem-off exceeded budget (30002 ms, cloud=0 radioOn=0 standby=0) - raising alert 15
11:25:55.997 MODEM_HEALTH: unstable reason=slow_teardown elapsed=30003
11:25:57.016 MODEM_POLICY: standby temporarily disabled reason=unstable_modem
11:25:58.053 LoopStage: stage=SLEEP_PREP elapsed=30006 state=3 q=2 connMs=0
11:25:59.108 StateReq: Sleep->Error reason=sleep-disconnect-timeout
11:26:00.938 Entering ERROR_STATE with alert=19, resetCount=1, resolution=0
11:26:03.738 Low-power idle: offline with 2 queued event(s) - sleeping and will flush on next connect
11:26:06.716 WARN: SLEEP: pre-sleep gate blocked (cloud=0 radioOn=1 standby=0) - requesting modem off   <- last before 14 min of silence
```
The serial has **no breadcrumb lines** (breadcrumbs are in status and the watchdog event only). **The three minutes between 11:21:36Z and the reset are dark:** the Pi had no app lines (USB flapping), so what the device did there is not known. Gaps with no app lines: 11:13:29-11:21:25, 11:21:37-11:25:36, 11:26:07-11:40:13, 12:03:01-12:25:07, 13:01:04-13:15:12. Last WakeReturn lines in 11:00-12:10: TIMER 11:45:19 (`elapsed=305251`), 11:55:13, 12:00:29 (`310243`); PIR 11:45:37 to 11:50:07. Last `Sleep:` line: `12:03:01.344 Sleep: td=18812ms cloud=16ms modem=18812ms standby=1/0 mode=IKA tier=H occ=1`. Last LoopStage lines: 11:21:30, 11:25:58 and 12:00:34.836 (`SLEEP_PREP elapsed=315835 q=2 connMs=0`).

**Dev-14's own breadcrumb-28 rate** (`watchdog` events in S3 17-25 Oct; sleeps = positive increments of the hook `slp` counter per boot segment, counting every wake/sleep cycle including PIR wakes; an undercount, since sleeps between the last hook and each reset are lost):

| Firmware | Sleeps counted | Plausible upper bound | Counter resets | Bc-28 events | Per 1,000 sleeps |
|---|---|---|---|---|---|
| v44 | 497 | about 650 | 8 | 0 | 0 |
| v45 | 417 | about 500 | 4 | 0 | 0 |
| v44 + v45 | 914 | about 1,150 | 12 | 0 | 0 (95% upper bound about 2.6-3.3) |
| **v46** | **765** | about 850 | 4 | **1** | **1.3** (1.2-1.3) |
| v46 before 03:59Z 24 Oct (antenna swap) | 393 | | | 0 | 0 |
| v46 after 03:59Z | 372 | | | 1 | 2.7 (about 6 on the 167 sleeps up to the reset) |

`watchdogResetCount` stayed 126 from the first v44 status to 23 Oct 22:00Z, and `lastWatchdogUptimeMs` 1,687,188 until this event: the count is carried over from before v44. **0 against 1 is not significant, and is confounded by the antenna** (zero in the 393 v46 sleeps before the swap, one in the 372 after it). One event cannot show v46 differs from v44/v45.

**Why the 11:21-13:15Z sessions were not credited (data, then inference).**
- **Data:** the exerciser starts a session about every 20 min (:01, :21, :41), each about 10 min with a PIR pulse every 30 s. The 11:01 session closed normally (`11:10:51 Occ: state=0 reason=debounce session=570s`; daily 384 to 394), published only at 11:13:24Z because the 11:00 connect failed (660 s). The 11:21:22 session opened and was reported `occ=1` at 11:21:36Z (daily 394, `cyc` 371). The reset followed at 11:24:36Z. After the reboot the daily total sat at **394 in every report** (12:00:27Z, 13:00:19Z; the 11:30 and 12:30 reports are absent from the hook stream) until a report at 13:15:07Z with `occ=0` and **398 (+4)**; normal counting resumed from 13:25Z. The total was frozen, not reset (the reset to 0 is at the 22:00Z hibernate wake). PIR wakes continued while the device was in its 5-minute timer-wake cycle (serial: 11:45:37-11:50:07, 12:26:40-12:29:10, 12:45:41-12:50:12). The wakes at 12:00:33 and 13:00:28 read `Wake: reason=TIMER open=1 ready=1 occ=1 led=0s`. **Do not read `led=0s` as 'debounce not running':** the Codex Step 0 found that `logWakeSummary()` divides `signalLEDTimeRemaining()` (already seconds) by 1000 again (`State_Sleep.cpp:34-40`; `device_pinout.cpp:124-138`), so the `Wake:` line's `led=` is always 0s; the `led=300s` on `Occ: state=1` lines is a different log.
- **Inference (not established):** the session opened at 11:21:22Z was still latched `occ=1` after the reboot (retained occupancy); the `led=0s` on the `Wake:` lines carries no information (the logging bug above), so whether the debounce deadline survived the reset is not shown; the sleep-plan behavior of item 20 (a fresh full debounce sleep) could also have stretched the session across the 5-minute timer wakes; it closed at 13:15:07Z and was credited only +4 min, far below the 114-minute span, so the start time was probably lost or restarted at the reset. This looks like the shape of recovery item 15 (occupancy lost across a reset) and item 19 (a session merged across an interruption), but the bench data cannot say which rule applied.
- **Not determined:** why the watchdog fired (no serial between 11:21:37 and 11:25:36, and no loop-stage trail beyond the 111 ms record); what the device did in those three minutes; whether PIR pulses were seen 11:26-11:40 and 12:03-12:25; what ended the latch at 13:15; why the credit was exactly 4 min; the exact count of missed sessions (the schedule is inferred from hooks and sparse serial, not an exerciser log).

**Dev-14 health, 24-25 Oct (item 18 check):** online; last hook 25 Oct 01:01:10Z (v46, battery 80%); resets: the watchdog and the 22:01:12Z hibernate wake (30). Gaps of 30 min or more between consecutive events (hooks, status, hibernate_wake, watchdog): 11:21:36Z to 12:01:43Z (40 min), 12:02:36Z to 13:00:40Z (58 min), and the normal overnight close 14:02:25Z to 22:00:54Z. **No gap of 3 h or more outside sleep: no item-18 recurrence.** Since 22:00Z it has run clean (10.5-min cadence, alerts 0). Dev-09 is also online (last hook 01:01:16Z, v45, battery 79%).

## What follows

1. **Recovery item 20** (hour-straddler over-credit) is recorded in `docs/RECOVERY_PLAN_2026-09-26.md`; the read-only Codex Step 0 (gpt-5.6-sol, high, stdin closed; dispatch and report in this directory) found the mechanism (above). No fix yet.
2. **Dev-14's watchdog** evidence is this file; Chip's ruling is to add it to #113 (the merged breadcrumb-28 investigation), which holds v46's fleet default until judged. #113 is merged, so that is a PR comment; **not posted yet** (awaiting Chip's go on the comment). The Dev-09 plan (lock to v46 after the Sunday 14:30Z run, v46 Mon-Tue) is not affected. Under WO §2, the antenna is the likely trigger, but a sleep-stage watchdog on v46 stays a v46 finding.
3. **Sunday and later runs** are graded per session category (normal, hour-straddler, close-cut) as in the table above, with the orphan and the delayed-connect sessions listed separately.
4. Sunday's day-2 run (job `53704073`, 14:30Z) is next, then Monday.
