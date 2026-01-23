# Generalized-Core-Counter

A generalized IoT firmware core for outdoor sensor devices supporting multiple operating modes and sensor types.

## Overview

This firmware provides a flexible, production-ready platform for outdoor IoT devices (Particle P2 WiFi and Boron Cellular) that can operate in different modes while maintaining a clean, extensible codebase. The architecture uses Particle Ledger for cloud configuration management, enabling offline device configuration updates and real-time data visibility.

## Features

- **Multiple Operating Modes**:
  - **Counting Mode**: Track individual events (counts per hour/day)
  - **Occupancy Mode**: Monitor occupied time with debounce logic
  
- **Power Management**:
  - **Connected Mode**: Stay connected for real-time updates
  - **Low-Power Mode**: Sleep between scheduled reports
  
- **Trigger Types**:
  - **Interrupt-Driven**: Event-based sensor triggering
  - **Scheduled Polling**: Periodic sensor checks

- **Cloud Configuration**:
  - Product-level defaults via Ledger
  - Device-specific overrides (editable offline in Console)
  - Auto-sync on connection
  
- **Data Publishing**:
  - Webhook integration (Ubidots)
  - Device-to-cloud ledger for Console visibility
  - Mode-aware JSON payloads

## Hardware

Supported Platforms:
- **Particle P2** (WiFi, DeviceOS 6.3.4)
- **Particle Boron** (Cellular, DeviceOS 6.3.4)

Sensor Support (Extensible):
- PIR motion sensor (implemented)
- Ultrasonic distance sensor (template)
- Custom sensors via ISensor interface

## Documentation

- Bench validation checklist: [docs/bench-validation.md](docs/bench-validation.md)

- Generated API reference (Doxygen): **https://see-insights.github.io/Generalized-Core-Counter/**
  - To browse locally: Open `docs/html/index.html` in your browser

## Architecture

### Sensor Interface Pattern
All sensors implement the `ISensor` interface:
- `setup()`: Initialize hardware
- `loop()`: Update sensor state
- `getSensorData()`: Return latest readings
- `getSensorType()`: Identify sensor type

New sensors are added via `SensorFactory.h` without modifying core code.

### State Machine
- INITIALIZATION → IDLE → REPORTING → IDLE (connected mode)
- INITIALIZATION → IDLE → SLEEPING → IDLE (low-power mode)
- ERROR handling with cloud reporting (no reset loops)

### Application State Machine & Handlers

The main application logic is implemented as a small, explicit state machine:

- **Core states** (see src/StateMachine.h): INITIALIZATION, IDLE, CONNECTING,
  REPORTING, SLEEPING, FIRMWARE_UPDATE, ERROR.
- **State handlers** (see src/StateHandlers.cpp) encapsulate behaviour for each
  state (e.g. connection policy, publish cadence, sleep scheduling, and
  firmware/config update flows).
- **State driver** (see src/Generalized-Core-Counter.cpp) selects the current
  state in `loop()` and logs transitions via `publishStateTransition()`.

This split keeps the outer `setup()/loop()` structure simple while allowing the
per-state behaviour to evolve independently as new modes or error conditions
are added.

### Persistent Storage
Three storage structures:
- `sysStatus`: System configuration and state
- `sensorConfig`: Sensor-specific parameters
- `current`: Live data (counts, occupancy, battery, temp)

## Configuration Management

### Ledger Architecture

| Ledger | Scope | Direction | Purpose | Editable? |
|--------|-------|-----------|---------|-----------|
| `default-settings` | Product | Cloud→Device | Product-wide defaults for all devices | Yes (Console) |
| `device-settings` | Device | Cloud→Device | Device-specific config overrides | Yes (Console, even offline) |
| `device-status` | Device | Device→Cloud | Current device configuration snapshot | No (auto-updated by device) |
| `device-data` | Device | Device→Cloud | Latest sensor readings | No (auto-updated by device) |

**Key Concepts:**
- **Cloud→Device**: You edit in Console, device reads/applies on connection
- **Device→Cloud**: Device writes, you view in Console (read-only from Console)
- **device-settings vs device-status**: Settings = what you *want*, Status = what device *has*

### Configuration Structure

