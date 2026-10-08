# WO-2026-10-07-004: Occupancy changes report by mode

**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 2 and 5–8, §2 role restrictions, §12.1–§12.4.
**Base:** main after PR #72 merges. This WO depends on `PowerManager::effectiveConnectionMode()`.
**Evidence:** `docs/work-orders/WO-2026-10-07-003-occupancy-report-rules.md`, plus the Dev-14 soak lines 0001535994–0001536104 (2026-10-07). They show the debounce wake deciding to sleep 108 ms before it processes the occupancy end.
**Recorded by:** Claude Code, verbatim from the architect's WO and rulings (2026-10-07).

## Plain goal

Every occupancy change, start or end, is reported right away unless the mode in use is INTERMITTENT. In INTERMITTENT, changes wait for the scheduled report, as they do today.

*(Refined by ruling 2: right away only when the mode in use is KEEP_ALIVE or CONNECTED.)*

## Size budget

At most +20 net `src/` lines. Going over means stop and report. No compressed code (§12.3).

## Agents and models

- **Step 0:** Claude Code in its own session, Sonnet tier, high reasoning. It has to trace state-machine ordering, which needs care.
- **Implementation:** Copilot, Sonnet tier, high reasoning. The change is small, but ordering bugs are easy to introduce.
- **Verification:** Codex, standard tier (`gpt-5.6-sol`), high reasoning.

Every model is confirmed with a one-line probe first (§5).

## Step 0 (Claude Code)

- List every place that decides whether an occupancy change reports immediately (the `reportNow` tests WO-003 found), with file:line.
- Propose the one smallest change that gives the plain goal on every path.
  - Choose between reordering the debounce wake so the occupancy end is evaluated before the sleep-or-report decision, and having the sleep path honour a pending occupancy-change flag before committing to sleep. Pick whichever covers more paths with less code.
  - If more than two places need the same condition, route them through one shared predicate based on `effectiveConnectionMode()`, rather than repeating it.
- Confirm that CONNECTED reaches the deciding point, since the main-loop handler only starts a report from Idle.
- Stop and report if the estimate exceeds +20, or if the fix needs a new state, timer or flag.

**Result:** `docs/work-orders/WO-2026-10-07-004-step0-report.md`. **PROCEED**, with option (B) plus one shared predicate, estimated at +18.

- Reordering (A) would change nothing: the order is already right. The soak's occupancy end was missed because the wake path's LED gate was false; untrusted-clock close attempts had switched the LED off.
- The main-loop handler then latched the flag while the state was already SLEEPING.

## Architect's rulings (2026-10-07)

1. **The `State_Report.cpp:275` clear: fold it in.** With (B), it is no longer a side issue; it is part of making the fix correct. Without it, a connected device loops. It is one line inside the +18. Add a host test for exactly that case: a CONNECTED device with a pending flag reports once and returns to Idle, with no second report on the next pass.
2. **DISCONNECTED: the predicate is positive.** `reportsOccupancyChangesNow()` returns true only when the mode in use is KEEP_ALIVE or CONNECTED.

## Implementation (Copilot)

- Make the Step 0 change, as amended by the rulings.
- Add a host test of the rule table: 3 modes × start/end × the awake path and the wake-from-sleep path, plus a downgraded KEEP_ALIVE case that must behave as INTERMITTENT.
- Leave the log strings unchanged. *(Ruling 1 supersedes the original instruction to leave the latched flag at `State_Report.cpp:275` alone.)*

## Verification (Codex, Stage 7)

- The linkage check, a local build with a fresh `BUILD_PATH_BASE`, and tests reported as `N/N (sh via zsh, py via python3)`.
- Verify the binary.
- Confirm that no INTERMITTENT path gained an immediate report.

## Bench (Chip, Dev-14)

- **KEEP_ALIVE:** after occupancy ends, the debounce wake goes straight to Report and connects.
- **INTERMITTENT** (the device-settings override from WO-002's step 4): occupancy end shows `report=1`, and the report goes out at the next boundary.
- **CONNECTED:** optional.

## Rules

Agents may open a PR and never merge. The two-round rule applies.

## Stage 6 result and the architect's decisions (2026-10-07)

**Stage 6** (Copilot, `claude-sonnet-5.5`, high): +14 net `src/` lines against +20. Tests 68/68 → 69/69 (sh via zsh, py via python3); Claude Code re-ran the suite and got the same. Six targeted mutations were caught. ARM text went from 150964 to 150980.

**Decisions:**
- **Both Stage 6 deviations are accepted:**
  - the sleep-prep pending check sits after the state-entry bookkeeping but before any sleep or suppress decision;
  - one test line was edited with `sed`.
- **A stale flag after a mode change is accepted, on one condition.** A flag latched in KEEP_ALIVE and still pending when the mode in use becomes INTERMITTENT sends one report, matching Report's existing flag branch. Codex must confirm it is **at most one report per occupancy change, never a repeat**. If it can repeat, that is the round-2 fix. The pending checks are not to re-test the mode.
- **Duplicated test code:** keep the pattern for this WO. Codex confirms the extracted copies match `src/` at the verified commit. A backlog line is added to the recovery plan: tests should compile the real source rather than copies, when the test harness is next touched.

**Bench note (Chip):** Dev-14 is still in INTERMITTENT because the ledger restore hasn't landed. Flashing the WO-004 build reboots it, which gives the restore another chance at the boot connection. If it still doesn't land, the KEEP_ALIVE half of the bench waits on WO-005. The INTERMITTENT half (no immediate report; the flag is paid at the boundary) can run as is.

## Stage 7 round 1, Stage 6 round 2 and the controller edit (2026-10-08)

- **Stage 7 round 1** (Codex, `gpt-5.6-sol`, high): **NOT VERIFIED.** Three findings:
  1. the flag survived Report's early exits (repeat reports, and a loop when the config is invalid);
  2. the sleep-prep check could interrupt a teardown already underway;
  3. the test copies weren't byte-identical, and the soak test didn't follow the real order.
- **Round 2 approved** (architect, 2026-10-08), using existing patterns only:
  - **F1:** Report takes the flag right after `publishData()`, as it does for service requests; the `:275` clear is removed.
  - **F2:** the sleep-prep check moves below `disconnectRequested` and acts only while it is false.
  - **F3:** byte-identical copies, source-order passes, and no-repeat, teardown and connected-once tests.
- **Stage 6 round 2** (Copilot, `claude-sonnet-5.5`, high): net 0 lines this round, WO total +14. Tests 69/69 (sh via zsh, py via python3).
- **Controller edit** (Claude Code, pre-authorized by the architect; **not a round**):
  - **What:** the sleep pending exit now sets `cloudSyncStartMs = 0` before its transition, matching the firmware-update exit. Copilot flagged the gap; Claude Code confirmed it in the source.
  - **Why:** leaving mid-gate with a stale start time made the next sleep's gate time out at once, so it could tear down without draining the queued occupancy report.
  - **Size and test:** +1 line, bringing the WO total to **+15**. Test "midgate:", with a mutation that drops the line, caught by `midgate:`.
  - **Checks after the edit:** fresh-path ARM build 150956 / 1090 / 2196 (unchanged text: the store shares the firmware-update exit's tail); ELF shows the zero store at `c2b24`. Suite 69/69.
- **Not this cause:** the Dev-14 false alert 44 (recovery plan). That gate waited the full 70 s.

## Closing record (to be completed)

- Budget versus actual.
- The rule table as tested.
- The release it ships in (suggested: v39).
