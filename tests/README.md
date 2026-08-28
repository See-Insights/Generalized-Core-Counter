# Host regression tests

The reporting-policy calculation is isolated from Particle Device OS so its
battery tiers, effective intervals, boundary alignment, and open-window filter
can be checked with the host compiler:

```sh
c++ -std=c++11 -Wall -Wextra -Werror -Isrc \
  tests/reporting_policy_test.cpp src/reporting/ReportingPolicy.cpp \
  -o /tmp/reporting_policy_test
/tmp/reporting_policy_test
```

The runtime adapter itself (`ReportingPolicyResolver::resolveRuntime()` in
`src/reporting/RuntimeReportingPolicy.cpp`) is compiled and exercised directly
against a stub Particle/SensorManager surface - see
`tests/reporting_policy_adapter_test.sh` (WO-2026-08-25-001 Amendment C,
Decision C2 / AC-C6). This closes the gap where the guard's unit tests passed
in isolation while the production adapter still let an Invalid or Unavailable
vcell silently bypass the trust substitution and the 3.5V floor. The normal
Particle firmware compile remains the final integration check.

The boot-scoped startup snapshot has a separate host test:

```sh
c++ -std=c++11 -Wall -Wextra -Werror -Isrc \
  tests/startup_snapshot_test.cpp src/observability/StartupSnapshot.cpp \
  -o /tmp/startup_snapshot_test
/tmp/startup_snapshot_test
```

## WO-2026-08-10-001 (watchdog reset instrumentation) host tests

The AB1805 wake-reason classification, watchdog alert code, sleep-entry
breadcrumb sequence, and item-4 serial-settle changes added by
`docs/work-orders/WO-2026-08-10-001-watchdog-instrumentation.md` cannot be
compiled standalone (they sit inside `Generalized-Core-Counter.cpp` and
`State_Sleep.cpp`, both of which pull in heavy Particle/AB1805/PublishQueue
dependencies). Each of the following tests either mirrors the relevant
decision logic in a dependency-free host function (cross-checked against the
real source via `grep`/`awk` so the mirror can't silently drift) or parses
the actual shipped source directly:

```sh
zsh tests/watchdog_ab1805_classification_test.sh   # item 1: AB1805 wake-reason classification
zsh tests/watchdog_alert_code_test.sh              # item 3: new alert code severity/auto-clear/payload
python3 tests/sleep_breadcrumb_sequence_test.py    # item 2: sleep-entry breadcrumb ordering (traces real source)
python3 tests/serial_settle_test.py                # item 4: always-on serial-settle placement/gating (traces real source)
```

## WO-2026-08-11-001 (SLEEP_PREP threshold exclusion) host test

`noteLoopStageDuration()`/`setLoopStage()`/`transitionTo()`
(`src/Generalized-Core-Counter.cpp`) and `handleSleepingState()`
(`src/state/State_Sleep.cpp`) pull in heavy Particle/PublishQueuePosix
dependencies and can't be compiled standalone on the host. This test parses
the actual shipped source directly to confirm the corrected design:

- `LOOP_STAGE_SLEEP_PREP` is still excluded from the
  `kLoopStageWarnThresholdMs`/`kLoopStageErrorThresholdMs` WARN/ERROR
  escalation inside `noteLoopStageDuration()` (a bare early return, before
  the threshold comparisons, with no logging call of its own).
- `setLoopStage()` is fully reverted to its simple original form (same-stage
  early-return guard, then unconditional overwrite of
  `lastLoopStage`/`stageStartMillis`), with no SLEEP_PREP-specific logic of
  any kind. An earlier corrective pass tried putting the SLEEP_PREP exit-log
  here, gated on `setLoopStage()`'s own same-stage guard - broken, because
  `loop()` retags the loop stage through a fixed sequence every iteration
  regardless of what the dispatched state handler did, so the guard fired
  the exit-log on nearly every gate-wait polling iteration (a log-storm
  regression under a different call site than the original bug).
- **The `INFO`-level `LoopStage:` line for `SLEEP_PREP` is logged from a
  single universal choke point: `transitionTo()`
  (`src/Generalized-Core-Counter.cpp`).** The codebase's `state` FSM
  variable is written in exactly one place in the whole codebase -
  `transitionTo()` - so every transition, from any call site in any file,
  passes through it. Before overwriting `state`, `transitionTo()` checks
  whether the OLD state is `SLEEPING_STATE` and a SLEEP_PREP span is
  currently open (`retainedLoopForensics.sleepPrepSpanStartMillis != 0`); if
  so, it closes the span (`closeSleepPrepSpan()`) and logs the one INFO line
  (`logLoopStageLine()`) before proceeding with the state change and its own
  `"StateReq: ..."` log line.

  An earlier corrective pass instead used a local wrapper function,
  `exitSleepingState()`, that all 13 real exit points inside
  `handleSleepingState()` called instead of `transitionTo()` directly. That
  design rested on an unstated, false assumption: that every real exit from
  `SLEEPING_STATE` happens via a `transitionTo()` call from *within*
  `handleSleepingState()`. Stage 7 found three live counterexamples that
  bypass any such wrapper entirely: out-of-memory handling and
  user-switch-press handling in `loop()` (both call `transitionTo()`
  directly after the state-handler switch), and
  `connectivityFailsafeSupervisor()`'s failsafe-stage-1 action (called
  *before* the switch even runs). None of these route through a
  `State_Sleep.cpp`-local wrapper; if any fires while a SLEEP_PREP span is
  open, the span would never close under that design - poisoning the next
  real dwell's elapsed computation. Moving the check into `transitionTo()`
  itself closes this gap completely and by construction: it applies to
  every exit from `SLEEPING_STATE`, regardless of which file or function
  calls `transitionTo()`.

  `exitSleepingState()` has been removed entirely; all 13 call sites in
  `State_Sleep.cpp` are plain `transitionTo()` calls again. Two
  `System.reset()` paths (the nightly heap-guard reset and the
  all-sleep-attempts-failed reset) never routed through `transitionTo()` (or
  the old wrapper) in the first place, since `System.reset()` itself doesn't
  return - both already carry more precise purpose-built `Log.warn`/
  `Log.error` diagnostics, and the device reboots regardless. The
  dead/unreachable `transitionTo(ERROR_STATE, "sleep-all-attempts-failed")`
  line that follows the second reset is also left untouched, out of scope.

  A non-negotiable structural test confirms `State_Sleep.cpp` contains
  **zero** occurrences of the shared logging helper name anywhere in the
  file (not scoped to any function) and **zero** occurrences of the
  identifier `exitSleepingState` - this directly catches a rogue logging
  call planted anywhere in the file (e.g. directly before the cloud-gate
  polling `return;`), which a wrapper-scoped presence check could not.
