# WO-2026-11-03-002: one safe-reset owner (recovery item 26): Step 0 (read-only)

**Author:** Claude Code, 3 Nov 2026, from Chip's design (3 Nov) · **Status:** dispatched on Chip's direction
**Branch:** `wo/2026-11-03-002-item26-step0`, from `main` (`21804e3`, v48)
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 4 (independent investigation); §12.1, §12.5; §13 (c), (j)
**Agent:** Codex `gpt-5.6-sol`, **high**, read-only, stdin closed, new output file `WO-2026-11-03-002-step0-report.md`, at most 150 lines.

## Plain goal
Stage 7 of WO-2026-11-02-004 (round 1) found that `/usr/current.dat` (250 ms delayed write) and the RAM publish queue are not guaranteed durable across a deliberate reset, and that the failsafe stage-2 reset has the same gap. **Chip's design:** the Error state becomes the only place that calls `System.reset()` or `deepPowerDown()`. Its sequence, each step time-limited: (1) publish a warning if connected, else queue an alert; (2) flush every store (current, sysStatus, sensor) and the publish queue to persistent memory; (3) disconnect from the cloud; (4) modem off; (5) reset or power down with the reset cause. Every other site **requests** a reset with its cause. The hardware watchdog stays as the backstop for a hung teardown. This Step 0 decides whether and how, with no code change.

## Questions (observation with file:line, kept apart from inference)
1. **Site map.** Every `System.reset`, `System.dfu`-style reset, `NVIC`/watchdog-triggered reset and `deepPowerDown`/power-down on current `main` (architect's older count: 7 resets, 2 power-downs in `Generalized-Core-Counter.cpp`, `ThrashGuard.cpp`, `LoopForensics.cpp`, `ConnectivityFailsafe.cpp`, `State_Error.cpp`, `State_Sleep.cpp`; re-count by bare-name search, §13 (c)). Also the sleep/hibernate entries that end in a reset-like wake.
2. **Per site: what is flushed or torn down today** (current, sysStatus, sensor stores; publish queue; cloud/modem), and the call order.
3. **Per site: can it route through the owner?** Especially out-of-memory (is there memory to run a teardown?), the application watchdog, the thrash guard, and any reset from a context where a blocking teardown is unsafe (ISR-like, system thread, during OTA). What does each need instead (a request flag, a cause, a deadline)?
4. **The nightly hibernate path:** exactly how it flushes today (stores, publish queue, `forceFlush`s, delays), and whether it is the model for step (2). Is the publish queue actually persisted before hibernate (RAM entries; `PublishQueuePosixRK` RAM-queue size 2)?
5. **Forced-flush availability.** Which stores have a synchronous flush (`SystemConfig::flushNow()`), which do not (`/usr/current.dat`, `/usr/sensor.dat`), and the size of adding them. Time costs of each flush step (cite measurements in the repo if any) and a proposed per-step limit.
6. **Error state mechanics.** `State_Error.cpp` today: what it does, how states enter it, and what a "request reset" API looks like in the existing state machine; interaction with the watchdog, `ThrashGuard` and `LoopForensics`, and with the failsafe's own staged recovery (items 18, 24).
7. **Net `src/` estimate** for the whole change and for a staged split (each WO ≤ +20): target ≤ 0 net, hard +20. Which removals the consolidation pays for.
8. **Behavior risks:** reset causes that must stay distinguishable (`ResetCause.h`, reset 140 data values, alert/telemetry readers), the hibernate wake path, and tests that would change. A bench plan.
9. **Recommendation:** v49 or v50 (alongside the AB1805 native-driver switch), the smallest first WO, and a STOP if the design cannot meet the hard limit or a site cannot safely route.

## Constraints
Read-only: no code, no device, no commit, no network. Report states model and reasoning actually used. Out of scope: any `src/` change, the persisted layout, item 25 (a one-line `DEFAULT_TIMEZONE` change, unaffected), item 19.

## Approval record
Chip's design 3 Nov; dispatch on his direction.
