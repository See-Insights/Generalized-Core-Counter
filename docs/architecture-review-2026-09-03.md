# Architecture Review - 2026-09-03

**Source:** Independent read-only review by Grok, not involved in this project's
development. Requested after a week of firmware work (`WO-2026-08-29-*` through
`WO-2026-09-03-*`) surfaced fragility on two field devices (Dev-11, MAFC-1) and
raised a concern that codebase complexity might be making both diagnosis and
maintenance harder than the problem set requires.

**Status:** Accepted as the priority for the next work session, ahead of
continuing the WO backlog.

## Verdict

Neither a clean platform nor a hairball. **A capable product core with
incomplete modularization.** Full-featured for the fleet it already runs; not
yet a platform you can casually extend with a second sensor family, radio, or
sleep policy. `PROJECT_STATUS.md` already lists codebase simplification as a
priority, so this corroborates a self-diagnosis already made.

## What is real and earned

- Domain folders (`cloud/`, `power/`, `state/`, `sensors/`, `reporting/`,
  `observability/`, `time/`, `diagnostics/`) are actually used, not decorative.
- 12 of 14 build flags default off - disciplined, not `#ifdef` sprawl.
- 27 retained (cross-reset) declarations across 6 files is proportionate for an
  outdoor, reset-prone device class.
- Test suite (8,822 LOC, 17/18 passing) is good hygiene; the single failure is
  pre-existing and unrelated to recent branches.

## Where complexity is concentrated - the actual finding

Four files dominate the system and are not really modules - they are cores with
directories arranged around them:

| File | Size |
|---|---|
| `Generalized-Core-Counter.cpp` | ~3,202 lines |
| `state/State_Sleep.cpp` | ~1,715 lines |
| `sensors/SensorManager.cpp` | ~1,416 lines |
| `MyPersistentData.cpp/.h` | ~1,842 lines combined |

`MyPersistentData.h` is included by **22 files** - highest fan-in in the repo.
The problem is not the count but the contents: mode enums, recovery stage,
watchdog forensics, webhook config and thermal-inhibit thresholds in one
persistence bag. Every future field pays a tax against this file.

Domain leakage is visible in the layout itself: `BatteryAuthorityPolicy` lives
under `sensors/` rather than `power/`; PMIC forensic retained fields are
declared from `SensorManager.h`; power policy, sensor runtime and forensics are
coupled at the header level.

Modularization is mid-flight: `ConfigMerge.cpp` is a one-line include stub,
`StateHandlers.cpp` is a "this file has been refactored" note, `SensorData.h` is
empty. Folders moved before ownership did.

## The load-bearing claim

> There is no single owner of "what time is it after this wake."

Clock trust, sleep, hibernate forensics and `sysStatus` are spread across
`Generalized-Core-Counter.cpp`, `State_Sleep.cpp`, `MyPersistentData.h` and the
retained forensics fields, with no single file able to answer that question.
Offered as the direct explanation for why `WO-2026-08-29-002` (clock trust) took
5 rounds and failed Stage 7 twice, and why `WO-2026-08-29-001` (hibernate-wake
observability) took 5 rounds across 5 distinct defect categories.

## Work-order cost, split by class

**Hardware-fault WOs** (RTC rate-halving, MAFC-1 sleep-path stalls) sit at zero
implementation rounds because the diagnosis is hard - Device OS / PMIC / AB1805
behaviour observed from field telemetry after a reset, not from a unit test.
Not fixable by refactoring.

**Code-defect WOs** (clock trust, hibernate observability, wake sequencing,
occupancy clock-read) took the rounds they did partly from genuine domain
complexity and partly from coupling at the supervisor / `State_Sleep` /
persistence / sensor-manager intersection. This part is addressable.

The current PR (7,997 insertions / 23 deletions, 40 files, tests+docs >50% of
the diff) reads as **additive operational tissue, not simplification** - a
system growing a second nervous system around paths it cannot yet isolate.

## Flagged as off-base in how this was being evaluated

- LOC (20k) and translation-unit count are not a complexity verdict for this
  device class - the issue is *concentration*, not *census*.
- Header fan-in alone proves nothing (`device_pinout.h` at 12 is fine); what is
  *inside* the high-fan-in header is the smell.
