# Hibernate Sleep Design (Boron)

## Scope

This design adds a conservative AB1805 RTC-alarm hibernate path only for Boron overnight/long-duration sleep.

Constraints:
- Device OS minimum: 6.4.0+
- Boron only
- No changes to AB1805_RK library code
- Photon 2 / P2 behavior unchanged
- Existing ULP/STOP path remains fallback

## Hardware Path

Validated wake route:

AB1805 alarm -> FOUT/nIRQ (active low) -> carrier wake line -> Boron WAKE pin -> `SystemSleepMode::HIBERNATE` wake on `FALLING`

Pre-sleep pin handling:
- `pinMode(WAKEUP_PIN, INPUT_PULLUP)`
- Require wake pin reads `HIGH` before entering hibernate

Secondary recovery wake:
- `BUTTON_PIN` on `FALLING`

## Guardrails

Hibernate is attempted only when all checks pass:
- `modes.enableHibernateSleep` is `true` (kill switch)
- requested sleep in `[900, 21600]` seconds
- Device OS version is 6.4.0+
- AB1805 RTC is set/readable
- RTC time is in a valid range (`2024-01-01 <= rtc < 2035-01-01`)

If any check fails, firmware logs and falls back to existing ULP/STOP behavior.

## Alarm Arming

Before programming the new alarm, stale AB1805 interrupt state is cleared by direct register operations from application code (no library edits):
- clear status register pending bits
- clear alarm/timer interrupt-enable bits in interrupt mask

Then arm a one-shot RTC alarm using:
- `ab1805.interruptAtTime(wakeTime)`

No countdown timer path is used for this long-sleep hibernate mode.

## Retained Diagnostics

Retained fields capture hibernate scheduling and validation:
- `retainedHibernateRtcBefore`
- `retainedHibernateWakeTime`
- `retainedHibernateRequestedSleep`
- `retainedHibernateCount`
- `retainedHibernatePending`

On startup, when wake is confirmed as Boron hibernate alarm wake:
- compute `actualSleep = rtcAfter - retainedHibernateRtcBefore`
- compute `sleepError = actualSleep - retainedHibernateRequestedSleep`

## Startup Status Payload

Hibernate fields are included only for confirmed hibernate alarm wake events:
- `sleepMode: "hibernate"`
- `wakeReason`
- `actualSleep`
- `sleepError`
- `hibernateCount`

This avoids noise for normal wakes.

## Rollback

Immediate rollback is ledger-only:
- set `modes.enableHibernateSleep=false`

Firmware then uses the existing ULP/STOP path without removing code.
