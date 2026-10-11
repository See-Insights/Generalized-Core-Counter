AGENT: Claude Code (a separate headless session: `claude -p`) · MODEL: claude-sonnet-5-5 (confirmed with a one-line probe on 2026-10-09, §5) · REASONING: high (`--effort high`; gate timing across states and Device OS behaviour)
AUTHORIZATION SCOPE:
- **Authorized:** read-only. Read, Grep and Glob, and read-only shell (`git log`, `git show`, `git diff`, `git blame`, `grep`, `sed -n`, `wc`, `ls`). Your final message is the report; Claude Code saves it to `docs/work-orders/WO-2026-10-09-002-step0-report.md`, then commits and pushes it on this branch.
- **Not authorized:** editing or writing files, state-changing git, builds, tests, device or network access.

# WO-2026-10-09-002 Step 0: the once-a-day config window

**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 2, §12.1 (history first), §13 (b) and (c).

**Plain goal:** a ledger config change reaches every sleeping device within a day. On one connection per day, and on the boot connection, the sleep gate holds briefly after the outbound syncs and ends early when an input `onSync` arrives. No other connection pays anything.

**Base:** branch `wo/2026-10-09-002-config-window`, the checked-out worktree: main `83df4a3`, which is v40 plus docs. **Note:** WO-2026-10-09-001 (PR #85, TimeDiag on change, unmerged) touches only `State_Idle.cpp`'s CONNECTED branch and entry test. Say if your proposal would touch the same lines. Cite file:line at HEAD.

**Budget:** the eventual fix must be **≤ +20 net `src/` lines**. Step 0 itself is 0 lines.

## Inputs (read first)

- `docs/work-orders/WO-2026-10-07-005-ledger-sync-evidence.md`, the Stage 2 evidence:
  - Device OS 6.4.1 gives no inbound-pending signal.
  - `onSync` fires only on completion.
  - The gate's inbound test is `lastSynced()`-only.
  - This is the per-connection window history (`b6dd353`, `4c2b734`).
- `docs/RECOVERY_PLAN_2026-09-26.md`:
  - **Chip's 7 Oct decision**, under "Observations to watch", the inbound-ledger item: no per-connection dwell; one config wait per day (the first connection after open hour, plus the boot connection); the gate holds a few seconds after the outbound syncs and ends early on an input `onSync`.
  - **Dev-09's 9 Oct field evidence**, under "Queued WOs", item 2.
- **Dev-09's 9 Oct evidence, in short (v40, KEEP_ALIVE):**
  - `device-settings` `connectionMode: 0` was set at 13:04:51 SGT. It did not land through five connections.
  - At 14:00:41 the log shows `GateRelease: wait=5670 reason=ledger`; at 14:00:44 `[system.ledger] ERROR: Request failed: -1001`; at 14:00:45 `Synchronization failed: -1001; retrying in 30s`. The same happened at 14:10:46 to 14:10:50.
  - It landed only on the boot connection after a pin reset:
    ```
    14:26:57 LedgerCb: kind=DATA …    14:26:59 LedgerCb: kind=STATUS …
    14:27:00.081 LedgerCallback: kind=input ledger=device-settings synced=…
    14:27:00.943 Config: Connection mode -> CONNECTED
    ```
- **7 Oct timing (from the WO-005 context):** the input `onSync` arrived about **764 ms** after the last outbound callback. There were three cold releases with `-1001` within 20 ms, where the last blockers were webhook 2.7 s, ledger 7.5 s and queue 3.3 s.
- **Scope note:** the 7 Oct decision names INTERMITTENT devices. The WO's plain goal says **every sleeping device**, and the field case was KEEP_ALIVE. Say whether the mechanism should differ by mode.

## Questions

1. **Reuse.** Can the existing per-connection inbound window supply the hold, just by skipping the early return on the chosen connection? That window is the early return in `src/cloud/LedgerClient.cpp` (about `:138-152`), with the 10 s window at `src/power/ConnectivityPolicy.h` (about `:170`).
   - Trace exactly what `areLedgersSynced()` does today on a cold and on a warm connection.
   - Say what skipping the early return would wait for: does `lastSynced()` advance on an inbound `onSync`?
   - Confirm it **does not bring back the alert-44 problem `4c2b734` fixed**. Quote that commit's message and diff, and explain the original failure.
   - Say how the gate's ledger blocker and its budget would behave during the hold.
2. **"Once a day."** Which **existing** day-boundary or first-connection-after-open-hour marker can decide it, **without a new persisted field**? Candidates: `DailyBoundary`, the daily-close or open-hour logic, `lastReport` or `lastConnection` against today's open time, a RAM-only per-boot flag for the boot connection, and Clock's open-hours helpers. Say what each covers: boot, first open-hour connection, a device that never connects in the first hour. Also say whether a RAM flag that resets on every boot is acceptable (boot connections are wanted anyway).
3. **The hold's bound.**
   - What do the logs justify? The input `onSync` came about 764 ms after the last outbound callback on 7 Oct, and about 1 s after the STATUS callback on 9 Oct. Note the three cold `-1001` cases.
   - Propose a bound (for example "until input `onSync` or N s"), and justify N against the 10 s cellular window that already exists.
   - What happens when **no** config change is pending: does the device then pay the full N on that one connection each day?
4. **Interactions.**
   - **WO-004's pending-flag checks** (the sleep-prep exit and `cloudSyncStartMs = 0` at about `State_Sleep.cpp:491-498`): could a pending occupancy change cut the hold short, and does that matter?
   - **1b's failsafe timing:** does a longer hold change `lastConnection`, or the failsafe's view?
   - **The `cloudSyncStartMs` reset patterns,** including the CONNECTED+open abort that is known not to reset it, `:406-409`.
   - **The standby latch** (recovery plan, "Queued WOs" item 3): does a longer hold change teardown timing enough to matter?
5. **The smallest change.** One proposal, with file:line, a sketch in words, and a net-line estimate. Say whether any field evidence would let a bench check confirm it.
6. **Stop conditions.** End with **STOP** or **PROCEED**. **STOP** if the estimate exceeds **+20**, or if the fix needs a new **persisted field, timer or schedule**. A RAM-only per-boot flag is allowed; say so explicitly if you use one.

## Constraints

- No per-connection dwell (Chip's 7 Oct decision): every connection except the daily one and the boot one pays nothing.
- No design beyond the one proposal. No code.

## Report format (your final message only: Markdown, **≤ 150 lines**, nothing before or after it)

- **Header:** "WO-2026-10-09-002 Step 0", base commit, date 2026-10-09, "Model used: <the model you actually ran as>, reasoning: high (set by `--effort high`)".
- **The plain goal.**
- **Sections 1–6.** Use tables. Mark observation (OBS) and inference (INF).
- **"Open points".**
- **Closing:** a budget-versus-actual table with the row "Step 0 report | 0 | — | 0 | none run".

Re-open every `src/` file:line you cite before finishing.
