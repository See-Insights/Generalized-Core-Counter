# WO-2026-08-29-001: Publish hibernate-wake diagnostics to cloud telemetry when the qualification gate fails

## Context

Two separate open investigations have now stalled on the same missing
signal, so this is being raised on its own rather than folded into either.

Dev-11 slept on its correct close boundary on 2026-08-28 (`22:00:29` SGT,
matching `closeHour=22` in its device-settings ledger) and did not report
again until `2026-08-29 11:09:12` SGT — a 13.15 h quiet window against an
intended 8 h. Two competing explanations fit the wake-timing evidence
equally well:

1. the sleep duration was computed wrong, or
2. the duration was correct and the device woke on time but could not
   reach the cloud for five hours (Dev-11 is the known poor-cellular unit).

The firmware already computes exactly the value that separates these —
`startupHibernateActualSleepSec` and `startupHibernateSleepErrorSec`
against `retainedHibernateRequestedSleep` — and logs it at
`Generalized-Core-Counter.cpp:1179` as
`HibernateWake: reason=%s req=%lu actual=%lu err=%ld count=%lu`.
That line was not captured, and structurally could not have been.

The same gap blocked the separate question of whether Dev-11's +12 h
sleep-schedule regime (Jul 15-19, reproduced on `20.0-dev`) recurs on
later builds: correlating relapse days against the device's own
requested-vs-actual sleep is not possible from the data that reaches the
cloud.

## Why the line is unreachable on the deployed build

`setup()` (`:709`) has two serial settles, and neither applies here:

- The always-on settle at `:715` is guarded by `if (Serial.isConnected())`.
  On a hibernate wake USB CDC has not enumerated yet at that point, so it
  returns false and no delay occurs.
- The blocking `waitFor(Serial.isConnected, 10000)` at `:812` is inside
  `#if ALLOW_BLOCKING_SERIAL_WAITS` (`:811`), which defaults to `0`
  (`BuildProfile.h:78`) on every non-DEV build. The soak devices run
  field builds, so this is compiled out.

The hibernate-wake classification block at `:1165-1193` therefore runs
before the Pi serial forwarder can attach. Measured on Dev-11's
2026-08-29 wake: the forwarder's first captured line is at device
uptime `4750 ms`, and the classification block runs well before that.
This is not a forwarder defect and is not fixable by changing forwarder
timing — the log simply predates USB enumeration.

## The actual defect: the failure branch publishes nothing

The success path is already cloud-visible. When the gate at `:1168-1174`
passes, `startupHibernateStatusReady` is set and `:2071` appends
`sleepMode/wakeReason/actualSleep/sleepError/hibernateCount` to the
status payload. Dev-14's 2026-08-28 wake carries `hibernateCount=1` and
its siblings; this works.

When the gate **fails**, `hibernateFields` stays `""` and the status
payload carries nothing at all — and the `else` at `:1186` sends the only
diagnostic (`pending=1 reason=%d wake=%s rtcOk=%d`) to serial, which on a
field build nothing can read.

So the observability is inverted: the uninteresting case is reported and
the interesting case is silent. Dev-11's 2026-08-29 status event
published no hibernate fields, which tells us the gate failed but not
which of its five conditions failed — and `reason` was confirmed `30`
(`RESET_REASON_POWER_MANAGEMENT`) from the same payload, so at least one
of `retainedHibernatePending`, `wakeReason == ALARM`, `rtcReadOk`, or
`rtcTime >= retainedHibernateRtcBefore` is the answer. We cannot tell
which, and that distinction is exactly what separates "alarm never fired"
from "woke fine, connected late".

## Fix

Publish the failure-branch diagnostics in the status payload, mirroring
the pattern the file already uses for PIN_RESET.

There is a direct precedent at `:2090-2091`: `pinResetAb1805Fields` is
emitted under its own `startupPinResetAb1805Checked` flag specifically so
"a PIN_RESET boot can be told apart in cloud telemetry, without requiring
serial-log tracing" (existing comment at `:2082-2089`). This WO asks for
the same treatment for the hibernate-wake failure branch, for the same
stated reason.

Concretely:

1. Set a flag in the `else` at `:1186` (parallel to
   `startupHibernateStatusReady`) capturing that a hibernate wake was
   pending but did not qualify.
2. Emit a small field group when that flag is set — enough to identify
   *which* gate arm failed: at minimum the resolved `wakeReason` name,
   the `rtcReadOk` result, and `retainedHibernateRequestedSleep`.
   `retainedHibernateRtcBefore`/`rtcTime` are the pair that decides the
   ordering condition; publish whatever subset the payload budget allows,
   preferring the ordering pair if a choice is forced.
3. Keep the existing success path untouched.

Payload budget must be checked before implementation, not assumed. The
status payload is already near its limit on some devices
(`LedgerPayloadStatus: bytes=612/896 schema=2` observed on Dev-11), and
`hibernateFields` is a fixed `192`-byte buffer (`:2070`). If the failure
group cannot fit alongside the existing fields, it is acceptable for the
failure group to *replace* the success group — they are mutually
exclusive by construction.

## Explicitly out of scope

- **Any change to the sleep decision, the hibernate duration
  computation, or the AB1805 alarm handling.** This WO adds reporting
  only. Whether Dev-11's oversleep is a wrong computation or a late
  connect is precisely the question this reporting is meant to answer;
  changing the mechanism before that answer exists would destroy the
  evidence.
- **Changing `ALLOW_BLOCKING_SERIAL_WAITS` on field builds.** Re-enabling
  a blocking 10 s serial wait on deployed devices to make one log line
  visible is the wrong trade and carries watchdog interactions already
  characterized in WO-2026-08-12-001.
- **Forwarder-side changes.** The log predates USB enumeration; no
  collector change can capture it.

## Validation

Because the failure branch only fires on a non-qualifying hibernate wake,
this cannot be validated by waiting for one to occur naturally. It needs a
forced case on the bench — e.g. a DEV build with
`ALLOW_BLOCKING_SERIAL_WAITS=1` so both the serial line and the new
cloud fields are visible simultaneously, confirming they agree, then a
field build confirming the cloud fields alone still appear.

## Provenance

Raised 2026-08-29 from the Dev-11 non-responsiveness investigation.
Evidence: S3 `particle-events/2026-08-29/status/e00fce683f6063bf254283dd/`
(no hibernate fields, `resetReason=30`), the corresponding Dev-14 object
under `2026-08-28/status/e00fce688e592afaf23ac4fb/` (hibernate fields
present), and the Pi forwarder's first-captured-line uptime of `4750 ms`
from the `serial` timeline for the same wake.
