# WO-2026-08-31-004: Investigate why the AB1805 loses hours during HIBERNATE

**Type:** Investigation. No fix proposed - the mechanism is not yet known.

**Status:** This is the ACTUAL fault. `WO-2026-08-29-002` repairs the
clock after a wake; `WO-2026-08-31-002` will make that repair reachable.
Neither stops the loss. A device with this defect will keep oversleeping
its operating window every night and being corrected the following
morning.

## The observation

Boron-Dev-11 (product 42131, v24-Thermal-Inhibit) loses hours of real time
across a single closed-hours HIBERNATE, and the loss compounds because
nothing corrects it:

```
clock offset before 2026-08-28 hibernate   -0.007h   (correct)
after that hibernate                       -5.153h
after the following hibernate             -13.45h
```

Real sleep durations against an 8 h request: **13.13 h** and **16.29 h**.
The device believes it slept exactly its requested 8 h each time.

Measured three independent ways, all agreeing:
- device-reported `timestamp` minus cloud `published_at`, across 15+
  isolated hourly reports;
- the firmware's own `TimeDiag` line (`epoch=1788092724` = device
  `2026-08-30 12:25:24Z` while real time was `2026-08-31 01:52:20Z`);
- uptime back-calculation from the `millis()` counter in serial logs.

It has now entered a wrong-time hibernate on three consecutive nights
(2026-08-29, -30, -31), each time on a decision that was correct by its own
clock: its last report before sleeping reads 22:00 SGT, exactly its
`closeHour=22`.

## What is ruled out

- **Not USB/VBUS related.** All three bench Borons hibernate on identical
  power (`vbus=USB pg=1 src=USB_HOST`, `chg=DONE(3)`, `fault=0x00`).
  Dev-09 and Dev-14 sleep 8.01 h cleanly under exactly that condition;
  Dev-14's wake reported `actualSleep=28798` against a `28797` s request -
  a **one second** error.
- **Not battery exhaustion.** On USB throughout, `soc` 77-78%, charge
  complete.
- **Not a calculation bug reusing bad state.** The alarm is
  `wakeTime = rtcNow + wakeInSeconds` (`State_Sleep.cpp:1064`), a relative
  offset from the RTC's own reading, so a wrong starting value shifts the
  absolute target but cannot stretch the duration. And
  `kMaxHibernateSleepSec = 36000` (10 h, `State_Sleep.cpp:281`) caps any
  requested sleep - the observed 13.13 h exceeds what any permitted request
  can produce with an RTC ticking at true rate.
- **Not thermal.** 74/74 `ChargeDiag` lines across all three units read
  `th=OK`; zero inhibit arm/clear lines.

## Leading hypothesis: no independent backup power on the AB1805

Established 2026-08-31 by the Chief Engineer, and **not documented
anywhere in this repository**: disconnecting both the LiPo and USB clears
the AB1805 entirely - the RTC loses time and reads unset until re-set. So
no coin cell or supercap is populated on its `VBACKUP` rail; it rides the
system rail with no independent hold-up.

If HIBERNATE drops, gates or browns out that rail, an RTC with no backup
would lose time exactly as observed. This would also explain why the fault
is confined to one unit: a marginal rail, connector or solder joint on
Dev-11 would present this way while identical peers stay correct.

**This is a hypothesis, not a conclusion.** It has not been measured.

## Evidence that would confirm or kill it

- Scope or log the AB1805 supply rail across a HIBERNATE entry and wake on
  an affected unit. A sag or dropout during sleep confirms it; a clean rail
  kills it.
- Compare the AB1805 supply between Dev-11 and a healthy peer under
  identical conditions.
- Read the AB1805's own oscillator-failure / power-fail status registers on
  wake, if the part exposes them, and surface them in telemetry.
- Reflow or swap the affected unit's AB1805 and see whether the loss
  follows the board or the part.

## What is missing to proceed

This repository is firmware-only and carries **no carrier-board hardware
documentation**. `docs/datasheets/` holds two battery datasheets and the
Particle module datasheets (`P105_ds.pdf`, `P126_ds.pdf`) - no carrier
schematic, no pinout, no notes. Searching all docs for
`backup`/`VBAT`/`supercap`/`coin cell`/`I2C.*AB1805` returns only software
references to nRF52 `retained` backup RAM, which is unrelated.

Needed from the carrier-board design source: whether `VBACKUP` is
populated and with what, whether the AB1805 supply is switched or gated
during sleep, and whether the rail is measurable on a header.

## Related work orders

