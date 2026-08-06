# WO-2026-08-05-003: Gate the ledger write on the SOC-vs-Vcell consistency check (same-cycle, not just future resync)

## Context

Follow-up audit after WO-2026-08-05-002 reached Stage 8, prompted by
asking whether SOC is read/written from only one place in the codebase.
It isn't, and one of the other sites is structurally significant.

Confirmed via Codex investigation plus independent direct source reading:

**The ledger write happens before the new consistency check runs, in the
same function.** `current.set_stateOfCharge(soc)` (~line 739/743) is gated
only by the pre-existing `!rejectAuthoritativeOverwrite` mechanism (the
pre-radio/post-connect delta check from before this WO). The new
`BatteryAuthorityPolicy::staleSocConditionsMet()` consistency check doesn't
run until ~line 1169 — well after that write has already committed.

Practical effect: WO-2026-08-05-002's fix changes whether a resync gets
triggered for a *future* cycle. It does not prevent the *current* cycle's
inconsistent value from being written to the ledger and reported
externally (Ubidots, DeviceStatus, connection-mode policy) in that same
cycle. This is a direct gap against the WO's own stated intent — "trust
Vcell... rather than waiting... reactively" — since the current
implementation still reports the bad value once before anything reactive
happens.

**Second confirmed finding:** the 6-hour stuck-fast-charging detector
(alert 21, ~lines 1419-1456) reads raw `soc`/`vcell` directly, not the
arbitrated/authoritative value, and never checks `rejectAuthoritativeOverwrite`.
A sample the authority logic already flagged as an implausible post-connect
candidate can still set this detector's baseline or count as "meaningful
progress" — independent of, and not benefiting from, the arbitration this
WO built.

**Other sites audited and found low-risk / not applicable, no action
needed as part of this WO:**
- ~1286-1287 (power-mismatch diagnostic log) — logging only.
- ~1807-1808 (PMIC test snapshot lambda) — logging only.
- ~1485 (Photon 2/P2 voltage-estimate branch) — different platform, no
  fuel gauge, mechanism doesn't apply.
- ~1765 (`runPmicChargeCycleTest()` safety gate) — real gap, but currently
  dormant (no caller anywhere in the repo) — not an active risk today.
  Worth fixing before it's ever wired up, included below as lower-priority
  scope rather than a separate WO.

## Fix

1. **Primary**: restructure `batteryState()` (or wherever the write
   occurs) so `current.set_stateOfCharge(soc)` cannot commit a value for
   the current cycle until the SOC-vs-Vcell consistency check has had a
   chance to evaluate it — not just influence a future cycle's resync
   decision. This needs investigation into the actual mechanism, not a
   prescribed implementation: options include reordering the write to
   occur after the consistency check, or running a fast preliminary
   corroboration check before the write while the fuller
   debounce/cooldown-gated resync logic still runs separately. Propose
   options with tradeoffs rather than implementing whichever is simplest.
2. Also close the "radio-on/cloud-not-connected gap" Codex flagged as part
   of this same ordering issue — needs investigation to specify exactly
   what scenario this refers to before it can be fixed or explicitly
   documented as an accepted limitation.
3. Fix the stuck-fast-charging detector to use the arbitrated/authoritative
   SOC value rather than raw `soc`/`vcell`, so an already-rejected
   implausible sample can't set its baseline or count as progress.
4. **Lower priority, can split into a smaller follow-up if scope needs
   trimming**: bring the dormant `runPmicChargeCycleTest()` under the same
   authority contract before it's ever wired up to a caller.

## Decisions needed (not prescribed by this WO)

- Which mechanism for closing the ordering gap (reorder the write vs. a
  fast preliminary check) — needs investigation into `batteryState()`'s
  actual control flow and what else depends on write timing before this
  can be decided responsibly.
- Exact nature of the "radio-on/cloud-not-connected gap" — needs
  investigation before it's even fully understood, let alone fixed.

## Acceptance Criteria

- A same-cycle SOC-vs-Vcell-inconsistent sample does not get written to
  the ledger in its raw/unarbitrated form — either the write is deferred
  until arbitration completes, or a corroboration check gates it before
  commit.
- The stuck-fast-charging detector uses the arbitrated SOC value, not a
  raw pre-arbitration reading.
- The radio-on/cloud-not-connected gap is either closed or explicitly
  documented as an accepted limitation with rationale.
