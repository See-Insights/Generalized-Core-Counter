#!/usr/bin/env python3
"""Persisted-layout preservation for all three records (WO-2026-09-23-001, Step 5).

Revised acceptance criterion 10. The Stage 3 draft named only SysData and
CurrentData; Finding 3 caught that SensorData (/usr/sensor.dat) is a third
StorageHelperRK-backed record with exactly the same exposure.

A deployed device reads back /usr/sysStatus.dat, /usr/current.dat and
/usr/sensor.dat using nothing but these sizes, offsets and magic/version
pairs. If the facade split moved any of them, every provisioned device would
misread or re-initialise its stored configuration on the next boot.

Method: extract the three struct definitions plus the magic/version constants
from the baseline revision AND from the working tree, compile both for the
real ARM target with the device toolchain, and diff the emitted data words.
Comparing compiler output rather than source text is deliberate - a padding,
alignment or type-width change can alter layout while leaving the source
looking reassuringly similar.
"""
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
HEADER = REPO_ROOT / "src" / "MyPersistentData.h"
BASELINE_REV = os.environ.get("PERSIST_LAYOUT_BASELINE_REV", "d9e3b6a")
ARM_GXX = Path("/Users/chipmc/.particle/toolchains/gcc-arm/10.2.1/bin/arm-none-eabi-g++")
ARM_FLAGS = [
    "-std=gnu++14", "-mcpu=cortex-m4", "-mthumb", "-mabi=aapcs",
    "-mfloat-abi=softfp", "-mfpu=fpv4-sp-d16", "-Os", "-S",
]

PROBE = """
extern "C" const unsigned long layout_probe[] = {
  sizeof(SysData), sizeof(CurrentData), sizeof(SensorData),
  offsetof(SysData, structuresVersion), offsetof(SysData, timeZoneStr),
  sizeof(((SysData *)0)->timeZoneStr), offsetof(SysData, webhookName),
  sizeof(((SysData *)0)->webhookName), offsetof(SysData, lastWatchdogSource),
  offsetof(SysData, thermalChargeArmHighC), offsetof(SysData, thermalChargeReleaseLowC),
  offsetof(SysData, currentBatteryTier), offsetof(SysData, connectivityRecoveryStage),
  offsetof(SysData, lastConnection), offsetof(SysData, connectAttemptBudgetSec),
  offsetof(CurrentData, alertCode), offsetof(CurrentData, lastAlertTime),
  offsetof(CurrentData, stateOfCharge), offsetof(CurrentData, hourlyCount),
  offsetof(CurrentData, occupied), offsetof(CurrentData, occupancyStartTime),
  offsetof(CurrentData, totalOccupiedSeconds), offsetof(CurrentData, internalTempC),
  offsetof(SensorData, type), offsetof(SensorData, setting1), offsetof(SensorData, setting4),
  SYS_DATA_MAGIC, SYS_DATA_VERSION,
  CURRENT_DATA_MAGIC, CURRENT_DATA_VERSION,
  SENSOR_DATA_MAGIC, SENSOR_DATA_VERSION,
};
"""

PREAMBLE = """#include <stdint.h>
#include <stddef.h>
#include <time.h>
// 16-byte stand-in, field-for-field identical to
// StorageHelperRK::PersistentDataBase::SavedDataHeader.
struct SavedDataHeader { uint32_t magic; uint16_t version; uint16_t size; uint32_t hash; uint32_t reserved1; };
"""


def fail(msg):
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def extract_struct(header_text, name):
    """Return the body of `class <name> { ... };` with the header type localised."""
    start = header_text.find(f"class {name} {{")
    if start < 0:
        fail(f"could not find `class {name} {{` in the header - extraction is broken, not the layout")
    depth = 0
    i = header_text.index("{", start)
    for j in range(i, len(header_text)):
        if header_text[j] == "{":
            depth += 1
        elif header_text[j] == "}":
            depth -= 1
            if depth == 0:
                body = header_text[start : j + 1] + ";"
                return body.replace(
                    "StorageHelperRK::PersistentDataBase::SavedDataHeader", "SavedDataHeader"
                )
    fail(f"unbalanced braces while extracting {name}")


def build_probe(header_text):
    parts = [PREAMBLE]
    for name in ("SysData", "SensorData", "CurrentData"):
        parts.append(extract_struct(header_text, name))
    consts = re.findall(
        r"((?:SYS|CURRENT|SENSOR)_DATA_(?:MAGIC|VERSION))\s*=\s*(0x[0-9a-fA-F]+|\d+)", header_text
    )
    if len(consts) != 6:
        fail(f"expected 6 magic/version constants, found {len(consts)}: {consts}")
    for cname, value in consts:
        parts.append(f"constexpr unsigned long {cname} = {value};")
    parts.append(PROBE)
    return "\n".join(parts)


def data_words(source, workdir, tag):
    src = workdir / f"{tag}.cpp"
    asm = workdir / f"{tag}.s"
    src.write_text(source)
    result = subprocess.run(
        [str(ARM_GXX)] + ARM_FLAGS + [str(src), "-o", str(asm)],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        fail(f"ARM compile of the {tag} layout probe failed:\n{result.stderr}")
    words = [
        line.strip()
        for line in asm.read_text().splitlines()
        if re.match(r"^\s*\.(word|long|quad)\b", line)
    ]
    if not words:
        fail(f"{tag} probe emitted no data words - extraction is broken, not the layout")
    return words


def main():
    if not ARM_GXX.is_file():
        print(f"SKIP: ARM toolchain not found at {ARM_GXX}")
        return

    current_header = HEADER.read_text()
    baseline_header = subprocess.run(
        ["git", "-C", str(REPO_ROOT), "show", f"{BASELINE_REV}:src/MyPersistentData.h"],
        capture_output=True, text=True,
    )
    if baseline_header.returncode != 0:
        fail(f"could not read src/MyPersistentData.h at {BASELINE_REV}: {baseline_header.stderr}")

    with tempfile.TemporaryDirectory() as tmp:
        workdir = Path(tmp)
        baseline_words = data_words(build_probe(baseline_header.stdout), workdir, "baseline")
        current_words = data_words(build_probe(current_header), workdir, "current")

    if baseline_words != current_words:
        print(f"FAIL: persisted layout CHANGED vs {BASELINE_REV} - a deployed "
              "device would misread its stored data", file=sys.stderr)
        for b, c in zip(baseline_words, current_words):
            if b != c:
                print(f"  baseline {b!r} != current {c!r}", file=sys.stderr)
        sys.exit(1)

    print(f"OK: SysData/CurrentData/SensorData sizes and offsets identical to {BASELINE_REV} (ARM cortex-m4)")
    print("OK: all three magic/version pairs unchanged")
    print(f"OK: {len(current_words)} emitted layout words compared, all equal")
    print("persisted_layout_preservation_test: layout preserved for all three records")


if __name__ == "__main__":
    main()