```json
{
    "messaging": {
        "disconnectedMode": false,
        "serial": false,
        "verboseMode": false
    },
    "power": {
        "lowPowerMode": false,
        "solarPowerMode": true
    },
    "sensor": {
        "threshold1": 60,
        "threshold2": 60
    },
    "timing": {
      "closeHour": 22,
      "openHour": 6,
      "pollingRateSec": 0,
      "reportingIntervalSec": 3600,
      "timezone": "SGT-8"   
  },
    "modes": {
        "countingMode": 0,
        "operatingMode": 0,
        "triggerMode": 0,
        "occupancyDebounceMs": 0,
        "connectedReportingIntervalSec": 300,
      "lowPowerReportingIntervalSec": 3600,
      "connectAttemptBudgetSec": 300
    }
}
```

  **Timezone notes:**
  - `timing.timezone` must be a POSIX timezone string (not an IANA name like "Asia/Singapore").
  - Example for Singapore (UTC+8, no DST): `"SGT-8"`.
  - See the LocalTimeRK documentation for examples and guidance on building POSIX strings for major cities: https://rickkas7.github.io/LocalTimeRK/

### Mode Values

**countingMode**: 
- `0` = COUNTING (count events)
- `1` = OCCUPANCY (track occupied time)

**operatingMode**:
- `0` = CONNECTED (stay connected)
- `1` = LOW_POWER (sleep between reports)

**triggerMode**:
- `0` = INTERRUPT (event-driven)
- `1` = SCHEDULED (periodic polling)

## Device Commissioning Workflow

1. **Device Flashed**: Start with generic Particle firmware
2. **Added to Product**: Assign to "Generalized-Core-Counter-P2" product
3. **Auto-Flashed**: Receives product firmware OTA
4. **First Connection**: 
   - Syncs `default-settings` (product-level defaults)
   - Checks for `device-settings` ledger (Cloud→Device)
   - **If no device-settings exists** (new device):
     - Applies `default-settings` to persistent memory
     - **Writes `device-status` ledger** (Device→Cloud) for visibility
     - Device waits for you to create `device-settings` in Console
   - **If device-settings exists** (configured device):
     - Applies `device-settings` (overrides defaults)
     - Updates `device-status` to reflect current config
   - May reset if sensor initialization requires I2C driver changes
5. **Data Collection**: Starts operating based on configured modes
6. **Data Publishing**:
   - Publishes to webhook (real-time Ubidots integration)
   - Updates `device-data` ledger (Console visibility)
   - Updates `device-status` periodically
7. **Configuration Updates**: 
   - On each connection, checks if `device-settings` changed
   - Auto-applies updates if Console values were modified
   - Logs all configuration changes
   - Updates `device-status` to confirm new config applied

### Configuration Management Flow

**Most Devices (Use Product Defaults):**
1. Device connects → Loads `default-settings` → Writes `device-status`
2. Device operates with product-level configuration
3. No manual intervention needed

**Devices Needing Custom Config (Different park hours, sensor types, testing mode):**
1. Device connects → Loads `default-settings` → Writes `device-status`
2. In Console, view `device-status` ledger (shows current config as JSON)
3. **Copy JSON from `device-status`**
4. **Create new `device-settings` ledger** for that device (Cloud→Device, Device scope)
5. **Paste JSON, modify only the fields you need** (e.g., openHour, closeHour, operatingMode)
6. Save ledger
7. Device syncs `device-settings` on next connection (or trigger connection)
8. `device-status` updates to confirm new config

**Example: Park with Different Hours**
- `default-settings`: `"openHour": 6, "closeHour": 22`
- For specific device, create `device-settings`:
  ```json
  {
    "timing": {
      "openHour": 7,
      "closeHour": 20
    }
  }
  ```
- Only override fields that differ, device uses defaults for everything else

**Example: Testing Device (Stay Connected)**
- Create `device-settings`:
  ```json
  {
    "modes": {
      "operatingMode": 0
    }
  }
  ```
- Device stays in CONNECTED mode instead of LOW_POWER

**Offline Editing:**
- Edit `device-settings` in Console anytime (even while device offline)
- Device syncs changes on next connection
- `device-status` updates to confirm applied config
- Compare `device-settings` (desired) vs `device-status` (actual) to verify sync

## Offline Device Management

You can:
- ✅ Edit device configuration in Console while device is offline
- ✅ View last sensor data in `device-data` ledger
- ✅ Changes sync automatically on next connection
- ✅ Real-time data continues to flow to Ubidots via webhook

## Data Reporting

### Counting Mode Payload
```json
{
    "timestamp": 1702345678,
    "deviceId": "e00fce68...",
    "battery": 85.2,
    "temp": 23.5,
    "mode": "counting",
    "hourlyCount": 42,
    "dailyCount": 327,
    "lastCount": 1702345670,
    "powerMode": "connected",
    "triggerMode": "interrupt"
}
```