- Dormant `runPmicChargeCycleTest()` is brought under the same contract
  (may ship separately if scope needs splitting).
- No regression to normal-case ledger write latency for consistent
  samples.

## Required Tests

- A same-cycle inconsistent-sample scenario does NOT result in a ledger
  write of the raw inconsistent value (test the actual write path, not
  just the resync/cooldown decision logic — this is exactly the class of
  gap that slipped through WO-2026-08-05-002's test suite before its
  fidelity issue was caught).
- Stuck-fast-charging detector test: a rejected/implausible sample as
  input does not reset its baseline or count as meaningful progress.
- Regression: consistent samples still write to the ledger without added
  delay.
- Whatever scenario the radio-on/cloud-not-connected gap turns out to be,
  once specified.

## Permitted Files

Likely `src/sensors/SensorManager.cpp`, `BatteryAuthorityPolicy.cpp`/`.h`,
and their test files. Codex to confirm exact scope.

## Stage 4 — Independent investigation (Codex) + Claude verification

Full report: read-only investigation, no files changed. Codex confirmed
the core defect but corrected and substantially refined several parts of
the initial framing. Two of Codex's most load-bearing claims were
independently re-verified by Claude via direct source reading (both
confirmed): the split-write structure on Boron, and the network-standby
sleep exposure.

### Correction: the "write" is not one write

On Boron, the early `#else` branch (~line 743) writes only
`set_stateOfCharge(soc)`. `current.set_batteryState()` for Boron is a
**separate, later, unconditional write** at ~line 1109
(`current.set_batteryState(pmicBattState)`, using PMIC-derived state, not
the early `battState`) — confirmed directly by Claude. Any ordering fix
must be platform-aware and account for this split, not treat "the write"
as a single relocatable statement. Argon/M-SoM compile the two-field early
branch but do not compile the BQ24195 stale-SOC block at all.

### Option A ("reorder the write") is more involved than it sounds

`staleSocConditionsMet()`'s result currently only drives counters,
forensics, and the resync decision — it doesn't produce a value the
commit could just wait for. A real Option A needs:
1. The *immediate*, single-sample consistency result (not just the
   2-sample-debounced version) able to veto the commit — the debounce
   exists to gate *resync*, but the first inconsistent sample still needs
   to be vetoable for the *write*, or the same-cycle leak persists.
2. A defined answer for what to write when `quickStart()` fires this same
   cycle. Per the MAX17043 datasheet, an immediate post-`quickStart()`
   re-read isn't reliably fresh (first estimate assumes 30 min relaxation;
   this project's existing stabilization path already waits 500ms after
   `quickStart()` before re-reading). Two defensible sub-options, neither
   picked here: hold the previous accepted ledger value that cycle (no
   added latency, may retain a stale historical value), or a bounded
   settled re-read + revalidation (adds latency, only on the rare resync
   path, needs bench validation).

Confirmed low-risk: the intervening PMIC fault-remediation block
(~747-1090) reads fresh hardware state itself, not ledger SOC, so it
doesn't block reordering. One real side effect: `PowerDiagnostics::logPowerState()`
reads `current.get_stateOfCharge()` for its own logs in that same window —
deferring the write means those specific diagnostic lines would show the
previous cycle's SOC. Every caller of `batteryState()` discards its return
value, confirmed — not a constraint either way.

### Option B ("fast preliminary veto") has a real hardware hazard

