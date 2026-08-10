# Power Management Consolidation — vision doc (not yet a Work Order)

Status: captured for later scoping, explicitly sequenced AFTER
WO-2026-08-05-002 and WO-2026-08-05-003 ship. Not to be folded into either.

## Motivation

WO-2026-08-05-002/003 fixed a real bug, but the investigation exposed a
structural problem the fix doesn't address: power/battery-related logic
(SOC estimation, charge-state, PMIC fault handling, operational policy,
corrective actions) is scattered across `SensorManager.cpp` with multiple
independent read sites, no single owner, and at least one confirmed hazard
(the BQ24195's latching fault register) where uncoordinated reads from
different code paths can interfere with each other.

## Proposed structure: four functions, layered

1. **Best SOC estimate** — accounts for post-sleep warm-up/settle time,
   includes its own plausibility check (the Vcell-vs-SOC consistency logic
   from WO-2026-08-05-002/003) so it can self-correct rather than just
   report a possibly-bad value. Single canonical source of truth for "what
   is the SOC right now" — nothing else in the codebase should read the
   fuel gauge directly.
2. **Battery state** — Charging / Discharging / Charged / Charging
   Disabled (thermal) / Unknown. Owns PMIC status/fault-register reads.
   Whether thermal charge-disable logic lives here or in the corrective-
   actions function is an open call, flagged for the eventual WO rather
   than decided now.
3. **Supported operations** — a pure decision layer, no hardware I/O of
   its own, consuming (1) and (2)'s outputs: can the device support
   network-attached sleep? does the reporting interval need adjusting?
   full vs. abbreviated connection attempt? stop-and-preserve-critical?
   This is the one genuinely new consolidation — the logic exists today,
   but scattered and implicit rather than centralized.
4. **Corrective actions** — charge-fault detection, PMIC reset, device
   reset, ledger alerting. The `quickStart()`-resync mechanism from
   WO-2026-08-05-002/003 becomes an implementation detail inside this
   function rather than logic embedded in `SensorManager.cpp` directly.

Each layer should only depend on the layers below it — (3) and (4) consume
(1) and (2)'s outputs but don't reach around them to read hardware
directly. That keeps each layer independently testable and keeps the
"which function do I touch to change X" question unambiguous.

## Explicit simplification: drop live post-connect sag measurement

The pre-radio/post-connect delta comparison built in WO-2026-08-05-002
exists to filter a transient connect-current artifact. Once the SOC-vs-
Vcell consistency check exists as a general plausibility mechanism, this
specific transient-artifact filtering is likely redundant — operational
decisions (function 3) should run off the most recent *trusted* estimate
rather than trying to freshly sample and filter mid-connect. A genuinely
failing battery shows up as a persistent problem across wake cycles, which
function 4's corrective-action logic (built on function 1's plausibility
checks) is already positioned to catch, without needing fine-grained
in-connect voltage monitoring. Worth confirming during scoping whether the
existing pre-radio/post-connect machinery can be retired in favor of this
simpler model, rather than carried forward as a second, parallel mechanism.

## Platform awareness

Devices with a PMIC (Boron) support the full set of functions above.
Devices without one (Photon2/P2, per the WO-2026-08-05-002 audit) need
graceful fallback — likely function 2 always reports a reduced state set,
and functions 3/4 have narrower scope. Exact fallback behavior TBD at
scoping time, not decided here.

## Explicitly out of scope for now

- No implementation, acceptance criteria, or test requirements yet — this
  needs a proper scoping pass (as its own WO or WOs) before any code
  changes.
- Whether this becomes one new module/class or several is not decided.
- Whether the existing pre-radio/post-connect machinery gets retired,
  kept as a secondary guard, or something else is a scoping-time decision,
  not made here.