- `WO-2026-08-29-002` (committed) - repairs the clock after a wake that
  connects. Does not prevent the loss.
- `WO-2026-08-31-002` - will make that repair reachable when the device
  wakes believing it is closed. Does not prevent the loss.
- `WO-2026-08-29-001` - would report which wake-validation gate arm failed.
  Dev-11 failed that gate on two consecutive wakes and we still cannot say
  which arm, because the diagnostic is unreachable without it.
- `WO-2026-08-31-003` - the bench hook needed to validate the above.

## Provenance

Field evidence from Boron-Dev-11, 2026-08-28 to 2026-08-31; queries
recorded in the WO-2026-08-29-002 investigation. Backup-power observation
from the Chief Engineer, 2026-08-31.

---

# Amendment A (2026-09-03) - the loss is a RATE fault, not a hibernate fault

**Status:** Specification only. **Not dispatched, and deliberately NOT to be
flashed to Boron-Dev-11 in its current state** - see "Scheduling" below.

## What changed

The first direct measurement of Dev-11's RTC rate. On the manual reset of
2026-09-03 10:24 SGT, three queued `hibernate_wake` events published at once
(`count=1,2,3`; all `osReason=20` PIN_RESET - events 2 and 3 are the manual
resets themselves, not faults). Event 1 spans the overnight sleep:

```json
{"result":"fail","gateArm":"reset_reason","osReason":20,"rtcOk":1,
 "req":28380,"rtcBefore":1788356249,"rtcAt":1788380865,"count":1}
```

| Window | Real elapsed | RTC advanced | Rate |
|---|---|---|---|
| **Awake** 21:08:47 -> 22:07:22 | 58m35s | 28m52s | **49.3%** |
| **Asleep** 22:07:22 -> 10:24:15 | 12h16m53s | 6h50m16s | **55.7%** |

The awake window is measured between the `ClockResync` RTC write at 21:08:47
(`epoch=1788354517`) and the sleep entry (`rtcBefore=1788356249`), on a device
awake and powered from USB throughout.

**This retitles the Work Order in substance.** The AB1805 does not lose hours
*during HIBERNATE*; it counts at roughly half real time **continuously**,
awake and asleep alike. The hibernate loss (5h26m37s) is the same defect
observed over a longer window.

It also dissolves the "oversleep". The alarm was set for
`1788356249 + 28380 = 1788384629`; at the reset the RTC had reached only
`1788380865`, still **1h02m44s short** - about **1h53m more real time** before
it would have fired. Dev-11 never missed a wake. The alarm had not come due.
This retires the open puzzle from 2026-09-01/02, where Dev-11 hibernated with a
verified-correct clock (+/-2s) and still overslept: initial correctness is
irrelevant if the rate is wrong.

Corroboration is independent, per `STYLE_GUIDE.md` §6: real elapsed comes from
cloud publish and serial timestamps; the RTC delta comes from the device. The
suspect subsystem is not vouching for itself.

## Effect on the leading hypothesis

The backup-rail hypothesis above is **not confirmed and is partly contradicted**:

- The loss occurs while the device is **awake and on USB power**, when the
  system rail is by definition up. A hibernate-only rail collapse cannot
  explain a 49.3% awake rate.
- Dev-11's carrier was swapped on 2026-09-02, so this is a **new AB1805 and a
  new crystal**, and the fault persisted unchanged. "A marginal rail or joint
  on this board" no longer explains it without assuming the same defect
  recurred on a second board.

A ~50% rate is also the wrong shape for analog drift - a mis-loaded crystal
drifts in ppm, not by half. That points at a discrete oscillator-selection or
divider fault rather than a degraded part.

## New leading hypothesis: the AB1805 is running on its RC oscillator

The part has exactly this failure mode. `REG_OSC_CTRL_OSEL` selects 32.768 kHz
(0) or **128 Hz** (1); `AOS` auto-switches to the RC oscillator **on battery**,
and `FOS` auto-switches **on crystal failure**
(`lib/AB1805_RK/src/AB1805_RK.h:866-869`). Given this hardware has no populated
`VBACKUP` cell, an `AOS`-triggered switch is a plausible mechanism that would
apply to *any* AB1805 in this design, not just this unit.

The vendored library already exposes the check - `AB1805::usingRCOscillator()`
(`AB1805_RK.h:89`), *"true if RC oscillator is being used, false if XT
(crystal)"* - and **it has zero callers in application code.**

## Scope - deliberately minimal

**One I2C read at boot, logged. No behaviour change of any kind.**

