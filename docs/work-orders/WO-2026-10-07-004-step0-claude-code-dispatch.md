AGENT: Claude Code (a separate headless session: `claude -p`) · MODEL: claude-sonnet-5-5 (confirmed with a one-line probe on 2026-10-07, §5) · REASONING: high (`--effort high`)
AUTHORIZATION SCOPE:
- **Authorized:** read-only. Read, Grep and Glob, and read-only shell (`git log`, `git show`, `git diff`, `git blame`, `grep`, `sed -n`, `wc`, `ls`). Your final message is the report; Claude Code saves it to `docs/work-orders/WO-2026-10-07-004-step0-report.md`.
- **Not authorized:** editing or writing any file, git commands that change state, builds, tests, Particle/AWS/device commands, and network access.

# WO-2026-10-07-004 Step 0: occupancy changes report by mode

**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 2, §2 role restrictions, §12.1–§12.4.

**Plain goal:** every occupancy change, start or end, is reported right away unless the mode in use is INTERMITTENT. In INTERMITTENT, changes wait for the scheduled report, as they do today.

**Size budget for the later implementation:** at most **+20 net `src/` lines**. Step 0 itself changes 0 lines.

**Base:** the WO's base is main after PR #72 merges. PR #72 is not merged yet, so cite its head, **`79e3c5f`**, the checked-out branch with a clean tree. Its `src/` is what main will have after the merge. This WO depends on `PowerManager::instance().effectiveConnectionMode()` from PR #72.

**Evidence:**
- `docs/work-orders/WO-2026-10-07-003-occupancy-report-rules.md`: the current rule table, the `reportNow` tests and the history. Read it first.
- The Dev-14 soak log, `/Users/chipmc/Downloads/2026-10-07 17-12-25 Boron CDC Mode #1.log`. These are lines 0001535994–0001536104, verbatim:

```
0001535994 [app] INFO: Wake: reason=TIMER open=1 ready=1 occ=1 led=0s
0001535995 [app] INFO: LoopStage: stage=SLEEP_PREP elapsed=308991 state=3 q=0 connMs=309722
0001535996 [app] INFO: StateReq: Sleep->Sleep reason=sleep-timer-occupied-suppress-report
0001536104 [app] INFO: Occ: state=0 reason=debounce session=300s total=983s report=1
```

  The debounce wake decides to stay asleep (suppress) 108 ms **before** the occupancy end is processed (`report=1`), so the end only reached the report at the 18:00 boundary. You may read the whole log for context. The device was in KEEP_ALIVE (mode 3).

## Tasks

1. **Deciding points.** List every place that decides whether an occupancy change reports right away: the `reportNow` tests WO-003 found (`State_Idle.cpp`, `State_Modes.cpp` start and end, `State_Sleep.cpp` debounce-end and PIR-start), plus any it missed. Give file:line, the condition, and the path (awake in Idle, main-loop handler, or wake from sleep).
2. **Explain the soak lines.** At `0001535994` the device woke with `occ=1 led=0s`, yet the debounce-end branch (`State_Sleep.cpp` around `:1636`, `signalLEDTimeRemaining() == 0 && signalLEDStatus()`) did not fire before the suppress decision (around `:1752`). Find exactly why: what was `signalLEDStatus()`, and what ordering or condition let the suppress decision run first? Then find which code produced the `Occ: state=0` line 108 ms later, and why it didn't start a report then (the `state == IDLE_STATE` gate in `State_Modes.cpp`?). Cite each step. Mark observation and inference.
3. **The smallest change.** Propose one change that gives the plain goal on every path. Choose between:
   - **(A)** reordering the debounce wake so the occupancy end is evaluated before the sleep-or-report decision; and
   - **(B)** having the sleep path honour a pending occupancy-change flag (`session.occupancyChangeTriggered`) before committing to sleep.

   Pick whichever covers more paths with less code. Show which paths each one covers and misses. If more than two places need the same condition, route them through **one shared predicate based on `effectiveConnectionMode()`** (for example "reports occupancy changes right away" == mode in use is not INTERMITTENT), rather than repeating it. Then:
   - name every file and line that changes;
   - give a net `src/` line estimate with its reasoning;
   - describe the change in words or as a short sketch. It is a proposal for the architect, not code to apply.
4. **CONNECTED.** Confirm whether CONNECTED (mode 0) reaches a deciding point for start and end. The main-loop handler only starts a report from Idle (`State_Modes.cpp`, `state == IDLE_STATE`), and CONNECTED doesn't sleep while open. Show the path, and say whether CONNECTED already has a connection, so that "report right away" means the queue sends at once.
5. **DISCONNECTED (mode 2).** The plain goal says "unless INTERMITTENT", which literally includes DISCONNECTED. Say what an immediate report would mean in mode 2 (does it ever connect?), and flag this for the architect. Don't decide it.
6. **History check (§12.1).** WO-003 found the all-modes sleep-wake report removed by `737ceb3` (Release 9.00). Say whether restoring any part of that code is a smaller route to the goal than (A) or (B).
7. **Stop conditions.** End with STOP or PROCEED and the reason. The answer is **STOP** if:
   - the estimate exceeds +20;
   - the fix needs a new state, timer or flag (reusing `session.occupancyChangeTriggered` is not new);
   - the paths can't be determined from the code alone; name the runtime state you'd need.

## Out of scope

- The latched flag at `State_Report.cpp:275`: it's logged separately; don't fix it. Note it if your change interacts with it.
- Any design beyond the one proposal.
- Tests, builds and device operations.

## Report format (your final message, Markdown only, ≤ 150 lines, nothing before or after it)

- **Header:** "WO-2026-10-07-004 Step 0", base `79e3c5f`, date 2026-10-07, "Model used: <the model you actually ran as>, reasoning: high (set by `--effort high`)".
- **The plain goal.**
- **Sections 1–7.** Use tables.
- **"Open points".**
- **Closing:** a budget-versus-actual table with the row "Step 0 report | 0 | — | 0 | none run", plus a list of citations worked out from diffs rather than re-opened.

Before finishing, re-open every `src/` file:line you cite and confirm that it says what you claim.
