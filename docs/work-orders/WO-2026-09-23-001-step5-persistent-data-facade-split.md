# WO-2026-09-23-001: Step 5 — split MyPersistentData.h by concern

**Status:** **CLOSED — PASS** (2026-09-25). Merged to `main` via PR #42
(`8163295`). Dev-14 bench validation passed; see "Bench validation result"
at the end of this document.

**Prior status:** APPROVED (Stage 5, 2026-09-23) — conditional approval satisfied:
WO-2026-09-23-002 (the unrelated `serial_settle_test.py` regression) merged
clean to `main` at `3fff587`, full 38-test host suite (21 `.sh` + 17 bare
`.py`) confirmed green on that commit. Ready for Stage 6 (Copilot
implementation). Codex's independent Stage 4 investigation materially
expanded scope beyond the original Stage 3 draft below (kept intact for the
record — see "Stage 4: Codex findings and reconciliation" for what changed
and why); those findings are now part of the approved architecture, not
open questions.

**Workflow role:** Produced by Claude (Architect) per `AI_DEVELOPMENT_WORKFLOW.md`.
Stage 4 (Codex independent investigation) and Stage 5 (Chip approval) are
both complete — see the Approval record below. **This WO is now approved and
ready for Stage 6.** Claude does not implement.

**If you are Copilot reading this to implement it:** you are the Implementer
under `AI_DEVELOPMENT_WORKFLOW.md` — read that file's "GitHub Copilot —
Implementer" section before starting if you have not already. In summary:
work only from this approved WO; do not expand scope or change the
architecture unilaterally (stop and report back instead if you find you
need to); add/update tests as required; run the build/test commands this WO
specifies; **do not commit, push, merge, or release** — leave a complete,
uncommitted working-tree diff for Chip and Codex to review; end with an
Implementation Report (files changed, behavior changed, tests run and
results, deviations or concerns). **You must stop at the checkpoint gate in
§10 before touching any of the 27 consumers** — that gate is not optional.

## 1. Problem statement

`MyPersistentData.h` bundles five unrelated concerns behind one header: system
mode/schedule/webhook config, failsafe/watchdog forensics, power/thermal
config, current sensor/occupancy readings, and the `StorageHelperRK.h`
storage-layer dependency itself. Every one of its 23 direct includers pulls in
all five concerns and the storage-layer dependency, regardless of which (if
any) it actually uses. This is Step 0's root problem recurring at header
granularity: consumers that need one field are structurally coupled to
`StorageHelperRK.h` and to every other consumer's fields, which is what has
been producing one narrow test-stub exception at a time (most recently the
`power_source_override` stub gap this session found and fixed).

## 2. Operational impact

No runtime defect — this is a structural/maintainability change. Impact is on
test-stub fidelity risk (six stub directories each hand-maintain a shadow of
the full interface) and on the cost of every future change that touches
`MyPersistentData.h`, which currently forces a rebuild of all 23 includers.

## 3. Scope

