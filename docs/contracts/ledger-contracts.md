# Ledger Contracts

## Overview

### Purpose

This document defines the public contract between the firmware and cloud monitoring platform for the Particle Ledgers.

The contracts are intended to remain:

- stable
- compact
- operationally focused

Ledgers describe current device state. They are not historical records, and they should not duplicate configuration or diagnostic detail that belongs in Product Default, Device Settings, serial logs, or telemetry storage.

### Versioning

Every ledger contains a `schemaVersion` field.

Schema evolution follows these rules:

- Schema evolution should be additive whenever practical.
- Existing fields are not renamed or removed within a schema version.
- Consumers must ignore unknown fields.
- Breaking changes require a new schema version.

### Schema Evolution Rules

The firmware is the authoritative publisher for operational ledger state.

New fields should be added only when they support long-term operational decisions. Fields that are primarily useful for debugging should remain in logs or telemetry rather than becoming part of a ledger contract.

---

# Device Status Ledger

## Purpose

Device Status reports the current operational state of the device.

It answers questions such as:

- What firmware is running?
- What effective configuration generation is active?
- When did the device last report?
- When is it expected to report next?
- What are the current power, battery, and connection states?

## Ownership

Owner: Firmware.

Device Status is published by firmware after runtime state has been finalized for a successful cloud connection.

## Publishing Rules

Device Status is updated after a successful cloud connection.

Device Status describes runtime reality. It does not generally duplicate configuration from Product Default or Device Settings. The `config.generation` field identifies the effective merged configuration. `reporting.configuredIntervalSec` is a narrow operational exception: it exposes the baseline used by runtime reporting policy so consumers can interpret effective cadence without reconstructing firmware decisions.

## JSON Example

{
    "battery": {
        "chargeState": "DONE",
        "soc": 78.6,
        "vcell": 4.03
    },
    "config": {
        "generation": "BA69F072"
    },
    "connection": {
        "elapsedMs": 10317,
        "lastResult": "ok"
    },
    "firmware": {
        "resetCount": 22,
        "version": "20.0-dev"
    },
    "power": {
        "overrideActive": false,
        "profile": "UsbBench",
        "source": "USB_HOST"
    },
    "reporting": {
        "adjustmentReason": "none",
        "configuredIntervalSec": 3600,
        "effectiveIntervalSec": 3600,
        "lastReportEpoch": 1783583647,
        "nextReportEpoch": 1783587247,
        "windowOpen": true
    },
    "startup": {
        "epoch": 1784550125,
        "reason": "pin-reset",
        "firmware": "20.0-dev",
        "deviceOS": "6.4.1",
        "resetCount": 22
    },
    "schemaVersion": 2
}

## Field Definitions

| Field | Description |
|--------|-------------|
| schemaVersion | Ledger schema version. |
| firmware.version | Running firmware version. |
| firmware.resetCount | Number of firmware resets recorded by the device. |
| config.generation | Deterministic identifier representing the effective merged Product Default and Device Settings configuration. |
| reporting.lastReportEpoch | Epoch timestamp when firmware most recently generated an application report and attempted its local queue and device-data ledger handoff. It does not indicate queue acceptance, ledger synchronization, webhook delivery, or another successful publication event. |
| reporting.nextReportEpoch | The next cloud/application reporting opportunity the firmware currently intends to attempt after applying all active runtime reporting policies. This firmware-published value is authoritative for Fleet Operations. It is battery-adjusted, epoch-aligned, and constrained to configured open hours; zero means firmware cannot currently identify a valid future opportunity. |
| reporting.configuredIntervalSec | Configured reporting interval before runtime policy adjustments. |
| reporting.effectiveIntervalSec | Effective reporting cadence selected by firmware after runtime policy adjustments. |
| reporting.adjustmentReason | Why the configured interval differs from the effective interval. Current values are `none` and `low-battery`; consumers must tolerate future additive values. |
| reporting.windowOpen | Whether the reporting window is currently open. |
| startup.epoch | Epoch when firmware initialization successfully completed. If valid time is unavailable during initialization, the timestamp is finalized once after time synchronization and then remains immutable for the lifetime of the execution. |
| startup.reason | Reset reason for the current execution instance. |
| startup.firmware | Firmware version running during this execution instance. |
| startup.deviceOS | Particle Device OS version for this execution instance. |
| startup.resetCount | Firmware-maintained reset counter captured at initialization. It is not a lifetime boot counter, does not increment on normal sleep/wake cycles, and reflects tracked firmware reset events only. |
| power.source | Current Particle power source classification. |
| power.profile | Active firmware power profile. |
| power.overrideActive | True when firmware is using an override or fallback power-source interpretation. |
| battery.soc | Battery state of charge percentage. |
| battery.vcell | Battery cell voltage. |
| battery.chargeState | Current compact PMIC charge state. |
| connection.lastResult | Result of the most recent connection attempt. |
| connection.elapsedMs | Elapsed time for the most recent successful connection. |

