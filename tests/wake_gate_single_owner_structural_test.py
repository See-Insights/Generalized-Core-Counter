#!/usr/bin/env python3
"""
WO-2026-09-14-002 (Step 1 of the Structural Ownership Map roadmap) -
structural regression test for the hibernate wake-validation gate.

Background: the production gate at Generalized-Core-Counter.cpp used to be a
hand-written six-term `&&` chain, duplicated position-for-position by
HibernateWakeDiagnostics::classifyGateArm() so a forensic event could name
which term failed. Nothing enforced the two staying in sync except a doc
comment asking a reviewer to check them by hand - this is Incident 1 from
docs/architecture-review-2026-09-03.md and the reason
WO-2026-08-31-002 Amendment B (the DEEP_POWER_DOWN finding) had nowhere
single to land.

The fix retires the duplicate: the production `if` now calls
classifyGateArm() directly and branches on `GateArm::kNone`, so there is
exactly one place that decides whether the gate passed.

This is a source-invariant check on the REAL checked-in file, not a mirror.
Comments are stripped before each check so prose (including this file's own
history, if pasted into the source) cannot produce a false positive or a
false negative.

  1. Generalized-Core-Counter.cpp contains exactly one call to
     classifyGateArm() - if it contains zero, the production gate has
     stopped consulting the classifier (silently reintroducing a
     hand-written duplicate); if it contains more than one, a second
     decision site has crept back in.
  2. Generalized-Core-Counter.cpp contains no occurrence of the old raw
     `WakeReason::ALARM &&` chain fragment - that fragment could only exist
     as part of a hand-written boolean chain (the seam's own field
     assignment ends in `);`, never `&&`), so its presence means the old
     duplicated `if` is back.
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
PRODUCTION_SOURCE = REPO_ROOT / "src" / "Generalized-Core-Counter.cpp"

CLASSIFY_CALL_PATTERN = re.compile(r"classifyGateArm\s*\(")
RAW_CHAIN_FRAGMENT_PATTERN = re.compile(r"WakeReason::ALARM\s*&&")


def strip_comments(code: str) -> str:
    """Remove // line comments and /* */ block comments so prose describing
    this history (in either file) cannot produce a false positive or a
    false negative."""
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)
    code = re.sub(r"//[^\n]*", "", code)
    return code


def fail(msg):
    print(f"FAIL: {msg}")
    sys.exit(1)


def main():
    if not PRODUCTION_SOURCE.is_file():
        fail(f"{PRODUCTION_SOURCE} does not exist")

    code_only = strip_comments(PRODUCTION_SOURCE.read_text())

    # --- Invariant 1: exactly one call site decides the gate. ---
    call_count = len(CLASSIFY_CALL_PATTERN.findall(code_only))
    if call_count != 1:
        fail(
            f"{PRODUCTION_SOURCE} contains {call_count} classifyGateArm( "
            "call(s) in code, expected exactly 1 - the wake-validation gate "
            "must have a single decision site (WO-2026-09-14-002 Step 1); "
            "either the production `if` has stopped calling the classifier, "
            "or a second decision site has been added"
        )

    # --- Invariant 2 (positive control): the old hand-written chain is
    # gone. Without this, invariant 1 alone would pass even if a
    # hand-duplicated `if` were reintroduced ALONGSIDE a single
    # classifyGateArm() call left over elsewhere - this specifically
    # catches the six-term chain reappearing. ---
    if RAW_CHAIN_FRAGMENT_PATTERN.search(code_only):
        fail(
            f"{PRODUCTION_SOURCE} contains the old raw `WakeReason::ALARM "
            "&&` chain fragment in code - the hand-duplicated gate `if` "
            "WO-2026-09-14-002 Step 1 retired appears to be back, which "
            "reintroduces the two-implementations-kept-in-sync-by-hand "
            "problem this step fixed"
        )

    print("OK: exactly one classifyGateArm( call decides the wake-validation gate")
    print("OK: the old raw WakeReason::ALARM && chain fragment is gone")
    print("wake_gate_single_owner_structural_test: all invariants hold")


if __name__ == "__main__":
    main()
