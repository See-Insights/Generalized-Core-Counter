# WO-2026-08-06-001: Power Management Consolidation Phase 1 — wire up PowerManager scaffolding

## Context

Per `docs/architecture/power-management-consolidation-findings.md`:
`PowerManager` already has almost exactly the vision doc's four-function
shape at the interface level, but is dead scaffolding — zero consumers
anywhere in the codebase, landed in one commit back in May, well before the
vision doc existed. Reads as intentionally broad-but-unfinished.

Phase 1's goal: make `PowerManager` the actual call path for power/battery
queries and decisions, migrating existing scattered call sites (per the
full inventory in the findings doc) to go through it — **without changing
current behavior**. The messy/inconsistent parts already found (incompatible
low-battery thresholds, sleep-mode taking no battery input, the dead
critical-battery shutdown, thermal-charging fusing state-classification
with actuation) are explicitly Phase 2 scope. Phase 1 consolidates *where*
things live, not *what* they decide.

This is a refactor of central, safety-relevant plumbing that WO-2026-08-05-002/
003's battery-authority logic already depends on — treat regression risk
accordingly. Full Codex Stage 7 verification applies (not the lighter
tooling-diff pattern used for the CLI work), per the standing two-tier
verification rule.

## Investigation needed (this WO doesn't yet know enough to specify the fix)

1. Read the actual current `PowerManager.h`/`.cpp` interface in full —
   what's already declared/stubbed for each of the four functions, and how
   closely it actually matches the vision doc's shape versus how closely
   Claude Code's summary characterized it.
2. Using the findings doc's full call-site inventory, enumerate exactly
   which call sites in `SensorManager.cpp` (and anywhere else) would need
   to redirect through `PowerManager` instead of reading the fuel gauge/PMIC/
   config directly.
3. Propose a migration plan: incremental (one call site/function at a time,
   verified at each step) versus a single larger cutover — with a
   recommendation and tradeoffs, not just an implementation.
4. Specify how function 1 (best SOC estimate) and function 4 (corrective
   actions) expose the already-built `BatteryAuthorityPolicy`/consistency-
   check logic from WO-2026-08-05-002/003 through `PowerManager` — does
   `PowerManager` wrap/call into that existing logic, or does logic need to
   move? Avoid duplicating logic that already exists and is tested.
5. For functions 2 and 3, where Phase 2 will eventually need to change
   real behavior (thermal-charging fusion, the incompatible low-battery
   thresholds) — does Phase 1's wiring need any interface shape
   considerations now to make Phase 2 easier later, or is that premature
   optimization better left until Phase 2 actually scopes it?
6. Flag anything found where preserving exact current behavior through
   `PowerManager` isn't straightforward — don't silently resolve it,
   surface it for a decision.

## Decisions needed (not prescribed by this WO)

- Migration strategy (incremental vs. single cutover) — pending
  investigation into the actual scope/risk per call site.
- Exact mechanism for function 1/4 to expose existing WO-002/003 logic
  without duplication.

## Acceptance Criteria

TBD pending investigation. At minimum: `PowerManager` becomes the actual
call path for SOC estimate and battery state (the two functions with
already-built logic to wire up), with byte-for-byte identical behavior to
today's scattered call sites for every migrated site, and full regression
coverage proving it.

## Required Tests

- For each migrated call site: a regression test confirming identical
  output before and after migration.
- Unit tests for `PowerManager`'s own public interface.
- Full existing WO-002/003 test suite continues passing unchanged.

## Permitted Files

Likely `src/power/PowerManager.h`/`.cpp`, `src/sensors/SensorManager.cpp`,
and test files. Codex to confirm exact scope once the migration plan is
proposed.

## Stage 4 — Investigation (Claude + independent Codex pass, cross-verified)

### 1. Confirmed current PowerManager interface

`PowerReading`/`PowerPolicy`/`PowerAction` are declared with the full
four-function shape, but only a narrow slice is actually implemented:
capability detection, power-source reading/classification, USB-vs-solar
input-profile selection/application, and `action.chargingControlAvailable`.
Nothing populates SOC, battery context, any `PowerPolicy` field, or
`action.shouldEnableCharging`/`chargingActionNeeded` — confirmed by grep,
zero writers anywhere in the repo, including inside `PowerManager.cpp`
itself.

