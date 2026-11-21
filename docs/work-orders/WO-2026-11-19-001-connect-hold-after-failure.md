# WO-2026-11-19-001 (v2.1): One connect decision, with a hold after failure

**Author:** architect, 19 Nov 2026 · **Status:** DRAFT for Chip's approval (Stage 5) · **Release:** v50.2, first item (Chip's ruling, 19 Nov) · **Replaces:** v1 and v2 of this WO (unmerged). v2.1 applies the architect's Step 0 rulings (19 Nov 13:15 SGT; see "Step 0 results and rulings").
**Base:** `integration/v50-ab1805` at `eb398b8` · **Branch:** `wo/2026-11-19-001-connect-decision`.
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5–8; §4 (dispatch format; the three-level Stage 7 verdicts of 11 Nov; at most two Codex rounds); §5 (model routing); §6 (WO contents).
**Agents (§5):**
- Stage 6: **Copilot, top model, high reasoning.** This is a refactor across several files, and §5 escalates those.
- Stage 7: **Codex, top model, highest reasoning.** This is power and timing logic.
- Claude Code resolves the model IDs at dispatch.

**Evidence base:** `docs/reviews/2026-11-19-one-place-map.md` (PR #242), which gives the R/C/X/S row and flag ids below. Every citation in this WO was re-opened at `eb398b8`.

## Budget
- **`src/`: ≤ 0 net (target), with a hard cap of +10.** Above +10, stop and report: don't widen the scope.
- **Running total for the one-place path (steps 1–5):** starts at 0. This step's actual figure goes into the closing record.
- Tests aren't capped, but keep them proportionate.

## Plain goal
1. **One place decides whether a report connects:** a pure function. `State_Report` works out the trigger, asks that function, and does what it answers.
2. **After a failed connect, the device waits before trying again.** The wait is 1 h, then 2 h, then 4 h, then 6 h at most. A good connect resets it. This sets **how often** the device attempts a connect. **How long** each attempt lasts doesn't change.

Counts are still recorded on time and queued; only the upload waits.

## Principle (Chip, 19 Nov)
Facts → Decisions → Operating model → the state machine acts. This step moves the **Connect decision** for the REPORTING path into one place, and deletes the separate 30 s rule. It is step 1 of 5. Steps 2–5 come after RC1 and aren't in this WO.

## The bug (evidence)
- **Dev-09, fixed 5001, mode IKA, bad antenna** (`RECOVERY_PLAN_2026-09-26.md:243`):
  - The last good connect was 07:29Z on 18 Nov.
  - From then until the stage-2 failsafe reset at 13:03Z (140/2), an attempt started about every 11 min.
  - From 13:16Z to about 14:22Z, and from 22:00Z to about 23:48Z, attempts ran back to back again.
  - Source: the archive and Ubidots, Claude Code, 19 Nov.
- **Serial, 23:48–23:49Z** (Chip's own USB capture; these lines are not in the S3 archive):
  - `ConnSummary: ok … cellMs=521772`
  - `PPP link failed: error 5`
  - `MODEM_HEALTH: unstable reason=slow_teardown`
  - `Sleep: ULP standby=0 reason=debounce dur=70s occ=1`
  - then `INV: awake_ms=6513806 > ceiling_ms=1090000`
- **Why:** there is no failure back-off at all.
  - `modemConnectTimeoutCount` has a single reader, the latch at 2 (`power/ModemHealth.cpp:60–61`).
  - After a timeout (`state/State_Connect.cpp:608–654`, row X4), the next attempt is the next report that passes the chain (`state/State_Report.cpp:220–282`), with a full budget.
  - The 30 s defer (`state/State_Report.cpp:27`, `:199–207`) covers only rows R6 and R8. Occupancy change (R3, `:226–231`) and four other triggers skip it (map flag F1).
- **The fleet runs the same logic in v49.1.** It's not a v50 regression, but a field board with a weak radio and steady occupancy searches for most of the day.

## Approved architecture (once Chip approves)

### 1. A new pure module: `src/power/ConnectDecision.h` / `.cpp`
- **Location:** beside `ConnectivityPolicy.h`.
- **Constraints:** no Particle or Device OS calls, no globals, no logging. It must be host-testable, like `ReportingPolicyResolver::resolve()` (`reporting/ReportingPolicy.h`).
- **The call:** `ConnectDecision::Result ConnectDecision::decide(const Inputs&)`.
- **Inputs:**
  - the five chain triggers as booleans: `serviceRequest` (R2), `occupancyChange` (R3), `closeDue` (R4), `webhookHealthDue` (R5), `cadenceDue` (R8/R9). Config invalid (R1) is not an input (ruling 3);
  - `effectiveMode`;
  - `failureCount` (`uint8_t`);
  - `msSinceLastFailure`.
- **Result:**
  - `{ action: CONNECT | WAIT, reason }`. The reason is one of the **existing** outcome strings, so `StateReq` logs read as they do today.
  - On WAIT: the reason `"connect hold"` or `"not aligned"`, plus `holdRemainingMs`.
- **Rule order (first match wins).** This is the current chain without rows R6/R8:
  1. **The hold**, which applies **unless `serviceRequest`**:
     - while `failureCount ≥ 1` and `msSinceLastFailure < holdMs(failureCount)`, the answer is WAIT `"connect hold"`;
     - `holdMs(n) = HOLD_BASE_MS << (n−1)`, capped at `HOLD_MAX_MS`;
     - `HOLD_BASE_MS` = 1 h and `HOLD_MAX_MS` = 6 h, in `ConnectivityPolicy.h`.
  2. `serviceRequest` → CONNECT `"service request"`
  3. `occupancyChange` → CONNECT `"occupancy change"`
  4. `closeDue` → CONNECT `"daily close"`
  5. `webhookHealthDue` → CONNECT `"webhook health check"`
  6. mode KEEP_ALIVE → CONNECT `"keep alive mode"` (R7, kept as it is: F4 is step 5)
  7. `cadenceDue` → CONNECT `"boundary aligned"`
  8. otherwise → WAIT `"not aligned"`

### 2. `state/State_Report.cpp`
- **Keep where they are, unchanged:**
  - the webhook stage-2 ERROR exit (R0, `:141–151`). It's a recovery action, not a connect decision; it belongs to step 3.
  - the config-invalid connect (R1, `:167–170`), which runs before the connected test at `:173` (ruling 3). It's a fault recovery and belongs to step 3;
  - the battery commit (`:184–192`);
  - `returnToSleepAfterReport` (`:211–216`; flag F8 is step 2);
  - the already-connected exit (R1b, `:283–286`).
- **Replace with "build Inputs, call `decide`, act":** the `if / else if` chain (`:220–282`), and nothing else.
- **Acting:** CONNECT → `transitionTo(CONNECTING_STATE, reason)`. WAIT → `transitionTo(IDLE_STATE, reason)`.
- **Logging:** on `"connect hold"`, log once per pass: `CONNECT_HOLD: remaining=<s> failures=<n> trigger=<name>`.
- **Delete** the `ENABLE_CONNECT_DECISION_TRACE` blocks inside the chain only. **The flag stays:** Step 0 found other users (`State_Connect.cpp:267`, `:273`, `:279`; build-flags bit `0x0040` at `BuildProfile.h:304`).

### 3. Delete the 30 s defer, and what only it uses
- **The defer itself:** `MODEM_UNSTABLE_RECONNECT_DEFER_MS` (`State_Report.cpp:27`), the defer block (`:199–207`) and its two `MODEM_POLICY: reconnect deferred` logs.
- **Its field:** `session.lastTeardownEndMs`. It has no other reader, per map §1.5; Step 0 confirms. Remove its only write, at `state/SleepTeardown.cpp:243` (`:228` is an `if` that reads teardown state, not a write).
- **Three write-only fields:** `lastCloudDisconnectElapsedMs`, `lastModemOffElapsedMs` and `lastTotalTeardownElapsedMs` (`state/StateMachine.h:66–68`, writes at `state/SleepTeardown.cpp:240–242`).
- **Same PR:** update the two host tests and the stub that name these fields (Step 0 item 2).
- **Keep:**
  - `modemUnstable`, and its standby suppression (`state/State_Sleep.cpp:680`), which is unchanged;
  - the 10 s slow-teardown latch.

### 4. The failure facts (RAM, in `SessionState`)
- **`failureCount`:** reuse `modemConnectTimeoutCount`. It's already incremented at the timeout (`ModemHealth.cpp:55–58`, from `State_Connect.cpp:641`) and zeroed on success (`ModemHealth.cpp:65–69`, from `State_Connect.cpp:590`). Its latch at 2 is unchanged.
- **The count has three clears today:** `ModemHealth.cpp:47` (recovered), `:67` (success) and `:76` (daily close). **Ruling 4: the daily close always starts a fresh day.** In `clearAtDailyClose()` (`ModemHealth.cpp:71–79`), zero `modemConnectTimeoutCount` unconditionally, not only when `modemUnstable` is set. That's one line moved out of the `if`. The close report then gets its one attempt whatever the count, and the hold restarts from zero.
- **One new field, `lastConnectFailMs`:** set beside `noteConnectTimeout()` at `State_Connect.cpp:641`. It takes the place of the deleted `lastTeardownEndMs`.
- **RAM only, on purpose (map Q5):**
  - every hibernate wake and every reset starts the count fresh, so each morning has one immediate attempt;
  - after a failsafe reset, there's one attempt. The failsafe's own 6 h cooldown bounds that.
- Nothing new is persisted.

### 5. Unchanged
- connect budgets, and their 50 % rule (C1–C5). Chip, 19 Nov, citing Particle's `Cellular.connect()` guidance on the 5- and 11-minute windows;
- the six bypass sites S1–S6 (boot, OTA, and the CONNECTED reconnect at `State_Sleep.cpp:1038`): step 2;
- DISCONNECTED (F3) and KEEP_ALIVE hours and battery multiplier (F4): step 5, pinned as they are;
- the ConnectivityFailsafe ladder;
- payloads, ledgers and persisted layout.

## Step 0 (read-only, inside Stage 6: report before editing)
1. **`millis()` across ULP sleep:** the hold measures with `millis()`. Confirm it keeps counting through ULP sleep on Boron with Device OS 6.x. Cite the Particle docs, or a bench serial line, for example the uptime before and after a ULP sleep (map §1.5 notes this assumption isn't verified). **If it doesn't keep counting, stop and report.** The fallback is an RTC epoch, which is the architect's call.
2. **Readers:** re-grep the readers of `lastTeardownEndMs`, of the three write-only fields, and of `ENABLE_CONNECT_DECISION_TRACE`. Delete only what has no other reader.
3. **Row order:** confirm that R1 runs before the already-connected check, and say exactly where R1b sits relative to `:220–282`. The new call must keep today's order for every row except R6/R8.
4. **The new field:** confirm that `lastConnectFailMs` is set only on the timeout path (X4). Stage-1 and stage-2 recoveries inside one attempt (S15/S16) must not count.

## Step 0 results and rulings (19 Nov; report `wo-2026-11-19-001-step0-report.md`, Fable 5.2, read-only)
1. **`millis()` across ULP sleep: PASS.** Bench serial, Device OS 6.6.0, IKA: sleeps of 27 s to 10.5 min advance `millis()` by the full sleep, within 1–3 s of the Pi clock. For example, Dev-09 went from `0000801408` at 13:16:06Z to `0001422950` at 13:26:30Z, which is 621.5 s against 624.2 s. Particle's reference docs don't name `millis()`, so the bench evidence carries this. `millis()` is accepted as the hold's clock.
2. **Readers: PASS.** Delete the four fields, and update the tests and stub that name them in the same PR. The trace flag stays.
3. **Ruling: R1 (config invalid) stays where it is, outside `decide()`.** Moving it would change two behaviours: a connected device with invalid config would go to IDLE instead of CONNECTING, and the battery commit and `returnToSleepAfterReport` would run on that path. Neither change is wanted. R1 is a rare fault recovery; it isn't held in this step, and step 3 (Health/recovery) owns it.
4. **Ruling: the daily close clears the count always** (§4). This replaces answer 6's "daily close is held". The close report gets one attempt per day, and the hold restarts the next day. Hibernate clears RAM overnight anyway, so this only makes the rule state what happens.
5. **Accepted as they are:**
   - the exits from CONNECTING that don't count as failures (user switch, ThrashGuard tier 2, resets): they aren't attach failures;
   - a timeout reached from a bypass site (S1–S6) starts the hold for REPORTING: a failure is a failure.

## Tests (host)
1. **Characterization table first.** Encode map rows R1–R10 (without R6/R8) as a table test of `decide` with `failureCount = 0`. Every outcome string must match the map's "Outcome today" column. The old chain can't be host-tested, so Codex checks the equivalence of the old chain and the new function line by line at Stage 7.
2. **Hold rows:**
   - after failures 1–5, `occupancyChange`, `closeDue`, `webhookHealthDue` and a keep-alive report all give WAIT `"connect hold"` inside the hold;
   - each gives its own CONNECT once the hold has passed;
   - the holds are 1, 2, 4, 6 and 6 h;
   - `serviceRequest` connects during a hold.
3. **Reset:** with `failureCount = 0` there's no hold, whatever the time since the last failure. `clearAtDailyClose()` zeroes a count of 1 (with `modemUnstable` clear) as well as a count of 2 or more.
4. **Mutations** (on a scratch copy, or with a `cp` backup and restore; never `git checkout`):
   - drop the hold rule: a hold row fails;
   - remove the doubling or the cap: the sequence test fails;
   - exempt `occupancyChange` from the hold: a hold row fails.
5. **The full suite stays green.** Report a fresh Device OS build's text/data/bss against `eb398b8`.

## Bench (Chip, after Stage 7)
Use **Dev-09 as it is now:** bad antenna, mode IKA, `occ=1`, serial attached. Over one open day, the serial should show:
- `CONNECT_HOLD` lines after the first failure;
- `Connect: start` at about +1 h, then +2 h, then +4 h;
- no back-to-back attempts.

Then fit a good antenna. The first good connect clears the hold, and normal reporting resumes.

## Expected telemetry
- A failing board makes about 4–5 attempts per open day, instead of attempts back to back.
- Alert 31 is raised once per failed attempt, as today.
- A healthy board shows no change: `failureCount` stays at 0.

## Non-goals
- The bypass sites S1–S6;
- flags F3–F9;
- the budget rule (F6);
- merging the battery block, the debounce and the report-due tests;
- the idle ceiling;
- any other recovery mechanism.

## Rollback
Revert the PR. This is behaviour only: nothing persisted, nothing in the ledgers.

## Answers to map PR #242's open questions (architect, 19 Nov)
1. **The areas:** Chip's nine (Power, Time, Config, Sensor model, Health/recovery, Report cadence, Report content and delivery, Connect, Sleep). The idle ceiling and webhook supervision belong to Health/recovery. The webhook-health **trigger** is an input to Connect.
2. **Step 1 covers the REPORTING path only.** S1–S6 are step 2.
3. **DISCONNECTED:** step 5, pinned as it is today.
4. **KEEP_ALIVE hours and battery multiplier:** step 5, pinned.
5. **The hold's count:** RAM, as §4 above.
6. **Exemptions:** only `serviceRequest` is exempt from the hold. The daily close clears the count first, so the close always gets one attempt (Step 0 ruling 4). Config invalid stays outside `decide()` (Step 0 ruling 3).
7. **F5:** not confirmed, because the bench logs couldn't show mode 0. The fleet runs mode 3. Step 1 doesn't touch S6, so F5 is left for step 2.
8. **The 660 s rule above 50 %:** unchanged, by Chip's decision of 19 Nov.

## Closing record (19 Nov 2026)
**Stage 6:** Copilot (`claude-fable-5.2`), committed as `230cce7`. **Stage 7:** round 1 on `230cce7` (Codex `gpt-6-astra`, xhigh; `gpt-6.1-sol` is rejected on this account): VERIFIED WITH FIXES, one Low (the Stage 6 report's "no warning" claim; two inherited warnings from files identical to the base, corrected in the report), no finding in `src/` or the tests; head `4ad117d`. **Stage 8:** passed (architect, 19 Nov); the zero-warning clause is met (no new warnings). Backlog for steps 2-5: the two inherited warnings (`ConnectivityFailsafeTest.cpp:18`, `SensorManager.cpp:978`) and the stale trace-flag comment at `BuildProfile.h:53`.
**Controller checks on `230cce7`:** suite 125/125 (sh via zsh, py via python3, full history); Codex's independent run 125/125 and 195,840 decision cases under UBSan; twelve mutations caught. Local build text/data/bss `151116/1114/2248` against base `151124/1114/2264` (-8/0/-16).

| Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |
|---|---|---|---|---|
| Connect decision + hold | ≤ 0 (cap +10) | — | **0** (+88 / −88) | 2 new tests and 1 helper (`connect_decision_test`, `connect_hold_wiring_test`, `connect_hold_base_delta.py`); 10 existing tests updated, each no wider than its reason |

Running one-place total after this step: **0**.
**Merge:** HOLD for v50.2; merge only after v50.1 is tagged (architect, 19 Nov).
