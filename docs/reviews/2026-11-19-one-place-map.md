# One-place map, Step 0 (base eb398b8)

Read-only source analysis. All citations are `path:line` under `src/` and were opened before citing. Nothing here was checked against device logs.

**The nine areas below are my proposal, taken from the code, for the architect to correct:** (1) Connect, (2) Sleep and hibernate, (3) Clock and opening hours, (4) Occupancy and sensing, (5) Reporting and publish, (6) Power and battery, (7) Config and ledger, (8) Failsafe, watchdog and recovery, (9) Observability.

---

## Part 1. The Connect decision today

### 1.1 Headline

- There are 13 `transitionTo(CONNECTING_STATE, ...)` call sites and 4 direct `Particle.connect()` sites. Seven of the 13 pass through REPORTING_STATE; six bypass it and apply no gate at all.
- The gates in REPORTING are one `if / else if` chain (`state/State_Report.cpp:220-282`). Order decides the outcome: service request, occupancy change, daily close, webhook health, keep alive, cadence due, else not aligned.
- **There is no failure back-off today.** `modemConnectTimeoutCount` has one reader: at 2 or more it sets `modemUnstable` (`power/ModemHealth.cpp:60-61`). 0 and 1 change nothing; 3+ is the same as 2. After a failed connect the next attempt is simply the next report that passes the chain, with a full 5 or 11 minute budget.
- `modemUnstable` has two effects only: standby is suppressed (`state/State_Sleep.cpp:680`), and the 30 s defer applies to two of the seven REPORTING branches (`state/State_Report.cpp:242`, `:254`).
- CONNECTING_STATE itself checks nothing about mode, hours or modem health. It only picks a budget.

### 1.2 Every site that starts a connection

| # | Site | Reason string | Via REPORTING? | Condition at the site | Gates applied |
|---|---|---|---|---|---|
| S1 | `Generalized-Core-Counter.cpp:493` | `boot button override` | No | Button low at boot | None |
| S2 | `Generalized-Core-Counter.cpp:631` | `OTA update` | No | Reset reason is UPDATE | None |
| S3 | `Generalized-Core-Counter.cpp:905` | `invalid config` | No | `!Config::isValid(true)` at boot | None |
| S4 | `Generalized-Core-Counter.cpp:953` | `time invalid` or `no confirmed time sync ever (Finding 3)` | No | `!Clock::isTimeValid()` or `lastTimeSync == 0` (`:951-952`) | None |
| S5 | `Generalized-Core-Counter.cpp:1010` | `sensor init failed` | No | Open (fail-open test) and sensor not ready | None |
| S6 | `state/State_Sleep.cpp:1038` | `sleep-open-hours-reconnect` | No | ULP wake, not button; `isWithinOpenHours()` (fail-open, `:1006`); effective mode CONNECTED; not connected (`:1036`) | None (no health, no count) |
| S7 | `state/State_Report.cpp:169` | `config invalid` | Yes, before the chain | `!Config::isValid(true)` (`:167`); runs even when already connected | None |
| S8 | `state/State_Report.cpp:225` | `service request` | Yes | see R2 | |
| S9 | `state/State_Report.cpp:231` | `occupancy change` | Yes | see R3 | |
| S10 | `state/State_Report.cpp:234` | `daily close` | Yes | see R4 | |
| S11 | `state/State_Report.cpp:239` | `webhook health check` | Yes | see R5 | |
| S12 | `state/State_Report.cpp:251` | `keep alive mode` | Yes | see R6/R7 | |
| S13 | `state/State_Report.cpp:266` | `boundary aligned` | Yes | see R8/R9 | |
| S14 | `state/State_Connect.cpp:371` | (the one real connect) | n/a | First pass of every CONNECTING entry | Budget only |
| S15 | `state/State_Connect.cpp:92` | CloudRecover stage 1 | n/a | 60 s in CLOUD_ACQUIRE (`:382`) | Inside an attempt |
| S16 | `state/State_Connect.cpp:98-99` | CloudRecover stage 2 (`Cellular.on()` + connect) | n/a | 120 s in CLOUD_ACQUIRE, after stage 1 (`:394-395`) | Inside an attempt |
| S17 | `state/State_Connect.cpp:681` | FIRMWARE_UPDATE entry | No | Not connected on entry to FIRMWARE_UPDATE_STATE (`:680`) | None |

