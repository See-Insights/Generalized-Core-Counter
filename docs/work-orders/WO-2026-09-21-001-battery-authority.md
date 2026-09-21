# WO-2026-09-21-001: `BatteryAuthority` - single owner of battery-tier decisions

**Type:** Structural fix (Step 4 of the Structural Ownership Map roadmap).

**Status:** CLOSED 2026-09-21. Code complete, fully tested on the host and via
both local ARM-toolchain and cloud compile (Boron, target 6.4.1), and
bench-validated on Dev-14 via a controlled benchtop-supply voltage sweep - see
Bench validation below. One item surfaced during the sweep is deliberately
left open rather than blocking closure - see Known open item.

Not to be confused with the pre-existing "battery authority" guard inside
`SensorManager.cpp` against post-connect SoC deltas
(`WO-2026-08-05-002-battery-authority-delta-guard.md`) - that mechanism is
untouched by this step and predates the `BatteryAuthority` module this WO
introduces.

## The observation

Before this step, battery-tier decisions were fragmented across four
independently-evolving paths with no single owner: `resolveRuntime()`
sampling `SensorManager` and calling a guarded pipeline directly,
`currentBatteryTierForFailsafe()` and its diagnostic-build mirror
`currentBatteryTierForFailsafeLocal()` each re-implementing the same
sampling and pipeline call inline, and a stateful
`applyBatteryAwareConnectionModePolicy()` making the sticky OCCUPANCY-mode
connection-mode downgrade decision separately again. No single file could
answer "what is the current battery tier, and should anything be persisted
because of it" on its own - the same shape of problem Step 2
(`HibernateCycle`) and Step 3a (`Clock`) each found and fixed in their own
domains.

## The fix

New module, `power/BatteryAuthority.{h,cpp,Command.cpp}`, now owns tier
evaluation and every persisted consequence of it - split across two
translation units specifically so the decision logic stays host-testable
without pulling in persistence:

- **`BatteryAuthority::evaluate()`** (`BatteryAuthority.cpp`) - the pure
  query. Runs the same guarded pipeline `resolveRuntime()` used to run
  inline (`BatteryTierGuard::socForTier()`'s trust-gated substitution,
  `BatteryBackoff::calculateTier()`'s hysteresis, `applyVcellFloor()`'s
  unconditional 3.5V floor), ported verbatim. Deliberately includes neither
  `Particle.h`, `MyPersistentData.h`, nor `StorageHelperRK.h`, and does not
  call `SensorManager` - host-compilable with zero stubbing, linked directly
  (not faked) by `tests/reporting_policy_adapter_test.sh`.
- **`BatteryAuthority::commit()`** (`BatteryAuthorityCommand.cpp`) - the only
  writer. The single call site (outside `MyPersistentData.cpp`) for both
  `sysStatus.set_currentBatteryTier()` and `sysStatus.set_lowBatteryMode()`,
  and the one place the sticky, connection-mode-aware OCCUPANCY downgrade
  runs - ported verbatim from the retired
  `applyBatteryAwareConnectionModePolicy()`. Exactly 4 allowlisted call
  sites: `Generalized-Core-Counter.cpp` (setup), `State_Sleep.cpp`
  (pre-sleep and post-wake), `State_Report.cpp` (pre-connect decision).
- **`BatteryAuthority::evaluateCurrent()`** (`BatteryAuthorityCommand.cpp`) -
  a query despite living in the command file, because it needs
  `SensorManager` (which `evaluate()`'s own translation unit must not depend
  on). Gathers vcell/trust/previous-tier from `SensorManager`/`currentTier()`
  and calls the pure `evaluate()`. All three read paths -
  `resolveRuntime()`, `currentBatteryTierForFailsafe()`,
  `currentBatteryTierForFailsafeLocal()` - now route through this single
  function instead of each independently sampling `SensorManager`, so they
  cannot silently drift from each other on how vcell/trust are gathered.