- No new gate, branch, retry, or state.
- No change to sleep, wake, clock trust, or the hibernate path.
- Nothing conditional on the result. The firmware must behave identically
  whether the answer is XT or RC.
- No new build flag. This ships enabled.

Log, at **every boot on every device**:

- `AB1805::usingRCOscillator()`
- `REG_OSC_STATUS` (`0x1d`) - at minimum `OMODE` (bit 0) and `OF` (bit 1)
- `REG_OSC_CTRL` (`0x1c`) - the raw byte, so `OSEL`/`ACAL`/`AOS`/`FOS` are
  all recoverable

**Every device, not only Dev-11.** The diagnostic value is entirely in the
comparison. A line on Dev-11 alone proves nothing; Dev-09 and Dev-14 are the
controls, and their reading is what makes Dev-11's meaningful.

## Falsifiable prediction

State it before the data arrives, as with this WO's timeline entries:

- **If Dev-11 reports RC and Dev-09/Dev-14 report XT**, the fault is a
  configuration / oscillator-selection issue. The module-and-carrier swap plan
  is unnecessary and should be cancelled.
- **If all three report the same oscillator state**, this hypothesis is
  **wrong**. The rate-halving needs a different explanation and this amendment
  has cost one log line, which is the point of keeping it this size.

## If it is RC: the follow-up question that matters

The carrier swap reset retained memory (`count` restarted at 1), so there is
**almost no history on this specific new part's oscillator behaviour**. If
Dev-11 reads RC, determine which of these it is:

- **RC from the first boot after the swap** - the condition was present
  immediately. Consistent with a manufacturing defect in this particular part,
  or with a config the firmware applies at every setup.
- **Started XT and fell back later** - matches the intermittent "clean for a
  day and a half, then wrong" pattern seen *before* the swap, now reproduced on
  new hardware. That would make it a **systemic `AOS`-triggering condition that
  any AB1805 in this design without a backup cell would eventually hit** - a
  fleet problem, not a unit problem.

These have very different consequences, and the boot-by-boot log distinguishes
them at no extra cost.

## Scheduling

**Do not dispatch or flash this before 2026-09-03 end of day.** Dev-11 has just
come through a disruptive few hours (carrier swap, repeated manual resets) and
is finally stable and sleeping normally. Its current soak is more valuable
running than interrupted, and this is a passive log addition with no urgency.

Queue for the **first Copilot cycle after the Chief Engineer returns.**

## Provenance

Boron-Dev-11 (`e00fce683f6063bf254283dd`) manual reset 2026-09-03 10:24 SGT;
three `hibernate_wake` events published 02:24:22-02:24:29Z. Rate arithmetic from
those payloads against serial timestamps of the 21:08:47 `ClockResync` and the
22:07:22 sleep entry. Oscillator register semantics from
`lib/AB1805_RK/src/AB1805_RK.h:866-878`.

---

# Amendment B (2026-09-21/22): Amendment A dispatched - CLOSED, prediction resolved

**Status:** CLOSED. Both parts of Amendment A's queued scope, plus one related
addition, implemented, bench-verified on Dev-09/Dev-11/Dev-14, and merged.

## Part 1 - the falsifiable prediction, resolved

Amendment A's exact queued scope was dispatched: `usingRCOscillator()`,
`REG_OSC_STATUS`, and `REG_OSC_CTRL` (decoding `AOS`/`FOS`) logged once at
every boot, unconditionally, on all three bench devices - no gate, no branch,
no behaviour change, one I2C read.

Between when Amendment A was drafted (2026-09-03) and this dispatch, ten more
days of field data refined the framing: the daily rate is not a fixed ~50%
but varies 56.5%-82.1%, never below ~50%, never above ~83%, with no settled
pattern. A fixed divider or static oscillator misselection cannot produce a
*varying* daily rate; a part that spends part of each interval on the wrong
source could. This dispatch therefore also decoded `AOS`/`FOS` explicitly (not
just `OMODE`), since a part with automatic switching armed could show the
static read as XT while still switching during the interval.

**Result, all three devices, identical:**

| Device | `usingRC` | `oscStatus` | `oscCtrl` | `aos` | `fos` |
|---|---|---|---|---|---|
| Dev-09 | false | 34 (`0x22`) | 0 | false | false |
| Dev-11 | false | 34 (`0x22`) | 0 | false | false |
| Dev-14 | false | 34 (`0x22`) | 0 | false | false |

Confirmed stable and unchanged across a full overnight soak (multiple resets
per device, including a genuine multi-hour HIBERNATE on each - see the
`hibernate_wake` table below) - not a one-boot fluke.

