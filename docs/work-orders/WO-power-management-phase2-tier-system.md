# WO: Power Management Consolidation Phase 2 — unified battery tier system

## Context

Phase 1 (WO-2026-08-06-001) wired up `PowerManager` as the live call path
for SOC/battery-state reads across the codebase. This WO addresses what
Phase 1 deliberately left alone: the actual *decisions* driven by battery
state remain scattered and inconsistent. The original investigation
(`docs/architecture/power-management-consolidation-findings.md`) found a
concrete example — a 60%-SOC device gets reporting-throttled by one
threshold while still receiving full-length connection attempts from a
completely unrelated one — and found sleep-mode selection takes zero
battery input at all.

This WO replaces that scattered decision-making with one unified,
ledger-configurable battery tier system, with sleep mode, counting
behavior, and connectivity level all reading from the same source of
truth.

## Design (settled through discussion, not open for re-litigation without cause)

### Tier table

| Tier | Trigger | Sleep Mode | Counting | Connectivity |
|---|---|---|---|---|
| Outside park hours | Any charge level | `HIBERNATE` | Off | Normal (existing behavior, unchanged) |
| Full charge | Park open, charge ≥ ledger threshold | `ULP` | On (interrupt-based sensors) | Full (1×) |
| Low charge | Park open, charge < ledger threshold | `ULP` | On | Reduced (½×) |
| Very low charge | Park open, charge < ledger threshold | `ULP` | On | Significantly reduced (¼×) |
| **Critical** | **Vcell ≤ 3.5V — overrides park-hours schedule** | `HIBERNATE` | Off | Check-in only, fixed local times: 8am / noon / 4pm |

### Key decisions, with rationale

- **Critical tier is gated on Vcell, not SOC%.** Direct consequence of the
  WO-2026-08-05-002/003 investigation: SOC is the value confirmed capable
  of desyncing, jumping, and lagging reality; Vcell is the direct physical
  measurement, already available live via `PowerManager`. Not open for
  reconsideration without new evidence — this is the one settled,
  non-negotiable piece of this design.
- **3.5V specifically** (not 3.3V, the lower end of the cited safe-stopping
  range) — chosen deliberately for margin, since the device only checks in
  at fixed times rather than continuously monitoring; discharge between
  checks could otherwise cross into the 3.0V danger threshold before the
  next opportunity to detect it.
- **Critical overrides park-hours scheduling**, not just battery-level
  decisions within open hours — multi-week critical periods (severe
  weather, extended cold) shouldn't wait for park-open to resume charge
  monitoring.
- **8am/noon/4pm are local time, per device's own deployment location**,
  not UTC or a fixed reference timezone — solar/thermal recovery logic
  only makes sense against each device's own local morning/midday/
  afternoon. Explicit design requirement, not an implementation detail to
  default silently.
- **Threshold values (Full/Low/Very-Low boundaries, and the Reduced/
  Significantly-Reduced multipliers) live in the ledger**, tunable with
  fleet experience rather than hardcoded, and overridable per-device for
  known-atypical deployment sites (e.g. a consistently cold, low-sun
  mountain-top location). Reasonable defaults still need to be proposed
  for initial ledger values — not zero-configuration, just tunable after
  the fact.
- **Reduced connection *duration* is explicitly deferred, not part of this
  WO.** Particle recommends against shortening connection attempts,
  because it can prevent the SARA modem from completing a full
  try/retry/reset cycle. This isn't just external guidance — it's
  consistent with this project's own NCP/watchdog investigation
  (WO-2026-08-10-001 and follow-ups), which found `CloudRecover` routinely
  needs a large fraction of the connect budget to work through slow modem
  cold-starts (up to ~88-90s observed) and usually — but not always —
  succeeds within budget. Shortening that budget specifically during
  low-battery states risks cutting a legitimate recovery off mid-attempt,
  right when a successful connection matters most. Revisit only with a
  concrete design that accounts for this risk, as its own follow-up WO.
