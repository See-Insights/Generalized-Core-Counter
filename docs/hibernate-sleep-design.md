# Hibernate Sleep Design

## Overview

This branch is intended to add a conservative hibernate sleep path for Boron-class devices running Device OS 6.4.1 or newer. The new path is intentionally opt-in, guarded by multiple checks, and must remain reversible through the existing sleep implementation.

The validated hardware path is:

AB1805 RTC Alarm -> FOUT/nIRQ -> Carrier WAKE line -> Boron D8 -> SystemSleepMode::HIBERNATE -> wake on D8 FALLING

## Architecture

The hibernate path should be selected only when all of the following are true:

- Boron platform
- Device OS 6.4.1 or newer
- Ledger setting `enableHibernateSleep` is enabled
- RTC time is valid
- Requested sleep duration is within the hibernate window
- AB1805 is present and usable

If any check fails, the firmware must fall back to the existing sleep implementation.

The hibernate path uses only RTC alarm scheduling:

```cpp
time_t wakeTime = rtcNow + sleepSeconds;
ab1805.interruptAtTime(wakeTime);
```

It must not use `deepPowerDown()`, `interruptCountdownTimer()`, or any countdown timer path.

## D8 Polarity Discovery

Field validation confirmed the wake path works only when the Boron wake input is treated as active-low and configured with a pull-up.

The validated behavior is:

- `pinMode(D8, INPUT_PULLUP)` before sleep
- wake source is `D8 FALLING`
- AB1805 FOUT/nIRQ is active LOW

This matches the observed hardware path from AB1805 alarm output to Boron hibernate wake.

## Input Pull-up Requirement

The wake pin must be held with an internal pull-up before sleeping:

```cpp
pinMode(D8, INPUT_PULLUP);
```

This should happen immediately before the hibernate configuration is armed.

## Wake Sources

Hibernate should use two GPIO wake sources:

```cpp
config.mode(SystemSleepMode::HIBERNATE)
      .gpio(D8, FALLING)
      .gpio(userSwitchPin, FALLING);
```

The secondary wake source exists for:

- field recovery
- technician wake-up
- RTC programming mistake recovery
- unexpected alarm behavior

## RTC Validity Gate

Hibernate is only allowed when the RTC is known valid.

Suggested validation:

- `rtcTime != 0`
- `rtcTime > Jan 1 2024`
- `rtcTime < Jan 1 2035`

If RTC validity cannot be established, the firmware must log a warning and fall back.

## Sleep Bounds

Hibernate should only be used for long sleeps.

```cpp
constexpr uint32_t MIN_HIBERNATE_SLEEP_SEC = 900;
constexpr uint32_t MAX_HIBERNATE_SLEEP_SEC = 21600;
```

The lower bound avoids paying reboot overhead for short sleeps. The upper bound prevents accidental multi-day or multi-year sleeps during soak.

The intended production ceiling may later be increased to 36000 seconds after field validation.

If the request exceeds the maximum, the firmware must log a warning and fall back to the existing implementation.

## Ledger Kill Switch

Add a ledger-controlled setting:

- `enableHibernateSleep` default `false`

Behavior:

- disabled: use existing sleep logic
- enabled: evaluate hibernate eligibility

This provides immediate rollback without code removal.

## Retained Diagnostics

Use retained RAM only. Do not add FRAM dependency for the diagnostic path.

Retained variables:

- `retained time_t retainedRtcBefore`
- `retained time_t retainedWakeTime`
- `retained uint32_t retainedRequestedSleep`
- `retained uint32_t retainedHibernateCount`

Before sleep:

- store `rtcBefore`
- store `wakeTime`
- store `requestedSleep`
- increment hibernate count

After wake:

- compute `actualSleep = rtcAfter - retainedRtcBefore`
- compute `sleepError = actualSleep - retainedRequestedSleep`
- log `requestedSleep`, `actualSleep`, `sleepError`, `wakeReason`, and `resetReason`

## Cloud Status Reporting

When waking from hibernate, publish status fields only for hibernate wake events:

