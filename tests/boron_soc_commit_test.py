#!/usr/bin/env python3
"""
Regression/evidence test for WO-2026-08-25-001 Amendment B, Blocker 2 / AC-B4:
an accepted Boron fuel-gauge sample must reach current.stateOfCharge().

At HEAD (before this round) there were three current.set_stateOfCharge()
call sites in SensorManager.cpp::batteryState(): the non-cellular guard, the
Boron path (inside FuelGaugeResyncActions::commitSoc(), reached via the
STALE_SOC resync machinery's resolveSocCommit()), and the Photon path.
Retiring the STALE_SOC latch/retry/correction machinery (WO line 620, a
correct and intentional retirement) silently deleted commitSoc() along with
it - the ONLY path that committed an ordinary accepted Boron sample. Codex
Stage 7 found the regression: only two call sites survived, and the
surviving non-Photon one is inside
`#if !(HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM))`, which
EXCLUDES Boron. A live Boron device would silently freeze its reported SOC
forever.

SensorManager.cpp cannot be compiled standalone on the host (it pulls in
PMIC, System, Log, retained storage, and other heavy Particle/Device-OS
dependencies with no existing stub harness for the whole file - see
tests/README.md for the established precedent of tracing real shipped
source directly in this situation instead of hand-mirroring the logic).
This test instead traces the actual shipped source to prove the commit is
restored, on the correct (Boron) path, and gated on the pre-existing
authoritative-sample fence rather than any retired stale-SOC condition:

  1. There are (again) three current.set_stateOfCharge(soc) call sites.
  2. Exactly one of them is INSIDE the
     `#if HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM)` region
     that also contains the Boron authority-fence machinery
     (rejectAuthoritativeOverwrite) - i.e. the Boron path, not the
     `#if !(...)` guard that excludes it, and not the Photon fallback.
  3. That call site is gated by `if (!rejectAuthoritativeOverwrite)` - the
     same fence used by every other accepted-sample write in this function
     - and is NOT inside any retired STALE_SOC/resync/latch/retry construct
     (no ResyncActions, no commitSoc, no resolveSocCommit, no quickStart
     identifiers anywhere in the file - those stay retired per the WO).
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC = REPO_ROOT / "src" / "sensors" / "SensorManager.cpp"


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def main() -> None:
    text = SRC.read_text()
    lines = text.splitlines()

    commit_line_numbers = [
        i + 1 for i, line in enumerate(lines) if "current.set_stateOfCharge(soc);" in line
    ]
    if len(commit_line_numbers) != 3:
        fail(
            "expected exactly 3 current.set_stateOfCharge(soc) call sites "
            f"(non-cellular guard, Boron, Photon); found {len(commit_line_numbers)} "
            f"at lines {commit_line_numbers}"
        )

    # Retired STALE_SOC/resync machinery must not have been reintroduced as
    # code (not merely mentioned in an explanatory comment, e.g. this test's
    # own docstring-style references above, or this file's own comment
    # documenting what was retired and why). `fuelGauge.quickStart()` is
    # intentionally excluded: it is the legitimate MAX17043 driver API for a
    # hardware recalibration pulse on wake-stabilization, unrelated to the
    # retired software latch/resync mechanism that happened to share the
    # word "quickStart" in its own vocabulary.
    retired_identifiers = [
        "ResyncActions",
        "commitSoc",
        "resolveSocCommit",
        "staleSocConditionsMet",
        "shouldResyncFuelGauge",
    ]
    code_lines = [line for line in lines if not line.strip().startswith("//")]
    code_text = "\n".join(code_lines)
    for identifier in retired_identifiers:
        if identifier in code_text:
            fail(f"retired STALE_SOC identifier '{identifier}' reappeared in SensorManager.cpp")

    # Locate the `#if HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM)`
    # region within batteryState() - the Boron path - by scanning preprocessor
    # directives with a simple depth counter from that #if to its matching
    # #endif/#else.
    boron_if_pattern = re.compile(
        r"#if HAL_PLATFORM_CELLULAR && \(PLATFORM_ID != PLATFORM_MSOM\)"
    )
    # Multiple `#if HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM)`
    # regions exist in this file (fuel-gauge selection, TMP112 detection,
    # etc.) - find the one that is ALSO the batteryState() Boron authority
    # region, identified unambiguously by containing the
    # rejectAuthoritativeOverwrite fence.
    boron_if_line = None
    boron_region_end = None
    for i, line in enumerate(lines):
        if not boron_if_pattern.search(line):
            continue
        depth = 1
        region_end = None
        for j in range(i + 1, len(lines)):
            stripped = lines[j].strip()
            if stripped.startswith("#if"):
                depth += 1
            elif stripped.startswith("#endif"):
                depth -= 1
                if depth == 0:
                    region_end = j
                    break
        if region_end is None:
            continue
        region_text = "\n".join(lines[i:region_end])
        if "rejectAuthoritativeOverwrite" in region_text:
            boron_if_line = i
            boron_region_end = region_end
            break
    if boron_if_line is None or boron_region_end is None:
        fail(
            "could not find the batteryState() Boron '#if HAL_PLATFORM_CELLULAR && "
            "(PLATFORM_ID != PLATFORM_MSOM)' region (identified by containing "
            "rejectAuthoritativeOverwrite)"
        )

    boron_commit_lines = [
        n for n in commit_line_numbers if boron_if_line < (n - 1) < boron_region_end
    ]
    if len(boron_commit_lines) != 1:
        fail(
            "expected exactly 1 current.set_stateOfCharge(soc) call site inside the "
            f"Boron '#if HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM)' region "
            f"(lines {boron_if_line + 1}-{boron_region_end + 1}); found {len(boron_commit_lines)} "
            f"at {boron_commit_lines}"
        )

    boron_commit_line_idx = boron_commit_lines[0] - 1  # back to 0-based
    # The commit must be gated by the authority fence, on the line directly
    # above it (mirrors the restored `if (!rejectAuthoritativeOverwrite) {`
    # block).
    guard_line = lines[boron_commit_line_idx - 1].strip()
    if guard_line != "if (!rejectAuthoritativeOverwrite) {":
        fail(
            "Boron current.set_stateOfCharge(soc) commit is not directly gated by "
            f"'if (!rejectAuthoritativeOverwrite) {{' on the preceding line; found: {guard_line!r}"
        )

    print(
        "PASS: Boron current.set_stateOfCharge(soc) commit restored at line "
        f"{boron_commit_lines[0]}, inside the Boron platform region, gated on "
        "rejectAuthoritativeOverwrite, with no retired STALE_SOC identifiers reintroduced"
    )


if __name__ == "__main__":
    main()
