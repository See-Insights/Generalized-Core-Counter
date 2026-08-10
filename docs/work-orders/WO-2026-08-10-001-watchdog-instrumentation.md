# WO: Watchdog reset instrumentation and fleet-wide visibility

## Context

Investigation (`docs/work-orders/investigation-2026-08-10-watchdog-ncp-timing.md`)
found a real, pre-existing fault: execution stalls in the sleep-entry path
(`breadcrumb 21` / `stage:"sleep"`) long enough that the AB1805 external
watchdog (carrier board, ~120s main-loop-not-transited threshold) resets
the device via the reset pin — which Device OS correctly reports as
`RESET_REASON_PIN_RESET` (20), not `RESET_REASON_WATCHDOG` (60), since the
reset is external to the MCU. Confirmed on Boron-Dev-09 (this branch) and
independently on Boron-Dev-11 and Boron-Dev-14 running plain `main` — this
predates and is unrelated to Phase 1/diagnostics-publish-mode.

**This WO does not fix the underlying stall — that root cause is not yet
pinpointed.** It closes the instrumentation gap so the *next* occurrence
is fully self-diagnosing and fleet-trackable, without requiring another
manual serial-log investigation like this one. A follow-up WO to actually
fix the stall itself should be scoped once this instrumentation produces
a definitive trigger.

## Fix

1. **AB1805 wake-reason capture, gated on `RESET_REASON_PIN_RESET`.**
   Currently only checked inside the `retainedHibernatePending` path, never
   generally on `resetReason` — meaning it never fires on the ULP/STOP path
   actually implicated in this fault. Add the gated check
   (`AB1805::getWakeReason()`, called only when `resetReason==20`), capture
   `updateWakeReason()`'s success/fail result (currently discarded), and
   classify `UNKNOWN` as inconclusive — not as "the AB1805 didn't fire."
   New append-only `lastWatchdogSource` field.
2. **Finer breadcrumb granularity in the sleep-entry code path.** The
   existing breadcrumb system narrowed the fault to a broad `"sleep"`
   stage but not precisely enough to pinpoint the exact stuck line without
   manual source tracing. Add intermediate breadcrumb points through the
   sleep-entry sequence so the next occurrence identifies the specific
   stuck call from cloud data alone.
3. **Add a new watchdog-reset alert code that reaches the Ubidots-bound
   payload.** Resolved below — a new code is required, not an existing one.
4. **Default "brief wait if already connected" serial behavior**, distinct
   from the existing bench-only `ALLOW_BLOCKING_SERIAL_WAITS` flag —
   always-on, ~0 field cost (confirmed: nothing is ever attached to a
   deployed device's serial port), 500ms settle only when a monitor is
   already connected at that point in `setup()`. Confirmed feasible:
   `Serial.isConnected()`'s prerequisite (`Serial.begin()`) is already
   satisfied by the global `SerialLogHandler`'s constructor, which runs
   before `setup()`.

**Explicitly out of scope for this WO:** mirroring the breadcrumb into the
AB1805's own RTC RAM (separate power domain, would add resilience against
a more severe MCU-power-loss failure mode, not the one under
investigation) — deferred, revisit only if evidence emerges it's needed.

## Item 3 resolved: new alert code confirmed required

Verified directly against source, not inferred from logs.

