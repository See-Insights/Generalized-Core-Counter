# WO-2026-09-16-002: `HibernateCycle` - single owner of the hibernate-sleep/wake lifecycle

**Type:** Structural fix (Step 2 of the Structural Ownership Map roadmap).

**Status:** CLOSED 2026-09-18. Code complete, fully tested on the host and
via cloud compile (Boron, target 6.4.1), and bench-validated on Dev-09 and
Dev-14 across two nights each - see Bench validation below. No behaviour
change was intended, and none was observed.

## The observation

Before this step, five retained fields
(`retainedHibernateRtcBefore`/`WakeTime`/`RequestedSleep`/`Count`/`Pending`)
were declared in `Generalized-Core-Counter.cpp`, armed by `State_Sleep.cpp`
immediately before a HIBERNATE sleep, and read back by
`Generalized-Core-Counter.cpp` on the wake boot - three files sharing one
lifecycle with no single owner, exactly the "no single owner of what time
is it after this wake" finding from `docs/architecture-review-2026-09-03.md`.
Separately, `setup()` read `ab1805.getWakeReason()` at two different
points - once for PIN_RESET/AB1805-watchdog confirmation, once for the
hibernate wake-validation gate (owned by `WO-2026-09-14-002` Step 1's
`classifyGateArm()`) - two independently-timed reads of the same
underlying classification.

## The fix

New module, `time/HibernateCycle.{h,cpp}`, now owns the whole lifecycle:

- **The five retained fields become four.** `retainedHibernateWakeTime` was
  confirmed dead - a full-tree search found no reader anywhere - and was
  not carried forward. The remaining four are declared in
  `HibernateCycle.cpp`'s own anonymous namespace, with no `extern`
  anywhere else in the tree, so single ownership is enforced by the
  language, not convention.
- **`HibernateCycle::armForSleep(rtcNow, requestedSec)`** replaces
  `State_Sleep.cpp`'s inline `retainedHibernate*`/`++` sequence. Note the
  signature drops the `wakeAt` parameter the roadmap's original sketch
  included - since `retainedHibernateWakeTime` wasn't carried forward,
  there was nothing left for a parameter to feed; `State_Sleep.cpp` still
  computes `wakeTime` locally for `ab1805.interruptAtTime()`, it just isn't
  passed through.
- **`HibernateCycle::abandon()`** replaces both
  `retainedHibernatePending = false;` sites - the wake-boot's post-gate
  clear, and the `State_Sleep.cpp` fallback path where a HIBERNATE
  `System.sleep()` call returns instead of resetting the MCU. The fallback
  site is easy to miss since it isn't on the happy path; both are now one
  function.
- **`HibernateCycle::classifyWake(osResetReason, ab1805)`** absorbs both
  `ab1805.getWakeReason()` consumers into one call. This is safe: per
  `AB1805::setup()`'s own contract, `getWakeReason()` is a plain getter
  onto the result of the ONE destructive `updateWakeReason()` call already
  made during setup - reading it twice is not a second destructive read,
  unlike calling `updateWakeReason()` itself twice would be (the Stage 7
  bug `watchdog_ab1805_classification_test.cpp` guards against).
- **`classifyGateArm()` no longer exists as a separately callable
  function.** Its six-condition logic is `classifyWake()`'s own
  implementation now - "absorb, don't wrap," not a second thing calling
  into a third thing. `HibernateWakeDiagnostics.h` keeps `GateInputs`/
  `GateArm`/`EventFields`/`buildEventFields()`/`buildEventPayload()` - the
  shared, Particle-free data and cloud-event-rendering layer both
  `HibernateCycle` and `Generalized-Core-Counter.cpp` still use.
  `buildEventFields()` takes the already-classified `GateArm` as an
  explicit parameter instead of re-deriving it internally, removing a
  redundant second classification call it used to make on every boot.
- **`retainedHibernateCount` serves both of its confirmed consumers**
  through `HibernateCycle::currentHibernateCount()` - the gate-block reads
  and `publishStartupStatus()`'s `status`-event read, ~1,400 lines away
  from the gate and easy to miss. No raw `retainedHibernateCount` reference
  remains outside `HibernateCycle.cpp`.

### A linkage fix surfaced by the cloud compile, not a test

`classifyWake()` needs `ab1805WakeReasonName()`, previously called only
from within `Generalized-Core-Counter.cpp`'s own `setup()`. That function
turned out to be defined inside `Generalized-Core-Counter.cpp`'s anonymous
namespace - internal linkage, invisible to `HibernateCycle.cpp` - which the
linker caught (`undefined reference to ab1805WakeReasonName(...)`), not a
compiler error and not something any host test could have caught. Fixed by
splitting it into `ab1805WakeReasonNameLocal()` (kept in place) plus a thin
external-linkage wrapper, mirroring the `failsafeDeferReasonName()`/
`failsafeDeferReasonNameLocal()` pattern already established in that same
file. Zero logic change.

### The breadcrumb cleanup was reverted, not completed

Folding in the Q6 breadcrumb-literal cleanup "since `State_Sleep.cpp` was
open anyway" hit a real compile failure: `AppBreadcrumb`'s named constants
are declared inside `Generalized-Core-Counter.cpp`'s anonymous namespace,
invisible to `State_Sleep.cpp` (and `State_Connect.cpp`, which turned out
to have two more raw-numeric call sites of its own). This is a structural
fix - the enum needs a real cross-translation-unit home - not a text
substitution, and touches more than `State_Sleep.cpp` alone, so it was
reverted rather than expanded mid-step. `State_Sleep.cpp` and
`sleep_breadcrumb_sequence_test.py` are both byte-identical to their
pre-Step-2 state. **Filed separately as `WO-2026-09-16-001`** - deliberately
scoped out, not missed.