- **`ULP`/`NOT-COUNT` is kept**, not discarded, for sensor types that poll
  frequently but report infrequently (e.g. a water sensor testing every 15
  minutes, reporting every 4 hours) — consistent with this being a
  generalized codebase, not Boron-Dev-09-specific.
- **`HIBERNATE`/`COUNTING` is not a supported combination** — an
  interrupt-based sensor in `HIBERNATE` resets on every trigger, defeating
  the purpose of `HIBERNATE`'s reduced-reset design intent.

## Investigation needed before implementation

1. Locate every currently-scattered battery-threshold decision point (the
   findings doc's original inventory should mostly cover this, but
   reconfirm current state given time has passed and other WOs have
   touched adjacent code) — reporting-interval logic, connection-depth
   logic, sleep-mode selection, and confirm no other undiscovered
   decision points exist.
2. Confirm the existing per-device timezone/location data source — is
   local time already computed and available (the serial logs throughout
   this investigation have consistently shown `TimeDiag:` lines with both
   `utc=` and `local=` — confirm this is derived from a real per-device
   config, not a fixed build-time constant) for the 8am/noon/4pm check
   logic to use directly.
3. Propose the actual ledger schema additions needed (tier thresholds,
   per-tier connectivity multipliers, per-device override mechanism) and
   how they interact with existing ledger fields already in use elsewhere
   in this codebase (`deviceSettingsLedgerData`, etc., referenced
   throughout this investigation's earlier findings).
4. Propose reasonable default threshold values for initial ledger
   population — informed by whatever real SOC/Vcell field data exists so
   far (the bench soak data collected this week, and any fleet history),
   not picked arbitrarily.
5. Confirm how `HIBERNATE`/`ULP` selection actually gets wired to read
   from the new tier system, and how this interacts with the existing
   overnight/park-closed `HIBERNATE` path (already working, per the
   overnight soak validation earlier this investigation) — this WO adds a
   *second* trigger for `HIBERNATE` (critical battery), and the two must
   not conflict or double-fire.

## Acceptance Criteria

- A single, unified tier-resolution function/mechanism is the sole source
  of truth for sleep mode, counting behavior, and connectivity level —
  no decision point reads its own independent threshold.
- Critical-tier trigger uses Vcell, confirmed end-to-end (not silently
  falling back to SOC anywhere in the implementation).
- 8am/noon/4pm check-ins use each device's own local time, verified
  against at least one non-default-timezone test case.
- Existing overnight/park-closed `HIBERNATE` behavior is unaffected by
  the new critical-battery `HIBERNATE` trigger — both coexist without
  conflict.
- Ledger-configured thresholds are actually read from the ledger at
  runtime, not hardcoded with the ledger as unused scaffolding.
- `ULP`/`NOT-COUNT` remains available and functional for non-interrupt
  sensor configurations.

## Required Tests

- Tier resolution at every boundary (exact threshold values, both sides).
- Critical-tier trigger fires on Vcell alone, confirmed not to
  accidentally also depend on SOC.
- Critical-tier correctly overrides an in-progress park-hours schedule,
  not just gates new decisions.
- 8am/noon/4pm timing correct across at least two different device
  timezones.
- No double-fire or conflict between overnight-`HIBERNATE` and
  critical-`HIBERNATE` triggers.
- Ledger threshold changes actually take effect without requiring a
  reflash.
- `HIBERNATE`/`COUNTING` combination is structurally prevented or
  explicitly rejected, not silently allowed to misbehave.

## Permitted Files

TBD pending investigation item 1 — likely spans `PowerManager`, whatever
currently implements reporting-interval/connection-depth logic, sleep-mode
selection code, and ledger schema/config files. Codex to confirm exact
scope.

## Status

Design fully settled through discussion — ready for Codex investigation
into the five items above, then standard reconcile → approve → dispatch
loop. Explicitly not scoped for implementation before Friday's flash —
this is dispatch-ready groundwork for after the vacation break, not a
rushed pre-departure change. Reduced connection duration explicitly out
of scope, deferred to its own future WO.