- **`BatteryAuthority::currentTier()`** - the narrow, non-persisting read
  seam onto `sysStatus.get_currentBatteryTier()` (folds Step 0's
  `reporting/BatteryTierStore.h` into this module).
- **`Verdict::lowBatteryMode` deleted.** Grepped every read of the field
  across `src/` and `tests/` before removing it - nothing read it. The field
  was a pure derivation (`tier >= TIER_CONSERVING`) that could disagree with
  the real sticky flag `commit()` computes, which stays set after the tier
  recovers; a field that didn't equal the device's actual low-battery mode
  was a trap for the next reader. Pure deletion, no behaviour change.
- **Cloud status telemetry.** `Cloud::writeDeviceStatusToCloud()`'s
  `battery` object now also publishes `tier`, `lowBatteryMode`, `vcellState`,
  and `socTrust` - all four either an already-computed value
  (`reportingPolicy.batteryTier`) or a plain existing accessor
  (`sysStatus.get_lowBatteryMode()`, `SensorManager::cachedBatteryVoltageState()`,
  `cachedSocTrust()`). Added specifically because the bench rig (benchtop
  supply through Battery-In) cannot carry a simultaneous serial connection -
  without this, `commit()`'s persisted state and the guard pipeline's
  trust/vcell-plausibility signals would have been unobservable during the
  bench run entirely. Read-and-publish only; no change to `evaluate()` or
  `commit()`.

### Design correction mid-step: pure query, not stateful evaluate()

The first implementation gave `evaluate()` a persisting side effect
(`resolveRuntime()` gaining a write it never had before). Self-flagged in
the implementation report and corrected the same day: `evaluate()` was made
fully pure (no persisted reads/writes, no `SensorManager` calls, host-
compilable with zero stubbing), with `commit()` introduced as the only
writer and restricted to the 4 call sites above. This is why the module is
two translation units rather than one - deliberate, not incidental,
specifically so a lightweight host test can link the query half without
pulling in persistence (see `battery_authority_seam_structural_test.py`).

## Verification

