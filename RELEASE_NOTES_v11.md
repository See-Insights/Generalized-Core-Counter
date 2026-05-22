# Release Notes: v11.0.0

Release name: Connectivity resiliency soak release

## Intent

v11.0.0 is the soak-release candidate for the connectivity recovery work that was bench-validated before field rollout. This release keeps the production timing model conservative while adding a persisted escalation ladder for devices that remain awake and operational but fail to complete a successful cloud connection for an extended period.

This is a stability and resiliency release. It is not an architectural refactor release.

## Highlights

- Added a persisted long-duration connectivity failsafe with three escalation stages:
  - radio reset
  - system reset
  - AB1805 deep power-down
- Ensured the failsafe does not interrupt an active, in-budget `CONNECTING_STATE` attempt.
- Added targeted diagnostics so boot logs and startup status clearly show recovery stage, recovery count, and whether bench-only test timing is enabled.
- Added a developer-focused architecture overview covering subsystem boundaries, data flow, recovery flow, power management, and cloud integration.
- Removed the runtime battery-test harness so battery data and connectivity validation no longer share test-only code paths.
- Refreshed release-facing documentation, recovery documentation, and public API comments for operator and collaborator onboarding.

## Production Behavior

Production timing remains:

- connectivity stale threshold: `12h`
- recovery cooldown: `6h`
- jitter cap: `30m`

The bench-only flag `CONNECTIVITY_FAILSAFE_TEST_MODE` remains default `OFF` and is not part of the production soak build.

## Bench Validation Summary

Bench validation confirmed the entire recovery ladder:

1. Stage 1 radio reset
2. Stage 2 system reset
3. Stage 3 AB1805 deep power-down

Bench validation also confirmed:

- successful cloud recovery clears the persisted failsafe stage and count
- stage 1 does not repeat indefinitely during a single stale episode
- overnight closed-hours sleep deferral is visible separately from eligibility status
- production behavior remains unchanged when the test flag is disabled

## Deployment Scope

- `1` Photon 2 in Singapore
- `6` Borons in North Carolina

These devices should be monitored specifically for:

- unexpected recovery-stage growth
- cloud connection age drift
- repeated alert `31` or alert `45`
- wake-cycle length and teardown stability
- any evidence of test-mode leakage into production logs

## Rollback Guidance

Rollback is appropriate if any of the following occurs during soak:

- repeated recovery-stage escalation on otherwise healthy sites
- evidence that the supervisor interrupts legitimate connection windows
- abnormal battery drain correlated with recovery behavior
- platform-specific regressions on Boron, Photon 2, or Argon builds

Rollback target: previous production release `10.00`.

## Post-Soak Follow-Up

Post-soak work that remains intentionally out of scope for this release:

- larger module boundary cleanup in `Generalized-Core-Counter.cpp`
- broader Doxygen normalization across every remaining public surface
- any architecture-level extraction beyond the focused clarity changes already made