- **Every existing alert code was mapped** (`raiseAlert()` call sites
  across the codebase, cross-referenced against the severity table in
  `MyPersistentData.cpp`'s `getAlertSeverity()`): 14 (out-of-memory), 15
  (modem/disconnect failure), 16 (repeated sleep failures), 17 (boot storm
  during setup), 18 (state-machine thrash, `ThrashGuard.cpp`), 20/21/23
  (PMIC thermal/charge-timeout/battery-fault), 30-32/40-44 (connectivity/
  webhook/ledger/publish-queue issues). **None represents "watchdog or
  AB1805-confirmed external reset."** The `alert:18` value seen in the
  fault's status payload during the original investigation is the
  coincidentally-active `ThrashGuard` alert, not anything set by the
  watchdog reset itself — confirmed nothing currently raises an alert for
  this condition at all.
- **The Ubidots pipeline is real and already fully wired** — no new
  plumbing needed, just a new code value. `publishData()`
  (`Generalized-Core-Counter.cpp:1629`) reads
  `current.get_alertCode()` directly into the webhook JSON's `"alerts"`
  field (both `OCCUPANCY` and `COUNTING` mode payload formats), and that
  payload is the one following "the legacy Ubidots field contract" per
  its own doc comment (confirmed by the `"key1"` field, the literal legacy
  Ubidots dashboard variable name). A new alert code raised via the
  existing `current.raiseAlert(N)` mechanism reaches Ubidots through this
  exact path with zero additional wiring.
- **`raiseAlert()` is a single severity-gated scalar, not a bit set**
  (`MyPersistentData.cpp:807-817`): a new code only takes effect if its
  `getAlertSeverity()` tier is higher than whatever's currently active.
  Recommend tier 3 (critical) — the same tier as codes 14-21, all of which
  represent comparable boot/hardware-level failure conditions.
- **Do not add the new code to `isAutoClearAfterReportAlert()`**
  (`Generalized-Core-Counter.cpp:1607`, list: 15/31/41/43/44). Those are
  transient conditions re-evaluated and re-raised each cycle if still
  present; a watchdog-reset alert is a one-time forensic marker for a
  specific past incident and should stay sticky (persist until superseded
  by a more severe alert, matching codes 14/16/17/18/20/21/23's existing
  behavior) so it reliably survives to at least one report cycle.
- **Suggested code value: 19** — next free number in the tier-3 range,
  sits naturally alongside the other boot/reset-related codes (14-18).
  Not binding, just the natural fit; confirm at implementation time nothing
  else has claimed it since this investigation.

## Decisions needed

None remaining that block scoping — item 3 was the only open question and
is now resolved above.

## Acceptance Criteria

- A `RESET_REASON_PIN_RESET` boot correctly classifies as AB1805-watchdog,
  something-else, or inconclusive (never silently misclassified as "not
  the AB1805" on an `UNKNOWN` result).
- The sleep-entry stall, if it recurs, is identifiable to a specific
  breadcrumb point from cloud data alone, without needing serial-log
  reconstruction.
- A watchdog-reset event is confirmed reaching the Ubidots-bound payload
  (not just fleet-ops telemetry), verified end-to-end, not just published
  and assumed delivered.
- Field builds show zero behavioral change from item 4 when nothing is
  attached to serial (verify via the same before/after compile check used
  throughout this project's tooling work).

## Required Tests

- `RESET_REASON_PIN_RESET` with AB1805 `WakeReason::WATCHDOG` → correctly
  classified and logged.
- `RESET_REASON_PIN_RESET` with AB1805 `WakeReason::UNKNOWN` → classified
  as inconclusive, not "not watchdog."
- `updateWakeReason()` failure path is captured, not silently discarded.
- New breadcrumb points fire in the correct order through a normal
  sleep-entry sequence (regression).
- Watchdog alert code (new, value confirmed at implementation time)
  verified present in a test Ubidots-bound payload, and confirmed absent
  from `isAutoClearAfterReportAlert()`.
- Item 4: confirm no behavioral change to field builds; confirm the
  500ms settle only triggers when already connected, not unconditionally.

## Permitted Files

`src/Generalized-Core-Counter.cpp` (setup() wake-reason/alert logic,
`getAlertSeverity()`/`isAutoClearAfterReportAlert()`, `publishData()`
webhook payload — read-only reference, no change needed there),
`src/state/State_Sleep.cpp` (breadcrumb granularity), `src/MyPersistentData.{h,cpp}`
(new `lastWatchdogSource` field, new alert code's severity-tier entry).
Codex to confirm exact scope during implementation dispatch.

## Status

Drafted from a fully completed investigation (both the original
sleep-entry-stall root-cause tracing and this AB1805/serial-visibility
follow-up). Item 3 (watchdog alert code) verified directly against source
— confirmed a new code is required, no existing one applies, pipeline
already wired end-to-end. **Approved by Chip 2026-08-10.** First
implementation pass (GitHub Copilot CLI) came back **Not verified** at
Stage 7 — two real bugs found and confirmed independently: (1) alert 19
sat at the same severity tier as codes 17/18, so `raiseAlert(19)` was a
silent no-op whenever 17 or 18 was already active — including the exact
signature of the original incident; (2) the AB1805 wake-reason
classification called `updateWakeReason()` a second time after
`AB1805::setup()` already called it once, and because that call
destructively clears the WDT status bit on read, a combined-bit status
(WDT+TIMER or WDT+ALARM) could have its correct WATCHDOG classification
overwritten by the second, now-stale read. Corrective design approved:
bump alert 19 to its own severity tier strictly above 14-21 (so it always
supersedes any of them regardless of what's active), and remove the
redundant second `updateWakeReason()` call entirely — reuse
`AB1805::setup()`'s already-classified result via `getWakeReason()` only,
relying on the library's own WDT-first priority ordering in its single
read.

**Corrective pass implemented, independently verified, and re-reviewed at
Stage 7: Verified with concerns (no blocking defect) 2026-08-10.** Both
bugs confirmed fixed against the real diff (alert 19 now its own tier
strictly above 14-21; zero remaining `updateWakeReason()` calls in the
PIN_RESET path). Strengthened tests confirmed as genuine regressions
(reverting either fix breaks its corresponding test). One non-blocking
concern raised — a stale code comment still describing alert 19 as
"tier-3" after the tier-4 bump — fixed directly (comment-only, recompiled
to confirm zero behavioral/size change: Flash 129302 / RAM 4812,
unchanged). Ready for Chip's final commit gate.
