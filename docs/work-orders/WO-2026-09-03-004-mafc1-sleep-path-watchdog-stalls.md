# WO-2026-09-03-004: Morrisville-MAFC-1 hangs entering sleep and is rescued by the watchdog

**Status:** Drafted, not dispatched. Investigation first; no fix authorised.

**Device:** `Morrisville-Tennis-MAFC-1-SWAPPED`, `e00fce686548d46c4b45e380`,
product 42131, firmware `21.0_Test`.

**Control:** `Morrisville-Tennis-MAFC-2`, `e00fce6841443bcc0f3178e4` - same site,
same firmware, same siting, same power design.

## The observation

MAFC-1 is reset by the watchdog far more often than its own site-mate, and the
resets are **not confined to one subsystem**.

Watchdog events, 2026-08-01 -> 2026-09-03:

| | MAFC-1 | MAFC-2 |
|---|---|---|
| Total | **16** | 5 |
| Since 2026-08-14 | **8** | **0** |
| Lifetime `watchdogResetCount` | 57 | 19 |

MAFC-2 has been completely clean for three weeks. MAFC-1 reset again on
2026-09-03 at 00:56Z.

### `stage` distribution (ThrashGuard label - NOT the code location; see decode below)

| Stage | MAFC-1 | MAFC-2 |
|---|---|---|
| `connectivity` | **4** | 0 |
| `sleep` | **4** | 0 |
| `diag` | 2 | 5 |

MAFC-1's events in full:

```
08-13 16:26  stage=connectivity  state=4  queue=1  connAge=1823887
08-13 16:26  stage=sleep         state=3  queue=3  connAge=0
08-14 23:49  stage=connectivity  state=4  queue=1  connAge=1844097
08-15 00:20  stage=sleep         state=3  queue=0  connAge=1734308
08-18 23:21  stage=sleep         state=3  queue=0  connAge=727276
08-25 01:40  stage=diag          state=1  queue=0  connAge=2597929
08-25 01:47  stage=sleep         state=3  queue=0  connAge=256592
08-29 00:37  stage=diag          state=1  queue=0  connAge=1462150
08-29 16:00  stage=connectivity  state=4  queue=1  connAge=945696
09-03 00:56  stage=connectivity  state=4  queue=1  connAge=379808
```

MAFC-2's five were **all** `diag`.

## Breadcrumb decode - RESOLVED 2026-09-03 for `bc=21`, partly corrected 2026-09-13

The zero-cost step below was run immediately. Decoding `bc` against the v21 tree
(`48692fe`) rather than `main` is what makes the values readable at all, and it
**refutes the "stage-agnostic shared-bus stall" hypothesis this WO was opened on.**
It resolves `bc=21` to a single site; it does **not** resolve `bc=18`, which the v21
tree writes from two - see the correction below.

| `bc` | v21 location | What the device was doing |
|---|---|---|
| **18** | **ambiguous** - `Generalized-Core-Counter.cpp:1215` **or** `State_Sleep.cpp:852` | Either the **publish-queue exit** or the **sleep-precondition gate** (waiting for `!Particle.connected() && !isRadioPoweredOn()`, i.e. for modem teardown to finish). Both sites write the literal 18 in this tree |
| **21** | `State_Sleep.cpp:1078`, `:1322`, `:1371`, `:1383` | Immediately before `System.sleep(config)`, after `drainSerialBeforeSleep()` - the hibernate, ULP, and stop-fallback paths. `BREADCRUMB_IDLE_ENTRY` (21) is declared but never written in this tree, so this value **is** unambiguous |

**Corrected 2026-09-13.** **MAFC-1's four `bc=21` resets are confirmed in
`State_Sleep.cpp`**, immediately before `System.sleep()`. **The six `bc=18` resets are
not attributable to a single site**: the v21 tree writes the literal 18 both at the
publish-queue exit and at the sleep-precondition gate, so they are one of two
possibilities pending further decode.

The original claim here - *"every one of MAFC-1's ten watchdog resets is in
`State_Sleep.cpp`"* - overstated this, and is retained in this sentence rather than
deleted so the correction is visible. It held for `bc=21` and was assumed for `bc=18`.
Surfaced by `particle-fleet-operations/tools/breadcrumb-decode.js`, which enumerates
every `setAppBreadcrumb(` call site per tree rather than reading the enum alone.

