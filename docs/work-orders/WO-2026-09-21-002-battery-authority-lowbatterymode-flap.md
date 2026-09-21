# WO-2026-09-21-002: `lowBatteryMode` clears unexpectedly at `CRITICAL`, not just `HEALTHY`

**Type:** Backlog investigation (deferred finding from Step 4 bench
validation).

**Status:** OPEN. Filed 2026-09-21, not yet investigated. Explicitly not
blocking `WO-2026-09-21-001-battery-authority.md`'s closure - see that WO's
"Known open item" for the decision to defer.

## The observation

During Dev-14's benchtop-supply voltage sweep (full data in
`WO-2026-09-21-001-battery-authority.md`'s Bench validation section),
`sysStatus.get_lowBatteryMode()` - as published in the `device-status` cloud
ledger's `battery.lowBatteryMode` field - cleared `false` twice at points
where tier was `CRITICAL`, not `HEALTHY`:

| Dial | tier | lowBatteryMode |
|---|---|---|
| 3.9V descending (first cross) | CRITICAL | true (expected: initial downgrade) |
| 3.8V descending | CRITICAL | **false (unexpected)** |
| 3.7V-3.6V descending | SURVIVAL | false |
| 3.5V descending | SURVIVAL (floor) | true |
| 3.6V-3.8V ascending | SURVIVAL | true |
| 3.9V ascending | CRITICAL | **false (unexpected)** |
| 4.0V-4.1V ascending | CONSERVING | true |
| 4.2V ascending | HEALTHY | false (expected: genuine recovery) |

Per `BatteryAuthorityCommand.cpp:75-100`, `commit()`'s OCCUPANCY-mode branch
only clears the flag in two cases: `tier == HEALTHY` while
`connectionMode == INTERMITTENT` (the intended recovery path), or
`connectionMode != INTERMITTENT` while the flag was set (a normalization
case). Neither should apply at a `CRITICAL` reading if `connectionMode` was
already `INTERMITTENT` from the preceding downgrade - which is the expected
state at both anomalous points, based on the sequence of prior commits.

## Why this wasn't resolved on the spot

The bench rig (benchtop supply through Battery-In) cannot carry a
simultaneous serial connection, so the `"Battery tier transition:"` /
`"Battery conservation:"` / `"Battery recovery:"` log lines
`BatteryAuthorityCommand.cpp` emits on every `commit()` call were not
capturable during this sweep. The cloud `device-status` ledger - the only
telemetry available - does not currently expose `connectionMode`, and
cannot distinguish whether a single `commit()` ran per bench step or
whether multiple call sites (`State_Sleep.cpp`'s pre-sleep/post-wake commits
and `State_Report.cpp`'s pre-connect commit both sample `SensorManager`
independently) fired within one button-triggered cycle with slightly
different vcell/trust snapshots, disagreeing right at a boundary.

## Leading hypotheses, unconfirmed

1. **Multiple `commit()` calls per bench-forced cycle, sampling vcell
   independently, disagreeing near a boundary.** A button press wakes the
   device (`State_Sleep.cpp:1651`'s post-wake commit) and then forces
   `REPORTING_STATE`, which itself commits again
   (`State_Report.cpp:168`, gated on `!Particle.connected()`) using a fresh
   sample taken moments later. If the two samples straddle a tier boundary,
   the ledger would show whichever commit ran last - and no memory of the
   first from the cloud alone.
2. **A `connectionMode` state not accounted for in this WO's reading of
   `commit()`'s branches** - e.g. if `connectionMode` were being restored to
   its configured default independently of `commit()` (a config-apply path),
   the "`connectionMode != INTERMITTENT` -> clear only" branch could fire
   without a full recovery.

Both are plausible from the code alone; neither is confirmed.

## Recommended next step

Add `sysStatus.get_connectionMode()` to the `device-status` cloud ledger's
`battery` (or a new) object - same pattern as
`WO-2026-09-21-001-battery-authority.md`'s telemetry addition: a one-line,
already-existing accessor, read-and-publish only, no `evaluate()`/`commit()`
change. With `connectionMode` visible alongside `tier`/`lowBatteryMode` at
each step, hypothesis 2 becomes directly checkable from the ledger with no
serial needed, and repeating just the two anomalous crossings (3.8V
descending, 3.9V ascending) would be enough to confirm or rule it out
without a full re-sweep.

If `connectionMode` alone doesn't explain it, hypothesis 1 would need either
serial (via a different power source that permits it, even briefly) or
additional instrumentation - e.g. a monotonic `commit()` call counter
published alongside the tier fields, to reveal whether more than one commit
landed inside a single bench step.

## Related work orders

- `WO-2026-09-21-001-battery-authority.md` - the parent WO whose bench
  validation surfaced this finding, and the source of `commit()`'s branch
  logic referenced above.
