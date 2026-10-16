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

Any breadcrumb-28 watchdog, a payload difference or a report missing that v42 would have sent is a FINDING and holds the fleet rollout. *(Rule corrected by the architect, 2026-10-16: the original wording, "a missing report", also caught a timer slot that falls inside a running connect attempt, which v42 skips the same way; see the Dev-14 ruling in the Result.)*

## 3. Grading

PASS or FINDING per device, with log lines, ≤ 60 lines in chat; every number names its source and window (§13 (j)). Add the result to this file and commit on a docs branch with a PR.

## 4. Fleet steps (Chip, only if both pass)

1. Set v43 as the product default (the soaked `.bin` is already product v43; recheck size and sha256).
2. Dev-09 and Dev-14 stay locked for the next soak.
3. Run the usual fleet health check about 24 h after the default changes.

## Result

**Overall verdict: PASS on both devices (architect, 2026-10-16).** Dev-09 PASS (final); Dev-14 PASS after the ruling below. v43 behaves like v42 on Dev-09 and Dev-14. The fleet steps in §4 are Chip's.

### Boron-Dev-09 (graded 2026-10-16, about 07:30Z)

**Grader:** Sonnet 5.5 subagent (`claude-sonnet-5-5`), medium effort, read-only; the controller (Claude Code, Opus) re-checked S3 for the open slot. Dev-14 is not graded here.
**Verdict: PASS (final, 2026-10-16 07:33Z).** The 07:00Z (15:00 SGT) timer slot that was open at 07:29Z arrived: S3 `Ubidots-Sensor-Hook-v1` report `timestamp` 1792134002000 (07:00:02Z), published 07:32:15Z with the queued batch after the Singtel connect stall (`cyc` 592, `slp` 591, so `cyc − slp` = 1). No breadcrumb-28 watchdog, no payload difference and no lost report.

**Sources and windows (UTC).** v43 window: 15 Oct 07:03:21Z (S3 status, reset 70) to 16 Oct 07:03:21Z. Baseline v42: 14 Oct 07:03Z to 15 Oct 07:03Z (Dev-09 booted v42 at 06:52:47Z on 14 Oct, so the baseline is a full 24 h). S3 days 14–16 Oct: `Ubidots-Sensor-Hook-v1` (155 reports), `status`, `watchdog`, `hibernate_wake`, `serialLog`. Serial is incomplete (the forwarder reconnects USB at each wake and drops lines; wall-clock stamps lag the device by 8–16 s), so intervals use device uptime ms and slots use S3 `timestamp`.

1. **Payload: PASS.** All 155 reports (S3 hook, 14–16 Oct) share one key sequence: `occupancy,dailyoccupancy,battery,vc,key1,temp,alerts,resets,connecttime,fh,lfb,cyc,slp,timestamp`. The S3 envelope order is identical for fw 41, 42 and 43. Formats are the same: `battery`, `temp`, `vc` with 2 decimals, the rest integers, `key1` a string. The v43 boot status (15 Oct 07:03:21Z) matches the v42 one (14 Oct 06:52:47Z) key for key.
2. **PIR wakes: PASS, with a connect-stall caveat.** Serial, v43 window: 362 `WakeReturn reason=PIR`, 10 TIMER; v42: 66 PIR, 4 TIMER. Every `Occ: … reason=pir-wake` was followed by `Report: occ=` 0.4–0.6 s later (device uptime ms), in both versions. Example: 16 Oct 00:57:10 start, report in S3 at 00:57:13Z. Reports that took longer than 20 s landed while a Connect (660 s budget) was already running: 15 Oct 13:42:41 (+284 s), 13:50:30 (+581 s), 13:51:37 (+515 s), 14:01:55 (+192 s); 16 Oct 06:03:57 (+97 s). v42 shows the same pattern (S3 reports 14 Oct 07:32:54, `connecttime` 342, and 08:23:35, `connecttime` 297). Not verified: the 20 s limit during a stalled Connect, because v42 has no stall of the same length to compare.
3. **Timer wakes: PASS (the 07:00Z slot arrived late).** `cyc − slp = 1` in all 155 reports (S3 hook, 14–16 Oct, both windows). Every slot from 22Z (15 Oct) through 06Z (16 Oct), and 08Z to 14Z on 15 Oct, is present. Some arrived late but delivered (S3 `published_at` vs `timestamp`): 02Z slot (ts 02:01:45), 05Z (published 05:05:27), 06Z (06:05:40), 14Z close (14:05:07, `connecttime` 293). **Was open, now closed:** the 07:00Z slot (`timestamp` near 1792134000) was not in S3 at 07:29:30Z; it arrived at 07:32:15Z (ts 1792134002000, queued delivery). Serial shows a Connect from about 07:00:03Z, CloudRecover stage 1 at 07:01:03 and stage 2 at 07:02:05, then `Connect: fail … last=CELLULAR_ACQUIRE sig=na` at 07:11:03. The two reports published at 06:46:53Z and 06:46:55Z carry `timestamp` 06:36:02Z and 06:46:50Z, so they are earlier events, not this slot. The second alert-31 report (`dailyoccupancy` 334, serial 16 Oct 07:11:20Z) also arrived in the 07:32Z batch (ts 1792134663000, `alerts` 31), as did ts 1792135323000 (alerts 31) and ts 1792135926000 (alerts 0).
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

