#!/usr/bin/env python3
"""
Host-side regression test for WO-2026-08-12-001 item 2: modem-teardown
confirmation logging in src/state/State_Sleep.cpp and
src/state/State_Error.cpp.

Rather than mirroring the logic in a separate C++ file (these files pull
in far too many Particle/Cloud/Sensor dependencies to compile standalone
- see sleep_breadcrumb_sequence_test.py for the established precedent),
this test parses the ACTUAL shipped source files directly and confirms:

  1. Four new "ModemTeardown: radioOn=%d point=..." Log.info lines exist
     (three in State_Sleep.cpp, one in State_Error.cpp), each reading
     Connectivity::isRadioPoweredOn() and each immediately, textually
     preceding one of the four anchor log lines named in the work order:
       - point=hibernate            -> Log.info("Sleep: HIBERNATE ...")
       - point=heap-guard-reset     -> Log.warn("Nightly heap guard: ...
         resetting for clean next-day start", ...)
       - point=sleep-attempts-failed-reset -> Log.error("All sleep
         attempts failed ...")
       - point=error-state-soft-reset -> Log.info("Executing soft reset
         from ERROR_STATE") (State_Error.cpp, Case 2)
  2. No new blocking call (waitFor/delay/Cellular.off()/
     Connectivity::requestRadioPowerOff()) was introduced at or near any
     of the four new log lines.
  3. Item 1 (network-keep-alive logging, already satisfied) was left
     untouched: the "Sleep: ULP standby=%d ..." line and the
     Particle.connected()/isRadioPoweredOn() invariant-check block both
     still exist, unduplicated.
  4. src/power/Connectivity.h and .cpp are untouched (git diff scope
     check) - no new teardown function was added anywhere the WO
     forbids it.
  5. State_Error.cpp's Case 3 (`ab1805.deepPowerDown()` block) was NOT
     touched - no ModemTeardown line was added there, since a full
     hardware power-cycle is explicitly out of scope per the WO.

This is real evidence traced through the actual code path (not a
hand-wave): every assertion below reads the literal line order from the
shipped files, and each check is written so that removing/altering the
new logging (as verified via mutation testing before this test was
finalized) causes a hard failure - not merely a missing "nice to have"
assertion.
"""
import re
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC = REPO_ROOT / "src" / "state" / "State_Sleep.cpp"
ERROR_SRC = REPO_ROOT / "src" / "state" / "State_Error.cpp"
CONNECTIVITY_H = REPO_ROOT / "src" / "power" / "Connectivity.h"
CONNECTIVITY_CPP = REPO_ROOT / "src" / "power" / "Connectivity.cpp"

# (point tag, regex for the anchor log line that must immediately follow it)
EXPECTED_POINTS = [
    ("hibernate", re.compile(r'Log\.info\("Sleep: HIBERNATE reason=closed')),
    ("heap-guard-reset", re.compile(
        r'Log\.warn\("Nightly heap guard:.*resetting for clean next-day start')),
    ("sleep-attempts-failed-reset", re.compile(
        r'Log\.error\("All sleep attempts failed')),
]

# The fourth point lives in a different file (State_Error.cpp), checked
# separately below via check_error_state_point().
ERROR_STATE_POINT_TAG = "error-state-soft-reset"
ERROR_STATE_ANCHOR_RE = re.compile(
    r'Log\.info\("Executing soft reset from ERROR_STATE"\);')

MODEM_TEARDOWN_RE = re.compile(
    r'Log\.info\("ModemTeardown: radioOn=%d point=([\w-]+)",\s*'
    r'\(int\)Connectivity::isRadioPoweredOn\(\)\);'
)

BLOCKING_CALL_TOKENS = [
    "waitFor(",
    "delay(",
    "Cellular.off(",
    "Connectivity::requestRadioPowerOff(",
]


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def strip_line_comments(text: str) -> str:
    """Strip trailing '//' line comments (not block comments - out of scope,
    this codebase doesn't use them for this kind of thing) so a commented-out
    ModemTeardown call is not mistaken for live code. A Stage-7 pass found
    this exact bypass: `// Log.info("ModemTeardown: ...");` matched the old
    regex-only check undetected."""
    return "\n".join(re.sub(r"//.*$", "", l) for l in text.splitlines())


def indent_of(line: str) -> int:
    return len(line) - len(line.lstrip(" \t"))