- A dedicated `RetainedLoopForensics.sleepPrepSpanStartMillis` field (new;
  `kLoopForensicsVersion` bumped 1 -> 2 since this is `retained` RAM) tracks
  the elapsed-time reference for the whole span - NOT the pre-existing
  `stageStartMillis`, which stays contaminated by loop-stage tag churn. It
  is set with a **zero-gated** check via `maybeStartSleepPrepSpan()`, not an
  `enteredState`-gated one: two of the 13 real exit paths transition
  `SLEEPING_STATE -> SLEEPING_STATE`
  (`sleep-timer-occupied-suppress-report`, `sleep-pir-return-to-sleep`) - a
  real cycle ends and a new one begins, but the FSM `state` value doesn't
  change, so an `enteredState`-gated reset would never fire for the new
  cycle and would silently report `elapsed=0`. `transitionTo()`'s
  choke-point guard reads the OLD `state` value (before the assignment), so
  it still correctly fires on a `SLEEPING_STATE -> SLEEPING_STATE`
  self-transition.
- A shared `logLoopStageLine()` helper still backs both
  `noteLoopStageDuration()`'s WARN/ERROR lines and `transitionTo()`'s
  SLEEP_PREP `INFO` line, so the five-field format string
  (`stage`/`elapsed`/`state`/`q`/`connMs`) can't drift between the two.
- Other stages' WARN/ERROR threshold behavior is unchanged.

