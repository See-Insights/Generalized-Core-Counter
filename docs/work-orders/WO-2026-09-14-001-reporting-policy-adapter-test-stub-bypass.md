# WO-2026-09-14-001: `reporting_policy_adapter_test.sh` has not compiled since 2026-08-28 - CLOSED

**Type:** Investigation and fix.

**Status:** CLOSED 2026-09-14. Both steps implemented, tested, and committed.
`tests/reporting_policy_adapter_test.sh` compiles, links, and passes for real -
see Resolution below.

**Severity (at filing):** The test did not run at all. It was not failing an
assertion - it failed to compile, and had done since `516332a` (2026-08-28, the
v24 release). A green suite had not included it since that date.

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

## Candidate directions - Direction 1 chosen (see Resolution)

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

## Resolution (2026-09-14)

Direction 1 chosen and implemented, in two dispatches ("Step 0" and
"Step 0.5"), because a second, structurally identical defect surfaced
partway through and was resolved in the same arc rather than treated as a
new mystery - see below.

### Step 0 - the persistence header

Gave `RuntimeReportingPolicy.cpp` a narrow seam,
`src/reporting/BatteryTierStore.h`/`.cpp`, exposing exactly the one accessor
it uses (`sysStatus.get_currentBatteryTier()`), included non-relatively so a
test's `-I` override directory can shadow it. The old, now-dead
`MyPersistentData.h` stub was removed. A structural test,
`tests/battery_tier_store_seam_structural_test.py` (same pattern as
`thermal_coupling_structural_test.py`), asserts the adapter never
references the persistence header directly and that the seam stays narrow.
Committed as `7d412c4`.

Running the actual test script at the end of Step 0 surfaced a **second**
failure - not a regression in this fix, but a defect of the exact same
shape that had been sitting one layer beneath the first, invisible until
the first was gone:

```
Undefined symbols for architecture arm64:
  "Config::reportingIntervalSecForRuntime()", referenced from:
      ReportingPolicyResolver::resolveRuntime(float, long) in RuntimeReportingPolicy-*.o
ld: symbol(s) not found for architecture arm64
```

`RuntimeReportingPolicy.cpp:6` also had `#include "../Config.h"` - the same
kind of relative, unshadowable include, this time of the header documented
in this WO's own "What is actually wrong" section as deliberately relative
for a different reason (the Device OS name collision). `Config.h` only
*declares* `reportingIntervalSecForRuntime()`; the definition lives in
`Config.cpp`, which was never on the test's compile line. The fatal
compile error fixed in Step 0 had aborted the build before this link-time
defect could ever be reached, so it was completely masked until that
point - not a new bug introduced by Step 0, and not something the original
diagnosis above missed; it was structurally unobservable until then.

Step 0's own verification of its mutation test (Done Condition #2) had to
run against an isolated, hand-built host harness rather than the real
script, specifically because this second defect still blocked the real
script from linking at that point.

### Step 0.5 - the shared configuration header

Same treatment, same file, comparably small - authorized explicitly as a
sibling fix rather than a scope expansion. A new seam,
`src/reporting/ReportingIntervalStore.h`/`.cpp` - a sibling to
`BatteryTierStore`, not folded into it, since it wraps an unrelated module
and a combined file would no longer be named for what it checks - narrows
the dependency to `reportingIntervalSecForRuntime()` alone. The seam's own
`.cpp` keeps the relative include of `Config.h`, matching every other
`src/reporting/`, `src/state/`, and `src/cloud/` file's convention for it;
only the adapter's now-indirect dependency needed narrowing. A sibling
structural test, `tests/reporting_interval_store_seam_structural_test.py`,
guards it the same way. Committed as `c8afbc1`.

With both seams in place, `tests/reporting_policy_adapter_test.sh` compiles,
links, and passes for real - confirmed against the actual script, not a
harness:

```
reporting_policy_adapter_test: all tests passed
```

Both steps' mutation tests (`TIER_SURVIVAL -> TIER_HEALTHY` in the
Invalid-vcell branch) were re-verified against the real script at the end
of Step 0.5, producing the correct assertion failure, then restored
byte-identical (hash-verified). Full suite: 27/27, with no regressions
against the pre-existing baseline.

Neither step touched any other includer of the persistence header or of
`Config.h`; neither changed persisted layout, added a retained field, or
changed runtime behavior.

## Acceptance

- [x] `tests/reporting_policy_adapter_test.sh` compiles and passes. Confirmed
  after Step 0.5 - the real script, not an isolated harness: `reporting_policy_adapter_test: all tests passed`, exit 0.
- [x] Mutating the adapter's Invalid/Unavailable vcell handling makes it fail,
  for the right reason, before any fix is applied. Done twice: once against an
  isolated harness after Step 0 (the real script could not yet run), and once
  for real against the actual script after Step 0.5. Both times:
  `TIER_SURVIVAL -> TIER_HEALTHY` in the Invalid-vcell branch produced
  `Assertion failed: (policy.batteryTier == TIER_SURVIVAL), function
  testInvalidVcellForcesSurvivalRegardlessOfRawSoc`, not a compile or link
  error. Both times restored byte-identical, hash-verified before and after.
- [x] No copied production source that can drift from the real file without
  detection. The two seams (`BatteryTierStore`, `ReportingIntervalStore`) are
  narrow accessor wrappers, not copies - each backed by a structural test
  (`battery_tier_store_seam_structural_test.py`,
  `reporting_interval_store_seam_structural_test.py`) that parses the real
  shipped source and fails if the seam is bypassed or widened, in the same
  pattern as `thermal_coupling_structural_test.py`.
- [ ] **Not done:** `tests/README.md` still describes the adapter test only in
  terms of WO-2026-08-25-001 Amendment C; it says nothing about the seam
  mechanism this WO added or that the file's host-compilability now depends on
  two narrow headers rather than the whole persistence bag. Left as an honest
  gap rather than silently closed or silently fixed outside the scope that was
  authorized for Steps 0/0.5.

## Provenance

Found while running the full suite during `WO-2026-08-31-003` round 5
verification, 2026-09-14. Diagnosis by `clang++ -H` resolution trace and by
attempting the naive `-I` fix. Regression window established from
`git log -1 -- tests/reporting_policy_adapter_test.sh` (`516332a`, 2026-08-28);
the test's compile line has not been touched since, so the break was no
later than that commit. Was not blocking any in-flight work order while
open.

Resolved 2026-09-14 on branch `wo/2026-09-14-001-reporting-adapter-seams`,
commits `7d412c4` (Step 0) and `c8afbc1` (Step 0.5).

## Related work orders

- `WO-2026-08-25-001` Amendment C - the guard this test now has working
  coverage for again.
- `docs/architecture-review-2026-09-03.md` - `MyPersistentData.h`'s fan-in was
  the structural reason Direction 1 was preferred, and remains the reason a
  narrow-seam approach (rather than a copied stub tree or a preprocessor
  override) is the right default for the next file found in this shape.