**One correction to the prior "entirely dead" characterization**:
`PowerReading::fallbackUsed` *is* read — by
`DeviceStatusPublisher.cpp:157`, published as `"overrideActive"` in the
device-status ledger. It's never written, so it always publishes `false`
today — but it's real, externally-visible telemetry. Phase 1 must not
accidentally start populating it as a side effect of other wiring; that
would silently change published output.

### 2. Exact external call-site migration list (verified, current line numbers)

15 SOC reads, 2 battery-state reads, across 8 files — cross-verified
independently by both investigations, identical results:

**SOC** (`current.get_stateOfCharge()`): `ConnectivityFailsafeTest.cpp:61`,
`DeviceStatusPublisher.cpp:160,334`, `Generalized-Core-Counter.cpp:316,1124,1635`,
`PowerDiagnostics.cpp:141`, `State_Report.cpp:137`, `State_Idle.cpp:128`,
`State_Connect.cpp:116`, `State_Sleep.cpp:1144,1308,1315,1506,1562`
(1308/1315 are mutually-exclusive `#if` alternatives — same call site,
migrate together).

**Battery state** (`current.get_batteryState()`):
`Generalized-Core-Counter.cpp:1625`, `State_Sleep.cpp:1146`.

**Explicitly excluded from migration** — `SensorManager.cpp`'s own
internal reads of the ledger it produces (`:527`, `:1114`, `:1660/1676/1724`,
`:1838`). Redirecting these through `PowerManager` would be an ownership
inversion (producer → facade → producer-owned ledger) with no benefit.
Two exceptions worth documenting rather than silently ignoring: line
`:1838` (the dormant `runPmicChargeCycleTest()` safety gate) and the
diagnostic raw-gauge rereads (`:1358`, power-mismatch log) are deliberate,
permanent exceptions to the vision doc's eventual "nothing reads the gauge
directly" end state, not oversights.

No external call site reads the fuel gauge directly — confirmed. All raw
hardware access is already confined to `SensorManager.cpp`.

### 3. Function 1/2 design: live pass-through accessors, no caching

**Confirmed as the only behavior-safe design, with concrete evidence
(verified directly) that caching would be actively wrong, not just
risky**: `PowerManager::refreshInputProfile()` is called from *inside*
`SensorManager::batteryState()` at line 753 — well *before* that same
invocation's SOC commit resolves at line ~1321
(`BatteryAuthorityPolicy::resolveSocCommit()` at ~1300). If `PowerManager`
cached SOC into `report_` during refresh, it would capture the
**pre-commit, stale value on every single cellular cycle** — not an edge
case, the normal case. `batteryState()` is also called from seven
distinct places in the cycle (Setup, Connect, Idle, Report, PreSleep, two
PostWake paths), none synchronized with `PowerManager`'s own refresh
cadence.

Design contract for the two new accessors:
- Pure pass-through: `current.get_stateOfCharge()` / `current.get_batteryState()`,
  exactly one direct ledger read, no side effects.
- Must NOT call `setup()`, `refreshInputProfile()`, the gauge, PMIC, or
  `BatteryAuthorityPolicy` — read-only, full stop. (Verified directly:
  `current.setup()` at line 822 → `PowerManager::instance().setup()` at
  823 → first Setup battery sample not until line 1120 — an accessor that
  triggered any refresh/sample side effect could recurse or read
  mid-initialization state.)
