# WO-2026-10-02-002: report every hour, occupied or not (v33)

**Goal, in plain language:** the device makes its scheduled report every hour whether or not the site is occupied, as in the original reporting design. That also means v32's "3 hours without a successful connection" only happens when the device actually can't connect.

**Status:** Stage 7 **VERIFIED WITH NOTES** (2026-10-02). **At USER GATE 2** (Stage 8: Chip).

**Workflow:** Per `AI_DEVELOPMENT_WORKFLOW.md`: Stage 5 → Stage 6 (Copilot) → Stage 7 (Codex), §5 model routing, §12 guardrails (plain goal, size budget, two-round rule, facts checked against their source, budget vs actual in the closing record).

**Branch:** `wo/2026-10-02-002-hourly-while-occupied`, **stacked on** `wo/2026-10-02-001-recovery-visibility` at `e8e33a7` (v32 committed and unreleased, plus the WO-2026-09-24-004 docs commit). Released together with v32 as v33. All file:line below were checked against `e8e33a7`.

**Version:** `v33-HourlyWhileOccupied`, product version 33. It includes all of v32.

## Why

The v32 work found that scheduled reports are held back while the site is occupied. In occupancy mode with `INTERMITTENT_KEEP_ALIVE`, the product's default connection mode (`connectionMode: 3`), a long occupied session therefore makes no connection. v32's failsafe would then reset the device partway through a session longer than 3 hours, losing that session's minutes.
- **Archive 14 Sep–1 Oct:** one clean continuous session of 3 h or more, PCKL3 on 27 Sep, 06:50–10:50 EDT (4.0 h, dd 7 → 247, with no report for 4 h).
- **The same hold-back** also blinds the Ubidots 3-hour loss-of-communication alert during long sessions.
- **Where it came from:** it was added with occupancy mode in `eda6b7e` ("v3.24 - Occupancy Mode", 9 Feb 2026) "so occupancy=1 is only reported on 0->1 transition".
- **Ubidots (checked by Chip):** dashboards use only `dailyoccupancy`, so `occupancy: 1` points in the middle of a session are harmless.

**Cost** (archive, 18–19 days per US device):
- About 0.7–6.4 extra reports per device per day: one per hour boundary crossed while occupied. That's +10–20% at the busiest courts.
- Each is cheap in keep-alive mode: median connect time 1 s (PCKL1 12 s, Trail02 9 s).

## Where the report is held back (checked against the source)

The dispatch cited `State_Sleep.cpp:1719–1722`; at `e8e33a7` it's at `:1746–1752`. **There are three sites, each conditioned on occupancy mode + occupied + `INTERMITTENT_KEEP_ALIVE`:**

| # | Site | Today |
|---|---|---|
| 1 | `State_Sleep.cpp:1746–1752` | Timer wake while occupied → `transitionTo(SLEEPING_STATE, "sleep-timer-occupied-suppress-report")` |
| 2 | `State_Sleep.cpp:1762–1768` | PIR wake while occupied: the opportunistic "overdue" report check is skipped (`sleep-pir-overdue-report` never runs) |
| 3 | `State_Idle.cpp:185–191` | Idle while occupied: the periodic "report interval" check is skipped |

