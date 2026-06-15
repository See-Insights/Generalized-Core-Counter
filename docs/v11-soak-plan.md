# v11 Soak Plan

## Scope

This soak validates release `11.0.1` as the connectivity resiliency candidate.

Deployment plan:

- `1` Photon 2 in Singapore
- `6` Borons in North Carolina

The soak is intended to validate resiliency, observability, and operational clarity. It is not intended to validate a new architecture or a large feature expansion.

## Preconditions

- Firmware version reports `11.0.1`.
- `PRODUCT_VERSION(12)` is present in the firmware.
- Device OS target is `6.4.1`.
- Production builds were compiled for Boron, Photon 2, and Argon.
- `CONNECTIVITY_FAILSAFE_TEST_MODE` is disabled.
- Startup status includes `failsafeStage`, `failsafeCount`, `lastConnectionAgeSec`, and `failsafeTest`.

## Primary Success Criteria

- No evidence of test-mode timing in production logs.
- No unexpected modem-on dwell caused by connection or teardown paths.
- Healthy devices do not climb the recovery ladder during normal site conditions.
- Devices in marginal connectivity can recover without entering unstable reset loops.
- Ledger-backed configuration continues to apply cleanly with no new sync regressions.

## Monitoring Checklist

Review these signals daily during soak:

- firmware version in startup status
- `failsafeStage`
- `failsafeCount`
- `lastConnectionAgeSec`
- alert codes, especially `31`, `44`, and `45`
- wake-cycle summary logs
- queue drain and teardown timing
- battery state and any unexpected drop correlated with repeated recovery actions

## Escalation Guidance

Investigate immediately if any device shows:

- `failsafeTest=1`
- repeated stage-2 or stage-3 recovery on an otherwise healthy site
- repeated reset attribution pointing to the connectivity failsafe
- repeated long wake cycles dominated by connect or teardown time
- abnormal battery drain after the v11 rollout

## Suggested Soak Cadence

### Day 0

- Confirm first boot after deployment.
- Confirm ledger sync and device-status publication.
- Confirm expected operating mode and local timezone.

### Days 1-3

- Review daily connection age, alert activity, and wake-cycle summaries.
- Compare Boron behavior against the Photon 2 baseline.

### Days 4-7

- Focus on stability trends rather than single events.
- Watch for any device whose recovery count continues to grow without clearing.

## Rollback Criteria

Rollback to the previous production release if any of the following is observed:

- widespread regressions across Boron or Photon 2 deployments
- recovery ladder activates under normal healthy connectivity
- supervisor behavior causes loss of expected reporting cadence
- production devices show test-mode leakage

Recommended rollback target: release `10.00`.

## Post-Soak Decision

Promote v11 beyond soak only if:

- no release blocker emerges from the monitored fleet
- recovery behavior matches the documented design
- operator diagnostics are sufficient to explain the few failures that do occur
- no platform-specific compile or runtime regressions appear on Boron, Photon 2, or Argon