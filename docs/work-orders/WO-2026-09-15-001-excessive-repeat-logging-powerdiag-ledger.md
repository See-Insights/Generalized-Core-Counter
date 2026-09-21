# WO-2026-09-15-001: Excessive repeat logging in PowerDiag and LedgerPayloadStatus

**Status:** Root cause confirmed for both PowerDiag and LedgerPayloadStatus
(2026-09-21, Amendment B). **Still no fix authorized** - this WO remains
scoped to diagnosis only; a follow-on WO will authorize implementation.

**Origin:** Observed on both Dev-14 and Dev-09 during Step 1
(`WO-2026-09-14-002`) bench validation flashes, 2026-09-14. Confirmed present
on both devices independently, so not device-specific. Not introduced by
Step 1 - Step 1 touched only the wake-gate call site
(`Generalized-Core-Counter.cpp:1285-1290`) and `HibernateWakeDiagnostics.h`;
neither `PowerDiag` nor the ledger publish path was part of that change.
Both devices are confirmed on the identical build (app-hash match), so this
is pre-existing behaviour newly noticed under closer post-flash observation,
not a regression.

## The observation

Two independent instances of the same shape - a diagnostic line re-logged on
every check rather than on change:

- **`PowerDiag[N]`** - Dev-14's post-wake sequence showed 13 `PowerDiag[N]`
  lines in a short window, most reporting byte-identical values (`vbus=1
  pg=1 soc=80.3%` etc.) across nine-plus consecutive lines.
