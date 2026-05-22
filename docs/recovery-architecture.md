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