#!/usr/bin/env python3
"""
WO-2026-09-14-001 - structural regression test for the reporting-adapter's
persistence seam.

Background: src/reporting/RuntimeReportingPolicy.cpp used to include
"../MyPersistentData.h" - a RELATIVE quoted include, which resolves against
the including file's own directory before any -I search path is consulted.
That made the include unshadowable by a test's stub override directory, and
is why tests/reporting_policy_adapter_test.sh could not compile from
2026-08-28 onward: pulling in the real persistence header also pulls in
StorageHelperRK.h and the rest of the Device OS surface the test's stub
tree exists to avoid.

The fix narrows RuntimeReportingPolicy.cpp's dependency to exactly the one
accessor it needs (sysStatus.get_currentBatteryTier(), in production) via a
new seam, src/reporting/BatteryTierStore.h/.cpp, included non-relatively so
a test's -I override directory can shadow it.

This is a source-invariant check on the REAL checked-in files, not a mirror.
It asserts the two properties that made the original defect possible, so a
future edit cannot silently reintroduce either half of it:

  1. RuntimeReportingPolicy.cpp does not reference the persistence header
     (by any spelling) - if it does, the seam has been bypassed and the
     compile-shadowing defect this WO fixed is back.
  2. BatteryTierStore.h - the seam itself - does not #include the
     persistence header or StorageHelperRK.h. If it did, anything that
     includes ONLY this narrow seam (as the test stub tree does) would
     transitively regain the same Device OS dependency the seam exists to
     avoid, defeating the whole point of the narrowing.

Comments are stripped before each check so prose (this file's own docstring
included, if it were ever pasted into the source) cannot produce a false
positive or a false negative.
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
ADAPTER_SOURCE = REPO_ROOT / "src" / "reporting" / "RuntimeReportingPolicy.cpp"
SEAM_HEADER = REPO_ROOT / "src" / "reporting" / "BatteryTierStore.h"
SEAM_SOURCE = REPO_ROOT / "src" / "reporting" / "BatteryTierStore.cpp"

# Matches "MyPersistentData.h" or "StorageHelperRK.h" by filename stem, not by
# a specific #include spelling - this must catch a relative include, a
# non-relative include, or any other form a future edit might reintroduce.
PERSISTENCE_HEADER_PATTERN = re.compile(r"MyPersistentData\.h")
STORAGE_HELPER_PATTERN = re.compile(r"StorageHelperRK\.h")


def strip_comments(code: str) -> str:
    """Remove // line comments and /* */ block comments so prose mentioning
    these filenames (explaining why they must NOT appear, as in this test's
    own docstring if pasted into the source, or in the source's own
    rationale comments) does not produce a false positive."""
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)
    code = re.sub(r"//[^\n]*", "", code)
    return code


def fail(msg):
    print(f"FAIL: {msg}")
    sys.exit(1)


def main():
    if not ADAPTER_SOURCE.is_file():
        fail(f"{ADAPTER_SOURCE} does not exist")
    if not SEAM_HEADER.is_file():
        fail(f"{SEAM_HEADER} does not exist")
    if not SEAM_SOURCE.is_file():
        fail(f"{SEAM_SOURCE} does not exist")

    adapter_code_only = strip_comments(ADAPTER_SOURCE.read_text())
    seam_header_code_only = strip_comments(SEAM_HEADER.read_text())
    seam_source_code_only = strip_comments(SEAM_SOURCE.read_text())

    # --- Invariant 1: the adapter does not reference the persistence
    # header, in code (comments are allowed to mention it, e.g. to explain
    # this history - only the stripped code is checked). ---
    if PERSISTENCE_HEADER_PATTERN.search(adapter_code_only):
        fail(
            f"{ADAPTER_SOURCE} references MyPersistentData.h in code - the "
            "narrow BatteryTierStore seam has been bypassed, which "
            "reintroduces the exact compile-shadowing defect "
            "WO-2026-09-14-001 fixed (a relative include of the persistence "
            "header cannot be shadowed by a test's -I override directory)"
        )

    # --- Invariant 2: the seam header stays narrow - it must not #include
    # the persistence header or StorageHelperRK.h. If it did, any file that
    # includes ONLY this seam (as the test stub tree does) would
    # transitively regain the Device OS dependency the seam exists to
    # avoid. ---
    if PERSISTENCE_HEADER_PATTERN.search(seam_header_code_only):
        fail(
            f"{SEAM_HEADER} references MyPersistentData.h in code - the seam "
            "is supposed to declare only currentBatteryTier() and leave the "
            "persistence header to the .cpp; a reference here defeats the "
            "point of the narrowing"
        )
    if STORAGE_HELPER_PATTERN.search(seam_header_code_only):
        fail(
            f"{SEAM_HEADER} references StorageHelperRK.h in code - same "
            "failure mode as above, one level more direct"
        )

    # --- Invariant 3 (positive control): the seam's .cpp DOES include the
    # persistence header - i.e. the accessor is actually implemented against
    # real persisted state, not stubbed out or left disconnected. Without
    # this check, satisfying invariants 1/2 by simply deleting the
    # implementation would pass silently. ---
    if not PERSISTENCE_HEADER_PATTERN.search(seam_source_code_only):
        fail(
            f"{SEAM_SOURCE} does not reference MyPersistentData.h - the seam "
            "implementation must be backed by the real persisted "
            "currentBatteryTier, not stubbed out here"
        )

    print("OK: RuntimeReportingPolicy.cpp does not reference the persistence header")
    print("OK: BatteryTierStore.h stays narrow (no persistence header, no StorageHelperRK.h)")
    print("OK: BatteryTierStore.cpp implements the seam against the real persisted state")
    print("battery_tier_store_seam_structural_test: all invariants hold")


if __name__ == "__main__":
    main()