- **`LedgerPayloadStatus`** - both devices showed roughly 20-30 near-identical
  `bytes=669/896 schema=2` lines in under a second during the post-connect
  ledger sync window (Dev-09's example: 30 lines between 23729 and 24790 ms).

Neither appears to indicate a functional problem - both devices completed
connect, ledger sync, and (per the surrounding log) proceeded normally. This
is a log-volume/signal-to-noise issue, not a correctness one, as currently
understood.

## Severity

Low. No known functional impact. Worth fixing because: (a) it obscures
genuinely interesting log lines in exactly the kind of dense, fast-moving
window where something else worth noticing might also be happening, and (b)
at fleet scale this is unnecessary data volume repeated across every device,
every connect cycle.

## Open questions for the diagnostic phase

- Is each line logged on a fixed timer/poll regardless of value change, or
  triggered by some other repeated event (e.g., once per ledger callback,
  once per loop iteration during a specific state)?
- Does this predate this session's work entirely, or did it appear/worsen at
  some identifiable point? (Not yet checked against pre-Step-1 baseline
  logs.)
- Are there other diagnostic call sites in the codebase with the same
  log-every-check-not-every-change pattern, or are `PowerDiag` and
  `LedgerPayloadStatus` the only two?

## Acceptance criteria (once scoped)

- Identify the call site(s) responsible for each.
- Determine the intended log-on-change condition (what value(s) should gate
  a new line).
- Add a change-detection guard so repeated identical states produce at most
  one line (or a periodic heartbeat at a much lower rate), not one line per
  check.
- Confirm no diagnostic value is lost - if something genuinely changes
  between two otherwise-identical-looking lines (e.g., a field not shown in
  the current log format), that should still surface.
- Bench-verify on at least one device across a normal connect/sleep cycle
  that log volume drops without losing any state transition.

## Not authorized in this WO

Any code change. This is filed for backlog prioritization alongside the
roadmap's Steps 2-7; not on the critical path of `WO-2026-09-14-002` (Step 1)
or the ownership work, and shouldn't block either.

## Amendment A (2026-09-20): stronger evidence from Step 3b's bench run

`WO-2026-09-19-001` (Step 3b)'s Dev-14 acceptance run produced a
considerably more severe instance of the same `LedgerPayloadStatus` pattern
than the original observation: roughly **300** identical
`LedgerPayloadStatus: bytes=670/896 schema=2` lines in about 9 seconds
during a single ledger sync - an order of magnitude more repeats, in a
comparable window, than the original 20-30-line observation. Confirms this
is not a one-off or device-specific volume; the log-every-check-not-every-
change shape holds under a second, independent bench run. Still not
attributed to a root cause and still no fix authorized - recorded here
because it materially strengthens the severity case for whoever picks this
WO up next.

## Amendment B (2026-09-21): root cause confirmed for both, via real telemetry

Investigation-only dispatch, no code changed. Pulled real evidence via the
fleet `telemetry` CLI (timeline + S3-archived event payloads) and the
cloud-forwarded serial reconstruction, rather than reasoning from source
alone.

### PowerDiag - confirmed logging-only, cosmetic, no cloud cost

Traced to `PowerDiagnostics::logPowerState()` (`src/power/PowerDiagnostics.cpp:218`),
called from 7 sites (`Generalized-Core-Counter.cpp`, `PowerPlatform.cpp`,
`PowerManager.cpp`, `State_Connect.cpp`, `State_Sleep.cpp` x2). Confirmed via
a real serial trace of one entirely normal Dev-14 wake-report-connect-idle
cycle (2026-09-21, 12:50:15-12:50:37 SGT): 5 near-duplicate `PowerDiag[N]:`
lines fired from separate call sites in 22 seconds, values essentially
unchanged. This is purely a serial-readability issue - `logPowerState()`
only logs and appends to the in-RAM diag batch; it does not itself publish.

Genuinely good news for the eventual fix: `logPowerState()` already takes a
`bool forceLog` parameter, wired through all 7 call sites (some pass `true`,
most the default `false`) - but inside the function it is dead:
`(void)forceLog; // Suppression disabled for diagnostic purposes`. The
suppression mechanism was apparently built and then switched off, not never
built. **Fix size: small, single-site** - all 7 callers already funnel
through one function, and the plumbing to gate on it already exists end to
end.

### LedgerPayloadStatus - confirmed as a real publish-volume problem, root cause fully traced

`Cloud::loop()` (`src/cloud/Cloud.cpp:673-696`) has zero backoff: every main
loop pass, if `pendingStatusPublish` is true and connected, it calls
`Cloud::writeDeviceStatusToCloud()` again. That function only clears the
flag when it returns `true`; it returns `false` (retry next pass, no delay)
whenever `noteLedgerSyncRequest()` reports the status ledger sync is
*already in-flight*. Every repeated `LedgerPayloadStatus:` line therefore
corresponds to a genuine repeated `deviceStatusLedger.set()` attempt, not
just a log call - a real publish-class cost, not merely cosmetic.

Located and pulled the actual 2026-09-19 Amendment-A-class instance directly
from the fleet archive: Dev-14, 2026-09-20 06:00:39-06:01:01 SGT, 24
`LedgerPayloadStatus:` lines in ~23 seconds, triggered by that boot's first
`ClockResync` (`requestClockResync` -> `Cloud::instance().requestStatusPublish("ClockResync")`).
The retry stopped in the exact same second `LedgerCb: kind=STATUS ...
countAfter=0` confirmed the underlying ledger sync had finally completed -
direct confirmation of the busy-retry-until-ledger-confirms mechanism. The
rate is not fixed - it tracks however fast the main loop happens to be
spinning while stuck retrying, which is exactly why one observed instance
was ~1/sec (this one) and another (the original Amendment A) was ~33/sec.
**Fix size: small, single-site** - the retry lives entirely inside
`Cloud::loop()`; it needs a minimum retry interval or an in-flight check
before re-attempting.

### Not both from one root cause

Confirmed these are two independent problems (Task 1 of the investigating
dispatch), not one call site feeding both - traced to genuinely different
functions with no shared trigger.

### pdiag - investigated separately, NOT folded into this WO

A third diagnostic-noise candidate (`pdiag` cloud events) was investigated
in the same dispatch. It turned out not to be a logging-cadence defect at
all - see `WO-2026-09-21-003-pdiag-rapid-wake-cycle.md` for why this needed
its own WO rather than an Amendment here.
