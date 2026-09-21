#!/usr/bin/env python3
"""
WO-2026-09-21 Step 4 (corrected same day) - structural regression test for
BatteryAuthority::commit()'s call-site allowlist.

Background: the query/command split means evaluate() (pure, read-only) may
be called from anywhere - the connectivity failsafe, the reporting-policy
adapter, the device-status publisher - with no risk of a persisting side
effect. commit() is the opposite: every call is a deliberate decision to
persist a tier/low-battery-mode transition and apply the paired
connection-mode side effect. Committing from an unexpected place (a status
publisher, a diagnostic read, a future refactor that "simplifies" a read
path by calling commit() instead of evaluate()) would silently reintroduce
persisting side effects into code that looks like a plain read - exactly the
defect this step's own report flagged and this correction fixes.

This is a source-invariant check on the REAL checked-in files, not a mirror.
Comments are stripped before each check so prose (including this file's own
history, if pasted into source) cannot produce a false positive or a false
negative. Matched as `BatteryAuthority::commit(` specifically - the
qualified call form every caller outside the BatteryAuthority namespace must
use - so commit()'s own (unqualified, in-namespace) definition in
BatteryAuthorityCommand.cpp is never counted as a call site.

  1. `BatteryAuthority::commit(` appears exactly four times in the entire
     src/ tree, at exactly the four deliberate policy-application sites:
     Generalized-Core-Counter.cpp's setup() (x1), state/State_Report.cpp's
     reporting cycle (x1), and state/State_Sleep.cpp's two low-battery-
     recovery checks (x2).
  2. No other file contains a call to BatteryAuthority::commit() - in
     particular, the four read-path files this step's report named
     (cloud/DeviceStatusPublisher.cpp, diagnostics/ConnectivityFailsafeTest.cpp,
     reporting/RuntimeReportingPolicy.cpp, and Generalized-Core-Counter.cpp's
     own currentBatteryTierForFailsafe()) must never call commit() - only
     evaluate().
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_ROOT = REPO_ROOT / "src"

COMMIT_CALL_PATTERN = re.compile(r"BatteryAuthority::commit\s*\(")

# The only sites permitted to call BatteryAuthority::commit(), and exactly
# how many times each is allowed to.
ALLOWED_COMMIT_SITES = {
    SRC_ROOT / "Generalized-Core-Counter.cpp": 1,
    SRC_ROOT / "state" / "State_Report.cpp": 1,
    SRC_ROOT / "state" / "State_Sleep.cpp": 2,
}

# Read paths that must never call commit() - named explicitly so a failure
# here points straight at the regression, not just "count mismatch".
FORBIDDEN_READ_PATHS = [
    SRC_ROOT / "cloud" / "DeviceStatusPublisher.cpp",
    SRC_ROOT / "diagnostics" / "ConnectivityFailsafeTest.cpp",
    SRC_ROOT / "reporting" / "RuntimeReportingPolicy.cpp",
]


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def strip_comments(code: str) -> str:
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)
    code = re.sub(r"//[^\n]*", "", code)
    return code


def main() -> None:
    all_source_files = [p for p in SRC_ROOT.rglob("*") if p.suffix in (".cpp", ".h")]
    texts = {p: strip_comments(p.read_text()) for p in all_source_files}

    call_counts = {}
    for path, text in texts.items():
        count = len(COMMIT_CALL_PATTERN.findall(text))
        if count:
            call_counts[path] = count

    total_calls = sum(call_counts.values())
    expected_total = sum(ALLOWED_COMMIT_SITES.values())
    if total_calls != expected_total:
        fail(
            f"BatteryAuthority::commit( appears {total_calls} time(s) in "
            f"src/, expected exactly {expected_total}: "
            + ", ".join(f"{p.relative_to(REPO_ROOT)}={c}" for p, c in call_counts.items())
        )

    # --- Invariant 1: every call site is on the allowlist, at the exact
    # expected count. ---
    for path, expected_count in ALLOWED_COMMIT_SITES.items():
        actual_count = call_counts.get(path, 0)
        if actual_count != expected_count:
            fail(
                f"{path.relative_to(REPO_ROOT)} calls BatteryAuthority::commit( "
                f"{actual_count} time(s), expected exactly {expected_count} "
                "(the allowlisted policy-application sites)"
            )

    # --- Invariant 2: nothing outside the allowlist calls commit() - in
    # particular, not the named read-path files. ---
    offenders = [p for p in call_counts if p not in ALLOWED_COMMIT_SITES]
    if offenders:
        fail(
            "BatteryAuthority::commit( is called from outside the allowlist: "
            + ", ".join(f"{p.relative_to(REPO_ROOT)}={call_counts[p]}" for p in offenders)
            + " - only evaluate() may be called from a read path"
        )
    for forbidden_path in FORBIDDEN_READ_PATHS:
        if forbidden_path in call_counts:
            fail(
                f"{forbidden_path.relative_to(REPO_ROOT)} - an explicitly "
                "named read path - calls BatteryAuthority::commit(); it must "
                "call only evaluate()"
            )

    print(
        "OK: BatteryAuthority::commit( appears exactly "
        f"{expected_total} time(s), at exactly the four allowlisted "
        "policy-application sites"
    )
    print("OK: no read-path file (DeviceStatusPublisher.cpp, ConnectivityFailsafeTest.cpp, RuntimeReportingPolicy.cpp) calls commit()")
    print("battery_authority_commit_sites_structural_test: all invariants hold")


if __name__ == "__main__":
    main()
