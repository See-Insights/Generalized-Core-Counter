#!/usr/bin/env python3
"""Persisted-layout freeze guard for all three records (WO-2026-10-22-003, P7-3).

A deployed device reads /usr/sysStatus.dat, /usr/current.dat and /usr/sensor.dat
using only these sizes, offsets and magic/version pairs, and v45 must still read
them after a downgrade. The records are frozen: no member may be appended,
inserted, removed, resized or moved, and no version may change.

The comparison is positional, not by name: every data member of each class is
extracted from the header in order, an offsetof/sizeof table is compiled for the
real ARM target, and it is compared to the frozen (offset, size) list below.
Renaming a member (e.g. to a reserved slot) passes; anything that changes a
position fails.
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
FROZEN_MEMBERS = {
    "SysData": [
        (0, 16), (16, 1), (17, 1), (18, 1), (19, 1), (20, 1),
        (21, 1), (22, 39), (61, 1), (62, 1), (64, 8), (72, 8),
        (80, 8), (88, 2), (90, 1), (91, 1), (92, 2), (94, 1),
        (95, 1), (96, 2), (104, 8), (112, 8), (120, 8), (128, 1),
        (129, 1), (130, 1), (131, 1), (132, 2), (134, 2), (136, 2),
        (138, 1), (139, 1), (140, 4), (144, 2), (146, 1), (147, 64),
        (211, 1), (212, 4), (216, 1), (224, 8), (232, 1), (234, 2),
        (236, 1), (240, 4), (244, 4), (248, 1), (249, 1), (250, 1),
        (252, 4), (256, 4), (260, 4), (264, 4),
    ],
    "SensorData": [
        (0, 16), (16, 1), (20, 4), (24, 4), (28, 4), (32, 4),
    ],
    "CurrentData": [
        (0, 16), (16, 2), (18, 2), (20, 2), (22, 2), (24, 8),
        (32, 4), (36, 4), (40, 1), (48, 8), (56, 4), (60, 1),
        (62, 2), (64, 2), (66, 1), (68, 4), (72, 8), (80, 4),
    ],
}
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

MEMBER_RE = re.compile(r"^[\w:]+\s+(\w+)(?:\[[^\]]*\])?\s*;$")


def fail(msg):
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def extract_struct(header_text, name):
    """Return (body, member names in order) of `class <name> { ... };`."""
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
    members = [m.group(1) for line in code.splitlines()[1:]
               for m in [MEMBER_RE.match(line.strip())] if m]
    return body, members


def build_probe(header_text):
    """ARM source emitting, per class, (offsetof, sizeof) of each member, then sizeof(class)."""
    parts, entries, counts = [PREAMBLE], [], {}
    for name in CLASSES:
        body, members = extract_struct(header_text, name)
        parts.append(body)
        counts[name] = len(members)
        for m in members:
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
        if counts[name] != len(FROZEN_MEMBERS[name]):
            fail(f"{name}: member count changed ({counts[name]} now, {len(FROZEN_MEMBERS[name])} frozen)")
    with tempfile.TemporaryDirectory() as tmp:
        words = data_words(source, Path(tmp), "current")

    pos = 0
    for name in CLASSES:
        frozen = FROZEN_MEMBERS[name]
        n = 2 * len(frozen) + 1
        got = words[pos : pos + n]
        pos += n
        want = [v for pair in frozen for v in pair] + [FROZEN_SIZE[name]]
        if got != want:
            gp = list(zip(got[0:-1:2], got[1:-1:2]))
            for k, (g, f) in enumerate(zip(gp, frozen)):
                if g != f:
                    fail(f"{name}: member #{k} (offset,size) is {g}, frozen {f}")
            fail(f"{name}: sizeof is {got[-1]}, frozen {FROZEN_SIZE[name]}")
        print(f"OK: {name} header + {len(frozen) - 1} data members, {FROZEN_SIZE[name]} bytes, every (offset, size) frozen (ARM cortex-m4)")
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