### Occupancy Mode Payload
```json
{
    "timestamp": 1702345678,
    "deviceId": "e00fce68...",
    "battery": 85.2,
    "temp": 23.5,
    "mode": "occupancy",
    "occupied": true,
    "sessionDuration": 120,
    "occupancyStart": 1702345558,
    "totalOccupiedSec": 3847,
    "powerMode": "lowPower",
    "triggerMode": "scheduled"
}
```

## Getting Started

### Setup in Particle Console

1. **Create Product**: "Generalized-Core-Counter-P2"

2. **Create Product-Level Ledger** (`default-settings`):
   - Direction: Cloud→Device
   - Scope: Product
   - Add JSON configuration (see Configuration Structure above)
   - **This is the ONLY ledger you need to manually create**

3. **Device Ledgers** (Auto-Created by Firmware):
   - `device-status` (Device→Cloud): Auto-created on first connection, shows current config
   - `device-data` (Device→Cloud): Auto-created when device publishes data
   - These are read-only from Console perspective

4. **Device-Settings Ledger** (Optional Override):
   - **Create manually in Console** if you want device-specific overrides
   - Direction: Cloud→Device
   - Scope: Device
   - Start with JSON from `device-status`, then modify as needed
   - If absent, device uses `default-settings`

5. **Set Up Webhook** for Ubidots integration:
   - Event name: `sensor-data`
   - URL: Your Ubidots endpoint
   - Request type: POST
   - JSON template as needed

### Flash Firmware

```bash
particle compile p2 --saveTo firmware.bin
particle flash <device-name> firmware.bin
```

Or flash via OTA when device is added to product.

### Monitor Operation

```bash
particle serial monitor
```

Look for:
- "Configuration loaded from cloud"
- "Counting mode set to: X"
- "Operating mode set to: X"
- "Published to webhook: {...}"
- "Published data to ledger: {...}"

## How-To: Common Configuration Tasks

### Creating Device-Specific Settings

**Step 1: View Current Configuration**
1. In Particle Console, go to your device
2. Click "Ledger" tab
3. Find `device-status` ledger (Device→Cloud)
4. Copy the entire JSON

**Step 2: Create Override**
1. In Console, still on device page
2. Click "Create Ledger"
3. Name: `device-settings`
4. Scope: Device
5. Direction: Cloud→Device
6. Paste JSON from `device-status`

**Step 3: Modify Only What's Different**
Example - Different park hours:
```json
{
    "timing": {
        "timezone": "PST8PDT",
        "reportingIntervalSec": 3600,
        "pollingRateSec": 0,
        "openHour": 7,          ← Changed from 6
        "closeHour": 20         ← Changed from 22
    }
}
```

**Tip:** You only need to include sections with changes. Device merges with defaults.

### Common Customizations

**Different Operating Hours:**
```json
{
    "timing": {
        "openHour": 8,
        "closeHour": 18
    }
}
```

**Testing Device (Stay Connected, Verbose Logs):**
```json
{
    "messaging": {
        "serial": true,
        "verboseMode": true
    },
    "modes": {
        "operatingMode": 0
    }
}
```

**Different Sensor Type:**
```json
{
    "sensor": {
        "threshold1": 75,
        "threshold2": 50
    }
}
```

**Occupancy Mode Instead of Counting:**
```json
{
    "modes": {
        "countingMode": 1,
        "occupancyDebounceMs": 600000
    }
}
```

### Verifying Configuration Applied

1. After creating/editing `device-settings`, wait for device to connect (or force connection)
2. Check `device-status` ledger - should match your `device-settings`
3. Check device logs for "Configuration loaded" messages
4. If mismatch, check logs for validation errors

## Extending the Firmware

### Adding a New Sensor

1. **Create sensor class** implementing `ISensor` interface:
```cpp
class MyNewSensor : public ISensor {
public:
    static MyNewSensor* instance();
    bool setup() override;
    bool loop() override;
    SensorData getSensorData() override;
    const char* getSensorType() const override;
};
```

2. **Add to SensorFactory.h**:
```cpp
case MYNEWSENSOR:
    return MyNewSensor::instance();
```

3. **Update Config.h** with new sensor type enum

4. **No changes needed** to core state machine or data handling!

## Memory Constraints

