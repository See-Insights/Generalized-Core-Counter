# v46 release and bench soak (the sleep split: S-1 to S-6)

**Author:** Claude Code, 23 Oct 2026, from Chip's rulings and plan (the architect may amend) · **Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §2, §13 (g), (h), (i), (j)
**Agents:** release PR, Claude Code itself (no model dispatch); soak and baseline runs, a **Sonnet subagent, medium**, read-only. Report the model used.
**Budget:** 0 `src/` lines beyond the version string.
**Start:** after the sleep PRs are merged by Chip in order (#128, #130, #131, #132, #136, #139). **Chip is travelling and has no bench access until 31 Oct: every step is remote** (S3, the Particle API, the console, OTA). P7-A (#142) ships in v47, not here.

## Plain goal

Ship the sleep split (S-1 teardown module; S-2 in-file phase functions; S-3 cloud gate; S-4 plan and nightly heap guard; S-5 pre-sleep summary and Wi-Fi guard; S-6 wake-event processing and post-wake routing; `handleSleepingState()` 1,492 → 155 lines) as v46 on Device OS 6.6.0. **The device should behave exactly like v45.** The split was proven by frozen-transcript tests, not by the bench; this soak is the field check, and the exerciser (a repeatable 570-second occupancy session every 20 minutes) is the first measured occupied-minute baseline to compare.

## 1. Release PR (Claude Code)

- Branch `release/v46` from main **after** #139 merges. Confirm the `src/` diff from v45's commit contains only the S-1 to S-6 changes and the version string; list anything else and stop.
- `bump_version.sh` to the v46 name per the convention; product version 46. CHANGELOG, plain language: no intended behavior change; the sleep path moved into named functions and one module; **nothing to expect on the first boot** (no layout change; P-0's one-time reset was v45's).
- Fresh `BUILD_PATH_BASE` build on 6.6.0 (Boron); `strings` shows the version; record the `.bin` size and sha256 against v45's (153,054 B; a small text change is expected from the S-6 move, about −8 B, and the S-5 move); suite `N/N`, serial. Open the PR; **don't merge until the soak passes.**

## 2. Bench soak: Dev-09 is the decision device; Dev-14 is the bad-RF stress case (roles changed 24 Oct)

**Roles (Chip, 24 Oct, after the antenna verdict):**
- **Dev-09** (e00fce68399ee6244a963935) is the **v46 decision device.** It stays on v45 for **Sat 24 and Sun 25 Oct (the baseline)**; **Chip locks it to v46 on Sunday after the 14:30Z run**; the OTA lands at the nightly 22:00Z wake = **06:00 local Monday**; **v46 Mon 26 and Tue 27 Oct.** The v46 decision compares **v46 (Mon-Tue) with v45 (Sat-Sun) on the same device and the same antenna** (Dev-14's old, healthy antenna: `sig` about 78/29 to 80/33). Dev-09's data before its first normal session after the 24 Oct 00:53Z reset (the first start report after the 01:00:08Z close report; that session starts 01:13:04Z) are excluded (recovery item 18), so Saturday is a partial day.
- **Dev-14** (e00fce688e592afaf23ac4fb) **stays on v46 (since 23 Oct 08:30:18Z) as a bad-RF stress case**: it has had Dev-09's old, bad antenna since about 03:59Z on 24 Oct (`sig` quality 6 to 12, `CloudRecover` network resets, a 660 s failure), until 31 Oct. **Judge its sleep path, payload, serial `session=` values and report delivery; exclude its connect metrics after 03:59Z** (`connecttime`, `ConnSummary`, alerts 31/40/15 caused by failed connects). **On Dev-14 long connects mean more sessions close through the awake path, which may shift its session lengths: report them separately from Dev-09's.** **If Dev-14 goes dark (no hook or status) for 3 h or more, record a possible item-18 recurrence** (the #151 fix is not in v46) **and change nothing.**


Chip uploads the PR's `.bin` as product v46 (Particle's size and sha256 must match the PR) and **releases it to Dev-09 (Dev-14 already runs v46)**; Dev-09 stays locked, KEEP_ALIVE, no config changes, and v46 is **not** set as the product default. **The OTA lands at the nightly 22:00Z wake = 06:00 local (SGT) the next morning**, so Sunday 25 Oct is a second full v45 day and Monday 26 Oct a full v46 day from the open. Dev-09 is graded over 24 h from its first v46 boot against its own v45 days.

**Checks, per device** (as the v45 soak, WO `2026-10-20-v45-release-and-soak.md` §2, without the P-0 reset):
1. The payload unchanged; PIR and timer wakes; the daily reset; standard checks (resets, `watchdogResetCount`, `fh`, alerts, GateFail). "Missing" means a report v45 would have sent.
2. **Sleep returns:** every cycle sleeps (hook `cyc` − `slp` = 1 per cycle); no breadcrumb-28 watchdog; reset types as v45 (30 hibernate wake, 70 once for the OTA); no all-modes-failed reset.
3. `connecttime`, alert 15 and `slow_teardown` against v45; the `MODEM_HEALTH`/`MODEM_POLICY` wording identical (the format strings are unchanged in source).
4. **Counts only (no pass/fail):** the STOP fallbacks, the hibernate bypass, the nightly heap guard and the all-modes-failed reset, **if the serial captured them.**

**Serial is sparse by design for this soak** (serial off for the baseline; the bench Pi forwarder only captures reconnects): the **ThrashGuard counters and the `MODEM_*` lines are read if captured, not a pass condition.** The pass evidence is S3 (`status`, `watchdog`, `hibernate_wake`, `Ubidots-Sensor-Hook-v1`) and the exerciser comparison below.

## 3. The exerciser comparison (the occupied-minute baseline)

`tools/exerciser_baseline.py` (WO-2026-10-23-001, PR #147) for each local day (SGT, TZ `SGT-8`, open 6, close 22, `--debounce-ms 300000`): expected credit 570 s (9.5 min) per session. **The comparison is on Dev-09 alone: Saturday 24 Oct (from 01:13:04Z) and Sunday 25 Oct (v45) against Monday 26 Oct and Tuesday 27 Oct (v46)**, same device and antenna: accuracy (the mean daily error) and precision (the standard deviation of the daily error), the per-window error, and the **per-session start-to-end distribution** (hooks, and serial `session=` where captured) beside the day totals. **Session length on the bench is not settled:** 569-581 s observed; the two long sessions (580-581 s, hooks) ran while Dev-14 was on laptop USB, and the clean serial values after the move to the forwarder were 569 s and 570 s; cause open; Saturday's full day on the forwarder decides. Compare v46 against Dev-09's v45 days as measured; do not correct for a bias. A v46 error mean within the v45 spread is no change; anything outside is a finding. **Dev-14 is run through the same script for its session lengths only, and its numbers are reported separately** (its long connects shift sessions to the awake path). 23 Oct was shakedown.

Any breadcrumb-28 watchdog, a payload difference, a missing report not explained by a failed connect, or an occupied-minute error on Dev-09 outside the v45 spread is a FINDING and holds the rollout.

## 3a. Bench events that bear on the grading (keep this list current)

- **24 Oct about 03:59Z: Chip swapped the antennas between Dev-09 and Dev-14.** Dev-14 (v46) now has the antenna Dev-09 had, and Dev-09 (v45) has Dev-14's. **Any Dev-14 connect failure, slow registration or long `connecttime` after 03:59Z is judged against the swap, not against v46**, until the swap's effect is understood (the watch of 24 Oct 04:00 to 05:03Z reports which device's registration failures follow the antenna; see its report). The same applies to Dev-09 and to any reset or alert caused by a failed connect on either device. Before the swap Dev-09 had been failing to register (CELLULAR_ACQUIRE 660 s failures from 02:11Z, then a self-recovery at 03:53:17Z), while Dev-14 connected cleanly (`sig` 78/29 to 78/33, connecttime 1 to 2 s). The Monday and Tuesday grading, the v46 day-1 check and the baseline comparison treat a Dev-14 connect failure after 03:59Z as an antenna question first; a **sleep-path** finding (a breadcrumb-28 watchdog, a payload difference, a missing report not explained by a connect failure) is still a v46 finding. If the antennas are swapped back, add the time here.

- **24 Oct about 05:09Z: the antenna verdict.** `ConnSummary sig=` quality followed the antenna (S3 serial, 24 Oct): Dev-14 on Dev-09's old antenna read 77/6, 75/6, 75/6, 77/12, 75/6 and one 660 s failure after its first 80/29; Dev-09 on Dev-14's old antenna read 78-80/29-33 over nine connects. **Dev-09's old antenna (or its cable or connector) is bad; it is on Dev-14 until 31 Oct.** Dev-14's connect metrics after 03:59Z are excluded from the grading (section 2).

## 4. Default (if both pass)

Set v46 as the product default (recheck size and sha256); merge the release PR; the fleet health check about 24 h later. Dev-09 and Dev-14 stay locked for v47 (the P7-A persistence freeze; its bench step is already done, PASS).

## 5. Dated schedule and relay (SGT = UTC+8; the local day 24 Oct = 23 Oct 16:00Z to 24 Oct 16:00Z)

| When | Step | Who | Paste-able relay |
|---|---|---|---|
| **Sat 24 Oct 14:30Z** (22:30 SGT) | **v45 baseline, day 1** (SGT 24 Oct, Dev-09 from its first normal session at 01:13:04Z; Dev-14 reported separately) | Claude Code (job `2edc80e6`) | "Run the v45 baseline for Dev-09 (decision device; hooks with payload timestamp >= 2026-10-24T01:13:04Z) and Dev-14 (stress case, sleep path and session lengths only): sync hooks and status for UTC 23 and 24 Oct, run `tools/exerciser_baseline.py` with `--dates 2026-10-24 --tz SGT-8 --open 6 --close 22 --debounce-ms 300000`, report per-day and per-window error, accuracy and precision, the per-session start-to-end distribution (hooks and serial session=), Dev-09's sig= values, and any session off 570 s by more than 30 s." |
| Sat 24 Oct (after the merge sequence) | **Cut v46:** Chip merges #128 → #130 → #131 → #132 → #136 → #139 (each retargets to main after the one below it); Claude Code prepares `release/v46` | Chip merges; Claude Code builds | "Prepare release/v46 from main after the sleep series is merged: bump_version, release.sh, build for Boron on Device OS 6.6.0, report the .bin path, size and sha256, open the PR, do not merge." |
| **Sun 25 Oct 14:30Z** (22:30 SGT) | **v45 baseline, day 2** (a full v45 day for Dev-09) and the v45 accuracy/precision over Sat+Sun | Claude Code (job `53704073`) | The Saturday relay with `--dates 2026-10-25` and UTC days 24 and 25 Oct. |
| **Sun 25 Oct, after the 14:30Z run** | **Lock Dev-09 to v46** (upload v46 to the product if not yet done; release it to Dev-09 only; do not set it as the product default) | **Chip** (console) | "Upload `<path>` as product firmware v46 (size `<n>`, sha256 `<h>`); lock Dev-09 to v46 after its 14:30Z run on Sunday; do not set it as the product default. Dev-14 already runs v46." |
| Sun 25 Oct 22:00Z (Mon 06:00 SGT) | The OTA lands at the nightly wake | automatic | Dev-09: first v46 boot about Sun 22:01Z (status reset 70, the v46 version string). |
| **Mon 26 Oct 14:30Z** | **v46 day 1 on Dev-09** (SGT 26 Oct, a full v46 day from the 06:00 open) plus the day-1 check; Dev-14 judged on its sleep path, payload and delivery | Claude Code (job `32ef1d06`) | The Saturday relay with `--dates 2026-10-26` and UTC days 25 and 26 Oct, plus: "confirm Dev-09's first v46 boot from serial and S3 status; check resets, `watchdogResetCount`, `fh`, alerts, GateFail, missing reports, the payload and cyc - slp against its v45 days; Dev-14: sleep path, payload, session= and delivery only; exclude its connect metrics." |
| **Mon 26 Oct 22:20Z** (Tue 06:20 SGT) | **24 h soak grading** from Dev-09's first v46 boot (Dev-14 on its sleep path only) | Claude Code (read-only Sonnet subagent; job `b686b4ab`) | The v45 grading prompt (`2026-10-22-v45-bench-soak-result.md` as the format) with v46 in place of v45, Dev-09's v45 days as the baseline, and the Dev-14 rules of section 2. |
| **Tue 27 Oct 14:30Z** | **v46 day 2 on Dev-09** and the v45-vs-v46 comparison on Dev-09 (Sat+Sun vs Mon+Tue); Dev-14 session lengths reported separately | Claude Code (job `91e5b6e2`) | The Saturday relay with `--dates 2026-10-27`, then: "compare accuracy and precision of Dev-09's Mon and Tue (v46) with its Sat and Sun (v45), the session-length distributions, and report whether v46 changed the occupied-minute error; report Dev-14's session lengths separately." |
| Tue 27 Oct | **Decision** on v46 as the product default and the release PR | **Chip** | (his step; then the fleet health check about 24 h after the default is set) |

**If this session ends:** none of the scheduled jobs survive. Paste the relay text above into a new session at the stated time. All reads need `AWS_PROFILE=particle-admin`; if the SSO token has expired the job stops and says "token needed".

## Result

(to be filled in by the grader)
