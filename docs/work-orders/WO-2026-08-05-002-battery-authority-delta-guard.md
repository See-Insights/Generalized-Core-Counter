# WO-2026-08-05-002: Add SOC-vs-Vcell consistency check to battery authority (supersedes pre-radio-delta framing)

## Context

This is a firmware finding from the Boron-Dev-09 SOC/charge-state
investigation, in `Generalized-Core-Counter/src/sensors/SensorManager.cpp`
— not the fleet-ops CLI work from the earlier WOs today.

The firmware already has a "battery authority" mechanism intended to guard
against spurious SOC readings that appear immediately after a cellular
radio connect (voltage sag/recovery from connect current draw can produce a
misleadingly high post-connect reading). Confirmed directly in a raw serial
capture around a real event on 2026-07-31:

```
Battery post-connect sample ignored: preRadio=12.6 postConnect=78.6 delta=66.0
Battery authority: keeping pre-radio sample SoC=12.59% ...; rejecting later
  sample SoC=78.63% ...
```

The guard worked as designed for that one report cycle — but one hour
later, the next connect cycle's live reading showed `soc=78.6%` anyway, no
rejection logged.

Traced via direct source reading (`sed -n '670,730p'` on
`SensorManager.cpp`) plus confirming grep for `batterySampleLooksSuspicious`,
`batterySampleHasUnrealisticDelta`, `batterySampleDelta`:

- `_authoritativeBatterySoc` (the cached baseline used for comparison) is
  only ever updated inside the `sampleIsPreRadio` branch (~line 679-682).
