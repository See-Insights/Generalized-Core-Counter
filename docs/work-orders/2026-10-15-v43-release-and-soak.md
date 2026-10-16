# v43 release and bench soak (Step 6 2a + 2b)

**Author:** architect, 2026-10-15 · **Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §2, §13 (g), (h), (i), (j)
**Agent:** Claude Code. Release PR: Claude Code itself (no model dispatch). Soak grading needs S3, the Particle API and the forwarder logs, so it goes to a **Sonnet subagent, medium**. Report the model actually used.
**Budget:** 0 `src/` lines beyond the version bump.
**Start:** only after #103 (2b-6) is merged.

## Plain goal

Ship the Step 6 restructuring (2a dead code #94; 2b-1 to 2b-6: #97, #98, #99, #100, #101, #103) as v43. **v43 is meant to behave exactly like v42.** The soak's job is to show that on real hardware: same reports, same wakes, same resets, same heap.

## 1. Release PR (Claude Code)

- From main after #103; confirm all seven PRs above are merged and that no other `src/` change has landed since v42 (`git diff --stat <v42 tag or ff1715c>..HEAD -- src/`; list anything unexpected and stop).
- `bump_version.sh` to `v43-SensorShape` (or the closest name under the convention); product version 43. **The version string is final before the soak**, so the soaked `.bin` is the one released (the v41/v42 lesson).
- CHANGELOG, plain language: no intended behavior change. Code restructured: about 460 lines of dead code removed; the sensor now supplies its own report fields, wake pin and hourly/daily reset; a golden test pins the report payload byte for byte.
- Fresh `BUILD_PATH_BASE` build; `strings` shows the version; record the `.bin` size (versus v42's 152558 bytes) and sha256; tests `N/N (sh via zsh, py via python3)`.
- **Before Chip uploads:** check the production devices' `device-settings` ledgers for `sensorMode`. If any runs COUNTING, say so in the PR and stop; I'll add a short COUNTING step on Dev-09.
- Open the PR for Chip; don't merge.

## 2. Bench soak — Chip does the device steps

Chip uploads the PR head's `.bin` as product v43 (Particle's size and sha256 must match the PR's, §13 (j)), then OTAs it to **Dev-09 and Dev-14 only**, both locked to v43 and both left in KEEP_ALIVE. Each device is graded on its own 24 h window from its first v43 boot (reset 70). No config changes during the soak.

**Checks, per device, each against the same device's previous 24 h on v42:**
1. **Payload unchanged end to end** (S3 webhook): the same keys in the same order with the same number formats as a v42 report from the same device; no new, missing or renamed field; `battery`, `alerts` and `resets` in their usual places.
2. **PIR wakes unchanged** (serial): every motion wake from sleep is followed by occupancy handling, and every occupancy start and end is reported immediately (≤ 20 s, as on v41/v42).
3. **Timer wakes unchanged** (serial and S3): scheduled reports in every open-hour slot; none missing; `cyc − slp = 1`.
4. **Daily reset unchanged** (S3 webhook): the close report stamped close − 1 s, then the next morning's daily totals are 0. `resetCount` is cleared at the close as before.
5. **Hourly reset:** only runs in COUNTING mode, and both bench devices run OCCUPANCY, so it isn't exercised here (host-tested in 2b-6). Checked before the soak, in §1.
6. **Standard:** no unexpected resets; `watchdogResetCount` unchanged; heap (`fh`, serial `heap=`) in the v42 range; no alerts except the known post-boot 44; no `GateFail`; one daily config wait per device per day.

Any breadcrumb-28 watchdog, a payload difference or a missing report is a FINDING and holds the fleet rollout.

## 3. Grading

PASS or FINDING per device, with log lines, ≤ 60 lines in chat; every number names its source and window (§13 (j)). Add the result to this file and commit on a docs branch with a PR.

## 4. Fleet steps (Chip, only if both pass)

1. Set v43 as the product default (the soaked `.bin` is already product v43; recheck size and sha256).
2. Dev-09 and Dev-14 stay locked for the next soak.
3. Run the usual fleet health check about 24 h after the default changes.

## Result

### Boron-Dev-09 (graded 2026-10-16, about 07:30Z)

**Grader:** Sonnet 5.5 subagent (`claude-sonnet-5-5`), medium effort, read-only; the controller (Claude Code, Opus) re-checked S3 for the open slot. Dev-14 is not graded here.
**Verdict: PROVISIONAL PASS, one OPEN item.** The 07:00Z (15:00 SGT) timer slot of 16 Oct is not in S3 at 07:29Z. It becomes a FINDING (missing report) if it never arrives. No breadcrumb-28 watchdog, no payload difference and no lost report was found otherwise.

**Sources and windows (UTC).** v43 window: 15 Oct 07:03:21Z (S3 status, reset 70) to 16 Oct 07:03:21Z. Baseline v42: 14 Oct 07:03Z to 15 Oct 07:03Z (Dev-09 booted v42 at 06:52:47Z on 14 Oct, so the baseline is a full 24 h). S3 days 14–16 Oct: `Ubidots-Sensor-Hook-v1` (155 reports), `status`, `watchdog`, `hibernate_wake`, `serialLog`. Serial is incomplete (the forwarder reconnects USB at each wake and drops lines; wall-clock stamps lag the device by 8–16 s), so intervals use device uptime ms and slots use S3 `timestamp`.

1. **Payload: PASS.** All 155 reports (S3 hook, 14–16 Oct) share one key sequence: `occupancy,dailyoccupancy,battery,vc,key1,temp,alerts,resets,connecttime,fh,lfb,cyc,slp,timestamp`. The S3 envelope order is identical for fw 41, 42 and 43. Formats are the same: `battery`, `temp`, `vc` with 2 decimals, the rest integers, `key1` a string. The v43 boot status (15 Oct 07:03:21Z) matches the v42 one (14 Oct 06:52:47Z) key for key.
2. **PIR wakes: PASS, with a connect-stall caveat.** Serial, v43 window: 362 `WakeReturn reason=PIR`, 10 TIMER; v42: 66 PIR, 4 TIMER. Every `Occ: … reason=pir-wake` was followed by `Report: occ=` 0.4–0.6 s later (device uptime ms), in both versions. Example: 16 Oct 00:57:10 start, report in S3 at 00:57:13Z. Reports that took longer than 20 s landed while a Connect (660 s budget) was already running: 15 Oct 13:42:41 (+284 s), 13:50:30 (+581 s), 13:51:37 (+515 s), 14:01:55 (+192 s); 16 Oct 06:03:57 (+97 s). v42 shows the same pattern (S3 reports 14 Oct 07:32:54, `connecttime` 342, and 08:23:35, `connecttime` 297). Not verified: the 20 s limit during a stalled Connect, because v42 has no stall of the same length to compare.
3. **Timer wakes: OPEN.** `cyc − slp = 1` in all 155 reports (S3 hook, 14–16 Oct, both windows). Every slot from 22Z (15 Oct) through 06Z (16 Oct), and 08Z to 14Z on 15 Oct, is present. Some arrived late but delivered (S3 `published_at` vs `timestamp`): 02Z slot (ts 02:01:45), 05Z (published 05:05:27), 06Z (06:05:40), 14Z close (14:05:07, `connecttime` 293). **Open:** the 07:00Z slot (`timestamp` near 1792134000) is not in S3 at 07:29:30Z. Serial shows a Connect from about 07:00:03Z, CloudRecover stage 1 at 07:01:03 and stage 2 at 07:02:05, then `Connect: fail … last=CELLULAR_ACQUIRE sig=na` at 07:11:03. The two reports published at 06:46:53Z and 06:46:55Z carry `timestamp` 06:36:02Z and 06:46:50Z, so they are earlier events, not this slot. A second alert-31 report (`occ=1 totalMin=334 alert=31`, serial 16 Oct 07:11:20Z) is also not yet in S3. **Re-check S3 for the 07:00Z slot; if it never arrives, this is a FINDING.**
4. **Daily reset: PASS on stamps, one deviation.** v43 close (serial 15 Oct): `Daily boundary reached (boundary=1792072800 … close=22)` at 14:00:19Z, `Running Daily Cleanup` at 14:00:26Z. Close report: S3 `timestamp` 1792072799000 (13:59:59Z, close − 1 s), `dailyoccupancy` 291, published 14:05:07Z. The next report (ts 1792073104) has `dailyoccupancy` 0. v42 close: ts 1791986399000 (13:59:59Z), `dailyoccupancy` 321, next-morning report (14 Oct 22:00:38Z) 0. **Deviation:** the v43 next-morning report (15 Oct 22:00:43Z, ts 1792101624) has `dailyoccupancy` 8, not 0. Serial shows PIR activity after cleanup (`Occ state=1` at 14:01:55Z), and the Singtel Connect kept the device awake until 14:22Z; its S3 report is ts 1792073429, `dailyoccupancy` 8. Cleanup runs only at the close boundary (`State_Report.cpp:45`, `origin/main`), so these 8 min carried over. v42 had no after-close activity, so there is no direct comparison. `resets` is 0 in every report in both windows, so "cleared at close" can't be shown from the reports.
5. **Standard.**
   - **Resets** (S3 `status`, `watchdog`): v43 window has the boot at 15 Oct 07:03:21Z (reset 70) and 22:00:41Z (reset 30, hibernate wake, count 7); v42 has 14 Oct 22:00:37Z (reset 30, count 6). The `watchdog` folders are empty on 14, 15 and 16 Oct.
   - **`watchdogResetCount`** (S3 `status`): 46 at the v43 boot and 46 at 15 Oct 22:00:41Z (last status); no later status in the window.
   - **Heap:** report `fh`, v43 75240–89784 (post-wake max), median 76792 (61 reports); v42 75904–89776, median 77088 (43 reports). Serial `heap=`, v43 75320–89784, median 76336 (105 lines); v42 75984–77232. In the v42 range, no downward drift.
   - **Alerts** (S3 hook): v43 has 0, 31 and 44; v42 has 0 and 44. Alert 44 is not post-boot only: v43 16 Oct 05:05:27Z (ts 05:00:03) follows a GateFail at 04:53:38Z; v42 14 Oct 23:00:10Z follows a GateFail at 22:01:43Z. **Alert 31 is new vs v42:** report published 15 Oct 13:52:16Z (ts 13:47:21) after `Connect: fail elapsed=660001ms … last=CELLULAR_ACQUIRE sig=na` at 13:47:26Z. It looks like a Singtel stall; no firmware cause found either way.
   - **GateFail** (serial): one in v43 (16 Oct 04:53:38Z, `reason=ledger timeout=70000`), one in v42 (14 Oct 22:01:43Z, same text). Both follow delivery delays.
   - **Delivery:** one duplicate in v43 (ts 1792073429, published 14:21:12Z and 14:22:02Z, `LedgerDuplicateStillInflight` pattern). None lost.
   - **Config wait:** not verifiable. No config-wait log line in serial; the only config message is the OTA boot's `loading configuration from cloud` (15 Oct 07:00:56Z).
   - **Battery:** 80.26, then 80.09 at 16 Oct 05:05Z (S3 hook).

**For the architect:** (a) the 07:00Z slot decides PASS or FINDING; (b) the 8-minute carry-over past the close and the new alert 31 are deviations from the WO wording that I read as Singtel stalls and after-close PIR activity, not v43 changes, but v42 has no matching case; (c) check 6's "one config wait per day" could not be checked from serial.
