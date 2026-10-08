# NOT VERIFIED

**Model:** gpt-5.6-sol, standard tier · **Reasoning:** high  
**Tests:** 69/69 — 38 `.sh` via zsh, 31 `.py` via python3

The implementation builds and its ordinary rule table is mostly correct, but the architect’s stale-flag condition fails: several Report paths can retain the flag and cause duplicate reports or a loop.

## Checks 1–13

| # | Result | Evidence |
|---|---|---|
| 1. Linkage | PASS | ELF contains `reportsOccupancyChangesNow()` at `0xc1e44`; four calls plus Idle’s inlined body. Chain: `State_Modes.cpp:80` at `0xc1fda` → predicate → `PowerManager::effectiveConnectionMode()` at `0xbef92`. Idle pending check is at `0xc1a62`; Sleep check at `0xc27f6`. |
| 2. Local build | PASS | Boron, Device OS 6.4.1, fresh `BUILD_PATH_BASE`: base **150964/1090/2196**, changed **150980/1090/2196**; text +16, data/bss unchanged. |
| 3. Tests | PASS | **69/69**: 38 shell tests via zsh and 31 Python tests via python3. Suite ran before any scratch `src/` copy existed. |
| 4. Binary verification | PASS | Sleep flag load/branch at `0xc27f6` precedes sleep calls at `0xc3710`, `0xc3afc`, `0xc3c82`, `0xc3d20`. Report’s connected clear compiles to `strb` at `0xc23f4`. |
| 5. Alerts | PASS | Zero changed lines involving alert raise, ranking, or clearing APIs. No new occupancy path directly touches `RecoveryState`. |
| 6. No new INTERMITTENT path | PASS | All five flag writers are guarded by the positive predicate. INTERMITTENT, DISCONNECTED and downgraded KEEP_ALIVE return false. Only the architect-approved stale-flag exception bypasses the current mode. |
| 7. Independent rule table | CONCERN | Ordinary cells match, except Copilot overstates CONNECTED wake start/end: while open it normally remains awake, and an offline open-hours wake reconnects at Sleep:1635–1638 before the wake occupancy blocks. The blocks report immediately only if actually reached. |
| 8. Stale flag | **FAIL** | Config-invalid, service-request-priority, and alert-40 escalation can leave the flag set before either clear. Connect/Error can then return to Idle or Sleep, causing another Report. |
| 9. Livelock | **FAIL** | Normal connected pending flag is one-shot, but config-invalid can form `Idle/Sleep → Report → Connect → Idle/Sleep` indefinitely with the flag still set. |
| 10. Sleep-prep ordering | **FAIL** | Entry bookkeeping and sleep ordering are correct, but the consumer can fire after teardown began on an earlier pass: Sleep requests disconnect and returns; the main-loop tail latches the flag; the next pass exits to Report while teardown is underway. |
| 11. Test copies | **FAIL** | Eleven direct slices matched `src/`, but the emitted Modes block is not byte-identical: the extractor replaces `SensorManager::instance().loop()` with `testSensorLoop()`. Missing anchors fail loudly; unasserted source changes can be silently re-extracted. |
| 12. Mutations/tests | **FAIL** | The six supplied mutations were caught. Moving the Sleep consumer below suppress was caught only by the structural order assertion—**zero soak failures**. Legacy test edits did not weaken their existing assertions. |
| 13. Budget | PASS | **+14** nonblank, non-comment `src/` lines versus +20; five call-site replacements; no moved source lines. Source diff: 45 insertions, 12 deletions. |

## Independent rule table

`IMM` = occupancy-triggered Report in that deciding pass.  
`LATCH` = flag set; first subsequent Idle/Sleep-prep pass reports.  
`WAIT` = no occupancy flag; data waits for scheduled reporting.

| Mode in use | Idle start | Idle end | Outside-Idle start | Outside-Idle end | Sleep-wake start | Sleep-wake end |
|---|---|---|---|---|---|---|
| KEEP_ALIVE | IMM | IMM | LATCH | LATCH | IMM | IMM when the end gate fires |
| CONNECTED | IMM | IMM | LATCH | LATCH | Normally unreachable while open; IMM if block reached | Normally unreachable while open; IMM if block reached |
| INTERMITTENT | WAIT | WAIT | WAIT | WAIT | WAIT | WAIT |
| DISCONNECTED | WAIT | WAIT | WAIT | WAIT | WAIT | WAIT |
| Downgraded KEEP_ALIVE | WAIT | WAIT | WAIT | WAIT | WAIT | WAIT |