- "17/18 tests pass" is hygiene, not architectural health. The suite is
  policy/unit/shell witnesses, not an integration harness that can answer "did
  this wake preserve clock trust, then sleep, then survive a reset, then publish
  the right startup snapshot."
- Retained-state count alone is not a smell for this device class.
- WO round-count is noisy; do not read it as a code-quality signal without
  separating by WO class.

## Bottom line

Runs an unattended outdoor counter fleet today - already happening. Not yet a
platform that extends without a coupling tax, paid at the main / sleep /
persistence / sensor-manager intersection.

**Do not rip it apart. Do not keep adding observability and retained fields into
the four cores and call the result a platform.** The next increment that
improves maintainability is shrinking those cores' responsibilities - not
another forensic counter.

## Next session - agreed direction

Fix the sleep/time/persistence ownership boundary **before** picking up the next
WO. If "what time is it after this wake" gets a single clear owner, the WOs
paying the coupling tax should proceed faster rather than each re-discovering
the same cross-file coupling.

To be scoped properly, not started tonight:

1. **Name the owner.** Decide which single module is authoritative for post-wake
   clock state - likely a consolidated `time/` component that `State_Sleep.cpp`
   and the supervisor *call into*, rather than each independently reading and
   writing retained time fields and `sysStatus`.
2. **Audit `MyPersistentData.h`'s 22 includers.** Determine which need the whole
   persistence bag versus one or two fields, as a first step toward splitting by
   concern (mode/recovery vs forensics vs webhook/thermal config).
3. **Resolve or delete the stub modules** (`ConfigMerge.cpp`, `StateHandlers.cpp`,
   empty `SensorData.h`) - cheap, low-risk, and they currently misrepresent the
   structure to anyone reading fresh.
4. **Check whether `SessionState` (RAM) and retained forensics can disagree**
   after an unexpected reset; resolve as part of establishing clock/sleep
   ownership.
5. Only then resume the open WOs, against a narrower interface.

---

# Verification of the review's factual claims (2026-09-03, read-only)

Checked against the working tree before adopting this as a plan.

## Confirmed exactly

| Claim | Result |
|---|---|
| `ConfigMerge.cpp` is a one-line include stub | **1 line**: `#include "cloud/Cloud.h"` |
| `StateHandlers.cpp` is a refactor note | **9 lines**, "This file has been refactored." |
| `SensorData.h` is empty | **0 lines** |
| `BatteryAuthorityPolicy` under `sensors/` | `src/sensors/BatteryAuthorityPolicy.{h,cpp}` |
| PMIC forensic retained fields declared from `SensorManager.h` | `SensorManager.h:24-29`, 6 `extern retained` |
| `MyPersistentData.h` mixes concerns | **150 scalar fields**; `Mode`, `Recovery`, `Thermal`, `ThermalThresholds`, `thermalChargeArmHighC/LowC`, `stage`, 22 `watchdog` and 12 `webhook` matches |
| `PROJECT_STATUS.md` lists codebase simplification | line 139, "## Priority 3 - Codebase Simplification" |
| 12 of 14 build flags default off | Confirmed; `ENABLE_PMIC_FORENSICS` and `ENABLE_DIAGNOSTICS_PUBLISH_MODE` are the two ON |
| 27 retained declarations across 6 files | Confirmed |
| One pre-existing test failure | `reporting_policy_adapter_test.sh`, byte-identical to `main`, missing `-I` for vendored `StorageHelperRK.h` |

## Not supported as attributed - substance stands, citation does not

**Claim:** the `SessionState` / retained-forensics divergence is "flagged by the
existing `ARCHITECTURE_OVERVIEW.md` as an open question."

`ARCHITECTURE_OVERVIEW.md` exists at the repo root but contains **no mention of
`SessionState`**, and has no open-questions section. The same is true of
`docs/architecture-overview.md`.

**The underlying concern is nonetheless real and worth item 4.** `SessionState`
is declared at `state/StateMachine.h:50` with a global `session`
(`Generalized-Core-Counter.cpp:154`) and is **plain RAM, not `retained`** - so it
is zeroed by any reset while the retained forensics fields survive. The two
*can* disagree after an unexpected reset by construction. Keep the item; drop
the citation.