**Architect's rulings (2026-10-16):**
- **Alert 31** is the connect timeout (`State_Connect.cpp:666`), expected after the 660 s stall; the connect code is unchanged v42 to v43. Not a finding.
- **Alert 44 after a GateFail:** the WO's "post-boot only" expectation is corrected. Not a finding.
- **Config wait:** unmeasured, accepted (v41 proved it; the code is unchanged).
- **`dailyoccupancy` 8:** pre-existing (the accrual in `State_Common.h` is unchanged from `ff1715c` to `71e346f`). Recorded in the recovery plan, item 10. It doesn't hold v43.
- **Dev-14:** graded after 11:58Z (window from 11:43:38Z on 15 Oct; baseline the 24 h ending 03:30Z on 15 Oct). Result to be added here.

### Boron-Dev-14 (graded 2026-10-16, about 12:15Z)

**Grader:** Sonnet 5.5 subagent (`claude-sonnet-5-5`), medium effort, read-only; the controller (Claude Code, Opus) re-read S3 for the one open slot.
**Grader's verdict: FINDING by the WO's original letter, one item. Architect's ruling: PASS (see the ruling at the end of this section).** S3 has no report for the 06:30Z (14:30 SGT) timer slot of 16 Oct. It has an exact twin in the v42 baseline (22:30Z, 14 Oct). Both fall inside a Singtel 660 s Connect stall, so the grader read it as not caused by v43. No breadcrumb-28 watchdog, no payload difference, no other missing slot.

**Sources and windows (UTC).** S3 14–16 Oct: `Ubidots-Sensor-Hook-v1` (158 reports), `status` (6), `hibernate_wake` (2), `watchdog` (0 files), `serialLog` (2249 lines, very sparse).
- **v43 window:** 15 Oct 11:43:38Z (S3 status, reset 70) to 16 Oct 11:43:38Z; 70 reports by `timestamp`. Serial puts the real v43 boot at about 11:42:58Z; the status was queued.
- **Baseline:** v42 from 14 Oct 06:01:15Z (S3 status, reset 70, fw 42) to 15 Oct 03:30Z, about 21.5 h and 45 reports. 14 Oct 03:30–06:01Z was v41, so it is **not a full 24 h**.
- **Labels:** reports published 11:43:11–35Z on 15 Oct carry `fw_version` 43 but are queued v42-built reports; classed by `timestamp`.
- **Outside both windows:** a Singtel outage on v42 from about 03:30Z to 11:43Z on 15 Oct ended in a failsafe reset (status 11:43:20Z, reset 140, data 2), then the OTA (status 11:43:36Z, reset 40).

