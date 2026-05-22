# Generalized-Core-Counter

**Version:** 11.0.0 | **Latest:** Connectivity resiliency soak release

Generalized-Core-Counter is a Particle firmware core for low-power outdoor sensor deployments that need flexible sensing modes, field-safe connectivity behavior, and durable configuration management. The v11 release is the production soak candidate for the new connectivity recovery ladder and its supporting observability.

## Release Focus

- Stable soak candidate for `1` Photon 2 in Singapore and `6` Borons in North Carolina.
- Recovery-first connectivity behavior for marginal RF conditions.
- Clear operator documentation for ledger-backed configuration, wake-cycle diagnostics, and release validation.
- No new feature refactor in this release; focus is resiliency, maintainability, and deployment clarity.

## Supported Platforms

- Particle Boron, Device OS `6.4.1`
- Particle Photon 2 / P2, Device OS `6.4.1`
- Particle Argon, Device OS `6.4.1`

The firmware is written to keep platform-specific power behavior behind platform guards. Cellular-only behaviors such as modem standby and AB1805 deep power-down apply only where the hardware supports them.

## Supported Sensor Patterns

- Counting mode for discrete events.
- Occupancy mode for occupied/unoccupied state with debounce and accumulated occupied time.
- Measurement mode for threshold-driven analog or custom sensors.
- Interrupt-driven and polling-driven sampling paths.

The current repository ships PIR support and the abstractions needed to add additional sensors through the `ISensor` interface and `SensorFactory`.

## Core Architecture

The application is a small explicit state machine with focused handler modules:

- `INITIALIZATION_STATE`
- `IDLE_STATE`
- `CONNECTING_STATE`
- `REPORTING_STATE`
- `SLEEPING_STATE`
- `FIRMWARE_UPDATE_STATE`
- `ERROR_STATE`

`Generalized-Core-Counter.cpp` owns global lifecycle, retained breadcrumbs, startup telemetry, and top-level supervision. State-specific behavior lives under `src/state/`. Power and connectivity policies live under `src/power/`. Cloud configuration and ledger publishing live under `src/cloud/`.

## Recovery Architecture

v11 adds a persisted long-duration connectivity failsafe on top of the existing bounded connect and teardown budgets.

Normal protection layers:

- Per-wake connect attempt budgets prevent indefinite cloud connect waits.
- Service and disconnect gates prevent the modem from remaining powered after work is complete or stuck.
- ThrashGuard detects no-progress state machine behavior.

Long-duration recovery ladder:

1. Stage 1: radio reset
2. Stage 2: `System.reset()`
3. Stage 3: AB1805 `deepPowerDown()`

The ladder persists its stage and count in `sysStatus` so it can escalate across resets. A successful cloud connection clears the recovery state. The supervisor now defers action while `CONNECTING_STATE` still owns an in-budget connect attempt, which prevents the long-duration monitor from fighting the short-term connect policy.

See [docs/recovery-architecture.md](docs/recovery-architecture.md) for the operator-facing description.

## Power And Connectivity Model

Connectivity and power policy are centralized in `ConnectivityPolicy.h`. The important operational guardrails are:

- Production failsafe stale threshold: `12h`
- Production failsafe cooldown: `6h`
- Production failsafe jitter cap: `30m`
- Connect attempt default budget: `5m`
- Periodic deep connect budget: `11m`
- Cloud operations gate: `30s`
- Output-ledger sync gate: `70s` cellular, `30s` Wi-Fi

Battery-aware connection behavior can reduce connectivity aggressiveness as state of charge drops. Occupancy keep-alive is retained only when the platform and current battery tier can justify it.

## `CONNECTIVITY_FAILSAFE_TEST_MODE`

`CONNECTIVITY_FAILSAFE_TEST_MODE` exists only to validate the long-duration recovery ladder on the bench.

When enabled, it changes only the failsafe timing:

- stale threshold: `5m`
- cooldown: `15m`
- jitter: `0s`
- closed-hours sleep cap: `60s`

Production rules:

- Default is `OFF` in `BuildProfile.h`.
- Do not enable it for soak or production builds.
- If you enabled it via `EXTRA_CFLAGS`, remove the flag or force `-DCONNECTIVITY_FAILSAFE_TEST_MODE=0` before cutting a release build.
- Production validation must confirm `FailsafeTest=0` in logs and production timing values in the failsafe boot line.

See <a href="docs/bench-validation.md">docs/bench-validation.md</a> for the short-threshold validation workflow.

## Ledger Configuration Model

The firmware uses Particle Ledger for both cloud-to-device configuration and device-to-cloud status visibility.