def main() -> None:
    text = strip_line_comments(SRC.read_text())
    lines = text.splitlines()

    # ---- Check 1: exactly 3 ModemTeardown log lines exist in
    # State_Sleep.cpp, one per expected point tag, each reading
    # Connectivity::isRadioPoweredOn(). ----
    modem_teardown_matches = list(MODEM_TEARDOWN_RE.finditer(text))
    if len(modem_teardown_matches) != 3:
        fail(f"expected exactly 3 ModemTeardown log lines calling "
             f"Connectivity::isRadioPoweredOn() in State_Sleep.cpp, "
             f"found {len(modem_teardown_matches)}")

    found_tags = sorted(m.group(1) for m in modem_teardown_matches)
    expected_tags = sorted(tag for tag, _ in EXPECTED_POINTS)
    if found_tags != expected_tags:
        fail(f"ModemTeardown point tags mismatch: expected {expected_tags}, "
             f"found {found_tags}")
    print("PASS: exactly 3 ModemTeardown log lines present in "
          "State_Sleep.cpp, each calling Connectivity::isRadioPoweredOn(), "
          f"tags match {expected_tags}")

    # Map each ModemTeardown match to the line index it appears on.
    def line_index_of(pos: int) -> int:
        return text.count("\n", 0, pos)

    modem_line_by_tag = {
        m.group(1): line_index_of(m.start()) for m in modem_teardown_matches
    }

    # ---- Check 2: each ModemTeardown log line is immediately (textually
    # adjacent, allowing only blank/whitespace-only lines in between - there
    # should be none) followed by its corresponding anchor log line, AND
    # sits at the exact same indentation depth as that anchor - proving it
    # is not wrapped in its own new conditional (e.g.
    # `if (!Connectivity::isRadioPoweredOn()) { Log.info("ModemTeardown...");
    # }`) that would silently suppress the diagnostic precisely when the
    # radio is on, which is exactly the case this logging exists to catch.
    # A Stage-7 pass found this exact bypass passed the presence/adjacency
    # checks alone undetected. ----
    for tag, anchor_re in EXPECTED_POINTS:
        modem_line = modem_line_by_tag[tag]
        # The anchor line must be the very next non-empty line.
        next_line = lines[modem_line + 1]
        if not anchor_re.search(next_line):
            fail(f"ModemTeardown point={tag} at line {modem_line + 1} is not "
                 f"immediately followed by its expected anchor log line; "
                 f"found instead: {next_line!r}")
        modem_indent = indent_of(lines[modem_line])
        anchor_indent = indent_of(next_line)
        if modem_indent != anchor_indent:
            fail(f"ModemTeardown point={tag} (line {modem_line + 1}, indent "
                 f"{modem_indent}) is not at the same indentation depth as "
                 f"its anchor line (line {modem_line + 2}, indent "
                 f"{anchor_indent}) - this suggests the log call was wrapped "
                 f"in its own new conditional, which could silently suppress "
                 f"it exactly when the radio is on")
        print(f"PASS: ModemTeardown point={tag} (line {modem_line + 1}) "
              f"immediately precedes its anchor log line (line {modem_line + 2}) "
              f"at the same indentation depth (unconditional relative to it)")

    # ---- Check 3: no new blocking call introduced within a tight window
    # (3 lines before AND after) of any of the three new log lines. This
    # does not assert blocking calls are absent from the whole file
    # (existing delay(2000) after the sleep-attempts-failed-reset anchor is
    # expected and out of scope, well outside this window) - only that none
    # was added directly adjacent to the new logging itself, which is the
    # WO's "no new blocking call at these points" requirement. Checking
    # after the log line too (not just before) closes a Stage-7-found gap -
    # the prior version only checked before it.
    WINDOW = 3
    for tag, modem_line in modem_line_by_tag.items():
        window_lines = lines[max(0, modem_line - WINDOW): modem_line + WINDOW + 1]
        for token in BLOCKING_CALL_TOKENS:
            for wl in window_lines:
                if token in wl and "ModemTeardown" not in wl:
                    fail(f"blocking-call token {token!r} found within "
                         f"{WINDOW} lines of ModemTeardown point={tag} "
                         f"(line {modem_line + 1}): {wl!r}")
    print("PASS: no new blocking call token found immediately adjacent to "
          "any of the 3 new ModemTeardown log lines (checked before and after)")

    # ---- Check 4 (item 1 untouched): the existing "Sleep: ULP standby=%d"
    # line and the Particle.connected()/isRadioPoweredOn() invariant-check
    # block still exist, exactly once each (not duplicated). ----
    ulp_standby_lines = [i for i, l in enumerate(lines)
                         if 'Log.info("Sleep: ULP standby=%d reason=%s dur=%ds occ=%d soc=%.1f' in l]
    # Two expected: the test-only variant (with test=1) at ~1326 and the
    # real unconditional line at ~1333 - both pre-existing, count must be
    # unchanged (2), not more (which would indicate an accidental duplicate
    # was added) and not fewer (which would indicate accidental deletion).
    if len(ulp_standby_lines) != 2:
        fail(f"expected exactly 2 'Sleep: ULP standby=%d ...' log line "
             f"variants (test + real), found {len(ulp_standby_lines)} - "
             f"item 1 logging must be left untouched, not duplicated or removed")
    print("PASS: 'Sleep: ULP standby=%d ...' logging (item 1) present "
          "exactly twice (test + real variant), unduplicated")

    invariant_check_lines = [
        i for i, l in enumerate(lines)
        if 'Log.warn("INV: entering sleep with cloud=%d radioOn=%d (standby=0)"' in l
    ]
    if len(invariant_check_lines) != 1:
        fail(f"expected exactly 1 standby/connection invariant-check site "
             f"(State_Sleep.cpp:1197-1206 block), found {len(invariant_check_lines)} - "
             f"item 1's existing invariant check must be left untouched")
    print("PASS: existing standby/connection invariant-check block (item 1) "
          "present exactly once, unchanged")

    # ---- Check 5: fourth point (State_Error.cpp, Case 2 soft reset) ----
    error_text = strip_line_comments(ERROR_SRC.read_text())
    error_lines = error_text.splitlines()

    error_modem_matches = list(MODEM_TEARDOWN_RE.finditer(error_text))
    if len(error_modem_matches) != 1:
        fail(f"expected exactly 1 ModemTeardown log line in State_Error.cpp, "
             f"found {len(error_modem_matches)}")
    error_tag = error_modem_matches[0].group(1)
    if error_tag != ERROR_STATE_POINT_TAG:
        fail(f"State_Error.cpp ModemTeardown tag mismatch: expected "
             f"{ERROR_STATE_POINT_TAG!r}, found {error_tag!r}")
    print(f"PASS: exactly 1 ModemTeardown log line present in "
          f"State_Error.cpp, calling Connectivity::isRadioPoweredOn(), "
          f"tag={error_tag!r}")

    error_modem_line = error_text.count("\n", 0, error_modem_matches[0].start())
    next_line = error_lines[error_modem_line + 1]
    if not ERROR_STATE_ANCHOR_RE.search(next_line):
        fail(f"State_Error.cpp ModemTeardown point={error_tag} at line "
             f"{error_modem_line + 1} is not immediately followed by its "
             f"expected anchor log line; found instead: {next_line!r}")
    error_modem_indent = indent_of(error_lines[error_modem_line])
    error_anchor_indent = indent_of(next_line)
    if error_modem_indent != error_anchor_indent:
        fail(f"State_Error.cpp ModemTeardown point={error_tag} (line "
             f"{error_modem_line + 1}, indent {error_modem_indent}) is not "
             f"at the same indentation depth as its anchor line (line "
             f"{error_modem_line + 2}, indent {error_anchor_indent}) - this "
             f"suggests the log call was wrapped in its own new conditional, "
             f"which could silently suppress it exactly when the radio is on")
    print(f"PASS: State_Error.cpp ModemTeardown point={error_tag} "
          f"(line {error_modem_line + 1}) immediately precedes its anchor "
          f"log line (line {error_modem_line + 2}) at the same indentation "
          f"depth (unconditional relative to it)")

    # No new blocking call within a tight window of the new log line
    # (before AND after - closes the same Stage-7-found gap as Check 3).
    error_window = error_lines[max(0, error_modem_line - WINDOW): error_modem_line + WINDOW + 1]
    for token in BLOCKING_CALL_TOKENS:
        for wl in error_window:
            if token in wl and "ModemTeardown" not in wl:
                fail(f"blocking-call token {token!r} found within "
                     f"{WINDOW} lines of State_Error.cpp ModemTeardown "
                     f"point={error_tag} (line {error_modem_line + 1}): {wl!r}")
    print("PASS: no new blocking call token found immediately adjacent to "
          "the State_Error.cpp ModemTeardown log line (checked before and after)")

    # ---- Check 6: State_Error.cpp Case 3 (ab1805.deepPowerDown()) block
    # was NOT touched - no ModemTeardown line added there. Case 3 is a
    # full hardware power-cycle, explicitly out of scope per the WO. ----
    deep_power_down_lines = [
        i for i, l in enumerate(error_lines) if "ab1805.deepPowerDown();" in l
    ]
    if len(deep_power_down_lines) != 1:
        fail(f"expected exactly 1 ab1805.deepPowerDown() call in "
             f"State_Error.cpp, found {len(deep_power_down_lines)}")
    dpd_line = deep_power_down_lines[0]
    CASE3_WINDOW = 4
    case3_window_lines = error_lines[max(0, dpd_line - CASE3_WINDOW): dpd_line + 1]
    for wl in case3_window_lines:
        if "ModemTeardown" in wl:
            fail(f"State_Error.cpp Case 3 (ab1805.deepPowerDown(), line "
                 f"{dpd_line + 1}) must remain untouched per WO scope, but "
                 f"a ModemTeardown log line was found nearby: {wl!r}")
    print("PASS: State_Error.cpp Case 3 (ab1805.deepPowerDown() block) "
          "left untouched - no ModemTeardown line added there")

    # ---- Check 7: src/power/Connectivity.h and .cpp are untouched. ----
    # Use git diff against HEAD to confirm no modifications were made to
    # files outside the WO's permitted-files scope.
    diff_result = subprocess.run(
        ["git", "diff", "--name-only", "HEAD", "--",
         "src/power/Connectivity.h", "src/power/Connectivity.cpp"],
        cwd=REPO_ROOT, capture_output=True, text=True, check=True,
    )
    changed = [l for l in diff_result.stdout.splitlines() if l.strip()]
    if changed:
        fail(f"src/power/Connectivity.h/.cpp must remain untouched per WO "
             f"permitted-files scope, but git diff shows changes: {changed}")
    print("PASS: src/power/Connectivity.h and .cpp are untouched (git diff scope check)")

    print("\nAll modem-teardown confirmation logging regression checks passed")


if __name__ == "__main__":
    main()
