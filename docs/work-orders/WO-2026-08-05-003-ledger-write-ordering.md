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

## Status

Drafted directly from Codex's investigation findings, already independently
verified by the project author via direct source reading. Ready for a
scoped investigation into the exact ordering-fix mechanism and the
radio-on/cloud-not-connected gap specifics, following the same
investigate → reconcile → approve → dispatch loop used for
WO-2026-08-05-002.
