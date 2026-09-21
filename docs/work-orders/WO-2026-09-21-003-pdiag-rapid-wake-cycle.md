# WO-2026-09-21-003: `pdiag` publish frequency reflects real, chronic rapid wake/sleep cycling - not a logging defect

**Type:** Backlog investigation, filed with an urgency flag.

**Status:** OPEN. Filed 2026-09-21 from an investigation-only dispatch (no
code touched) originally scoped to classify `PowerDiag`/`LedgerPayloadStatus`/
`pdiag` as logging noise, alongside `WO-2026-09-15-001`. Deliberately filed
as its own WO rather than an amendment to that one - see "Why this isn't a
`WO-2026-09-15-001` amendment," below.

## The observation

`pdiag` (`PowerDiagnostics::flushDiagBatch()`, `src/power/PowerDiagnostics.cpp`,
called from exactly 4 sites, all in `src/state/State_Sleep.cpp`) publishes
one batched cloud event per real sleep-entry attempt. Pulled real cloud-event
history via the fleet `telemetry` CLI and the S3-archived raw payloads
(`particle-events/.../pdiag/...json`) for two devices:

- **Boron-Dev-14**, last 6 hours (includes today's Step 4 battery-authority
  bench sweep): 73 `pdiag` events vs. 41 actual sensor reports
  (`Ubidots-Sensor-Hook-v1`) in the same window - `pdiag` publishing ~1.8x
  more often than real reports.
- **Boron-Dev-09** (control - zero involvement in today's bench work), last
  ~7 hours: 94 `pdiag` events, median gap between consecutive events **1.1
  seconds**, 75 of 93 gaps under 5 seconds.

Fetched the actual decoded payloads (not just timestamps) for several tight
clusters on both devices. Every single event in every cluster carries a
*distinct* `r=9(post-wake)/2(post-refresh, xN)/4(connect-success)/6(pre-sleep-ulp)`
sequence with slightly different soc/vbus/powerSource each time - never
byte-identical repeats of the same batch.

**Since `flushDiagBatch()` only fires once per genuine sleep-entry attempt,
a cluster of distinct `pdiag` publishes seconds apart means the device
completed that many full wake -> report/connect -> sleep-attempt cycles in
that same short window.** Dev-09's data implies the device is cycling
through complete wake/connect/sleep sequences roughly **every 1-3 seconds**,
chronically, unrelated to any bench activity - it had none today.

## Why this isn't a `WO-2026-09-15-001` amendment

That WO's whole frame is "a diagnostic line re-logged on every check rather
than on change" - a change-detection guard fixes it. That frame does not
apply here: `pdiag`'s entries are not stale repeats of unchanged state: they
are fresh, correct reports of a real event happening far more often than it
should. A value-comparison guard would not reduce this volume at all, since
the values genuinely differ each time. Folding this into that WO would have
mislabeled a functional problem as a cosmetic one.

## Why this deserves a higher-priority flag, not just "another WO"

If real, a device completing a full wake/report/connect/sleep cycle every
1-3 seconds instead of actually sleeping has direct battery-life
implications - this is not merely elevated telemetry volume/cost, it is
potentially the proximate cause of battery drain that has otherwise been
investigated as a sensor/gauge problem elsewhere in this project's history.
It is also plausibly connected to (or the same class of issue as) the
sleep/time ownership boundary work already flagged as this project's
standing next priority ahead of further WO work.

## Not yet done - explicitly out of scope for the dispatch that found this

- Root-causing *why* the device re-enters a sleep attempt every 1-3 seconds
  (a state-machine/sleep-gate investigation, not a diagnostics-logging one -
  likely `state/State_Idle.cpp`/`state/State_Sleep.cpp`'s sleep-readiness
  loop, given `flushDiagBatch()`'s 4 call sites are all on mutually
  exclusive branches of that same pre-sleep sequence, ruling out "one cycle
  hits multiple flush sites" as the explanation).
- Confirming whether this is new, or has been present since before this
  session's roadmap work began (no historical baseline was pulled for this
  WO; only today's and yesterday's windows were checked).
- Any code change of any kind.

## Recommended next step

Scope a dedicated investigation (not a fix dispatch) into the sleep-entry
retry/thrash behavior itself, treating this as a real functional finding,
not a logging-noise item. Once that investigation identifies why the device
isn't settling into sleep, `pdiag`'s elevated publish rate should resolve
as a side effect - a rate-limit/coalescing change to `flushDiagBatch()`
itself would only mask the symptom.

## Related work orders

- `WO-2026-09-15-001-excessive-repeat-logging-powerdiag-ledger.md` - the
  sibling logging-noise investigation this WO was split out of (Amendment
  B); `PowerDiag` and `LedgerPayloadStatus` are confirmed there as
  small, single-site, genuinely cosmetic/backoff fixes - the opposite
  conclusion from this WO.
- `WO-2026-09-03-004-mafc1-sleep-path-watchdog-stalls.md` - a related,
  previously-filed sleep-path finding (a different device, a hang rather
  than a rapid cycle, but the same general area of the state machine).
