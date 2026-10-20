#!/usr/bin/env python3
"""WO-2026-10-20-001: one definition of each power-source code and PMIC REG09 mask in src/;
availabilityLabel, batteryContextLabel and hasFuelGauge are gone from src/."""
import os
import re
import sys
from pathlib import Path

ROOT = Path(os.environ.get("POWER_CONSTANTS_SRC_ROOT") or Path(__file__).resolve().parent.parent)
SRC = ROOT / "src"

CODES = ["kPowerSourceUnknown", "kPowerSourceVin", "kPowerSourceUsbHost",
         "kPowerSourceUsbAdapter", "kPowerSourceUsbOtg", "kPowerSourceBattery"]
MASKS = ["kPmicChargeFaultMask", "kPmicBatFaultMask", "kPmicNtcFaultMask",
         "PMIC_CHRG_FAULT_MASK", "PMIC_BAT_FAULT_MASK"]
GONE = ["availabilityLabel", "batteryContextLabel", "hasFuelGauge"]
EXPECTED_HOME = {
    **{c: "power/PowerPlatform.h" for c in CODES},
    "kPmicChargeFaultMask": "power/PmicFaultMonitor.h",
    "kPmicBatFaultMask": "power/PmicFaultMonitor.h",
    "kPmicNtcFaultMask": "power/PmicFaultMonitor.h",
}


def fail(msg):
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


texts = {}
for p in sorted(SRC.rglob("*")):
    if p.suffix in (".h", ".cpp", ".c", ".hpp", ".ino"):
        texts[str(p.relative_to(SRC))] = strip_comments(p.read_text(errors="replace"))

for name in CODES + MASKS:
    hits = []
    for rel, text in texts.items():
        n = len(re.findall(rf"\b{name}\s*=(?!=)", text)) + len(re.findall(rf"#\s*define\s+{name}\b", text))
        hits += [rel] * n
    expected = [EXPECTED_HOME[name]] if name in EXPECTED_HOME else []
    if hits != expected:
        fail(f"{name} defined in {hits or 'nowhere'}; expected exactly {expected or 'none (retired name)'}")

for name in GONE:
    for rel, text in texts.items():
        if re.search(rf"\b{name}\b", text):
            fail(f"{name} must not appear in src/ ({rel})")

print("PASS: power-source codes and PMIC masks each defined once; dead labels and hasFuelGauge gone")
