# WO-2026-09-15-002: `CELLULAR_ACQUIRE` registration stall has no escalation path

**Status:** Drafted, not dispatched. Documentation only - **no fix authorized,
nothing dispatched to Copilot/Codex.**

**Origin:** Observed live on Dev-09 during Step 1 (`WO-2026-09-14-002`) bench
validation, overnight 2026-09-14/15. Confirmed to block bench validation for
that night - a real, not theoretical, cost.

## The observation

Dev-09's serial log for the overnight window (`serial-forwarder-pi-01`,
2026-09-14 22:00-23:27 UTC):

```
22:00:06  port opens, "Using internal SIM card"
22:10:16  WARN: Resetting the modem due to the network registration timeout
22:10:25  INFO: Using internal SIM card
22:11:06  WARN: ConnSummary: fail elapsed=660002 last=CELLULAR_ACQUIRE
                 cellMs=660001 netMs=0 cloudMs=0 cloudRecoverStage=0
                 cloudRecoverCount=0 sig=0/0 heap=79480 q=8
22:11:07  WARN: Connect: fail elapsed=660002ms budget=660000ms heap=79480
22:11:08  SerialException (port drops)
22:11:11  port reappears
          [48-minute gap - device presumably in SLEEPING_STATE after the
           connect-timeout transition]
22:59:58  port reappears, retries
23:10:08  WARN: Resetting the modem due to the network registration timeout
23:10:57  WARN: ConnSummary: fail elapsed=660001 ... sig=0/0 heap=83688 q=10
23:10:59  SerialException (port drops)
23:11:02  port reappears
          [15-minute gap]
23:26:51  retrying again (window ends here, still unresolved)
```

Two full connect attempts observed, each burning its entire 660-second
(11-minute) budget stuck at `CELLULAR_ACQUIRE` with `sig=0/0` -
[[sig-0-0-cellular-registration-indicator]]: the modem never registers with a
serving cell at all (not a weak-signal case). `cloudRecoverStage=0
cloudRecoverCount=0` on both attempts confirms the device never reached
`CLOUD_ACQUIRE` - the staged cloud-recovery machinery never had a chance to
run, because it isn't gated to fire at this earlier phase. No
`MODEM_HEALTH: unstable` line was captured in this window - see "What we
could not confirm" below.

This blocked Step 1's bench validation on Dev-09 for the full night: no
`hibernate_wake` event, no completed report cycle, nothing to compare against
the pre-flash baseline. See `WO-2026-09-14-002`'s bench-validation section.

## The defect

**A true `CELLULAR_ACQUIRE` registration stall at `sig=0/0` has no escalation
path within its own attempt.** Confirmed directly against
`src/state/State_Connect.cpp`:

- The staged cloud-recovery machinery (`requestCloudAcquireStage1Recovery()`
  at `CLOUD_RECOVER_STAGE1_MS` = 60s - a soft `Particle` reconnect, line 79;
  `requestCloudAcquireStage2Recovery()` at `CLOUD_RECOVER_STAGE2_MS` = 120s -
  `Connectivity::requestRadioPowerOff()` + `Cellular.on()` + reconnect, line
  84) is gated to `currentConnPhase == ConnAcquirePhase::CLOUD_ACQUIRE` only
  (line 391). `CLOUD_ACQUIRE` means cellular is already registered and only
  the cloud handshake remains - the phase after the one Dev-09 is stuck in.
- `CELLULAR_ACQUIRE` (`!Cellular.ready()` - not registered) gets no
  equivalent within-attempt action. The phase-elapsed accounting
  (`connPhaseCellMs`, lines ~334-345) only tracks time; nothing runs until
  the full attempt budget (660s) expires.
- At that point, `markModemUnstableFromConnectTimeout()` (line 138) runs:
  increments `session.modemConnectTimeoutCount`, and only sets
  `session.modemUnstable = true` (reason `connect_timeout` = 2) once that
  count reaches `MODEM_UNSTABLE_CONNECT_TIMEOUT_THRESHOLD` = 2 (line 23) -
  i.e. not until the *second* full-budget failure. Only then does
  `requestFullDisconnectAndRadioOff()` (called unconditionally at timeout,
  line 715) plus the next attempt's fresh `Cellular.on()` give the modem a
  cold restart.

Net effect, confirmed against Dev-09's data above: a hopeless registration
search burns the entire 11-minute budget once, with no escalation attempted
during it, and even the "unstable" bookkeeping doesn't engage until a second
consecutive failure. Each attempt already cold-starts the modem at its own
start, but there is no cold-cycle attempted *within* a stuck attempt - this
is the gap.

This finding was previously established read-only on 2026-08-15 while
investigating the Singapore poor-connectivity units
([[cellular-acquire-radio-recovery-gap]]) and is not new; this WO's
contribution is confirming it against a specific, dated bench-validation
cost rather than the general observation. Remediation options captured then
remain the candidates (see that memory): extend the staged machine to
`CELLULAR_ACQUIRE` with a `sig=0/0`-persistence gate, or short-circuit a
hopeless attempt early so the next cold attempt arrives sooner. Neither is
authorized here.

### What we could not confirm

Whether `MODEM_HEALTH: unstable reason=connect_timeout` actually logged on
the second attempt (23:10-23:11 UTC, which should have crossed the
threshold=2). The `SerialException` at 23:10:59 - the port dropping at
essentially the same moment - means it may simply not have been captured,
the same structural serial-capture gap noted in `WO-2026-08-29-001` and
discussed for `WO-2026-09-14-002`'s bench validation. Not resolved here;
would need either a cloud-published confirmation of `modemUnstable` state or
a cleaner capture on a future occurrence.

