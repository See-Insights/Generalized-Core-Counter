#!/usr/bin/env python3
"""
WO-2026-09-21 Step 4 - structural regression test for BatteryAuthority's
single ownership of the persisted battery tier and low-battery-mode fields.

Background: before this step, the tier decision built on state-of-charge had
four competing answerers - the properly guarded
ReportingPolicyResolver::resolveRuntime(), the unguarded (no vcell floor, no
trust check) Cloud::calculateBatteryTier() - reachable from the connectivity
failsafe - applyBatteryAwareConnectionModePolicy() (the persisted-field
writer), and Cloud::testBatteryBackoffLogic() (dead code that used the
persisted tier as scratch space). This step consolidates all four into
power/BatteryAuthority.{h,cpp}. WO-2026-09-21 Step 4 was corrected the
same day to split the module into a pure query (evaluate(), in
power/BatteryAuthority.cpp) and a command (commit(), in
power/BatteryAuthorityCommand.cpp) - the persisted tier and low-battery-mode
fields each get exactly one non-initializer write site, inside commit(),
and the unguarded Cloud::calculateBatteryTier() is deleted outright.

This is a source-invariant check on the REAL checked-in files, not a mirror.
Comments are stripped before each check so prose (including this file's own
history, if pasted into source) cannot produce a false positive or a false
negative. Matched as `sysStatus.set_X(` specifically (not a bare
`set_X(uint8_t value);` declaration, which never carries the `sysStatus.`
qualifier) so the accessor's own declaration in MyPersistentData.h does not
inflate the count.

  1. `sysStatus.set_currentBatteryTier(` appears exactly once in the entire
     src/ tree, and that one call site is in
     src/power/BatteryAuthorityCommand.cpp.
  2. `sysStatus.set_lowBatteryMode(` appears exactly once outside
     src/MyPersistentData.cpp (its own initializer write, in
     resetEverything()-shaped code, is excluded per this step's own
     "non-initializer write site" scope), and that one call site is in
     src/power/BatteryAuthorityCommand.cpp.
  3. No function named `calculateBatteryTier` is defined anywhere in src/ -
     Cloud::calculateBatteryTier() must have zero definitions, not merely
     zero callers.
  4. applyBatteryAwareConnectionModePolicy() - the function this step
     retired - has zero definitions and zero declarations remaining
     anywhere in src/.
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_ROOT = REPO_ROOT / "src"
BATTERY_AUTHORITY_CPP = SRC_ROOT / "power" / "BatteryAuthorityCommand.cpp"
MY_PERSISTENT_DATA_CPP = SRC_ROOT / "MyPersistentData.cpp"

SET_TIER_PATTERN = re.compile(r"PowerConfig::set_currentBatteryTier\s*\(")
SET_LOW_BATTERY_PATTERN = re.compile(r"PowerConfig::set_lowBatteryMode\s*\(")
CALCULATE_BATTERY_TIER_DEF_PATTERN = re.compile(
    r"\bcalculateBatteryTier\s*\([^;]*\)\s*\{"
)
APPLY_BATTERY_AWARE_PATTERN = re.compile(r"\bapplyBatteryAwareConnectionModePolicy\b")


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def strip_comments(code: str) -> str:
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)
    code = re.sub(r"//[^\n]*", "", code)
    return code


def main() -> None:
    if not BATTERY_AUTHORITY_CPP.is_file():
        fail(f"{BATTERY_AUTHORITY_CPP} does not exist")

    all_source_files = [
        p for p in SRC_ROOT.rglob("*") if p.suffix in (".cpp", ".h")
    ]
    texts = {p: strip_comments(p.read_text()) for p in all_source_files}

    # --- Invariant 1: PowerConfig::set_currentBatteryTier( appears exactly ---
    # --- once outside MyPersistentData.cpp, inside BatteryAuthorityCommand. ---
    # WO-2026-09-23-001: MyPersistentData.cpp is excluded because it now also
    # holds the facade forwarder definition; the single-writer invariant is
    # about production callers, which is what the facade spelling identifies.
    tier_sites = []
    for path, text in texts.items():
        if path == MY_PERSISTENT_DATA_CPP:
            continue
        count = len(SET_TIER_PATTERN.findall(text))
        if count:
            tier_sites.append((path, count))
    total_tier_writes = sum(c for _, c in tier_sites)
    if total_tier_writes != 1:
        fail(
            f"PowerConfig::set_currentBatteryTier( appears {total_tier_writes} "
            f"time(s) in src/, expected exactly 1: "
            + ", ".join(f"{p.relative_to(REPO_ROOT)}={c}" for p, c in tier_sites)
        )
    if tier_sites[0][0] != BATTERY_AUTHORITY_CPP:
        fail(
            f"the one PowerConfig::set_currentBatteryTier( call site is in "
            f"{tier_sites[0][0].relative_to(REPO_ROOT)}, expected "
            f"{BATTERY_AUTHORITY_CPP.relative_to(REPO_ROOT)}"
        )

    # --- Invariant 2: PowerConfig::set_lowBatteryMode( appears exactly once ---
    # --- outside MyPersistentData.cpp, inside power/BatteryAuthority.cpp.---
    low_battery_sites = []
    for path, text in texts.items():
        if path == MY_PERSISTENT_DATA_CPP:
            continue
        count = len(SET_LOW_BATTERY_PATTERN.findall(text))
        if count:
            low_battery_sites.append((path, count))
    total_low_battery_writes = sum(c for _, c in low_battery_sites)
    if total_low_battery_writes != 1:
        fail(
            f"PowerConfig::set_lowBatteryMode( appears {total_low_battery_writes} "
            f"time(s) outside {MY_PERSISTENT_DATA_CPP.relative_to(REPO_ROOT)}, "
            "expected exactly 1: "
            + ", ".join(f"{p.relative_to(REPO_ROOT)}={c}" for p, c in low_battery_sites)
        )
    if low_battery_sites[0][0] != BATTERY_AUTHORITY_CPP:
        fail(
            f"the one PowerConfig::set_lowBatteryMode( call site outside "
            f"MyPersistentData.cpp is in {low_battery_sites[0][0].relative_to(REPO_ROOT)}, "
            f"expected {BATTERY_AUTHORITY_CPP.relative_to(REPO_ROOT)}"
        )

    # --- Invariant 3: calculateBatteryTier has zero definitions anywhere. ---
    offenders = []
    for path, text in texts.items():
        if CALCULATE_BATTERY_TIER_DEF_PATTERN.search(text):
            offenders.append(str(path.relative_to(REPO_ROOT)))
    if offenders:
        fail(
            "calculateBatteryTier must have zero definitions anywhere in "
            "src/ (Cloud::calculateBatteryTier() was deleted - the unguarded "
            "path reachable from the connectivity failsafe) - found a "
            "definition in: " + ", ".join(offenders)
        )

    # --- Invariant 4: applyBatteryAwareConnectionModePolicy is fully gone -
    # --- zero declarations or definitions, not just zero non-comment      -
    # --- callers (a stray forward declaration left behind would silently  -
    # --- suggest the function still exists). ---
    offenders = []
    for path, text in texts.items():
        if APPLY_BATTERY_AWARE_PATTERN.search(text):
            offenders.append(str(path.relative_to(REPO_ROOT)))
    if offenders:
        fail(
            "applyBatteryAwareConnectionModePolicy must not appear anywhere "
            "in src/ (declaration, definition, or call) - its entire body "
            "folded into power/BatteryAuthorityCommand.cpp's commit() - found in: "
            + ", ".join(offenders)
        )

    print(f"OK: PowerConfig::set_currentBatteryTier( appears exactly once, in {BATTERY_AUTHORITY_CPP.relative_to(REPO_ROOT)}")
    print(f"OK: PowerConfig::set_lowBatteryMode( appears exactly once outside MyPersistentData.cpp, in {BATTERY_AUTHORITY_CPP.relative_to(REPO_ROOT)}")
    print("OK: calculateBatteryTier has zero definitions anywhere in src/")
    print("OK: applyBatteryAwareConnectionModePolicy has zero declarations/definitions/callers anywhere in src/")
    print("battery_authority_structural_test: all invariants hold")


if __name__ == "__main__":
    main()
