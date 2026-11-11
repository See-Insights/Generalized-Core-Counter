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
