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

Device Status describes runtime reality. It does not duplicate configuration from Product Default or Device Settings. The `config.generation` field identifies the effective merged configuration without copying configuration values into the status ledger.

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
        "resetCount": 5,
        "version": "20.0-dev"
    },
    "power": {
        "overrideActive": false,
        "profile": "UsbBench",
        "source": "USB_HOST"
    },
    "reporting": {
        "lastReportEpoch": 1783583647,
        "nextReportEpoch": 1783587247,
        "windowOpen": true
    },
    "schemaVersion": 1
}

## Field Definitions

| Field | Description |
|--------|-------------|
| schemaVersion | Ledger schema version. |
| firmware.version | Running firmware version. |
| firmware.resetCount | Number of firmware resets recorded by the device. |
| config.generation | Deterministic identifier representing the effective merged Product Default and Device Settings configuration. |
| reporting.lastReportEpoch | Epoch timestamp of the most recent report. |
| reporting.nextReportEpoch | Epoch timestamp of the next scheduled report calculated by firmware. |
| reporting.windowOpen | Whether the reporting window is currently open. |
| power.source | Current Particle power source classification. |
| power.profile | Active firmware power profile. |
| power.overrideActive | True when firmware is using an override or fallback power-source interpretation. |
| battery.soc | Battery state of charge percentage. |
| battery.vcell | Battery cell voltage. |
| battery.chargeState | Current compact PMIC charge state. |
| connection.lastResult | Result of the most recent connection attempt. |
| connection.elapsedMs | Elapsed time for the most recent successful connection. |

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
    "schemaVersion": 1,
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
| timestamp | Epoch timestamp for this observation. |
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
