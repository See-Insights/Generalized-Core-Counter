#!/usr/bin/env python3
"""
WO-2026-09-14-001 / WO-2026-09-21 Step 4 - structural regression test for the
reporting-adapter's persistence seam, and for BatteryAuthority's own query/
command split.

Background: src/reporting/RuntimeReportingPolicy.cpp used to include
"../MyPersistentData.h" - a RELATIVE quoted include, which resolves against
the including file's own directory before any -I search path is consulted.
That made the include unshadowable by a test's stub override directory, and
is why tests/reporting_policy_adapter_test.sh could not compile from
2026-08-28 onward: pulling in the real persistence header also pulls in
StorageHelperRK.h and the rest of the Device OS surface the test's stub
tree exists to avoid.

WO-2026-09-14-001's fix narrowed RuntimeReportingPolicy.cpp's dependency to
exactly the one accessor it needed (sysStatus.get_currentBatteryTier(), in
production) via src/reporting/BatteryTierStore.h/.cpp, included
non-relatively so a test's -I override directory could shadow it.

WO-2026-09-21 Step 4 folded that seam into the new battery tier/low-battery
owner - RuntimeReportingPolicy.cpp now depends on src/power/BatteryAuthority.h
instead, and reporting/BatteryTierStore.h/.cpp were deleted - then was
corrected the same day to split BatteryAuthority into a PURE query
(evaluate(), in src/power/BatteryAuthority.cpp - genuinely host-compilable,
no persisted reads at all, not merely a narrowed dependency) and a command
(commit()/currentTier()/clearLowBatteryMode(), in
src/power/BatteryAuthorityCommand.cpp - where the real persisted-tier
seam now lives). Renamed from battery_tier_store_seam_structural_test.py;
the underlying concern (the adapter must not directly depend on the heavy
persistence header; the seam header must stay narrow; the seam's
implementation must be backed by real persisted state) is unchanged, and
this file adds the stronger query-side guarantee the split introduced.

This is a source-invariant check on the REAL checked-in files, not a mirror.
It asserts the properties that made the original defect possible, and the
new one the query/command split promises, so a future edit cannot silently
reintroduce or weaken any of them:

  1. RuntimeReportingPolicy.cpp does not reference the persistence header
     (by any spelling) - if it does, the seam has been bypassed and the
     compile-shadowing defect WO-2026-09-14-001 fixed is back.
  2. power/BatteryAuthority.h (the shared, public interface header) does not
     #include the persistence header or StorageHelperRK.h. If it did, any
     file that includes only this header (as the test stub tree does) would
     transitively regain the Device OS dependency the seam exists to avoid.
  3. power/BatteryAuthority.cpp - evaluate()'s own translation unit - does
     not reference the persistence header or StorageHelperRK.h either. This
     is the stronger, Step-4-correction-specific guarantee: not just a
     narrowed dependency behind an accessor, but genuinely zero persisted
     state in the query path's implementation.
  4. power/BatteryAuthorityCommand.cpp - the seam's real implementation
     (positive control) - DOES reference the persistence header, i.e.
     currentTier()/commit() are actually backed by real persisted state, not
     stubbed out or left disconnected. Without this check, satisfying
     invariants 2/3 by simply deleting the implementation would pass
     silently.

Comments are stripped before each check so prose (this file's own docstring
included, if it were ever pasted into the source) cannot produce a false
positive or a false negative.
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
ADAPTER_SOURCE = REPO_ROOT / "src" / "reporting" / "RuntimeReportingPolicy.cpp"
SEAM_HEADER = REPO_ROOT / "src" / "power" / "BatteryAuthority.h"
EVALUATE_SOURCE = REPO_ROOT / "src" / "power" / "BatteryAuthority.cpp"
COMMAND_SOURCE = REPO_ROOT / "src" / "power" / "BatteryAuthorityCommand.cpp"

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
    for path in (ADAPTER_SOURCE, SEAM_HEADER, EVALUATE_SOURCE, COMMAND_SOURCE):
        if not path.is_file():
            fail(f"{path} does not exist")

    adapter_code_only = strip_comments(ADAPTER_SOURCE.read_text())
    seam_header_code_only = strip_comments(SEAM_HEADER.read_text())
    evaluate_source_code_only = strip_comments(EVALUATE_SOURCE.read_text())
    command_source_code_only = strip_comments(COMMAND_SOURCE.read_text())

    # --- Invariant 1: the adapter does not reference the persistence
    # header, in code (comments are allowed to mention it, e.g. to explain
    # this history - only the stripped code is checked). ---
    if PERSISTENCE_HEADER_PATTERN.search(adapter_code_only):
        fail(
            f"{ADAPTER_SOURCE} references MyPersistentData.h in code - the "
            "narrow BatteryAuthority seam has been bypassed, which "
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
            "header is supposed to declare only evaluate()/currentTier()/"
            "commit()/clearLowBatteryMode() and leave the persistence header "
            "to the .cpp files; a reference here defeats the point of the "
            "narrowing"
        )
    if STORAGE_HELPER_PATTERN.search(seam_header_code_only):
        fail(
            f"{SEAM_HEADER} references StorageHelperRK.h in code - same "
            "failure mode as above, one level more direct"
        )

    # --- Invariant 3: evaluate()'s own translation unit references neither
    # header - the query half of the query/command split must be genuinely
    # persistence-free, not merely hidden behind a narrowed accessor. ---
    if PERSISTENCE_HEADER_PATTERN.search(evaluate_source_code_only):
        fail(
            f"{EVALUATE_SOURCE} references MyPersistentData.h in code - "
            "evaluate() must be pure (no persisted reads/writes at all), not "
            "merely narrowed behind an accessor"
        )
    if STORAGE_HELPER_PATTERN.search(evaluate_source_code_only):
        fail(
            f"{EVALUATE_SOURCE} references StorageHelperRK.h in code - same "
            "failure mode as above, one level more direct"
        )

    # --- Invariant 4 (positive control): the command half DOES include the
    # persistence header - i.e. currentTier()/commit() are actually
    # implemented against real persisted state, not stubbed out or left
    # disconnected. Without this check, satisfying invariants 2/3 by simply
    # deleting the implementation would pass silently. ---
    if not PERSISTENCE_HEADER_PATTERN.search(command_source_code_only):
        fail(
            f"{COMMAND_SOURCE} does not reference MyPersistentData.h - the "
            "command implementation must be backed by the real persisted "
            "currentBatteryTier/lowBatteryMode/connectionMode/sensorMode, "
            "not stubbed out here"
        )

    print("OK: RuntimeReportingPolicy.cpp does not reference the persistence header")
    print("OK: BatteryAuthority.h stays narrow (no persistence header, no StorageHelperRK.h)")
    print("OK: BatteryAuthority.cpp (evaluate()'s translation unit) is genuinely persistence-free")
    print("OK: BatteryAuthorityCommand.cpp implements the seam against the real persisted state")
    print("battery_authority_seam_structural_test: all invariants hold")


if __name__ == "__main__":
    main()