## Reporting Cadence and Hardware Wake Cadence

Cloud/application reporting cadence and hardware wake cadence are separate concepts. Firmware may wake more frequently to sense, count, maintain occupancy state, or re-evaluate policy while intentionally delaying cloud communication.

Fleet Operations must interpret `reporting.nextReportEpoch` as the next cloud/application reporting opportunity firmware currently intends to attempt. It must not interpret that value as the next hardware wake, derive reporting cadence from battery state, or reconstruct firmware scheduling policy.

The reporting contract follows this ownership flow:

```text
Configuration
    -> Runtime reporting policy
    -> Effective reporting schedule
    -> Published Device Status
    -> Fleet Operations
```

## Startup Snapshot

The `startup` object describes the current execution instance of the firmware. It is immutable for the lifetime of that execution and is replaced only after the next successful initialization.

It is not current runtime state, an event log, or reboot history.

The device-status ledger persists the most recently published startup snapshot in the cloud. Firmware does not add another persistent store for this object; it retains the current execution's snapshot in RAM until publication.

### Runtime vs Startup

Current runtime values in `battery`, `reporting`, `power`, and `connection` continue changing as device conditions and policy decisions change. The `startup` object remains constant until the next reboot. This distinction is intentional.

---

# Device Data Ledger

## Purpose

Device Data publishes the most recent observation produced by the device.

It answers:

> What is the latest observation?

Device Data is not historical. Historical analysis belongs in the telemetry platform.

## Ownership

Owner: Sensor Runtime.

Device Data is produced from the current sensor/runtime observation state.

## Publishing Rules

Device Data contains the latest observation only.

The payload is sensor-focused and should remain compact. It should not accumulate prior observations or attempt to represent history.

## JSON Example

{
    "battery": {
        "soc": 78.6
    },
    "environment": {
        "temperature": 27.9
    },
    "occupancy": {
        "occupied": false,
        "totalOccupiedSec": 4574
    },
    "schemaVersion": 2,
    "system": {
        "freeHeap": 77768,
        "resetReason": 20,
        "resetReasonData": 0
    },
    "timestamp": 1783584002
}

## Field Definitions

| Field | Description |
|--------|-------------|
| schemaVersion | Ledger schema version. |
| timestamp | Epoch timestamp when firmware generated this latest structured observation. It is not a ledger-sync-success timestamp. |
| occupancy.occupied | Whether the monitored space is currently occupied. |
| occupancy.totalOccupiedSec | Accumulated occupied duration for the current reporting period, in seconds. |
| environment.temperature | Latest internal or environmental temperature observation. |
| battery.soc | Battery state of charge percentage at observation time. |
| system.freeHeap | Available heap at observation time. |
| system.resetReason | Device OS reset reason code. |
| system.resetReasonData | Device OS reset reason data. |

---

# Compatibility

## Schema Version History

### Schema Version 1

- Initial production contract.
- Introduced Device Status V1.
- Introduced Device Data V1.
