# WO-2026-10-07-003: occupancy report rules (evidence)

Base `79e3c5f` (PR #72 head) · Date 2026-10-07 · Model used: claude-sonnet-5-5, reasoning: high (set by `--effort high`)
**OBS** means read directly in the code or git history. **INF** means inferred from the code plus the supplied bench lines.

## The plain goal
Find out when the firmware reports an occupancy change right away and when it waits for the scheduled report. Also find out whether reporting every occupancy change in normal operations ever existed.

**Short answer.** (OBS)
- Only effective mode INTERMITTENT_KEEP_ALIVE (3) ever reports an occupancy change right away. INTERMITTENT and CONNECTED set `reportNow=false` at every site.
- Even in mode 3, the end of occupancy is reported right away only if the code that closes it is running while `state == IDLE_STATE`, or on the sleep-wake path. Otherwise the report is latched and waits for the next report pass. This explains observation 1.
- An all-modes immediate report existed once, only on the sleep-wake path (2026-02-09 to 2026-04-24, release 9.00). The awake paths were mode-3-only from the start.

## 1. Rule table
Cells: **IMM** = goes to REPORTING right away. **OWED** = a report is queued or flagged and sent later. **no** = nothing owed (`report=0`). `Report:` lines are `State_Report.cpp`.

| Event | INTERMITTENT (1) | KEEP_ALIVE (3) | CONNECTED (0) | Deciding line |
|---|---|---|---|---|
| Start, awake (main-loop handler, any state) | no | IMM if `state==IDLE`; in any other state OWED (flag latched, no transition) | no | `Modes:64,80,85-89`; `Generalized-Core-Counter.cpp:1720-1721` |
| Start, PIR wake from sleep | no. Then `Sleep:1783` overdue check, else back to sleep `Sleep:1793-1795` | IMM, `sleep-pir-occupancy-report` | n/a: CONNECTED aborts sleep while not Closed | `Sleep:1719-1728`; `Sleep:406` |
| End, awake in Idle | no | IMM `occupancy transition`, unless `stillOpen` (clock untrusted) | no | `Idle:57-71`; `State_Common.h:328-331` |
| End, awake via the main-loop handler | no | IMM only if `state==IDLE`; any other state OWED (latched) | no | `Modes:142-157` |
| End, found on sleep wake (LED timeout) | no at close. Then `timerWake` goes to REPORTING `sleep-timer-report` (INF: see section 5) | IMM `sleep-occupancy-debounce-report` | n/a | `Sleep:1636-1650`; `Sleep:1750-1763` |
| Scheduled boundary | REPORTING entered, payload queued. Connects only if `cadenceDue` (within 30 s of a battery-scaled boundary), else `not aligned`, OWED until the next connect | IMM connect `keep alive mode`, unless modem unstable. If occupied, only when `reportDueThisInterval()` | already connected, so the queue sends | `Idle:181-200`; `Sleep:1750-1763`; `Report:231-243,244-257,272,275`; `Idle:185-187`; `Sleep:1756-1759` |
| Alert raised | no | no | no | no `transitionTo(REPORTING_STATE` site is keyed on an alert (14 sites by grep); the alert rides the next payload, `Generalized-Core-Counter.cpp:1991` |
| Daily close | `daily close` connects immediately if offline | same | already connected, `Report:275` | `Report:223-225`; entered from `Idle:200`, `Sleep:995`, `Sleep:1762`. An open session is closed at the boundary with no end report, `Report:55-60` |

**PR #72 (`73f752e..79e3c5f`) (OBS).** Every mode reader in the table became `effectiveConnectionMode()`, a getter swap. The affected lines:
- `Modes:80,143`; `Idle:59,112,186,210`
- `Sleep:373,406,1638,1719,1757,1776,1793`
- `Report:201,231`

`State_Common.h` (`reportDueThisInterval`, `closeOccupancySessionSafely`) is not in the diff. The swap changes behaviour only while `downgradeActive()` is true (`PowerManager.cpp:239-243`): OCCUPANCY, configured mode 3, and `lowBatteryMode` set. Rows then read as INTERMITTENT. Before PR #72 the downgrade overwrote the stored mode with INTERMITTENT, so the occupancy rules during a downgrade are the same. The difference is that the configured mode is now kept.

## 2. Suppression: `sleep-timer-occupied-suppress-report`
| Item | Finding |
|---|---|
| What sets it | `Sleep:1752-1759`. On a timer wake, with OCCUPANCY, occupied, effective mode 3 and `!reportDueThisInterval()`: `transitionTo(SLEEPING_STATE, "sleep-timer-occupied-suppress-report")` |
| What it suppresses | The `sleep-timer-report` at `Sleep:1762`. While occupied, the sleep timer is the debounce wake, `min(debounce, boundary)` (`Sleep:1096-1103`), so these wakes are not boundary wakes. It does not suppress the end report, which is handled earlier at `Sleep:1636-1650`. |
| Why (current comment) | `Sleep:1753-1755`: "suppress periodic reports while occupied UNLESS one is due for this reporting interval (WO-2026-10-02-002: report every hour, occupied or not)." |
| Why (original) | `eda6b7e` (2026-02-09, "v3.24 - Occupancy Mode", no body): "suppress periodic reports while occupied so occupancy=1 is only reported on 0->1 transition". Original log text: "Timer wake while OCCUPIED (KEEP_ALIVE) - suppressing scheduled report". |
| Commit messages | `691b6c7` (2026-10-02, v33): "While occupied (occupancy mode + keep-alive), the scheduled report is now made when due instead of being held back: one shared due test, reportDueThisInterval()..." The label was added by `c6fc382` ("Add state transition observability for v15 soak", 2026-06-10) and kept by `20b4a05` ("Finished visibility - state transitions logging fixed."). Neither has a body. |
| Other modes | `Sleep:1750` ("Timer wake = scheduled report. No checks, no gates.") applies, so there is no suppression. |

## 3. "Normal operations": conditions the code checks (not a proposed definition)
| Condition | Checked before an immediate occupancy report? | Where |
|---|---|---|
| Sensor mode OCCUPANCY | yes, handler dispatch | `Generalized-Core-Counter.cpp:1720` |
| Effective mode == 3 (this includes `downgradeActive()`) | yes, the only mode test | `Idle:59`, `Modes:80,143`, `Sleep:1638,1719` |
| Battery tier | indirect only. Tier >= CONSERVING sets `lowBatteryMode`, which makes the effective mode 1. There is no tier test at report time. | `BatteryAuthorityCommand.cpp:80-83`; `PowerManager.cpp:239-246`; re-committed on wake at `Sleep:1614-1622,1683-1692` |
| Debounce | the end waits for it (`Idle:57` uses `>=`, `Modes:142` uses `>`). A start while already occupied reports nothing. | `Modes:64`, `Sleep:1694` |
| Clock trusted | the end is skipped and retried one debounce later (`stillOpen`) | `State_Common.h:328-331` |
| `state == IDLE_STATE` | the main-loop handler only transitions from Idle. In any other state it latches the flag. | `Modes:87,155` |
| Open hours / `Clock::openness()` | **not checked** for occupancy-change reports. They gate scheduled and overdue reports (`Idle:181`, `Sleep:1771`), network standby (`Sleep:372-374`, `State_Connect.cpp:441-443`) and sensor re-enable (`Sleep:1597`). | |
| `reportDueThisInterval()` | not checked. It gates only the occupied hourly report (`Idle:187`, `Sleep:1757,1777`). Reports made on occupancy change set `lastReport`, so they count (`Report:82`). | |
| Power source | no reference in the `State_*.cpp` and `State_Common.h` files (grep) | |
| Connection state | the flag is consumed only when `!Particle.connected()` (`Report:163,216-222`). If already connected it goes `already connected` (`Report:275`) and the flag stays set. Config invalid forces CONNECTING (`Report:158`). The unstable-modem deferral does not apply to the occupancy branch (`Report:233`, `Report:246` only). | |

## 4. History (§12.1)
| Date | Commit | Release | What changed |
|---|---|---|---|
| 2026-02-05 | `217a4ae` | 3.23 | Modes handler sets `state=REPORTING_STATE` on start and end, only for mode 3. CHANGELOG 3.23/3.24: "In DISCONNECTED_KEEP_ALIVE mode, device immediately reports when occupancy state changes". |
| 2026-02-09 | `eda6b7e` | 3.24 | The sleep-wake path reported start and end in **every connection mode**, with no mode test. The Idle path was mode-3-only from the start; its comment (`Idle:66-68`) says other modes "do not force an immediate report/connect". |
| 2026-03-13 | `8843d41` | 3.29 | Battery downgrade from mode 3 to 1 at tier >= CONSERVING added. Its comment: "Use KEEP_ALIVE for instant occupancy change reporting". |
| 2026-03-18 | `98d98d8` | 4.05 | Added `if (state == IDLE_STATE)` around `state = REPORTING_STATE` in `Modes`; the flag still latches (message has no body). Before this, the transition happened from any state. |
| 2026-04-24 | `737ceb3` | 9.00 | **Removed the all-modes immediate report on the sleep-wake path** for both start and end. It added the comment "Match the rest of the occupancy state machine: only KEEP_ALIVE mode forces an immediate report/connect on occupancy transitions." and a log "no immediate report (connectionMode=%d)". The message has no body and CHANGELOG 9.00 does not mention it. |
| 2026-05-22 | `c254467` | 11.0.0 | `reportNow` variable and `Occ:` log lines (log-only refactor; the `report=` field is not a stored flag). |
| 2026-10-02 | `691b6c7` | v33 | `!reportDueThisInterval()` added to the occupied suppression (section 2). |

- **End:** an immediate report on occupancy end in all modes existed only on the sleep-wake path (`eda6b7e` to `737ceb3^`). No awake path ever had it.
- **Start:** same window and commit, same wake path.

Searches run:
- `git log -S` on: `occupancyChangeTriggered`, `suppress-report`, `sleep-pir-occupancy-report`, `sleep-occupancy-debounce-report`, `occupancy transition`, `occupancy cleared`, `returnToSleepAfterReport`, `reportDueThisInterval`, `Match the rest of the occupancy state machine`, `only KEEP_ALIVE mode`, `only reported on 0->1`, `suppressing scheduled report`, `no immediate report`, `state == IDLE_STATE` (State_Modes).
- `-G` on: `report=1` (no hits), `reportNow`, `logOccupiedEvent|logUnoccupiedEvent`, `set_occupied\(false`, `closeOccupancySessionSafely`, `LED timeout expired`, `suppress periodic|occupied-suppress`, `INTERMITTENT_KEEP_ALIVE|DISCONNECTED_KEEP_ALIVE|connectionMode`, `occupancyDebounceMs|debounceMs|setting1`.
- `git show` of `State_Modes.cpp` at `217a4ae`, `eda6b7e`, `8d8ca46`, `98d98d8`, `c254467`, `88721ff`, `a95e284` and `f129d91`.

## 5. Bench evidence mapped to rules
| # | Observation | Rule that produced it | Tag |
|---|---|---|---|
| 1 | `Occ: state=0 ... report=1`, then `Sleep: ULP ... scheduled`. The report went out at 18:00 as `occupancy change`. | The `Occ:` line comes from `logUnoccupiedEvent`, which prints `reportNow=1` for mode 3 (`Modes:143-146`). The Idle path (`Idle:69-71`) and the sleep-wake path (`Sleep:1647-1650`) would have transitioned to REPORTING, which is not what the log shows next. The main-loop path (`Modes:153-157`) sets `occupancyChangeTriggered` and transitions only if `state==IDLE`. The state was not Idle (most likely sleep prep), so the flag stayed latched. At 18:00 the timer wake went to REPORTING (`Sleep:1762`). `Report:216-222` consumes the flag before `daily close` (`Report:223`) and `keep alive mode` (`Report:231`), giving `occupancy change`. | OBS flag mechanics; INF that this path ran |
| 2 | `Occ: state=1 reason=pir-wake ... report=1`, then `Sleep->Report sleep-pir-occupancy-report` | `Sleep:1719-1728`. Mode 3 and a PIR wake with `!occupied` go straight to REPORTING. | OBS |
| 3 | INTERMITTENT, 1 to 0, no report | Effective mode is 1 (`downgradeActive()` needs configured mode 3, so PR #72 is irrelevant here). `reportNow=false` at `Idle:59`, `Modes:143` and `Sleep:1638`. Chip would have seen `Occ: state=0 reason=debounce session=...s total=...s report=0`. **If awake in Idle:** next `StateReq: Idle->Sleep reason=low power idle` (`Idle:210-250`); the report waits for the boundary. **If found on a debounce timer wake:** `StateReq: Sleep->Report reason=sleep-timer-report` (`Sleep:1762`), then `Report: occ=0 ...`, then `StateReq: Report->Idle reason=not aligned` (`Report:272`) unless within 30 s of a boundary. The payload is queued (`Report:69`) and not sent until the next connect (`Idle:244`). | INF |

## Open points
- Observation 1: the log gives no `StateReq:` lines before `0001536104`, so the state when the debounce expired is not shown. Sleep prep versus CONNECTING cannot be told apart from the supplied lines. I need the 60 s of `StateReq:` lines before it.
- Observation 3: the log was not captured, so which branch ran (awake Idle versus sleep timer wake) is not established.
- Whether the latched flag from a close in a non-Idle state is cleared when the device is already connected: `Report:275` leaves it set. No code clears it elsewhere (grep). A stale latch would relabel a later connect as `occupancy change`. Not tested.
- `lowBatteryMode` and tier at the time of each bench line are not in the supplied logs. Observations 1 and 2 show `report=1`, so the effective mode was 3 then.
- No `src/` change is indicated by this evidence.

## Closing
| Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |
|---|---|---|---|---|
| Evidence report | 0 | — | 0 | none run (out of scope) |

Model used: claude-sonnet-5-5.

Citations worked out from diffs or `git show`, not re-opened in the working tree:
- All historical code and comments in section 4 (`217a4ae`, `eda6b7e`, `8843d41`, `98d98d8`, `737ceb3`, `c254467`, `691b6c7`).
- The statement that PR #72 only swapped getters, and that `State_Common.h` is unchanged, come from `git diff 73f752e..79e3c5f`.
- The 3.23 CHANGELOG, 3.24 CHANGELOG and 4.05 CHANGELOG wording.

Every current-tree `src/` line cited above was re-opened and checked.
