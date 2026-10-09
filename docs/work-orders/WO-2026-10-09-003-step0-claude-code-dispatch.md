AGENT: Claude Code (a separate headless session: `claude -p`) · MODEL: claude-sonnet-5-5 (confirmed with a one-line probe on 2026-10-09, §5) · REASONING: high (`--effort high`)
AUTHORIZATION SCOPE:
- **Authorized:** read-only. Read, Grep and Glob, and read-only shell (`git log`, `git show`, `git diff`, `git blame`, `grep`, `sed -n`, `wc`, `ls`). Your final message is the report; Claude Code saves it to `docs/work-orders/WO-2026-10-09-003-step0-report.md`, then commits and pushes it on this branch.
- **Not authorized:** editing or writing files, state-changing git, builds, tests, device or network access.

# WO-2026-10-09-003 Step 0: the modem standby latch

**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 2, §12.1 (history first), §13 (b) and (c).

**Plain goal:** a device whose standby was suppressed after a slow teardown gets standby back without needing a reboot, unless its modem is genuinely still failing.

**Base:** branch `wo/2026-10-09-003-standby-latch`, the checked-out worktree (main plus docs). Cite file:line at HEAD. **Note:** two unmerged WOs touch `State_Sleep.cpp`. PR #86 adds `cloudSyncStartMs = 0;` at the CONNECTED+open abort and moves that variable's declaration up (about `:397-409`). PR #85 touches only `State_Idle.cpp`. Say if your proposal would touch the same lines.

**Budget:** the fix must be **≤ +20 net `src/` lines**. Step 0 itself is 0 lines.

## Evidence (Dev-09, a BRN404X on Singtel, 8 and 9 Oct 2026)

- **The constants:** suppression trips when a teardown takes more than 10 s (`MODEM_UNSTABLE_SLOW_TEARDOWN_MS`, `src/state/State_Sleep.cpp:31`; the check at about `:887`). Recovery needs a teardown under 5 s (`MODEM_UNSTABLE_RECOVERY_TEARDOWN_MS`, `:32`; `maybeClearModemUnstable()` at about `:129-148`, check at about `:136`).
- **8 Oct, v39:** after a breadcrumb-28 watchdog and `Failed to power off` (the NCP client), the log shows `12:19:02 SLEEP: disconnect/modem-off exceeded budget (30001 ms …) - raising alert 15`, then `12:19:03 MODEM_HEALTH: unstable reason=slow_teardown elapsed=30003`, then `12:19:04 MODEM_POLICY: standby temporarily disabled reason=unstable_modem`. Every one of the 56 forwarded `Sleep:` lines after that shows `standby=1/0` with `td≈19 s` (`modem≈19 s`), until the next reboot. No `MODEM_HEALTH: recovered` appears.
- **9 Oct, v40:** after a failed dial at 13:49 (`Failed to initialize cellular NCP client: -210`), every sleep again shows `standby=1/0` with `td≈19.1–19.7 s`. Cleared only by a pin reset.
- **Session flags are RAM** (`session.modemUnstable`, `session.modemStandbySuppressed`, `StateMachine.h`), so a reboot clears them.

## Questions

1. **History first (§12.1).** Why was the suppression added? Find the commit(s), WO, CHANGELOG entry and any recovery-plan or WO notes. Use `git log -S` on `markModemUnstable`, `MODEM_UNSTABLE_SLOW_TEARDOWN_MS`, `MODEM_UNSTABLE_RECOVERY_TEARDOWN_MS`, `modemStandbySuppressed` and `unstable_modem`. Then:
   - What failure does it prevent? Quote the record.
   - Is it related to the **breadcrumb-28 watchdogs** inside `System.sleep()` (WO-2026-10-04-001 item A2, `State_Sleep.cpp` around `:725-728`, and the 9 Oct watchdog analysis)?
   - Did the recovery path ever work as intended, or was the latch there from the start?
2. **The flaw.**
   - Confirm, with file:line, that once tripped it can't clear, because recovery is measured only on teardowns that run with standby *disabled*, and those always include a full modem-off taking far longer than 5 s.
   - Trace every path that sets and clears `modemUnstable` and `modemStandbySuppressed`, including the connect-timeout path in `State_Connect.cpp` (about `:150-165`).
   - Is there any path today where recovery can happen? For example, a device whose modem-off is fast.
3. **The smallest recovery that keeps the protection.** Compare at least these two, with a line estimate and the risk of each:
   - **(a) One retry:** after some number of clean (successful-connect) cycles with standby off, try one teardown with standby **enabled**, and judge recovery on that teardown's time. If it is slow again, re-suppress.
   - **(b) The daily boundary:** re-allow standby once a day (at the open hour or the close), using existing day-boundary state, and let the slow-teardown check re-trip it if the modem is still failing.

   Also name any simpler option, for example measuring recovery on the *cloud-disconnect* time instead of the total. Pick one.
4. **Interactions.**
   - The sleep gate and the teardown budget (`computeDisconnectBudgetMs`), and alert 15.
   - The breadcrumb-28 watchdog risk if standby is re-enabled on a still-failing modem.
   - KEEP_ALIVE, INTERMITTENT and CONNECTED: which ever request standby?
   - WO-2026-10-09-002's config hold (PR #86), which doesn't change teardown timing.
   - Whether any status or log field would show the latch in the field. The status payload is near its byte limit, so no new status fields.
5. **Stop conditions.** End with **STOP** or **PROCEED**. **STOP** if the estimate exceeds **+20**, or the fix needs a new **persisted field, timer or schedule**. A RAM-only counter or flag in `session` is allowed; say so explicitly if you use one.

**Fleet relevance:** you have no network access. Claude Code will add fleet evidence separately (how often suppression has tripped and cleared in the forwarder serial logs). Note in your report which log lines would show it.

## Report format (your final message only: Markdown, **≤ 150 lines**, nothing before or after it)

- **Header:** "WO-2026-10-09-003 Step 0", base commit, date 2026-10-09, "Model used: <the model you actually ran as>, reasoning: high (set by `--effort high`)".
- **The plain goal.**
- **Sections 1–5.** Use tables. Mark observation (OBS) and inference (INF).
- **"Open points".**
- **Closing:** a budget-versus-actual table with the row "Step 0 report | 0 | — | 0 | none run".

Re-open every `src/` file:line you cite before finishing.