Not connect sites: the connectivity failsafe resets or powers off (`power/ConnectivityFailsafe.cpp:319`, `:329`) and the next boot goes to IDLE; ledger code never calls connect (grep of `cloud/` found none). Boot sets the state several times; the last call wins, and `Generalized-Core-Counter.cpp:1075-1076` only sets IDLE if nothing else did.

How REPORTING is reached (these feed R1-R12): Idle scheduled report when `openness() != Closed` (`state/State_Idle.cpp:189-208`); Idle latched occupancy change (`:79-80`, `:61-63`); ULP timer wake, "no gates" (`state/State_Sleep.cpp:1054-1064`); PIR wake with an overdue report (`:1073-1086`); wake button (`:999-1000`); close due before night sleep (`:506-507`); occupancy pending in Sleep (`:1218-1220`); occupancy from the wake path (`state/State_Modes.cpp:171`, `:255`) and the main loop (`:94`, `:149`); user switch from any state (`Generalized-Core-Counter.cpp:1188-1192`).

### 1.3 Decision table: REPORTING_STATE (first matching row wins)

Dimensions: **mode** is the effective mode (`power/PowerManager.cpp:225-227`); it differs from the configured mode only when OCCUPANCY + configured KEEP_ALIVE + `lowBatteryMode` gives INTERMITTENT (`:219-222`). **Hours**: three different tests are used, named per row. **Unstable** = `session.modemUnstable`, from slow teardown over 10 s with count 0 (`state/SleepTeardown.cpp:244-245`, `power/ModemHealth.h:4`), from a teardown budget overrun (`state/SleepTeardown.cpp:199`, `:320`), or from count >= 2 (`power/ModemHealth.cpp:60`). **30 s** = `millis() - session.lastTeardownEndMs < 30000` with `lastTeardownEndMs != 0` (`state/State_Report.cpp:201-206`, constant `:27`). **Count** never matters by itself in any row.

