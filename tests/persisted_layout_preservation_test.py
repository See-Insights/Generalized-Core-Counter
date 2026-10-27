#!/usr/bin/env python3
"""Persisted-layout freeze guard for all three records (WO-2026-10-22-003, P7-3).

A deployed device reads /usr/sysStatus.dat, /usr/current.dat and /usr/sensor.dat
using only these sizes, offsets and magic/version pairs, and v45 must still read
them after a downgrade. The records are frozen: no member may be appended,
inserted, removed, resized or moved, and no version may change.

Every data member of each class is extracted from the header in order (any
unrecognized declaration in the class body fails the test), and an offsetof/sizeof
table is compiled for the real ARM target. Each member must match the frozen
(name, type, offset, size) exactly; only `reserved*` slots may be renamed, and
only to another `reserved*` name of the same type, offset and size.
"""
import re
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
HEADER = REPO_ROOT / "src" / "MyPersistentData.h"
ARM_GXX = Path("/Users/chipmc/.particle/toolchains/gcc-arm/10.2.1/bin/arm-none-eabi-g++")
ARM_FLAGS = [
    "-std=gnu++14", "-mcpu=cortex-m4", "-mthumb", "-mabi=aapcs",
    "-mfloat-abi=softfp", "-mfpu=fpv4-sp-d16", "-Os", "-S",
]
CLASSES = ("SysData", "SensorData", "CurrentData")
PREFIX = {"SysData": "SYS", "SensorData": "SENSOR", "CurrentData": "CURRENT"}

# Frozen layout: (offset, size) per data member in declaration order, then the
# record size, from the v45 header. Do not edit to make a change pass.
FROZEN_SIZE = {"SysData": 272, "SensorData": 36, "CurrentData": 88}
# Each member is name:type:offset:size. Live members match by exact name and
# type; only `reserved*` slots may be renamed (to another `reserved*`).
_FROZEN_TEXT = {
    "SysData": """
        sysHeader:SavedDataHeader:0:16 reservedUint8_0:uint8_t:16:1 verboseMode:bool:17:1
        solarPowerMode:bool:18:1 reservedBool0:bool:19:1 lowBatteryMode:bool:20:1
        resetCount:uint8_t:21:1 timeZoneStr:char[39]:22:39 openTime:uint8_t:61:1
        closeTime:uint8_t:62:1 lastReport:time_t:64:8 lastConnection:time_t:72:8
        lastHookResponse:time_t:80:8 lastConnectionDuration:uint16_t:88:2
        reservedUint8_1:uint8_t:90:1 reservedBool1:bool:91:1 reportingInterval:uint16_t:92:2
        reservedBool2:bool:94:1 serialConnected:bool:95:1 verboseTimeoutMin:uint16_t:96:2
        reservedTime0:time_t:104:8 lastDailyCleanup:time_t:112:8 lastTimeSync:time_t:120:8
        sensorMode:uint8_t:128:1 connectionMode:uint8_t:129:1 reportingMode:uint8_t:130:1
        samplingMode:uint8_t:131:1 connectAttemptBudgetSec:uint16_t:132:2
        cloudDisconnectBudgetSec:uint16_t:134:2 modemOffBudgetSec:uint16_t:136:2
        currentBatteryTier:uint8_t:138:1 connectionAttemptCounter:uint8_t:139:1
        reservedFloat0:float:140:4 testConnectionDurationOverride:uint16_t:144:2
        reservedByte0:uint8_t:146:1 webhookName:char[64]:147:64 webhookEnabled:bool:211:1
        webhookTimeoutMs:uint32_t:212:4 connectivityRecoveryStage:uint8_t:216:1
        lastConnectivityRecoveryAction:time_t:224:8 connectivityRecoveryCount:uint8_t:232:1
        watchdogResetCount:uint16_t:234:2 lastWatchdogBreadcrumb:uint8_t:236:1
        lastWatchdogUptimeMs:uint32_t:240:4 lastWatchdogResetReasonData:uint32_t:244:4
        hasValidLedgerConfig:bool:248:1 configSource:uint8_t:249:1
        lastWatchdogSource:uint8_t:250:1 thermalChargeArmHighC:float:252:4
        thermalChargeArmLowC:float:256:4 thermalChargeReleaseHighC:float:260:4
        thermalChargeReleaseLowC:float:264:4
    """,
    "SensorData": """
        sensorHeader:SavedDataHeader:0:16 type:uint8_t:16:1 setting1:uint32_t:20:4
        setting2:uint32_t:24:4 setting3:uint32_t:28:4 setting4:uint32_t:32:4
    """,
    "CurrentData": """
        currentHeader:SavedDataHeader:0:16 reservedUint16_0:uint16_t:16:2
        reservedUint16_1:uint16_t:18:2 reservedUint16_2:uint16_t:20:2
        reservedUint16_3:uint16_t:22:2 lastCountTime:time_t:24:8 internalTempC:float:32:4
        reservedFloat1:float:36:4 alertCode:uint8_t:40:1 lastAlertTime:time_t:48:8
        stateOfCharge:float:56:4 batteryState:uint8_t:60:1 hourlyCount:uint16_t:62:2
        dailyCount:uint16_t:64:2 occupied:bool:66:1 lastOccupancyEvent:uint32_t:68:4
        occupancyStartTime:time_t:72:8 totalOccupiedSeconds:uint32_t:80:4
    """,
}
FROZEN_MEMBERS = {
    k: [(n, t, int(o), int(z)) for n, t, o, z in (tok.rsplit(":", 3) for tok in v.split())]
    for k, v in _FROZEN_TEXT.items()
}
# Members in the frozen table, header included: 52 / 6 / 18 (51 / 5 / 17 data members).
# (magic, version) per record.
FROZEN_CONSTS = {
    "SysData": (0x20a15e75, 4),
    "SensorData": (0x20a47e74, 1),
    "CurrentData": (0x20a99e74, 1),
}