- Repository: `Generalized-Core-Counter`, branch off current `main` (confirmed
  post-punch-list: `main` is at `efe0e4c`, includes PR #37 and PR #38).
- Files: `src/MyPersistentData.h`/`.cpp`, four new `src/persist/*.h` facade
  headers, all 23 direct includers, six `tests/stubs/.../MyPersistentData.h`
  stub variants.
- No device/Fleet operations. No layout change to the persisted
  `SysData`/`CurrentData` structs — facades only.

## 4. Evidence (fresh, re-verified 2026-09-23 — do not reuse prior snapshot)

### 4.1 Fan-in: confirmed 23, unchanged from snapshot

```
$ grep -rl "MyPersistentData\.h" src/ | wc -l
23
```

23 files reference `MyPersistentData.h` (including relative-path form
`"../MyPersistentData.h"` in `observability/StartupSnapshotRuntime.cpp`,
missed by a naive exact-string grep). This count includes `MyPersistentData.cpp`
itself — 22 external consumers + the owner.

Two apparent hits, `power/BatteryAuthority.cpp` and `power/BatteryAuthority.h`,
are **false positives**: both only *mention* `MyPersistentData.h` in a comment
explaining that the Query half of `BatteryAuthority` deliberately does **not**
depend on it (CQS split from an earlier step). Confirmed via direct inspection
— neither file has a real `#include`.

Full includer list (23):

| # | File | Distinct accessors used |
|---|------|--------------------------|
| 1 | `src/MyPersistentData.cpp` | (owner) |
| 2 | `src/Config.cpp` | 9 |
| 3 | `src/Generalized-Core-Counter.cpp` | 50 |
| 4 | `src/ThrashGuard.cpp` | 1 |
| 5 | `src/cloud/Cloud.h` | 0 |
| 6 | `src/cloud/Particle_Functions.cpp` | 0 |
| 7 | `src/diagnostics/ConnectivityFailsafeTest.cpp` | 7 |
| 8 | `src/observability/StartupSnapshotRuntime.cpp` | 1 |
| 9 | `src/power/BatteryAuthorityCommand.cpp` | 7 |
| 10 | `src/power/PmicFaultMonitor.cpp` | 6 |
| 11 | `src/power/PowerDiagnostics.cpp` | 0 |
| 12 | `src/power/PowerManager.cpp` | 3 |
| 13 | `src/power/PowerPlatform.cpp` | 0 |
| 14 | `src/sensors/PIRSensor.h` | 0 |
| 15 | `src/sensors/SensorManager.cpp` | 13 |
| 16 | `src/state/State_Common.h` | 7 |
| 17 | `src/state/State_Connect.cpp` | 13 |
| 18 | `src/state/State_Error.cpp` | 6 |
| 19 | `src/state/State_Idle.cpp` | 8 |
| 20 | `src/state/State_Modes.cpp` | 14 |
| 21 | `src/state/State_Report.cpp` | 13 |
| 22 | `src/state/State_Sleep.cpp` | 22 |
| 23 | `src/time/Clock.cpp` | 3 |

### 4.2 Zero-accessor claim: confirmed, unchanged

`cloud/Cloud.h`, `cloud/Particle_Functions.cpp`, `power/PowerDiagnostics.cpp`,
`power/PowerPlatform.cpp`, `sensors/PIRSensor.h` — grepped for any
`sysStatus.`/`current.`/`sensorConfig.` usage, zero real hits in all five (the
two textual hits in `Cloud.h` and `PowerPlatform.cpp` are comments). **Not yet
determined why these include it at all** — dead includes vs. an implicit
transitive need (e.g. a macro, a forward-declared type, or a build-order
dependency) is unresolved and must be checked per-file before deciding each
one needs zero facades, per the dispatch's own instruction.

### 4.3 One-accessor claim: confirmed, unchanged

`ThrashGuard.cpp` — `current.raiseAlert()`, 2 call sites, 1 distinct accessor.
`observability/StartupSnapshotRuntime.cpp` — `sysStatus.get_resetCount()`, 1
call site.

### 4.4 StorageHelperRK.h fan-in: 1 today (the header, not yet the .cpp)

```
$ grep -rl "StorageHelperRK.h" src/
src/MyPersistentData.h
```

Today, `StorageHelperRK.h` is included by `MyPersistentData.h` (the header).
The target state (done condition 2) requires it be included by
`MyPersistentData.cpp` only. This is a real, required change — not already
true — and is the mechanism that actually closes Step 0's seam for every
facade consumer.

## 5. Proposed architecture

Confirmed against the fresh accessor inventory in 4.1: **the four-way split
from the ownership map's Q5 section does not fully cover the field set.** Two
boundary questions surfaced that this WO does not resolve (per the dispatch's
explicit instruction to name, not resolve, boundary questions found
mid-implementation):

**Open question A — alert/forensics fields live on `current`, not `sysStatus`.**
`current.get_alertCode()` / `set_alertCode()` / `raiseAlert()` /
`get_lastAlertTime()` / `set_lastAlertTime()` are physically part of
`CurrentData` (accessed via the `current` object), but semantically they are
forensics/alert-tracking, the same concern `RecoveryState.h` is named for
(which otherwise only covers `sysStatus` fields: failsafe stage/count,
watchdog forensics). Used by 6 of the 23 includers
(`Generalized-Core-Counter.cpp`, `PmicFaultMonitor.cpp`, `State_Common.h`,
`State_Connect.cpp`, `State_Error.cpp`, `State_Report.cpp`). A facade can
legally expose accessors that call into both underlying objects (facades are
a header-level grouping, not new storage), so this is answerable either way —
it just needs a decision: does `RecoveryState.h` cross into `current`'s alert
fields, or does `CurrentReadings.h` absorb them despite the "sensor/occupancy"
framing not obviously covering "alert code"?

**Open question B — connectivity/connection-budget fields have no home in the
four named facades.** `connectAttemptBudgetSec`, `cloudDisconnectBudgetSec`,
`modemOffBudgetSec`, `connectionAttemptCounter`, `lastConnection`,
`lastConnectionDuration`, `serialConnected` are all `sysStatus` fields used
heavily by `State_Connect.cpp`, `State_Sleep.cpp`, `State_Idle.cpp`,
`State_Report.cpp`, `ConnectivityFailsafeTest.cpp` — none is "mode, schedule,
webhook config" (SystemConfig), "failsafe stage, counts, watchdog forensics"
(RecoveryState), or "thermal thresholds, solar, tier, low-battery" (PowerConfig).
This looks like a fifth cluster (connectivity/connection-budget config) that
the Q5 section's four-way split didn't anticipate, or these fields need to be
folded into `SystemConfig.h` under a broadened definition of "schedule."

Everything else in the fresh accessor inventory maps cleanly onto the four
named facades as originally proposed.

## 6. Risks / tradeoffs

- **Two unresolved facade-boundary questions (5.A, 5.B)** must be settled
  before or during implementation — they affect which facade several
  high-traffic files (`State_Connect.cpp`, `State_Sleep.cpp`,
  `Generalized-Core-Counter.cpp`) end up including, and getting them wrong
  means a second pass.
- **5 zero-accessor includers' actual reason for the include is unconfirmed**
  — risk of silently dropping a transitive dependency (e.g. a type used only
  by name, a macro) if the include is removed without checking why it was
  there.
- Standard facade-split risk: a missed conversion among 23 includers compiles
  clean against the old monolithic header (if any include path is left
  unchanged) and only surfaces as a fan-in-count structural-test failure, not
  a compile error — the done conditions already require confirming each
  includer specifically for this reason.

## 7. Acceptance criteria (from the dispatch, unchanged)

1. Structural test: `MyPersistentData.h` has at most a small allowlist of
   includers (ideally just `MyPersistentData.cpp` and the four facade
   headers), mutation-tested.
2. Structural test: `StorageHelperRK.h` has exactly one includer
   (`MyPersistentData.cpp`), mutation-tested.
3. Each of the 23 original includers confirmed individually to compile
   against its new, narrower facade(s).
4. Full host suite green, including all six re-pointed stub directories.
5. Local ARM-toolchain build clean, cloud compile clean, flash/RAM reported
   against current baseline (expect size-neutral; investigate if not).
6. No layout change: persisted file magic/version bytes unchanged, stored
   data reads back identical on a real device.
7. Bench validation: Dev-14, one full day, stored files read back identical,
   no re-init, no data loss.

## 8. Non-goals

- No layout change to `SysData`/`CurrentData` (Step 7).
- No resolution of open questions 5.A/5.B beyond flagging them — Codex and
  Chip decide.
- No improvement to anything else noticed mid-implementation.
- Step 5.5 and Step 6 do not begin as part of this WO.

## 9. Rollback

Revert the facade-split commit(s); no persisted-data migration involved since
layout is unchanged, so rollback is a pure source revert with no device-side
consequence.

## 10. Recommendation (superseded — see "Stage 4" section below for what actually happened)

~~Route to Codex (Stage 4, independent investigation) before Copilot
implementation begins.~~ Done — see Stage 4 section. **Current
recommendation (post-Stage-5 approval): proceed directly to Copilot for
Stage 6 implementation.** No second pre-implementation Codex investigation
round is needed — Stage 4 already happened, found real scope gaps, and
those findings (1-6), both open-question resolutions (A, B), and the
zero-accessor resolutions are now part of this WO's approved architecture,
not open questions for Codex to re-litigate. Codex's next role is Stage 7:
reviewing Copilot's actual uncommitted diff against this WO, same as
WO-2026-09-23-002's three-round implement/review cycle.

### Checkpoint gate (required — not optional, not a suggestion)

Before converting **any** of the 27 consumers, Copilot must produce the four
facade headers' complete public interface — declarations only, no
implementation — and **stop for review** before proceeding further. Each
facade header's checkpoint deliverable must show:

- Every declaration it exposes (free functions, or a class/namespace shape —
  Copilot's call, but state it explicitly and consistently across all four).
- **Every `#include` it has**, with no exceptions. This is the specific
  thing being gated: Codex's Finding 1 was that a facade including a shared
  persistence header (or transitively re-exposing `StorageHelperRK.h`)
  would silently reintroduce the exact coupling this entire step exists to
  remove — and that failure mode is invisible by inspection of any single
  facade in isolation, cheap to catch here, and expensive to catch after 27
  files already depend on the wrong shape.
- How each resolves the two decided open questions concretely: where
  `RecoveryState.h`'s alert/forensics declarations forward into (still
  `CurrentData`, per decision A), and `SystemConfig.h`'s full field list
  under its broadened definition (per decision B).
- How `ConfigApply.cpp`'s capacity constants and `validate()` call are
  exposed (Finding 2) — not deferred to the implementation pass.
- Where `sensorConfigData`/`SensorData`'s fields land (Finding 3), keeping
  the two distinct `sensorType` fields distinct.

Do not proceed to converting the 27 consumers until this checkpoint has been
reviewed and explicitly approved. This is the single highest-leverage point
to catch a wrong architectural guess in a step that has already grown twice
during evidence-gathering alone.

## Approval record

- [x] Codex independent investigation (Stage 4) — 2026-09-23, checked out
      `72b15fb` (tracked tree identical to stated baseline `efe0e4c`), no
      repository changes made.
- [x] Chip approval (Stage 5) — 2026-09-23. A and B decided (see below).
      Conditional on WO-2026-09-23-002 landing clean first — condition met:
      merged to `main` at `3fff587`, full 38-test suite confirmed green.
- [x] Copilot implementation (Stage 6) — 2026-09-23, `97748b8`.
- [x] Codex verification (Stage 7) — 2026-09-23. Two "Not verified" regression
      guards fixed and re-verified clean (detail in `97748b8`'s message).
- [x] Chip final gate / commit (Stage 8) — merged 2026-09-25 04:59Z, PR #42
      (`8163295`), after the Dev-14 bench validation below.

## Stage 4: Codex findings and reconciliation

Codex's recommendation: **revise before Stage 5 approval.** Claude
independently re-verified the four most consequential findings below (marked
✅ CONFIRMED) by direct repository inspection; all four held up exactly as
reported. The remainder were not independently re-checked but are accepted
on the strength of Codex's stated methodology (host compile probes, actual
test runs) and the track record of the confirmed subset.

### Finding 1 — backend separation needs an explicit design (not just "move the include")

All three persistence classes (`sysStatusData`, `currentStatusData`,
`sensorConfigData`) inherit from `StorageHelperRK::PersistentDataFile` and
embed its header type directly. Moving the `#include` alone cannot satisfy
"facades don't include StorageHelperRK.h" — the concrete classes themselves
must live inside `MyPersistentData.cpp` (or a header only `.cpp` sees), with
the four facade headers exposing only narrow public declarations, not the
storage-backed class definitions. **Revises the original architecture** from
"just relocate one include" to "the facade headers forward-declare/narrow the
public interface; the storage-coupled class definitions move out of the
public header entirely."

### Finding 2 — fan-in is 23 direct + 4 transitive, not 23 total ✅ CONFIRMED

`Cloud.h` includes `MyPersistentData.h` and is itself included by `Cloud.cpp`,
`ConfigApply.cpp`, `DeviceStatusPublisher.cpp`, and `LedgerClient.cpp` — none
of which directly include `MyPersistentData.h`, so the original grep-based
fan-in count (correctly measuring *direct* inclusion) missed them entirely.
Verified directly:

| File | Distinct accessors used (via `Cloud.h` transitively) |
|---|---|
| `cloud/Cloud.cpp` | 9 |
| `cloud/ConfigApply.cpp` | 55 |
| `cloud/DeviceStatusPublisher.cpp` | 31 |
| `cloud/LedgerClient.cpp` | 2 |

`ConfigApply.cpp`'s 55 exceeds every direct includer in the original table,
including `Generalized-Core-Counter.cpp`'s 50 — this is the single largest
real consumer, and it was entirely absent from the Stage 3 evidence. **This
is a gap in Claude's own Stage 2 evidence pass**: the fan-in question needs
to be "effective access," not "direct `#include`," and the true scope is 27
files needing conversion attention, not 23.

`ConfigApply.cpp` additionally uses concrete storage types to size
timezone/webhook buffers and calls `validate(sizeof(sysStatus))` directly —
passing `sizeof()` of a new facade type would silently change validation
behavior (a different struct size). This needs public capacity constants and
a backend validation operation exposed through the facade, not a `sizeof()`
on whatever the facade type turns out to be.

### Finding 3 — a third persisted record is missing from the preservation contract ✅ CONFIRMED

`sensorConfigData::SensorData` (backing `/usr/sensor.dat`) is a third
`StorageHelperRK::PersistentDataFile`-backed store, structurally identical in
kind to `SysData`/`CurrentData` but entirely unmentioned in the Stage 3 done
conditions' "no layout change" section (§7.6-7.7 only named the two).
Confirmed present at `src/MyPersistentData.h:523` (`sensorConfigData` class)
and `:570` (`SensorData` inner class). Its layout/magic/version must be
preserved with the same rigor as the other two, and its fields (sensor
type/settings) need an assigned facade — noting two distinct `sensorType`
fields exist across the persisted stores and must stay distinct through the
split.

### Finding 4 — lifecycle operations need coverage beyond getters/setters

Setup ordering, `loop()` servicing, the existing 100/250/250ms save delays,
exact forced-flush call sites, occupancy revalidation
(`revalidateOccupancyStartTimeIfTimeAvailable()`, this session's own recent
work), and `resetEverything()`'s cross-store reset-count update are all
behavior that a purely accessor-level conversion could silently change even
while every individual getter/setter test passes. Not independently
re-verified line-by-line, but the specific behaviors named (occupancy
revalidation, the reset-count cross-store update) are real and traceable to
this session's own prior work — credible.

### Finding 5 — test changes extend beyond the six stub headers ✅ CONFIRMED

`tests/battery_authority_seam_structural_test.py`'s Invariant 4 (a *positive*
control) explicitly requires `BatteryAuthorityCommand.cpp` to textually
reference `MyPersistentData.h`, specifically to prove
`currentTier()`/`commit()` are backed by real persisted state and not
stubbed out. Verified directly at line 156. If the facade split changes this
file's include to `persist/PowerConfig.h`, this existing test breaks *for
the right reason to need updating, not because anything is actually wrong* —
Codex's point that authorized test changes must extend beyond the six
`tests/stubs/.../MyPersistentData.h` variants is confirmed correct, and this
specific test is now a named, concrete instance of that class of required
update.

