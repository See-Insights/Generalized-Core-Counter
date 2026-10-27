# v47 release and bench soak (the persistence freeze, the failsafe fix, the debounce sleep)

**Author:** Claude Code, 27 Oct 2026, from Chip's rulings (26 Oct) and the v46 WO (`2026-10-25-v46-release-and-soak.md`); the architect may amend · **Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §2, §3 ("Release gate: the Particle cloud compile"), §13 (g), (h), (i), (j)
**Agents:** release PR, Claude Code itself; soak and baseline runs, a **Sonnet subagent, medium**, read-only. Report the model used.
**Budget:** 0 `src/` lines beyond the version string.
**Start:** v46 is the product default (27 Oct ~00:09Z) and #142, #151 and #155 are merged. **Chip is travelling and has no bench access until 31 Oct: every step is remote** (S3, the Particle API and console, OTA).

## Plain goal

Ship #142 (Step 7 P7-A: the stored layout frozen, dead accessors removed, no behavior change), #151 (recovery item 18: the connectivity failsafe acts when openness is Unknown) and #155 (recovery item 20: an occupied device sleeps the time left in the debounce) as **v47-PersistFreeze** on Device OS 6.6.0. **Item 20's fix is the one this soak must measure on the bench:** on v45 and v46 a session that spans a report is credited 12-13 min instead of 9.5; on v47 it should be credited like any other (9 or 10 min, about 570 s). #151's night case (an Unknown clock may act only 22:00-24:00 local) is not bench-testable remotely; its bench test is deferred to after 31 Oct (recovery item 18).

## 1. Release PR (done: #158, branch `release/v47`, release commit `f624c15`)