```json
{
  "sleepMode": "hibernate",
  "wakeReason": "ALARM",
  "actualSleep": 36002,
  "sleepError": 2
}
```

This should be emitted only when the wake reason indicates hibernate wake so fleet validation remains accurate and low-noise.

## Recovery and Rollback

Rollback procedure:

1. Disable `enableHibernateSleep` in the ledger.
2. Deploy the configuration update.
3. Confirm the device returns to the legacy sleep path.

Field recovery procedure:

1. Use the secondary user button wake source.
2. Inspect `wakeReason`, `resetReason`, and retained hibernate diagnostics.
3. Recheck RTC validity and the AB1805 alarm schedule.

## Validated Results

Hardware validation completed successfully with the following results:

1 hour test:

- requestedSleep=3600
- actualSleep=3602
- wakeReason=AB1805 WakeReason::ALARM
- resetReason=RESET_REASON_POWER_MANAGEMENT

10 hour test:

- requestedSleep=36000
- actualSleep=36002
- wakeReason=AB1805 WakeReason::ALARM
- resetReason=RESET_REASON_POWER_MANAGEMENT

Observed notes:

- D8 must be configured `INPUT_PULLUP`
- FOUT/nIRQ is active LOW
- wake occurs on D8 FALLING
- countdown timer support in `AB1805_RK` is limited by an 8-bit timer value and is not suitable for long sleeps
- RTC alarm scheduling is the approved production mechanism

## Risk Assessment

Primary risks:

- RTC drift or invalid RTC state causing incorrect wake scheduling
- accidental overlong sleep if the request is not clamped
- recovery lockout if only a single wake source is used
- platform drift if Boron-only behavior leaks into Photon 2 or LoRa builds

Mitigations:

- explicit Boron-only eligibility check
- Device OS minimum version gate
- RTC validity gate
- maximum sleep clamp
- minimum sleep threshold
- user button secondary wake source
- ledger kill switch

## Test Plan

1. Verify Boron on Device OS 6.4.1+ selects hibernate only when `enableHibernateSleep` is true.
2. Verify Boron on older Device OS versions falls back to the existing sleep path.
3. Verify invalid RTC time falls back to the existing sleep path.
4. Verify sleep requests under 900 seconds use the existing ULP sleep path.
5. Verify requests above 21600 seconds log a warning and fall back.
6. Verify the wake path uses D8 FALLING and the user button secondary wake source.
7. Verify retained diagnostics match requested and actual sleep after wake.
8. Verify cloud status publishes only for hibernate wake events.
9. Verify Photon 2 and LoRa nodes keep their current behavior unchanged.
# Hibernate Sleep Design (Boron, Device OS 6.4.1+)

## Objective
Integrate AB1805 RTC alarm driven hibernate sleep for Boron devices with conservative guardrails, diagnostics, and rollback controls.

This design keeps the existing sleep implementation available as fallback and does not remove existing sleep modes.

## Validated Hardware Wake Path
AB1805 RTC Alarm -> FOUT/nIRQ (active low) -> carrier wake line -> Boron D8 -> `SystemSleepMode::HIBERNATE` wake on FALLING.

Validated observations:
- D8 requires `INPUT_PULLUP`.
- FOUT/nIRQ is active LOW.
- Wake trigger is D8 FALLING.
- Countdown timer is limited to 8-bit value and is not suitable for long sleeps.
- RTC alarm scheduling is the approved production mechanism.

## Guardrails
The hibernate path is gated by `shouldUseHibernateSleep(uint32_t requestedSleepSeconds)` and returns true only when all checks pass:
- Boron platform only.
- Device OS >= 6.4.1.
- Ledger kill switch `enableHibernateSleep == true`.
- AB1805 RTC available/readable.
- RTC valid (`rtc > 2024-01-01`, `rtc < 2035-01-01`, `rtc != 0`).
- Requested sleep >= `MIN_HIBERNATE_SLEEP_SEC` (900 sec).
- Requested sleep <= `MAX_HIBERNATE_SLEEP_SEC` (21600 sec, 6 hours initial soak cap).