- That branch's acceptance condition is `sampleIsPreRadio && (fallbackUsed
  || previousKnownGoodUsed || !batterySampleLooksSuspicious(...))` —
  `batterySampleLooksSuspicious()` checks only intrinsic plausibility
  (battery state, vcell range, etc.), taking no authoritative-value or delta
  parameter at all.
- `batterySampleHasUnrealisticDelta()`/`batterySampleDelta()` — the actual
  delta-sanity functions — are only called from the separate post-connect
  comparison branch (~line 696-698), never from the pre-radio acceptance
  path.

So the exact same magnitude of jump that gets correctly caught and rejected
when it arrives as a post-connect sample sails through completely unguarded
when it arrives as a pre-radio sample instead — which is what almost
certainly happened at the 21:00 connect cycle: the gauge's internal state
had already changed (likely via `fuelGauge.quickStart()` per the earlier
code-level root-cause investigation into this same incident), and the next
pre-radio read simply picked up the new value directly, with no delta check
applied before it silently became the new authoritative baseline.

## Reframing: this is a more fundamental gap than the asymmetry alone

Two further observations (from reviewing the captured serial log directly)
reframe the fix:

1. **The post-connect delta check is direction-blind.** Powering up the
   radio can only cause a transient voltage *sag* (current draw), which
   would make SOC read *lower* post-connect — never higher. The 20:00
   rejection (`postConnect=78.6` vs `preRadio=12.6`, delta=66.0) was an
   *increase*, which cannot be explained by connect-current artifact at
   all. The magnitude-only delta check treats increases and decreases
   identically, when only decreases fit the justification the check exists
   for. The 78.6 reading wasn't a connect artifact — it was very likely the
   aftermath of `fuelGauge.quickStart()` (confirmed present in this file's
   wake path per the earlier code review) firing during the same wake/
   connect sequence, and deserves distinct handling, not the same bucket as
   a sag-explained drop.

2. **A single-sample Vcell-vs-SOC consistency check would have caught this
   hours earlier, with no history or comparison logic needed at all.** The
   `08:00:19` sample — `vcell=4.028 soc=11.8` — is already physically
   inconsistent: 4.028V is a near-full resting voltage for this chemistry,
   nowhere close to consistent with 11.8% SOC on any normal discharge
   curve. `batterySampleLooksSuspicious()` checks intrinsic validity (state,
   vcell range) but not SOC-against-Vcell plausibility. This inconsistency
   was very likely present through the entire slow 0.34%→12.59% climb
   documented earlier in this investigation (via the Ubidots export), not
   just at the moment of the jump.

## Proposed fix direction

Add a **single-sample SOC-vs-Vcell consistency check**, evaluated on every
sample regardless of radio/connect state, using this battery chemistry's
known OCV curve (or a simpler banded threshold if a full curve isn't
practical). When SOC and Vcell disagree beyond the threshold: **trust
Vcell, and proactively resync via `fuelGauge.quickStart()`** (or equivalent)
rather than waiting for `batterySocIsValid()` to fail reactively at wake.

This directly addresses the root cause rather than filtering its symptoms,
and may substantially simplify or replace the existing pre-radio/
post-connect comparison machinery — worth Codex assessing whether the
existing `_authoritativeBatterySoc` comparison logic becomes largely
redundant once this check exists, or whether it should be kept as a
secondary guard.

Also: make the post-connect comparison direction-aware regardless of
whether the Vcell-consistency check replaces it — an SOC *increase*
post-connect should never be explained away as a connect artifact.

## Historical context (six years of prior art) — verified against source

Chip surfaced two Particle community threads as prior art. Both were
fetched and checked directly rather than taken at face value, since
external forum content is unverified evidence, not settled fact:

**2020 thread, ["Sleepy Boron - SOC Not
Updating"](https://community.particle.io/t/sleepy-boron-soc-not-updating/55086):**
confirmed real. Original poster was **Backpacker87** (not Chip), on a
Boron LTE on Device OS 1.5.0-rc.1/rc.2, SOC stuck at 55.36% overnight in
`SLEEP_NETWORK_STANDBY`, only changing after a physical battery pull/
reinsert.

- **ScruffR's suggestion is confirmed verbatim**: "Try adding
  `fuel.quickStart()` in `setup()` and after your sleep statement."
  However, **Backpacker87 (the original poster) tested it and reported it
  did not resolve the problem** — this is a meaningful correction to the
  WO's framing. `quickStart()`'s presence in today's wake path is
  plausibly downstream of this suggestion, but it was not validated as an
  effective fix in the thread that proposed it.
- **The confirmed actual resolution in that thread was a Device OS
  downgrade to 1.4.4**, isolating the bug to the combination of Sleep 2.0
  API + Power Management API. **Chip (posting as `chipmc`) was an active
  participant in this exact thread** and is credited with that isolation.
  Six years and many Device OS releases later (this project is on 6.4.1),
  whether that underlying defect still exists, still applies, or was fixed
  upstream is unknown — the historical case for *needing* `quickStart()`
  in the wake path today is weaker than "adopted as the validated fix,"
  more accurately "adopted as a defensive carryover from an unresolved-at
  -the-time community workaround."
- **Rftop's suggestion is confirmed**: "Has anyone tried `fuel.getVCell()`
  instead of SOC? That's a direct measurement," alongside a proposal to
  briefly disable charging after wake to stabilize the voltage reading.
  **Chip tested this voltage-tracking approach** (confirmed, same thread) —
  real historical precedent for the "trust Vcell" direction this WO
  already proposes, independent of and predating this investigation.

**2024 thread, ["FuelGauge vs System Battery Level Race
Condition?"](https://community.particle.io/t/fuelgauge-vs-system-battery-level-race-condition/66426):**
confirmed real, unrelated poster (`pbass450`, solar-powered Boron).
Particle's `rickkas7` confirmed the underlying mechanism as characterized:
"The fuel gauge is powered off during sleep, and it takes time for it to
start up and be able to return a value" — recommending retry-with-bound
rather than a fixed delay. This is a real, separate settle-time mechanism,
but — as already noted in this WO — likely **not** the primary explanation
for the 2026-07-31 incident specifically: `vcell` read an identical, stable
4.028V across all three incident timestamps (08:00, 20:00, 21:00) while
only reported SOC changed, which doesn't fit an insufficient-settle-time
story (a not-yet-ready chip would more plausibly produce an unstable or
invalid voltage too, not a rock-steady one paired with a wrong
percentage). Kept as a secondary consideration, not the leading hypothesis.

**Net effect on this WO's architecture**: the historical record supports
the "trust Vcell" direction more strongly than it supports treating
`quickStart()` in the wake path as untouchable — it was a plausible-but-
unvalidated community suggestion, not a proven fix, and the real fix six
years ago was an OS downgrade unrelated to this app's own logic. This is
consistent with, and does not change, the reconciled recommendation below:
keep the existing wake-path `quickStart()` call, but validate its output
against Vcell rather than trusting it blindly — the historical nuance just
means "keep quickStart() because removing it risks reintroducing a proven
fix" is not quite the right justification; "keep it as low-risk legacy
behavior, but no longer trust its result unconditionally" is more accurate.

## Decisions needed (not prescribed by this WO)

- **Consistency threshold**: how much SOC-vs-Vcell disagreement should
  trigger a resync? Needs the fuel gauge chemistry's actual OCV curve or
  datasheet, and should probably be temperature-compensated (captured logs
  show temperatures ranging ~22-33°C, which shifts the OCV curve somewhat).
- **Resync frequency/debounce**: `quickStart()` discards coulomb-counter
  history — resyncing too eagerly on borderline disagreement could itself
  introduce noise. Consider requiring the disagreement to persist across
  more than one sample before triggering a resync, versus acting
  immediately.
- **Scope of the existing pre-radio/post-connect machinery**: keep it as a
  secondary guard, simplify it, or remove it in favor of the consistency
  check — Codex's investigation should inform this rather than deciding it
  in advance.

These should be resolved before implementation proceeds, not left to
Copilot to infer.

## Acceptance Criteria

TBD pending the decisions above and Codex's investigation into the fuller
authority-state machine and the fuel gauge's actual OCV characteristics.

## Required Tests

- A sample with SOC inconsistent with Vcell (beyond the chosen threshold)
  triggers the chosen resync behavior.
- A sample with SOC consistent with Vcell is left alone (regression).
- A post-connect sample with a large *decrease* from the pre-radio baseline
  is still handled by the existing sag-artifact logic (regression, if that
  logic is retained).
- A post-connect sample with a large *increase* is NOT treated as a
  sag-artifact rejection candidate (new: catches the direction-blindness
  bug directly).
- The specific 2026-07-31 sequence (11.8%@4.028V at 08:00, eventual jump to
  78.6%) should be reproducible as a test fixture if feasible, to confirm
  the fix would have caught it at 08:00 rather than only resolving it by
  21:00.

## Permitted Files

Likely `src/sensors/SensorManager.cpp` and its test file, if one exists.
Codex to confirm exact scope and whether `SensorManager.h` needs changes.

## Stage 4 — Independent investigation (Codex, gpt-5.6-sol, high reasoning)

Full report: read-only investigation, no files changed. Codex confirmed the
control-flow gap is real but revised the root-cause framing on two points,
and surfaced one piece of prior art in this codebase that changes the
recommended architecture. Summary (see reconciliation below for how each
was resolved):

1. **The "silent bypass an hour later" is not a bypass.**
   `_authoritativeBatterySampleActive`/`_authoritativeBatterySoc` are reset
   to false/0 on every low-power wake
   (`SensorManager::noteWakeFromLowPowerSleep()`, line 422-428, confirmed by
   Claude independently). So the post-connect comparison guard is scoped to
   a single wake cycle by design — it has no authority baseline left to
   compare against on the next wake, and 78.6% becoming authoritative is the
   expected result of that design, not an unguarded overwrite. The real gap
   is narrower: **the pre-radio acceptance path has no plausibility check at
   all**, intrinsic or historical, on any wake — not "the delta check gets
   skipped."

2. **A stale-SOC consistency detector already exists in this file** (lines
   1173-1255), explicitly labeled `Stale SOC Detection (Phase 1: detection
   and instrumentation only)`. It already does SOC-vs-Vcell banded
   plausibility checking with a 2-consecutive-sample debounce
   (`staleSocConsecutiveCount >= 2`) and publishes a `stale_soc` forensics
   event — but it is gated to `externalPowerPresent && (chargeDone ||
   notChargingWithPower) && vcell >= 4.05` and is diagnostic-only (no
   remediation). The captured 08:00:19 sample (vcell=4.028V) falls 22mV
   below this detector's floor, and — unverified without the raw log's PMIC/
   VBUS state at that timestamp — may also have failed the external-power
   gate entirely. **This is very likely "Phase 2" of an already-planned
   mechanism, not a new subsystem to build from scratch.**

3. **`quickStart()` does not discard coulomb-counter history.** The
   MAX17043 is not a coulomb counter — it's a voltage-model (ModelGauge)
   chip with no current-sense resistor; `quickStart()` restarts its internal
   voltage-model "first guess," which assumes 30 minutes of relaxation.
   Firing it while charging or under radio load can itself produce a large
   jump. Confirmed against the datasheet and `spark_wiring_fuel.cpp:252`
   (writes `0x4000` to MODE).

4. **Device OS itself calls `fuel.quickStart()` independent of app code.**
   `system_power_manager.cpp:618` (Device OS 6.4.1) quick-starts the fuel
   gauge on every transition from `BATTERY_STATE_DISCONNECTED` to any other
   state. Confirmed directly by Claude. This means the 78.6% jump may not
   have originated from anything in `SensorManager.cpp` at all — app-level
   debounce/threshold changes cannot prevent this trigger, only detect and
   quarantine its aftermath.

5. **Recommends against a hardcoded generic OCV curve.** No installed-cell
   identity is confirmed in the repo (Particle's standard pack is a
   ZN-103450 3.7V/1800mAh per Particle's docs, but that's not verified for
   Boron-Dev-09). Per the MAX17043 datasheet, a banded plausibility
   detector characterized against the actual pack is preferable to
   borrowing a generic curve.

6. **Direction-awareness is correct but not safely separable.** Making the
   post-connect comparison ignore positive jumps, by itself, would remove
   the only guard that caught the observed 66-point jump. Codex recommends
   deploying positive-jump quarantine (via the new consistency check)
   *simultaneously* with the direction-aware fix, not as a sequenced
   follow-up.

7. **Gate asymmetry**: pre-radio branch gates on `!Connectivity::isRadioPoweredOn()`;
   post-connect branch gates on `Particle.connected()`. Samples with radio on
   but not yet cloud-connected (mid-attach, failed connect) are neither
   authoritative-eligible nor delta-checked — a real gap, not proven to be
   the cause of this specific incident.

8. **Missing evidence**: the complete raw 2026-07-31 sequence — PMIC charge
   status, VBUS/power-good, and radio state at both 08:00:19 and the 20:00/
   21:00 connect cycles — is not yet in hand. Without it, Codex frames
   78.6% as "unverified but voltage-plausible," not definitively spurious.

## Reconciliation and proposed architecture (Claude, pending Chip approval)

Claude independently verified findings 1, 2, and 4 above by direct source
reading (`noteWakeFromLowPowerSleep()` at line 422; the Phase 1 stale-SOC
block at line 1173; and Device OS's own `quickStart()` call at
`system_power_manager.cpp:618`). All three hold up and materially change
the fix from "add a new mechanism" to "extend and re-scope an existing one,
plus close a narrower gap than originally framed."

**Proposed direction, superseding the original WO's framing:**

- **Extend the existing Phase 1 stale-SOC detector into Phase 2**, rather
  than building a parallel consistency-check mechanism. Concretely:
  - Lower/relax its gating so it can evaluate on **any** sample where Vcell
    is usable, not only `externalPowerPresent` + specific PMIC charge
    states — the incident sample may have occurred off external power,
    which the current gate would never evaluate regardless of voltage
    threshold. (Needs the raw-log PMIC/VBUS state to confirm this was
    actually the failure mode here, versus the 22mV threshold miss — see
    open decision below.)
  - Keep its existing 2-consecutive-sample debounce pattern
    (`staleSocConsecutiveCount`) as the model for the new resync debounce,
    per Codex's recommendation — do not resync on a single sample.
  - Add a "quiet state" gate before allowing remediation to fire: radio off
    and not actively PRE/FAST charging, so a forced `quickStart()` isn't
    itself issued under conditions likely to produce a bad first guess.
  - Move it from diagnostic-only to actually invoking `fuelGauge.quickStart()`
    once the debounce and quiet-state conditions are met (this is the
    "Phase 2" step; Phase 1's forensics publish can remain as-is or be
    folded into the same event).
- **Keep the existing pre-radio/post-connect authority machinery as a
  secondary, wake-scoped guard**, not the primary defense — per Codex,
  it solves a different problem (short-lived post-connect load artifacts)
  than intrinsic implausibility does, and removing it would drop real
  coverage.
- **Make the post-connect comparison direction-aware, deployed together
  with the new consistency check** (not as a standalone change) — an SOC
  *increase* should never be classified as sag-artifact rejection, but a
  large increase still needs *some* quarantine path (the new detector),
  or removing direction-blindness would silently let a real bad jump
  straight through.
- **Do not** build a generic hardcoded OCV curve, and **do not** attempt
  RCOMP/temperature compensation via raw I2C writes to the MAX17043 —
  both out of scope per Codex (unverified installed-cell identity; RCOMP
  tuning requires characterization data this project doesn't have).
  Firmware-side temperature compensation of the *threshold band itself*
  (using already-sampled enclosure temp) is architecturally feasible but
  Codex assesses it as overkill relative to a 60+ point discrepancy, and
  notes fresh temperature isn't reliably available at the point the
  battery check runs (temp sampling happens later in the same cycle,
  spread across multiple calls on Boron/TMP36 platforms). Recommend
  deferring temperature compensation, not building it into this WO.

## Update 2026-08-05: confirmed installed cell (Chip)

Chip confirmed all devices use the same cell from the same vendor: PKCELL
LP-803860, 3.7V nominal, 2000mAh (min 1900mAh), with PCM. Datasheet
(`QA.S.0228`, PKCELL) and a vendor discharge-curve reference article were
reviewed directly. Findings:

- Datasheet gives only the standard rated-performance endpoints: nominal
  3.7V, charge voltage 4.2V, discharge cutoff 3.0V, standard charge 0.2C
  CC/4.2V CV/0.01C cutoff, discharge -20~60°C. **No OCV-vs-SOC curve or
  table is published** — this is a generic commodity Li-poly datasheet, not
  a characterized-cell spec sheet. This resolves Codex's "unconfirmed
  installed-cell identity" caveat (we now know the exact part), but does
  **not** give us a first-party numeric curve.
- The vendor's discharge-curve article gives a storage-voltage
  recommendation of ~3.8-3.85V per cell for long-term storage at 50-60% SOC.
  This lines up well with the shape of a standard/generic 3.7V-nominal
  Li-ion discharge curve (flat-ish through the midrange, dropping toward
  3.0V near empty and rising toward 4.15-4.20V near full) — a useful
  cross-check that this cell behaves like a typical commodity LiCoO2-type
  pack, not an unusual chemistry, even without a first-party curve.
- Using that typical/generic curve shape as an estimate (not a manufacturer
  number): `vcell=4.028V` corresponds to roughly 85-90% SOC, not the logged
  11.8%. This raises confidence in the original WO's "physically
  implausible" framing considerably, though it's still an estimate from a
  generic curve rather than a bench-characterized one for this exact cell.

## Update 2026-08-05: raw serial log recovered (Chip)

Chip supplied the actual 2026-07-31 serial log spanning 08:00-22:00.
Verified directly rather than inferred:

- **08:00:19**: `vbus=2` (USB_ADAPTER, external power present), `pg=1`,
  `chg=DONE(3)`, `ichg=0`, `fault=0x00`, `vcell=4.028`, `soc=11.8`. The
  existing Phase 1 stale-SOC detector's gate
  (`externalPowerPresent && chargeDone && noFault`) **was fully satisfied**
  — the only reason it didn't fire is the 22mV shortfall against its 4.05V
  floor. **This resolves the open "does the external-power gate need
  relaxing" question for this specific incident: it does not** — the gate
  was never the problem here, only the threshold number was, exactly as
  Codex's inference predicted.
- **chg=DONE, ichg=0, and vcell essentially frozen (4.028V -> 4.026V) for
  the entire 08:00-22:00 span** (14 hours, across the 09:00 sample, the
  8-11 hour gap, and the 20:00/21:00/22:00 samples) — this is a battery at
  rest, fully charge-terminated, not under any load or charge transient.
- At 20:00:27, the "Battery sample" log line shows the raw fuel-gauge
  registers already reading `raw=78.63 norm=92.50` at the same instant
  `vcell=4.028` — i.e. by the time this sample was taken, the internal
  ModelGauge state had already moved; the log does not show the moment it
  changed (it's within the 09:00-20:00 gap).

### Reframe: the direction of the error may be backwards

Using the confirmed PKCELL cell's typical discharge-curve shape,
`vcell=4.028V` at rest implies roughly 85-90% SOC — much closer to the
raw fuel-gauge values (`78.63`/`92.50`) that appeared at 20:00 than to the
`11.8%`/`12.6%` the authority guard spent all day protecting. Combined
with `chg=DONE` + `ichg=0` + frozen voltage for 14 hours (the classic
signature of the original 2020 community bug — SOC not tracking up after
charge completes), **the low reading was very likely the stale/wrong one,
and the jump to 78.6% was very likely the fuel gauge correcting itself
toward the truth** — not a spurious artifact requiring quarantine. The
existing authority guard's rejection at 20:00 (keeping 12.59%, rejecting
78.63%) was arguably the wrong call in this specific instance, not a
near-miss of the right one.

This does not overturn the WO's core proposal (a Vcell-vs-SOC consistency
check, catching the anomaly as early as 08:00) — if anything it
strengthens it, since the check would have flagged `11.8%` as
*suspiciously low* given `vcell=4.028V`, and the natural remediation
(resync toward Vcell) would have moved the reading up sooner, not
suppressed a later correction. It does mean the "quarantine large positive
jumps" framing needs to be read as "quarantine positive jumps that
*aren't* independently explained by a preceding low-SOC-vs-high-Vcell
flag" rather than "positive jumps are inherently suspect" — in this
incident, the jump was corroborated by Vcell the whole time, not
contradicted by it.

One scope note: this is `Boron-Dev-09`, `profile=UsbBench` — a bench unit
continuously on USB power, not a field battery-only deployment. The
externalPowerPresent-gated path being satisfied here may be more
characteristic of bench units sitting on chargers than of field devices
running on battery alone; the battery-only case remains untested by this
specific log.

## Decisions — CLOSED (Chip, 2026-08-05)

All four previously-open decisions are now resolved:

1. **Consistency threshold — accepted as proposed.** `vcell >= 4.00V`
   paired with `soc <= 20-25%` for the new check, evaluated when radio is
   off and not actively PRE/FAST charging; the existing stricter
   `>=4.10V / <30%` external-power/charging-context tier is untouched.
   Chip's rationale for keeping the two tiers separate: while actively
   charging, terminal voltage runs above true rest-OCV for the same real
   SOC (IR drop through charge current), so the charging-context tier
   correctly needs a higher voltage bar to avoid false negatives — sound
   physical reasoning, no changes needed. Empirically validated against
   the recovered 2026-07-31 log: this band would have caught the
   4.028V/11.8% pair with no gate changes required.

2. **Resync debounce/cooldown — two separate counters, not one.** Claude
   confirmed by tracing all call sites of `SensorManager::batteryState()`
   (`State_Connect.cpp:561`, `State_Idle.cpp:127`, `State_Report.cpp:60`,
   `State_Sleep.cpp` pre-sleep/post-wake) that Phase 1's existing
   `staleSocConsecutiveCount >= 2` debounce is a `retained` counter
   incremented on every qualifying call across those sites — it typically
   resolves **within a single wake, in seconds**, not across separate
   wake-to-wake boundaries. This is a different unit than Chip's proposed
   cooldown, which is about wake-cycle cadence. Resolution: keep two
   distinct counters —
   - **Trigger debounce** (when to act): reuse Phase 1's existing per-call
     `staleSocConsecutiveCount >= 2` pattern unchanged.
   - **Cooldown** (how soon a resync can fire again): a new counter
     incremented once per actual wake, via the existing
     `SensorManager::noteWakeFromLowPowerSleep()` hook (already fires
     exactly once per low-power wake) — set to **3 wake cycles**, per
     Chip's decision, balancing flapping risk against leaving a genuine
     desync uncorrected too long (the same failure shape as the original
     stuck-SOC bug this mechanism must not reintroduce).

3. **External-power gate — removed, agreed.** The new consistency check
   evaluates on any sample with usable Vcell, regardless of power source.
   Chip's rationale: gating to charging-only would miss battery-only field
   operation, the primary real-world case this check needs to catch — even
   though the recovered log shows the gate was *not* the failure mode in
   this specific incident (external power was present), it would be for
   the more common field scenario this WO exists to protect against.

4. **Raw serial log — recovered and reconciled** (see update above).

## Scope of existing pre-radio/post-connect machinery (recommendation, not a blocking decision)

Keep temporarily as a secondary guard; simplify its role to "wake-scoped
transient quarantine" rather than primary truth; ship direction-awareness
and positive-jump quarantine together, not sequenced. Given the log
reframe above, "positive-jump quarantine" should specifically mean
"quarantine a positive jump that arrives *without* prior Vcell
corroboration," not "treat all positive jumps as suspect" — in the
2026-07-31 incident, Vcell corroborated the higher reading the entire
time, so the new consistency check would have already flagged and started
correcting the stale low value before the jump was even observed.

## Required tests (combined WO + Codex list)

- SOC-vs-Vcell disagreement (within the new band) triggers quarantine/
  resync only when the quiet-state gate is also satisfied; does not fire
  while radio is on or charging is active.
- Same disagreement pair evaluated across PMIC states: FAST, DONE, not
  charging, no external power — expected outcomes differ per state.
- Consistent SOC-vs-Vcell sample is left alone (regression).
- Post-connect large *decrease* still handled by existing sag logic
  (regression).
- Post-connect large *increase* is not classified as sag-artifact, and
  independently reaches the new consistency/quarantine path (not silently
  accepted).
- Two-sample debounce: single transient disagreement followed by a
  consistent sample does not trigger resync; two consecutive disagreements
  do.
- Cooldown prevents repeated quick-start loops on persistent mismatch.
- Authority state resets correctly across low-power wake, ULP wake, and
  cold boot (regression on existing `noteWakeFromLowPowerSleep()` behavior).
- The 2026-07-31 sequence reproduced as a fixture, if the raw PMIC/VBUS/
  radio context can be recovered — otherwise document as best-effort/
  partial fixture.

## Permitted files (proposed, pending Chip approval)

`src/sensors/SensorManager.cpp`, `src/sensors/SensorManager.h`, and a new
or extended unit test file under `tests/`. `ConnectivityPolicy.h` only if a
new named constant (debounce count, cooldown duration) is added there to
match existing convention — no behavioral changes outside battery-authority
logic.

## Non-goals / protected areas

- No generic hardcoded OCV curve.
- No raw I2C writes to the MAX17043 CONFIG_REGISTER / RCOMP tuning.
- No change to Device OS's own `quickStart()`-on-`BATTERY_STATE_DISCONNECTED`
  behavior (not controllable from application code).
- No temperature-compensated threshold in this WO (deferred).

## Approval Record

**Approved by Chip, 2026-08-05.** Architecture and all four decisions
above accepted as written. Authorized to proceed to Stage 6
(implementation by GitHub Copilot). No changes to scope requested.

## Status

**Stage 5 complete — approved for implementation.** No code has been
changed by Claude or Codex at any point in this investigation; both
remained read-only throughout, per role restrictions. Handed off via
GitHub issue (see repo) for Copilot to implement inside VS Code, scoped to
the Permitted Files section above. Per the workflow, Copilot must leave
the resulting diff uncommitted for Codex's Stage 7 verification and Chip's
Stage 8 final gate — only Chip commits and pushes.