- `src/` differs from v46 (`75f1cc9`) only by #142, #151, #155 (8 files) and the version string. Release commit: `FirmwareVersion.h`, `CHANGELOG.md`, `README.md`, `Doxyfile`.
- **Local build (§3, mandatory per change):** Boron, Device OS 6.6.0, fresh `BUILD_PATH_BASE`, `-frandom-seed=$$@`: `app.bin` **152,610 B** (v46 rebuilt the same way 152,690 B, −80 B), **sha256 `a1c14ba1b5f4ee0423185d7bc9aa5dd518eb78b57e5ca0e15ab0425fcba61a13`**, text/data/bss 151492/1114/2200, `.backup` 152 B identical with no retained symbol differing; `strings`: `v47-PersistFreeze`; `particle binary inspect`: CRC ok (b275746e), firmware version 47, system module 6600. The file: `/Users/chipmc/Downloads/v47-PersistFreeze-boron.bin`. (A rebuild of v46's source here gave the same size but a different sha256 than v46's recorded one: the hash is not reproducible across builds, so upload this file and compare Particle's size and sha256 with the numbers above, §13 (j).)
- **Suite:** 106/106 on the release head, serial (sh via zsh, py via python3).
- **Release gate, Particle cloud compile (§3): not completed.** `particle compile boron . --target 6.6.0` failed on this head after about 2 minutes ("Compiler timed out or encountered an error", no diagnostics), as it did on 25-26 Oct for WO-2026-10-25-001's candidate and the unmodified v46 base. **Per the gate: recorded; release from the verified local build.** Retry before the upload; if it passes, add its flash and RAM here.

## 2. Roles, the interval, and the plan (decision needed from Chip: lock timing)

- **Dev-09** (e00fce68399ee6244a963935) is the **item 20 test device.** **Chip set its `timing.reportingIntervalSec` to 1800 on 27 Oct at about 01:00Z while it runs v46** (reports at :00 and :30; Device ledger `device-settings`, `timing.reportingIntervalSec`; the serial line is `Config: Reporting interval -> 1800s`). Its sessions currently start at :00:35, :20:36 and :40:36 and close at :10:06, :30:26 and :50:05, so each **:20 session spans the :30 report**. That makes today's v46 data the **control**: a v46 day with spanning sessions at the same interval and phase as the v47 test. The interval stays 1800 for the whole soak and is restored (3600, `Config.h`) by Chip when the soak ends.
- **Dev-14** (e00fce688e592afaf23ac4fb) is the **bad-antenna stress case until 31 Oct** (Dev-09's old antenna): judge its sleep path, payload, serial `session=` values and report delivery; exclude its connect metrics (`connecttime`, `ConnSummary`, alerts 31/40/15 from failed connects). A failsafe stage-2 or stage-3 reset on Dev-14 after the 22:00 SGT close (14:00-16:00Z) is the **designed #151 behavior** (record it; not a failure). A gap of 3 h or more outside the overnight close is a possible item-18 recurrence: record it and change nothing. Its breadcrumb-28 events stay on the recovery item 14 watch.
- **Is the control valid? (check before locking):** the Tuesday 27 Oct 14:33Z run (job `227cf4fa`) reports Dev-09's report-spanning sessions on v46 at 1800. **The control is valid if at least 8 sessions span a :30 report and credit 12 or more minutes.** Whether a :20 session spans the :30 report depends on the day's exerciser phase (it needs the last pulse at :25:02 or later, i.e. a start at :20:32 or later; today's :20:36 start has only about 5 s of margin), so categorize every session by observed overlap (occ=1 before and occ=0 after a report), never by clock phase. If the control fails, stop and ask the architect (a different interval is the fallback); do not lock.
- **Plan B (recommended, if the control is valid): lock both devices to v47 after the Tuesday run.** Chip uploads v47 and locks Dev-09 and Dev-14 on Tue 27 Oct after 14:33Z; the OTA lands at the nightly 22:00Z wake (the flash completes within the hour, as on 25 Oct); Wed 28 Oct SGT is a mixed day; **Thu 29 Oct SGT is the first full v47 day** (the Dev-09 v47 days Thu and Fri 30 against its v46@1800 control on Tue). Decision Fri 30 Oct, before Chip is back.
- **Plan A (if Chip prefers a full control day):** keep Dev-09 on v46 at 1800 through Wed 28 Oct SGT, lock after the Wed 14:33Z run, first full v47 day Fri 30 Oct, decision Sat 31 Oct.
- **Default and fleet:** v47 is **not** set as the product default during the soak; Dev-09 and Dev-14 only (the Dev-09 and Dev-14 lock, KEEP_ALIVE, no other config changes).

## 3. Checks, per device (as the v46 soak §2)

1. **Payload unchanged** (same keys and order as v46); PIR and timer wakes; the daily reset (what the new day starts from); standard checks (resets, `watchdogResetCount`, `fh`, alerts, GateFail); "missing" = a report v46 would have sent (the 30-min interval doubles the report count on Dev-09: compare against the 1800 control, not against hourly).
2. **First boot and P7-A:** reset 70 once (the OTA) and nothing else; `dailyoccupancy` continuity across the OTA (the value before and after; the bench step of P7-A already passed); the stored values read back unchanged (no one-time reset, no layout change).
3. **Sleep returns:** every cycle sleeps (hook `cyc` − `slp` = 1); no breadcrumb-28 watchdog; reset types as v46 (30 hibernate wake; 70 once); no all-modes-failed reset. `connecttime`, alert 15 and `slow_teardown` against v46.
4. **Failsafe (#151):** `failsafeStage` and `failsafeCount` in status; counts of serial `Failsafe:` lines (stage 2 `system-reset`, stage 3 `deep-powerdown`), if captured. **Counts only; no pass/fail.** Record the clock state (trusted or not) at any stage-2 reset.
5. **Counts only (serial, a lower bound):** STOP fallbacks, the hibernate bypass, the nightly heap guard, the all-modes-failed reset.

Serial is sparse by design (the bench Pi forwarder captures reconnects only); ThrashGuard counters and `MODEM_*` lines are read if captured, not a pass condition. The pass evidence is S3 (`status`, `watchdog`, `hibernate_wake`, `Ubidots-Sensor-Hook-v1`).

## 4. The item 20 test (Dev-09): pass criteria

Per session category (normal / **report-spanning** = occ=1 before and occ=0 after a report / close-cut / orphan / delayed-connect listed separately), count, credit, mean, error against 9.5, and start-to-end from hooks and serial `session=`.
- **Control (v46 at 1800, 27 Oct from the first spanning session):** report-spanning sessions credit 12-13 min and run about 730-810 s (as Saturday's ten on v45).
- **PASS on v47:** the report-spanning sessions credit **9 or 10 min and run 570 s ± a few seconds** (the range of normal sessions: hooks 567-594 s, serial 568-595 s), with **at least 8 spanning sessions on the v47 days**; normal sessions stay within the v45/v46 spread (v45 Sat 9.58, Sun 9.54; v46 Mon 9.59); any v47 report-spanning session at 12 or more minutes is a FINDING and holds the rollout.
- Any breadcrumb-28 watchdog on Dev-09, a payload difference, a missing report not explained by a failed connect, or a normal-session error outside the v45/v46 spread is a FINDING and holds the rollout. Dev-14 is judged on its sleep path.
- **Grading:** 24 h from Dev-09's first v47 boot (status reset 70, `v47-PersistFreeze`), against its own v46@1800 control; the baseline script's per-category report and the hook increments are the evidence; read the first v47 boot from S3 `status` (the hook `fw_version` is stamped at publish: v46-generated hooks delivered after the reboot carry fw 47; exclude the session interrupted by the OTA).

## 5. Default (if both pass)

Set v47 as the product default (recheck size and sha256 against Particle); merge #158; the fleet health check about 24 h later (the v46 check's format, `2026-10-28-v46-fleet-health-check` or the v44 WO as the model). Dev-09 and Dev-14 stay locked for the next release. Restore Dev-09's `timing.reportingIntervalSec` to its normal value (Chip, after the soak).

## 6. Schedule (Plan B; SGT = UTC+8; to be confirmed by Chip's lock time)

| When | Step | Who |
|---|---|---|
| Tue 27 Oct 14:33Z | v46 day 2 and the **control check** (job `227cf4fa`): is the control valid? | Claude Code |
| after that run | Upload v47 (`/Users/chipmc/Downloads/v47-PersistFreeze-boron.bin`, 152,610 B, sha256 `a1c14ba1…ba61a13`), lock Dev-09 and Dev-14 to v47, **do not set it as the default** | **Chip** |
| Tue 27 Oct ~22:00-23:00Z | The OTA at the nightly wake (reset 70, `v47-PersistFreeze`) | automatic |
| Wed 28 Oct 00:40Z | v46 fleet health check (job `9b8a67aa`) | Claude Code |
| Wed 28 Oct 14:33Z | v47 day 1 (mixed day for Dev-09, SGT 28 Oct) and the first-boot check | Claude Code |
| Wed 28 Oct ~23:20Z | 24 h grading from Dev-09's first v47 boot (Dev-14 on its sleep path) | Claude Code (Sonnet subagent) |
| Thu 29 Oct 14:33Z | v47 day 2 (the first full v47 day) and the item 20 comparison against the v46 control | Claude Code |
| Fri 30 Oct | **Decision** on v47 as the product default and the release PR | **Chip** |

The jobs are session-only (Claude Code's session); if it ends, paste the relays at the stated time. All reads need `AWS_PROFILE=particle-admin`; an expired SSO token stops a job with "token needed". The relays follow the v46 WO §5 (the baseline script call `--dates <SGT day> --tz SGT-8 --open 6 --close 22 --debounce-ms 300000`, per-category report, Dev-14 rules), with the categories of §4.

## Result

(to be filled in by the grader)
