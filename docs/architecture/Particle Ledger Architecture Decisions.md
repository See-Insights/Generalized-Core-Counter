# ADR: Ledger Architecture

## Purpose

The firmware exposes four Particle Ledgers that together form the device's digital twin.

Each ledger has a single responsibility. Preserving this separation is important as the fleet scales because ledger contracts become increasingly expensive to change once cloud monitoring, dashboards, and automation depend on them.

The guiding model is:

```
Product Default + Device Settings = Intent

Device Status = Reality

Device Data = Observation

Telemetry Platform = History
```

---

# Ledger Responsibilities

| Ledger | Owner | Purpose |
|---------|--------|---------|
| Product Default | Engineering | Defines the default configuration for every device in the product. |
| Device Settings | Fleet Operator | Stores per-device configuration overrides. |
| Device Status | Firmware | Reports the current operational state of the device. |
| Device Data | Sensor Runtime | Publishes the latest observation from the device. |

---

# 1. Product Default Ledger

## Purpose

Defines how every device of this product should operate.

Managed by engineering.

Changes infrequently.

Examples include:

- sensor type
- reporting interval
- operating hours
- sleep policy
- connection policy
- firmware capabilities
- feature flags

This ledger represents engineering intent.

---

# 2. Device Settings Ledger

## Purpose

Contains customer- or deployment-specific overrides to the Product Default.

Typically sparse.

Only values that differ from Product Default are stored.

Examples include:

- reporting interval override
- operating hours override
- calibration values
- site-specific options

Firmware merges

```
Product Default
        +
Device Settings
```

to produce the effective runtime configuration.

The cloud should perform the same merge.

Configuration is intentionally **not duplicated** into Device Status.

This preserves a single source of truth.

---

# 3. Device Status Ledger

## Purpose

Represents the current operational state of the device.

Updated every successful cloud connection.

Contains runtime information only.

Typical examples include:

- firmware version
- nextReportEpoch
- connection result
- battery state
- power source
- active input profile
- sleep state
- pending ledger synchronization
- boot count
- reset reason
- configuration generation (or deterministic hash)

The monitoring platform uses Device Status to answer questions such as:

- Is the device healthy?
- Is it behaving normally?
- When should it report next?
- Is it using the expected configuration?
- Does it appear offline?

### Device Status Is Ephemeral

Device Status represents the device's current operational state.

It is **not** intended to be a historical record.

Historical analysis belongs in telemetry, logs, or cloud databases.

Device Status should remain compact.

---

# 4. Device Data Ledger

## Purpose

Contains the latest observation produced by the device.

Sensor-specific.

Examples include:

- occupancy
- counts
- environmental values
- battery snapshot
- free heap
- reset reason

This ledger answers:

> "What is the most recent observation?"

---

# Design Principles

## Single Source of Truth

Configuration lives only in:

- Product Default
- Device Settings

Device Status must never duplicate configuration.

---

## Firmware Is Authoritative

Firmware is the authoritative source for device behavior.

The monitoring platform consumes operational state but should not attempt to infer or recreate firmware decision logic.

For example, the firmware publishes `nextReportEpoch` rather than requiring the cloud to reimplement scheduling logic.

---

## Firmware Owns Scheduling

The cloud should not reimplement firmware scheduling algorithms.

Instead, firmware publishes:

- nextReportEpoch

The monitoring platform simply compares the current time with `nextReportEpoch`.

This allows scheduling algorithms to evolve without requiring cloud changes.

---

## Runtime vs Diagnostics

**Logs are for diagnosis. Ledgers are for operation.**

Device Status exists for operational monitoring.

Serial logs exist for diagnostics.

If information is only useful when debugging a problem, it belongs in logs rather than Device Status.

The benchtop monitoring platform is responsible for collecting detailed serial diagnostics during development and soak testing.

Production devices should publish only the information required for fleet operations.

---

## Configuration Synchronization

The cloud should be able to verify that the device is running the intended configuration.

Each device should publish a configuration generation (or deterministic hash) representing the effective merged configuration.

The cloud compares this value with the expected configuration.

Matching values indicate the device has successfully applied the intended configuration.

---

# Long-Term Architecture

The four ledgers together form the operational digital twin of every deployed device.

```
Intent
    Product Default
        +
    Device Settings

Reality
    Device Status

Observation
    Device Data

History
    Telemetry Platform
```

Keeping these responsibilities separate:

- minimizes duplication
- keeps payloads compact
- simplifies cloud logic
- allows firmware and monitoring capabilities to evolve independently
- provides a stable contract between devices and the monitoring platform

---

 # Particle Ledger identifiers.
 
 These names are part of the firmware/cloud contract and must remain synchronized with the Generalized Core Counter firmware. See ADR: Ledger Architecture.

 ---
 