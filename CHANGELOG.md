# Changelog

All notable changes to this project will be documented in this file.

## Unreleased

### Added

- (none)

### Changed

- (none)

### Fixed

- (none)

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