### The error that produced the original hypothesis

The `stage` field (`connectivity` / `sleep` / `diag`) is the **ThrashGuard**
label, which tracks what the guard believed was in progress. The **breadcrumb**
is what locates the code. This WO's first draft read `stage` as if it located
the code, saw three different values, and concluded the stalls were spread
across unrelated subsystems. They are not.

The two fields are consistent once read correctly: a device sitting at
`State_Sleep.cpp:852` is *waiting on connectivity teardown*, so the guard
labels that wait `connectivity` while the breadcrumb correctly reports the
sleep path. `stage=connectivity` with `bc=18` is one situation, not two.

**Do not cite "stalls across three unrelated stages" from this WO.** It was
wrong, it is retained here only so the reasoning error is visible, and the
device comparison below is unaffected by it.

## What still stands

The device-level comparison is untouched by the correction and remains the
reason this WO exists:

- MAFC-1: **16** watchdog resets since 2026-08-01, **8 since 2026-08-14**.
- MAFC-2: 5 total, **0 since 2026-08-14** - same site, same firmware, same
  siting, same power design.
- Lifetime counters 57 vs 19.

Morrisville is the fleet's healthy-siting reference (median `connectTime` about
1 second). Contrast the Singapore bench units, where poor registration is the
deliberate test condition and watchdog recovery from `CELLULAR_ACQUIRE` stalls
is the design working, not a fault. **Do not conflate the two populations.**

## Revised hypothesis: MAFC-1 hangs on the way into sleep

Two distinct hang points, both in the sleep path:

1. **`bc=18` - modem teardown does not complete.** The device requests cloud
   disconnect and radio-off, then waits at the precondition gate for
   `isRadioPoweredOn()` to go false. If the modem does not release, the app
   watchdog eventually fires. Six of ten events - **but see the decode correction
   above: these six are ambiguous between the sleep-precondition gate and the
   publish-queue exit, so this hypothesis rests on the sleep-path reading of an
   ambiguous value.**
2. **`bc=21` - the sleep call does not return or does not wake as expected.**
   The breadcrumb is set after the AB1805 alarm has been programmed and after
   `stopWDT()`, immediately before `System.sleep()`. Four of ten events.

Both point at the **modem/sleep interaction** rather than a general peripheral
fault. Note the fleet already has a known slow-teardown signature elsewhere
(`MODEM_HEALTH: unstable reason=slow_teardown`), which is the same boundary.

### What this does to the I2C reading

**Substantially weakened, not eliminated.** The stalls are not spread across
unrelated subsystems, which was the entire basis for suspecting a shared bus.
The `bc=21` sites do sit just after AB1805 alarm programming, so a peripheral
stall there is still conceivable - but it is now one candidate among several,
and no longer the leading one.

Two further negatives from MAFC-1's `status` payload of 2026-09-01:
`pmicAnomalyCount = 0` and `failsafeCount = 0`. Power-path anomalies are counted
and are zero, which weakens a marginal-power explanation as well.

### Relationship to Dev-11 (WO-2026-08-31-004) - do not conflate

The original draft argued these two "rhyme" in mechanism-shape. **After the
decode, even that is weaker** and the comparison is retained only to keep a
future reader from merging them:

| | Dev-11 (WO-2026-08-31-004) | MAFC-1 (this WO) |
|---|---|---|
| Symptom | RTC **counts at ~50% of real time**, continuously | Device **hangs entering sleep**, watchdog-reset |
| Timekeeping | Wrong rate, device otherwise responsive | No timekeeping anomaly observed |
| Suspected cause | Oscillator selection (RC vs XT) in the AB1805 | Modem teardown / sleep-call interaction |
| Site | Singapore bench, poor signal by design | Morrisville, healthy siting |

These are not the same defect. If both eventually involve the AB1805, that is a
shared *component*, not a shared *defect*.

## Next step