1. **Payload: PASS.** All 158 reports (S3 hook, 14–16 Oct) have one key order: `occupancy,dailyoccupancy,battery,vc,key1,temp,alerts,resets,connecttime,fh,lfb,cyc,slp,timestamp`, the same formats, and the same envelope order. The v43 boot status (24 keys, 15 Oct 11:43:38Z) matches the v42 one (14 Oct 06:01:15Z); the hibernate-wake status (29 keys, 15 Oct 22:01:36Z) matches 14 Oct 22:48:34Z.
2. **PIR wakes: PASS, with limits.** S3 hook, v43: 38 off-slot occupancy reports, 33 published within 20 s of their timestamp (5–11 s); v42: 18 of 19 (5–13 s). Late ones all had a Connect running or stalled: 16 Oct 01:11:03 (440 s, alert 31), 04:16:40 (155 s), 06:11:02 (4808 s) and 06:22:02 (4150 s, delivered in the 07:31Z batch), 09:37:27 (93 s); v42 had one (14 Oct 22:41:53, 404 s, alert 31). Serial: 2 `Occ: state=1 reason=pir-wake` lines (16 Oct 01:28:37, 02:31:30), each with the S3 report 5–7 s later; one `Occ: state=0` followed by `Report: occ=` 0.44 s later by uptime (16 Oct 09:37:36). **Not verified:** per-wake serial proof for most wakes (the forwarder drops lines), and the 20 s limit during a stalled Connect.
3. **Timer slots: one missing, with a v42 twin.** `cyc − slp = 1` in all 70 v43 and 45 v42 reports (S3 hook). The bench cadence is every 30 min (:00 and :30). v43: 33 slots expected (15 Oct 12:00Z to 16 Oct 11:30Z), 32 present; **06:30Z 16 Oct is missing**. Serial shows a Connect starting 06:22:20Z (660 s budget) and a modem reset at 06:32:14Z; the next serial activity is the 07:00:14Z timer wake. S3 `timestamp`s in 05–07Z: 05:00:02, 05:30:03, 06:00:02, 06:11:02, 06:22:02, 07:00:02, 07:30:03 (controller re-read at 12:14Z). v42: 27 expected, 26 present; **22:30Z 14 Oct is missing**, inside the 22:00:50Z boot stall (S3 `timestamp`s: 22:00:50, 22:41:53, 23:00:02). Many other slots were queued late but delivered (05:30–07:30 in the 07:31Z batch, 08:00–09:30 at 09:32Z, 10:00–11:30 at 12:08Z). **Not verified:** whether the 06:30Z report was never built or was built and lost; serial has nothing between 06:33Z and 07:00Z.
4. **Daily reset: PASS.** Close report: S3 `timestamp` 1792072799000 (15 Oct 13:59:59Z), `dailyoccupancy` 206, `resets` 1, published 14:00:10Z; serial 14:00:18Z `Daily boundary reached (boundary=1792072800 … close=22)`. The next report (wake, ts 22:00:18Z) has `dailyoccupancy` 0 and `resets` 0; `resets` was 1 in every report from 06:30:14Z 15 Oct to the close, so the clear is visible. No after-close occupancy accrued. v42: close ts 1791986399000 (14 Oct 13:59:59Z), `dailyoccupancy` 81, next report (22:00:50Z) 0.
5. **Standard.**
   - **Resets** (S3 status): v43 window has reset 70 at 11:43:38Z and reset 30 (hibernate alarm, `hibernateCount` 1) at 22:01:36Z; v42 has 70 at 06:01:15Z and 30 at 22:48:34Z. `watchdog` folders are empty for 14–16 Oct. Serial shows no watchdog or breadcrumb-28 line, but coverage is sparse.
   - **`watchdogResetCount`** (S3 status): 126 at the v43 boot, 126 at the last status in the window (15 Oct 22:01:36Z), and 126 in the v42 baseline. The memory-noted expectation was 126.
   - **Heap**, report `fh` (S3 hook): v43 75968–89768 (the maximum is `cyc` 1 wake reports; 81528 without them), median 77112 (70 reports); v42 77224–89744, median 77344 (45). Serial `heap=`: v43 73064–81456, median 76372 (86 lines); v42 75736–89744. The v43 low 73064 appears only in `Connect: fail` lines during 660 s stalls, and the same 73104–73120 values appear on pre-OTA v42 in the 15 Oct outage (serial 04:11Z, 06:11Z). No downward drift.
   - **Alerts** (S3 hook): v43 has 0 (57), 31 (12) and 44 (1); v42 has 0 (43), 31 (1) and 44 (1). Alert 31 follows the 660 s `CELLULAR_ACQUIRE` stall; alert 44 (22:30:03Z 15 Oct) follows `GateFail` at 22:02:43Z. Both accepted under the Dev-09 rulings. No other alert.
   - **GateFail** (serial): one in v43 (15 Oct 22:02:43Z) and one in v42 (14 Oct 22:49:39Z), same text, both just after a hibernate wake.
   - **Connectivity** (S3 `connecttime`): v43 has 19 reports of 10 s or more (13 at 660) against 5 in v42, during the 05:41–07:31Z and 08:00–12:08Z stalls on 16 Oct. Treated as Singtel; no code link found.
   - **Config wait:** unmeasured (serial shows only `ConfigHold: ended reason=timeout elapsed=10009` on the OTA boot), accepted per the Dev-09 rulings. Battery 79.12 at the 15 Oct close, 79.61 at 16 Oct 06:00Z (S3 hook), stable.

