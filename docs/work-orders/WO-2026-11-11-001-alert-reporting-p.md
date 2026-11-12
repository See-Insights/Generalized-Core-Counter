# WO-2026-11-11-001: Alert reporting P1–P5 — every GCC code has a clear path and the right Slack outcome

**Author:** architect, 11 Nov 2026 · **Status:** READY to dispatch; **merge held until RC1 is tagged** (§8 ruling 4: P lands before the counter conversion, not in RC1)
**Branch:** `wo/2026-11-11-001-alert-reporting-p` from the then-current `main`
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5–8; §4 (the SHA rule; at most two Stage 7 rounds, then an architect ruling) · **Agents:** Stage 6 **Copilot** (standard model, medium effort; five small edits plus tests); Stage 7 **Codex `gpt-5.6-sol` at high** (alert lifecycle, not time logic, so not xhigh)
**Budget:** `src/` **≤ +10 net** (estimate +2; the handover allowed 40). Tests are not capped but stay proportionate. If the budget is exceeded, stop and report; don't trim style to fit.
**Inputs:** `docs/handover/2026-11-11-alert-reporting-handover.md` (§8 rulings, §9); the Stage 4 Codex verdict on `aa418d1` (archived, PR #225); `docs/reference/alert-codes.md` at `f00aa04`; the Ubidots checklist `docs/handover/2026-11-11-ubidots-gcc-devices-checklist.md` (Chip's half of P2/P3).

## Plain goal
A GCC alert reaches Slack only when someone needs to visit or act. Every code clears so that it can't hide a later one, and the cloud sees each code at least once in a scheduled report. The Ubidots side is Chip's checklist. This WO is the firmware half.

## The change (lines at `f00aa04`; Step 0 re-cites them on the dispatch base)
1. **18 reports once, then clears (P5; §8 ruling 5, tier 2 included).** Add `case 18:` to `isAutoClearAfterReportAlert()` (`cloud/ReportPublisher.cpp:25–37`). +1.
2. **17 reports once, then clears (P3; Codex Stage 4 inference).** Today 17 is cleared on the first good connect (`state/State_Connect.cpp:560–564`). That can run before any scheduled report carries it, so the Ubidots "boot storm" event (which watches the report's `alerts`) can miss it. **Delete that clear and add `case 17:`** to the same list. The comment there already calls 17 "a one-shot incident marker", which report-once matches. About −5/+1. Step 0 confirms that no other code reads "17 cleared at connect".
3. **PMIC 20 and 23 clear on their own predicates (P5; Codex Stage 4, confirmed by Claude Code).** The healthy clear at `power/PmicFaultMonitor.cpp:363` runs only when `consecutiveFaults > 0`. So a 20 raised by an NTC fault alone (P:201), or a 23 raised by BAT_FAULT alone (P:416), never clears. Add the two predicate lines at P:383–389: clear 20 when there's no NTC fault, and 23 when BAT_FAULT is absent. +2.
4. **Alert 21 doesn't fire on a full pack (architect ruling, 11 Nov).** The "fast charge ≥ 6 h without material gain" trigger (P:549) is skipped when cell voltage `vc ≥ 4.05 V`. Evidence: ChurchSt-1's 10 Nov false positive at `vc` 4.08–4.16 V. Use a named constant. About +2.
5. **No P1 change.** The golden test already catches renaming, quoting or dropping any of the five names in both modes. **Add:** pin all seven `key1` strings (about 4 vectors).
6. **P4: confirm only.** Show that the ERROR_STATE actions for 15, 16, 31 and 40 are unchanged.

## Step 0 (read-only, inside Stage 6; stop and report if an answer breaks the plan)
- Re-cite items 1–4 on the dispatch base. Also report whether `integration/v50-ab1805` (`1d1d5af`) changes any of those lines. Claude Code said it doesn't; confirm it at dispatch.
- Item 2: list every reader of alert 17 and of `lastAlertTime` around the connect path.
- Item 4: name where `vc` is available at P:549, and confirm its units.

## Tests (host; the real functions)
- 17 and 18: each stays set until a report is queued, then clears. A re-raise after the clear reports again. A tier-3 reset path still delivers 18 in the first report after reboot. **Rewrite** the existing test that requires 18 to stay set.
- 20 from NTC only clears when NTC clears; 23 from BAT_FAULT only clears when BAT_FAULT clears; the existing clear tests still pass.
- 21: `vc` 4.04 fires, 4.05 does not, 4.16 does not.
- **Mirror fix:** `watchdog_alert_code_test.cpp`'s severity-tier-3 list omits 22. Make it match the source table.
- **Mutations (each must fail):** drop `case 18`; drop `case 17` with the connect clear deleted; restore the connect clear; drop either PMIC predicate; `>` in place of `>=` at 4.05 V; drop one `key1` vector.
- Update `alert-codes.md` in the same PR (17 and 18 become report-once; the clears for 20 and 23; the 21 exclusion).

## Bound rulings for later items (recorded now so the X1 WO doesn't reopen them)
- **X1 (after RC1's soak):** approved as **`ats`**, a 4-character key (`alertAt` doesn't fit the 18 spare bytes), reusing `reservedTime0` (formerly `verboseModeStartTime`) as the onset time, with no layout change. Three rules keep it simple:
  - it is stamped only on a trusted clock, and is otherwise 0;
  - it is zeroed when the code clears;
  - it is **reported only while `alerts` ≠ 0**.

  So stale bytes in the reused slot can show only once, on an upgrade with an alert already active. That's a documented limit. Budget: 7 lines.
- **No separate clear report.** The next scheduled report showing `alerts` 0 is the clear.

## Closing record
Record budget against actual for each item, any increase with its reason, and the Stage 7 SHA and verdict.

## Amendment 1 (architect, 11 Nov): Stage 7 round 1 findings, fixed in one last round (round 2 of 2)
Stage 7 round 1 (`007504c`, `docs/work-orders/WO-2026-11-11-001-stage7-r1-verdict.md`) was NOT VERIFIED: the change passed items 1, 3 and 5-8; two PMIC alert-20 lifecycle findings (item 2) came back. Ruling: fix both, in one last round.

**Goal: a PMIC code clears when its condition is gone, never before.**
- **2a (High).** The new "clear 20 when no NTC fault" line (`PmicFaultMonitor.cpp:390-393`) also requires `consecutiveFaults == 0`, so it cannot run during an unfinished remediation cycle. About +1 `src/` line.
- **2b (Medium; older behaviour, in scope).** The broad "charging resumed" clear for 20-23 (`:363-379`) does not clear 20 while `ntcFaultCode != 0`. About +1 `src/` line.
- **Tests (two real-function scenarios):** (a) a thermal 0x02 fault raises 20; 20 stays through the first clean poll and clears only after the cycle completes; (b) a combined charge and NTC fault raises 20; 20 stays on the next poll that shows only the NTC fault, and clears once NTC clears.
- **Mutation:** removing either guard must fail one of the two scenarios.
- **Budget:** `src/` at most **+2 on top of round 1** (net about +1 against `f00aa04`). If either guard needs more than 2 lines, stop and report.
- **Stage 6 round 2:** Copilot (standard model, medium), only the two guards and the two scenarios. **Stage 7 round 2:** Codex `gpt-5.6-sol` at high, the SHA in the dispatch header; the last round.
- **New findings in round 2:** anything outside these two lines is recorded as a known gap in `alert-codes.md` and in this WO's closing record. There is no round 3.
- The merge stays held until RC1 is tagged.

## Closing record (Claude Code, 11 Nov)
- **Branch and commits:** `wo/2026-11-11-001-alert-reporting-p`; round 1 `007504c` (Stage 7 round 1 NOT VERIFIED, `...-stage7-r1-verdict.md`), round 2 `467eea7` (Amendment 1), Stage 7 round 2 **PASS** on `467eea70e9e9ff9572c041305d9168fbc8e528e2` (`...-stage7-r2-verdict.md`, Codex `gpt-5.6-sol` high; the verdict's first line quotes the full SHA). Suite 118/118 (sh via zsh, py via python3), controller-run on both commits. **Merge held until RC1 is tagged.**
- **Budget against actual (`src/`, `git diff --numstat f00aa04`):** total **+12/−13, net −1** against the ≤ +10 budget and the +2 estimate (round 2 itself net 0, inside its +2). By file: `cloud/ReportPublisher.cpp` +2 (items 1 and 2, `case 18` and `case 17`); `state/State_Connect.cpp` −10 (item 2: the connect clear for 17 and the write-only `ledgerPublishOk`; the WO estimated about −5); `power/PmicFaultMonitor.cpp` +9/−3 (item 3's two-predicate block, item 4's full-pack skip, and the two Amendment 1 guards, which are edits of existing lines); `power/PmicFaultMonitor.h` +1 (item 4's constant `kPmicFullPackVcell`). Item 3 and item 4 exceeded their estimates (+2 and +2) because the predicate is one block and the full-pack skip joins the ambiguous-load branch (a warning and a 6 h window restart); the total stayed negative because of item 2. Build (Device OS 6.6.0): text 153108 → 153172, data 1114, bss 2240.
- **Increases with reasons:** none over budget. Deviations recorded in Copilot's reports: `ledgerPublishOk` removed; the alert-21 skip restarts the 6 h window; the 23 predicate not widened (BAT_FAULT never touches `consecutiveFaults`).
- **Fixed after Stage 7 (architect ruling 12 Nov, VERIFIED WITH FIXES path; no Codex round 3):** (1) the `uint8_t` wrap: `PmicFaultMonitor.cpp:208` is now `if (consecutiveFaults < UINT8_MAX) consecutiveFaults++;` (Copilot, standard model, low; `src/` +1/−1, net 0 against a +1 budget; total against `f00aa04` is now +13/−14, net −1); new scenario 16 holds alert 20 for 509 polls of a frozen cycle (3 + 509 = 512 would wrap), and removing the saturation fails it. (2) the test gap: scenario 15 is parameterized over NTC codes `0x01`, `0x05` and `0x07` (controller edit under the test-only pre-authorization, about 20 lines), and a guard narrowed to `ntcFaultCode == 1` now fails. Build text 153172 → 153180 (+8), data 1114, bss 2240 (Copilot). Mutations re-run by the controller: no saturation, guard 2b narrowed to `== 1`, guard 2a without the NTC term: all caught; Copilot's table covers the rest.
- **Known gap (recorded, historical):** `CHANGELOG.md:116` still says 18 "stays sticky" (a historical entry).
- **Follow-ups outside this WO:** X1 (`ats`) after RC1's soak; the Ubidots checklist `docs/handover/2026-11-11-ubidots-gcc-devices-checklist.md` (Chip).

