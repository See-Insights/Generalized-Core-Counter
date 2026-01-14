# Bench Validation Checklist (Pre-Field)

This guide validates the **power-safety** and **non-blocking** behaviors required for unattended solar/cellular deployments.

## Goals

- Device reaches a **sleepable state** in all paths.
- **Modem/radio never stays powered unintentionally**.
- `loop()` stays responsive (no long blocking waits).
- Connect/publish failures still lead to **bounded-time** progress (timeouts + recovery).

## Recommended Test Config (Ledger)

Use short budgets on the bench so failures are observable quickly.

Suggested values (Device ledger `device-settings` → `modes` / `timing`):

- `timing.reportingIntervalSec`: `300` (5 min)
- `modes.operatingMode`: `1` (LOW_POWER)
- `modes.connectAttemptBudgetSec`: `60` (1 min)
- `modes.cloudDisconnectBudgetSec`: `10`
- `modes.modemOffBudgetSec`: `15`
- `messaging.serial`: `true`
- `messaging.verboseMode`: optional (`true` for more logs)

Also set open/close hours wide open for bench testing:

- `timing.openHour`: `0`
- `timing.closeHour`: `24`

## Bench Setup

- Put the device on a **bench supply** or a known-good LiPo.
- Measure current with either:
  - inline USB power meter (rough), or
  - DMM in series / power analyzer (preferred).
- Have a way to degrade connectivity:
  - remove antenna (cellular) / shield the device, or
  - power it where reception is intentionally poor.

## What to Watch in Logs

Use your usual serial log view. Key messages to confirm:

- Entering sleep: `... entering SLEEPING_STATE`
- Disconnect phases:
  - `Requesting Particle cloud disconnect before sleep`
  - `Cloud disconnected - powering down modem before sleep`
  - `Modem powered down successfully before sleep`
- Connect attempt lifecycle:
  - `Requesting Particle cloud connection`
  - `Connected in ... secs`
  - On failure: `Connection attempt exceeded budget ... raising alert 31`
- Error safety:
  - `Entering ERROR_STATE ...`
  - (You should see modem/radio get powered down immediately on entry.)

## Test 1 — Normal Wake → Connect → Publish → Sleep

**Purpose:** Prove the happy path reaches low current sleep and modem powers down.

Steps:

1. Ensure good reception.
2. Let the device run for one reporting interval.
3. Confirm it connects, drains the publish queue, then sleeps.

Pass criteria:

- You see the connect logs, then `SLEEPING_STATE` logs.
- The modem power-down log appears before sleep.
- Current drops to a stable low level after sleep.

## Test 2 — Forced Connect Failure (Budget Timeout)

**Purpose:** Prove the modem does not stay powered after a failed connect attempt.

Steps:

1. Degrade reception (shield/remove antenna) so cloud connect fails.
2. Trigger a report (wait for interval or press the user button to force reporting).
3. Observe `Connection attempt exceeded budget ... alert 31`.

Pass criteria:

- Within the same cycle, the device transitions into `ERROR_STATE`.
- Modem powers down (current drops) while waiting for backoff/reset.
- Device does not sit indefinitely in a “modem on, not connected” state.

## Test 3 — Cloud Disconnect Budget Exceeded

**Purpose:** Validate bounded disconnect and that you still end up safe.

Steps:

1. Start from connected.
2. Force the device into sleep (wait for low-power idle transition).
3. If cloud won’t disconnect quickly, the disconnect budget should trip.

Pass criteria:

- You see a warning about disconnect exceeding budget and alert 15.
- Device enters `ERROR_STATE`.
- Modem powers down while in error handling.

## Test 4 — Sleep Return / Sleep Not Honored

**Purpose:** Detect pathological sleep behavior that can cause rapid wake/sleep battery drain.

Steps:

1. Configure open/close so it will attempt a long sleep (or leave open hours normal and test ULTRA_LOW_POWER).
2. Confirm it sleeps for a meaningful amount of wall-clock time.

Pass criteria:

- If a long sleep returns immediately, you should see an alert 16 and transition to `ERROR_STATE`.

## Test 5 — Loop Responsiveness (No Blocking)

**Purpose:** Confirm no long waits for serial, and loop continues servicing background tasks.

Steps:

1. Boot with USB serial disconnected.
2. Confirm the device proceeds immediately (no 10s wait + delay).
3. Confirm periodic operations continue (publish queue loop, state machine). 

Pass criteria:

- No long “startup pause” waiting for serial.
- Device still reaches a sleep state and/or runs its state machine.

## Quick Interpretation of Alerts

- `31`: connect attempt budget exceeded
- `15`: disconnect/modem power-down budget exceeded
- `16`: sleep returned unexpectedly or not honored

## If Something Fails

Most common failure signatures and what they mean:

- **Current stays high after a connect timeout:** modem likely remained on (verify `Cellular.off()` path hit; check for ERROR_STATE entry).
- **Device never enters SLEEPING_STATE:** publish queue might not be sleepable, or update mode may be holding connection.
- **Repeated rapid wakes:** sleep not being honored; treat as critical.
