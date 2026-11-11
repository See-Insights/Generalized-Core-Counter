# Alert reporting: Stage 2 evidence (11 Nov 2026)

**Governing roles:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 2. Input: `docs/handover/2026-11-11-alert-reporting-handover.md` with Chip's §8 rulings. **Code under review:** `origin/main` `f00aa04` (PR #222). Read-only; no code change is made or authorized here. Author: Claude Code (Sonnet 5.5). Not run: any Ubidots or Slack read (Chip's), any bench or hardware step.

## 1. What was done

`docs/reference/alert-codes.md` is re-cited to `f00aa04`, every row, by statement content. A script then checked 26 raise sites (file, line, code) and 12 clear sites against the source: all match (G:614 is the `if`, the clear is inside the range G:614–618). Two sections were added to the reference: the report field list and a masking section. The "Differences" section keeps its old line numbers and is marked historical.

## 2. Findings from the re-cite (new against the handover and the old reference)

1. **Raise sites moved.** Code 15's two raises are in `state/SleepTeardown.cpp:201` and `:321` (they were in `State_Sleep.cpp`). Every other raise is in the file the reference named, at a new line.
2. **Clear paths, code by code, at `f00aa04`:**

| Code | Clear path | Notes |
|---|---|---|
| 14 | boot after reset-driven recovery (G:614–618) | not raised any more |
| 15, 31, 41, 43, 44 | after an accepted report (RP:143–152; list RP:25–37) | 15, 16, 31 also cleared in non-CONNECTED modes (E:136–140); 31 also on connect (C:518–520); 44 also after the boot status (G:929–933) |
| 16 | next boot (G:591–595) | |
| 17 | after a clean connect with config and ledger OK (C:560–563) | written with `set_alertCode`, bypassing severity (G:554) |
| 18 | **none** | raised T:141 (tier 2, no reset) and T:150 (tier 3, then reset). Not in the RP:25–37 list |
| 19, 42 | after an accepted report | 42 is also raised at the sleep gate for OTA pending |
| 20, 21, 22, 23 | PMIC monitor (P:179–181, P:376–379, P:384–388) | 21 also clears when charge status leaves 2 |
| 40 | any non-empty webhook response (RP:299–302) | |
| 45 | connect (C:493 → F:134–152) | not raised |
| 30, 32 | n/a | not raised |

3. **Only 18 has no clear site** among the codes the firmware raises. At tier 2 it is raised with no reset, so it sits in the persisted code (sev 3) and blocks every equal or lower raise until 19 or 17 overwrites it.
4. **The report has two field orders** (RP:104–117): occupancy mode `alerts` then `resets`; counting mode `resets` then `alerts`. Both carry `battery`, `key1`, `alerts`, `resets`, `connecttime`. `lastAlertTime` is not in the report (status event only, G:1297, written G:1360 and G:1389).
5. **`lastAlertTime` is not a pure onset time.** (a) It is stamped only when `raiseAlert()` replaces the code (M:829); a same-severity re-raise leaves it. (b) State_Report reuses it as the alert-40 escalation timestamp (R:144–149: `lastEscalation == 0` means "cooldown passed"), so a write there changes the value an X1 field would show. (c) Alert 17's path stamps it separately (G:560). (d) It is zeroed by 11 clear sites (G:594, 617, 932; RP:151, 301; F:152; P:181, 379, 388; C:563; E:139). X1 as "put `lastAlertTime` in the report" therefore reports the last escalation for alert 40, not the onset.
6. **P1 already has tests:** `tests/report_payload_golden_test.{sh,cpp}` (renders the real format string per mode over alert values -128..127) and `tests/report_payload_fields_test.py` (extracts both format strings, checks numbers are unquoted). Whether either fails on a rename of `battery`, `key1`, `alerts`, `resets` or `connecttime` is not established; that is a Stage 4 question.
7. **Budget cross-check (src/):** the P5 clear for 18 would be one entry in the RP:25–37 list (1 line), if the ruling "report once, then clear, like 19 and 42" is taken literally. See the open question in §4.

## 3. ChurchSt-1a: 7 Nov code 20 and 10 Nov code 21 (the `vc` charging check)

Source: S3 `Ubidots-Sensor-Hook-v1` and `status` events, device `e00fce686f8cb2133cd3f8ed`, 6 to 11 Nov, read-only. The S3 archive does not hold the legacy alert hook, counting-webhook-v1 or the Slack post.

- **7 Nov, code 20.** Not seen in GCC data. All 79 report hooks on 7 Nov and all 44 on 8 Nov carry `alerts` 0; so do 6 and 9 Nov. The first GCC boot's status event (12:57:06Z, `v49.1-DurableResets`, reset 70) has `alert` 0 and `lastAlert` 0. First GCC report 12:55Z. After it, `Charging` from 12:07Z to 22:01Z with `vc` 4.04 to 4.21 V, temperature 14 to 22 °C (no thermal signature), then `Not Charging`. **Reading (inference):** the E30-20 post was the legacy firmware's own code 20 ("firmware update completed", the handover's legacy meaning) published on the legacy alert hook around the migration flash, not a GCC PMIC thermal fault. **Chip can confirm from the Slack post time: before 12:55Z supports this; after 12:57Z would mean GCC raised 20 and the report never carried it.**
- **Side finding, 7 Nov.** The legacy firmware reported `battery` 64 to 66 from the start of the archive's data (6 Nov 20:00Z) to 7 Nov 12:18Z (74 at 12:48Z, the last legacy hook), while the first GCC report read 76.18 with `vc` 4.07. If legacy devices sit near 65, E11 (battery ≤ 65 % for 3 days, a Freshdesk ticket) can fire on a gauge difference at migration. Not checked on other devices.
- **10 Nov, code 21.** Charging 14:00Z to 22:00Z; battery 89.12 flat from 15:00Z to 21:00Z while `Charging`; `vc` 4.08 to 4.16 V; alert 21 on the hooks at 20:59Z and 21:00Z and 22:00Z, cleared 22:06Z when the state went to `Not Charging`. This is the fast-charge-6-h-no-gain raise (P:549), a stale-gauge or full-pack false positive (recovery plan item 23, solar case), not a latch (item 22: `vc` would be low and flat). Recorded in the migration WO.

## 4. Open questions for the architect and Chip

1. **P5 for 18.** Raise sites are tier 2 (a live device, no reset, then forced sleep) and tier 3 (reset). "Report once, then clear, like 19 and 42" clears after the first accepted report, which for tier 2 is the hourly report after the forced sleep; the thrash can recur and re-raise. Confirm that is the intent (Stage 4 asks Codex for the exact consequences).
2. **P1 scope.** If the existing golden and fields tests already pin the five names, the "new test" in P1 may be a small addition or none. Stage 4 answers this.
3. **X1 form.** `lastAlertTime` is ambiguous for 40 (see §2.5). A new field set at raise time only is the alternative; Stage 4 asks for the smallest correct form and its `src/` cost.
4. **E11 and the legacy gauge** (§3). Chip's call whether it needs a migration-checklist step.