### Finding 6 — a live, unrelated regression found along the way

Running the full bare-`.py` test set (17 files, none wrapped in a `.sh` and
therefore never part of any "full host suite" sweep this session ran)
surfaced one failure: `tests/serial_settle_test.py`, broken by the merged
Item 2 (`waitForDebugSerialIfConnected()` extraction, PR #37) — the test
literally scans for `if (Serial.isConnected())` inline in `setup()`, which
the extraction moved into a named function. Independently reproduced:
`python3 tests/serial_settle_test.py` fails on current `main` with "Serial.isConnected()
check not found before ensureRetainedLoopForensicsInitialized() in setup()".
All other 16 bare-`.py` tests pass — this is isolated, not systemic. **Not
part of Step 5's scope** — flagged for Chip to route separately (bundle into
Step 5's Copilot dispatch, or its own small WO first). Also surfaces a
standing process gap: this session's "full host suite" verification covered
only `.sh`-wrapped tests and missed these 17 standalone Python tests
entirely; future verification passes need to include both.

### Open questions A and B — Codex's positions

**A (alert/forensics fields on `current`):** Codex recommends putting
`alertCode`/`lastAlertTime`/`raiseAlert()` in `RecoveryState`, with the
facade forwarding to the existing `CurrentData` storage underneath (facades
crossing the underlying-object boundary is legal, confirmed in the original
draft's framing of the question). Preserve severity-arbitration and
timestamp behavior exactly.

**B (connectivity/connection-budget fields):** Codex recommends keeping four
facades but broadening `SystemConfig`'s definition to "system configuration
and operational bookkeeping" — folding in connection budgets, the attempt
counter, connection history, serial configuration, modes, and sensor
settings, rather than introducing a fifth facade. `RecoveryState` keeps
recovery escalation/watchdog state, `PowerConfig` keeps power settings,
`CurrentReadings` keeps measured values/counts/occupancy.

Both positions are internally consistent with the original four-facade
structure (no fifth header needed) and Claude has no basis to disagree with
either — recorded here for Chip's decision, not Claude's, per the workflow's
separation of roles.

**Chip's decision (2026-09-23): both approved.** A approved as proposed —
matches the original intuition, no new concerns raised. B approved as
proposed, with a standing note: `SystemConfig` is now the broadest of the
four facades (mode, schedule, webhook config, plus connection
budgets/attempt counter/connection history/serial config absorbed from open
question B), and is the facade most likely to need a second look if it keeps
absorbing "operational bookkeeping" fields in future steps. Not a reason to
hold Step 5 — a marker for whoever next reviews this area.

### Zero-accessor files — resolved, not just confirmed empty

| File | Resolution |
|---|---|
| `cloud/Cloud.h` | Needs the `BatteryTier` type, not persistence accessors — redirect to `cloud/BatteryBackoffPolicy.h` (confirmed via Codex's host compile probe). Not a dead include; a wrong one. |
| `cloud/Particle_Functions.cpp` | Persistence include confirmed unused by source inspection — dead include, drop it. |
| `power/PowerDiagnostics.cpp` | Compiles without it in the existing host test environment — dead include, drop it. |
| `power/PowerPlatform.cpp` | Compiles without it in the existing host test environment — dead include, drop it. |
| `sensors/PIRSensor.h` | Persistence include unused; its actual `SensorData` need comes from `ISensor.h` — dead include, drop it. |

### Revised acceptance criteria (supersedes §7 above where they overlap)

In addition to the original seven:

8. Independent compilation of each real facade header on its own (not just
   as included by a consumer), plus a mutation check for indirect dependency
   leakage (a facade that compiles only because it transitively pulls in
   something from a sibling facade or from `MyPersistentData.h` itself).
9. Backend/facade tests proving shared state, alert-severity-arbitration
   behavior, `validate()` behavior (including `ConfigApply.cpp`'s
   capacity-constant-based validation, not a `sizeof()` on a facade type),
   and deferred-vs-forced persistence timing.
10. ARM layout checks for **all three** persisted records (`SysData`,
    `CurrentData`, `SensorData`) — sizes, offsets, magic/version, string
    capacities — not just the two named in the Stage 3 draft.
11. All 27 real consumers (23 original + 4 transitive-via-`Cloud.h`)
    confirmed individually, superseding done condition 3's original count of
    23.
12. A defined Dev-14 preservation procedure that explicitly distinguishes
    "unchanged stored configuration" (must read back identical) from
    "expected runtime counter/timestamp updates" (will legitimately differ
    across a day of operation) — the original "reads back identical"
    framing in §7.6-7.7 is too strong as written and needs this
    qualification to be checkable at all.

### Revised recommendation

Scope is confirmed larger and more specific than the Stage 3 draft, but
nothing here contradicts the underlying premise — Codex's assessment is
"the structural problem is real," not "don't do this." Recommend: fold
findings 1-5 and the zero-accessor resolutions into the architecture before
Stage 5 approval; decide A and B; decide routing for the serial_settle_test.py
regression (finding 6) separately from Step 5 itself.

**Resolved (2026-09-23):** A and B both approved (see above). Finding 6
routed to its own WO, `WO-2026-09-23-002-serial-settle-test-regression.md`,
dispatched separately and not blocking Step 5 — reasoning: Step 5 is already
the largest, highest-risk step in the roadmap and just grew (23→27 files, a
third persisted store); folding an unrelated pre-existing regression into it
would make Step 5's diff harder to review and would leave it ambiguous,
if something goes wrong during Step 5, whether the serial-test breakage was
old or new. **Chip's Stage 5 approval of this WO is conditional on
WO-2026-09-23-002 landing clean on `main` first** — Step 5 starts from a
genuinely green baseline, not one with a known, unrelated crack in it.

## Bench validation result (Dev-14) — PASS

**Result: PASS.** 24h of bench time on Dev-14 counted from the flash of
`v25-Facades-BenchDiag` (2026-09-24 04:34Z), checked 2026-09-25 04:49Z. The
window included one intentional power-down (05:52Z) and one reset (05:56Z pin
reset). Both passed: stored data came through each unchanged.

`v25-Facades-BenchDiag` = this WO's commit plus the two diagnostic-only log
lines from WO-2026-09-23-003 (not part of this WO).

| Criterion | Evidence |
|---|---|
| 7 — no re-init, no data loss | `dailyOccupancy` preserved across the reflash (333→333), the power-down (367→367) and the overnight hibernate (426→426). `alertCount`/`lastAlert` unchanged all day. No watchdog or unexpected resets on this build; uptime continuous from 22:00Z to the check. |
| 12 — unchanged stored configuration | `config.generation` = `FA0C8923` before and after the day, with `connectionMode`=3 and battery tier HEALTHY. `generation` alone cannot rule out a re-init, because `ConfigApply` re-applies cloud settings on every connect. That is closed by `SysData` fields that are never cloud-sourced, all intact across the 22:00Z hibernate reload: `lastConnection` (read back 1790262006, the value written at 15:00Z), `watchdogResetCount`=104, `lastWatchdogUptimeMs`=1075, `resetCount`=18. |
| Facade write/read-back | The `lastConnection: old -> new` diagnostic chain is continuous across every captured connection, including the hibernate boundary. |

Evidence sources: `./tools/telemetry` (device, timeline, serial) and read-only
S3 reads of the `status`/`watchdog` event payloads.

### Notes (recorded verbatim from the 2026-09-25 04:49Z assessment)

1. **Clock start.** If you count the day from the 05:56Z power-up after the power-down, instead of from the flash, the day ends at 13:56 SGT, about an hour from now. I'd count from the flash: a power-down is a hardware event, and the stored data came through it unchanged.
2. **`OccupancyWebhook` is still unconfirmed.** That diagnostic line has never been captured, because the serial forwarder drops lines. It needs one direct USB serial capture at a report, and it's part of the diagnostic-logging change (WO-2026-09-23-003), not a Step 5 criterion.
3. **Thermal thresholds aren't covered by `generation`.** Close them with a direct read, or explicitly leave them out of criterion 12's scope.
4. **The daily count didn't reset at midnight.** That's the known bug in WO-2026-09-24-001, not Step 5.

### Decisions (Chip, 2026-09-25)

- **Note 1:** the 24h window is counted from the flash (04:34Z 2026-09-24).
- **Note 2:** the `OccupancyWebhook` capture over direct USB serial is part of
  WO-2026-09-23-003's validation, not this WO's.
- **Note 3:** the thermal-charge thresholds (`thermalChargeArmHighC`,
  `thermalChargeArmLowC`, `thermalChargeReleaseHighC`,
  `thermalChargeReleaseLowC`) are **excluded from criterion 12**.
  `config.generation` does not cover them (it also omits `solarPowerMode`,
  `lowPowerMode`, `disconnectedMode`, `structuresVersion` and
  `testConnectionDurationOverride`), so criterion 12's `generation` evidence
  says nothing about them. Filed separately as #43.
- The serial forwarder dropping lines (note 2) is filed as
  chipmc/local-serial-log-forwarder#1.
- **Note 4:** pre-existing; fixed by WO-2026-09-24-001, next in sequence.

### Also observed (not Step 5)

- Two `bc=28` sleep-stage watchdog resets at 2026-09-24 03:55Z occurred on
  `v24-Thermal-Inhibit`, before this build was flashed (WO-2026-09-03-004
  class).
- SoC jumped from 68% to 99.7% after the power-down; `BatteryHealth` flagged
  it `Suspect` (resting estimate ~74%). Fuel-gauge re-seed, not persistence.
