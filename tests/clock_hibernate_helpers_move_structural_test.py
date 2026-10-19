#!/usr/bin/env python3
"""WO-2026-10-19-003: the clock and hibernate helpers live in time/ClockDiagnostics.cpp
and time/HibernateEligibility.cpp, each defined once, and not in main or State_Sleep.cpp."""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src"
MAIN = SRC / "Generalized-Core-Counter.cpp"
SLEEP = SRC / "state" / "State_Sleep.cpp"
CLOCK_CPP = SRC / "time" / "ClockDiagnostics.cpp"
CLOCK_HDR = SRC / "time" / "ClockDiagnostics.h"
HIB_CPP = SRC / "time" / "HibernateEligibility.cpp"
HIB_HDR = SRC / "time" / "HibernateEligibility.h"
CYCLE = SRC / "time" / "HibernateCycle.cpp"

CLOCK_FUNCS = ["ab1805WakeReasonNameLocal", "ab1805WakeReasonName", "logTimeDiag"]
HIB_FUNCS = ["isDeviceOsAtLeast6400", "clearAb1805StaleAlarmInterrupts", "shouldUseBoronRtcAlarmHibernate"]


def fail(msg):
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def defs(text, name):
    return re.findall(rf"^[A-Za-z_][\w \*]*?\b{name}\s*\([^;{{]*\)\s*{{", text, re.M)


def strip_comments_and_strings(code):
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.S)
    code = re.sub(r"//[^\n]*", "", code)
    return re.sub(r'"(?:\\.|[^"\\\n])*"', '""', code)


main_text, sleep_text, clock_text, hib_text = (p.read_text() for p in (MAIN, SLEEP, CLOCK_CPP, HIB_CPP))
clock_hdr, hib_hdr = CLOCK_HDR.read_text(), HIB_HDR.read_text()

for name in CLOCK_FUNCS + HIB_FUNCS:
    for label, text in (("Generalized-Core-Counter.cpp", main_text), ("State_Sleep.cpp", sleep_text)):
        if defs(text, name):
            fail(f"{name} is still defined in {label}")

for name in CLOCK_FUNCS:
    if len(defs(clock_text, name)) != 1:
        fail(f"{name} must be defined exactly once in ClockDiagnostics.cpp")
for name in HIB_FUNCS:
    if len(defs(hib_text, name)) != 1:
        fail(f"{name} must be defined exactly once in HibernateEligibility.cpp")

# No other translation unit may define any moved function.
for path in SRC.rglob("*"):
    if path.suffix not in (".cpp", ".h") or path in (CLOCK_CPP, HIB_CPP):
        continue
    text = path.read_text()
    for name in CLOCK_FUNCS + HIB_FUNCS:
        if defs(text, name):
            fail(f"{name} is also defined in {path.relative_to(SRC)}")

# The Boron guard wraps the Sleep functions in the .cpp and their declarations in the header.
guard = "#if PLATFORM_ID == PLATFORM_BORON"
if hib_text.count(guard) != 1 or not hib_text.rstrip().endswith("#endif"):
    fail("HibernateEligibility.cpp must wrap its three functions in one Boron guard")
if hib_hdr.count(guard) != 1 or not hib_hdr.rstrip().endswith("#endif"):
    fail("HibernateEligibility.h must wrap its declarations in one Boron guard")
if hib_text.index(guard) > min(hib_text.index(f"{n}(") for n in HIB_FUNCS):
    fail("the Boron guard must precede the first moved function")
for name in HIB_FUNCS:
    if len(re.findall(rf"\b{name}\s*\(", hib_hdr)) != 1:
        fail(f"HibernateEligibility.h must declare {name} exactly once")
if len(re.findall(r"\bab1805WakeReasonName\s*\(", clock_hdr)) != 1:
    fail("ClockDiagnostics.h must declare ab1805WakeReasonName exactly once")
if not re.search(r"^namespace\s*{.*?^const char \*ab1805WakeReasonNameLocal.*?^}\s*// namespace",
                 clock_text, re.M | re.S):
    fail("ab1805WakeReasonNameLocal must stay internal (anonymous namespace)")
if "ab1805WakeReasonNameLocal" in clock_hdr:
    fail("ab1805WakeReasonNameLocal must not be declared in the header")
if '#include "state/StateMachine.h"' not in hib_text:
    fail("HibernateEligibility.cpp must reach ab1805 through state/StateMachine.h")
if '#include "time/HibernateEligibility.h"' not in sleep_text:
    fail("State_Sleep.cpp must include time/HibernateEligibility.h")

# logTimeDiag stays declared in State_Common.h.
if "void logTimeDiag(bool isOpen);" not in (SRC / "state" / "State_Common.h").read_text():
    fail("logTimeDiag must stay declared in state/State_Common.h")

# HibernateCycle.cpp keeps the hibernate retained fields and must not be touched by this move.
diff = subprocess.run(["git", "diff", "--quiet", "HEAD", "--", str(CYCLE)], cwd=ROOT)
if diff.returncode != 0:
    fail("time/HibernateCycle.cpp must be unchanged")

# Clock and hibernate-eligibility helpers are not retained state.
for label, text in (("ClockDiagnostics.cpp", clock_text), ("ClockDiagnostics.h", clock_hdr),
                    ("HibernateEligibility.cpp", hib_text), ("HibernateEligibility.h", hib_hdr)):
    if re.search(r"\bretained\b", strip_comments_and_strings(text)):
        fail(f"{label} must not use the 'retained' keyword")

print("PASS: clock and hibernate helpers in time/ClockDiagnostics and time/HibernateEligibility, each defined once")
