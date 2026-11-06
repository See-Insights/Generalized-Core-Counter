#!/usr/bin/env python3
"""WO-2026-11-02-001 (Amendment 1): pollAndRemediate() takes vcell as a required parameter (no
default that could silently disable the latch recovery), the one production call passes vcell, the
latch raises alert 22 (not 21), and alert 22 ranks at severity 3 with 20 and 21."""
import os
import re
import sys
from pathlib import Path

ROOT = Path(os.environ.get("PMIC_LATCH_SRC_ROOT") or Path(__file__).resolve().parent.parent)
SRC = ROOT / "src"

header = (SRC / "power/PmicFaultMonitor.h").read_text(errors="replace")
decl = re.search(r"Registers pollAndRemediate\(([^)]*)\);", header)
if not decl:
    sys.exit("FAIL: pollAndRemediate declaration not found")
if not re.search(r",\s*float vcell\s*$", decl.group(1)) or "=" in decl.group(1):
    sys.exit("FAIL: pollAndRemediate must end with a required `float vcell` parameter (no defaults)")

calls = []
for p in sorted(SRC.rglob("*.cpp")):
    if p.name == "PmicFaultMonitor.cpp":
        continue
    calls += re.findall(r"PmicFaultMonitor::pollAndRemediate\((pmic,[^;]*?)\);", p.read_text(errors="replace"), re.S)
if len(calls) != 1:
    sys.exit(f"FAIL: expected 1 production pollAndRemediate call, found {len(calls)}")
if not re.search(r",\s*vcell\s*$", calls[0].strip()):
    sys.exit("FAIL: pollAndRemediate call must pass vcell as its last argument")

monitor = (SRC / "power/PmicFaultMonitor.cpp").read_text(errors="replace")
latch = re.search(r"const bool chargeStuck =.*?\n  \}\n", monitor, re.S)
if not latch:
    sys.exit("FAIL: charge latch block not found")
if re.findall(r"raiseAlert\((\d+)\)", latch.group(0)) != ["22"]:
    sys.exit("FAIL: the charge latch must raise alert 22 (and only 22)")
if "raiseAlert(22)" in monitor.replace(latch.group(0), ""):
    sys.exit("FAIL: alert 22 must be raised only by the charge latch")
if "currentAlert == 21 && regs.chargeStatus != 2)" not in monitor:
    sys.exit("FAIL: the legacy alert-21 clear must stay `regs.chargeStatus != 2`")

severity = (SRC / "MyPersistentData.cpp").read_text(errors="replace")
tier3 = re.search(r"switch \(code\) \{(.*?)return 3;", severity, re.S)
if not tier3 or not re.search(r"case 20:.*case 21:.*case 22:", tier3.group(1), re.S):
    sys.exit("FAIL: alert 22 must share severity 3 with alerts 20 and 21")
print("pmic_latch_call_site_structural_test: passed")
