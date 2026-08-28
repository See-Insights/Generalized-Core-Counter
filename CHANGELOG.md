# Changelog

All notable changes to this project will be documented in this file.

## Unreleased

### Added

- (none)

### Changed

- (none)

### Fixed

- (none)

## [v24-Thermal-Inhibit] - 2026-08-28

### Added

- **Thermal charge-inhibit with hysteresis (F2a)**: New `ChargeInhibitPolicy` (pure, host-testable) plus `ChargeInhibit` mechanism. Arms charging inhibit at/above `armHighC` (37 C) or at/below `armLowC` (0 C); releases only once back inside the tighter `releaseHighC` (35 C) / `releaseLowC` (3 C) band, so the boundary cannot chatter. Replaces `SensorManager::isItSafeToCharge()`, whose hardcoded 0/45 C arm and 2/43 C release band was the LiPo spec limit rather than the field-proven arming values. Thresholds are ledger-configurable and per-device overridable; a candidate set is validated **as a whole** (`isValidThermalThresholds()`) and rejected wholesale rather than partially applied, and no ledger-supplied `armHighC` may exceed the cell's own 45 C charge maximum. WO-2026-08-25-001.
- **Validity-gated thermal evaluation (Amendment B, Decision B2)**: `evaluateThermalWithValidity()` distinguishes a genuine this-boot temperature reading from a stale/persisted one. Without a fresh measurement the arm decision is suppressed outright, and an already-armed inhibit is held but flagged via `thermal_inhibit_held_without_fresh_temp` so it cannot persist unobserved. Closes the gap where an unmeasured-but-persisted-hot temperature could arm on every boot while a hibernate cycle reset the TMP36 sample counter before it could release.
- **Boot-time inhibit resync**: `inhibited` is a non-retained static, but the DCT `DISABLE_CHARGING` bit it controls survives reset. The state is now read back from `System.getPowerConfiguration()` once per boot so a device that rebooted while hot recognizes it is already holding an armed inhibit instead of re-deciding from a false negative.
- **`BatteryHealth`, `PmicFaultMonitor`, `PowerTier`, and `BatteryTierGuard`**: Battery SoC-trust classification, PMIC fault reaction (separate concern from the proactive thermal inhibit), and battery-tier resolution extracted into their own host-testable units.

### Changed

- **Release metadata updated for v24-Thermal-Inhibit**: Bumped the firmware version to `v24-Thermal-Inhibit` and the Particle product version to `PRODUCT_VERSION(24)` for controlled OTA deployment.
- **`applyInputProfile()` preserves the DISABLE_CHARGING bit (Decision C5)**: Profile application now routes every config through `applyDisableChargingBit()` instead of replacing the DCT power configuration wholesale, so re-applying an input profile can no longer silently clear an active thermal inhibit.

### Known issues

- `tests/reporting_policy_adapter_test.sh` does not compile and is committed in a failing state. `src/reporting/RuntimeReportingPolicy.cpp` includes `"../MyPersistentData.h"` by relative path, which C++ resolves against the including file's own directory and therefore cannot be intercepted by the test's `-I` stub directory; the stub `MyPersistentData.h` in `tests/stubs/reporting_policy_adapter_overrides/` is consequently dead. Fixing it requires either a host stub for the Particle persistence layer or changing that include to a bare `#include "MyPersistentData.h"`. The other 14 host tests pass, including both thermal suites (`charge_inhibit_thermal_test.sh`, `thermal_threshold_migration_test.sh`).

## [v23-Diag-Soak] - 2026-08-24

### Added

- **Compiled build-flag witness (`flags=0x%04x` / ledger `flags`)**: `Boot:` serial line and the ledger `firmware` object now publish a compact bitmask derived directly from the same `#if` conditions that gate `DEV_BUILD`, `ALLOW_BLOCKING_SERIAL_WAITS`, `CONNECTIVITY_FAILSAFE_TEST_MODE`, `ENABLE_PMIC_FORENSICS`, the trace flags, `ENABLE_PMIC_CHARGE_CYCLE_TEST`, and `ENABLE_DIAGNOSTICS_PUBLISH_MODE`, so what was actually compiled into a running device is externally observable and cannot silently drift from what the version string implies. WO-2026-08-24-001.
- **Addendum A — witness bit for the USB override's platform guard (bit `0x4000`)**: Added a bit to the same `compiledBuildFlags` witness (both the `Boot:` line and the ledger `firmware` object), derived from the exact same `#if defined(PLATFORM_ID) && defined(PLATFORM_BORON) && (PLATFORM_ID == PLATFORM_BORON)` guard that gates the USB source override in `PowerManager::refreshInputProfile()`. Set means the override is compiled into this binary; clear means it is not. **A clear bit is expected and correct on non-Boron platforms (P2, Argon, MSOM)** — those platforms never compile the override, so an absent bit there is not a defect. The bit is a defect signal only on a Boron build, where it closes the exact blind spot that let this incident's override compile out silently: a default Boron build now emits `0x6008` at both sites (was `0x2008`). WO-2026-08-24-001 Addendum A.

