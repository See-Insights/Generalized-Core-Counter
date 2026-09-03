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

---

# Amendment A - Pre-teardown hibernate-plan publish

**Added 2026-09-03**, after the 2026-09-02 overnight run. Boron-Dev-11
overslept a fourth time, this time with a **replaced carrier board and
therefore a new AB1805**, which exonerates the RTC hardware and leaves the
fault uncharacterised. Dev-14 woke on the same night with `err=1` second.

## Two constraints that invalidate the obvious implementation

Both were found while siting this work and must shape any implementation.
Do not start by trying to publish from the hibernate block.

1. **The radio is already off where the hibernate data exists.** The
   pre-sleep gate at `State_Sleep.cpp:876` returns early until
   `!Particle.connected() && !Connectivity::isRadioPoweredOn()` (non-standby
   path, `sleepPreconditionsSatisfied()` at :867). The hibernate block that
   computes `rtcNow`, `wakeTime` and `retainedHibernateRequestedSleep`
   (:1058-1085) is downstream of that gate. **A `Particle.publish()` there
   cannot transmit.** Any publish must happen *before* teardown.

2. **`hibernate_wake` already covers the ordinary late-wake case.** It
   carries `req`, `actual`, `err` and `gateArm` (confirmed in the field:
   Dev-14 `{"req":28260,"actual":28261,"err":1,"gateArm":"none"}`). A device
   that oversleeps but *eventually* wakes already reports everything this
   amendment would add. That is the common case, so this amendment's value
   is narrower than "we are blind to Dev-11".

## Residual justification

Two gaps survive point 2, and they are the whole scope of this amendment:

- **The never-wakes case.** A device requiring manual reset publishes
  nothing, and its retained state is destroyed by the reset. Today we learn
  nothing at all from that outcome.
- **Independent corroboration.** `hibernate_wake` is self-reported *after*
  the event by the same subsystem under suspicion - retained SRAM plus the
  AB1805. If the fault corrupts retained state, `req` and `rtcBefore` are
  themselves untrustworthy and nothing detects it. A record emitted at a
  different time, over a different path, before the suspect sleep, makes the
  two cross-checkable. This is the stronger of the two reasons.

## Required behaviour

1. **Extract the hibernate plan into a pure, host-testable function.**
   Follow the existing pattern (`src/time/ClockTrust.h`,
   `src/time/HibernateWakeDiagnostics.h`): a header, no Particle
   dependencies, no I/O. It must take the inputs the decision already uses
   (candidate `wakeInSeconds`, occupancy, sensor mode, debounce setting,
   `overnightFallbackSleep`, RTC epoch) and return the planned duration plus
   whether hibernate is eligible.

2. **Call it exactly once per sleep cycle, before teardown, and carry the
   result forward.** Do **not** duplicate the computation at the two sites -
   a predicted plan that can silently diverge from the executed one is worse
   than no plan at all. The hibernate block must consume the same value it
   published, and any divergence between plan and execution must be logged.

3. **Publish before teardown, gated by build flag.** Add
   `ENABLE_HIBERNATE_PLAN_PUBLISH` to `src/BuildProfile.h`, **default 0**.
   This costs one publish of airtime per hibernate; production devices must
   not pay it until we choose to. Bench devices set it to 1. Follow the
   lifecycle documented for `ENABLE_RTC_SKEW_TEST`.

4. **It must not delay, block, or prevent sleep.** Bounded and best-effort:
   a failed or unsent publish proceeds to sleep normally. It must not extend
   the connected window waiting for an ack, must not trip either watchdog
   (app 60s, AB1805 124s), and must respect connection mode and battery tier
   exactly as required by item 3 of the parent WO.

5. **Reconcile on wake.** `hibernate_wake` should carry enough to be matched
   against its plan - reuse `retainedHibernateCount`, which is already
   `retained` (verified: `Generalized-Core-Counter.cpp:487`,
   `State_Common.h:146`) and already published as `count`.

## Acceptance criteria

- The plan function is pure, host-tested, and every test mutation-tested.
- Exactly one computation site. A test must fail if plan and executed
  duration diverge.
- With the flag at 0 the emitted binary is unchanged in behaviour and no
  publish occurs; demonstrate by comparing a build at 0 against current
  `main`, not by inspection.
- A publish failure at any point still results in a normal hibernate.
- Alert-40 opening-hour suppression and the wake-validation gate are
  untouched (parent WO, criterion 4).
- Boron build (`particle compile boron .`, explicit platform).
- Mandatory Codex Stage 7 at the AB1805/watchdog tier - this touches the
  sleep path.
- Bench validation: a real hibernate on a bench device with the flag at 1,
  showing the plan event and the matching `hibernate_wake` with the same
  `count`.

## Out of scope

- Diagnosing the oversleep itself. This amendment buys evidence; it does not
  attempt a fix.
- Any change to hibernate duration computation *semantics*. Extracting it to
  a pure function must be behaviour-preserving.

## Provenance

2026-09-02 overnight run. Dev-11 (`e00fce683f6063bf254283dd`) last serial
22:07:29 SGT, silent 9h, `soc=78.8% vcell=4.016 vbus=USB chg=DONE th=OK` at
sleep entry - exhaustion and thermal ruled out. Dev-14
(`e00fce688e592afaf23ac4fb`) `hibernate_wake` at 2026-09-02T22:00:47Z,
`err=1`. Slow-teardown hypothesis refuted the same morning: Dev-14 tore down
in 17564ms and Dev-11 in 17481ms, both tripping `slow_teardown`, both
entering sleep with `standby=0/0`, opposite outcomes.
