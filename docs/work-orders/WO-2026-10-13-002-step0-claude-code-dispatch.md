AGENT: Claude Code (a separate headless session: `claude -p`) · MODEL: claude-sonnet-5-5 (confirmed with a one-line probe on 2026-10-13, §5) · REASONING: high (`--effort high`)
AUTHORIZATION SCOPE:
- **Authorized:** read-only. Read, Grep and Glob, and read-only shell (`git log`, `git show`, `git diff`, `git blame`, `git grep`, `grep`, `sed -n`, `wc`, `ls`). Your final message is the report; Claude Code saves it to `docs/work-orders/WO-2026-10-13-002-step0-report.md`, then commits and pushes it on this branch.
- **Not authorized:** editing or writing files, state-changing git, builds, tests, device or network access.

# WO-2026-10-13-002 Step 0: Step 6 WO 2b, one sensor shape

**Binding spec:** `docs/work-orders/WO-2026-10-13-002-step6-2b-sensor-shape-step0.md`: the plain goal, the agreed design ("do not reopen"), and Step 0 questions 1–5. Where this dispatch and the WO differ, the WO wins; report the difference.

**Base:** branch `wo/2026-10-13-002-step6-2b-sensor-shape`, the checked-out worktree. It is WO 2a's branch (`f666d40`, dead code already removed) plus this WO. Cite file:line at HEAD. 2a is not yet on main; its closing record (`docs/work-orders/WO-2026-10-13-001-step6-2a-dead-code.md`) lists the 2b hand-off items.

## Method rules (`AI_DEVELOPMENT_WORKFLOW.md` §13)

- **Caller and reader searches use the bare name as well as the qualified one** (§13 (c)): member functions call each other unqualified, and inlined calls leave no ELF symbol.
- **Every number states its source:** the search, the file:line, or the commit (§13 (j)). No explanation without evidence.
- **Mark observation (OBS) and inference (INF).**

## Questions

Answer the WO's questions 1–5. In addition:

- **Q1:** a table, *Operation | Where today (file:line) | Covering `ISensor` method (full / partial / none) | Gap*. Include who calls each `ISensor` method today (bare-name search), and which `ISensor` methods have no caller.
- **Q2:** a table of every webhook field in **both** `publishData()` branches (occupancy and counting), in order, with its `snprintf` format, source expression (file:line), and class (sensor field / common field). Note any field whose value or format depends on mode beyond the sensor's own data (for example the `timestamp` stamping rule), because a golden test must pin it. Also say what the golden test must control to make the output deterministic (time, `millis`, heap figures, battery, counters), and how existing tests stub `publishData()`'s dependencies, if any already do.
- **Q3:** use `git log -S` / `-L` on `publishData`, `getData`, `ISensor` and `SensorFactory`. Quote the commits. If an earlier version built the payload from the sensor, say whether restoring it is smaller than the proposed path.
- **Q4:** for each proposed WO give: goal, files touched, estimated net `src/` lines (moves counted separately), the golden test that proves byte-identical JSON, and the dependencies on earlier WOs. Keep each WO ≤ +20 net. Say where the 2a leftovers (`ISensor::setup()`, `sensorDetect`, legacy `SystemConfig::get_sensorType()`, `PowerCapabilities::hasFuelGauge`, the `PowerDiagnostics` labels) fit, or why one should stay.
- **Q5:** end with **PROCEED** or **STOP**, with the reason. If STOP, say what to cut.

## Report format (your final message only: Markdown, **≤ 150 lines**, nothing before or after it)

- **Header:** "WO-2026-10-13-002 Step 0", base commit, date 2026-10-13, "Model used: <the model you actually ran as>, reasoning: high (set by `--effort high`)".
- **The plain goal.**
- **Sections Q1–Q5.**
- **"Open points".**
- **Closing:** a table *WO | Goal | Est. net `src/` | Moves | Golden test | Depends on*, then the row "Step 0 report | — | 0 | 0 | — | —".

Re-open every `src/` file:line you cite before finishing.