## Verification

- **Structural test.** `tests/hibernate_cycle_single_owner_structural_test.py`
  asserts each of the four carried-forward fields is `retained`-declared
  exactly once, in `HibernateCycle.cpp`, with no `extern` anywhere else,
  and that the dropped field was not quietly re-added. Mutation-tested:
  added a second declaration elsewhere, confirmed the test failed for the
  correct reason, restored byte-identical.
- **Existing tests updated.** `hibernate_wake_diagnostics_test.cpp` now
  carries its own manually-maintained mirror of the absorbed six-condition
  classification (the same pattern `watchdog_ab1805_classification_test.cpp`
  already used for the PIN_RESET half), since the real function it used to
  test directly no longer exists as an exported symbol.
  `watchdog_ab1805_classification_test.sh`'s fidelity checks re-pointed to
  `HibernateCycle.cpp`. `wake_gate_single_owner_structural_test.py` (Step
  1's own test) updated to assert a `HibernateCycle::classifyWake()` call
  site instead of `classifyGateArm()` - a direct, mechanical consequence of
  this step's authorized absorption - plus a title/docstring fix so a
  future reader isn't confused finding a "Step 1" test enforcing a Step 2
  property.
- **Full suite.** 29/29, no regressions.
- **Cloud compile.** `particle compile boron . --target 6.4.1` succeeds.
  Flash 149614 / RAM 3406 (Step 1 baseline: 149550 / 3414).
- **`SensorManager.cpp:490` confirmed untouched** - byte-identical
  (sha256 match) to `main`. Explicit out-of-scope decision: it fires
  battery stabilization on the raw OS reset reason, not the stricter
  gate-passed notion `HibernateCycle` defines; swapping it would be a real
  behaviour change hiding inside what would look like a refactor.

## Bench validation

Two nights each, post-flash (commit `0ad3206`, sha256
`a2de9dd1daa824c6e47336494b5a39f0414fd36918400513f8510add2ab7f4d5`), Dev-09
and Dev-14, flashed together from the same binary at 8:04 SGT 2026-09-16.

| | Dev-14 Night 1 | Dev-14 Night 2 | Dev-09 Night 1 | Dev-09 Night 2 |
|---|---|---|---|---|
| result | ok | ok | fail | fail |
| gateArm | none | none | wake_reason | wake_reason |
| wakeReason | ALARM | ALARM | DEEP_POWER_DOWN | DEEP_POWER_DOWN |
| rtcOk | 1 | 1 | 1 | 1 |
| req | 28797 | 28797 | 28798 | 28798 |
| actual | 28798 | 28798 | 0 | 0 |
| err | 1 | 1 | 0 | 0 |
| count | 1 | 2 | 1 | 2 |

**No connectivity stall this round** - both devices' events published
promptly both nights (unlike Step 1's night 1, where Dev-09 spent ~2h44m
stuck in the `CELLULAR_ACQUIRE` registration stall before its
already-correct event reached the cloud). **`count` incremented cleanly,
1->2, on both devices, both nights - no anomaly requiring investigation**,
in clean contrast to Step 1's antenna-swap complication (a deliberate
manual power cycle mid-week reset Dev-09's retained state that week; no
such disturbance happened here). No watchdog events either night on
either device. Both devices otherwise healthy throughout (batteries
80-82%, actively reporting).

**Dev-09's `DEEP_POWER_DOWN` classification held unchanged across the code
move** - the specific acceptance case the roadmap named for this step:
proof that moving the classification out of `setup()` and into
`HibernateCycle::classifyWake()` did not change what it decides, even for
the device that has failed this gate every single night on record.

Dev-11 was deliberately held out of this round - reserved as the
acceptance device for Step 3b, and Step 2 touches nothing in its
clock-trust path, so there was no signal to gain from disturbing it.

## Not in scope for this step

Named per the roadmap and not touched here: `time/Q1`/clock ownership
(Step 3), `BatteryAuthority` (Step 4), the persistence-header split (Step
5), `sensors/SensorManager.cpp:490` (explicit decision, see Verification
above). The breadcrumb-enum cross-TU-visibility fix is filed separately as
`WO-2026-09-16-001` - see above.

## Provenance

Scoped by `docs/architecture-review-2026-09-03.md` and the Structural
Ownership Map roadmap agreed 2026-09-03 (Step 2 of that roadmap).
Implemented and bench-validated 2026-09-16 through 2026-09-18 on branch
`wo/2026-09-16-step2-hibernate-cycle-owner`, commits `9ac41bf` (the
`HibernateCycle` implementation) and `0ad3206` (the sibling
`WO-2026-09-16-001` finding, filed in the same arc rather than treated as a
separate mystery - the same discipline `WO-2026-09-14-001`'s Step 0/Step
0.5 established for this project).

## Related work orders

- `docs/architecture-review-2026-09-03.md` - source of the roadmap this
  step is item 2 of.
- `WO-2026-09-14-002` - Step 1, established the single-decision-site
  pattern this step absorbed further.
- `WO-2026-09-16-001` - the `AppBreadcrumb` cross-TU-visibility gap
  surfaced (and deliberately not fixed) during this step's Q6 cleanup
  attempt.
- `WO-2026-08-31-002` Amendment B - the `DEEP_POWER_DOWN` finding that now
  has one place to land (`HibernateCycle::classifyWake()`), still not
  itself implemented.