### Changed

- **Release metadata updated for v23-Diag-Soak**: Bumped the firmware version to `v23-Diag-Soak` and the Particle product version to `PRODUCT_VERSION(23)` for controlled OTA deployment.

### Fixed

- **Boron USB power-source override silently compiled out of a shipped build**: Removed `ENABLE_BORON_USB_SOURCE_OVERRIDE`; the `PowerManager::refreshInputProfile()` override now compiles unconditionally on Boron (still gated only by the real `PLATFORM_ID == PLATFORM_BORON` platform check). The flag defaulted to 1 in source but was found compiled OFF in a shipped product build (v22, `app_hash 64188ac3...`) with a version string identical to a build where it was ON, leaving a genuine VIN misread on a USB-powered device uncorrected and contributing to Boron-Dev-14's over-discharge. Measured cost, cloud builds of the same source: the override itself is **176 bytes** (145,758 B with `ENABLE_BORON_USB_SOURCE_OVERRIDE=1` vs. 145,582 B with it at 0), and the build-flags witness added alongside it is a further **48 bytes** (145,806 B vs. 145,758 B) — **224 bytes total** against a build with the override compiled out. A local (non-cloud) build measures the witness at 56 bytes; the two toolchain paths differ slightly in newlib, so figures should only be compared within the same path. WO-2026-08-24-001.

## [v22-Diag-Soak] - 2026-08-12

### Added

- **Diagnostics publish mode for field soak**: Promoted `ENABLE_DIAGNOSTICS_PUBLISH_MODE` from bench-only to field-soak builds. Feature batches PowerDiag/ChargeDiag diagnostic emissions into compact "pdiag" JSON events and queues them via PublishQueuePosix for delivery on next cloud connection, enabling diagnostics capture across fully-disconnected soaks. End-to-end verified this week, including nightly heap-guard flush and JSON truncation fixes. Survived real production failure (660s connect timeout) with correct queuing and draining behavior.

### Changed

- **Disable blocking serial waits on reset**: Set `ALLOW_BLOCKING_SERIAL_WAITS` to 0. The bench-only forced wait for serial monitor attachment no longer serves a purpose now that the Pi forwarder captures serial continuously, and wastes 20-30s of battery on every reset. The always-on "brief wait only if already connected" default is unaffected.
- **Release metadata updated for v22-Diag-Soak staged rollout**: Bumped firmware version to `v22-Diag-Soak` and Particle product version to `PRODUCT_VERSION(22)` for targeted device-group OTA via Particle Fleet Manager.

### Validation

- Staged rollout via Particle Fleet Manager device selection (Boron-Dev-09 today, Boron-Dev-11/Dev-14 ~24h later, TRAIL02 Friday conditional, Photon2-Dev and Raleigh park devices deferred).

## [21.0_Test] - 2026-07-22

### Added

- **Shared reporting policy resolver**: Added `reporting/ReportingPolicy` as the single source of truth for effective report interval, next-report epoch, battery-tier multiplier, and open-hours gating, used by both the connection-mode policy and Device Status publishing.
- **Startup snapshot observability**: Added `observability/StartupSnapshot` and `StartupSnapshotRuntime` to capture an immutable per-boot snapshot (epoch, reset reason, firmware, Device OS, reset count) once initialization completes successfully.
- **Device Status `startup` object**: Device Status ledger now publishes a `startup` object describing the current execution instance, separate from continuously-changing runtime fields.
- **Reporting cadence transparency fields**: Device Status `reporting` now includes `configuredIntervalSec`, `effectiveIntervalSec`, and `adjustmentReason` so Fleet Ops can see why the effective cadence differs from the configured baseline without reconstructing firmware policy.
- **Unit test coverage**: Added `tests/` with a startup-snapshot unit test.

### Changed

- **Device Status schema bumped to v2**: `schemaVersion` for both Device Status and Device Data ledgers incremented to `2` to reflect the new `startup` object and reporting transparency fields.
- **`BatteryBackoff.cpp` delegates to shared policy**: Battery tier calculation and interval multiplier logic moved to `cloud/BatteryBackoffPolicy.h`; `Cloud::calculateBatteryTier()` and `Cloud::getIntervalMultiplier()` are now thin wrappers.
- **`lastReport` timestamp semantics documented and reordered**: `sysStatus.set_lastReport()` now runs immediately after `publishData()` and is documented as the application-report generation time, not a transport- or delivery-success timestamp.
- **Ledger contract documentation**: `docs/contracts/ledger-contracts.md` clarifies the ownership flow from configuration through runtime policy to published Device Status, and documents `startup` snapshot semantics and reporting-vs-wake-cadence separation for Fleet Ops.
- **Release metadata updated for v21 rollout**: Bumped firmware version to `21.0_Test` and Particle product version to `PRODUCT_VERSION(21)`.

