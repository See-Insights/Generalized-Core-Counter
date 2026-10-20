AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-15, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- Read-only investigation of this worktree (branch `wo/2026-10-15-003-step6-power-owner-split`, from main `71e346f`).
- Read files, search, and run `git log`/`show`/`diff`/`blame`/`grep`, `wc`, `sed -n`.
- Your final message is the report. Claude Code saves it as `docs/work-orders/WO-2026-10-15-003-step0-report.md`, checks every file:line in it, then commits and pushes it.

**Not authorized:**
- Editing or writing any file.
- Builds or tests.
- Commits, pushes, stash, reset or checkout.
- Device, network or AWS access.

# WO-2026-10-15-003 Step 0: Step 6, the power-owner split

**Binding spec:** `docs/work-orders/WO-2026-10-15-003-step6-power-owner-split-step0.md`. That includes its questions 1–7, its standing rulings and its naming note: "Step 7" is the `SysData`/`CurrentData` layout change, which is out of scope here. Where this dispatch and the WO differ, the WO wins; report the difference.

**Start from:**
- `docs/work-orders/2026-10-06-step6-ownership-codex-report.md`;
- `docs/work-orders/2026-10-07-step6-extensibility-evidence.md`.

Update what changed since 6 Oct (WO 2a #94; 2b-1–2b-6 #97–#101, #103; v41–v43); don't redo it.

**Other sources:**
- `docs/RECOVERY_PLAN_2026-09-26.md`, sections "Step 6: the power-owner split" and "Connectivity", plus queued item 3 (the modem standby latch). Its 15 Oct field evidence is on the open branch `docs/2026-10-15-dev14-sleep-loop-observation` (PR #105); read it with `git show origin/docs/2026-10-15-dev14-sleep-loop-observation:docs/RECOVERY_PLAN_2026-09-26.md`.
- `docs/architecture-review-2026-09-03.md`.

## Method rules (`AI_DEVELOPMENT_WORKFLOW.md` §12.1, §13)

- **History first (§12.1):** use `git log -S`/`-L` before proposing a new home. Restoring an earlier home is the default.
- **Caller and reader searches use the bare name as well as the qualified one** (§13 (c)): member functions call each other unqualified, and inlined calls leave no symbol.
- **Every number states its source** (file:line, search or commit), and there's no explanation without evidence (§13 (j)).
- **Mark observation (OBS) and inference (INF).**
- **Line counts:** "pure move" means code relocated unchanged plus its include and declaration, which counts 0 net. Anything else counts toward the hard **+20 net `src/`** per WO.

## Report format (your final message only: Markdown, **≤ 150 lines**, nothing before or after it)

- **Header:** "WO-2026-10-15-003 Step 0", base commit, date 2026-10-15, "Model used: <the model you actually ran as>, reasoning: high".
- **The plain goal.** One line.
- **Sections Q1–Q7,** as the WO asks.
  - **Q1:** may use compact tables, grouped by (a)/(b)/(c), not one row per trivial function; give the group totals in lines.
- **"Open points".**
- **Closing:** a table *WO | Goal | Kind (move / fix) | Core lines removed (Sleep / main) | Est. net `src/` | Expected ELF change | Depends on*.
- **End with PROCEED or STOP** and the reason.

**Re-open every `src/` file:line you cite before finishing.** Claude Code will re-check each one against `71e346f`, and any wrong citation goes back to you.
