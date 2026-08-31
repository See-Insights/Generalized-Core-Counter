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