| Row | Trigger | Mode | Hours | Unstable / 30 s / count | Outcome today | Deciding line | Test? |
|---|---|---|---|---|---|---|---|
| R0 | Webhook stage 2: alert 40 active, age > 6 h, connected within 2 h, 3 h cooldown passed | any | `Clock::openness()==Open` only | not checked | ERROR_STATE `webhook long-term failure`; return before any connect decision | `state/State_Report.cpp:141-151` | yes |
| R1 | Config invalid | any | any | not checked | CONNECTING `config invalid`, even if already connected | `state/State_Report.cpp:167-170` | yes |
| R1b | Already connected | any | any | not checked | IDLE `already connected`; stamps `lastConnection` | `state/State_Report.cpp:283-286` | yes |
| R2 | Service request (button) | any, including DISCONNECTED | any | not checked | CONNECTING `service request` | `state/State_Report.cpp:220-225` | yes |
| R3 | Occupancy change | flag is only set in KEEP_ALIVE or CONNECTED (`state/State_Common.h:56-58`) | new detections dropped when `openness()==Closed` (`state/State_Common.h:63`, `state/State_Modes.cpp:66`) | **not checked: no defer** | CONNECTING `occupancy change` | `state/State_Report.cpp:226-231` | yes (FLAG F1) |
| R4 | Daily close due | any, including DISCONNECTED and any battery tier | needs a trusted clock (`time/DailyBoundary.cpp:34`) | not checked; `clearAtDailyClose()` has already run (`state/State_Report.cpp:87`) | CONNECTING `daily close` | `state/State_Report.cpp:232-234` | yes |
| R5 | Webhook health: no hook response > 3 h, not awaiting, and `lastConnection` is 0 or older than 30 min | any | `openness()==Open` and not `suppressAlert40ThisSession` (`:113`) | not checked | CONNECTING `webhook health check` | `state/State_Report.cpp:121-134`, `:235-239` | yes (FLAG F7) |
| R6 | Any other report | KEEP_ALIVE | **not checked** | unstable AND inside 30 s | IDLE `modem unstable`; log `trigger=keep_alive`; then Sleep `low power idle` (`state/State_Idle.cpp:258`) | `state/State_Report.cpp:240-245` | yes (deleted in Step 1) |
| R7 | Any other report | KEEP_ALIVE | **not checked**; battery cadence multiplier not applied | stable, or unstable outside 30 s; count irrelevant | CONNECTING `keep alive mode` | `state/State_Report.cpp:246-251` | yes (FLAG F4) |
| R8 | Cadence due | CONNECTED, INTERMITTENT, DISCONNECTED, or downgraded KEEP_ALIVE | `isWithinOpenHoursAt(now)` on the raw clock, fail-open (`time/Clock.cpp:61-64`); then untrusted clock = always due, trusted = within 30 s of the interval x tier multiplier boundary (`reporting/ReportingPolicy.cpp:82-83`) | unstable AND inside 30 s | IDLE `modem unstable aligned`; log `trigger=aligned` | `state/State_Report.cpp:253-257` | yes (deleted in Step 1) |
| R9 | Cadence due | as R8 | as R8 | stable, or unstable outside 30 s; count irrelevant | CONNECTING `boundary aligned` | `state/State_Report.cpp:258-266` | yes (FLAG F3, F5) |
| R10 | Cadence not due (closed on raw clock, or trusted and off the boundary) | as R8 | as R8 | not checked | IDLE `not aligned`; report stays queued | `state/State_Report.cpp:268-281` | yes |

Side effects on the way through the chain, whatever the row: `returnToSleepAfterReport = true` when occupied and effective mode is not CONNECTED (`state/State_Report.cpp:211-216`); battery tier and `lowBatteryMode` are committed (`:184-192`), so the effective mode can change inside this pass before `:240` reads it.

After an IDLE outcome: effective mode not CONNECTED goes to Sleep `low power idle` (`state/State_Idle.cpp:218-258`); CONNECTED stays awake unless `openness()==Closed` (`:111-130`).

### 1.4 Decision table: CONNECTING_STATE entry and exit

| Row | Condition | Outcome | Deciding line | Test? |
|---|---|---|---|---|
| C1 | Configured budget 120-900 s | Base budget = configured | `state/State_Connect.cpp:118-120` | yes |
| C2 | Configured budget 0 or > 900 | Base = 300 s, silent | `state/State_Connect.cpp:107`, `:117-125` | yes |
| C3 | Configured budget 1-119 | Base = 300 s, warning | `state/State_Connect.cpp:121-124`, `:258-260` | no |
| C4 | `connectionAttemptCounter >= 3` | Budget 660 s (overrides C1-C3); counter reset to 0 | `state/State_Connect.cpp:129-134`, `:266-271` | yes |
| C5 | SoC > 50 % and counter < 3 | Budget 660 s (overrides C1-C3); counter untouched | `state/State_Connect.cpp:131`, `:272-277` | yes (FLAG F6) |
| C6 | Mode, hours, modem health, DISCONNECTED | **Not checked anywhere in Connect** | whole of `state/State_Connect.cpp:202-304` | n/a (FLAG F3) |
| X1 | Cloud connected, updates pending | FIRMWARE_UPDATE `updates-pending` | `state/State_Connect.cpp:596-597` | no |
| X2 | Cloud connected, `returnToSleepAfterReport` | Sleep `return-to-sleep-after-report`; flag cleared | `state/State_Connect.cpp:598-601` | yes (FLAG F8) |
| X3 | Cloud connected, otherwise | IDLE `connect-complete` | `state/State_Connect.cpp:603` | yes |
| X4 | `elapsedMs > budgetMs` | Count++, alert 31, cloud disconnect + radio off, attempt counter++, Sleep `connect-timeout` | `state/State_Connect.cpp:608-654` | yes |

