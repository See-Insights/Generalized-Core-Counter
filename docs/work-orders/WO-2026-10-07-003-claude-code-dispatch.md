AGENT: Claude Code (a separate headless session: `claude -p`) · MODEL: claude-sonnet-5-5 (confirmed with a one-line prompt on 2026-10-07, §5) · REASONING: high (`--effort high`)
AUTHORIZATION SCOPE:
- **Authorized:** read-only. Read, Grep and Glob, and read-only shell (`git log`, `git show`, `git diff`, `git blame`, `grep`, `sed -n`, `wc`, `ls`). Your final message is the report; Claude Code saves it to `docs/work-orders/WO-2026-10-07-003-occupancy-report-rules.md`.
- **Not authorized:** editing or writing any file, git commands that change state, builds, tests, Particle/AWS/device commands, and network access.

# WO-2026-10-07-003: occupancy report rules (evidence)

**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §2 (Claude Code's role and restrictions), §3 Stage 2 (Evidence), §12.1 (history first).

**Plain goal:** find out exactly when the firmware reports an occupancy change right away and when it waits for the scheduled report. Also find whether reporting every occupancy change during normal operations ever existed.

**Requirement being checked (Chip, 2026-10-07):** any occupancy change, start or end, should trigger a report while the device is in normal operations.

**Size budget:** 0 `src/` lines. If any `src/` change seems needed, stop and say so in the report; don't make it. The report must be **≤ 120 lines**.

**Base:** PR #72 head, `79e3c5f` (branch `wo/2026-10-07-002-config-downgrade`, which Dev-14 is running). The working tree is clean at that commit, so working-tree line numbers are the citations. Cite file:line for every claim. For each deciding condition, say whether PR #72 (`73f752e..79e3c5f`) changed it. Note that PR #72 changed every `SystemConfig::get_connectionMode()` reader to `PowerManager::instance().effectiveConnectionMode()` or `SystemConfig::get_configuredConnectionMode()`; that is a getter swap. Say when a swap changes behaviour, which happens only while a downgrade is active.

## Bench evidence to explain (Dev-14, PR #72 build, 2026-10-07 SGT)

1. **Occupancy end, no immediate report, KEEP_ALIVE:** `0001536104 Occ: state=0 reason=debounce session=300s total=983s report=1`, then `Sleep: ULP standby=1 reason=scheduled dur=1225s occ=0`. The report went out at the 18:00 boundary with `Report->Connect reason=occupancy change`.
2. **Occupancy start, immediate report, KEEP_ALIVE:** `0007931274 Occ: state=1 reason=pir-wake led=300s report=1`, then `Sleep->Report reason=sleep-pir-occupancy-report`.
3. **Occupancy end, no immediate report, INTERMITTENT:** Chip saw 1 → 0 with no report after switching to connectionMode 1 (the log was not captured).

## Questions

1. **Rule table.** For each connection mode (INTERMITTENT, INTERMITTENT_KEEP_ALIVE, CONNECTED), which of these events cause an immediate report, and which only mark a report as owed (`report=1`)?
   - occupancy start;
   - occupancy end;
   - scheduled boundary;
   - alert raised;
   - daily close.

   Give the file:line of each deciding condition. Where the answer depends on awake versus asleep (PIR wake from sleep versus a change while awake in Idle), split the row.
2. **Suppression.** What sets `reason=sleep-timer-occupied-suppress-report`? What is it suppressing, and why? Quote the code comments and the commit messages (`git log -S`).
3. **"Normal operations".** List the conditions the code checks before reporting an occupancy change right away, as the code has them: open hours or `Clock::openness()`, battery tier, `downgradeActive()` and the mode in use, power source, debounce, `reportDueThisInterval()`, connection state, and anything else. Don't propose a definition.
4. **History (§12.1).** Did an immediate report on occupancy **end** ever exist?
   - Search with `git log -S` and `-G` on the relevant strings and symbols: for example `report=1`, `reportNow`, `suppress-report`, `occupancy change`, `returnToSleepAfterReport`, `set_occupied(false`, debounce.
   - If it existed, give the commit that removed or gated it, its message, its date, and the release it shipped in (from `CHANGELOG.md` or `FirmwareVersion.h`).
   - Do the same for occupancy **start** if its rules changed.
   - List the searches you ran.
5. **Explain the evidence.** Map each of the three bench observations to the rule (file:line) that produced it. For observation 3, say which rule applies and what log lines Chip would have seen.

## Out of scope

- Any fix or design proposal.
- The sleep gate not waiting for incoming ledger syncs (a separate observation for the recovery plan's Connectivity group).
- Any test runs or device operations.

**Stop and report** if the rules can't be determined from the code alone, for example if they depend on runtime state the logs don't show. Name that state.

## Report format (your final message, Markdown only, ≤ 120 lines, nothing before or after it)

- **Header:** "WO-2026-10-07-003: occupancy report rules (evidence)", base `79e3c5f`, date 2026-10-07, "Model used: <the model you actually ran as>, reasoning: high (set by `--effort high`)".
- **The plain goal.**
- **Sections 1–5,** one per question, using tables.
- **"Open points":** anything you couldn't establish, or state the logs don't show.
- **Closing:**
  - a budget-versus-actual table: | Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |, with the row "Evidence report | 0 | — | 0 | none run (out of scope)";
  - the model used;
  - a list of citations worked out from diffs rather than re-opened.

Before finishing, re-open every `src/` file:line you cite and confirm that the line says what you claim. Mark observation and inference separately (OBS / INF).
