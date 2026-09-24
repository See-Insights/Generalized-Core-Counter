# WO-2026-09-24-004: Retire the `openHour == closeHour` always-open convention

**Status:** FILED - **BLOCKED**, not started. Recorded per
WO-2026-09-24-001 (Revised) Stage 5 decision 4.

**Blocked on:** an authoritative per-site configuration audit. Not blocked
on any code question.

## The convention

`Clock.cpp:21-23` treats `openHour == closeHour` as "always open":

```cpp
} else {
  // openHour == closeHour: treat as always open
  return true;
}
```

The intended replacement is an explicit `0`/`24` configuration, which would
make always-open a normal window rather than a sentinel value.

## Why it is blocked

Retiring the convention requires confirming that **no deployed device relies
on it** - and that cannot currently be established:

| Devices | Open/close visibility | Result |
|---|---|---|
| Dev-09, Dev-11, Dev-14 | Settings ledger populated | `open=6 close=22` - not always-open |
| 8 production devices (ToM-MCP-*, Morrisville-Tennis-*) | `deviceSettingsLedgerData` is `null` | **Unknown** |
| SAMIT-TRAIL02 | Partial: `closeHour=23`, no `openHour` | **Unknown - live candidate.** If its on-device `openTime` is also 23, it is in always-open mode today |

The settings ledger is the cloud-pushed override, not the effective
on-device value, and the device-status ledger publishes no timing block at
all. See WO-2026-09-24-003, which would close exactly this visibility gap.

## Unblocking path

Either:

1. Land WO-2026-09-24-003 (publish effective open/close hours in the
   device-status payload), wait for the fleet to report, then audit; or
2. Audit the authoritative per-site deployment configuration directly,
   through whatever source of record holds it - not fleet telemetry.

Then, if no device depends on the sentinel, migrate any that do to explicit
`0`/`24` and remove the branch.

## Interaction with WO-2026-09-24-001

WO-2026-09-24-001 (Revised) **normalizes** the sentinel inline
(`int close = (openHour == closeHour) ? 24 : closeHour;`) rather than
retiring it. That is deliberate: the daily-reset fix must keep working for
always-open devices whether or not the sentinel is eventually removed, and
it must not depend on this WO landing first.