For a scheduled timer wake, WAIT modes may still enter Report on that pass because the scheduled report is due—not because occupancy forced it.

The KEEP_ALIVE soak path is correct in production: end latched by the main-loop handler while SLEEPING → next pass checks the flag at Sleep:369 → REPORTING before any sleep or suppress decision.

CONNECTED reaches deciding points while open through Idle/main-loop handling for both start and end.

## Check 8: flag fate through Report

| Report path with flag set | Flag fate | Outcome |
|---|---|---|
| Alert-40 escalation at [State_Report.cpp:136](</Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Report.cpp:136>) | Retained | ERROR may reset, or return Idle under a no-recovery result; an Idle return reports again. |
| Configuration invalid at [State_Report.cpp:157](</Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Report.cpp:157>) | Retained | CONNECTING can return Idle, Sleep, or firmware-update state; the pending consumer reports again. Persistent invalid configuration can loop. |
| Offline + service request at [State_Report.cpp:210](</Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Report.cpp:210>) | Retained | First payload already carries the occupancy state; return from Connect triggers a second Report. |
| Offline + occupancy branch at [State_Report.cpp:216](</Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Report.cpp:216>) | Cleared | Correct one-shot behavior before CONNECTING. |
| Daily close / KEEP_ALIVE / cadence / not-aligned | Not reachable with flag still set | The occupancy branch has priority over these branches. |
| Already connected at [State_Report.cpp:274](</Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Report.cpp:274>) | Cleared | Correct one-shot Idle return. |

## Findings

1. **High — round 2 required:** stale occupancy flags are not consumed on every Report path.

   - Observation: the early exits and service-request branch precede the offline clear at line 217 and connected clear at line 275.
   - Observation: [State_Connect.cpp:619](</Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Connect.cpp:619>) can subsequently return through firmware update, Sleep, or Idle.
   - Inference: one occupancy change can queue multiple reports; persistent invalid configuration can livelock.

2. **Medium — round 2 required:** pending occupancy can interrupt an already-started teardown.

   - Observation: teardown requests return at [State_Sleep.cpp:765](</Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:765>) and [State_Sleep.cpp:790](</Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:790>), after which the main-loop occupancy handler still runs.
   - Observation: the next Sleep pass consumes the newly latched flag at [State_Sleep.cpp:369](</Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:369>).
   - Inference: Report/Connect can begin while the prior asynchronous disconnect or radio-off is still progressing.

3. **Medium — round 2 test fix required:** the new harness does not satisfy the byte-identical/test-mutation conditions.

   - Observation: [occupancy_report_by_mode_test.sh:144](</Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/occupancy_report_by_mode_test.sh:144>) rewrites the extracted Modes function.
   - Observation: [occupancy_report_by_mode_test.cpp:114](</Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/occupancy_report_by_mode_test.cpp:114>) always models the extracted consumer at the top of sleep prep.
   - Observation: moving the real consumer below suppress produced one structural failure and zero `soak:` failures.
   - Inference: the behavioral soak test cannot detect this ordering regression independently.

The four edited legacy tests retain their prior assertions: reader counts were adjusted for the shared getter, mutation 4 was retargeted, the Sleep transition count changed 17→18, and the restart harness added the required predicate extraction.

## Closing record

| Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |
|---|---:|---|---:|---|
| WO-2026-10-07-004 implementation | +20 | — | **+14** | **69/69** |

## Worktree confirmation

- `git diff --stat` after cleanup matches the start exactly: **10 tracked files, 62 insertions, 24 deletions**.
- `git status` does **not** exactly match the start: an unrelated untracked file, `docs/work-orders/WO-2026-10-07-005-ledger-sync-evidence.md`, appeared during verification at 20:55:54 +0800. I did not create, modify, or delete it.
- I deleted only `build-tmp/wo20261007-004-stage7/`.
- No lasting edits, commits, pushes, stash, reset, checkout, network access, or device operations were performed.