PREAMBLE = """#include <stdint.h>
#include <stddef.h>
#include <time.h>
// 16-byte stand-in, field-for-field identical to
// StorageHelperRK::PersistentDataBase::SavedDataHeader.
struct SavedDataHeader { uint32_t magic; uint16_t version; uint16_t size; uint32_t hash; uint32_t reserved1; };
"""

MEMBER_RE = re.compile(r"^([\w:]+)\s+(\w+)(\[[^\]]*\])?\s*(?:=[^;]*|\{[^;]*\})?;$")
ACCESS_RE = re.compile(r"^(public|private|protected)\s*:$")
OTHER_OK_RE = re.compile(
    r"^(?:using\s|typedef\s|static\s+(?:const|constexpr)\s|friend\s)[^;]*;$"
    r"|^[\w:<>&*\s~]+\([^;{}]*\)\s*(?:const)?\s*(?:=\s*(?:delete|default))?\s*;$")


def fail(msg):
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def extract_struct(header_text, name):
    """Return (body, [(member name, type)] in order) of `class <name> { ... };`.

    Fails closed: any non-blank, non-comment line in the class body that is not a
    recognized member, access label, method declaration or using/static const is an error.
    """
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
                break
    else:
        fail(f"unbalanced braces while extracting {name}")
    body = body.replace("StorageHelperRK::PersistentDataBase::SavedDataHeader", "SavedDataHeader")
    code = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    code = re.sub(r"//[^\n]*", "", code)
    members = []
    for line in code.splitlines()[1:]:
        line = line.strip()
        if not line or line == "};" or ACCESS_RE.match(line):
            continue
        m = MEMBER_RE.match(line)
        if m:
            members.append((m.group(2), m.group(1) + (m.group(3) or "")))
        elif not OTHER_OK_RE.match(line):
            fail(f"{name}: unrecognized declaration in record class body (not a member the guard can freeze): {line}")
    return body, members


