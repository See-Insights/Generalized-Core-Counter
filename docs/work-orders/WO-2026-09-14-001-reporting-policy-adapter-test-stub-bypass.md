# WO-2026-09-14-001: `reporting_policy_adapter_test.sh` has not compiled since 2026-08-28

**Type:** Investigation and fix. The defect is understood; the correct fix is
not yet chosen, and the obvious one is wrong (see below).

**Status:** Drafted, not dispatched. Specification only.

**Severity:** The test does not run at all. It is not failing an assertion - it
fails to compile, and has done since `516332a` (2026-08-28, the v24 release).
A green suite has never included it since that date.

## The observation

`tests/reporting_policy_adapter_test.sh` is the only test in `tests/` that does
not pass. Running it:

```
In file included from src/reporting/RuntimeReportingPolicy.cpp:8:
src/reporting/../MyPersistentData.h:41:10: fatal error: 'StorageHelperRK.h' file not found
#include "StorageHelperRK.h"
         ^~~~~~~~~~~~~~~~~~~
1 error generated.
```

The other 24 shell and Python tests pass, including every other test that uses
the same `tests/stubs/*_overrides` pattern (`power_composition_overrides`,
`pmic_fault_monitor_overrides`, `power_source_override_overrides`,
`clock_status_republish_overrides`, `diag_overrides`). This is isolated to one
test today.

## What is actually wrong

**Not** a missing `-I`. The test ships a deliberate stub at
`tests/stubs/reporting_policy_adapter_overrides/MyPersistentData.h`, whose own
comment states its purpose:

> Minimal host-side stand-in for the `../MyPersistentData.h` that
> `src/reporting/RuntimeReportingPolicy.cpp` includes. Only the one accessor the
> adapter actually calls (`sysStatus.get_currentBatteryTier()`) is provided.

That stub is never consulted. `RuntimeReportingPolicy.cpp:8` includes
`"../MyPersistentData.h"` - a **relative** quoted include, which resolves against
the including file's own directory before any `-I` path is searched. So
`src/reporting/../MyPersistentData.h` always wins and the real persistence
header is compiled, which pulls `StorageHelperRK.h` at line 41.

Confirmed with `clang++ -H`, which prints the resolved chain:

```
. src/reporting/../MyPersistentData.h
.. lib/StorageHelperRK/src/StorageHelperRK.h
```

**A stub directory on the include path cannot shadow a relative-path include.**
The two mechanisms are structurally incompatible, and this test is built on the
assumption that they compose.

The relative include is itself deliberate and documented in place
(`RuntimeReportingPolicy.cpp:3-7`): Device OS ships its own
`services/inc/Config.h`, so a bare `#include "Config.h"` can resolve to the wrong
file under the local toolchain's include order. Relative paths are the stated
convention in `src/state/` and `src/cloud/` for that reason. **Do not "fix" this
by making the include non-relative without addressing that.**

## Why the obvious fix is wrong

Adding `-I"$repo_root/lib/StorageHelperRK/src"` to the test's compile line is the
naive reading of the error message. It does not work, and it would be wrong even
if it did. Verified:

```
lib/StorageHelperRK/src/StorageHelperRK.h:30:17: error: unknown type name 'os_mutex_recursive_t'
lib/StorageHelperRK/src/StorageHelperRK.h:41:30: error: unknown type name 'os_mutex_recursive_t'
```

It cascades into Device OS types, because compiling the real
`MyPersistentData.h` on the host pulls in the entire Particle surface the stub
exists to avoid. Chasing that with more `-I` and more stubs reconstructs the
whole persistence layer on the host to exercise one accessor.

## Why this matters

The test is not incidental coverage. Per `tests/README.md`, it exists to close a
specific gap from `WO-2026-08-25-001` Amendment C (Decision C2 / AC-C6):

> This closes the gap where the guard's unit tests passed in isolation while the
> production adapter still let an Invalid or Unavailable vcell silently bypass
> the trust substitution and the 3.5V floor.

That guard currently has no working host test. The unit tests for
`BatteryTierGuard` still pass, which is exactly the isolation the adapter test
was written to see past - so the suite looks green on the thing this test was
built not to trust.

This is also a repeat of a named pattern in `particle-fleet-operations`
`docs/STYLE_GUIDE.md` §5: *"A test or mock that cannot fail, or that passes on
both sides of a known defect, is not a passing test - it's a missing one."* A
test that cannot compile is the degenerate case.

## Candidate directions (not chosen - investigation first)

1. **Make the seam explicit.** Give `RuntimeReportingPolicy.cpp` a narrow
   dependency it can be compiled against on the host (an interface or a single
   accessor header) rather than the whole persistence bag, so no shadowing is
   needed. Most aligned with the sleep/time/persistence ownership work already
   scheduled - `MyPersistentData.h` has the repo's highest fan-in and this is one
   of its costs.
2. **Compile the adapter against a stub tree that mirrors the relative path** -
   i.e. put the stub where `../MyPersistentData.h` resolves to, from a copied
   source tree. Works without touching production, but adds a copied tree, which
   this repo's testing convention has repeatedly rejected in favour of testing
   the real source.
3. **Preprocessor seam** (e.g. a test-only include guard override). Cheapest,
   and the easiest to get subtly wrong; it changes what production compiles.

Direction 1 is the one worth scoping first. Whichever is chosen, the acceptance
condition is that the test **fails** when the adapter's trust substitution or
3.5V floor is mutated - per §5, confirm it fails for the correct reason before
the fix is written.

## Acceptance

- `tests/reporting_policy_adapter_test.sh` compiles and passes.
- Mutating the adapter's Invalid/Unavailable vcell handling makes it fail, for
  the right reason, before any fix is applied.
- No copied production source that can drift from the real file without
  detection.
- `tests/README.md` states what the test covers and what it does not.

## Provenance

Found while running the full suite during `WO-2026-08-31-003` round 5
verification, 2026-09-14. Diagnosis by `clang++ -H` resolution trace and by
attempting the naive `-I` fix. Regression window established from
`git log -1 -- tests/reporting_policy_adapter_test.sh` (`516332a`, 2026-08-28);
the test's compile line has not been touched since, so the break is no later
than that commit. Not blocking any in-flight work order.

## Related work orders

- `WO-2026-08-25-001` Amendment C - the guard this test exists to cover.
- `docs/architecture-review-2026-09-03.md` - `MyPersistentData.h`'s fan-in is the
  structural reason direction 1 is preferred.