### Fixed

- **Device Status `nextReportEpoch` drift from actual battery-adjusted cadence**: `writeDeviceStatusToCloud()` previously computed `nextReportEpoch` from a naive `lastReport + configuredInterval` calculation, independent of the battery-tier-adjusted interval actually used by `applyBatteryAwareConnectionModePolicy()`. Both now resolve through the same `ReportingPolicyResolver`, so published cadence matches actual firmware behavior.

### Validation

- Soak-tested on hardware for 48 hours prior to release; reporting cadence and Device Status fields behaved as designed.

## [16.0.0] - 2026-06-10

### Fixed

- **Watchdog forensic event suppression**: Removed conditional gating that prevented `publishWatchdogForensics()` from executing when the device was within open hours or sensor was ready. Watchdog forensic events are now queued unconditionally on every watchdog reset immediately after detection, ensuring forensic data (breadcrumb, loop stage, queue depth, state, connection age) is always captured and delivered for post-reset RCA.

### Changed

- **Release metadata updated for v16 patch**: Bumped firmware version to `16.0.0` and Particle product version to `PRODUCT_VERSION(16)`.

## [15.0.0] - 2026-06-10

### Added

- **Watchdog forensic instrumentation**: Added loop-stage forensics snapshots and startup watchdog forensic publish payloads to improve post-reset RCA for breadcrumb/stage/queue/state/connection-age context.

### Changed

- **Hibernate validation objective**: Release objective includes validating Boron hibernate behavior with extended 36000-second sleep duration in limited deployment.
- **Release metadata updated for v15 rollout**: Bumped firmware version to `15.0.0` and Particle product version to `PRODUCT_VERSION(15)`.

### Fixed

- **Breadcrumb collision risk in forensic ranges**: Reassigned breadcrumb IDs so `14-18` are uniquely reserved for `REPORT_LEDGER_DONE`, `CLOUD_LOOP_ENTER`, `CLOUD_LOOP_EXIT`, `PUBLISH_QUEUE_ENTER`, and `PUBLISH_QUEUE_EXIT`.

### Validation

- Revalidated Boron compile on Device OS `6.4.1` after v15 forensic instrumentation changes.
- Confirmed watchdog forensic publish payload remains compact and well under Particle event-data limits.

## [14.0.0] - 2026-06-06

### Added

- **Release trace controls**: Added compile-time trace controls for ledger, connect, connect-decision, sleep, performance, PMIC, and config logging so release builds can stay quiet without removing forensic and escalation visibility.

### Changed

- **Cloud recovery tuning**: Refined cloud recovery tuning and connect-decision observability while keeping the existing connection, recovery, and sleep behavior unchanged.
- **Release logging cleanup**: Reduced release log noise by gating routine trace and narrative logs while retaining state transitions, report/connect summaries, gate release logs, sleep/time diagnostics, and all warning/error paths.
- **Release metadata updated for v14 rollout**: Bumped the Particle product version, firmware version string, README version header, and Doxygen project number to `14.0.0` / `PRODUCT_VERSION(14)`.
- **Timezone normalization retained**: Carried forward explicit normalization of known ambiguous Singapore aliases to POSIX-correct `SGT-8` in ledger config apply (`SGT8`, `UTC+8`, `GMT+8`, `Singapore` -> `SGT-8`).

### Fixed

- **Misleading warning-level performance logs**: Successful `ReportPerf` timing entries are no longer emitted as warnings in normal release builds; only failures or abnormal latency remain warning-level.
- **Temporary diagnostic log noise**: Removed or trace-gated temporary timezone investigation logs (`TimeLedgerUpdate`, `TimeTZObject`, `TimeLedgerVerify`, `TimeBootVerify`) while keeping standard production diagnostics (`ConfigApply`, `ConfigValidate`, `ConfigDiag`, `TimeDiag`).

### Validation

- Revalidated production compiles for Boron and Photon 2 / P2 with Device OS `6.4.1` and release-safe trace defaults.

## [13.0.0] - 2026-05-27

### Added