The detection thresholds themselves (`vcellRestingHighWithLowSoc`,
`legacyChargingContextStaleSoc`) are not uniformly radio-gated — only the
*resync* trigger is (`quietForFuelGaugeResync`). So a same-cycle veto
should be radio-state-independent in principle. But at the actual write
point, PMIC fault/charge-status data (`faultReg`, `chargeStatus`,
`vbusStatus`, `powerGood`) isn't established yet — that happens later at
~line 833. Three options, each with a cost: use only a narrower SOC/Vcell
band (weaker coverage than the real check), read PMIC registers early too,
or hoist/share PMIC acquisition (approaches Option A's scope anyway).

**The middle option is not safe as a casual add**: the BQ24195's REG09
fault register is latching — TI's datasheet specifies two consecutive
reads are needed to distinguish latched-from-current fault status. An
extra premature read to support a "lightweight" veto could change which
fault the existing remediation block observes. This was not part of the
original framing and is a genuine reason to be cautious about Option B.

Additional Option B risks Codex flagged: a single-sample veto removes the
existing debounce protection from the trust decision specifically (not
just the resync decision); falling back to `previousKnownGoodSoc` isn't
itself independently validated as consistent; if the veto substitutes a
fallback value for `soc`, the later stale-SOC block might stop observing
the original inconsistent raw value, breaking its own debounce counting;
and reusing `rejectAuthoritativeOverwrite` for this new purpose is unsafe
since `_authoritativeBatterySoc` may be cleared (e.g. post-wake, no
authority baseline yet) — a genuinely distinct candidate/accepted/commit
state model is needed, not reuse of the existing flag.

### Codex's recommendation (not a decision)

Option A "in the semantic sense of one final evaluate-then-commit point,"
not a literal two-line move — centralizes one authority decision, avoids
duplicating PMIC-sensitive thresholds, avoids the REG09 double-read
hazard. Presented as a recommendation for Chip/Claude to weigh, not
implemented or unilaterally chosen.

### Radio-on/cloud-not-connected gap — refined, not just confirmed

The predicate hole is real (radio-on + cloud-disconnected satisfies
neither `sampleCanBeAuthoritative` nor the post-connect rejection
branch's `Particle.connected()` gate — raw SOC gets written unarbitrated).
But the *original* hypothesized scenario (mid-attach, i.e. `CONNECTING_STATE`
actively trying to reach the cloud) turns out **not to be reachable** in
current code — `batteryState()` is not called during that wait; its only
call is after `Particle.connected()` is already true
(`State_Connect.cpp:561`). That specific framing was narrow/theoretical.

**The real, repeatable exposure is different**: network-standby sleep.
`INACTIVE_STANDBY` (used in `INTERMITTENT_KEEP_ALIVE` mode) deliberately
keeps the cellular modem powered while disconnecting cloud during sleep
teardown (`Connectivity::requestCloudDisconnectOnly()`, confirmed directly
by Claude at `State_Sleep.cpp:~750`), and `batteryState(PreSleep)` is
called in exactly that radio-on/cloud-disconnected window
(`State_Sleep.cpp:1142`, also confirmed directly). This is a **designed,
regularly-occurring operating mode**, not a rare race — the gap matters
for a different reason than originally framed.

Closing it isn't fully subsumed by either ordering option: a
radio-independent consistency veto (under either A or B) closes the
threshold-inconsistency case, but a voltage-plausible-yet-implausibly-large
*jump* during this window is a separate concern needing its own explicit
policy (hold previous value / accept only intrinsically-corroborated
samples / maintain an authority baseline across standby wake) —
logically separate from write ordering, though a centralized Option A
commit point is the natural place to combine and test both together.

### Stuck-fast-charging detector — confirmed simple, one integration caveat

Confirmed straightforward and safe in the algorithmic sense (a rejected
candidate reverting toward the prior authority value doesn't typically
manufacture false "progress"; the fix is conservative — untrusted raw SOC
no longer *delays* a legitimate stuck-charge alert). Real caveat: the
existing `rejectAuthoritativeOverwrite ? _authoritativeBatterySoc : soc`
expression only covers the *old* post-connect mechanism — once this WO
adds a new same-cycle consistency veto, the detector needs to depend on
one unified "final accepted/committed SOC" defined once, not a third
repetition of a ternary that wouldn't yet reflect the new veto. Testing
nuance: "meaningful progress" is SOC gain **or** ≥15mV Vcell gain — a
rejected SOC sample with genuine Vcell rise can still legitimately reset
the window; a regression test needs to account for that OR clause rather
than assert rejection never resets progress.

## Status

Investigation complete (Stage 4). Both proposed mechanisms (reorder the
write / preliminary veto) are now understood in enough depth to choose
between responsibly — neither is as simple as originally framed, and
Option B carries a real hardware-semantics risk (BQ24195 REG09 is
latching) that wasn't part of the initial proposal. The radio-on/cloud-off
gap is confirmed real but for a different, more consequential reason
(network-standby sleep, not mid-attach) than originally hypothesized. The
stuck-charging detector fix is confirmed simple, with one integration
dependency on however the ordering fix defines "final accepted SOC."
**Awaiting Chip's review and decision between Option A / Option B (or a
hybrid) before this proceeds to Stage 5 approval.** No code has been
changed.
