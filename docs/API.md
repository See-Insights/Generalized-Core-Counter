# API

## Overview

This document describes the REST-facing API relationships for device state and telemetry consumers.

Ledger payload schemas are not duplicated here. API responses that expose ledger-backed device state return the contracts defined in [docs/contracts/ledger-contracts.md](contracts/ledger-contracts.md).

Use this document to understand endpoint responsibilities and response relationships. Use the ledger contract document for field-level payload definitions.

---

# Contracts

## Device Status Ledger Contract

Device status APIs expose the Device Status Ledger Contract defined in [docs/contracts/ledger-contracts.md](contracts/ledger-contracts.md).

Device Status represents current operational state published by firmware.

## Device Data Ledger Contract

Device data APIs expose the Device Data Ledger Contract defined in [docs/contracts/ledger-contracts.md](contracts/ledger-contracts.md).

Device Data represents the latest observation produced by the sensor runtime.

## Canonical Event Envelope

Telemetry/event APIs wrap event payloads in the Canonical Event Envelope used by the telemetry platform.

The envelope defines event identity, device identity, timestamps, and payload metadata. Ledger contracts define the shape of ledger-backed device state inside that broader API model.

---

# REST Endpoints

## Device Status

### `GET /devices/{deviceId}/status`

Returns the latest Device Status Ledger Contract for a device.

Response relationship:

- Contract: Device Status Ledger Contract
- Source: Particle Device Status ledger
- Semantics: current operational state

## Device Data

### `GET /devices/{deviceId}/data/latest`

Returns the latest Device Data Ledger Contract for a device.

Response relationship:

- Contract: Device Data Ledger Contract
- Source: Particle Device Data ledger
- Semantics: latest observation only

## Device Events

### `GET /devices/{deviceId}/events`

Returns historical telemetry events using the Canonical Event Envelope.

Response relationship:

- Contract: Canonical Event Envelope
- Source: telemetry platform
- Semantics: historical event stream

## Device Summary

### `GET /devices/{deviceId}/summary`

Returns a summary view composed from device state and telemetry sources.

Response relationship:

- Current operational state: Device Status Ledger Contract
- Latest observation: Device Data Ledger Contract
- Historical events: Canonical Event Envelope

---

# Design Rules

- This document defines endpoint responsibilities and response relationships.
- Ledger JSON schemas are defined only in [docs/contracts/ledger-contracts.md](contracts/ledger-contracts.md).
- APIs that expose device state should reference ledger contracts rather than redefining fields.
- Historical APIs should use the Canonical Event Envelope rather than treating ledgers as history.