The original zero-cost step - decode `bc` / `lastWatchdogBreadcrumb` against
`48692fe` rather than `main` - **has been done** (see the decode section above).
It cost nothing, required no flash and no device contact, and it overturned the
starting hypothesis. Recording it here as the pattern worth repeating: decode
the breadcrumb before theorising from the stage label.

**Now do this, also at zero cost:** for each of MAFC-1's ten watchdog events,
pull the surrounding `serialLog` (if any reached the cloud) and the preceding
`status` event, and establish for the `bc=18` cases whether the modem teardown
had *started* and simply not completed, or never started. `connAge` is already
in each watchdog payload and ranges from 256592 ms to 2597929 ms, which is
worth correlating against teardown duration. MAFC-2's clean record over the same
window is the control.

**Only after that**, if instrumentation is still wanted: `AB1805_RK`'s
`readRegister()` / `readRegisters()` return `bool` (`AB1805_RK.h:468,506`), so
I2C failures are detectable per call site, but the library keeps **no counter**,
and Device OS exposes no I2C bus-error or timeout counter readable without
adding code. Any counter therefore requires a firmware change - and MAFC-1 has
**no firmware target set** and sits on v21, so flashing it also moves it three
releases forward. That is a larger change than it appears and must be a separate
decision.

## Provenance

Written verbatim as established by the read-only investigation of 2026-09-03:

> Device notes indicate that the replacement unit, formerly
> `OCCUPANCY-DEVSAM04`, was likely assigned to Morrisville MAFC-1 on
> 2026-06-09. No direct hardware-swap or rename audit record was recoverable.

Evidence classification: **INDIRECT.** No swap, rename, claim/re-claim, or
device-ID reassignment audit record exists. Particle exposes no rename-history
endpoint; `git log --all -S'SWAPPED'` returns nothing in either repository. The
date comes from the device's own free-text notes field, verified live against
the Particle API on 2026-09-03:

```
Morrisville-Tennis-MAFC-1-SWAPPED  e00fce686548d46c4b45e380
    notes: JUNE-09-2026: This was OCCUPANCY-DEVSAM04
```

The note does not say "hardware swapped on this date" and could have been
written later. A telemetry inventory capture of 2026-08-09 shows the `-SWAPPED`
name already in use, which bounds the **rename**, not the physical swap.

**Circularity ban, observed and to be observed.** The swap date was NOT inferred
or narrowed from the device's watchdog-reset pattern, error rate, or any
behavioural change in its telemetry. That reset pattern is the thing being
evaluated *against* this date; using it to establish the date would be circular
and would destroy the analysis it is meant to support. Any future work on this
WO must hold to the same rule.

### Consequence: the swap cannot explain a change in behaviour

S3 telemetry partitions begin **2026-06-17**, eight days *after* the candidate
swap date. **Every reset examined here is on post-swap hardware**, and no
pre-swap baseline for this position exists or can be constructed from available
data.

Therefore:

- "Did the swap fix it?" is unanswerable - there is no before.
- The swap is not a candidate cause of a *change*, because no change is visible;
  the rate is flat across the entire observable window.

This slightly strengthens the assembly-specific reading: a physically newer unit
that has stalled steadily since June, while its site-mate on identical firmware
went clean for three weeks, points at something specific to *this* assembly -
carrier, wiring, enclosure - rather than a degradation that began at a datable
moment.

## Noted but not pursued

MAFC-1 came from the **`OCCUPANCY-DEVSAM`** fleet (`OCCUPANCY-DEVSAM04`). It may
have history under that name predating 2026-06-09, in a **different product**,
outside the 42131 data read here. If a longer baseline for this physical unit
matters - in particular whether it stalled before being reassigned - that is
where to look. Not pursued in this pass.

## Acceptance criteria

- The `bc=18` hang characterised: did modem teardown start and stall, or never
  start? Answered from data already in S3, without flashing.
- The `bc=21` hang characterised: does `System.sleep()` fail to return, or
  return and fail to wake? These have different causes.
- The modem-teardown hypothesis explicitly confirmed or killed, not left open.
  If killed, the next hypothesis stated before further work.
- Any residual I2C theory tested against the fact that all ten resets are in the
  sleep path - the original "spread across unrelated stages" argument is
  withdrawn and must not be reused.
