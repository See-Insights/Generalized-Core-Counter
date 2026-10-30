#!/usr/bin/env python3
"""WO-2026-10-29-002: occupancy ends at the close - structural checks on the real sources.

The behaviour is driven by occupancy_report_by_mode_test.sh. This test pins the shape the behaviour depends on:
one helper keyed on `== Closed` only, used at both start/extend sites, the credit clamp (only a session that started before the boundary), and the close cleanup's
explicit-boundary close plus the `close == 24` carry-over left unchanged. Each check is then run on a mutated text
(in memory, never in src/) and must fail by name.
"""
import pathlib
import re
import sys

SRC = pathlib.Path(__file__).resolve().parent.parent / "src" / "state"

HELPER = "inline bool occupancyIgnoredWhenClosed() { return Clock::openness() == Clock::Openness::Closed; }"
CLAMP = ("const time_t boundary = DailyBoundary::check(now).boundary;\n"
         "\t\tif (start < boundary) closeAt = std::min(closeAt, boundary);")
CLEANUP_CLOSE = 'closeOccupancySessionSafely("daily-cleanup", boundary);'
CARRY = ("if (close == 24 && wasOccupied) {\n"
         "      CurrentReadings::set_occupied(true);\n"
         "      CurrentReadings::set_occupancyStartTime(std::max(boundary, originalSessionStart));")


def body(text, sig):
    i = text.index(sig)
    j = text.index("{", i)
    depth = 0
    for k in range(j, len(text)):
        depth += (text[k] == "{") - (text[k] == "}")
        if depth == 0:
            return text[i:k + 1]
    raise ValueError(sig)


def problems(common, modes, report):
    out = []
    if common.count(HELPER) != 1:
        out.append("helper: occupancyIgnoredWhenClosed() must be defined once as `== Closed`")
    if re.search(r"openness\(\)\s*!=", common + modes + report):
        out.append("helper: no `openness() !=` form is allowed (Unknown must behave as before)")
    awake = body(modes, "void handleOccupancyMode() {")
    wake = body(modes, "bool processSleepWakeEvent(bool pirWake) {")
    if awake.count("occupancyIgnoredWhenClosed()") != 1 or "loop()" not in awake:
        out.append("sites: handleOccupancyMode() must call loop() and gate on the helper once")
    occ = wake[wake.index("} else if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY) {"):]
    if occ.count("occupancyIgnoredWhenClosed()") != 1 or \
            occ.index("occupancyIgnoredWhenClosed()") > occ.index("set_occupied(true)"):
        out.append("sites: the OCCUPANCY pirWake branch must test the helper before it starts or extends a session")
    if modes.count("occupancyIgnoredWhenClosed()") != 2:
        out.append("sites: the helper is used at exactly the two start/extend sites in State_Modes.cpp")
    close = body(common, "inline OccupancyCloseResult closeOccupancySessionSafely(")
    if close.count(CLAMP) != 1 or "if (occupancyIgnoredWhenClosed()) {" not in close:
        out.append("clamp: closeOccupancySessionSafely() clamps closeAt to the last boundary when Closed")
    if "if (wasOccupied) {\n      " + CLEANUP_CLOSE not in report:
        out.append("cleanup: a session open at the close is closed through the boundary")
    if CARRY not in report:
        out.append("carry-over: the close == 24 carry-over is unchanged")
    return out


common = (SRC / "State_Common.h").read_text()
modes = (SRC / "State_Modes.cpp").read_text()
report = (SRC / "State_Report.cpp").read_text()

found = problems(common, modes, report)
if found:
    for p in found:
        print(f"FAIL: {p}", file=sys.stderr)
    sys.exit(1)

mutants = {
    "gate removed": ("helper:", (common.replace("== Clock::Openness::Closed;", "== Clock::Openness::Closed && false;", 1)
                                 .replace(HELPER, "", 1), modes, report)),
    "gate on != Open": ("helper:", (common.replace("openness() == Clock::Openness::Closed", "openness() != Clock::Openness::Open", 1),
                                    modes, report)),
    "wake site gate removed": ("sites:", (common, modes.replace("if (occupancyIgnoredWhenClosed()) {\n          Log.info", "if (false) {\n          Log.info", 1), report)),
    "session left open at the close": ("cleanup:", (common, modes, report.replace(CLEANUP_CLOSE, "", 1))),
    "clamp dropped": ("clamp:", (common.replace(CLAMP, "", 1), modes, report)),
    "clamp without start < boundary": ("clamp:", (common.replace("if (start < boundary) closeAt", "closeAt", 1), modes, report)),
    "clamp with start <= boundary": ("clamp:", (common.replace("start < boundary", "start <= boundary", 1), modes, report)),
    "carry-over changed": ("carry-over:", (common, modes, report.replace("close == 24 && wasOccupied", "close == 23 && wasOccupied", 1))),
}
for name, (prefix, texts) in mutants.items():
    hit = [p for p in problems(*texts) if p.startswith(prefix)]
    if not hit:
        print(f"FAIL: mutation '{name}' was not caught by a '{prefix}' check", file=sys.stderr)
        sys.exit(1)
    print(f"caught: {name} -> {hit[0]}")
print("occupancy_ends_at_close_structural_test: ok")
