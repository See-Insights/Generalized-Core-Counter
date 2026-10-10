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

## Stage 6 and Stage 7 (2026-10-09)

- **Stage 6** (Copilot, `claude-sonnet-5.5`, medium): +8 code lines (+10 with comments).
  - **Code:** `enteredIdle` captured before `publishStateTransition()`; a four-field key; a static `lastTimeDiagKey`.
  - **Docs:** the `FIELD_MEANINGS_REFERENCE.md` entry.
  - **Test:** new `idle_timediag_on_change_test`, which extracts the real Idle code with a byte check.
  - **Deviation, accepted at Stage 7:** a narrow allowlist line in `clock_trust_standard_structural_test.py` for `Clock::isTimeValid()` in `State_Idle.cpp`.
  - **Results:** suite 74/74 (sh via zsh, py via python3); ARM 151068 / 1094 / 2196.
- **Stage 7** (Codex, `gpt-5.6-sol`, medium): **VERIFIED, no findings** (`WO-2026-10-09-001-stage7-verdict.md`).
  - **Checks:** all 9 pass; all 24 possible keys are unique; four mutations caught.
  - **Entry-pass gap:** the early returns are the occupancy reports at `State_Idle.cpp:73` and `:90`. Only the entry log is skipped, and it is recovered on the next key change, as ruling 1 accepted.

## Closing record (2026-10-09)

**Result:** **VERIFIED, no findings** at Stage 7 round 1 (`WO-2026-10-09-001-stage7-verdict.md`). **Stage 8 approved** by the architect. **Release: held for v41**, bundled with the config-window WO (WO-2026-10-09-002) and the standby-latch WO.

| Stage | Agent / model | Result |
|---|---|---|
| Step 0 | Claude Code, separate session, `claude-sonnet-5-5`, `--effort medium` | PROCEED, est. +6 to +8 |
| Stage 6 | Copilot, `claude-sonnet-5.5`, medium | +8 code lines; 73/73 → 74/74 |
| Stage 7 | Codex, `gpt-5.6-sol`, medium | VERIFIED, no findings; 4 of 4 mutations caught |

Every model was probed first (§5).

| Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |
|---|---|---|---|---|
| WO-2026-10-09-001 | +10 | — | **+8** (+10 with 2 comment lines) | **74/74 (sh via zsh, py via python3)**; new `idle_timediag_on_change_test` |

- **ARM build:** 151068 / 1094 / 2196 (+56 text against v40).
- **What changed:** in CONNECTED Idle, TimeDiag is logged on Idle entry and when `isOpen`, `openness`, `trusted` or `valid` changes, never on every pass. The `FIELD_MEANINGS_REFERENCE.md` entry was corrected.
- **Accepted:** the entry-pass gap (ruling 1), and a narrow test allowlist line for `Clock::isTimeValid()` in `State_Idle.cpp` (judged at Stage 7 not to weaken the guard).
- **Bench: deferred to the v41 soak.** Dev-09 in CONNECTED (`connectionMode` 0) for at least 4 open hours, with one TimeDiag per change and no reset 140 data 2. That step also completes WO-2026-10-08-001 (1b)'s deferred CONNECTED step.
