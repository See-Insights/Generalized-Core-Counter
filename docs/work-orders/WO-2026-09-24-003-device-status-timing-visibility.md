# WO-2026-09-24-003: Publish effective open/close hours in the device-status payload

**Status:** FILED - not started, not scoped beyond this note. Recorded per
WO-2026-09-24-001 (Revised) Stage 5 decision 4.

**Type:** Small config-visibility improvement.

## Problem

There is currently **no way to observe a deployed device's effective
open/close hours from fleet telemetry.**

Established while auditing the fleet for WO-2026-09-24-001:

- `deviceSettingsLedgerData` reflects the *cloud-pushed override*, not the
  effective on-device value. Of 12 devices, 8 have it as `null` entirely and
  one (`SAMIT-TRAIL02`) has a partial record with `closeHour: 23` and no
  `openHour` at all.
- The **device-status** ledger publishes no `timing` block - confirmed
  against the payload builder in `cloud/DeviceStatusPublisher.cpp`. It
  carries `battery`, `clock`, `config`, `connection`, `firmware`, `power`,
  `reporting`, `startup`, `schemaVersion` - but not open/close hours.
- `Cloud::hasNonDefaultConfig()` (`DeviceStatusPublisher.cpp:645`) is a
  composite boolean spanning timezone, reporting interval, sensor settings
  *and* open/close, so it cannot isolate the question.

The effective values live in persisted `sysStatus` (or compiled defaults)
and are simply not visible from the cloud side.

## Proposed change

Add a `timing` block to the device-status payload carrying the effective
`openHour` / `closeHour` (and plausibly `timezone`), sourced from the same
accessors the open-hours logic itself reads, so what is published is what
the device actually uses.

## Why it matters

This gap directly blocked a Stage 5 question on WO-2026-09-24-001 ("which
deployed devices use `openTime == closeTime` for always-open?"), which could
not be answered from telemetry and had to be recorded as inconclusive. It
also blocks WO-2026-09-24-004.

## Notes

- Additive telemetry only. No behavior change, no layout change - the
  device-status ledger is a JSON payload, not a persisted struct.
- Post-Step-5, the accessors are `SystemConfig::get_openTime()` /
  `get_closeTime()`.
