# WO-2026-09-15-001: Excessive repeat logging in PowerDiag and LedgerPayloadStatus

**Status:** Drafted, not dispatched. Specification only - **diagnostic first,
no fix authorized.**

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