**Why simply removing site 1 isn't enough:**
- While occupied, the sleep is cut to the occupancy debounce (`State_Sleep.cpp:1090–1099`, 300 s at the product's `setting1`), and every timer wake is a `timerWake` (`:1564`). So removing the suppression would report on every debounce wake, about 12 per hour.
- At a busy site, PIR wakes keep cutting the sleep short, so site 2 also needs to report when due, or the hourly report can be missed for long stretches.

## Change

**Stage 5 decision (Chip, 2026-10-02): the "due" rule is the clock hour.**
- **The rule:** while occupied (occupancy mode + occupied + `INTERMITTENT_KEEP_ALIVE`), a report is **due** when no report has been made yet in the current reporting interval: `lastReport == 0` or `now / interval != lastReport / interval`, with `interval = Config::reportingIntervalSecForRuntime()` and `lastReport = SystemConfig::get_lastReport()` (set by every report, `State_Report.cpp:82`).
- **The result:** the first wake after each hour boundary reports, matching the hour alignment of unoccupied reports (the timer is aligned to the interval at `State_Sleep.cpp:1058–1069`). Then nothing more until the next boundary. Occupancy-change reports in between set `lastReport` too, so they count.

**At each of the three sites, replace "skip while occupied" with "report if due while occupied":**
1. **Site 1 (timer wake):** while occupied, report (`transitionTo(REPORTING_STATE, "sleep-timer-report")`) **if due**; otherwise return to sleep as today. When unoccupied, nothing changes: the timer wake reports.
2. **Site 2 (PIR wake):** while occupied, report (`sleep-pir-overdue-report`) **if due**; otherwise continue as today (`sleep-pir-return-to-sleep`). When unoccupied, nothing changes (the existing elapsed-time overdue rule).
3. **Site 3 (Idle):** while occupied, report (`report interval`) **if due**; otherwise as today. When unoccupied, nothing changes.

**Use one shared due test** (a small inline helper, e.g. in `State_Common.h`, or one function), not three copies.

**The same cadence as unoccupied reports** (Chip, 2026-10-02, during Stage 6): reuse the unoccupied logic rather than new arithmetic, so the two line up exactly, including on 30-minute intervals (Dev-14 1800 s). Claude Code checked the source and found two cadences:
- **When an unoccupied report is made:** the timer wake is aligned in `State_Sleep.cpp:1039–1069`, using `Config::reportingIntervalSecForRuntime()` (the configured interval, no battery multiplier) and `Time.now() % interval` (epoch base).
- **Whether a report connects now or waits in the queue:** `ReportingPolicyResolver::resolveRuntime()` in `State_Report.cpp:165–270`. It produces `cadenceDue` and `nextReportEpoch` from `effectiveIntervalSec` = configured × the battery-backoff multiplier, and needs a trusted clock.

`nextReportEpoch` belongs to the **connection** cadence, so using it for the occupied due test would thin occupied reports under battery backoff and misalign them with unoccupied ones. It can't be used for report generation without changes. **So, per Chip's fallback, the due test uses exactly the unoccupied report cadence's inputs:** `Config::reportingIntervalSecForRuntime()` and `Time.now()`, on the same epoch boundaries as `:1039–1069`.

**The report doesn't end or restart the occupancy session.** The report path closes a session only at the daily close (`State_Report.cpp:57–75`). The report carries `occupancy: 1`, and the current `dailyoccupancy`, which excludes the session in progress until it ends.

**Size budget: about 8 net `src/` lines** (raised from the dispatch's "about 5" by the Stage 5 decision, because the due test is needed at three sites). Going over means stop and report.

## Unchanged (protected)

- Occupancy-change reports (`sleep-pir-occupancy-report`, `sleep-occupancy-debounce-report`).
- The daily close and close-before-sleep (v28).
- The unoccupied reporting rules at all three sites.
- Everything in v32: the failsafe, `MODEM_OFF`, the payload fields.
- `lib/`.
- The `WITH_ACK` path.

## Acceptance (Stage 7, narrow)

1. **Hourly while occupied:** a timer wake at the report time while occupied → a report with `occupancy: 1`, and the session continues (it isn't closed or restarted, `occupancyStartTime` is unchanged, and `totalOccupiedSeconds` keeps accumulating).
2. **No more than due:** debounce timer wakes and PIR wakes within the same interval, after a report, **don't** report. Continuous occupancy for 4 hours with PIR wakes every few seconds produces one scheduled report per hour boundary (plus occupancy-change reports).
3. **The v32 failsafe:** a host check that with continuous occupancy for 4 hours and a working connection, the v32 failsafe never resets (each hourly report's successful connection refreshes `lastConnection`).
4. **Nothing else changes:** unoccupied behavior at all three sites; occupancy-change reports; the daily close and close-before-sleep.
5. **Same cadence as unoccupied:**
   - the due test uses the same interval function and time base as the unoccupied wake alignment (`State_Sleep.cpp:1039–1069`);
   - for 3600 s and 1800 s intervals, occupied reports fall on exactly the boundaries an unoccupied device reports on.
6. **Mutation:** restoring the suppression at site 1 (or at site 2) fails a test.
7. **Suite and build:**
   - every `tests/*.sh` with zsh, plus every bare `tests/*.py` with python3; the `WITH_ACK` structural test green;
   - local ARM release build and linkage;
   - `v33-HourlyWhileOccupied`, product 33;
   - net `src/` lines against about 8.

## Bench (after the flash, Dev-09; Chip)

Keep Dev-09 occupied across an hour boundary by waving at the sensor regularly. **Pass:**
- a report at the hour with `occupancy: 1`;
- the session still counting afterwards;
- Ubidots' `dailyoccupancy` continuing normally.

### Bench results (Dev-09, e00fce68399ee6244a963935, 2026-10-02; read from the AWS archive by Claude Code): **PASS**

Chip held occupancy across the 13:00 SGT hour boundary. Dev-09's reporting interval is 3600 s.

| Stamp (SGT) | Published | Firmware | occupancy | dailyoccupancy | rs | cyc / slp |
|---|---|---|---|---|---|---|
| 12:39:25 | 04:40:42Z | 32 | 1 (session starts) | 193 | 4 | 5 / 4 |
| — | 04:50:30Z | update to **v33-HourlyWhileOccupied** (status reason 70) | | | | |
| **13:00:12** | **05:00:22Z** | **33** | **1** (scheduled hourly report while occupied) | 193 | 5 | 20 / 19 |
| 13:10:48 | 05:11:12Z | 33 | 0 (session ends) | **213** | 5 | 27 / 26 |

- **A report at the hour with `occupancy: 1`:** PASS (13:00:12 SGT, v33).
- **The session still counting afterwards:** PASS. `dailyoccupancy` rose by **20 minutes** at session end, which covers the session from the v33 boot (about 12:50:30) to about 13:10, straight across the 13:00 report. If the hourly report had ended or restarted the session, only about 10 minutes would have been credited.
- **`dailyoccupancy` continuing normally:** PASS (193 → 213; it never decreased). `cyc` and `slp` rose together, so the device kept sleeping.
- **Note (pre-existing, not v33):** the roughly 11 minutes before the update (12:39–12:50) were lost, because the update reset restarted the in-progress session at boot. That's the known restart limitation, the same as MAFC-1 on 30 Sep.

## Future enhancements (recorded, not now)

- **Occupied courts report more often than hourly.** A mid-session report could then include the minutes of the session in progress, so `dailyoccupancy` isn't stale during long sessions. (Ubidots dashboards use only `dailyoccupancy`, checked by Chip.)

## Approval record

- [x] Stage 5: Chip and the architect, 2026-10-02, in the opening dispatch (goal, the change, Stage 7 checks, version v33 / product 33, bench, routing: one Copilot round `claude-opus-5` medium, one narrow Stage 7 Codex `gpt-6-astra` high; not authorized: commits, flashing).
- [x] Stage 5 decision (Chip, 2026-10-02, after Claude Code's source check: three sites hold the report back, and removing only one would report every debounce wake): **the due rule is the clock hour**, applied at all three sites. Budget about 8.
- [x] Stage 6: Copilot `claude-opus-5` medium. One shared `reportDueThisInterval()` (`State_Common.h`), used at the three sites (`State_Sleep.cpp:1749–1750` timer, `:1770` PIR, `State_Idle.cpp:190`). Net **+8** `src/` lines. Suite 57/57. A 4 h simulated session gives exactly 4 scheduled reports. Both mutations caught. Release 150780 / 1090 / 2204. `loop_stage_sleep_prep_exclusion_test.py` 16 → 17 transitions. Deviations: `../Config.h` include (the repo convention); two declarations on one line, to fit the budget. Deleted only its own scratch directory. Report: `WO-2026-10-02-002-stage6-copilot-report.md`.
- [x] Stage 7: Codex `gpt-6-astra` high, **VERIFIED WITH NOTES**, with no defect.
  - **The session:** it continues across the hourly reports (14,400 s credited over 4 h).
  - **Reports:** exactly 4 in 4 h; an occupancy-change report after the boundary satisfies that interval.
  - **The v32 failsafe:** never resets with a working connection (positive control: it fires at 10,800 s when reports are suppressed).
  - **Cadence:** identical to the unoccupied boundaries for 3600 s and 1800 s, at every second offset, including the +1 s margin.
  - **Protected behavior** byte-identical or passing; mutations caught; 16 → 17 confirmed.
  - **Note:** the one-line declarations are a readability concern only (`time_t` 64-bit, no zero division, UBSan clean).
  - Suite 57/57 (29 sh via zsh, 28 py via python3). Linkage: the helper is called from both Sleep sites and inlined at Idle. Release 150780 / 1090 / 2204. Working tree byte-identical. Verdict: `WO-2026-10-02-002-stage7-verdict.md`.
- [x] Narrow edit (Claude Code, pre-authorized by Chip, 2026-10-02): `reportDueThisInterval()`'s two declarations, which were joined on one line to fit the budget, split onto two lines (`State_Common.h:36–37`). Budget 8 → 9, reason: readability, no compressed declarations. Re-verified: the `src/` and `tests/` diff differs from Stage 7's only by that split. Suite 57/57 (sh via zsh, py via python3). Release build 150780 / 1090 / 2204 (identical), `v33-HourlyWhileOccupied`, product 33. The "don't compress code to meet a budget" line was added to `AI_DEVELOPMENT_WORKFLOW.md` §12 guardrail 3.
- [x] Stage 8 (Chip, 2026-10-02): accepted. Commit v33 (with the two-line split) and push, by Claude Code on Chip's instruction; v32's bench result committed separately.
- [x] Bench on Dev-09 (occupied across the 13:00 SGT boundary): **PASS** (see Bench results).
- [ ] Gradual soak and deployment (v33 includes v32).

## Budget versus actual (closing record)

Per `AI_DEVELOPMENT_WORKFLOW.md` §12 guardrail 3. Actuals are from the Stage 7 verdict (`WO-2026-10-02-002-stage7-verdict.md`).

| Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |
|---|---|---|---|---|
| Hourly while occupied (one shared due test, three sites) | about 5 | about 8 (Stage 5 decision: three sites hold the report back, so the due test is needed at each); then **9** (Chip, 2026-10-02, readability: no compressed declarations) | +8 at Stage 7; **+9** after the narrow edit splitting the declarations | not recorded |