- **Watchdog forensic telemetry**: Added retained watchdog reset counters and startup telemetry fields including `watchdogResetCount`, `lastWatchdogBreadcrumb`, and `lastWatchdogUptimeMs` so post-reset analysis survives missing serial logs.
- **PMIC anomaly telemetry**: Added retained PMIC contradiction counters, transition-only anomaly capture, startup/device-status telemetry, and `PMIC_ANOM` logging for low-SOC `Charged` or `Not Charging` contradictions.
- **Occupancy anomaly logging**: Added `OccAnom` logging on invalid occupancy close paths so impossible durations and prior totals are preserved for RCA.

### Changed

- **Release metadata updated for v13 rollout**: Bumped the Particle product version, firmware version string, README version header, and Doxygen project number to `13.0.0` / `PRODUCT_VERSION(13)`.
- **Occupancy close validation hardened**: Occupancy close handling now validates impossible session durations before updating daily totals.
- **PMIC forensics build flag added**: Added `ENABLE_PMIC_FORENSICS` as the single compile-time deployment flag, enabled by default for release builds, to gate PMIC contradiction logging, counters, and telemetry.

### Fixed

- **Corrupted daily occupancy totals from invalid start times**: Protected against unsigned-wrap and otherwise invalid `occupancyStartTime` values from inflating `dailyoccupancy` with impossible occupied durations.

### Validation

- Revalidated production compiles for Boron, Photon 2, and Argon with `CONNECTIVITY_FAILSAFE_TEST_MODE=0` and `ENABLE_PMIC_FORENSICS=1`.

## [11.0.1] - 2026-05-22

### Added

- **Release artifact documentation completeness**: Included the new architecture overview and the refreshed soak-plan references in the `11.0.1` release artifact so the published release matches the current `main` branch onboarding and soak guidance.

### Changed

- **Release metadata updated for the patch soak cut**: Bumped the Particle product version, firmware version string, README version header, Doxygen project number, and soak preconditions to `11.0.1` / `PRODUCT_VERSION(12)`.

### Fixed

- **Version traceability for Singapore soak deployment**: The firmware tip that includes the architecture overview documentation now reports a distinct patch-release identity instead of continuing to present itself as `11.0.0`.

## [11.0.0] - 2026-05-22

### Added

- **Connectivity failsafe recovery ladder**: Added a persisted long-duration connectivity supervisor with bounded escalation from radio reset to system reset to AB1805 deep power-down after repeated stale-cloud episodes.
- **Bench-only failsafe validation mode**: Added `CONNECTIVITY_FAILSAFE_TEST_MODE` with short stale and cooldown windows plus explicit diagnostics so the recovery ladder can be validated quickly without changing production thresholds.
- **Recovery and soak documentation**: Added release notes, recovery architecture notes, an architecture overview, and a soak plan covering the Photon 2 deployment in Singapore and the Boron deployment in North Carolina.

### Changed

- **Release metadata updated for v11 rollout**: Bumped Particle product version, firmware version string, README version header, Doxygen project number, and project Device OS metadata to `11.0.0` / `6.4.1`.
- **Failsafe supervisor respects active connect ownership**: The long-duration supervisor now defers recovery while `CONNECTING_STATE` is actively using its in-budget connect attempt, preventing the supervisor from interrupting legitimate recovery work.
- **Public documentation refreshed for operators and collaborators**: README now documents supported platforms, state flow, recovery model, build workflow, ledger usage, and production guardrails for soak deployments.

### Fixed

- **Battery-test harness removal from production path**: Removed the runtime battery override/test scenario behavior that could contaminate field or bench connectivity results while preserving persistent storage layout compatibility.
- **Failsafe diagnostics clarity**: Closed-hours sleep deferrals are now reported separately from eligibility status, making it easier to tell when the supervisor is healthy but intentionally inactive.
- **Stage-1 repeat behavior during a single stale episode**: Recovery stage tracking now prevents repeated radio-reset actions before the ladder escalates.

### Validation

- Bench-validated stage 1 radio reset, stage 2 system reset, and stage 3 deep power-down using `CONNECTIVITY_FAILSAFE_TEST_MODE=1`.
- Revalidated production compiles for Boron, Photon 2, and Argon with `CONNECTIVITY_FAILSAFE_TEST_MODE=0`.

### Known Limitations

- `CONNECTIVITY_FAILSAFE_TEST_MODE` is for bench validation only and intentionally shortens recovery timing; it must remain disabled for soak and production releases.
- Two source TODOs remain in `Generalized-Core-Counter.cpp` for future maintainability work, but they are not release blockers for v11.

## 10.00 - 2026-04-30

### Added

- **Nightly heap guard policy**: Added conservative overnight warning and reset thresholds for devices that must fall back to long `ULTRA_LOW_POWER` sleep instead of `HIBERNATE`, giving the next day a clean start when free heap drifts low.

### Changed