**Per Amendment A's own pre-registered criterion - *"if all three report the
same oscillator state, this hypothesis is wrong"* - the RC-oscillator/AOS-FOS-
switching hypothesis is FALSIFIED.** Dev-11 is not running on the RC
oscillator, and it is not merely reading XT at boot while armed to switch:
`oscCtrl=0` means `AOS` and `FOS` are both disabled, so the automatic-switch
mechanism this hypothesis rested on is not even armed, on any of the three
devices.

This is corroborated at the source level, not just by the register read:
grepped every write site to `REG_OSC_CTRL` in both the vendored library and
this application. There are exactly two:
`AB1805::resetConfig(RESET_DISABLE_XT)` (`AB1805_RK.cpp:150-159`, the only
site that ever sets `OSEL`/`FOS`) and a `PWGT`-only bit-set inside the
library's own sleep-prep routine (`AB1805_RK.cpp:565`, unrelated to
`AOS`/`FOS`). `resetConfig()` has **zero callers anywhere in `src/`** - this
application never disables XT or arms automatic RC fallback. `oscCtrl=0`
is not a coincidence of read timing; nothing in this codebase can produce
anything else.

**The rate-halving mystery is therefore still open.** This closes out
Amendment A's specific, narrow question - it does not solve the parent WO.
The RC-oscillator/AOS-FOS hypothesis is retired; a new hypothesis is needed
before further register-level instrumentation is worth adding.

## Part 2 (Amendment A-2) - sync-correction magnitude

A related but separately-scoped addition, approved alongside Amendment A:
`Clock::checkClockResync()` now reads the RTC's own pre-write value (one new
`ab1805.getRtcAsTime()` call, immediately before the existing
`setRtcFromSystem()` write) and logs/publishes the resulting correction delta
(`Time.now()` at write time minus that pre-write reading) on every confirmed
resync. This does not explain the rate fault - it is a lower-cost, ongoing
instrument for tracking how large each correction is over time, without
needing to reconstruct it from cross-referencing serial timestamps by hand
the way the original 2026-09-03 rate arithmetic in this WO had to.

Confirmed live on real hardware, e.g. Dev-11: `ClockResync: sync advanced,
rtcUpdated=1 epoch=1789982254 correctionSec=0`.

## Cloud telemetry added

Both the oscillator state and the correction magnitude are now published to
the `device-status` cloud ledger's `clock` object
(`trusted`/`syncAgeSec`/`lastSyncEpoch`/`correctionSec`/`usingRC`/`oscStatus`/
`oscCtrl`/`aos`/`fos`), not serial-only - this bench rig (benchtop supply
through Battery-In, and later the Pi-forwarder USB transfer) cannot
guarantee a serial connection, and the cloud-forwarded serial reconstruction
was independently confirmed to miss anything logged before cellular/cloud
connectivity is established in a given boot (verified by checking that even
a long-pre-existing, unrelated boot line, `AppWDT:`, is equally invisible in
that reconstruction). The oscillator fields are stable per-boot by
construction (set once in `setup()`, never rewritten) - duplicate
suppression holding them steady between reports is expected behaviour, not
a defect.

## Bench validation

Dev-09, Dev-11, Dev-14, overnight soak 2026-09-21 into 2026-09-22, all three
on identical firmware. All three completed a genuine multi-hour HIBERNATE
overnight:

| Device | Hibernate start (SGT) | Wake (SGT) | Requested | Result |
|---|---|---|---|---|
| Dev-14 | 9/21 22:02:20 | 9/22 06:00:19 | 7.97h | OK - clean `ALARM` wake |
| Dev-11 | 9/21 23:03:30 | 9/22 06:00:23 | 6.95h | OK - clean `ALARM` wake |
| Dev-09 | 9/21 22:09:51 | 9/22 06:00:39 | 7.85h | FAIL - AB1805 reported `DEEP_POWER_DOWN`, not `ALARM`; `HibernateCycle`'s own gate correctly withheld a fabricated duration |

Dev-09's failed gate is noted here as a live example of the gate this WO's
own `WO-2026-08-29-001` exists to report - not a rate-fault symptom, and not
investigated further under this WO.

## Provenance

Implemented and bench-validated 2026-09-21/22 on branch
`wo/2026-08-31-004-oscillator-sync-magnitude`, commits `1628192` (Amendment A
+ A-2 implementation) and `1c5819b` (Amendment A cloud follow-up). Falsifiable
prediction stated in the original Amendment A section (2026-09-03), resolved
against real hardware readings pulled via the Particle Ledger API 2026-09-21
and 2026-09-22.