- Must NOT gate on `setupComplete_`.
- Function 2 returns raw `uint8_t` (today's exact semantics), not a
  mapped `PowerBatteryContext` enum — no `ChargingDisabled` state exists
  today (thermal-disable currently reports as plain `NotCharging`);
  inventing that separation is Phase 2, not Phase 1.
- Define in `.cpp`, not inline in the header (avoids exposing
  `MyPersistentData.h`/its macros through `PowerManager.h`'s public
  surface).
- Document explicitly that `latestReport().reading.soc`/`batteryContext`
  stay permanently unpopulated/`NaN` in Phase 1 — the new accessors are
  the authoritative path; don't attempt to reconcile or partially
  populate the cached struct, that reintroduces the staleness problem
  from a different angle.

### 4. Function 4: no public surface in Phase 1 — nothing to migrate yet

There is currently no external call site that triggers a corrective
action — `quickStart()` calls (`SensorManager.cpp:620` wake-time
stabilization, `:1266` the `resolveSocCommit()` callback) are both fully
internal to `SensorManager.cpp`, invoked automatically within its own
sampling cycle. Confirmed: no production caller reaches either from
outside the file.

A read-only "did a resync happen" status doesn't exist as reusable state
today either: `SocCommitResolution` is local to one function invocation,
the debounce/cooldown counters are file-static, and `quickStart()` itself
returns `void` — there's no hardware-confirmed "success," only
`resyncAttempted`/`settledSampleStale`/`shouldCommit`, which describe
different, non-interchangeable outcomes.

**Decision: Phase 1 adds no function-4 surface to `PowerManager`.**
Building one now would be speculative scaffolding — a `power` → `sensors`
dependency added for a facade with no real consumer, which doesn't
"migrate a call site" (there isn't one) and risks the exact kind of
premature-interface-that-never-gets-used problem this whole WO exists to
retrofit. If a concrete read-only consumer emerges later, the lower-risk
direction is `SensorManager` (which already depends on `PowerManager`)
publishing its completed resolution outward — not `PowerManager` querying
back into `SensorManager`, which would create a two-way ownership
dependency. Revisit in Phase 2 or later, only once an actual consumer is
named.

(`BatteryAuthorityPolicy.h` has zero Particle Device OS dependencies and
doesn't include `PowerManager.h`, so including it from `PowerManager.cpp`
would not create a circular-include problem, if/when this is revisited —
confirmed, not a blocker, just not needed yet.)

### 5. Recommended migration plan: incremental, module-grouped, single Stage-7 pass at the end

1. Add the two pass-through accessors + their own unit tests. Zero
   consumers migrated yet.
2. Migrate observational/diagnostic consumers first (lowest risk):
   `PowerDiagnostics.cpp`, both `DeviceStatusPublisher.cpp` payload sites,
   `State_Idle.cpp` logging, `State_Sleep.cpp` observability logging.
3. Migrate decision consumers one module at a time: `State_Report.cpp`
   (cadence policy), `State_Connect.cpp` (connection budget),
   `Generalized-Core-Counter.cpp` (startup + failsafe policy), then the
   `State_Sleep.cpp` wake-policy call sites.
4. Migrate `Generalized-Core-Counter.cpp:316` (production failsafe
   fallback-tier) and `ConnectivityFailsafeTest.cpp:61` (its diagnostic
   duplicate) **together, in the same change** — they already
   independently reimplement the same formula (flagged in the findings
   doc); migrating them separately risks them temporarily diverging.
5. Final static audit confirming only the documented `SensorManager.cpp`
   exceptions remain, then the full WO-002/003 regression suite plus all
   target-platform builds, then one aggregate Stage 7 pass covering the
   whole migration.

**Why incremental over single cutover**: lower regression/review risk per
step, easier bisection and Stage-7 attribution if something breaks,
mechanically small individual changes. The tradeoff (a temporary mixed
direct/facade-read state across the codebase) has no behavioral
consequence since the accessor is a pure pass-through — it's a style
inconsistency during migration, not a correctness risk. A single cutover
would produce a cleaner final diff but forces one review to cover
diagnostics, cloud payload serialization, connection policy, sleep
observability, and failsafe safety-gating simultaneously — harder to
verify and harder to attribute a regression to, on exactly the kind of
central safety-relevant plumbing this WO flagged as warranting full
Stage 7 rigor.

**What Stage 7 needs to specifically prove** (beyond "the final grep for
direct `get_stateOfCharge()`/`get_batteryState()` calls outside the
documented exceptions is clean"): exact payload serialization unchanged,
the `>50%` connection-budget boundary behavior unchanged, existing tier
hysteresis unchanged, invalid/NaN SOC handling unchanged, battery-state
bounds/default handling unchanged (including P2's forced `Unknown` and
thermal-disable's `NotCharging` mapping), pre-sleep observability mapping
unchanged, failsafe suppression logic unchanged, early-setup diagnostic
timing/ordering unchanged, all target-platform compile branches
(Boron/Argon/M-SoM/P2), and the full existing `BatteryAuthorityPolicy`
suite passing unmodified.

### 6. Behavior-preservation risks — full detail (expanded per Chip's request, 2026-08-06)

Chip declined to rubber-stamp Stage 5 without the specifics of each risk.
Full trace below; #3 corrects an initial wrong hypothesis from the first
pass.

1. **Early-setup diagnostic ordering.** `current.setup()` (line 822) →
   `PowerManager::instance().setup()` (823) → first `Setup`-context
   battery sample (1120). `PowerManager::setup()` internally calls
   `refreshInputProfile()`, which ends by calling
   `PowerDiagnostics::logPowerState()` — which reads
   `current.get_stateOfCharge()` directly (`PowerDiagnostics.cpp:141`).
   So at boot, before any fresh SOC sample this session, a diagnostic log
   fires reading the *persisted* (prior-session) value. A pure
   pass-through accessor preserves this exactly. **Concrete
   implementation trap**: an accessor gated on `setupComplete_` would
   break specifically here, because `logPowerState()` is called *from
   inside* `PowerManager`'s own setup path — a self-referential ordering
   bug, not a hypothetical one. This is why the accessor design explicitly
   forbids gating on `setupComplete_`.

2. **`PowerDiagnostics.cpp` has its own independent PMIC read.** Verified
   by reading it directly: `logPowerState()` constructs its own `PMIC`
   object and calls `getSystemStatus()` (REG08 — VBUS/power-good/charge-status/thermal),
   independent of `SensorManager.cpp`'s main acquisition and its own
   compact diagnostic — a third PMIC access point. Precision note: this
   reads REG08, not REG09 (the fault register WO-2026-08-05-003 confirmed
   is latching) — not independently confirmed to share that latching
   behavior, so not asserted to be hazardous in the same specific way.
   It is one more uncoordinated hardware access point in the pattern the
   vision doc's motivation section calls out. Phase 1's SOC/battery-state
   accessors don't touch this; consolidating PMIC reads themselves is a
   separate, later migration.

3. **`fallbackUsed` → `"overrideActive"` — corrected analysis.** Initial
   hypothesis (grouped under `"power"` in the JSON payload next to
   `"source"`/`"profile"`, so possibly related to the USB-source-override
   mechanism) was wrong. Its actual struct neighbors in `PowerReading` are
   `sampledPreRadio`, `quickStartUsed`, `stabilizationAttempts` — every
   one of those names exactly matches real, already-computed local
   variables inside `SensorManager::batteryState()`'s SOC-sampling logic
   (`SensorManager.cpp:519,639,662,669,718` has its own `fallbackUsed`,
   tracking whether that cycle's SOC came from the voltage-estimate
   fallback). Not a coincidence — `PowerReading` was almost certainly
   designed to eventually mirror exactly that SOC-acquisition telemetry.
   **Under the Phase 1 design in this WO specifically: no path to
   `true`** — the plan never touches `report_`, `refreshInputProfile()`,
   or any `PowerReading`/`PowerPolicy`/`PowerAction` field; the two new
   accessors read `current` directly, bypassing `report_` entirely. **But
   the naming match is a standing invitation for a future
   implementer — in this WO or later — to "helpfully" wire
   `PowerReading::fallbackUsed = fallbackUsed` while touching SOC-related
   code**, since the names line up so suggestively. If that happens,
   `overrideActive` — already published in the device-status ledger today,
   always `false` — would start reporting `true` on every voltage-estimate
   fallback: a real, live change to existing production telemetry. **Explicit
   non-goal for Phase 1 dispatch, not an assumed omission**: must not
   populate `fallbackUsed`, `sampledPreRadio`, `quickStartUsed`, or
   `stabilizationAttempts` under any circumstances in this phase, even
   though matching values already exist in `SensorManager.cpp`.

4. **`PowerReport::valid` — lower stakes than first implied.** Checked its
   actual consumers: all three uses are internal to `PowerManager.cpp`
   itself (`shouldLogProfileDecision`'s has-a-report-ever-run check, set
   after building a report, and `shouldApplyProfile`'s first-time check).
   **Nothing external reads `.valid` at all** — so unlike 1-3, there's no
   actual behavior-change path here. It's a documentation-clarity risk
   only: `valid` only ever meant "profile refresh has run once," not "the
   whole report is trustworthy" — worth a doc-comment so a future reader
   doesn't assume it covers fields Phase 1 doesn't touch.

## Approval Record

**Approved by Chip, 2026-08-06.** Full Stage 5 approval: the live
pass-through design (no caching) for functions 1/2, zero function-4
surface in Phase 1, the incremental module-grouped migration plan, and
all four behavior-preservation risk resolutions (§6) accepted as
specified. Risk 3's expanded analysis singled out as the standout finding
of this investigation — tracing past the initial (wrong) hypothesis to
the real risk, and reframing it correctly as a trap for whoever
implements this rather than a flaw in the plan itself. Explicit
instruction: the non-goal forbidding `PowerReading::fallbackUsed`/
`sampledPreRadio`/`quickStartUsed`/`stabilizationAttempts` population
must be stated emphatically in the Copilot dispatch, not left as a
footnote. Authorized to proceed to Stage 6.

## Stage 6 — Implementation (Copilot, 2026-08-06)

Copilot implemented on `refactor/powermanager-phase1`: the two live
pass-through accessors (`PowerManager::soc()`/`batteryState()`), all 17
external call-site migrations across the eight consumer files, and a
host-linked test (`tests/power_manager_accessors_test.cpp` +
`tests/stubs/`) exercising the real `PowerManager.cpp` — not a
hand-copied reimplementation. `SensorManager.cpp` and
`BatteryAuthorityPolicy.{h,cpp}` were left untouched, as required.

Claude's own review (independent of Copilot's report): traced every
migrated site by hand, confirmed the non-goal was honored, confirmed
`SensorManager.cpp` has zero diff, and independently recompiled/reran
both host test suites plus a full Boron Device OS 6.4.1 firmware build —
all passed clean. Flagged two out-of-scope-but-behavior-neutral touches
for Codex to independently assess: `PowerManager.cpp`'s
`#include "../Config.h"` changed to `#include "Config.h"`, and the Boron
USB-override preprocessor guard gaining a `defined(...)` check.

## Stage 7 — Verification (Codex, 2026-08-06)

**Verdict: Verified with concerns.** Codex independently confirmed, via
git object-hash comparison (not just `git diff`), that
`SensorManager.cpp` and `BatteryAuthorityPolicy.{h,cpp}` are
byte-for-byte identical to `HEAD`; confirmed all 17 call sites migrated
correctly with no surrounding logic change; confirmed the non-goal fully
honored (no writes to any of the four forbidden fields); confirmed
accessor purity; and confirmed both flagged out-of-scope touches are
behavior-neutral in real firmware builds (traced the same
`BuildProfile.h` chain independently, plus proved the two `Config.h`
include forms preprocess to an identical SHA-256).

Codex additionally caught a third out-of-scope touch Claude had seen in
the diff but not separately flagged to Chip: `[[maybe_unused]]` added to
`kPowerSourceUnknown` in `PowerManager.cpp` — a compile-warning
annotation with no runtime semantics, but an unreported item
nonetheless.

**The substantive finding**: the WO's Required Tests section calls for a
regression test "for each migrated call site where practical." Only the
two-accessor test was added — none of the 17 individual consumer sites
(JSON payload serialization, the `>50%` connection-budget boundary, tier
hysteresis, `publishData()`'s NaN/bounds guard) have an executable
regression test. Behavior-preservation at those sites is confirmed only
by static diff review (Claude's and Codex's, independently) plus the
unmodified existing test suites continuing to pass — not by a test that
would catch a future regression at any one of those sites specifically.

Codex could not independently execute the host test suites or firmware
build (sandbox blocked compiler temp-file creation, consistent with
prior WOs) — Claude's own independently-run build/test pass already
covers that gap.

**Chip's decision, 2026-08-06**: hold the line, same standard as
WO-2026-08-05-002's test-fidelity gap. Sent back to Copilot to add the
missing per-call-site regression tests before this proceeds to Stage 8.

## Stage 6/7, round 2 (2026-08-06)

Copilot's fix: `tests/power_consumer_regression_test.sh`, a new harness
using an `awk` brace-matcher (`extract_braced_block()`) to pull verbatim
production source blocks out of the real files at test-run time —
`ConnectBudgetContext`/`evaluateConnectBudget()` (`State_Connect.cpp`),
`Cloud::calculateBatteryTier()` (`BatteryBackoff.cpp`), both
failsafe-tier duplicates (`Generalized-Core-Counter.cpp`,
`ConnectivityFailsafeTest.cpp`), and `publishData()`'s battery/SOC
normalization (via a custom awk block that skips two verified-unrelated
interposed declarations) — then compiles them against real
`PowerManager.cpp`. This is not the "link the whole `.cpp`" pattern used
by the first two test files, but a deliberate answer to the same
no-hand-copied-logic requirement for files with disproportionate device
dependencies. Claude reviewed the technique in depth, traced each
extraction against the real source by hand, and flagged one residual
issue: `tests/stubs/MyPersistentData.h` hand-typed its own duplicate
`enum BatteryTier` rather than extracting it — exactly the category of
risk this standard exists to catch, just far smaller in scope. Sent back
once more; fixed by extracting the enum the same way as the function
bodies. Second Codex Stage 7 pass: **Verified with concerns** — the enum
fix was confirmed closed via SHA-256/`cmp` comparison, but coverage was
found incomplete: only 7 of the 17 migrated call sites had an executable
regression test, and the other 10 weren't explained.

## Stage 6/7, round 3 (2026-08-06)

Claude categorized the 10 uncovered sites by actual risk and got Chip's
sign-off on a scoped resolution: 4 pure-logging sites
(`PowerDiagnostics.cpp:141`, `State_Idle.cpp:129`,
`State_Sleep.cpp:1309/1316`) need no test (no decision logic — the value
only reaches a `Log.info()`/`Log.warn()` call); 4 sites
(`State_Report.cpp:138`, `Generalized-Core-Counter.cpp:1124`,
`State_Sleep.cpp:1507/1563`) need no new test because Claude verified via
`git diff` that each is an exact one-line getter substitution into
`applyBatteryAwareConnectionModePolicy()` with zero surrounding change;
2 sites (`State_Sleep.cpp:1145/1147`, the pre-sleep SOC-tenths/isCharging
encoding feeding `Observability::cycleStats().finalizeBeforeSleep()`)
were judged genuinely undertested — same risk shape as the already-tested
`publishData()` normalization — and sent back for a dedicated test.

Copilot added `testPreSleepBatteryEncoding()`, extracting the four-line
encoding block verbatim via a plain awk range extraction. Claude
independently hand-derived every expected `socTenths`/`isCharging` value
in Python from the real formula before accepting the test's own
assertions, and independently reran all three host suites plus a full
Boron firmware build.

**Third Codex Stage 7 pass: Verified.** No remaining correctness or
coverage gap. Confirmed: the new extraction is byte/hash-identical to
production source; all seven new assertions independently recomputed and
correct; the 4-site substitution audit re-verified independently (each
confirmed an exact getter-only change); the 4 logging-only sites
re-confirmed non-branching; full 17-site accounting closes exactly
(9 directly tested + 4 substitution-audited + 4 logging-audited = 17,
no site missing or duplicated); `SensorManager.cpp`/
`BatteryAuthorityPolicy.{h,cpp}` confirmed still byte-for-byte unchanged
from `HEAD`.

## Status

**Stage 7 complete and clean across three rounds.** Implementation
verified behavior-preserving by Claude's independent review and three
independent Codex passes, each targeting the gaps found by the previous
round, plus Claude's independently-run host test suites and Boron
Device OS 6.4.1 firmware build passing at every round. Ready for Chip's
Stage 8 final gate.