- **Release metadata updated for v10 rollout**: Bumped Particle product version, firmware version string, README version header, and Doxygen project number to `10.00`.
- **Battery telemetry now stabilizes before wake-time reporting**: Low-power wake paths now mark the first post-wake battery sample for bounded fuel-gauge stabilization before any later report/connect path can bring up the modem.

### Fixed

- **Post-sleep Boron battery misreads**: The first battery sample after low-power wake now issues `fuelGauge.quickStart()`, waits briefly, retries suspicious `0%`, `100%`, or `Unknown` readings, and falls back to voltage-estimated SoC when necessary.
- **Wake-time battery policy using distorted readings**: Battery SoC/state are now refreshed immediately after wake while the radio is still off, preventing modem current draw from skewing the measurement used for reporting and low-battery policy decisions.

## 9.00 - 2026-04-24

### Added

- **Retained app breadcrumbs for reset attribution**: Added retained startup breadcrumbs around sleep, reporting, connect-request, cloud-connected, and watchdog-reset milestones so connect-stage reboots can be attributed from the next boot log and startup status payload.
- **Centralized settings catalog documentation**: Added `Settings.h` as the single documentation entry point for build-profile flags, hardware override macros, and legacy project-level settings.

### Changed

- **Release metadata updated for v9 rollout**: Bumped Particle product version, firmware version string, README version header, and Doxygen project number to `9.00`.
- **Wake/connect service path hardened**: User-button wakes now route through `REPORTING_STATE`, explicit service requests bypass interval alignment without reusing occupancy semantics, and sleep now uses a queue-aware cloud-ops timeout budget.
- **Field-safe build defaults restored**: The repo now defaults to `DEV_BUILD=0` with blocking serial waits disabled, while still allowing developers to opt back into local debug behavior with build flags.
- **Carrier and settings documentation expanded**: Device pinout, project config, and settings comments were updated so generated API docs better explain build flags, pin roles, and fallback behavior.

### Fixed

- **Alert 44 on default-only devices**: `Cloud::areLedgersSynced()` now treats a synced `default-settings` ledger plus an empty `device-settings` ledger as valid, preventing false ledger-timeout alerts on new devices or fleets that only use product defaults.
- **Alert 44 reporting semantics**: Startup status now serves as the one-time report for a pending alert 44 and clears it immediately afterward so the alert does not linger into later service paths.
- **Boot-to-sleep battery telemetry gap**: Boot paths now refresh battery and PMIC telemetry before falling back to sleep, so short open-hours wake cycles still log current battery state.
- **PMIC status decoding**: Boron PMIC logging now reports correct VBUS, power-good, thermal, and VSYS-min state instead of mislabeling system-status bits.
- **Queue-drain self-churn and standby gating**: Verbose webhook diagnostics no longer feed the publish queue while it is draining, and network standby is only used after a successful cloud session in the current wake cycle.

## 8.00 - 2026-04-23

### Added

- **Targeted soak diagnostics for the ledger sync helper**: Retained a throttled `LEDGER_SYNC_DIAG` trace in `Cloud::areLedgersSynced()` so remote soak devices can confirm connection-window resets and immediate success when both ledgers are already synced.

### Changed

- **Release metadata updated for v8 soak rollout**: Bumped Particle product version, firmware version string, and Doxygen project number to `8.00`.
- **Ledger sync diagnostics narrowed**: Removed the broader sleep-gate Alert 44 diagnostic spam and kept only the helper-level logging needed to validate the semantic fix during fleet soak.

### Fixed

- **Alert 44 false positives after reconnect**: `Cloud::areLedgersSynced()` now returns true immediately when both ledgers are already synced instead of forcing the full connection window.
- **Stale ledger sync window reuse across cycles**: The helper now resets its timing window on each newly observed cloud connection, preventing one cycle's sync state from leaking into later sleep gates.

## 7.00 - 2026-04-21

### Added

- **Production-safe alert auto-clear allowlist**: Auto-clear after successful queued report now applies only to transient operational alerts `15`, `31`, `41`, `43`, and `44`.
- **Device-status heap checkpoint**: Added `freeHeap` logging after device-status ledger publish so the remaining `LedgerData::fromJSON()` path is visible during soak diagnostics.
- **Alert code 44**: New minor alert for ledger sync timeout before sleep (tier 1). Distinguishes cosmetic sync timeout (config already applied) from functional config apply failure (alert 41).
- **Platform-specific ledger sync timeout**: Cellular devices (Boron) now have 10-second ledger sync timeout vs 5 seconds for WiFi, reducing false alert triggers on slower networks.
- **Enhanced Alert 41 diagnostics**: Added detailed logging for configuration apply failures including ledger sync status, connection duration, and which config section failed (sensor/timing/messaging/modes/reporting).

### Changed