**Question put to the architect (ruled below):** rule on the 06:30Z slot. Dev-14's v42 baseline has the same gap during the same kind of stall, so it looks like a slot that passes while a Connect is running rather than a v43 change, but the WO counts a missing report as a FINDING and holds the fleet rollout. Also note the baseline is about 21.5 h, not 24 h.

**Architect's ruling on Dev-14 (2026-10-16):** the 06:30Z gap is not a v43 finding. v42 has a twin at 22:30Z on 14 Oct under the same Singtel stall, and no counts were lost. The WO's rule is corrected to "a report missing that v42 would have sent". **v43: PASS on both devices.**

**Code check (Sonnet 5.5 subagent, medium, read-only; citations re-verified by the controller against `origin/main` 71e346f): the report is never built.**
- "Report due" is a stateless clock check, not a queued slot: `reportDueThisInterval()` (`State_Common.h:38-42`) in Sleep and Idle (`State_Sleep.cpp:1766-1776`, `State_Idle.cpp:204`, `:217`). Nothing in `State_Connect.cpp` calls it.
- `publishData` is called only from `State_Report.cpp:69` and `lastReport` is stamped at `:96`, so no report exists for a slot that passed in Connect.
- After the 660 s budget runs out, Connect raises alert 31, turns the radio off (`State_Connect.cpp:666-667`) and goes to Sleep ("connect-timeout"), which sleeps to the next interval boundary (`State_Sleep.cpp:1075-1085`: about 07:00:01Z from 06:32Z). The missed slot is never revisited.
- **Not read:** the PublishQueuePosixRK eviction code. "Built and lost" is ruled out by the call path only, and no code that drops a queued report by slot was found.

**Accepted grading limits (architect):**
- Dev-14's v42 baseline is about 21.5 h, not 24 h (v41 until 06:01Z on 14 Oct).
- Serial is sparse, so per-wake PIR proof and the 20 s limit during a stalled connect are unverified on both devices.
- The one-config-wait-per-day check is unmeasured (v41 proved it; the code is unchanged).
- The S3 `fw_version` label on queued v42-built reports published 11:43Z on 15 Oct is wrong; reports are classed by `timestamp`.
- Dev-09's `dailyoccupancy` 8 after the close is pre-existing (recovery plan item 10).