Fallback behavior:
- If any eligibility check fails, existing sleep logic remains available.
- For short (< 900 s) or oversize (> 21600 s) requests when kill switch is enabled, hibernate is bypassed and ULP fallback is used.

## Alarm Programming
Production path uses RTC alarm scheduling only:
- `wakeTime = rtcNow + sleepSeconds`
- `ab1805.interruptAtTime(wakeTime)`

Not used in this implementation:
- `deepPowerDown()`
- `interruptCountdownTimer()`
- AB1805 countdown sleep for long-duration scheduling

## Hibernate Sleep Configuration
Before sleep:
- `pinMode(WAKEUP_PIN, INPUT_PULLUP)` (Boron wake pin path is D8/WKP on this platform mapping)

Sleep config:
- `SystemSleepMode::HIBERNATE`
- `.gpio(WAKEUP_PIN, FALLING)` for RTC alarm wake
- `.gpio(BUTTON_PIN, FALLING)` as a field recovery source

Recovery wake source purpose:
- Field recovery
- Technician wake-up
- RTC programming mistake recovery
- Unexpected alarm behavior recovery

## Retained Diagnostics
Retained fields capture hibernate scheduling and outcome:
- `retainedRtcBefore`
- `retainedWakeTime`
- `retainedRequestedSleep`
- `retainedHibernateCount`

Before hibernate sleep:
- Store RTC time before sleep
- Store target wake time
- Store requested sleep seconds
- Increment hibernate count

After wake (boot path):
- Compute `actualSleep = rtcAfter - retainedRtcBefore`
- Compute `sleepError = actualSleep - retainedRequestedSleep`
- Log `requestedSleep`, `actualSleep`, `sleepError`, `wakeReason`, `resetReason`

## Cloud Validation Status
On startup status publish, optional fields are appended only when wake indicates hibernate alarm wake:
- `"sleepMode":"hibernate"`
- `"wakeReason":"ALARM"`
- `"actualSleep":<seconds>`
- `"sleepError":<seconds>`

Purpose: fleet-wide validation during soak.

## Ledger Kill Switch
New modes setting:
- `enableHibernateSleep` (bool)

Default:
- `false`

Behavior:
- Disabled: use existing sleep logic.
- Enabled: evaluate hibernate eligibility guardrails.

Rollback:
- Set `modes.enableHibernateSleep=false` in ledger to immediately disable the new path.

## Measured Validation Results
Confirmed results from hardware validation:

1 hour test:
- `requestedSleep=3600`
- `actualSleep=3602`
- `sleepError=+2`
- wake reason: `ALARM`
- reset reason: `RESET_REASON_POWER_MANAGEMENT`

10 hour test:
- `requestedSleep=36000`
- `actualSleep=36002`
- `sleepError=+2`
- wake reason: `ALARM`
- reset reason: `RESET_REASON_POWER_MANAGEMENT`

## Field Recovery Procedure
1. If device appears stuck asleep, press user button (configured as secondary wake source).
2. Confirm startup status event and wake diagnostics in logs.
3. If needed, disable `enableHibernateSleep` from ledger and force reconnect.
4. Verify device returns to existing ULP/night fallback behavior.

## Soak Test Plan
Phase 1 (initial):
- Keep max hibernate cap at 6 hours (21600 sec).
- Enable `enableHibernateSleep=true` on a small Boron cohort.
- Monitor startup status events for `sleepMode=hibernate`, `actualSleep`, `sleepError`.
- Track alert 16, wake failures, and unexpected resets.

Phase 2 (expansion):
- Expand cohort after stable Phase 1 metrics.
- Continue trend checks for wake accuracy and error envelope.

Phase 3 (longer duration):
- Raise max cap to 36000 sec only after field validation supports it.

## Risks and Mitigations
- RTC invalid or unavailable: blocked by eligibility checks and fallback.
- Device OS regression: gated to >= 6.4.1.
- Misconfigured long sleeps: max cap guardrail prevents multi-day sleeps.
- Short sleep inefficiency: min threshold keeps short sleeps on ULP.
- Bad RTC alarm programming: user button recovery wake and kill switch rollback.
- Fleet rollback needs: ledger kill switch allows immediate disable without firmware rollback.