- No `String` allocations in hot path (loop)
- Static buffers for frequently-used data (deviceID)
- Stack allocation for JSON (512 bytes acceptable)
- Persistent storage auto-managed by StorageHelperRK

## Offline Data Retention & Firmware Updates

- **Persistent publish queue**: All cloud publishes go through PublishQueuePosixRK, which buffers events in RAM and on the `/usr` flash filesystem.
- **30+ days of hourly data**: The firmware configures a file-backed queue size of 800 events, allowing at least 30 days of hourly reports to be retained during extended network outages before the oldest events are discarded.
- **Guaranteed retry on reconnect**: Buffered events are sent on subsequent connections with `WITH_ACK` enabled; events are only removed from the queue after successful delivery.
- **Sleep-aware queue handling**: The state machine checks that the publish queue is in a sleep-safe state before entering long low-power sleeps, avoiding data loss due to mid-flight publishes.
- **Bounded firmware-update mode**: The FIRMWARE_UPDATE state is time-limited (5 minutes by default). If no updates are applied within this window, the device exits update mode and returns toward its normal connect/report/sleep cycle to protect battery life.

## Error Handling & Alert System

The firmware implements a comprehensive alert system that monitors device health and reports issues through webhooks:

### Alert Severity Levels

**Critical (Tier 3)** - Requires immediate attention:
- `14`: Out of memory
- `15`: Modem/disconnect failure
- `16`: Repeated sleep failures (HIBERNATE/ULP)
- `20`: **PMIC thermal shutdown** (charging stopped due to temperature)
- `21`: **PMIC charge timeout** (stuck charging - safety timer expired)

**Major (Tier 2)** - Should be addressed soon:
- `22`: **PMIC input fault** (VBUS overvoltage)
- `23`: **PMIC battery fault** (general charging issue)
- `30`: Connectivity timeout with radio up
- `31`: Failed to connect to cloud
- `32`: Connect taking too long
- `40`: Repeated webhook failures (>6 hours without response)
- `41`: Configuration/ledger apply failure
- `42`: Data ledger publish failure
- `43`: Publish queue not drained before forced sleep

**Minor (Tier 1)** - Informational warnings

### PMIC Monitoring & Remediation (Boron Only)

For Boron devices with BQ24195 PMIC, the firmware actively monitors charging health:

**Detection:**
- Reads PMIC fault registers every battery check cycle
- Monitors for thermal shutdown, charge timeout, input faults
- Tracks stuck charging states (>6 hours at same SoC)
- Logs detailed PMIC status (charge state, VBUS, thermal regulation)

**Smart Remediation with Anti-Thrashing:**
- **Level 0** (Initial): Monitor only, log diagnostics
- **Level 1** (2+ consecutive faults): Soft reset - cycle charging off/on
- **Level 2** (3+ consecutive faults): Power cycle with watchdog supervision
- **Cooldown**: 1 hour minimum between remediation attempts
- **Auto-Clear**: Resets counters when charging returns to healthy state

**Escalation Example:**
1. First fault detected → Alert raised, monitor only (wait for cooldown)
2. Second fault after cooldown → Level 1: Cycle charging (disable 500ms, re-enable)
3. Third fault after cooldown → Level 2: Power cycle with watchdog reset
4. Charging recovers → Clear alert, reset remediation level

**Benefits:**
- Automatic recovery from common "1Hz amber LED" charging faults
- Prevents thrashing (repeated fix attempts)
- Detailed diagnostic logs for root cause analysis
- Alert webhooks notify monitoring systems before manual intervention needed

### Error Recovery Strategy

- Sensor failures → Connect and report (no reset loops)
- Configuration validation with range checking
- Graceful degradation for remote deployments
- Alerts automatically clear when underlying condition resolves

## Dependencies

- AB1805_RK: RTC and watchdog
- LocalTimeRK: Timezone support
- PublishQueuePosixRK: Reliable publishing
- StorageHelperRK: Persistent storage
- BackgroundPublishRK: Non-blocking publishes
- SequentialFileRK: File operations

## Contributing

When adding features:
1. **Maintain simplicity**: Keep core logic clean
2. **Comment thoroughly**: Explain non-obvious decisions
3. **Extend, don't modify**: Use factory pattern for new types
4. **Test remotely**: Ensure devices don't brick in field

## License

This project is licensed under the MIT License.

You are free to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of this software, subject to the
conditions of the MIT License.

See the top-level `LICENSE` file in this repository for the
complete license text.

## Support

For issues or questions, contact [Your Contact Info]

