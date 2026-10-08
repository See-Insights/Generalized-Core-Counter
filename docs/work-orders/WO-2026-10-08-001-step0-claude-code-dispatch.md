AGENT: Claude Code (a separate headless session: `claude -p`) · MODEL: claude-sonnet-5-5 (confirmed with a one-line probe on 2026-10-08, §5) · REASONING: high (`--effort high`)
AUTHORIZATION SCOPE:
- **Authorized:** read-only. Read, Grep and Glob, and read-only shell (`git log`, `git show`, `git diff`, `git blame`, `grep`, `sed -n`, `wc`, `ls`). Your final message is the report; Claude Code saves it to `docs/work-orders/WO-2026-10-08-001-step0-report.md`, then commits and pushes it.
- **Not authorized:** editing or writing any file, git commands that change state, builds, tests, Particle/AWS/device commands, and network access.

# WO-2026-10-08-001 Step 0: the failsafe counts only overdue expected connections (Step 6 WO 1b)

**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §2, §3 Stages 2 and 4, §12.1 (history first), §12.3 (budget).

**Plain goal:** the connectivity failsafe escalates only when a connection the device was expected to make is overdue, never just because time passed while no connection was due.

**Base:** main after #75, **`2d77c2a`**. This is the checked-out branch `wo/2026-10-08-001-failsafe-overdue`, with a clean tree, so working-tree line numbers are the citations. Cite file:line at that commit.

**Budget:** 0 `src/` lines for Step 0. The target for the eventual fix is **≤ +20 net `src/` lines**.

## Context you need

- The failsafe supervisor is `connectivityFailsafeSupervisor()` in `src/Generalized-Core-Counter.cpp`. Its age base is around `:2612-2620`, and the hard-stage block (`lowBatteryHardActionBlocked`) is around `:2655-2671`. Read the whole function and its helpers.
- **The Codex ownership report:** `docs/work-orders/2026-10-06-step6-ownership-codex-report.md`. Its 1b claims are at line 45 (item 6, "Failsafe") and line 74 (the WO table row "1a → 1b").
- **What has changed since that report:**
  - **WO-2026-10-07-002 (#72):** the mode in use is `PowerManager::instance().effectiveConnectionMode()`. The failsafe's hard-stage block reads `PowerManager::instance().downgradeActive()`, and its DISCONNECTED gate reads the mode in use.
  - **WO-2026-10-07-004 (#75):** in KEEP_ALIVE and CONNECTED, occupancy changes report immediately, via `reportsOccupancyChangesNow()` and the pending checks.
- **Releases:** v31 (WO-2026-10-01-001: failed-attempt counting, OTA-aware dwell, reset-cause codes) and v32 (WO-2026-10-02-001: stage 1 retired; failsafe recovers in about 3 open hours). See `CHANGELOG.md` and `docs/work-orders/WO-2026-10-01-001-*` and `WO-2026-10-02-001-*`.

## Questions

1. **Is it still real?**
   - Re-open the 1b citations from the Codex report (`Main:355-382`, `Main:2612-2620`, `Main:2655-2671`, `ConfigApply.cpp:445-455`, `Main:2612-2671`) at `2d77c2a`. Mark each one HOLDS, MOVED TO file:line, or WRONG.
   - Then give **one concrete scenario on today's main**, with file:line for each step, where the failsafe counts toward escalation while no connection was due. Candidates: closed hours, INTERMITTENT scheduling (reports only at cadence boundaries, `reportDueThisInterval()`, the battery-scaled cadence), a downgrade, or night or hibernate sleep. Show exactly what "age" or count grows and why no connection was due during that time.
   - **If you can't construct one, STOP and say so:** 1b may be obsolete.
2. **History (§12.1).**
   - How did v31 (failed-attempt counting) and v32 (stage 1 retired, about 3 open hours to recover) change what the failsafe counts?
   - Did an earlier version count only expected connections, or attempts, rather than elapsed time? Search with `git log -S`/`-G` on the failsafe symbols (`connectivityFailsafe`, `ageBase`, `lastConnection`, `failsafe stage`, `FAILSAFE_`, `ConnectivityPolicy::` constants), and list the searches you ran.
   - If an earlier design counted expected connections, say whether restoring it is the default fix.
3. **Interactions.**
   - Confirm that the fix would not change how `downgradeActive()` blocks the hard stages (WO-002, `Main` around `:2664`).
   - Confirm that it would not change how often occupancy reports connect (WO-004).
   - If it would interact with either, say how.
4. **The smallest fix.**
   - Propose the one smallest change that meets the plain goal, using state that already exists. Candidates: the scheduled-report boundary, `reportDueThisInterval()`, `Clock::openness()`, `SystemConfig::get_lastConnection()` / `get_lastReport()`, the failed-attempt counter from v31.
   - Name every file and line that changes, and give a net `src/` line estimate with its reasoning.
   - Describe it in words or as a short sketch. It's a proposal for the architect, not code to apply.
   - Say which of today's escalation paths it would delay, and confirm that a genuinely failing device (connections due and failing) still escalates on the v32 schedule (about 3 open hours).
5. **Stop conditions.** End with **STOP** or **PROCEED** and the reason. STOP if:
   - no scenario exists (1b obsolete);
   - the estimate is over +20, in which case propose a split or a simpler rule;
   - the fix needs a new state, timer or persisted field.

## Out of scope

Any code change, test run, build or device operation. Any design beyond the one proposal.

## Report format (your final message, Markdown only, **≤ 100 lines**, nothing before or after it)

- **Header:** "WO-2026-10-08-001 Step 0", base `2d77c2a`, date 2026-10-08, "Model used: <the model you actually ran as>, reasoning: high (set by `--effort high`)".
- **The plain goal.**
- **Sections 1–5.** Use tables. Mark observation (OBS) and inference (INF).
- **"Open points".**
- **Closing:** a budget-versus-actual table with the row "Step 0 report | 0 | — | 0 | none run"; the model used; and the citations worked out from diffs rather than re-opened.

Before finishing, re-open every `src/` file:line you cite and confirm that it says what you claim.