Rows: 12 in REPORTING (R0-R10 plus R1b) and 10 in CONNECTING, 22 in total. Every distinct outcome string in the two files appears once.

### 1.5 Silent facts for Step 1

After a failed connect (`state/State_Connect.cpp:641-654`):
- `noteConnectTimeout()` increments the RAM count and marks unstable at 2 (`power/ModemHealth.cpp:55-63`).
- `raiseAlert(31)` only replaces an alert of lower severity (`MyPersistentData.cpp:817`); 31 is tier 2 (`:797`), so an active 40, 41, 42 or 43 hides it. Alert 31 is cleared on the next success only if it is the current code (`state/State_Connect.cpp:518-520`).
- The modem is turned off (`power/Connectivity.h:38-41`). Standby is not used for the following sleep because the cycle result is not SUCCESS (`state/State_Sleep.cpp:673-677`).
- The persisted `connectionAttemptCounter` goes up on failure (`state/State_Connect.cpp:649-652`) **and on success** (`:512-515`). It is a "every fourth attempt" counter, not a failure counter.
- The next attempt comes at the next report that passes the chain. In a sleeping mode that is the next interval boundary (`state/State_Sleep.cpp:575-585`). In CONNECTED mode the sleep is aborted straight back to IDLE (`:1160-1163`) with the modem off; see F5.
- Alert 31 never enters ERROR_STATE by itself. `resolveErrorAction()` handles 31 only if ERROR_STATE was entered for another reason (`state/State_Error.cpp:46-56`), and non-CONNECTED modes clear it there (`:135-141`).

What resets the counts:
- `noteConnectSuccess()` zeroes the count but leaves `modemUnstable` set (`power/ModemHealth.cpp:65-69`).
- `maybeClearModemUnstable()` clears unstable and the count only when this wake cycle's connect result is SUCCESS and teardown took 5 s or less (`power/ModemHealth.cpp:34-53`). A teardown of 5-10 s neither marks nor clears (`state/SleepTeardown.cpp:244-248`).
- `clearAtDailyClose()` clears everything, but only if unstable was set; a count of 1 survives it (`power/ModemHealth.cpp:71-79`).
- Any reset or hibernate wake clears all of it, because it is RAM.

RAM-only (lost on reset and on every hibernate wake, kept across ULP sleep): all of `SessionState`, including `modemUnstable`, `modemUnstableReason`, `modemConnectTimeoutCount`, `lastTeardownEndMs`, `returnToSleepAfterReport`, `serviceRequestTriggered`, `occupancyChangeTriggered` (`state/StateMachine.h:49-73`); `connectedStartMs` (`Generalized-Core-Counter.cpp:155`); the wake-cycle `connect_result` (`observability/WakeCycleStats.h:49`).

Persisted (file-backed `sysStatusData`, `MyPersistentData.h:65`): `connectionMode` (`:146`), `connectAttemptBudgetSec` (`:149`), `connectionAttemptCounter` (`:155`), `lastConnection` (`:131`), `lowBatteryMode` (`:125`), `currentBatteryTier` (`:154`), `lastTimeSync` (`:142`). `alertCode` is persisted in `currentStatusData` (`:665`).

The 30 s window is narrow in practice. `lastTeardownEndMs` is written only when a teardown that Sleep itself requested completes (`state/SleepTeardown.cpp:228`, `:243`), and the device normally sleeps straight after. The defer can fire only when a keep-alive or aligned report runs within 30 s of that: a nap shorter than 30 s, a PIR wake with an overdue report, or the counting-LED return to IDLE (`state/State_Sleep.cpp:597`). This assumes `millis()` advances during ULP sleep, as the code itself does (`state/State_Sleep.cpp:813`); not verified against Device OS.