def build_probe(header_text):
    """ARM source emitting, per class, (offsetof, sizeof) of each member, then sizeof(class)."""
    parts, entries, counts = [PREAMBLE], [], {}
    for name in CLASSES:
        body, members = extract_struct(header_text, name)
        parts.append(body)
        counts[name] = members
        for m, _ in members:
            entries.append(f"offsetof({name}, {m}), sizeof((({name} *)0)->{m}),")
        entries.append(f"sizeof({name}),")
    consts = dict(re.findall(
        r"((?:SYS|CURRENT|SENSOR)_DATA_(?:MAGIC|VERSION))\s*=\s*(0x[0-9a-fA-F]+|\d+)", header_text))
    for name in CLASSES:
        for kind in ("MAGIC", "VERSION"):
            cname = f"{PREFIX[name]}_DATA_{kind}"
            if cname not in consts:
                fail(f"constant {cname} not found in the header")
            entries.append(f"{consts[cname]},")
    parts.append('extern "C" const unsigned long layout_probe[] = {\n' + "\n".join(entries) + "\n};")
    return "\n".join(parts), counts


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
        int(line.split()[1], 0)
        for line in asm.read_text().splitlines()
        if re.match(r"^\s*\.(word|long)\s", line)
    ]
    if not words:
        fail(f"{tag} probe emitted no data words - extraction is broken, not the layout")
    return words


def main():
    if not ARM_GXX.is_file():
        print(f"SKIP: ARM toolchain not found at {ARM_GXX}")
        return

    source, counts = build_probe(HEADER.read_text())
    for name in CLASSES:
        if len(counts[name]) != len(FROZEN_MEMBERS[name]):
            fail(f"{name}: member count changed ({len(counts[name])} now, {len(FROZEN_MEMBERS[name])} frozen)")
        for k, ((n, t), (fn, ft, _, _)) in enumerate(zip(counts[name], FROZEN_MEMBERS[name])):
            if t != ft or not (n == fn or (fn.startswith("reserved") and n.startswith("reserved"))):
                fail(f"{name}: member #{k} is `{t} {n}`, frozen `{ft} {fn}` "
                     "(live members keep name and type; only reserved* slots may be renamed to reserved*)")
    with tempfile.TemporaryDirectory() as tmp:
        words = data_words(source, Path(tmp), "current")

    pos = 0
    for name in CLASSES:
        frozen = FROZEN_MEMBERS[name]
        n = 2 * len(frozen) + 1
        got = words[pos : pos + n]
        pos += n
        want = [v for f in frozen for v in f[2:]] + [FROZEN_SIZE[name]]
        if got != want:
            gp = list(zip(got[0:-1:2], got[1:-1:2]))
            for k, (g, f) in enumerate(zip(gp, frozen)):
                if g != f[2:]:
                    fail(f"{name}: member {f[0]} (#{k}) (offset,size) is {g}, frozen {f[2:]}")
            fail(f"{name}: sizeof is {got[-1]}, frozen {FROZEN_SIZE[name]}")
        print(f"OK: {name} header + {len(frozen) - 1} data members, {FROZEN_SIZE[name]} bytes, every (name, type, offset, size) frozen (ARM cortex-m4)")
    if len(words) != pos + 6:
        fail("probe emitted an unexpected number of words - extraction is broken")
    for name, k in zip(CLASSES, range(pos, pos + 6, 2)):
        if tuple(words[k : k + 2]) != FROZEN_CONSTS[name]:
            fail(f"{name}: magic/version {tuple(hex(w) for w in words[k:k+2])} != frozen "
                 f"{tuple(hex(w) for w in FROZEN_CONSTS[name])}")
    print("OK: all three magic/version pairs unchanged")
    print("persisted_layout_preservation_test: layout frozen for all three records")


if __name__ == "__main__":
    main()
