AGENT: Claude Code (a separate headless session: `claude -p`) · MODEL: claude-sonnet-5-5 (confirmed with a one-line probe on 2026-10-09, §5) · REASONING: high (`--effort high`)
AUTHORIZATION SCOPE:
- **Authorized:** read-only. Read, Grep and Glob, and read-only shell (`git log`, `git show`, `git diff`, `git blame`, `grep`, `sed -n`, `wc`, `ls`). Your final message is the report; Claude Code saves it to `docs/work-orders/2026-10-09-connected-idle-sleep-inventory.md`, then commits and pushes it on this docs branch.
- **Not authorized:** editing or writing files, state-changing git, builds, tests, device or network access.

# Read-only inventory: CONNECTED devices cycling Idle/Sleep and flooding TimeDiag

**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §12.1 (history first). **Budget:** 0 `src/` lines. **No fix or design proposal.**

**Base:** main at the checked-out HEAD (this worktree; v40 `v40-FailsafeAndSensorType` plus docs). Cite file:line at HEAD.

## The bug

A CONNECTED device in open hours goes Idle → Sleep → `sleep-abort-open-hours` → Idle, and floods TimeDiag. Field case: Dev-09, 2026-10-09 14:27 SGT, on v40, just after it applied `connectionMode` 0. Chip says several earlier WOs tried to fix it.

**Field evidence from today** (forwarder, Dev-09):

```
14:26:54 StateReq: Idle->Sleep reason=low power idle
14:27:00 Config: Connection mode -> CONNECTED
14:27:03 StateReq: Sleep->Idle reason=sleep-abort-open-hours
then: TimeDiag on nearly every loop pass (device uptime about 10 ms apart), for example
  0000197454 [app] INFO: TimeDiag: tz=SGT-8 valid=1 epoch=… isOpen=1 trusted=1 openness=0 …
```

In about 3 minutes of forwarded sample there were no further `StateReq` lines, only TimeDiag. The forwarder passes about 1 line/s of the roughly 100 lines/s the device emits.

## The inventory

Search `docs/work-orders/`, `CHANGELOG.md`, `docs/RECOVERY_PLAN_2026-09-26.md`, other `docs/`, and the git history.
- **Strings and symbols:** `sleep-abort-open-hours`, `TimeDiag`, `logTimeDiag`, `CONNECTED`, `park closed`, `CONNECTED mode`, `isWithinOpenHours`, `Clock::openness`, and the abort at `State_Sleep.cpp` (about `:406-409`) and its earlier locations.
- **Commands:** use `git log -S` and `-G`, `git log --follow` on `State_Sleep.cpp` and `State_Idle.cpp`, and `git blame` on the abort and on Idle's CONNECTED branch (`State_Idle.cpp` around `:108-133`, where `logTimeDiag()` is called).

For **each previous attempt** to fix this behaviour (WOs, commits, releases), give:
- **identity:** the WO or commit, its date and release;
- **change:** what it changed, with net lines;
- **status:** whether it merged, and whether it's still in the code today, reverted, or replaced (with the commit);
- **why it didn't fix the bug,** in the WO's own words where there is a record (quote and cite file:line in the WO or verdict);
- **leftovers:** anything it left behind that exists only because of that attempt, such as guards, flags, timers or logging. Give each with file:line at HEAD.

Also say which attempts touched TimeDiag's logging specifically (when it became per-pass in Idle's CONNECTED branch, and whether any WO rate-limited it).

## Closing section (short)

- **Today's path:** the current call path that produces the cycle and the flood, with file:line, and what each step does. Cover the loop order, Idle's CONNECTED branch, the low-power idle hand-off to Sleep, and the Sleep abort.
- **Where the attempts agree** on the cause, and where they disagree.
- **What not to try again:** approaches that already failed, and why.

## Report format (your final message only: Markdown, **≤ 150 lines**, nothing before or after it)

- **Header:** title, base commit, date 2026-10-09, "Model used: <the model you actually ran as>, reasoning: high (set by `--effort high`)".
- **Tables** where they help. Mark observation (OBS) and inference (INF).
- **"Open points."**
- **Closing:** a budget-versus-actual table with the row "Inventory | 0 | — | 0 | none run", and the citations worked out from diffs rather than re-opened.

Before finishing, re-open every `src/` file:line you cite at HEAD.
