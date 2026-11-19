# WO-2026-11-19-001: After a failed connect, hold further connect attempts (back-off)

**Author:** architect, 19 Nov 2026 (Chip: "an important bug to fix - a device that acts like this in the field would quickly run out of battery") · **Status:** DRAFT for Chip's approval (Stage 5). **Release:** first item of v50.2 unless Chip rules it into v50.1.
**Branch:** `wo/2026-11-19-001-connect-hold` from `integration/v50-ab1805` at `eb398b8` (so it can land in either v50.1 or v50.2).
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5–8; §4 (dispatch format; the three-level Stage 7 verdicts of 11 Nov; at most two Codex rounds); §5 model routing; §6 WO contents.
**Agents (§5):**
- Stage 6: **Copilot**, standard model, **medium** reasoning (two files, one small gate).
- Stage 7: **Codex**, **top model, highest reasoning**. §5 escalates power and time logic.
- Claude Code resolves the model IDs at dispatch, per §5.

**Budget:** `src/` **≤ +25 net lines**. Tests are not capped, but keep them proportionate. If the gate cannot be done within the budget, **stop and report**: don't widen the scope.

## Plain goal
When a connect attempt fails, the device must not try again straight away. It waits, and the wait grows with each further failure: **1 h, 2 h, 4 h, then every 6 h at most.** A good connect resets the wait. Counts are still recorded on time and queued; only the upload waits.

## The bug (evidence: Dev-09, 18–19 Nov, mode IKA, fixed 5001 `eb398b8`)
- **Ubidots and the archive (Claude Code, 19 Nov):**
  - The last good connect was 07:29Z on 18 Nov.
  - After it, a 660 s attempt started about every 11 min until the stage-2 failsafe reset at 13:03Z (140/2).
  - There was a brief connect at 13:15Z, back-to-back attempts again until about 14:22Z, then hibernate.
  - From 22:00Z to about 23:48Z there were back-to-back attempts again.
  - Every failure was `CELLULAR_ACQUIRE`, `sig=na`, `NCP client: -210`.
  - Dev-09 has a known bad antenna: `RECOVERY_PLAN_2026-09-26.md:243`.