## Relationship to other filed findings - checked, not assumed

**Not the same mechanism as `WO-2026-09-03-004` (MAFC-1 sleep-path
watchdog stalls),** despite both eventually touching `session.modemUnstable`
and logging under the `MODEM_HEALTH:` prefix. Confirmed by reading both the
WO and the code, not assumed from topic similarity:

- `WO-2026-09-03-004` itself already draws this line: *"Contrast the
  Singapore bench units, where poor registration is the deliberate test
  condition and watchdog recovery from `CELLULAR_ACQUIRE` stalls is the
  design working, not a fault. Do not conflate the two populations."*
- The two failure modes set **different** `modemUnstableReason` values from
  **different files**: MAFC-1's stalls set reason `1` (`slow_teardown`,
  `src/state/State_Sleep.cpp:97-98`) - the modem failing to power down
  *before* sleep, at the tail of a connected session. Dev-09's stalls set
  reason `2` (`connect_timeout`, `src/state/State_Connect.cpp:145-146`) -
  the modem failing to register *before* a connection ever starts. Acquire
  side versus teardown side; opposite ends of a connect/sleep cycle.
  (`src/state/StateMachine.h:64` documents both values on one field.)
- MAFC-1 is Morrisville, healthy siting, `pmicAnomalyCount = 0`; Dev-09 is a
  Singapore bench unit where poor registration is the deliberately chosen
  test condition. Different device populations, per `WO-2026-09-03-004`'s
  own framing.

Conclusion: related only in that both are facets of the same
`session.modemUnstable` bookkeeping and the same broader "modem recovery
coverage is uneven across phases" theme - not the same defect, and this WO
should not be merged into `WO-2026-09-03-004` or read as new evidence for it.

## Severity

At minimum: blocks the device from ever reaching a report or hibernate cycle
while the condition persists - direct operational impact (no data from the
device for that period) - and now a **confirmed** direct cost to bench
validation work (`WO-2026-09-14-002`), not merely a theoretical fleet risk.

## Open questions

- What recovery action, if any, is appropriate for a `CELLULAR_ACQUIRE`
  stall, and why wasn't it already covered by the existing `CLOUD_ACQUIRE`
  path when the two phases sit right next to each other in the same
  state machine?
- Should `sig=0/0` (no registration at all) trigger a different, faster
  escalation than a weak-but-present-signal case? The two are not the same
  failure and may warrant different budgets/thresholds.
- Is this bench-environment-specific (e.g., a SIM or antenna condition local
  to Dev-09 or the Singapore bench siting) or a genuine field risk that could
  recur on any device with marginal coverage? This distinction changes
  urgency substantially and is unresolved - `[[poor-connectivity-test-devices]]`
  notes Dev-09/Dev-14 are deliberately sited for poor cellular as a test
  condition, which argues for "expected of this siting" over "field-wide
  risk," but that has not been verified against any non-bench device
  experiencing the same signature.
- Does the serial-capture gap noted above (the port dropping at the same
  moment the second-attempt `MODEM_HEALTH` line would print) recur reliably,
  and does it matter given the cloud-side `hibernate_wake`/status events
  exist independently of serial?

## Acceptance criteria (once scoped)

- Confirm (from cloud event data, not serial, given the capture gap above)
  whether `modemUnstable`/`connect_timeout` escalation actually engaged on
  Dev-09's second attempt.
- Decide, with evidence, whether `CELLULAR_ACQUIRE` needs its own staged
  recovery (mirroring `CLOUD_ACQUIRE`'s stage1/stage2 shape) or a different
  design (e.g., early bail on persistent `sig=0/0`).
- If a fix is later authorized: bench-verify the new behavior specifically
  reduces wasted radio-on time during a genuine no-signal window, without
  interrupting a legitimate first-attach that is still in progress (per
  `[[cellular-acquire-radio-recovery-gap]]`'s caution that first attach can
  legitimately take 60-120s).
- Explicitly re-confirm no conflation with `WO-2026-09-03-004` in any future
  work on this WO, per the distinction established above.

## Not authorized in this WO

Any fix. Any change to the connect-retry, recovery, or escalation logic in
`State_Connect.cpp` or `State_Sleep.cpp`. Any dispatch to Copilot or Codex.

## Provenance

Gap originally established 2026-08-15, read-only, while investigating the
Singapore poor-connectivity units (session memory
`cellular-acquire-radio-recovery-gap`, `sig-0-0-cellular-registration-indicator`).
Confirmed against a specific, dated cost during `WO-2026-09-14-002` Step 1
bench validation, overnight 2026-09-14/15 UTC, via Dev-09's serial log
(`serial-forwarder-pi-01`) and cross-checked against
`src/state/State_Connect.cpp` (current tree, this session) rather than
re-derived from scratch.

## Related

- `WO-2026-09-14-002` - Step 1 (wake-gate single owner). This WO's bench
  validation was the incident that surfaced this cost; the two are otherwise
  unrelated (Step 1 touches only post-wake gate classification, not
  connectivity).
- `WO-2026-09-03-004` - MAFC-1 sleep-path watchdog stalls. Explicitly **not**
  the same mechanism; see "Relationship to other filed findings" above.
- `WO-2026-08-31-004` - Dev-11 AB1805 rate fault. Unrelated mechanism, same
  device population (Singapore bench units).
