#!/usr/bin/env python3
"""
WO-2026-10-07-002 (Step 6 WO 1a) - structural retirement check for the
config/downgrade overwrite.

Usage: connection_mode_downgrade_structural_test.py [SRC_DIR]
SRC_DIR defaults to the repo's src/. tests/connection_mode_downgrade_test.sh
runs it against a mutated copy to prove it can fail.

Invariants on the real source:
  1. The old ambiguous accessor get_connectionMode( no longer exists anywhere.
  2. set_connectionMode is called only from configuration apply
     (cloud/ConfigApply.cpp) and the factory defaults in MyPersistentData.cpp;
     the other files that name it are the declarations.
  3. get_configuredConnectionMode( is read only where configuration is
     reported or compared (DeviceStatusPublisher.cpp, ConfigApply.cpp), by
     the battery decision (BatteryAuthorityCommand.cpp), by the one derivation
     (PowerManager.cpp), and at the declarations/definitions.
  4. Every connect/sleep/standby/report reader uses the mode in use
     (PowerManager::instance().effectiveConnectionMode()); the per-file counts
     below are the reader table of the WO.
  5. The ConfigApply connection-mode block does not touch lowBatteryMode.
"""
import pathlib
import re
import sys

default_src = pathlib.Path(__file__).resolve().parent.parent / "src"
SRC = pathlib.Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else default_src

SETTER_FILES = {
    "cloud/ConfigApply.cpp",
    "MyPersistentData.cpp",
    "MyPersistentData.h",
    "persist/SystemConfig.h",
}

CONFIGURED_READERS = {
    "cloud/DeviceStatusPublisher.cpp",
    "cloud/ConfigApply.cpp",
    "power/BatteryAuthorityCommand.cpp",
    "power/PowerManager.cpp",
    "MyPersistentData.cpp",
    "MyPersistentData.h",
    "persist/SystemConfig.h",
}

EFFECTIVE_READERS = {
    "state/State_Connect.cpp": 1,
    "state/State_Sleep.cpp": 10,
    "state/State_Idle.cpp": 7,
    "state/State_Report.cpp": 2,
    "state/State_Error.cpp": 1,
    "state/State_Modes.cpp": 2,
    "Generalized-Core-Counter.cpp": 1,
    "diagnostics/ConnectivityFailsafeTest.cpp": 2,
}
EFFECTIVE_CALL = "PowerManager::instance().effectiveConnectionMode()"

failures = []


def fail(message):
    failures.append(message)


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


sources = {}
for path in sorted(SRC.rglob("*")):
    if path.suffix in {".cpp", ".h"}:
        sources[path.relative_to(SRC).as_posix()] = strip_comments(path.read_text())

for rel, text in sources.items():
    if "get_connectionMode(" in text:
        fail(f"{rel}: still calls the retired get_connectionMode()")

    if "set_connectionMode" in text and rel not in SETTER_FILES:
        fail(f"{rel}: names set_connectionMode but is not configuration apply or a declaration")

    if "get_configuredConnectionMode(" in text and rel not in CONFIGURED_READERS:
        fail(f"{rel}: reads the configured mode but does not report/compare configuration")

config_apply = sources.get("cloud/ConfigApply.cpp", "")
if "set_connectionMode(" not in config_apply:
    fail("cloud/ConfigApply.cpp: no longer applies the ledger connection mode")

battery_command = sources.get("power/BatteryAuthorityCommand.cpp", "")
if "set_connectionMode" in battery_command:
    fail("power/BatteryAuthorityCommand.cpp: writes the connection mode")

for rel, expected in EFFECTIVE_READERS.items():
    text = sources.get(rel)
    if text is None:
        fail(f"{rel}: missing")
        continue
    found = text.count(EFFECTIVE_CALL)
    if found != expected:
        fail(f"{rel}: expected {expected} mode-in-use reads, found {found}")

block_match = re.search(
    r'getMergedIntValue\(defaultModes, deviceModes, "connectionMode", connectionMode\)\) \{(.*?)getMergedIntValue\(defaultModes, deviceModes, "reportingMode"',
    config_apply,
    flags=re.S,
)
if not block_match:
    fail("cloud/ConfigApply.cpp: connection-mode block not found")
else:
    block = block_match.group(1)
    if "lowBatteryMode" in block or "BatteryAuthority" in block:
        fail("cloud/ConfigApply.cpp: the connection-mode block touches the battery's downgrade flag")
    if "get_configuredConnectionMode()" not in block:
        fail("cloud/ConfigApply.cpp: the connection-mode block does not compare the configured mode")

power_manager = sources.get("power/PowerManager.cpp", "")
if "effectiveConnectionMode" not in power_manager or "get_lowBatteryMode()" not in power_manager:
    fail("power/PowerManager.cpp: the derivation of the mode in use is missing")

if failures:
    for message in failures:
        print(f"FAIL: {message}", file=sys.stderr)
    sys.exit(1)

print("connection_mode_downgrade_structural_test: OK")