- **Serial, 23:48–23:49Z (Chip's capture):**
  - The connect succeeded after `cellMs=521772`.
  - Then came `PPP link failed: error 5`, `MODEM_HEALTH: unstable reason=slow_teardown`, and `Sleep: … standby=1/0 mode=IKA`.
  - Then `Sleep: ULP standby=0 reason=debounce dur=70s occ=1`.
  - The next occupancy change reconnects cold.
  - `INV: awake_ms=6513806 > ceiling_ms=1090000` is logged only (`State_Sleep.cpp:384`).
- **Why it happens, at `eb398b8`:**
  - `State_Report.cpp:199–206`: the only guard, `deferAutoConnectForUnstableModem`, holds a reconnect for **30 s** (`MODEM_UNSTABLE_RECONNECT_DEFER_MS`, `:27`).
  - That guard applies only to the keep-alive branch (`:242–251`) and the boundary-aligned branch (`:254–266`).
  - **The occupancy-change branch (`:226–231`) has no guard.** Neither do daily close (`:234`), webhook health (`:239`) and config invalid (`:168–169`).
  - `ModemHealth.cpp:55–63` already counts consecutive timeouts (`modemConnectTimeoutCount`). `:65` (`noteConnectSuccess`, called at `State_Connect.cpp:590`) resets the count, and `:71` (`clearAtDailyClose`, called at `State_Report.cpp:87`) clears it too.
  - Nothing uses that count to space out attempts.
- **The fleet runs the same logic.** The relevant constants and branches are the same in v49.1, so this isn't a v50 regression. A field unit with a weak or flaky radio and steady occupancy searches for most of the day.

## The change (approved architecture, once Chip approves)
1. **One hold, in one place:** `State_Report.cpp`, ahead of the immediate-connect branches.
   - The hold is active when the consecutive-failure count is N ≥ 1 and less than `holdMs(N)` has passed since the end of the last failed attempt.
   - While it's active, every **automatic** trigger goes to `IDLE_STATE` with the reason `"connect hold"`, logging `CONNECT_HOLD: remaining=<s> failures=<N> trigger=<name>` once per trigger. The automatic triggers are: occupancy change, keep alive, boundary aligned, daily close, webhook health and config invalid.
   - The existing path from Idle to Sleep handles the rest. The records stay queued.
2. **`holdMs(N)` = 1 h × 2^(N−1), capped at 6 h**: 1 h, 2 h, 4 h, 6 h, 6 h …
   - Put the 1 h base and the 6 h cap in `ConnectivityPolicy.h` beside the connect budgets, with a short comment.
   - Use one small function. No new class.
3. **The count and timestamp:**
   - Reuse `modemConnectTimeoutCount`, or add one RAM timestamp beside it, set at the timeout in `State_Connect.cpp:641` / `:654`.
   - Reset on success (`noteConnectSuccess`) and at the daily close (`clearAtDailyClose`), as today.
   - RAM only: a reset or hibernate starts fresh, which allows at most one extra attempt per failsafe cooldown. This is accepted; state it in the report.
4. **Exempt:** `service request` (`State_Report.cpp:225`, a deliberate human or remote action). No other trigger is exempt.
5. **Unchanged:**
   - the 30 s unstable-modem defer (it still covers `slow_teardown` with no failed connect);
   - the 5 and 11 minute connect budgets;
   - the ConnectivityFailsafe ladder;
   - CONNECTED mode's reconnect at `State_Sleep.cpp:1036–1038` (mains or bench only).

## Step 0 (read-only, inside Stage 6; report before editing)
1. Cite **every** `transitionTo(CONNECTING_STATE, …)` on the base, with file:line. Confirm that each automatic one passes through `REPORTING_STATE`, or list the ones that don't.
2. Cite how ConnectivityFailsafe stage 1/2 measures connection age (`lastConnection`). Confirm the hold doesn't delay or suppress the failsafe: the hold doesn't touch `lastConnection`. **Stop and report** if the failsafe starts its own connect outside `CONNECTING_STATE`.
3. Confirm where the end of a failed attempt is known (`State_Connect.cpp:641–654`) and that a stage-2 recovery inside an attempt (`:92–102`) doesn't count as a separate failure.

## Non-goals
- The 11-minute deep budget on any battery above 50 % (`ConnectivityPolicy.h:80–81`): a separate question, ask the architect.
- Enforcing the `INV` awake ceiling.
- Any change to the Ubidots or webhook payloads.
- Persisting the hold across resets.

## Tests (host)
- **Hold sequence:** after 1, 2, 3, 4 and 5 consecutive timeouts, an occupancy-change trigger inside the hold gives no connect. The first trigger after `holdMs(N)` connects. The holds are 1 h, 2 h, 4 h, 6 h, 6 h.
- **A success resets the hold:** after the reset, the next occupancy change connects immediately.
- **Daily close clears the hold.**
- **`service request` connects during a hold.**
- **The failsafe is unaffected:** the stage-1/2 timing with the hold active is the same as without it.
- **Mutations** (scratch copy, or `cp` backup and restore; never `git checkout`):
  - remove the hold check from the occupancy-change branch: the hold test fails;
  - set the cap to 0 or remove the doubling: the sequence test fails.
- The full suite stays green, and a fresh Device OS build reports text/data/bss against `eb398b8`.

## Bench (Chip, after Stage 7)
**Dev-09 as it is now** (bad antenna, mode IKA, exerciser off but `occ=1`) is the perfect test. Over 6 open hours with serial:
- `CONNECT_HOLD` lines appear after the first failure;
- attempts start at about +1 h, +2 h, +4 h;
- no back-to-back `Connect: start`.

Then fit a good antenna. The first connect clears the hold, and the board returns to normal reporting.

## Expected telemetry
- A failing device makes at most about 4–5 attempts per open day instead of dozens.
- `connecttime` rows thin out.
- Alert 31 is raised once per failed attempt, as today.

## Rollback
Revert the PR. This is behaviour only: there's no persistent or ledger change.

## Closing record
| Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |
|---|---|---|---|---|
| Connect hold | ≤ +25 | — | | |
