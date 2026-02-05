# Changelog

All notable changes to this project will be documented in this file.

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
    ```
  - Dashboard receives real-time occupancy state and cumulative daily occupied time
  - Battery nested in object structure per occupancy sensor requirements

### Changed
- **Operating Mode Validation**: Updated Cloud.cpp to accept operating mode values 0-3 (was 0-2)
- **Sleep Configuration**: ULTRA_LOW_POWER sleep now conditionally uses network standby based on operating mode and open hours
- **Mode Handler**: State_Modes.cpp occupancy handler now transitions to REPORTING_STATE on occupancy changes when in DISCONNECTED_KEEP_ALIVE mode
- **Startup Logging**: Operating mode display now includes DISCONNECTED_KEEP_ALIVE label

### Technical Details
- Solar-powered tennis court sensors get better light/signal than remote visitation counters
- Network standby power consumption (~14mA) acceptable for solar installations
- Prevents 30-60 second reconnection delays on each occupancy change
- Compatible with existing battery tier system (HEALTHY/CONSERVING/CRITICAL/SURVIVAL)
- Counting mode webhook format unchanged for backward compatibility

## 3.01 – 2025-12-23
- Add centralized firmware versioning, release notes, and include firmware metadata in device-status ledgers.

## 3.02 – 2025-12-23
- Fix PIR debounce timing

## 3.02 – 2025-12-23
- Fix PIR debounce timing

## 3.03 – 2025-12-23
- Updated workflow

## 3.04 – 2025-12-23
- Fixed issue with local time string

## 3.05 – 2025-12-24
- MVP for a PIR person counter

## 3.06 – 2025-12-26
- Fixing cleanups and logging

## 3.07 – 2025-12-27
- Now publishes to Ubidots and Ledger

## 3.08 – 2025-12-29
- MVP on Sleeping - Testing intervals

## 3.09 – 2025-12-30
- MVP starting to optimize

## 3.10 – 2025-12-31
- Firmware ready to start testing on Boron

## 3.10 – 2025-12-31
- Changes from Claude Sonnet 4.5 Code review - ready to test

## 3.11 – 2025-12-31
- Increased PublishQueue depth to 30 days

## 3.11 – 2026-01-13
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

