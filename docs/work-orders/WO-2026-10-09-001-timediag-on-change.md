# WO-2026-10-09-001: Idle logs TimeDiag only on change

**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3, §12.1, §13 (b) and (c).
**Base:** main `66d6715` (v40 plus docs) with PR #83 (the inventory, docs only) merged into `wo/2026-10-09-001-timediag-on-change`.
**Recorded by:** Claude Code, from the architect's WO and rulings (2026-10-09).

## Plain goal

In Idle's CONNECTED branch (`src/state/State_Idle.cpp:119-133`), `logTimeDiag()` runs only when its content changes or at a state transition, never on every pass. That is what `docs/FIELD_MEANINGS_REFERENCE.md` already says it does.

## Why

On Dev-09 (v40, 2026-10-09 14:27 SGT), once CONNECTED in open hours, Idle called `logTimeDiag()` on about every loop pass, about 100 lines/s. That saturated the serial log forwarder, and Chip powered the device off. History: unthrottled since `a95e284` (2026-06-06, v14); see `docs/work-orders/2026-10-09-connected-idle-sleep-inventory.md`.

## Step 0 (done)

`docs/work-orders/WO-2026-10-09-001-step0-report.md`: **PROCEED**, about +6 to +8. A separate read-only session ran on `claude-sonnet-5-5` at `--effort medium`.

## Architect's rulings (2026-10-09)

1. **Accept the entry-pass gap.** If Idle returns early on its entry pass, that entry log is skipped until the next key change. No second static.
2. **Key on `isOpen`, `openness`, `trusted` and `valid` only.** Not on `tz`, `open`, `close`, `lastSyncEpoch`, or anything that ticks (`epoch`, `utc`, `local`, `syncAgeMs`).
3. **Fold the docs correction into this WO.** `docs/FIELD_MEANINGS_REFERENCE.md`'s TimeDiag entry becomes: "Time diagnostic. Logged on entry to Idle, when `isOpen`, `openness`, `trusted` or `valid` changes while CONNECTED in Idle, and once per sleep prep." Also add `trusted=`, `openness=`, `syncAgeMs=` and `lastSyncEpoch=` to its heading.

**The fix (from Step 0):**
- Capture Idle's existing `state != oldState` test (`State_Idle.cpp:37`) in a local before `publishStateTransition()` overwrites `oldState`.
- In the CONNECTED branch, pack `isOpen`, `openness`, `trusted` and `valid` into one small key.
- Keep one function-local `static` last-key, initialised to an impossible value.
- Call `logTimeDiag()` only on Idle entry or when the key changes.

**Constraints:**
- Don't gate on `Time.isValid()` or `isWithinOpenHours()`; they only feed the key.
- Don't merge `isOpen=` and `openness=`.
- No change to the Unknown ceiling (`:295-298`).
- Sleep prep's call (`State_Sleep.cpp:1002`) is unchanged.

## Size budget

At most **+10** net `src/` lines. Over budget means stop and report.

## Agents and models

- **Implementation:** Copilot, Sonnet tier, medium.
- **Verification:** Codex, `gpt-5.6-sol`, medium.
- Every model is probed first (§5). The two-round rule applies.

## Tests

A host test simulates CONNECTED Idle for 1000 passes. TimeDiag must be logged **once**, then **once per key change**.

## Bench: none before merge

The bench is the **next release's soak**: Dev-09 in CONNECTED (`connectionMode` 0) for at least 4 open hours, expecting **one TimeDiag per change** and **no failsafe reset** (no reset 140 with data 2). That same step **completes WO-2026-10-08-001 (1b)'s deferred CONNECTED step**.

## Closing record (to be completed)