- **Cloud config apply path hardened for heap stability**: Removed merged ledger object materialization and now apply settings inline from defaults plus device overrides.
- **Hot-path string usage eliminated**: Replaced transient `String` use in webhook name, timezone, device-status diffing, and range-validation logging with `const char*` or fixed buffers.
- **Alert lifecycle semantics tightened**: Stale alert `41` is no longer cleared immediately on config success; transient alert clearing is now explicit and allowlist-based.
- **Alert 41 scope refined**: Now specifically indicates configuration apply failure during CONNECT phase (tier 2 - major). Previously also covered pre-sleep ledger sync timeouts which are now alert 44.
- **Documentation sync for release v7**: Updated firmware metadata, Particle product version, Doxygen project number, and alert 44 supervisor comments to match current behavior.

### Fixed

- **Alert 44 persistence during soak**: Pre-sleep ledger sync timeout alerts are now reported and then auto-cleared only under the new transient-alert allowlist model.
- **Misleading sensor merge diagnostic**: Removed the pre-read sensor merge log that could emit sentinel values like `setting1=-1` before the actual config read.

## 3.27 – 2026-03-12

### Fixed

- **ThrashGuard timeout protection**: Fixed ThrashGuard timeout conflicts in SLEEPING_STATE that caused spurious resets (RESET_REASON_USER/140).
  - Increased SLEEPING_STATE timeout from 30s to 60s to accommodate cloud operations sync (30s) + disconnect wait.
  - Added progress markers for serial reconnection (`SERIAL_WAIT`), cloud operations (`CLOUD_OPS_WAIT`), and disconnect wait (`DISCONNECT_WAIT`).
  - Added `WAKE_FROM_SLEEP` progress marker immediately after resuming from sleep.

### Added

- **Alert code 17**: State machine thrash detection alert raised on tier 2/3 thrash events, reported via webhooks to Ubidots.
- **Battery-aware connectivity for occupancy mode**: Automatically switches from `INTERMITTENT_KEEP_ALIVE` to `INTERMITTENT` when battery drops below 65% (CONSERVING tier) to save power, re-enables at 75% (HEALTHY tier).
- **ThrashGuard diagnostic variables**: Added `thrashTrips` and `thrashResets` Particle variables for remote monitoring.
- **Comprehensive ThrashGuard documentation**: Doxygen comments explaining tier system, state-specific timeouts, and escalation behavior.

## 3.26 – 2026-02-11

### Added

- Added `ThrashGuard` to detect lack-of-progress thrashing with tiered recovery (backoff, disconnect+sleep, reset) and retained counters.
- Added progress markers at state transitions and key milestones (connect start/success, ledger sync, queue drain, sleep attempt/return).

### Fixed

- ULTRA_LOW_POWER sleep now uses valid button edge (FALLING) and handles invalid sleep configs with STOP fallback to prevent tight wake/sleep loops.

## 3.25 – 2026-02-10

### Changed

- Refactored cloud responsibilities into focused modules (ledger I/O, merge, apply, status publishing) to reduce complexity and improve maintainability.
- Centralized connectivity budgets and related guardrails in `ConnectivityPolicy` to avoid scattered magic numbers.
- Added/validated local time snapshot caching for open-hours gating.
- Consolidated duplicated connectivity/radio helper functions into a single canonical implementation.
- Improved field/dev guardrails and wake-cycle observability (cycle summary logs and optional device-status fields).

## 3.24 – 2026-02-09

### Added

- Implemented occupancy mode end-to-end on top of the orthogonal mode architecture.
- Added support for real-time occupancy state-change reporting in keep-alive/standby connection modes.

## 3.23 – 2026-02-05

### BREAKING CHANGES - Orthogonal Mode Architecture Refactor

This release fundamentally restructures device configuration to use four independent, orthogonal mode dimensions instead of conflated multi-purpose modes. **Requires ledger reconfiguration for all devices.**

