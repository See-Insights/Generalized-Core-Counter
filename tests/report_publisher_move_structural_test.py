#!/usr/bin/env python3
"""WO-2026-10-19-002: report publishing lives in cloud/ReportPublisher.cpp and
dailyCleanup() in state/State_Report.cpp, each defined once and not in main."""
import re
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parent.parent / "src"
MAIN = SRC / "Generalized-Core-Counter.cpp"
CPP = SRC / "cloud" / "ReportPublisher.cpp"
HDR = SRC / "cloud" / "ReportPublisher.h"
REPORT = SRC / "state" / "State_Report.cpp"

PUBLISHER_FUNCS = [
    "isAutoClearAfterReportAlert", "publishData", "publishHibernateWakeForensics",
    "UbidotsHandler", "publishDiagnosticSafe",
]
HEADER_DECLS = ["UbidotsHandler", "publishHibernateWakeForensics"]
CONSTANTS = ["REPORT_FORENSICS_SLOW_LOG_THRESHOLD_MS", "REPORT_FORENSICS_ABNORMAL_WARN_THRESHOLD_MS"]


def fail(msg):
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def defs(text, name):
    return re.findall(rf"^[A-Za-z_][\w \*]*?\b{name}\s*\([^;{{]*\)\s*{{", text, re.M)


def strip_comments_and_strings(code):
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.S)
    code = re.sub(r"//[^\n]*", "", code)
    return re.sub(r'"(?:\\.|[^"\\\n])*"', '""', code)


main_text, cpp_text, hdr_text, report_text = (p.read_text() for p in (MAIN, CPP, HDR, REPORT))

for name in PUBLISHER_FUNCS + ["dailyCleanup"]:
    if defs(main_text, name):
        fail(f"{name} is still defined in Generalized-Core-Counter.cpp")

for name in PUBLISHER_FUNCS:
    if len(defs(cpp_text, name)) != 1:
        fail(f"{name} must be defined exactly once in ReportPublisher.cpp")
if defs(cpp_text, "dailyCleanup"):
    fail("dailyCleanup must not be defined in ReportPublisher.cpp")
if len(defs(report_text, "dailyCleanup")) != 1:
    fail("dailyCleanup must be defined exactly once in State_Report.cpp")

# No other translation unit may define any moved function.
for path in SRC.rglob("*"):
    if path.suffix not in (".cpp", ".h") or path in (CPP, REPORT):
        continue
    text = path.read_text()
    for name in PUBLISHER_FUNCS + ["dailyCleanup"]:
        if defs(text, name):
            fail(f"{name} is also defined in {path.relative_to(SRC)}")

if not re.search(r"^static\s+bool\s+isAutoClearAfterReportAlert\s*\(", cpp_text, re.M):
    fail("isAutoClearAfterReportAlert must stay static (internal to ReportPublisher.cpp)")
for name in CONSTANTS:
    if main_text.count(name):
        fail(f"{name} must move out of Generalized-Core-Counter.cpp")
    if len(re.findall(rf"^constexpr unsigned long {name} =", cpp_text, re.M)) != 1:
        fail(f"{name} must be defined once in ReportPublisher.cpp")

for name in HEADER_DECLS:
    if len(re.findall(rf"\b{name}\s*\(", hdr_text)) != 1:
        fail(f"ReportPublisher.h must declare {name} exactly once")
if '#include "cloud/ReportPublisher.h"' not in main_text:
    fail("Generalized-Core-Counter.cpp must include cloud/ReportPublisher.h")

# The close order in handleReportingState() is publish, then dailyCleanup(), then restart.
publish_pos = report_text.find("publishData(due ? boundary - 1 : 0);")
cleanup_pos = report_text.find("    dailyCleanup();")
if not (0 <= publish_pos < cleanup_pos):
    fail("State_Report.cpp must call publishData(...) before dailyCleanup()")

# Report publishing is not retained state: no 'retained' keyword in the new files or dailyCleanup().
daily = report_text[report_text.index("void dailyCleanup()"):]
for label, text in (("ReportPublisher.cpp", cpp_text), ("ReportPublisher.h", hdr_text),
                    ("dailyCleanup() in State_Report.cpp", daily)):
    if re.search(r"\bretained\b", strip_comments_and_strings(text)):
        fail(f"{label} must not use the 'retained' keyword")

print("PASS: report publishing in cloud/ReportPublisher.cpp, dailyCleanup in State_Report.cpp, each defined once")