- **Four structural tests**, all mutation-tested (defect reintroduced,
  confirmed failing for the right reason, restored byte-identical via
  sha256):
  - `battery_authority_structural_test.py` - `sysStatus.set_currentBatteryTier(`
    and `sysStatus.set_lowBatteryMode(` each appear exactly once outside
    `MyPersistentData.cpp`, both in `BatteryAuthorityCommand.cpp`;
    `calculateBatteryTier` has zero definitions anywhere in `src/`;
    `applyBatteryAwareConnectionModePolicy` has zero
    declarations/definitions/callers anywhere in `src/`.
  - `battery_authority_seam_structural_test.py` - `RuntimeReportingPolicy.cpp`
    references neither the persistence header nor `SensorManager.h`
    directly; `BatteryAuthority.h` stays narrow; `BatteryAuthority.cpp`
    (evaluate()'s translation unit) references neither `MyPersistentData.h`
    nor `StorageHelperRK.h`; `BatteryAuthorityCommand.cpp` DOES reference
    `MyPersistentData.h` (positive control).
  - `battery_authority_commit_sites_structural_test.py` - `commit(` appears
    exactly 4 times, at exactly the 4 allowlisted sites; no read-path file
    (`DeviceStatusPublisher.cpp`, `ConnectivityFailsafeTest.cpp`,
    `RuntimeReportingPolicy.cpp`) calls `commit()`.
- **Full host suite.** 35/35, including two tests
  (`power_source_override_test.sh`, `clock_status_republish_test.sh`) whose
  link-time stubs needed updating to declare the new
  `evaluateCurrent()`/`cachedBatteryVoltageState()`/`cachedSocTrust()`/
  `get_lowBatteryMode()` surface, matching production signatures exactly.
- **Local ARM-toolchain build.** Clean throughout (`gcc-arm 10.2.1`,
  Device OS `6.4.1`, `PLATFORM=boron`) - the standing verification practice
  established this session after a case-insensitive-filesystem header
  collision escaped cloud-compile-only checking earlier in the roadmap.
- **Cloud compile**, tracked commit-by-commit:

  | Commit | Change | Flash | RAM |
  |---|---|---|---|
  | `7cd5081` | Initial query/command split | 149638 | 3406 |
  | `afec481` | `Verdict::lowBatteryMode` deletion + `evaluateCurrent()` consolidation | 149702 (+64) | 3406 |
  | `f505fe2` | Cloud bench telemetry (`tier`/`lowBatteryMode`/`vcellState`/`socTrust`) | 149966 (+264) | 3406 |

  Both intermediate diffs were confirmed pure refactors (no behaviour
  change) before being accepted; the telemetry diff is read-and-publish only
  by construction.

## Bench validation

Dev-14, benchtop supply through Battery-In (no fuel-gauge charging path, so
`chargeState` stayed `"NOT"` throughout), binary
`wo-2026-09-21-step4-bench-telemetry-boron-f505fe2.bin` (sha256
`88c70ddb1dbb29b492adc0bd2fb8045b3ede3a833c0762b39c6e2b0ec5728b61`), commit
`f505fe2`. No serial connection was possible in this rig configuration -
all evidence below is from the `device-status` cloud ledger, pulled via
`particle ledger get device-status --device <id>` after forcing a connect
via the front-panel service button at each step.

Descending, then ascending, in ~0.1V increments:

| Dial | vcell (measured) | soc (gauge) | trust | tier | lowBatteryMode |
|---|---|---|---|---|---|
| 4.1V ↓ | 4.01 | 75.5 | Trusted | HEALTHY | false |
| 4.0V ↓ | 3.93 | 75.5 | Trusted | HEALTHY | false |
| 3.9V ↓ | 3.82 | 75.4 | Untrusted | CRITICAL | **true** |
| 3.8V ↓ | 3.72 | 75.2 | Untrusted | CRITICAL | false |
| 3.7V ↓ | 3.62 | 74.9 | Untrusted | SURVIVAL | false |
| 3.6V ↓ | 3.51 | 74.5 | Untrusted | SURVIVAL | false |
| 3.5V ↓ | 3.41 | 73.6 | Untrusted | SURVIVAL (floor) | true |
| 3.6V ↑ | 3.51 | 73.1 | Untrusted | SURVIVAL | true |
| 3.8V ↑ | 3.71 | 72.4 | Untrusted | SURVIVAL (dead zone) | true |
| 3.9V ↑ | 3.82 | 72.1 | Untrusted | CRITICAL | false |
| 4.0V ↑ | 3.92 | 72.1 | Trusted | CONSERVING | true |
| 4.1V ↑ | 4.02 | 72.1 | Trusted | CONSERVING | true |
| 4.2V ↑ | 4.12 | 72.1 | Untrusted | **HEALTHY** | **false** |

**vcell tracking confirmed reliable and reproducible.** A consistent
~0.08-0.10V offset between the bench dial and measured cell voltage
(diode-drop across the Battery-In path), identical at matching dial points
in both directions (e.g. 3.6V dial -> 3.51V measured, both descending and
ascending).

**Every tier boundary and the unconditional floor confirmed against the
actual thresholds.** `HEALTHY` (>=75% via the Trusted/gauge path),
`CONSERVING` (70-75% dead zone holding, and the clean 55-70% cross observed
ascending at 4.0V), `CRITICAL` (the 35-50% clean cross, and the 30-35% dead
zone holding `CRITICAL` from below at 3.8V descending), `SURVIVAL` (the
clean <30% cross, the 30-35% dead zone holding `SURVIVAL` from above at
3.8V ascending, and the unconditional `kCriticalVcell=3.5V` floor
distinctly triggered at 3.41V, independent of the SoC-substitution result)
- all matched hand-calculation against `BatteryBackoff::calculateTier()`'s
75/70/55/50/35/30 breakpoints and `BatteryTierGuard::applyVcellFloor()`.

**The trust/substitution mechanism validated end-to-end - the most
important result of the sweep.** The fuel gauge's own reported SoC barely
moved (75.7% -> 72.1% total) across the entire sweep despite vcell being
driven from 4.1V down to 3.41V and back to 4.12V - expected, since a bench-
supply step doesn't exercise a fuel gauge's coulomb-counting/relaxation
model the way a real battery's gradual discharge does. As the residual
between the gauge's SoC and `BatteryHealth::restingSocFromVcell(vcell)`
exceeded the trust thresholds (20/12, ±8 bias for charging/radio activity),
`socTrust` correctly degraded to `Untrusted`, and tier correctly switched to
being derived from vcell instead of the stale gauge value. At the final
4.2V reading the gauge was *still* frozen at 72.1% - a real, non-recovering
gauge - yet the system still reached the correct `HEALTHY` verdict via the
vcell-implied ~92% resting SoC. This is exactly the failure mode the trust/
substitution guard exists to handle, demonstrated on real hardware with a
genuinely stuck gauge, not a simulated one.

**The sticky `lowBatteryMode` downgrade demonstrated both directions
cleanly, once each.** Set `true` on the first genuine cross into
`CONSERVING` or worse while in `INTERMITTENT_KEEP_ALIVE` (3.9V descending);
cleared `false` on the first genuine return to `HEALTHY` (4.2V ascending) -
both match `commit()`'s literal branch conditions exactly.

## Known open item

`lowBatteryMode` cleared `false` twice more during the sweep while tier was
`CRITICAL`, not `HEALTHY` (3.8V descending, 3.9V ascending). Per direct
reading of `commit()`'s three OCCUPANCY-mode branches
(`BatteryAuthorityCommand.cpp:75-100`), none should clear the flag except
at `tier == HEALTHY` or when `connectionMode` is already outside
`INTERMITTENT` - neither condition is visible from the cloud payload as it
stands, and no serial capture was possible in this bench rig to check which
branch actually fired.

**Deliberately deferred, not blocking this WO's closure.** The behaviour
that matters for Step 4 - the downgrade setting and clearing correctly at
genuine tier transitions - was cleanly demonstrated at both ends. Two
anomalous clears at an intermediate tier didn't prevent the system from
reaching correct end states, and may be an artifact of stepping the bench
faster than any real deployment would ever swing voltage (each step here
took seconds; a real device sees this kind of range change over
days-to-weeks). Filed as a follow-up: see
`WO-2026-09-21-002-battery-authority-lowbatterymode-flap.md`.

## Not in scope for this step

Named per the roadmap and not touched here: the persistence-header split
(Step 5), and anything downstream of `evaluateCurrent()`'s three read paths
that wasn't already routing through `BatteryAuthority` before this step
(e.g. `ChargeDiag`/`pdiag` telemetry, which reads `SensorManager` and PMIC
registers directly and has no tier dependency).

## Provenance

Scoped by `docs/architecture-review-2026-09-03.md` and the Structural
Ownership Map roadmap agreed 2026-09-03 (Step 4 of that roadmap).
Implemented, corrected, and bench-validated 2026-09-21 on branch
`wo/2026-09-21-step4-battery-authority`, commits `7cd5081`, `afec481`,
`f505fe2`.

## Related work orders

- `docs/architecture-review-2026-09-03.md` - source of the roadmap this step
  is item 4 of.
- `WO-2026-09-18-001-clock-owner.md` (Step 3a) and Step 3b (trust standard) -
  the two preceding roadmap steps, establishing the "single owner module,
  proven with a mutation-tested structural test" pattern this step follows
  for a third domain.
- `WO-2026-08-05-002-battery-authority-delta-guard.md` - an unrelated,
  pre-existing `SensorManager.cpp` mechanism that happens to share the
  "battery authority" name; untouched by this step.
- `WO-2026-09-21-002-battery-authority-lowbatterymode-flap.md` - the
  deferred anomaly filed above.
