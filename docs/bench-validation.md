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

## Test 6 — PMIC Health Monitoring & Remediation (Boron Only)

**Purpose:** Validate PMIC fault detection and smart remediation system.

**Platform:** Boron only (cellular platforms with BQ24195 PMIC). Photon2/P2/Argon do not have this PMIC and will not execute this code path.

### Normal Operation Baseline

Steps:

1. Connect Boron with healthy battery (3.6V-4.2V range) to solar/USB power
2. Monitor logs during battery check cycles (occurs in REPORTING_STATE)
3. Look for healthy PMIC status logs

Expected logs (healthy state):
```
Battery: state=Charging (2), SoC=XX.XX%, powerSource=5
PMIC Status: charge=Fast Charging, VBUS=Good, thermal=Normal, faultReg=0x00
```

Pass criteria:
- `faultReg=0x00` (no faults)
- Charge status progresses: Pre-charge → Fast Charging → Charge Done
- No alerts raised
- consecutiveFaults counter stays at 0

### Simulating PMIC Faults (Advanced)

**⚠️ Warning:** Some tests require forcing fault conditions. Be prepared to power cycle.

#### Test 6A — Thermal Fault Detection

Steps:

1. Place Boron in warm environment while charging (e.g., heating pad, direct sunlight through window)
2. Monitor for thermal regulation in logs
3. Look for alert 20 if thermal shutdown occurs

Expected behavior:
- PMIC Status shows: `thermal=Warm` or `thermal=Hot`
- If thermal shutdown occurs: `PMIC: Thermal shutdown - charging stopped due to temperature`
- Alert 20 raised (critical severity)
- After cooldown + 1 hour: Level 1 remediation (cycle charging)

Pass criteria:
- Thermal status accurately reflected in logs
- Alert 20 raised on thermal shutdown
- Alert auto-clears when temperature normalizes

#### Test 6B — Charge Timeout Detection

This is the most common real-world fault ("stuck charging" with 1Hz amber LED).

Observation method (field data):

1. Deploy Boron with marginal solar setup or aging battery
2. Monitor for devices that stay in "Fast Charging" for >6 hours
3. Check logs for stuck charging detection

Expected logs:
```
PMIC: Stuck in Fast Charging for 6+ hours with no SoC increase (XX.X%) - possible fault
PMIC: Charge safety timer expired - charging timeout (common stuck charging indicator)
```

Expected behavior:
- Alert 21 raised (critical severity)
- After cooldown: Level 1 remediation (cycle charging off/on)
- If persists: Level 2 remediation (power cycle + watchdog)

Pass criteria:
- Alert 21 raised when charge timeout detected
- Remediation escalates from Level 1 → Level 2 over multiple cycles
- 1-hour cooldown prevents thrashing (see "in cooldown period" logs)
- Alert clears when charging resumes normally

#### Test 6C — Remediation Cooldown & Anti-Thrashing

**Purpose:** Confirm remediation doesn't thrash with repeated attempts.

Observation method:

1. Monitor device that triggers charging fault
2. Check timestamps of remediation attempts in logs
3. Verify >60 minutes between attempts

Expected logs:
```
PMIC: Attempting soft remediation - cycle charging (level 1)
[60+ minutes pass]
PMIC: Fault detected but in cooldown period (45 min remaining)
[more time passes]
PMIC: Attempting aggressive remediation - power cycle reset (level 2)
```

Pass criteria:
- Minimum 60 minutes between remediation attempts
- Cooldown countdown shown in logs
- Level escalates gradually: 0 → 1 (2+ faults) → 2 (3+ faults)
- Level resets to 0 after successful Level 2 attempt

#### Test 6D — Remediation Success & Alert Clearing

**Purpose:** Confirm alerts clear when charging recovers.

Steps:

1. Device with active alert 20, 21, 22, or 23
2. Resolve underlying issue (cool down device, replace battery, etc.)
3. Wait for next battery check cycle

Expected logs:
```
PMIC: Charging healthy - clearing fault counters
PMIC: Clearing battery/charging alert 21 - charging resumed
```

Pass criteria:
- Fault counters reset: `consecutiveFaults = 0`, `remediationLevel = 0`
- Alert code cleared: `current.get_alertCode() == 0`
- Subsequent reports show `alerts=0` in webhook payload

### PMIC Alert Webhook Integration

Verify alerts flow through existing webhook:

1. Check Ubidots webhook payload during fault
2. Confirm `"alerts":20` (or 21/22/23) appears in JSON
3. Monitor dashboard for alert visualization

Expected webhook payload (example):
```json
{
  "hourly":5,
  "daily":42,
  "battery":45.32,
  "key1":"Charging",
  "temp":23.4,
  "resets":2,
  "alerts":21,
  "connecttime":45,
  "timestamp":1737679200000
}
```

Pass criteria:
- Alert codes 20-23 appear in webhook during PMIC faults
- Alerts visible in monitoring dashboard
- Alert clears (returns to 0) when charging recovers

## Quick Interpretation of Alerts

### Connectivity Alerts
- `31`: connect attempt budget exceeded
- `15`: disconnect/modem power-down budget exceeded
- `16`: sleep returned unexpectedly or not honored

### Battery/Charging Alerts (Boron Only)
- `20`: **PMIC Thermal Shutdown** (critical) - charging stopped due to temperature
- `21`: **PMIC Charge Timeout** (critical) - stuck charging, safety timer expired
- `22`: **PMIC Input Fault** (major) - VBUS overvoltage detected
- `23`: **PMIC Battery Fault** (major) - general charging issue

## If Something Fails

Most common failure signatures and what they mean:

- **Current stays high after a connect timeout:** modem likely remained on (verify `Cellular.off()` path hit; check for ERROR_STATE entry).
- **Device never enters SLEEPING_STATE:** publish queue might not be sleepable, or update mode may be holding connection.
- **Repeated rapid wakes:** sleep not being honored; treat as critical.
