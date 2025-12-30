# Generalized-Core-Counter Style & Usage Guide

This document captures the main conventions and usage patterns for the Generalized-Core-Counter firmware so future changes stay consistent.

## File Responsibilities

- **src/Generalized-Core-Counter.cpp**
  - Main application entry point (`setup()` / `loop()`).
  - State machine for power, connectivity, reporting, and error handling.
  - High-level policy decisions (when to sleep, when to connect, when to report).
  - Top-level use of sensors via `SensorManager`, `SensorFactory`, and `SensorDefinitions`.

- **src/device_pinout.h / src/device_pinout.cpp**
  - Map logical roles (BUTTON_PIN, WAKEUP_PIN, intPin, disableModule, ledPower, etc.) to physical pins.
  - Configure pin modes that are purely electrical (input/output, default pull-ups) and do **not** depend on runtime configuration.
  - Do **not** embed application policy here (no state machine logic, no sensor-type conditionals).

- **src/MyPersistentData.* (sysStatus, sensorConfig, current)**
  - Persisted configuration, thresholds, and runtime counters across resets.
  - Getters/setters and small helpers only; no business logic.
  - Alert code storage and severity ranking (`current.raiseAlert`).

- **src/SensorManager.*, SensorFactory.*, SensorDefinitions.* and sensor classes**
  - Encapsulate hardware-specific sensor behavior (PIR, ultrasonic, etc.).
  - Implement the `ISensor` interface and any sensor-specific power/wake handling.
  - Avoid direct knowledge of cloud, queueing, or state machine.