With `ENABLE_CONNECT_DECISION_TRACE` at its default of 0 (`BuildProfile.h:153-154`), the only log of a connect decision is the `StateReq` reason string (`Generalized-Core-Counter.cpp:1431-1434`) plus the two `MODEM_POLICY: reconnect deferred` lines.

### 1.6 Flags: accidental or contradictory behaviour

- **F1. The defer is applied to two branches and skipped by five.** Occupancy change, service request, daily close and webhook health connect inside the 30 s window on an unstable modem; keep alive and aligned do not. The six bypass sites (S1-S6) skip it too. Nothing in the code says why occupancy is exempt.
- **F2. The failure count is not a back-off.** It is a one-step latch at 2 that feeds F1 and standby suppression. Slow teardown reaches the same latch with a count of 0. Step 1's single back-off rule replaces behaviour that does not exist today, so its characterization test must pin "retry at the next cadence boundary with a full budget" as the baseline.
- **F3. DISCONNECTED is not honoured.** `persist/SystemConfig.h:51` says "Stay offline unless manually overridden". In REPORTING it takes the same rows as INTERMITTENT (R2, R4, R5, R8-R10) and Connect has no check. The only code that treats it differently is the failsafe supervisor, which switches itself off (`power/ConnectivityFailsafe.cpp:186-191`).
- **F4. KEEP_ALIVE ignores hours and battery cadence.** R7 has no open-hours test and sits ahead of the cadence test, so the tier multiplier (`cloud/BatteryBackoffPolicy.h:45-57`) never applies. The battery downgrade to INTERMITTENT exists only in OCCUPANCY mode (`power/PowerManager.cpp:219-222`), so a COUNTING device in KEEP_ALIVE connects on every report at any tier.
- **F5. CONNECTED mode has no reconnect rule once the application has turned the modem off.** After X4, an idle-ceiling trip (`state/State_Idle.cpp:332`) or ThrashGuard tier 2 (`ThrashGuard.cpp:142`), the device sits in IDLE offline. Idle reports on elapsed time since `lastReport` (`state/State_Idle.cpp:201`), but R9 needs the epoch boundary within 30 s when the clock is trusted, so most reports land on R10. Reconnect then waits for an occupancy change, the daily close, R5 after 3 h, or the failsafe reset. Related, as read and not confirmed in logs: the idle ceiling measures from `connectedStartMs`, which is the time of the last successful connect (`state/State_Idle.cpp:319`), so a cloud drop in CONNECTED mode with the radio still on looks over budget on the first Idle pass.
- **F6. "Periodic deep attempt" is the normal case.** Above 50 % SoC every attempt gets 660 s and the configured budget is ignored (C5). Below 50 % it is three attempts at base then one deep, counting successes as well as failures.
- **F7. The webhook 30-minute back-off does not back off failures.** It keys on `lastConnection`, which only moves on success (`state/State_Connect.cpp:492`, `state/State_Report.cpp:285`), so after a failed connect R5 fires again on the next report.
- **F8. `returnToSleepAfterReport` goes stale.** It is set on every report while occupied (`state/State_Report.cpp:216`) and cleared only by X2. A timeout, a defer or an R10 leaves it set for a later, unrelated connect.
- **F9. Modem recovery reads an observability field.** `maybeClearModemUnstable()` and the standby decision both branch on `Observability::cycleStats().connect_result` (`power/ModemHealth.cpp:39`, `state/State_Sleep.cpp:676`). That is a Connect fact stored in the diagnostics struct.

---

## Part 2. The nine-area map

