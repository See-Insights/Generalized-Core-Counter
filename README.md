## Project Context

This repository includes key documentation files that contributors and AI coding assistants should review before proposing major changes:

- `PROJECT_STATUS.md` - Project purpose, current priorities, architecture principles, and constraints
- `ARCHITECTURE_OVERVIEW.md` - Detailed system architecture and design patterns
- `AI_DEVELOPMENT_WORKFLOW.md` - Development workflow and guidelines for AI-assisted engineering

# Generalized-Core-Counter

**Version:** 21.0_Test | **Latest:** Effective reporting policy contract, startup snapshot observability, and battery backoff refactor

Generalized-Core-Counter is a Particle firmware core for low-power outdoor sensor deployments that need flexible sensing modes, field-safe connectivity behavior, and durable configuration management. The v14 release packages cloud recovery tuning and release-safe logging cleanup without changing the existing production power, sleep, connectivity, watchdog, ledger, or PMIC behavior.

## Release Focus

- Production-ready Raleigh Boron soak release built from the tested watchdog-forensics baseline.
- Watchdog forensic visibility across resets via retained counters and startup telemetry.
- Occupancy close validation and anomaly logging to block impossible unsigned-wrap totals.
- PMIC contradiction forensic capture for low-SOC `Charged` or `Not Charging` events with no automatic remediation.

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

## Release Guardrails

Release v14 focuses on production-safe cloud recovery tuning and logging cleanup:

- No new runtime behavior changes.
- No sleep-policy changes.
- No watchdog-policy changes.
- No ledger lifecycle changes.
- No PMIC remediation-policy changes.

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

## Build Flags

### `CONNECTIVITY_FAILSAFE_TEST_MODE`

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

### `ENABLE_PMIC_FORENSICS`

`ENABLE_PMIC_FORENSICS` gates only the PMIC contradiction forensic path introduced for v13.

- Default is `ON` in `BuildProfile.h` for release builds.
- When enabled, the firmware records retained PMIC contradiction counters and includes PMIC forensic fields in startup and device-status telemetry.
- When disabled, PMIC contradiction logging, counters, and PMIC-specific startup/device-status telemetry are compiled out.
- It does not affect watchdog breadcrumbs, occupancy protection, charging policy, connectivity policy, sleep policy, or watchdog policy.

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

- Singapore / UTC+8: `SGT-8`
- Do not use `SGT8` for Singapore; in POSIX notation that is interpreted as UTC-8.

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

Startup status for v14 includes the connectivity recovery state plus the forensic fields needed for soak analysis:

- `failsafeStage`
- `failsafeCount`
- `lastConnectionAgeSec`
- `failsafeTest`
- `watchdogResetCount`
- `lastWatchdogBreadcrumb`
- `lastWatchdogUptimeMs`
- `pmicAnomalyCount`
- `lastPmicAnomalySoc`
- `lastPmicAnomalyChargeStatus`
- `lastPmicAnomalyAgeSec`

## Watchdog Forensics

Watchdog reset investigations now use retained startup fields to preserve the prior boot's evidence even when serial logs are missing.

- `watchdogResetCount`: retained count of watchdog-attributed resets seen by the application.
- `lastWatchdogBreadcrumb`: last retained application breadcrumb written before the watchdog reset path completed or the device stopped making progress.
- `lastWatchdogUptimeMs`: retained uptime snapshot from the watchdog forensic capture path.

Interpretation:

- Rising `watchdogResetCount` indicates repeat watchdog intervention.
- `lastWatchdogBreadcrumb` identifies the last known application phase.
- `lastWatchdogUptimeMs` helps distinguish immediate boot failures from long-uptime stalls.

## Occupancy Protection

Occupancy close handling now validates session durations before adding them to the retained daily total.

- Close paths reject `occupancyStartTime` values that are zero, implausibly in the future, or would create impossible day totals.
- This prevents unsigned-wrap corruption where `Time.now() - occupancyStartTime` could explode into a huge occupied duration.
- When validation fails, the firmware emits `OccAnom` logs with the close path, timestamps, and prior total for field RCA.

## PMIC Forensics

PMIC contradiction forensics are intended for field investigations where charging behavior changes after a reset and serial logs may be unavailable.

Permanent power-profile architecture notes live in
[`docs/architecture/power-management.md`](docs/architecture/power-management.md).
The June 2026 Boron USB/Solar charging root cause is documented in
[`docs/postmortems/2026-06-pmic-charging-root-cause.md`](docs/postmortems/2026-06-pmic-charging-root-cause.md).

Contradiction definition:

- SOC below `20%`
- Battery context is `Charged` or `Not Charging`
- Temperature is within the normal charging range
- The contradiction persists for the configured consecutive evaluations

When `ENABLE_PMIC_FORENSICS=1`, the firmware retains:

- `pmicAnomalyCount`
- `lastPmicAnomalySoc`
- `lastPmicAnomalyChargeStatus`
- `lastPmicAnomalyAgeSec` in telemetry

The detector records evidence only on the transition from inactive to active so one prolonged contradiction counts once. The feature is forensic-only in this release; it does not toggle charging, alter power policy, or introduce a new alert code.

## Documentation Index

- <a href="CHANGELOG.md">CHANGELOG.md</a>
- <a href="docs/architecture-overview.md">docs/architecture-overview.md</a>
- <a href="docs/architecture/power-management.md">docs/architecture/power-management.md</a>
- <a href="docs/postmortems/2026-06-pmic-charging-root-cause.md">docs/postmortems/2026-06-pmic-charging-root-cause.md</a>
- <a href="docs/recovery-architecture.md">docs/recovery-architecture.md</a>
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

1. Build a production image with `CONNECTIVITY_FAILSAFE_TEST_MODE=0` and `ENABLE_PMIC_FORENSICS=1`.
2. Flash or OTA deploy to the intended platform.
3. Confirm startup status includes the expected version plus watchdog and PMIC forensic fields.
4. Confirm ledger sync and publish behavior during the first connection window.
5. Monitor wake-cycle summaries, connection age, watchdog counters, occupancy anomaly logs, and any PMIC contradiction evidence.

## Contributor Notes

- Keep new power and connectivity timing values centralized in `ConnectivityPolicy.h`.
- Preserve persistent storage layout compatibility in `MyPersistentData.h` unless you are deliberately performing a migration.
- Prefer comment-only clarity improvements before architectural refactors during soak.
- Treat generated docs as release artifacts and regenerate them when public headers or release-facing markdown changes.

## Current Release Status

v14.0.0 is the cloud recovery tuning and logging cleanup release. It retains watchdog and PMIC forensic visibility while intentionally leaving charging, connectivity, sleep, and watchdog policies unchanged.
