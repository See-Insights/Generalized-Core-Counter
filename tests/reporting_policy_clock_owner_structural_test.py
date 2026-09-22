#!/usr/bin/env python3
"""
WO-2026-09-22: structural regression test for RuntimeReportingPolicy.cpp
routing its clock-trust input through Clock::isTrusted(), not raw
Time.isValid().

Background: RuntimeReportingPolicy.cpp set ReportingPolicyInputs.timeValid
directly from Time.isValid(), which ReportingPolicy.cpp then used to decide
whether a report schedule could be computed at all. This is the same class
of gap Step 3b's decision-site conversion closed everywhere else in the
codebase (see tests/clock_owner_structural_test.py, which already asserts
zero Time.isValid() decision sites under src/state/) - missed here because
this file was not in that sweep's original site list.

This is a source-invariant check on the REAL checked-in files, not a mirror.
It asserts the properties that made the original gap possible, so a future
edit cannot silently reintroduce it:

  1. RuntimeReportingPolicy.cpp does not call Time.isValid() in code (a
     comment explaining this history is fine - only stripped code is
     checked).
  2. RuntimeReportingPolicy.cpp DOES call Clock::isTrusted() - the positive
     control. Without this, invariant 1 could be satisfied by simply
     deleting the clockTrusted assignment entirely rather than routing it
     correctly.
  3. ReportingPolicyInputs (in reporting/ReportingPolicy.h) declares a
     clockTrusted field, not a timeValid one - so a revert of the struct
     itself (not just the assignment) is also caught.

Comments are stripped before each check so prose (this file's own docstring
included, if it were ever pasted into the source) cannot produce a false
positive or a false negative.
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
ADAPTER_SOURCE = REPO_ROOT / "src" / "reporting" / "RuntimeReportingPolicy.cpp"
POLICY_HEADER = REPO_ROOT / "src" / "reporting" / "ReportingPolicy.h"

TIME_ISVALID_PATTERN = re.compile(r"Time\s*\.\s*isValid\s*\(\s*\)")
CLOCK_ISTRUSTED_PATTERN = re.compile(r"Clock\s*::\s*isTrusted\s*\(\s*\)")
CLOCK_TRUSTED_FIELD_PATTERN = re.compile(r"\bbool\s+clockTrusted\b")
TIME_VALID_FIELD_PATTERN = re.compile(r"\bbool\s+timeValid\b")


def strip_comments(code: str) -> str:
    """Remove // line comments and /* */ block comments so prose mentioning
    these names (explaining why one must NOT appear, as in this test's own
    docstring if pasted into the source) does not produce a false positive."""
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)
    code = re.sub(r"//[^\n]*", "", code)
    return code


def fail(msg):
    print(f"FAIL: {msg}")
    sys.exit(1)


def main():
    for path in (ADAPTER_SOURCE, POLICY_HEADER):
        if not path.is_file():
            fail(f"{path} does not exist")

    adapter_code_only = strip_comments(ADAPTER_SOURCE.read_text())
    header_code_only = strip_comments(POLICY_HEADER.read_text())

    # --- Invariant 1: the adapter does not call Time.isValid() in code. ---
    if TIME_ISVALID_PATTERN.search(adapter_code_only):
        fail(
            f"{ADAPTER_SOURCE} calls Time.isValid() - this reintroduces the "
            "exact gap WO-2026-09-22 closed: a real report-scheduling "
            "decision resting on raw epoch-existence rather than "
            "Clock::isTrusted()'s trust verdict"
        )

    # --- Invariant 2 (positive control): the adapter DOES call
    # Clock::isTrusted() - without this, invariant 1 could be satisfied by
    # simply deleting the clockTrusted assignment rather than routing it
    # correctly through the owner. ---
    if not CLOCK_ISTRUSTED_PATTERN.search(adapter_code_only):
        fail(
            f"{ADAPTER_SOURCE} does not call Clock::isTrusted() - "
            "ReportingPolicyInputs.clockTrusted must be sourced from the "
            "real clock-trust owner, not left unset or stubbed out here"
        )

    # --- Invariant 3: the struct itself declares clockTrusted, not
    # timeValid - catches a revert of the field, not just the assignment. ---
    if not CLOCK_TRUSTED_FIELD_PATTERN.search(header_code_only):
        fail(
            f"{POLICY_HEADER} does not declare a bool clockTrusted field in "
            "ReportingPolicyInputs"
        )
    if TIME_VALID_FIELD_PATTERN.search(header_code_only):
        fail(
            f"{POLICY_HEADER} still declares a bool timeValid field - the "
            "field should have been renamed to clockTrusted, not left "
            "alongside it"
        )

    print("OK: RuntimeReportingPolicy.cpp does not call Time.isValid()")
    print("OK: RuntimeReportingPolicy.cpp calls Clock::isTrusted()")
    print("OK: ReportingPolicyInputs declares clockTrusted, not timeValid")
    print("reporting_policy_clock_owner_structural_test: all invariants hold")


if __name__ == "__main__":
    main()