**1. Connect.** Facts: modem health and count (`state/StateMachine.h:59-65`), persisted mode, budget, attempt counter, `lastConnection` (Part 1.5). Decisions, scattered over six files: the chain (`state/State_Report.cpp:167-287`), budget (`state/State_Connect.cpp:115-137`), health latch (`power/ModemHealth.cpp`), CONNECTED reconnect (`state/State_Sleep.cpp:1036`), boot forces (`Generalized-Core-Counter.cpp:493-1010`), idle ceiling (`state/State_Idle.cpp:263-340`). Fit: good; one function taking trigger, mode, openness, health and count would replace the chain. Consumers: State_Report, State_Connect, State_Sleep, State_Idle. Delete candidates: the defer block and constant (`state/State_Report.cpp:27`, `:199-207`, `:242-246`, `:254-257`), `lastTeardownEndMs` (no other reader), the write-only `lastCloudDisconnectElapsedMs`, `lastModemOffElapsedMs`, `lastTotalTeardownElapsedMs` (`state/StateMachine.h:66-68`, `state/SleepTeardown.cpp:240-242`), the dead `Particle.connect()` in S17 if FIRMWARE_UPDATE is only ever entered connected.

**Backlog (Connect / health, 26 Nov 2026, after RC1):** modem-cycle step plus a `cloud_status` signal; revisit only if session loss appears on non-BRN402 units after the swaps. Particle support (26 Nov): a modem power cycle (`Cellular.off()` / `Cellular.on()`) should recover a failed session, and Device OS gives no handshake notification but the `cloud_status` system event can be read. Context: the retired failsafe stage 1 was a radio reset (WO-2026-10-02-001 item B); no code until the trigger appears.

**2. Sleep and hibernate.** Facts: standby return state (`state/State_Sleep.cpp:662`), teardown state (`state/SleepTeardown.cpp`), hibernate eligibility (`time/HibernateEligibility.cpp:25`). Decisions: sleep plan (`state/State_Sleep.cpp:490-626`), standby (`:664-687`), hibernate or ULP (`:689-746`), post-wake routing (`:990-1102`), when to sleep (`state/State_Idle.cpp:127-130`, `:218-260`). Fit: plan and standby are already close to pure functions; post-wake routing mixes sleep, reporting and connect decisions. Consumers: State_Sleep, State_Idle. Delete candidate: the dead condition `state/State_Idle.cpp:219-228` (its own comment says it can never be true).

**3. Clock and opening hours.** Facts: trust (`time/Clock.cpp:475`), tri-state openness (`:565-582`), fail-open hour tests (`:43-59`, `:61-71`), daily boundary (`time/DailyBoundary.cpp:26-46`). Decisions: one owner for the facts, but each caller picks its own test and its own meaning for Unknown: `state/State_Idle.cpp:127`, `:189`, `:295`; `state/State_Sleep.cpp:504`, `:1006`, `:1073`, `:1148`, `:1160`; `state/State_Report.cpp:113`; `power/ConnectivityFailsafe.cpp:240`; `reporting/RuntimeReportingPolicy.cpp:51`. Fit: the facts are in place; the Unknown polarity belongs in each Decision, stated once. Consumers: all State files.

**4. Occupancy and sensing.** Facts: occupied, start time, last event (`MyPersistentData.h:675`, `state/StateMachine.h:70-72`), sensor readiness (`sensors/SensorManager.cpp`). Decisions: duplicated. Debounce close exists three times (`state/State_Idle.cpp:48-66`, `state/State_Modes.cpp:130-152`, `:158-174`); session open twice (`state/State_Modes.cpp:68-96`, `:220-257`); "occupied + KEEP_ALIVE reports only when due" three times (`state/State_Idle.cpp:193-195`, `state/State_Sleep.cpp:1058-1059`, `:1077-1079`). Only `reportsOccupancyChangesNow()` is single (`state/State_Common.h:56-59`). Fit: strong merge candidate. Consumers: State_Modes, State_Idle, State_Sleep, State_Report.

