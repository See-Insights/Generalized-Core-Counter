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

## 2. Bench soak: Dev-09 and Dev-14, 24 h, then two full days

Chip uploads the PR's `.bin` as product v46 (Particle's size and sha256 must match the PR), releases it to Dev-09 and Dev-14 only (both locked, KEEP_ALIVE, no config changes), **not** as the product default. **The OTA lands at the nightly 22:00Z wake = 06:00 local (SGT) the next morning**, so Sunday 25 Oct is a second full v45 day and Monday 26 Oct is a full v46 day from the open. Each device is graded over 24 h from its first v46 boot, against its own v45 days.

**Checks, per device** (as the v45 soak, WO `2026-10-20-v45-release-and-soak.md` §2, without the P-0 reset):
1. The payload unchanged; PIR and timer wakes; the daily reset; standard checks (resets, `watchdogResetCount`, `fh`, alerts, GateFail). "Missing" means a report v45 would have sent.
2. **Sleep returns:** every cycle sleeps (hook `cyc` − `slp` = 1 per cycle); no breadcrumb-28 watchdog; reset types as v45 (30 hibernate wake, 70 once for the OTA); no all-modes-failed reset.
3. `connecttime`, alert 15 and `slow_teardown` against v45; the `MODEM_HEALTH`/`MODEM_POLICY` wording identical (the format strings are unchanged in source).
4. **Counts only (no pass/fail):** the STOP fallbacks, the hibernate bypass, the nightly heap guard and the all-modes-failed reset, **if the serial captured them.**

**Serial is sparse by design for this soak** (serial off for the baseline; the bench Pi forwarder only captures reconnects): the **ThrashGuard counters and the `MODEM_*` lines are read if captured, not a pass condition.** The pass evidence is S3 (`status`, `watchdog`, `hibernate_wake`, `Ubidots-Sensor-Hook-v1`) and the exerciser comparison below.

## 3. The exerciser comparison (the occupied-minute baseline)

`tools/exerciser_baseline.py` (WO-2026-10-23-001, PR #147) for each local day (SGT, TZ `SGT-8`, open 6, close 22, `--debounce-ms 300000`): expected credit 570 s (9.5 min) per session. **Compare Saturday 24 Oct and Sunday 25 Oct (v45) against Monday 26 Oct and Tuesday 27 Oct (v46):** accuracy (the mean daily error) and precision (the standard deviation of the daily error), per device, and the per-window error. **The v45 days carry a steady bias of about +10 s per session** (Dev-14 sessions measured 581 s and 580 s against 570 s on 23 Oct: about +0.17 min per session, about +0.5 min per hour): compare v46 against that v45 bias, do not correct for it. A v46 error mean within the v45 spread is no change; anything outside is a finding. 23 Oct was shakedown (Dev-09 installed about 02:40Z, Dev-14 about 03:28Z; Dev-14's P7-A bench window 02:50Z to 03:20Z is excluded).

Any breadcrumb-28 watchdog, a payload difference, a missing report, or an occupied-minute error outside the v45 spread is a FINDING and holds the rollout.

## 4. Default (if both pass)

Set v46 as the product default (recheck size and sha256); merge the release PR; the fleet health check about 24 h later. Dev-09 and Dev-14 stay locked for v47 (the P7-A persistence freeze; its bench step is already done, PASS).

## 5. Dated schedule and relay (SGT = UTC+8; the local day 24 Oct = 23 Oct 16:00Z to 24 Oct 16:00Z)

| When | Step | Who | Paste-able relay |
|---|---|---|---|
| **Sat 24 Oct 14:30Z** (22:30 SGT) | **v45 baseline, day 1** (SGT 24 Oct) | Claude Code (job `837ac003`) | "Run the v45 baseline for Dev-09 and Dev-14: sync hooks and status for UTC 23 and 24 Oct, run `tools/exerciser_baseline.py` with `--dates 2026-10-24 --tz SGT-8 --open 6 --close 22 --debounce-ms 300000`, report per-day and per-window error, accuracy and precision, and any session off 570 s by more than 30 s." |
| Sat 24 Oct (after the merge sequence) | **Cut v46:** Chip merges #128 → #130 → #131 → #132 → #136 → #139 (each retargets to main after the one below it); Claude Code prepares `release/v46` | Chip merges; Claude Code builds | "Prepare release/v46 from main after the sleep series is merged: bump_version, release.sh, build for Boron on Device OS 6.6.0, report the .bin path, size and sha256, open the PR, do not merge." |
| **Sun 25 Oct 14:30Z** (22:30 SGT) | **v45 baseline, day 2** (SGT 25 Oct; a second full v45 day) | Claude Code (job to create) | The Saturday relay with `--dates 2026-10-25` and UTC days 24 and 25 Oct. |
| **Sun 25 Oct** (day) | **Upload v46; release it to Dev-09 and Dev-14 only** | **Chip** (console) | "Upload `<path>` as product firmware v46 (size `<n>`, sha256 `<h>`); release it to Dev-09 and Dev-14 only; do not set it as the product default." |
| Sun 25 Oct 22:00Z (Mon 06:00 SGT) | The OTA lands at the nightly wake | automatic | First v46 boot about Sun 22:01Z (status reset 70, the v46 version string). |
| **Mon 26 Oct 14:30Z** | **v46 baseline, day 1** (SGT 26 Oct, a full v46 day from the 06:00 open) plus the day-1 check (first-boot times from serial and S3, no watchdog, payload, reports) | Claude Code (job once `release/v46` exists) | The Saturday relay with `--dates 2026-10-26` and UTC days 25 and 26 Oct, plus: "confirm each first v46 boot from serial and S3 status; check resets, `watchdogResetCount`, `fh`, alerts, GateFail, missing reports, the payload and cyc − slp against the v45 days." |
| **Mon 26 Oct 22:20Z** (Tue 06:20 SGT) | **24 h soak grading** from each device's first v46 boot | Claude Code (read-only Sonnet subagent; job once `release/v46` exists) | The v45 grading prompt (`2026-10-22-v45-bench-soak-result.md` as the format) with v46 in place of v45 and the v45 days as the baseline. |
| **Tue 27 Oct 14:30Z** | **v46 baseline, day 2** (SGT 27 Oct) and the comparison with Saturday and Sunday (v45) | Claude Code (job once `release/v46` exists) | The Saturday relay with `--dates 2026-10-27`, then: "compare accuracy and precision of Mon and Tue (v46) with Sat and Sun (v45); report whether v46 changed the occupied-minute error, with the +10 s v45 bias noted." |
| Tue 27 Oct | **Decision** on v46 as the product default and the release PR | **Chip** | (his step; then the fleet health check about 24 h after the default is set) |

**If this session ends:** none of the scheduled jobs survive. Paste the relay text above into a new session at the stated time. All reads need `AWS_PROFILE=particle-admin`; if the SSO token has expired the job stops and says "token needed".

## Result

(to be filled in by the grader)
