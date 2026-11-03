# WO-2026-11-03-005: item 26 v49 durability — one flushAllBeforeReset() at the deliberate-reset sites

**Author:** Claude Code, 3 Nov 2026, DRAFT for the architect's review · **Status:** DRAFT; not dispatched
**Branch:** `wo/2026-11-03-005-item26-durability-flush`, from `main`
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-8; §4 (SHA rule); §5 (persistence across reset: Codex xhigh); §12; §13 (c), (j)
**Target:** **v49** (RC1 scope: #172 + item 25 + this + the binding-delete guard).
**Budget:** `src/` at most **+20 net**. Stop and report rather than trim style.
**Agents:** Stage 6 Copilot standard, medium. Stage 7 Codex `gpt-5.6-sol`, **xhigh**, an `UNDER REVIEW` header, `-stage7-r<N>` files, at most two rounds.

## Plain goal
A deliberate reset can lose data that was "saved": `/usr/current.dat` (occupancy, `dailyoccupancy`, open-session state; 250 ms delayed write, no forced flush), `/usr/sensor.dat` (250 ms delayed write, no forced flush) and the **RAM publish queue** (`PublishQueuePosixRK` RAM size 2; the library says a non-software-update reset loses it). Only `/usr/sysStatus.dat` has a synchronous flush (`SystemConfig::flushNow()`), and only failsafe stage 2/3 uses it (`ConnectivityFailsafe.cpp:77-87,301-314`). Found by Codex in Stage 7 round 1 of WO-2026-11-02-004 (B1, B2) and mapped in the item-26 Step 0 (`WO-2026-11-03-002-step0-report.md` §1-§5). A reset right after a day-boundary close can persist `lastDailyCleanup` while the current record lags, so the old open session or total can return, or the close credit be lost.
This WO adds **one** function that forces all of it to flash before the reset, and calls it from the sites that can run it safely. It is the durability half of the item-26 design; the single reset owner (phase state, timer) comes after the AB1805 switch.

## Architect ruling (3 Nov)
- v49 = durability: one `flushAllBeforeReset()` (force-flush **C, S, N and Q**), called at **thrash tier 3, Error soft and hard, the nightly heap guard, sleep-exhausted, and failsafe stages 2 and 3**.
- **OOM is excluded** (no flush, no warning; the immediate reset in `Generalized-Core-Counter.cpp:1208-1213` is unchanged; `tests/oom_immediate_reset_test.sh`).
- **Documented exceptions (no change):** the two hardware watchdogs (internal 60 s and AB1805), the software ApplicationWatchdog callback (`LoopForensics.cpp:395-399`), and HIBERNATE.
- **"Queue an alert" = write the alert code to the persistent store before the reset** (no publish, no queue event): the code is written into the record that `flushAllBeforeReset()` flushes.
- Phase state, timer, deadline and the single owner: later, after the AB1805 switch (likely post-RC1).

## The change (re-check at Step 0; lines are from main `21804e3`)
1. **Facades** for the two stores that lack one: a synchronous flush for `CurrentReadings` (`persist/CurrentReadings.h:20-28`, definition beside `CurrentReadings::loop()`, `MyPersistentData.cpp:1103-1108`) and for `SensorSettings` (`persist/SystemConfig.h:223-253`, `MyPersistentData.cpp:1010-1033`), each forwarding to the store's `flush(true)` exactly as `SystemConfig::flushNow()` does (`MyPersistentData.cpp:914-919`). About +4 lines.
2. **`flushAllBeforeReset()`** (one place, e.g. `SystemConfig.h/.cpp` or a small header the reset sites already include): `CurrentReadings` flush, `SystemConfig::flushNow()`, `SensorSettings` flush, then the publish queue's synchronous `writeQueueToFiles()` (`PublishQueuePosixRK.h:231-234`). Order: stores first, queue last (the queue file write is the longest). No allocation. About +6-8 lines.
3. **Seven call sites**, one line each, immediately before the reset or power-down: `ThrashGuard.cpp:149-152`, `State_Error.cpp:153-170` (soft and hard), `State_Sleep.cpp:630-649` (heap guard), `State_Sleep.cpp:892-909` (sleep exhausted), `ConnectivityFailsafe.cpp:301-314` (stage 2 and stage 3; the existing `SystemConfig::flushNow()` there is replaced by the new call, and the stage/action/count record must still be written **before** the flush). About +7 lines.
4. Alerts that a site raises just before its reset (thrash tier 3's alert 18, the Error alert) are written to the store **before** `flushAllBeforeReset()` so they are in the flushed record.

## Protected / not in scope
OOM, the watchdog callbacks and HIBERNATE; the Error state's behavior (suppression policy, the 30 s dwell); the failsafe ladder (stage values, cooldowns, TEST_MODE); the persisted layout and `retained` RAM (zero change); `deepPowerDown()`'s internal reset fallback (stays; native `POWER_OFF` replaces it at the AB1805 switch); `lib/PublishQueuePosixRK` (no library change); hibernate ordering (`HibernateCycle::armForSleep()`).

## Time budget (provisional; no latency is measured in the repo)
Each flush is synchronous with no cancel. The total worst case must stay well below the 60 s internal watchdog; the bench measures the real figure (below) and the WO records it. Proposed ceilings from the Step 0: each store 250 ms, queue 2 s. If a measured flush exceeds them, report; do not add a timeout in this WO.

## Tests (host; the real functions)
- **Order and presence:** each of the seven sites calls `flushAllBeforeReset()` before the reset/power-down, after its own record writes; OOM does not call it (structural test next to `oom_immediate_reset_test.sh`).
- **What it flushes:** a fake backend records C, S, N, Q flush events: all four, stores before queue.
- **Failsafe:** stage/action/count written, then flush, then reset; `ConnectivityFailsafe` harnesses stay green.
- **Mutations (each must fail):** drop each of the four flushes; reverse queue/store order; remove the call at each of the seven sites in turn; add a call at the OOM site; call it before the stage record is written.
- `nm` on the local-toolchain ELF: `flushAllBeforeReset` and its seven production call sites.

## Acceptance and bench (Codex xhigh; Dev-14)
Dirty all three stores and hold two RAM events (the Step 0 plan): trigger each reachable site connected and offline (failsafe TEST_MODE stage 2 then 3; Error soft; the nightly heap guard if reachable); after each reboot verify `dailyoccupancy`, the open-session state and the sensor settings are the pre-reset values and the two queued events are published or in the file queue; verify each `resetReasonData`; record the measured flush durations; then two nightly hibernates and one OTA-in-progress to confirm no change there.

## Rollback
Revert the PR; v48 behavior returns; no data to migrate.

## Open questions for the architect
1. Where does the function live (`persist/` header vs a new `src/` file)? Recommendation: beside `SystemConfig::flushNow()`.
2. Thrash tier 3 and the Error state raise an alert code and reset in the same pass: confirm the alert code's store (`RecoveryState`, in `sysStatus` or `current`?) is one of the three stores flushed (Step 0 must confirm).
3. Does the failsafe keep its own `SystemConfig::flushNow()` call as well (belt and braces) or only the new one? Recommendation: only the new one, called after the stage record.
