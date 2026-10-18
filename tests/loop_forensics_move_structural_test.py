#!/usr/bin/env python3
"""WO-2026-10-16-001: loop forensics and the awake watchdog live in
observability/LoopForensics.cpp; main keeps only the `retained` definitions."""
import re
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parent.parent / "src"
MAIN = SRC / "Generalized-Core-Counter.cpp"
LOOP = SRC / "observability" / "LoopForensics.cpp"
HDR = SRC / "observability" / "LoopForensics.h"

MOVED = [
    "awakeWatchdogSleepStrategyName", "appBreadcrumbName", "updateRetainedValue",
    "loopStageName", "loopStageForensicName", "resetRetainedLoopForensics",
    "ensureRetainedLoopForensicsInitialized", "refreshRetainedLoopForensics",
    "logLoopStageLine", "noteLoopStageDuration", "setAppBreadcrumb", "setLoopStage",
    "serviceAwakeWatchdog", "pauseAwakeWatchdogForSleep",
    "restoreAwakeWatchdogAfterWake", "awakeWatchdogExpiredHandler", "appWatchdogHandler",
]


def fail(msg):
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def defs(text, name):
    return re.findall(rf"^(?:static )?(?:const char \*|void |bool )\s*{name}\s*\([^;{{]*\)\s*{{", text, re.M)


main_text, loop_text, hdr_text = MAIN.read_text(), LOOP.read_text(), HDR.read_text()

for name in MOVED:
    if defs(main_text, name):
        fail(f"{name} is still defined in Generalized-Core-Counter.cpp")
    if name in ("appWatchdogHandler", "awakeWatchdogExpiredHandler"):
        continue  # one definition per Wiring_Watchdog branch, checked below
    if len(defs(loop_text, name)) != 1:
        fail(f"{name} must be defined exactly once in LoopForensics.cpp")
if len(defs(loop_text, "awakeWatchdogExpiredHandler")) != 1 or len(defs(loop_text, "appWatchdogHandler")) != 1:
    fail("each watchdog handler must be defined exactly once in LoopForensics.cpp")

for pat in (r"enum AppBreadcrumb\b", r"enum AwakeWatchdogSleepStrategy\b"):
    if re.search(pat, main_text) or re.search(pat, loop_text):
        fail(f"{pat} must be defined only in LoopForensics.h")
    if len(re.findall(pat, hdr_text)) != 1:
        fail(f"{pat} must be defined exactly once in LoopForensics.h")

if not re.search(r"^retained RetainedBoot retainedBoot\b", main_text, re.M):
    fail("retained retainedBoot must stay defined in Generalized-Core-Counter.cpp")
if not re.search(r"^retained RetainedLoopForensics retainedLoopForensics\b", main_text, re.M):
    fail("retained retainedLoopForensics must stay defined in Generalized-Core-Counter.cpp")
for var in ("retainedBoot", "retainedLoopForensics"):
    if re.search(rf"^retained [\w ]+ {var}\b", loop_text + hdr_text, re.M):
        fail(f"retained {var} must not be defined in the new files")
for var in ("appBreadcrumb", "appBreadcrumbMs"):
    if not re.search(rf"^static [\w]+ &{var} = retainedBoot\.{var};", loop_text, re.M):
        fail(f"LoopForensics.cpp must bind {var} to retainedBoot.{var}")
    if re.search(rf"^(?:extern )?retained [\w ]+ {var}\b", main_text + loop_text + hdr_text, re.M):
        fail(f"{var} must not be a separate retained variable")
if not re.search(r"^extern retained RetainedBoot retainedBoot;", hdr_text, re.M):
    fail("LoopForensics.h must declare extern retained RetainedBoot retainedBoot")

print("PASS: loop forensics and awake watchdog moved; retained definitions stay in main")
