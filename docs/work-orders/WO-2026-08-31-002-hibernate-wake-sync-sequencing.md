# WO-2026-08-31-002: Guarantee a completed time sync before a device may hibernate again

**Split from** `WO-2026-08-29-002` on 2026-08-31, after that Work Order
failed Codex Stage 7 twice. Both failures left the clock-trust *mechanism*
confirmed correct and concentrated every remaining HIGH finding in wake
sequencing - which `-002`'s own constraints forbade touching. **This Work
Order is explicitly permitted to touch the sleep gate and connection-mode
policy.** That permission is the point of the split.

**Depends on** the narrowed `WO-2026-08-29-002` landing first: this WO
assumes `isClockTrusted()`, `observedTimeSyncedLastMs()` and the RTC
write-back already exist and are correct.

## Problem

A device whose AB1805 is wrong wakes from HIBERNATE, has system time
seeded from that wrong RTC by `ab1805.setup()`, therefore reports
`Time.isValid() == true`, computes a wrong local time, concludes it is
outside open hours, and sleeps again - without ever connecting, so
nothing corrects the clock. The error then compounds across each
subsequent hibernate.

This is observed, not hypothetical. Boron-Dev-11 (product 42131,
v24-Thermal-Inhibit):

- Clock offset 0 before the 2026-08-28 hibernate, `-5.15h` after it,
  `-13.45h` after the next one. Measured three independent ways: device
  `timestamp` vs cloud `published_at` across 13+ isolated hourly reports;
  the firmware's own `TimeDiag` line (`epoch=1788092724` = device
  `2026-08-30 12:25:24Z` while real time was `2026-08-31 01:52:20Z`); and
  uptime back-calculation.
- Real sleep durations of 16.29h and 13.13h against an 8h request.
- `last_handshake_at` stuck at `2026-08-28T08:54:21Z` across two hibernate
  wakes and dozens of reconnects - the device resumes sessions rather than
  handshaking, so the automatic sync-at-handshake path never fires. This
  was reasoned in `-002`'s investigation and is now directly observed.

## Why the previous attempt failed

`-002` round 5 added an OR term to `setup()`:
`System.resetReason() == RESET_REASON_POWER_MANAGEMENT && !isClockTrusted()`.
Stage 7 found three problems, all traceable to the fence:

1. **It regressed existing behaviour.** `isClockTrusted()` is necessarily
   false at that point (the per-boot cache is still zero), so the term
   reduces to `resetReason == POWER_MANAGEMENT` and the forced-connect
   branch is taken on *every* hibernate wake. The opening-hour alert-40
   suppression lives in the mutually exclusive `else`
   (`Generalized-Core-Counter.cpp:1311`) and became unreachable, so
   `State_Report` can raise alert 40 immediately after an expected 8-hour
   closed period.
2. **It did not deliver the guarantee.** `CONNECTING_STATE` exits as soon
   as `Particle.connected()` is true (`State_Connect.cpp:647`), the sync
   request is issued later in the loop, and the sleep gate
   (`State_Sleep.cpp:475`) waits on queues, ledgers, webhooks and updates
   but **not** on a clock sync. A device can connect, fail to complete a
   sync, and hibernate anyway.
3. **It ignored connection mode.** `handleConnectingState()` does not
   consult connection mode before calling `Particle.connect()`, so a
   DISCONNECTED-mode or low-battery device incurs a 5-11 minute cellular
   attempt its configuration prohibits.

The round-5 gate is being reverted as part of narrowing `-002`. Do not
simply reinstate it.

## Required behaviour

1. **The sleep gate must wait on sync COMPLETION, not connection.** A
   device that woke from HIBERNATE with an unsynced clock must not be
   permitted to re-enter hibernate until either a sync has completed
   (`Particle.timeSyncedLast()` has advanced this boot) or a bounded,
   explicit give-up condition has been reached. "It connected" is not
   sufficient - that was finding 2.

2. **Alert-40 opening-hour suppression must be preserved.** The existing
   suppression must remain reachable on a normal hibernate wake. This is
   an explicit acceptance criterion with its own test, not an incidental
   property.

3. **The forced-connect path must respect connection mode.** Add the
   check `handleConnectingState()` currently lacks. A DISCONNECTED-mode
   or battery-conserving device must not be dragged into a long cellular
   attempt by this mechanism. If the correct behaviour for such a device
   is "accept a wrong clock rather than violate its mode", state that
   explicitly and document the consequence; do not decide it silently.

4. **The give-up path must be bounded and must not strand the device.**
   Whatever the retry policy, a device that cannot sync must still make
   progress and must not burn its battery or trip either watchdog (app
   60s, AB1805 124s).

## Acceptance criteria

- A device waking from HIBERNATE with an unsynced clock cannot re-enter
  hibernate before a completed sync or a bounded give-up. Demonstrated by
  test, not by inspection.
- Opening-hour alert-40 suppression still fires on a normal hibernate
  wake. Regression test required.
- The forced connect does not occur, or occurs in a documented and
  mode-appropriate form, for DISCONNECTED-mode and low-battery devices.
- No change to the hibernate duration computation, the wake-validation
  gate (`retainedHibernatePending` / `wakeReason` / `rtcReadOk` /
  `rtcTime >= retainedHibernateRtcBefore`), or the 12-hour local-time
  conversion issue.
- Host tests assert behaviour, not code shape, and every one is
  mutation-tested.
- Boron build (`particle compile boron .`, explicit platform -
  `project.properties` is `platform=p2` and the P2 break is tracked in
  `WO-2026-08-31-001`).
- Mandatory Codex Stage 7 at the AB1805/watchdog tier.
- Bench validation on a device with a deliberately skewed AB1805, through
  a real hibernate cycle - the step `-002` has never satisfied.

## Out of scope

- The clock-trust mechanism itself (narrowed `WO-2026-08-29-002`).
- Hibernate-wake observability (`WO-2026-08-29-001`).
- The 12-hour local-time-conversion defect - a separate failure mode
  (wrong sleep *entry*, correct 8h duration) still unassigned.
- The RTC losing hours during hibernate. This WO repairs the clock after
  a wake; it does not stop the loss. That remains uncharacterised and
  needs its own Work Order.

## Provenance

Codex Stage 7 re-review of `WO-2026-08-29-002`, 2026-08-31, findings 1, 2
and 4. Field evidence from Boron-Dev-11 as cited above; queries recorded
in the `-002` investigation.
