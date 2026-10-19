#!/usr/bin/env python3
"""WO-2026-10-19-001: the connectivity failsafe lives in
power/ConnectivityFailsafe.cpp, defined once; main only calls the supervisor."""
import re
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parent.parent / "src"
MAIN = SRC / "Generalized-Core-Counter.cpp"
CPP = SRC / "power" / "ConnectivityFailsafe.cpp"
HDR = SRC / "power" / "ConnectivityFailsafe.h"
COMMON = SRC / "state" / "State_Common.h"

MOVED = [
    "batteryTierShortName", "currentBatteryTierForFailsafe",
    "connectivityFailsafeHasExternalPower", "connectivityFailsafeJitterSec",
    "persistConnectivityFailsafeState", "failsafeDeferReasonNameLocal",
    "claimFailsafeDeferLogLocal", "failsafeDeferReasonName", "claimFailsafeDeferLog",
    "clearConnectivityFailsafeRecovery", "connectivityFailsafeSupervisor",
]
EXTERNAL = [
    "failsafeDeferReasonName", "claimFailsafeDeferLog",
    "clearConnectivityFailsafeRecovery", "connectivityFailsafeSupervisor",
]


def fail(msg):
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def defs(text, name):
    return re.findall(rf"^[A-Za-z_][\w \*]*?\b{name}\s*\([^;{{]*\)\s*{{", text, re.M)


main_text, cpp_text, hdr_text = MAIN.read_text(), CPP.read_text(), HDR.read_text()

for name in MOVED:
    if defs(main_text, name):
        fail(f"{name} is still defined in Generalized-Core-Counter.cpp")
    if len(defs(cpp_text, name)) != 1:
        fail(f"{name} must be defined exactly once in ConnectivityFailsafe.cpp")

# No other translation unit may define a failsafe function (no duplicates).
for path in SRC.rglob("*"):
    if path.suffix not in (".cpp", ".h") or path in (CPP, HDR):
        continue
    text = path.read_text()
    for name in EXTERNAL:
        if defs(text, name):
            fail(f"{name} is also defined in {path.relative_to(SRC)}")

# The four external entry points are in the anonymous namespace's complement.
ns_end = cpp_text.index("} // namespace")
for name in EXTERNAL:
    m = re.search(rf"^[A-Za-z_][\w \*]*?\b{name}\s*\([^;{{]*\)\s*{{", cpp_text, re.M)
    if m.start() < ns_end:
        fail(f"{name} must have external linkage (defined outside the anonymous namespace)")

if "void connectivityFailsafeSupervisor();" not in hdr_text:
    fail("ConnectivityFailsafe.h must declare connectivityFailsafeSupervisor()")
if '#include "power/ConnectivityFailsafe.h"' not in main_text:
    fail("Generalized-Core-Counter.cpp must include power/ConnectivityFailsafe.h")
if len(re.findall(r"^\s*connectivityFailsafeSupervisor\(\);", main_text, re.M)) != 1:
    fail("main must call connectivityFailsafeSupervisor() exactly once")

common = COMMON.read_text()
for name in ("clearConnectivityFailsafeRecovery", "claimFailsafeDeferLog", "failsafeDeferReasonName"):
    if len(re.findall(rf"\b{name}\s*\(", common)) != 1:
        fail(f"State_Common.h must declare {name} exactly once")

# Failsafe state is persisted, never retained: the moved code names no retained variable.
if re.search(r"\bretained(Boot|LoopForensics)\b", cpp_text):
    fail("ConnectivityFailsafe.cpp must not reference retained variables")


def strip_comments_and_strings(code):
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.S)
    code = re.sub(r"//[^\n]*", "", code)
    return re.sub(r'"(?:\\.|[^"\\\n])*"', '""', code)


for label, text in (("ConnectivityFailsafe.cpp", cpp_text), ("ConnectivityFailsafe.h", hdr_text)):
    if re.search(r"\bretained\b", strip_comments_and_strings(text)):
        fail(f"{label} must not use the 'retained' keyword")

print("PASS: connectivity failsafe moved to power/ConnectivityFailsafe.cpp, defined once")
