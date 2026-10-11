AGENT: Claude Code (a separate headless session: `claude -p`) · MODEL: claude-sonnet-5-5 (confirmed with a one-line probe on 2026-10-09, §5) · REASONING: medium (`--effort medium`; a small, local change in one function)
AUTHORIZATION SCOPE:
- **Authorized:** read-only. Read, Grep and Glob, and read-only shell (`git log`, `git show`, `git diff`, `git blame`, `grep`, `sed -n`, `wc`, `ls`). Your final message is the report; Claude Code saves it to `docs/work-orders/WO-2026-10-09-001-step0-report.md`, then commits and pushes it on this branch.
- **Not authorized:** editing or writing files, state-changing git, builds, tests, device or network access.

# WO-2026-10-09-001 Step 0: Idle logs TimeDiag only on change

**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 2, §12.1 (history first), §13 (b) and (c).

**Plain goal:** in Idle's CONNECTED branch (`src/state/State_Idle.cpp:119-133`), `logTimeDiag()` runs only when its content changes or at a state transition, never on every pass. That is what `docs/FIELD_MEANINGS_REFERENCE.md` already says it does.

**Base:** branch `wo/2026-10-09-001-timediag-on-change`, the checked-out worktree. It is main (`66d6715`, v40 plus docs) with PR #83 merged in (docs only; the inventory). Cite file:line at HEAD.

**Budget:** the fix must be **≤ +10 net `src/` lines**. Step 0 itself is 0 lines.

**Read first:** `docs/work-orders/2026-10-09-connected-idle-sleep-inventory.md`, the inventory of this bug's history and today's call path. Field case: Dev-09 on v40, 2026-10-09 14:27 SGT. Once CONNECTED in open hours, Idle called `logTimeDiag()` on about every loop pass (device uptime about 10 ms apart), about 100 lines/s, which saturated the serial log forwarder.

## Questions

1. **Every caller of `logTimeDiag()`** (definition at `Generalized-Core-Counter.cpp:1848`), with file:line, and which of them run on every pass. Include the sleep-entry call (`State_Sleep.cpp` around `:1002`). Say how often each one runs in each connection mode.
2. **What "its content" is.** List the fields `logTimeDiag()` prints (`Generalized-Core-Counter.cpp` around `:1848-1920`). Say which change every second (epoch, utc, local, `syncAgeMs`) and which change rarely (`valid`, `isOpen`, `trusted`, `openness`, tz, open and close hours). Then say what "on change" should key on so that it fires on meaningful changes and not on the clock ticking. This is a finding, not a design choice.
3. **The smallest change that meets the goal,** using existing state where possible (for example the last-logged openness, or the epoch minute). Give the exact file:line, a short sketch in words (not code to apply), and a net-line estimate with its reasoning. The goal also says "or at a state transition": say whether Idle's existing state-entry hook (`enteredState` or equivalent) can provide that without new state.
4. **The documentation.** Quote `docs/FIELD_MEANINGS_REFERENCE.md`'s TimeDiag entry (around `:27-29`). Say whether it would still need correcting after the change, and how (wording only).
5. **History (§12.1).** Has TimeDiag ever been logged on change or rate-limited in this repo? The inventory says no (`a95e284` onward). Confirm briefly with `git log -S`/`-G`; restoring is the default if anything existed.

## Constraints (from the inventory; don't propose anything that breaks them)

- Don't gate on `Time.isValid()` or `isWithinOpenHours()`.
- Don't merge `isOpen=` and `openness=`.
- No change to the Unknown ceiling behaviour (`State_Idle.cpp` around `:295-298`).

## Stop conditions

End with **STOP** or **PROCEED** and the reason. **STOP** if the estimate exceeds **+10**, or if the change needs a new persisted field, timer or flag beyond one "last logged" value (a single static or file-local variable is allowed).

## Report format (your final message only: Markdown, **≤ 80 lines**, nothing before or after it)

- **Header:** "WO-2026-10-09-001 Step 0", base commit, date 2026-10-09, "Model used: <the model you actually ran as>, reasoning: medium (set by `--effort medium`)".
- **The plain goal.**
- **Sections 1–5.** Use tables. Mark observation (OBS) and inference (INF).
- **The stop verdict, then "Open points".**
- **Closing:** a budget-versus-actual table with the row "Step 0 report | 0 | — | 0 | none run".

Re-open every `src/` file:line you cite before finishing.