It then runs a call-frequency/boundedness simulation matching the real
control flow: repeated polling calls with OTHER loop-stage tags retagged in
between each one (mirroring `loop()`'s actual per-iteration behavior),
proving the design is unaffected by that churn because it no longer depends
on loop-stage tags, or on which function/file calls `transitionTo()`, at
all - only on whether `state == SLEEPING_STATE` and a span is open at the
moment ANY `transitionTo()` call fires. A dedicated scenario models an
"external" transition (representing the OOM/user-switch/failsafe-stage-1
case from outside `handleSleepingState()` entirely) and proves it still
closes an open span correctly - this is the specific regression the
wrapper-based design could not catch.

A dedicated scenario also proves the zero-gated reset correctly starts a
fresh span for a second real cycle immediately following a
`sleep-pir-return-to-sleep`-style `SLEEPING_STATE -> SLEEPING_STATE`
self-transition - elapsed for the second cycle is neither `0` nor inclusive
of the first cycle's duration.

The test was mutation-tested against both of Stage 7's exact findings:
(a) temporarily removing the guard from `transitionTo()` (models an
external transition bypassing the choke point) makes the test fail; (b)
temporarily inserting a rogue `logLoopStageLine(...)` call directly in
`State_Sleep.cpp`'s cloud-gate polling section (before its `return;`) makes
the "zero occurrences in `State_Sleep.cpp`" check fail. Both mutations were
reverted after confirming.

```sh
python3 tests/loop_stage_sleep_prep_exclusion_test.py
```

## WO-2026-08-11-001 (SLEEP_PREP threshold exclusion, Third Corrective Pass) host tests

Stage 7 found two more issues on this WO after the Second Corrective Pass:
(1) `retainedLoopForensics.sleepPrepSpanStartMillis` survives a HIBERNATE
success reset (an implicit reset inside `System.sleep(config)` with
`SystemSleepMode::HIBERNATE`, not a syntactically visible `System.reset()`
call) and the two explicit `System.reset()` paths, poisoning the elapsed
computation for the next real `SLEEPING_STATE` dwell after a normal
overnight reboot; (2) the Second Corrective Pass's own regression test for
`exitSleepingState()`'s logging call was a presence check (does the helper
name appear anywhere in the wrapper's text), not a count/reachability
check, so it passed unchanged when Codex's Stage 7 mutated the wrapper to
log the SLEEP_PREP line twice, and also when it wrapped the logging call
in `if (false) { ... }` so it never logs at all.

Fix 1: `setup()` (`src/Generalized-Core-Counter.cpp`) now calls
`resetSleepPrepSpanOnBoot(retainedLoopForensics.sleepPrepSpanStartMillis)`
unconditionally on every boot, regardless of `System.resetReason()` -  any
real reboot invalidates any in-flight span by definition, since the call
stack that would have closed it via `closeSleepPrepSpan()` is gone.

Fix 2: the span start/close/boot-reset arithmetic - the exact logic all
three of this WO's regressions have lived in - is extracted into a new,
dependency-free header, `src/state/SleepPrepSpanTiming.h` (`<cstdint>`
only, no `Particle.h`/`AB1805_RK.h`/other project headers), defining three
pure inline functions: `maybeStartSleepPrepSpan()` (the zero-gated start,
used by `handleSleepingState()`), `closeSleepPrepSpan()` (computes
elapsed, resets to 0, used by `exitSleepingState()`), and
`resetSleepPrepSpanOnBoot()` (the new boot-time fix, called from
`setup()`). A new test, `tests/sleep_prep_span_timing_test.cpp`,
`#include`s this header directly and compiles/runs on the host with zero
stubbing, exercising the *real* production functions instead of a
hand-written simulation - closing the class of gap that recurred three
times for this specific logic. It covers a normal span; the
`SLEEPING_STATE -> SLEEPING_STATE` self-transition case (two spans back to
back through the same zero-gate); and the HIBERNATE-poisoning scenario as
a genuine before/after regression proof (a stale nonzero value survives a
`maybeStartSleepPrepSpan()` call made without first calling
`resetSleepPrepSpanOnBoot()`, proving the bug would occur, then is
correctly cleared once `resetSleepPrepSpanOnBoot()` is called, letting the
next span start fresh).

```sh
c++ -std=c++11 -Wall -Wextra -Werror -Isrc \
  tests/sleep_prep_span_timing_test.cpp \
  -o /tmp/sleep_prep_span_timing_test
/tmp/sleep_prep_span_timing_test
```

`tests/loop_stage_sleep_prep_exclusion_test.py` was subsequently rewritten
again in the Fourth Corrective Pass (see the section above) once Stage 7
found the wrapper-based design (`exitSleepingState()`, enumerating 13 call
sites in `State_Sleep.cpp`) itself missed real `SLEEPING_STATE` exits from
outside `handleSleepingState()` entirely. The current test asserts the
choke point lives in `transitionTo()` instead, and that `State_Sleep.cpp`
contains zero occurrences of the logging helper or `exitSleepingState`
identifier anywhere in the file - see above for the current, up-to-date
description.

```sh
python3 tests/loop_stage_sleep_prep_exclusion_test.py
```

## WO-2026-08-25-001 (power management consolidation), Amendment B, Round 3 host tests

Round 3 responded to a Codex Stage 7 "Not verified" verdict (four blockers)
against the Amendment B architectural decisions (B1/B2/B3) and AC-B1..AC-B8.

**Blocker 1 (F3/F4 architecture, Decision B1):** `BatteryBackoff` remains the
sole tier/hysteresis/persistence authority. `PowerBehavior.{h,cpp}` (a
duplicate of `BatteryBackoff::intervalMultiplier()` with zero production
callers) was deleted, along with `tests/power_behavior_test.{cpp,sh}`. F1's
trust signal and F3's vcell floor are applied only as a guard on the tier
input and a floor on its output - `src/reporting/BatteryTierGuard.{h,cpp}` -
wired into `RuntimeReportingPolicy::resolveRuntime()`. `PowerTier::evaluate()`
is retained solely for this floor/guard role.

```sh
zsh tests/battery_tier_guard_test.sh   # AC-B2 (untrusted SoC doesn't drive cadence), AC-B3 (vcell floor)
zsh tests/power_tier_test.sh           # AC-B1 (PowerTier's SoC breakpoints assert-equal BatteryBackoff's)
```

**Blocker 2 (Boron SoC commit regression, AC-B4):** retiring the STALE_SOC
resync machinery's `commitSoc()` silently deleted the ordinary Boron
`current.set_stateOfCharge()` commit alongside it. The plain commit (gated
on the same `rejectAuthoritativeOverwrite` fence used elsewhere in
`batteryState()`, not on any retired stale-SOC condition) was restored.
`SensorManager.cpp` cannot be compiled standalone (heavy PMIC/System/Log
dependencies, no full-file stub harness), so this is a source-tracing test
in the same style as the WO-2026-08-11-001 tests above:

```sh
python3 tests/boron_soc_commit_test.py
```

**Blocker 3 (thermal inhibit arming from an unmeasured temperature, Decision
B2, AC-B5/AC-B6):** `SensorManager::isItSafeToCharge()` now tracks whether a
genuine (non-fallback) temperature reading has occurred this boot
(`_temperatureMeasuredThisBoot`) and syncs its software `inhibited` state
from the DCT-persisted hardware flag once per boot before making any
decision. The arm/hold/release decision itself is delegated to a new pure,
host-testable function,
`ChargeInhibitPolicy::evaluateThermalWithValidity()`
(`src/power/ChargeInhibitPolicy.h`): a stale/unmeasured temperature may not
ARM the inhibit, and an already-armed inhibit is held-but-flagged
(`WakeCycleStats::thermal_inhibit_held_without_fresh_temp`) rather than
silently continued, while a fresh reading may always arm or release.
`ChargeInhibitPolicy::isValidThermalThresholds()` (AC-B6) enforces
`armHigh > releaseHigh`, `armLow < releaseLow`, and a hard `armHigh <= 45C`
cell charge-maximum ceiling; `Cloud::applyPowerConfig()` validates the whole
candidate threshold set before applying any field, rejecting the entire
update rather than partially applying an invalid one.

```sh
zsh tests/charge_inhibit_thermal_test.sh
```

This covers the original hysteresis tests plus: `evaluateThermalWithValidity()`
directly (unmeasured-cannot-arm, unmeasured-holds-but-flags, fresh-reading
can-arm-and-can-release), a reproduction of the exact reset/hibernate field
scenario Codex Stage 7 found reachable (a persisted-hot temperature across
several simulated interrupted boots never arms, and correctly releases once
a real reading lands), and `isValidThermalThresholds()` (defaults valid,
inverted high/low hysteresis rejected, `armHigh` above/at the 45C ceiling).
A fidelity check confirms `SensorManager.cpp`'s `isItSafeToCharge()` calls
the real `evaluateThermalWithValidity()` under test, not a hand-written
mirror of it.

**Blocker 4 (PMIC remediation suppression, `PmicFaultMonitor.cpp`):**
remediation suppression while `!safeToCharge` is now per fault class rather
than blanket. It is defensible (and retained) only for a thermal shutdown
fault (0x02) or a newly-classified NTC-status fault (REG09 bits 2:0,
previously neither classified nor alerted); it no longer applies to an input
fault (0x01) or safety-timer expiry (0x03), which have independent causes
and would otherwise lose their only attempted recovery for an entire hot
interval. A non-thermal fault additionally defers (rather than force-toggles)
only when `ChargeInhibit`'s hardware disable is READ-BACK VERIFIED active
this cycle (a new `chargeDisableVerified` parameter, threaded from
`SensorManager::chargeDisableVerified()`), not merely the software
`safeToCharge` intent. Suppression/deferral now FREEZES in-progress
remediation state instead of zeroing it, so a level-1/2 sequence that already
disabled charging in phase 0 is not abandoned mid-cycle with charging left
disabled and no software owner to re-enable it. This module has heavy PMIC/
Particle dependencies with no standalone host test; it is exercised via the
full firmware compile and the linkage/nm evidence in the round's
implementation report.

**ALSO FIX:** `PowerDiagnostics::recordResyncEvent()` (zero production
callers once its owning STALE_SOC machinery was retired) was removed, along
with its now-unreachable serialization branch and reason code.
`ChargeInhibit::apply()` now retries once on an immediate
`System.setPowerConfiguration()` failure as well as a successful-write-wrong-
readback (previously only the latter retried).
