#!/usr/bin/env python3
"""WO-2026-10-20-003 (Stage 6): state/SleepTeardown owns the nine sleep-teardown fields and the
cloud-disconnect / modem-teardown block; State_Sleep.cpp only calls it; the new files carry no
retained-memory keyword. Each check is first shown to fire on a probe that breaks it."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "src"
OWNER_CPP = SRC / "state" / "SleepTeardown.cpp"
OWNER_HDR = SRC / "state" / "SleepTeardown.h"
SLEEP_CPP = SRC / "state" / "State_Sleep.cpp"
FIELDS = ("cloudDisconnectCompleteLogged", "radioOffCompleteLogged", "disconnectRequested",
          "disconnectRequestStartMs", "cloudDisconnectStartMs", "cloudDisconnectElapsedMs",
          "modemOffElapsedMs", "lastDisconnectProgressMs", "lastGateProgressMs")
NAMES = "|".join(FIELDS)
WRITE = re.compile(
    rf"(?:\b(?:{NAMES})\b\s*(?:=(?!=)|\+\+|--|[-+*/|&^%]=|<<=|>>=)"
    rf"|(?:\+\+|--)\s*[\w.>-]*\b(?:{NAMES})\b"
    rf"|&\s*[\w.>-]*\b(?:{NAMES})\b)")
DEFINITION = re.compile(rf"\b(?:bool|unsigned long|uint32_t|uint16_t|int|long)\s+(?:{NAMES})\b(?!\s*\()")
# What moved out of State_Sleep.cpp and must not be spelled there any more.
MOVED_OUT = ("connectionModeLabel", "batteryTierLabel", "logTeardownContext", "requestFullDisconnectAndRadioOff",
             "sleep-disconnect-timeout",
             "sleep-precondition-timeout", "raiseAlert(15)", "markTeardownStart", "markTeardownEnd",
             "markModemUnstable", "maybeClearModemUnstable", "DISCONNECT_BUDGET", "DISCONNECT_STANDBY_MAX_MS",
             "DISCONNECT_WAIT", "pre-sleep gate blocked", "sleepPreconditionsSatisfied")


def fail(msg):
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def strip_comments_and_strings(code):
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.S)
    code = re.sub(r"//[^\n]*", "", code)
    return re.sub(r'"(?:\\.|[^"\\\n])*"', '""', code)


def field_writes_or_definitions(code):
    return [line.strip() for line in strip_comments_and_strings(code).splitlines()
            if WRITE.search(line) or DEFINITION.search(line)]


def outside_owner_problems(code):
    """Problems in any src file other than SleepTeardown.cpp."""
    problems = []
    hits = field_writes_or_definitions(code)
    if hits:
        problems.append(f"writes or defines a teardown field outside SleepTeardown.cpp: {hits[0]}")
    return problems


def sleep_cpp_problems(code):
    problems = outside_owner_problems(code)
    plain = strip_comments_and_strings(code)
    raw = code
    for word in MOVED_OUT:
        if word in raw:
            problems.append(f"State_Sleep.cpp still contains moved teardown text: {word}")
    if '#include "state/SleepTeardown.h"' not in raw:
        problems.append("State_Sleep.cpp does not include state/SleepTeardown.h")
    for call in ("SleepTeardown::enter()", "SleepTeardown::guardLostTimestamp()", "SleepTeardown::run("):
        if len(re.findall(re.escape(call), plain)) != 1:
            problems.append(f"State_Sleep.cpp must call {call} exactly once")
    if len(re.findall(r"SleepTeardown::disconnectRequested\(\)", plain)) != 3:
        problems.append("State_Sleep.cpp must read SleepTeardown::disconnectRequested() at its 3 readers")
    run = re.search(r"if \(SleepTeardown::run\(.*?\) != SleepTeardown::Result::READY\) \{\s*return;\s*\}", plain, re.S)
    if not run:
        problems.append("State_Sleep.cpp must return unless SleepTeardown::run(...) is READY")
    guard, first_read = plain.find("SleepTeardown::guardLostTimestamp()"), plain.find("SleepTeardown::disconnectRequested()")
    if guard < 0 or first_read < 0 or guard > first_read:
        problems.append("the lost-timestamp guard must run before the first disconnectRequested() read")
    if run and plain.find("setAppBreadcrumb(23)") < run.end():
        problems.append("setAppBreadcrumb(23) must follow the SleepTeardown::run gate")
    return problems


# The detector itself: writes and definitions are flagged, reads, calls, comments and strings are not.
for probe in ("cloudDisconnectStartMs = 0;", "disconnectRequested=true;", "modemOffElapsedMs += 5;",
              "++lastGateProgressMs;", "unsigned long *p = &lastDisconnectProgressMs;",
              "static bool cloudDisconnectCompleteLogged = false;", "bool radioOffCompleteLogged;",
              "unsigned long disconnectRequestStartMs = 0;", "if (x) disconnectRequested = false;"):
    if not field_writes_or_definitions(probe):
        fail(f"detector missed a write or definition: {probe}")
for probe in ("if (SleepTeardown::disconnectRequested()) {", "x = modemOffElapsedMs;",
              "if (disconnectRequested == false) {", "// disconnectRequested = true;",
              'Log.info("disconnectRequested = true");', "bool disconnectRequested();",
              "return s.disconnectRequested;"):
    if field_writes_or_definitions(probe):
        fail(f"detector flagged a read, call, comment or string: {probe}")

sleep = SLEEP_CPP.read_text()
owner = OWNER_CPP.read_text()
header = OWNER_HDR.read_text()

# Each State_Sleep.cpp rule fires on a probe built from the real file.
probes = {
    "a teardown field write": sleep + "\n  cloudDisconnectStartMs = 0;\n",
    "a teardown field definition": sleep + "\n  static unsigned long modemOffElapsedMs = 0;\n",
    "the moved label helper": sleep + "\nconst char *connectionModeLabel(int m) { return \"\"; }\n",
    "the moved alert": sleep + "\n  RecoveryState::raiseAlert(15);\n",
    "a second run call": sleep + "\n  SleepTeardown::run(a, b, c, d);\n",
    "a dropped include": sleep.replace('#include "state/SleepTeardown.h"', ""),
    "a dropped gate return": re.sub(r"(\) != SleepTeardown::Result::READY\) \{\s*)return;", r"\1", sleep, count=1),
}
for name, text in probes.items():
    if not sleep_cpp_problems(text):
        fail(f"probe not detected: {name}")

problems = sleep_cpp_problems(sleep)
if problems:
    fail(problems[0])

for path in sorted(SRC.rglob("*")):
    if path.suffix not in (".cpp", ".h") or path in (OWNER_CPP, SLEEP_CPP):
        continue
    problems = outside_owner_problems(path.read_text())
    if problems:
        fail(f"{path.relative_to(SRC)} {problems[0]}")

stripped = strip_comments_and_strings(owner)
for name in FIELDS:
    if len(re.findall(rf"^\s*(?:bool|unsigned long)\b[^;()]*\b{name}\b(?!\s*\()", stripped, re.M)) != 1:
        fail(f"SleepTeardown.cpp must declare {name} exactly once")
if len(field_writes_or_definitions(owner)) < 40:
    fail("SleepTeardown.cpp no longer writes the teardown fields (detector or owner broken)")
for call in ("Connectivity::requestRadioPowerOff()", "Connectivity::requestFullDisconnectAndRadioOff()",
             "Connectivity::requestCloudDisconnectOnly()", "RecoveryState::raiseAlert(15)",
             'transitionTo(ERROR_STATE, "sleep-disconnect-timeout")',
             'transitionTo(ERROR_STATE, "sleep-precondition-timeout")', "Particle.process()",
             "serviceAwakeWatchdog()", "setAppBreadcrumb(22)"):
    if call not in owner:
        fail(f"SleepTeardown.cpp lost {call}")
if "setAppBreadcrumb(23)" in owner:
    fail("setAppBreadcrumb(23) belongs to the handler, after the gate")

for token in ("enum class Result", "WAIT", "READY", "ERROR"):
    if token not in header:
        fail(f"SleepTeardown.h must declare {token}")
for fn in ("enter", "guardLostTimestamp", "disconnectRequested", "run"):
    if len(re.findall(rf"\b{fn}\(", header)) != 1:
        fail(f"SleepTeardown.h must declare {fn} once")

for path in (OWNER_CPP, OWNER_HDR):
    if re.search(r"retained", path.read_text(), re.I):
        fail(f"{path.name} must not mention retained")

print("sleep_teardown_structural_test: ok")