**5. Reporting and publish.** Facts: `lastReport`, interval, queue depth, webhook flags (`state/StateMachine.h:53-55`). Decisions: four different "is a report due" tests: elapsed (`state/State_Idle.cpp:201`), elapsed with `lastReport > 0` (`state/State_Sleep.cpp:1085`), same interval bucket (`state/State_Common.h:39-43`), epoch boundary (`reporting/ReportingPolicy.cpp:19-26`). Webhook supervision is in three places (`state/State_Report.cpp:112-156`, `Generalized-Core-Counter.cpp:1164-1176`, `state/State_Sleep.cpp:150`). Fit: "report due" and "connect due" should be two named decisions; today the first is implicit in who calls REPORTING. Consumers: State_Report, State_Idle, State_Sleep.

**6. Power and battery.** Facts: SoC and tier (`MyPersistentData.h:667`, `:154`), `lowBatteryMode` (`:125`), input profile (`power/PowerManager.cpp:95-205`). Decisions: tier is one pure function (`power/BatteryAuthority.cpp:17-62`); commit is one function (`power/BatteryAuthorityCommand.cpp:53-100`) but its seven-line call block is pasted four times (`state/State_Report.cpp:184-192`, `state/State_Sleep.cpp:1023-1032`, `state/State_Modes.cpp:209-218`, `Generalized-Core-Counter.cpp:1054-1068`). Fit: already Facts then Decisions; one wrapper saves about 25 lines. Consumers: State_Report, State_Sleep, State_Modes, State_Connect (SoC for budget).

**7. Config and ledger.** Facts: persisted config (`MyPersistentData.h:118-181`), validity (`Config.cpp:108-134`), ledger sync state (`cloud/LedgerClient.cpp:90-99`). Decisions: apply (`cloud/ConfigApply.cpp:122`), load on connect (`state/State_Connect.cpp:536`) and again in FIRMWARE_UPDATE (`:695`), invalid-config forces at boot and in REPORTING (S3, S7). Fit: config is an input to every decision; little to restructure beyond the duplicate load. Consumers: State_Connect, State_Sleep (gate).

**8. Failsafe, watchdog and recovery.** Facts: alert code, reset count, recovery stage (`persist/RecoveryState.h:84-94`), boot-storm counters (`Generalized-Core-Counter.cpp:204`). Decisions, scattered: failsafe supervisor (`power/ConnectivityFailsafe.cpp:185-332`), ThrashGuard tiers (`ThrashGuard.cpp:92-154`), error actions (`state/State_Error.cpp:33-113`, `:123-151`), idle ceiling (`state/State_Idle.cpp:263-340`), alert severity (`MyPersistentData.cpp:767-831`). Four mechanisms can each turn the modem off or reset on their own clock. Fit: a single "recovery action" decision is possible but is the riskiest area to merge. Consumers: all State files, main loop.

**9. Observability.** Facts: wake-cycle stats (`observability/WakeCycleStats.h`), loop forensics, breadcrumbs. Decisions: should be none, but see F9. Fit: write-only sink once `connect_result` moves to Connect. Consumers: State_Connect, State_Sleep, device status publisher.

---

## Part 3. Open questions for the architect

1. Are these the nine areas you meant? In particular, is the idle ceiling Connect or Recovery, and is webhook supervision Reporting or Recovery?
2. Should Step 1's Connect decision also own the six bypass sites (S1-S6), or only replace the REPORTING chain? The CONNECTED reconnect at S6 is the one that matters at run time.
3. DISCONNECTED (F3): make the decision honour it, or record that it means "INTERMITTENT without the failsafe"?
4. KEEP_ALIVE (F4): should hours and the battery multiplier apply? Today they do not, and Step 1 will have to state it either way.
5. The back-off rule: keyed on the RAM count (lost at every hibernate wake) or on something persisted? Today nothing about failures survives a reset except the attempt counter, which also counts successes.
6. Should the back-off cover service request and daily close, or are those always exempt as today?
7. F5 (CONNECTED mode offline until a boundary happens to line up): confirm from fleet logs before Step 1, since the new single decision will either fix or freeze it. Search for `IDLE ceiling trip` and `not aligned` on CONNECTED devices.
8. Budget (F6): is "660 s whenever SoC > 50 %" intended? If Step 1 touches the budget, the configured value is currently dead above 50 %.
