# Recovery Architecture

## Purpose

The recovery architecture exists to keep unattended outdoor devices safe under prolonged poor connectivity without letting the modem or cloud stack consume unbounded time or battery.

The design is layered. Short-term protections run every wake cycle. Long-term protections act only after a device has gone an unusually long time without a successful cloud connection.

## Short-Term Protection Layers

### Bounded connect attempts

Each wake cycle has a bounded connection budget. The device can use a normal connect budget or, periodically, a deeper budget that is long enough to allow modem behaviors that sometimes recover marginal cellular situations.

### Bounded service window

After a connection succeeds, cloud work is limited by a service gate. This keeps queue drain, ledger sync, webhook response waits, and OTA checks from holding the device awake indefinitely.

### Bounded teardown

Disconnect and modem power-down have separate budgets. If cloud or radio teardown exceeds those budgets, the device raises the appropriate alert and moves toward a safe state instead of waiting forever.

### Thrash protection

`ThrashGuard` watches for states that stop making progress. It escalates through backoff, forced sleep/disconnect, and reset depending on severity and repetition.

## Long-Duration Connectivity Failsafe

The long-duration failsafe handles a different class of problem: the device keeps cycling normally, but it has not successfully connected to Particle for many hours.

Production thresholds:

- stale threshold: `12h`
- cooldown between actions: `6h`
- jitter cap: `30m`

Escalation ladder:

1. Stage 1: radio reset
2. Stage 2: `System.reset()`
3. Stage 3: AB1805 `deepPowerDown()`

The stage and action count are persisted in `sysStatus`, so escalation can continue across resets. A successful cloud connection clears the recovery state.

## Why The Supervisor Defers During Active Connect

The long-duration failsafe is intentionally higher level than `CONNECTING_STATE`. It should not interrupt a connection attempt that is still within the policy budget for the current wake.

v11 explicitly enforces that rule. If `CONNECTING_STATE` still owns an active, in-budget attempt, the supervisor logs a defer reason and waits. This prevents recovery logic from fighting normal recovery work.

## Defer Reasons

The supervisor may intentionally defer instead of escalating. Common defer reasons include:

- invalid time
- no last successful connection timestamp yet
- disconnected mode
- update pending
- low battery with hard-stage suppression
- closed-hours long sleep

Defer reasons are logged separately from overall eligibility so operators can distinguish healthy intentional inactivity from an actual recovery failure.

## Bench Validation Mode

`CONNECTIVITY_FAILSAFE_TEST_MODE` is a compile-time bench helper. It exists so the recovery ladder can be fully validated in a reasonable bench session.

When enabled:

- stale threshold becomes `5m`
- cooldown becomes `15m`
- jitter becomes `0s`
- long closed-hours sleep is capped to `60s`

Important constraints:

- This flag is bench-only.
- It must remain disabled for soak and production images.
- Production verification should confirm `FailsafeTest=0` in the boot log and production timing values in the failsafe diagnostic line.

## Deliberate Resets And Persistence

From v49, every deliberate reset or power-down first calls `flushAllBeforeReset()` (`State_Error.cpp`), which force-flushes the current-readings store (occupancy, `dailyoccupancy`, open-session state, alert codes), `sysStatus`, the sensor settings, and then the publish queue to its file queue. Sites: thrash tier 3, Error soft and hard, the nightly heap guard, sleep exhausted, and connectivity failsafe stages 2 and 3 (the stage, action and count are written first). Alert codes are in the current-readings store, so an alert raised just before the reset is saved.

The RTC-binding delete at the top of `setup()` (reset data 8) is the one deliberate reset that does not flush (persistence is not initialised yet and nothing is open), and it is bounded to one reset per boot cause: a boot that the guard itself caused never resets again.

Not flushed, by ruling: out of memory (immediate reset), the two hardware watchdogs and the software ApplicationWatchdog callback, and HIBERNATE.

Known exception (WO-2026-11-03-005, architect 4 Nov): the publish queue's file write covers only the RAM list. **An event already handed to the publisher (`curEvent`) when a deliberate reset happens can be lost: at most one per reset.** The library's own reset callback has the same gap. The fix, if `lib/PublishQueuePosixRK` is ever touched, is to write `curEvent` too. The queue's file path is built with `String`, which allocates (short, on a path with heap headroom).

Flush time is not yet measured; the bench records it (proposed ceilings: 250 ms per store, 2 s for the queue; the 60 s internal watchdog is the limit).

## Persistent Fields Used By Recovery

The following `sysStatus` fields back the supervisor:

- `lastConnection`
- `connectivityRecoveryStage`
- `lastConnectivityRecoveryAction`
- `connectivityRecoveryCount`

These fields let the device resume the correct recovery posture after resets and allow startup status to report the current recovery context upstream.

## Operational Expectations During Soak

Healthy devices should show:

- stable `lastConnectionAgeSec`
- `failsafeStage=0`
- `failsafeCount=0` most of the time
- no persistent alert `45`

Investigate devices that repeatedly escalate, recover only after stage 2 or 3, or show any sign that test timing is active in a production image.