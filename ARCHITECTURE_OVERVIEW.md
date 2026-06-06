# Firmware Architecture Inventory

Source basis: `PROJECT_STATUS.md`, `README.md`, `docs/architecture-overview.md`, and source layout under `src`.

## 1. Application Lifecycle and State Machine

- Purpose: Orchestrates device behavior across initialization, idle, reporting, connecting, sleeping, firmware update, and error handling; enforces non-blocking lifecycle and recovery escalation.
- Key files:
	- `src/Generalized-Core-Counter.cpp`
	- `src/state/StateMachine.h`
	- `src/state/StateHandlers.cpp`
	- `src/state/State_Idle.cpp`
	- `src/state/State_Connect.cpp`
	- `src/state/State_Report.cpp`
	- `src/state/State_Sleep.cpp`
	- `src/state/State_Error.cpp`
- Dependencies:
	- Internal: Cloud, power policy, sensors, persistent data, observability stats, ThrashGuard.
	- External: Particle Device OS lifecycle APIs.
- Complexity: High.
	- Reason: Multiple state-specific files plus global session flags, failsafe deferral reasons, watchdog breadcrumbs, and escalation behavior spread between main and state modules.
- Technical debt:
	- State ownership and supervisory logic are split across several files, making transition reasoning harder.
	- Main translation unit remains large and cross-cutting.
	- RAM-only session state can be lost during unexpected resets, reducing post-mortem clarity.

## 2. Cloud, Ledger, and Remote Configuration

- Purpose: Syncs cloud-ledger configuration, merges product defaults with device overrides, applies validated config, publishes status/data, and exposes remote functions.
- Key files:
	- `src/cloud/Cloud.cpp`
	- `src/cloud/Cloud.h`
	- `src/cloud/ConfigMerge.cpp`
	- `src/cloud/ConfigApply.cpp`
	- `src/cloud/LedgerClient.cpp`
	- `src/cloud/DeviceStatusPublisher.cpp`
	- `src/cloud/Particle_Functions.cpp`
- Dependencies:
	- Internal: Config validation, persistent storage, connectivity policy.
	- External: Particle Cloud and Ledger APIs, PublishQueuePosixRK.
- Complexity: Medium-high.
	- Reason: Hierarchical config merge/apply path with runtime validation and cloud/offline behavior constraints.
- Technical debt:
	- Manual merge logic is brittle and schema-light.
	- Apply/merge/validate responsibilities are distributed, increasing coupling.
	- Retry/backoff behavior for sync failure paths appears limited compared to mission-critical connectivity requirements.

## 3. Power, Connectivity Policy, and Platform Power Abstraction

- Purpose: Implements battery-aware behavior, connect budgets, modem/power teardown timing, PMIC/fuel-gauge interpretation, and platform-specific power capabilities.
- Key files:
	- `src/power/ConnectivityPolicy.h`
	- `src/power/Connectivity.h`
	- `src/power/PowerManager.cpp`
	- `src/power/PowerPlatform.cpp`
	- `src/power/PowerDiagnostics.cpp`
- Dependencies:
	- Internal: System status persistence, state machine decisions, cloud connect/report cadence.
	- External: Particle PMIC/fuel APIs, AB1805_RK where applicable.
- Complexity: High.
	- Reason: Hardware-dependent branches, multiple battery tiers and connection modes, and strict timing/budget constraints.
- Technical debt:
	- Platform guards and capability checks are spread across files.
	- Policy coupling with occupancy/connectivity behavior can cause hidden regressions.
	- PMIC forensic path increases observability but adds additional conditional complexity without remediation automation.

## 4. Sensor Abstraction and Sensor Runtime

- Purpose: Provides a stable sensor interface, sensor instantiation, and runtime sensor loop while supporting counting/occupancy semantics.
- Key files:
	- `src/sensors/ISensor.h`
	- `src/sensors/SensorManager.cpp`
	- `src/sensors/SensorFactory.h`
	- `src/sensors/SensorData.h`
	- `src/sensors/PIRSensor.cpp`
- Dependencies:
	- Internal: Device pinout, config and persistent sensor settings, state handlers.
	- External: Particle GPIO/interrupt APIs.
- Complexity: Medium.
	- Reason: Interface itself is simple, but occupancy/counting semantics and lifecycle hooks introduce behavioral coupling.
- Technical debt:
	- Factory is extensible, but only PIR appears production-mature, so cross-sensor behavior risk remains.
	- Generic sensor payload fields can obscure semantics unless carefully interpreted.
	- Some power/forensics concerns leak near sensor-adjacent code paths, weakening boundaries.

## 5. Persistence and Configuration Domain Model

- Purpose: Stores long-lived system configuration, runtime counters/status, sensor config, and versioned persisted state across sleep/reset.
- Key files:
	- `src/MyPersistentData.h`
	- `src/MyPersistentData.cpp`
	- `src/Config.h`
	- `src/Config.cpp`
	- `src/Settings.h`
- Dependencies:
	- Internal: Cloud apply/merge, state machine and power policy consumers.
	- External: StorageHelperRK and retained-memory patterns.
- Complexity: Medium-high.
	- Reason: Multiple persisted structures, enum contracts, source precedence, and migration compatibility constraints.
- Technical debt:
	- Enum/value stability is a hard compatibility contract and risky to change.
	- Validation is split across layers, making invalid intermediate states harder to rule out.
	- Schema evolution and migration safety depend on careful manual discipline.

## 6. Resilience and Supervision (Thrash and Failsafe)

- Purpose: Detects non-progress/thrashing, bounds stuck states, and escalates recovery through staged actions while preserving diagnostics.
- Key files:
	- `src/ThrashGuard.cpp`
	- `src/ThrashGuard.h`
	- `src/Generalized-Core-Counter.cpp`
	- `src/state/State_Common.h`
- Dependencies:
	- Internal: State machine, sysStatus persistence, connectivity policy timers.
	- External: System reset/power-down APIs, AB1805_RK.
- Complexity: High.
	- Reason: Time-budget supervision plus staged recovery across reset boundaries is subtle and safety-critical.
- Technical debt:
	- Recovery behavior is distributed between supervisor and state handlers.
	- Deferred-action logic is correctness-sensitive and difficult to regression-test without dedicated fault-injection coverage.

## 7. Observability, Time, and Device Abstraction Utilities

- Purpose: Records wake-cycle metrics, manages timezone/local-time caching, and centralizes platform pin mapping/version metadata.
- Key files:
	- `src/observability/WakeCycleStats.cpp`
	- `src/time/LocalTimeCache.cpp`
	- `src/device_pinout.cpp`
	- `src/Version.cpp`
	- `src/BuildProfile.h`
- Dependencies:
	- Internal: State/reporting flows and configuration.
	- External: LocalTimeRK and Particle platform APIs.
- Complexity: Low-medium.
	- Reason: Individual utilities are straightforward, but they are consumed in critical runtime paths.
- Technical debt:
	- Diagnostics are intentionally compact, which can limit root-cause granularity in worst-case field failures.
	- Timezone/signature invalidation correctness depends on edge-case handling around config updates and time validity.

## Overall Architecture Readout

- Current architecture is modular by domain and aligned with PROJECT_STATUS priorities: reliability-first, battery-aware behavior, cloud-authoritative config with offline continuity, and non-blocking state-machine control.
- Highest structural pressure points are where lifecycle orchestration, recovery supervision, and cloud/config apply intersect, especially across low-power and intermittent-connectivity scenarios.