- **lib/**
  - Third-party or shared libraries (AB1805_RK, PublishQueuePosixRK, LocalTimeRK, etc.).
  - Treat these as external dependencies; do not modify them unless absolutely necessary.

## Naming & Structure

- **Types & classes**: `CamelCase` (e.g., `SensorManager`, `Cloud`, `AB1805`).
- **Functions & methods**: `lowerCamelCase` (e.g., `initializeFromConfig`, `handleCountingMode`).
- **Global flags/variables**:
  - Descriptive, lowerCamelCase: `userSwitchDetected`, `onlineWorkStartMs`, `forceSleepThisCycle`.
  - Avoid one-letter names outside of tight, local scopes.
- **Enums**:
  - Use UPPER_SNAKE for values (e.g., `COUNTING`, `OCCUPANCY`, `LOW_POWER`).
  - Keep state machine enum (`State`) values tightly scoped in the main file.

## Configuration Model (Ledgers)

The firmware uses four ledgers with a simple, consistent schema:

- `default-settings` (Product scope): product-wide defaults.
- `device-settings` (Device scope): per-device overrides.
- `device-status` (Device scope): device-published snapshot of effective configuration.
- `device-data` (Device scope): device-published sensor data and derived metrics.

### Common JSON structure (default-settings & device-settings)

Top-level sections:

- `sensor`
  - `threshold1` (number) – primary sensor threshold.
  - `threshold2` (number) – secondary threshold.
- `timing`
  - `timezone` (string, POSIX TZ).
  - `reportingIntervalSec` (int, 300–86400).
  - `openHour` (int, 0–23).
  - `closeHour` (int, 0–23).
- `modes`
  - `operatingMode` (int):
    - `0` – CONNECTED.
    - `1` – LOW_POWER.
    - `2` – DISCONNECTED.
  - `countingMode` (int):
    - `0` – COUNTING (interrupt).
    - `1` – OCCUPANCY (interrupt).
    - `2` – SCHEDULED (time-based).
  - `occupancyDebounceMs` (uint, 0–600000).
  - `connectedReportingIntervalSec` (int, 60–86400).
  - `lowPowerReportingIntervalSec` (int, 300–86400).
- `power`
  - `solarPowerMode` (bool).
- `messaging`
  - `serial` (bool).
  - `verboseMode` (bool).

Additional convenience key:

- `sensorThreshold` (number) – optional generic threshold; when present, applied to both `threshold1` and `threshold2` unless overridden inside `sensor`.

### Device-status schema

Published from the device as:

- `sensor`
  - `threshold1`, `threshold2` – effective thresholds in use.
- `timing`
  - `timezone`, `reportingIntervalSec`, `openHour`, `closeHour`.
- `power`
  - `lowPowerMode` (bool) – derived from `operatingMode`.
  - `solarPowerMode` (bool).
- `modes`
  - `countingMode` (0/1/2).
  - `operatingMode` (0/1/2).
  - `occupancyDebounceMs`.
  - `connectedReportingIntervalSec`.
  - `lowPowerReportingIntervalSec`.
- `firmware`
  - `version`.
  - `notes`.

### Device-data schema

Published from the device after each report:

- `timestamp` (int) – Unix time (UTC).
- `mode` (string):
  - `"counting"` when `countingMode == COUNTING`.
  - `"occupancy"` when `countingMode == OCCUPANCY`.
  - `"scheduled"` when `countingMode == SCHEDULED`.
- `hourlyCount` (int) – counting mode only.
- `dailyCount` (int) – counting mode only.
- `occupied` (bool) – occupancy mode only.
- `totalOccupiedSec` (int) – occupancy mode only.
- `battery` (float, %) – SoC.
- `temp` (float, °C) – internal temperature.

## State Machine Usage

- All high-level behavior flows through the `State` enum in `Generalized-Core-Counter.cpp`:
  - `INITIALIZATION_STATE`
  - `IDLE_STATE`
  - `SLEEPING_STATE`
  - `REPORTING_STATE`
  - `CONNECTING_STATE`
  - `FIRMWARE_UPDATE_STATE`
  - `ERROR_STATE`
- Only `setup()` should choose the initial state. Transitions should happen inside `loop()`.
- Use `publishStateTransition()` whenever `state` changes so logs clearly show flow.
- Keep each state block focused:
  - `IDLE_STATE`: sensor processing, deciding whether to report or sleep.
  - `REPORTING_STATE`: build and enqueue payloads, decide whether to connect.
  - `CONNECTING_STATE`: manage Particle.connect lifecycle and configuration loads.
  - `SLEEPING_STATE`: configure and enter sleep, then handle wake reasons.
  - `FIRMWARE_UPDATE_STATE`: stay online for config/OTA updates.
  - `ERROR_STATE`: centralized error supervisor using `resolveErrorAction()`.

## Sleep, Wake & Power

- Use `SystemSleepConfiguration config` as a shared object, but always set mode and wake sources immediately before sleeping.
- For **night sleep** (outside open hours):
  - Use `SystemSleepMode::HIBERNATE` with a duration to next open.
  - Expect a full reset on wake; code after `System.sleep()` is a fallback only.

- For **daytime naps** (within open hours):
  - Use `SystemSleepMode::ULTRA_LOW_POWER` with:
    - Timer-based wake at the next reporting boundary (`wakeBoundary`).
    - GPIO wake on:
      - `BUTTON_PIN` (front-panel button) for service wake.
      - `intPin` (PIR interrupt) for motion wakes.

- Always stop the AB1805 watchdog before calling `System.sleep()` and resume it after wake.

- Track online work windows with `onlineWorkStartMs` and `maxOnlineWorkMs` so we can force sleep if backend issues keep the device online too long.

## Alerts & Error Handling

- Use `current.raiseAlert(code)` to record problems. Severity is determined centrally in `MyPersistentData.cpp`:
  - Critical (3): OOM (14), modem/disconnect failure (15).
  - Major (2): connect timeouts (30–32), webhook failures (40), config/ledger failures (41–43).
  - Minor (1): everything else.
- `raiseAlert` only upgrades the stored code if the new alert is **more severe** than the existing one.
- `resolveErrorAction()` in `Generalized-Core-Counter.cpp` maps the active alert + reset count to recovery behavior:
  - `0`: no automatic action (return to IDLE and keep operating).
  - `2`: soft reset via `System.reset()`.
  - `3`: deep power-down via AB1805.
- When an underlying condition recovers (for example, webhook responses resume after alert 40), clear the alert in application code so new reports reflect a healthy state.

## Logging Conventions

- Use `Log.info` for normal state and high-level events:
  - State transitions, sleep/wake events, hourly reports, connection attempts.
- Use `Log.warn` for conditions that are unusual but not immediately fatal.
- Use `Log.error` for failures that are likely to trigger alerts or recovery.
- Include enough context to make logs self-explanatory:
  - For wake events: reason, wake pin, and any derived behavior.
  - For queue and cloud: queue depth, `getCanSleep()` flag, `lastHookResponse` when relevant.
- Avoid logs inside ISRs; instead, set flags (like `userSwitchDetected`) and log from `loop()`.

## Button & Sensor Usage

- **BUTTON_PIN** (front-panel button):
  - Configured as a digital input in `initializePinModes()`.
  - Used for:
    - Startup override to force CONNECTED mode and clear low-power defaults.
    - Service-mode wake from sleep (if hardware supports wake on this pin).
    - On-press behavior in `loop()` via `userSwitchDetected` flag.
  - When debugging, the main loop logs changes on `BUTTON_PIN` so hardware interactions are visible in logs.

- **intPin / disableModule / ledPower** (sensor carrier pins):
  - `intPin`: interrupt from the primary sensor (PIR, etc.).
  - `disableModule`: sensor enable/disable control (active polarity is sensor-specific).
  - `ledPower`: power for the sensor-board LED; default state is chosen in `setup()` based on `sysStatus.get_sensorType()` and `SensorDefinitions` metadata.

## Queue & Cloud Usage

- Use `PublishQueuePosix::instance()` for all webhook publishes:
  - Enqueue data in `publishData()`.
  - Call `.loop()` once per main loop iteration.
  - Use `.getCanSleep()` and `.getNumEvents()` to gate sleep **only when connected or radio-on**.

- Cloud configuration and status:
  - Use `Cloud::instance().loadConfigurationFromCloud()` after a successful connect to merge and apply ledger-based config.
  - Use `Cloud::instance().publishDataToLedger()` to update the `device-data` ledger after each hourly report.

## General Usage Guidelines

- Prefer small, focused helpers over large, monolithic functions.
- Keep all hardware-specific assumptions localized:
  - Pin mappings and electrical details → `device_pinout.*` and sensor classes.
  - Application policy (when to turn LEDs on, how to interpret alerts) → `Generalized-Core-Counter.cpp`.
- When altering power behavior, consider:
  - Interaction with AB1805 watchdog.
  - Sleep mode differences (HIBERNATE vs ULTRA_LOW_POWER).
  - Impact on queue draining and hourly reporting guarantees.
- When adding new alerts or error paths:
  - Add a code and description to the `getAlertSeverity` switch.
  - Decide where to raise it (`current.raiseAlert`) and where to clear it once healthy.

This guide is intentionally concise; extend it as new patterns emerge so future changes remain predictable and maintainable.
