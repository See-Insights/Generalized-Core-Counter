#!/usr/bin/env python3
"""
WO-2026-09-14-001 Step 0.5 - structural regression test for the reporting
adapter's configuration seam.

Background: restoring tests/reporting_policy_adapter_test.sh (Step 0)
exposed a second, previously-masked defect of the identical shape:
src/reporting/RuntimeReportingPolicy.cpp had "#include \"../Config.h\"" - a
RELATIVE quoted include, which resolves against the including file's own
directory before any -I search path is consulted. That made the include
unshadowable by a test's stub override directory. It was invisible until
Step 0 fixed the persistence-header compile error that had previously
aborted the build first; once that error was gone, this include surfaced as
a link-time "symbol not found" error instead (Config::
reportingIntervalSecForRuntime() is declared in Config.h but defined in
Config.cpp, which is not on the test's compile line).

The fix narrows RuntimeReportingPolicy.cpp's dependency to exactly the one
accessor it needs (Config::reportingIntervalSecForRuntime(), in production)
via a new seam, src/reporting/ReportingIntervalStore.h/.cpp, included
non-relatively so a test's -I override directory can shadow it.

This is a source-invariant check on the REAL checked-in files, not a mirror,
in the same pattern as tests/battery_tier_store_seam_structural_test.py (a
sibling test, not folded into that one, since this guards an unrelated
seam - the shared configuration module, not the persistence bag - and a
single file covering both would no longer be named for what it checks). It
asserts the two properties that made the original defect possible, so a
future edit cannot silently reintroduce either half of it:

  1. RuntimeReportingPolicy.cpp does not reference the shared configuration
     header (by any spelling, relative or not) - if it does, the seam has
     been bypassed and the compile-shadowing defect this fix closed is
     back, whether as the old relative include or a new direct one.
  2. ReportingIntervalStore.h - the seam itself - does not #include the
     shared configuration header. If it did, anything that includes ONLY
     this narrow seam (as the test stub tree does) would transitively
     regain the same dependency the seam exists to avoid.

Comments are stripped before each check so prose (this file's own docstring
included, if it were ever pasted into the source) cannot produce a false
positive or a false negative.
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
ADAPTER_SOURCE = REPO_ROOT / "src" / "reporting" / "RuntimeReportingPolicy.cpp"
SEAM_HEADER = REPO_ROOT / "src" / "reporting" / "ReportingIntervalStore.h"
SEAM_SOURCE = REPO_ROOT / "src" / "reporting" / "ReportingIntervalStore.cpp"

# Matches "Config.h" by filename stem, not by a specific #include spelling -
# this must catch the old relative include, a new non-relative include, or
# any other form a future edit might reintroduce.
CONFIG_HEADER_PATTERN = re.compile(r"Config\.h")


def strip_comments(code: str) -> str:
    """Remove // line comments and /* */ block comments so prose mentioning
    this filename (explaining why it must NOT appear, as in this test's own
    docstring if pasted into the source, or in the source's own rationale
    comments) does not produce a false positive."""
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

    # --- Invariant 1: the adapter does not reference the shared
    # configuration header, in code (comments are allowed to mention it,
    # e.g. to explain this history - only the stripped code is checked). ---
    if CONFIG_HEADER_PATTERN.search(adapter_code_only):
        fail(
            f"{ADAPTER_SOURCE} references Config.h in code - the narrow "
            "ReportingIntervalStore seam has been bypassed, which "
            "reintroduces the exact compile-shadowing defect this fix "
            "closed (a relative include of the shared configuration header "
            "cannot be shadowed by a test's -I override directory, and a "
            "non-relative one risks resolving to Device OS's own header of "
            "the same name)"
        )

    # --- Invariant 2: the seam header stays narrow - it must not #include
    # the shared configuration header directly. If it did, any file that
    # includes ONLY this seam (as the test stub tree does) would
    # transitively regain the dependency the seam exists to avoid. ---
    if CONFIG_HEADER_PATTERN.search(seam_header_code_only):
        fail(
            f"{SEAM_HEADER} references Config.h in code - the seam is "
            "supposed to declare only reportingIntervalSec() and leave the "
            "shared configuration header to the .cpp; a reference here "
            "defeats the point of the narrowing"
        )

    # --- Invariant 3 (positive control): the seam's .cpp DOES include the
    # shared configuration header - i.e. the accessor is actually
    # implemented against the real configured interval, not stubbed out or
    # left disconnected. Without this check, satisfying invariants 1/2 by
    # simply deleting the implementation would pass silently. ---
    if not CONFIG_HEADER_PATTERN.search(seam_source_code_only):
        fail(
            f"{SEAM_SOURCE} does not reference Config.h - the seam "
            "implementation must be backed by the real "
            "reportingIntervalSecForRuntime(), not stubbed out here"
        )

    print("OK: RuntimeReportingPolicy.cpp does not reference the shared configuration header")
    print("OK: ReportingIntervalStore.h stays narrow (no shared configuration header)")
    print("OK: ReportingIntervalStore.cpp implements the seam against the real configured interval")
    print("reporting_interval_store_seam_structural_test: all invariants hold")


if __name__ == "__main__":
    main()