#### New Configuration Structure
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
    "connectAttemptBudgetSec": 300
  },
  "modes": {
    "sensorMode": 0,
    "connectionMode": 0,
    "reportingMode": 0,
    "samplingMode": 0
  }
}
```

### Added

- **Four Orthogonal Mode Dimensions**:
  - **SensorMode**: COUNTING=0, OCCUPANCY=1, MEASUREMENT=2 (was CountingMode)
  - **ConnectionMode**: CONNECTED=0, INTERMITTENT=1, DISCONNECTED=2, INTERMITTENT_KEEP_ALIVE=3 (was OperatingMode)
  - **ReportingMode** (NEW): SCHEDULED=0, ON_CHANGE=1, THRESHOLD=2, SCHEDULED_OR_THRESHOLD=3
  - **SamplingMode** (NEW): INTERRUPT=0, POLLING=1

- **Generic Sensor Configuration**:
  - `sensor.type`: Integer identifying sensor hardware (1=PIR, 2=Analog, etc.)
  - `sensor.setting1-4`: Four generic configuration values
  - Replaces hardcoded threshold1/threshold2/pollingRate fields
  - Enables support for any sensor type without firmware changes

- **Verbose Mode for Field Diagnostics**:
  - `messaging.verboseMode`: Enable detailed diagnostic publishing
  - `messaging.verboseTimeoutMin`: Auto-disable after timeout (default 60 minutes)
  - Publishes to "v/<category>" events (e.g., "v/sensor", "v/connection")
  - Rate-limited to 1 publish per second to prevent event flooding
  - Battery-safe with automatic timeout to prevent drain

- **Serial Log Level Control**:
  - `messaging.serial`: Controls Log.level() (INFO when true, ERROR when false)
  - Reduces overhead in production deployments
  - Enables verbose logging during development/debugging

- **Timing Section Enhancements**:
  - Moved `connectAttemptBudgetSec` from power section to timing section
  - Centralized all time-related configuration

### Changed

- **Enum Renames** (backward-compatible via numeric values):
  - `CountingMode` → `SensorMode`
  - `OperatingMode` → `ConnectionMode`
  - `LOW_POWER` → `INTERMITTENT`
  - `DISCONNECTED_KEEP_ALIVE` → `INTERMITTENT_KEEP_ALIVE`
  - `TriggerMode` split into orthogonal `ReportingMode` + `SamplingMode`

- **Removed Deprecated Fields**:
  - Power section entirely removed (solarPowerMode, lowPowerMode)
  - `occupancyDebounceMs` moved to `sensor.setting1`
  - Sensor-specific threshold1/threshold2/pollingRate replaced by generic settings

- **Configuration Parsing**:
  - Ledger sync now expects v3.23 JSON structure
  - Backward compatibility maintained via default initialization
  - Unknown modes default to safe values (COUNTING, CONNECTED, SCHEDULED, INTERRUPT)

### Migration Guide

**From v3.22 → v3.23:**

1. **Update device-settings ledger**:
   ```
   OLD: modes.countingMode    → NEW: modes.sensorMode
   OLD: modes.operatingMode   → NEW: modes.connectionMode
   OLD: modes.occupancyDebounceMs → NEW: sensor.setting1
   ```

2. **Enum value mappings remain unchanged**:
   - COUNTING=0 (was CountingMode, now SensorMode)
   - CONNECTED=0 (was OperatingMode, now ConnectionMode)
   - INTERMITTENT=1 (was LOW_POWER)
   - INTERMITTENT_KEEP_ALIVE=3 (was DISCONNECTED_KEEP_ALIVE)

3. **Remove power section**: Delete `power` object from ledger

4. **Add new modes**: Initialize `reportingMode=0` and `samplingMode=0`

5. **Sensor configuration**: Map sensor-specific values to generic settings:
   - PIR debounce: `occupancyDebounceMs` → `sensor.setting1`
   - Future sensors: Use `sensor.type` + `setting1-4` as needed

### Technical Details

- All changes maintain numeric enum value compatibility
- Persistent storage migration handled automatically via defaults
- No behavioral changes to existing counting/occupancy/measurement modes
- State machine updated across State_Idle.cpp, State_Sleep.cpp, State_Modes.cpp, State_Report.cpp, State_Error.cpp
- Generic sensor architecture enables future hardware without firmware updates

## 3.22 – 2026-02-05

### Added - Occupancy Mode Enhancements
- **New Operating Mode: DISCONNECTED_KEEP_ALIVE (value 3)**
  - During open hours, maintains cellular network in standby (~14mA) to avoid rapid reconnection overhead
  - Prevents carrier blacklisting from frequent disconnect/reconnect cycles in occupancy sensors
  - Essential for tennis court occupancy sensors with frequent state changes (occupied/unoccupied transitions)
  
- **Real-Time Occupancy State Change Reporting**
  - In DISCONNECTED_KEEP_ALIVE mode, device immediately reports when occupancy state changes
  - Transitions from "unoccupied" to "occupied" trigger instant webhook
  - Debounce timeout expiration (occupied → unoccupied) triggers instant webhook
  - Enables real-time dashboard updates for occupancy status
  
- **Occupancy-Specific Webhook Format**
  - New webhook payload for OCCUPANCY counting mode:
    ```json
    {
      "occupancy": "occupied|unoccupied",
      "dailyoccupancy": <total_seconds_occupied_today>,
      "battery": {
        "value": <percentage>,
        "context": {"key1": "<battery_state>"}
      },
      "temp": <celsius>,
      "alerts": <code>,
      "resets": <count>,
      "connecttime": <seconds>,
      "timestamp": <epoch_milliseconds>
    }

      - (none)

      ### Changed

      - (none)

      ### Fixed

      - (none)

      ## 7.00 – 2026-04-21

      ### Added

      - **Production-safe alert auto-clear allowlist**: Auto-clear after successful queued report now applies only to transient operational alerts `15`, `31`, `41`, `43`, and `44`.
      - **Device-status heap checkpoint**: Added `freeHeap` logging after device-status ledger publish so the remaining `LedgerData::fromJSON()` path is visible during soak diagnostics.
      - **Alert code 44**: New minor alert for ledger sync timeout before sleep (tier 1). Distinguishes cosmetic sync timeout (config already applied) from functional config apply failure (alert 41).
      - **Platform-specific ledger sync timeout**: Cellular devices (Boron) now have 10-second ledger sync timeout vs 5 seconds for WiFi, reducing false alert triggers on slower networks.
      - **Enhanced Alert 41 diagnostics**: Added detailed logging for configuration apply failures including ledger sync status, connection duration, and which config section failed (sensor/timing/messaging/modes/reporting).

      ### Changed

      - **Cloud config apply path hardened for heap stability**: Removed merged ledger object materialization and now apply settings inline from defaults plus device overrides.
      - **Hot-path string usage eliminated**: Replaced transient `String` use in webhook name, timezone, device-status diffing, and range-validation logging with `const char*` or fixed buffers.
      - **Alert lifecycle semantics tightened**: Stale alert `41` is no longer cleared immediately on config success; transient alert clearing is now explicit and allowlist-based.
      - **Alert 41 scope refined**: Now specifically indicates configuration apply failure during CONNECT phase (tier 2 - major). Previously also covered pre-sleep ledger sync timeouts which are now alert 44.
      - **Documentation sync for release v7**: Updated firmware metadata, Particle product version, Doxygen project number, and alert 44 supervisor comments to match current behavior.

      ### Fixed

      - **Alert 44 persistence during soak**: Pre-sleep ledger sync timeout alerts are now reported and then auto-cleared only under the new transient-alert allowlist model.
      - **Misleading sensor merge diagnostic**: Removed the pre-read sensor merge log that could emit sentinel values like `setting1=-1` before the actual config read.
    ```
      ## 3.27 – 2026-03-12

      ### Fixed

      - **ThrashGuard timeout protection**: Fixed ThrashGuard timeout conflicts in SLEEPING_STATE that caused spurious resets (RESET_REASON_USER/140).
        - Increased SLEEPING_STATE timeout from 30s to 60s to accommodate cloud operations sync (30s) + disconnect wait.
        - Added progress markers for serial reconnection (`SERIAL_WAIT`), cloud operations (`CLOUD_OPS_WAIT`), and disconnect wait (`DISCONNECT_WAIT`).
        - Added `WAKE_FROM_SLEEP` progress marker immediately after resuming from sleep.
- Updated connection with phases to unblock connections

## 3.12 – 2026-01-13
- Testing OTA

## 3.12 – 2026-01-14
- Watchdogs for resilency and reconnection

## 3.13 – 2026-01-14
- hardware testing with optimizations

## 3.14 – 2026-01-17
- Long-term test candidate

## 3.15 – 2026-01-18
- Sleep - Wake refinements

## 3.16 – 2026-01-19
- Sleep - Wake Refinement

## 3.18 – 2026-01-21
- Release Candidate for Limited Deployment

## 3.19 – 2026-01-21
- Minor Update on serial logging

## 3.20 – 2026-01-23
- Added PMIC monitoring for Boron
- Implmeented Occupancy

## 3.24 – 2026-02-10
- Optimized to reduce complexity / improve maintainability

## 3.25 – 2026-02-10
- Optimized to reduce complexity / improve maintainability

## 3.26 – 2026-02-11
- Added Thrash Guard - Fixed sleep bug

## 3.27 – 2026-03-12
- Soak fixes - Thrashing and power

## 3.28 – 2026-03-13
- Bug fix - debounce timer reset

## 3.29 – 2026-03-13
- Bug fix - Thrashguard on PIR events

## 4.00 – 2026-03-13
- Adopt integer release numbering system

## 4.01 – 2026-03-15
- allow long connect budgets to complete

## 4.02 – 2026-03-16
- add IDLE modem-on ceiling safety net

## 4.04 – 2026-03-16
- Fix for reset storm in setup / early loop

## 4.05 – 2026-03-18
- Testing complete: boot storm and connectivity hardening

## 5.00 – 2026-03-18
- Hardware Soak Production Candidate

## 18 – 2026-06-11
- Watchdog forensics hard fault fix

## 19 – 2026-06-15
- addressed power management issues particularly on USB power

## 20.1-PowerMgt – 2026-08-07
- Testing Power Management
