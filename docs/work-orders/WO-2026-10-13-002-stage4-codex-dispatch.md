AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-13, §5) · REASONING: high
AUTHORIZATION SCOPE:
- **Authorized:** read-only review of this worktree (branch `wo/2026-10-13-002-step6-2b-sensor-shape`): read files, search, and run `git log`/`show`/`diff`/`blame`/`grep`. Your final message is the verdict; Claude Code saves it as `docs/work-orders/WO-2026-10-13-002-stage4-verdict.md`.
- **Not authorized:** editing any file, builds, tests, commits, pushes, stash, reset or checkout, and device, network or AWS access.

# Stage 4 dispatch: WO-2026-10-13-002, an independent check of the proposed 2b sequence

**Goal of 2b, in plain language:** every sensor fits one small shape, and the hourly report is built from what the sensors supply, so a new sensor can be added without touching the report, sleep or connect code. **Today's PIR occupancy and counting webhook JSON stays byte-for-byte identical.**

**Binding spec:** `docs/work-orders/WO-2026-10-13-002-step6-2b-sensor-shape-step0.md`. The section "The agreed design" is settled; **don't reopen it**.

**Under review:** `docs/work-orders/WO-2026-10-13-002-step0-report.md`, the Step 0 report proposing WOs 2b-1 to 2b-6. Treat every claim in it as unverified. The architect will accept, change or cut the sequence based on your check.

## Method rules (`AI_DEVELOPMENT_WORKFLOW.md` §13)

- **Searches use the bare name as well as the qualified one** (§13 (c)).
- **Every number states its source** (file:line, search, or commit), and there's no explanation without evidence (§13 (j)).
- **Mark observation (OBS) and inference (INF).**

## Checks (each one: AGREE / DISAGREE / CONCERN, with evidence)

1. **Facts.** Re-check the report's Q1 and Q2 tables at HEAD: the `ISensor` callers and the methods with no caller; every payload field, its format, its source and its order in both branches. That includes the `alerts`/`resets` swap, MEASUREMENT taking the counting branch, the timestamp rules, and the SOC and `battState` sanitising. List anything wrong or missing.
2. **History (Q3).** Confirm or refute "never built from the sensor; nothing to restore".
3. **Behavior-neutral.** For each of 2b-1 to 2b-6, can it really be byte-identical and behavior-neutral? Name any path that could change output or timing, for example:
   - the sensor-null case (a closing report when the device booted closed, with no `_sensor`);
   - MEASUREMENT mode;
   - `stampOverride`;
   - the daily-close report;
   - a `publishData()` call site other than `State_Report.cpp:69`;
   - order of evaluation or rounding in `snprintf` once the formats are split into fragments (2b-3);
   - `PIRSensor` vtable or ISR changes (2b-6a);
   - `sensorDetect` branches (2b-6b).
4. **Golden test (2b-1).** Is the extract-verbatim approach sound and deterministic? Check the list of inputs it must control against the code; anything missing is a hole. Does extracting `:1973–2051` capture every byte that reaches the publish, including the queueing that follows? Is the `alert19_clears_after_report_test.sh` pattern the right model?
5. **Sizes.** Is each estimate (+15, +18, +10, +14, −53) plausible from the code? Flag any WO likely to exceed +20 net, and say how to split it. Are the move counts (about 75 and 14) realistic?
6. **Order and dependencies.** Is the order right? Could 2b-4 or 2b-6 go first? Does anything depend on 2a being merged to main first (2a is PR #94, held until v42 is uploaded)?
7. **The 2a leftovers (2b-6a–d) and the "stay" for legacy `get_sensorType()`.** Agree or not. Check the claim that `tests/sensor_type_source_test.sh:114`/`:120` and `tests/persistence_facade_behavior_test.cpp:238` depend on it.
8. **What the report cut** (settings from `setting1–4` and `samplingMode`; "ask for a report" as a sensor method). Is deferring each consistent with the WO's goal and the agreed design? Does any cut leave the goal ("a new sensor without touching report, sleep or connect code") unmet? If so, say which WO would close the gap.
9. **The six open points.** Give a recommendation for each.

## Verdict (your final message only)

- **Header:** one of SEQUENCE SOUND / SOUND WITH CHANGES / NOT SOUND; the model and reasoning used.
- **A table of checks 1–9.**
- **Findings,** with file:line and severity.
- **Your recommended sequence:** the same table shape as the report's closing table, with any changes marked.

Limit: 120 lines.