- No conflation with WO-2026-08-31-004; any AB1805 involvement described as a
  shared component, not a shared defect.
- Any proposal to flash MAFC-1 raised as a separate decision, since it also
  moves the device from v21 to v24.

## Related

- `WO-2026-08-31-004` - Dev-11 AB1805 rate fault. Different manifestation; see
  the comparison table above.
- Morrisville pair both have `desired_firmware_version = None` and can never
  leave v21 without an explicit target being set. Tracked as a fleet-management
  gap, not part of this WO.

## Amendment A (2026-09-22): two `bc=28` instances on the Singapore bench units - same code shape, different population, do not merge

Two new watchdog resets, same "hangs entering sleep, rescued by the watchdog"
shape as this WO's `bc=21` (v21) findings, observed 2026-09-21 on the
Singapore bench devices during unrelated bench-testing work
(`WO-2026-08-31-004`'s Amendment A/A-2 dispatch):

```
Boron-Dev-14  2026-09-21T11:51:19.929Z  {"reset":"watchdog","osReason":60,"bc":28,"stage":"sleep","elapsed":38,"queue":4,"state":3,"connAge":0}
Boron-Dev-09  2026-09-21T12:23:50.758Z  {"reset":"watchdog","osReason":60,"bc":28,"stage":"sleep","elapsed":39,"queue":4,"state":3,"connAge":244260}
```

**Decoded against the current tree (`main`, v24), per this WO's own rule -
decode the breadcrumb before citing the number.** `bc=28` is
`BREADCRUMB_SLEEP_CALL_ENTER` (`Generalized-Core-Counter.cpp:224`), written at
four sites in `State_Sleep.cpp` (lines 1121, 1373, 1424, 1438), all
functionally the same point: immediately before `System.sleep()`, across the
hibernate/ULP/stop-fallback paths. Unambiguous - unlike v21's `bc=18`, no
collision exists among these four v24 sites. This is the v24 equivalent, in
kind, of this WO's own `bc=21` (v21): "the last breadcrumb recorded before
the sleep call, and the watchdog fired before it returned or woke." The
numbers differ because the enum differs per firmware version - do not read
`28` against `21` as evidence of anything on its own; the equivalence is in
what each number decodes to in its own tree, not in the digits.

**Do not read this as confirmation of MAFC-1's specific hypothesis, and do
not merge the two populations - the same rule this WO already states above
for Dev-11 applies here.** Dev-09 and Dev-14 are Singapore bench units under
a deliberately poor cellular-registration test condition; MAFC-1 is a
healthy-sited Morrisville production unit with a clean site-mate control.
`elapsed=38`/`elapsed=39` on the bench units and `connAge=0`/`244260` are not
compared against MAFC-1's numbers here, and no claim is made that the
underlying stall cause is the same - only that the *code shape* ("hangs at
the sleep-call boundary, watchdog rescues it") recurs on a third and fourth
device, on a different firmware version, which is worth this WO knowing about
even though it is not this WO's population.

Also decoded while checking this, as a correction to an informal
characterization made in conversation (not previously written into any WO):
`bc=18` in the current (v24) tree is unambiguously
`BREADCRUMB_PUBLISH_QUEUE_EXIT` (`Generalized-Core-Counter.cpp:1676`, inside
the main loop's publish-queue servicing, not the sleep path) - v24 resolved
the v21 collision this WO documents above by moving the sleep-precondition
gate to its own code (`BREADCRUMB_SLEEP_GATE_START = 22`,
`State_Sleep.cpp:865`, whose comment records the old collision directly:
"was stale literal 18, colliding with BREADCRUMB_PUBLISH_QUEUE_EXIT"). A
`bc=18` stall on the Singapore bench units today is therefore about the
publish-queue exit point in the main loop, not a cellular-registration or
sleep-path stall - not yet characterized further, and not part of this
amendment's scope.

Not investigated further here - recorded for whoever next looks at either
this WO or a future `bc=18`/`bc=28` occurrence on any device, so the same
decode-before-comparing discipline this WO already established is applied
again rather than re-derived from scratch.