| Ledger | Scope | Direction | Purpose |
| --- | --- | --- | --- |
| `default-settings` | Product | Cloud to device | Product-wide default configuration |
| `device-settings` | Device | Cloud to device | Per-device overrides |
| `device-status` | Device | Device to cloud | Current applied configuration and status snapshot |
| `device-data` | Device | Device to cloud | Latest sensor data |

Typical flow:

1. Device boots and connects.
2. Product defaults sync first.
3. Device overrides sync if present.
4. Merged configuration is applied to persistent storage.
5. Device publishes `device-status` so Console reflects what is actually running.

This split allows offline edits in Console without requiring the device to stay continuously connected.

### Configuration Shape

```json
{
  "messaging": {
    "serial": false,
    "verboseMode": false,
    "verboseTimeoutMin": 60
  },
  "sensor": {
    "type": 1,
    "setting1": 5000,
    "setting2": 0,
    "setting3": 0,
    "setting4": 0
  },
  "timing": {
    "openHour": 6,
    "closeHour": 22,
    "reportingIntervalSec": 3600,
    "timezone": "SGT-8",
    "connectAttemptBudgetSec": 300,
    "cloudDisconnectBudgetSec": 15,
    "modemOffBudgetSec": 30
  },
  "modes": {
    "sensorMode": 0,
    "connectionMode": 1,
    "reportingMode": 0,
    "samplingMode": 0
  }
}
```

Timezone values must be POSIX timezone strings, not IANA names.

## Build And Validation

The repository is build-validated on Boron, Photon 2, and Argon using Device OS `6.4.1`.

Typical local compile pattern:

```bash
DEVICE_OS_PATH="/Users/chipmc/.particle/toolchains/deviceOS/6.4.1" \
APPDIR="$PWD" \
PLATFORM="boron" \
DEVICE_OS_VERSION="6.4.1" \
GCC_ARM_PATH="/Users/chipmc/.particle/toolchains/gcc-arm/10.2.1/bin/" \
PATH="$PATH:/Users/chipmc/.particle/toolchains/gcc-arm/10.2.1/bin" \
make -f '/Users/chipmc/.particle/toolchains/buildscripts/1.17.2/Makefile' compile-user -s
```

Supported production targets for this release:

- `boron`
- `p2`
- `argon`

Bench-only short-threshold validation builds can add `EXTRA_CFLAGS="-DCONNECTIVITY_FAILSAFE_TEST_MODE=1"`, but release builds must not.

## Observability

The firmware emits compact operational diagnostics designed for poor-connectivity, power-constrained deployments.

Important signals:

- wake-cycle summary log (`CYCLE end ...`)
- startup status payload
- retained breadcrumb reason for prior reset or deep power-down attribution
- failsafe boot and action lines
- alert codes for bounded connect, ledger sync, and recovery conditions

Startup status for v11 includes the connectivity recovery state needed for soak analysis:

- `failsafeStage`
- `failsafeCount`
- `lastConnectionAgeSec`
- `failsafeTest`

## Documentation Index

- <a href="RELEASE_NOTES_v11.md">RELEASE_NOTES_v11.md</a>
- <a href="docs/architecture-overview.md">docs/architecture-overview.md</a>
- <a href="docs/recovery-architecture.md">docs/recovery-architecture.md</a>
- <a href="docs/v11-soak-plan.md">docs/v11-soak-plan.md</a>
- <a href="docs/bench-validation.md">docs/bench-validation.md</a>
- Generated API reference: `docs/html/index.html`

## Repository Layout

- `src/`: firmware source
- `src/state/`: state handlers and shared state-machine interfaces
- `src/power/`: connectivity and power policies, platform abstractions, and power manager
- `src/cloud/`: Particle Ledger integration and device/cloud publishing helpers
- `src/sensors/`: sensor abstraction and concrete sensor implementations
- `docs/`: operator and design documentation
- `lib/`: vendored dependencies

## Deployment Workflow

1. Build a production image with `CONNECTIVITY_FAILSAFE_TEST_MODE=0`.
2. Flash or OTA deploy to the intended platform.
3. Confirm startup status includes the expected version and recovery fields.
4. Confirm ledger sync and publish behavior during the first connection window.
5. Monitor wake-cycle summaries, connection age, and any recovery-stage transitions.

For the v11 soak rollout, follow <a href="docs/v11-soak-plan.md">docs/v11-soak-plan.md</a>.

## Contributor Notes

- Keep new power and connectivity timing values centralized in `ConnectivityPolicy.h`.
- Preserve persistent storage layout compatibility in `MyPersistentData.h` unless you are deliberately performing a migration.
- Prefer comment-only clarity improvements before architectural refactors during soak.
- Treat generated docs as release artifacts and regenerate them when public headers or release-facing markdown changes.

## Current Release Status

v11.0.0 is the connectivity resiliency soak release. It is intended to validate the new long-duration recovery ladder under real deployment conditions without widening scope into post-soak refactors.