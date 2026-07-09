# Ledger Contracts

**Version:** 1  
**Status:** Active  
**Applies To:** Generalized Core Counter Firmware

---

# Purpose

This document defines the public contract between the firmware and cloud monitoring platform for the Particle Ledgers.

The goal is to keep the contracts:

- stable
- compact
- operationally focused

The ledger contracts intentionally expose only the information required for fleet operations.

Detailed diagnostics belong in serial logs and telemetry rather than ledger payloads.

---

# General Principles

## Compatibility

The firmware and monitoring platform communicate through versioned ledger contracts.

Every ledger includes a `schemaVersion` field.

Future changes should be additive whenever practical.

---

## Operational State

Ledgers describe the current state of the device.

They are **not** intended to be historical records.

Historical analysis belongs in the telemetry platform.

---

## Compact by Design

Every published field should support an operational decision.

If a field is only useful during firmware debugging, it belongs in diagnostic logs rather than a ledger.

---

# Device Status Ledger

## Purpose

Reports the current operational state of the device.

Updated after every successful cloud connection.

---

## Schema

```json
{
  "schemaVersion": 1,

  "firmware": {
    "version": "25.01",
    "resetCount": 412
  },

  "config": {
    "generation": "4A31B62E"
  },

  "reporting": {
    "lastReportEpoch": 1783508400,
    "nextReportEpoch": 1783512000,
    "windowOpen": true
  },

  "power": {
    "source": "USB_HOST",
    "profile": "UsbBench",
    "overrideActive": false
  },

  "battery": {
    "soc": 80.4,
    "vcell": 4.02,
    "chargeState": "DONE"
  },

  "connection": {
    "lastResult": "ok",
    "elapsedMs": 2010
  },

  "ledger": {
    "pendingData": false,
    "pendingStatus": false
  },

  "sleep": {
    "mode": "hibernate"
  }
}
```

---

## Field Definitions

| Field | Description |
|--------|-------------|
| schemaVersion | Ledger schema version. |
| firmware.version | Running firmware version. |
| firmware.bootCount | Number of firmware boots since deployment. |
| config.generation | Deterministic identifier representing the merged Product Default + Device Settings configuration. |
| reporting.lastReportEpoch | Time of the most recent report. |
| reporting.nextReportEpoch | Next scheduled report time calculated by firmware. |
| reporting.windowOpen | Whether the reporting window is currently open. |
| power.source | Current Particle power source classification. |
| power.profile | Active firmware power profile. |
| power.overrideActive | True if firmware overrode the reported power source. |
| battery.soc | Battery state of charge. |
| battery.vcell | Battery voltage. |
| battery.chargeState | Current PMIC charging state. |
| connection.lastResult | Result of the most recent connection attempt. |
| connection.elapsedMs | Time required for the most recent successful connection. |
| ledger.pendingData | Device Data ledger awaiting synchronization. |
| ledger.pendingStatus | Device Status ledger awaiting synchronization. |
| sleep.mode | Sleep mode entered after the last reporting cycle. |

---

# Device Data Ledger

## Purpose

Publishes the most recent observation produced by the device.

Contents are sensor specific.

---

## Schema

```json
{
  "schemaVersion": 1,

  "timestamp": 1783508400,

  "occupancy": {
    "occupied": false,
    "totalOccupiedSec": 0
  },

  "environment": {
    "temperature": 30.3
  },

  "battery": {
    "soc": 80.4
  },

  "system": {
    "freeHeap": 78112
  }
}
```

---

## Field Definitions

| Field | Description |
|--------|-------------|
| schemaVersion | Ledger schema version. |
| timestamp | Epoch timestamp for this observation. |
| occupancy | Latest occupancy state and accumulated occupancy duration. |
| environment | Latest environmental sensor observations. |
| battery.soc | Current battery state of charge. |
| system.freeHeap | Available heap at time of observation. |

---

# Design Rules

The following rules apply to all ledger contracts:

- Ledgers describe the current state only.
- Firmware is the authoritative source of operational state.
- Configuration is never duplicated into Device Status.
- Scheduling decisions remain inside the firmware.
- Monitoring platforms consume ledger information but do not recreate firmware decision logic.
- Diagnostic information belongs in serial logs rather than ledger payloads unless it directly supports fleet operations.

---

# Future Evolution

New fields should be added only when they provide long-term operational value.

Before adding a field, ask:

> Would a fleet operator reasonably use this information to make an operational decision?

If the answer is **no**, the information should normally remain in diagnostic logging rather than becoming part of the ledger contract.
