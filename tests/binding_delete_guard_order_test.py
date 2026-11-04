#!/usr/bin/env python3
"""
WO-2026-11-04-002 - structural order: in setup() the Device OS binding-delete
guard precedes everything that reads Time or Wire, the boot-storm Time window,
and ab1805.withFOUT(WKP).setup(). Usage: binding_delete_guard_order_test.py [source.cpp]
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
src = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "src" / "Generalized-Core-Counter.cpp"


def strip(code):
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)
    return re.sub(r"//[^\n]*", "", code)


code = strip(src.read_text())
start = code.index("void setup() {")
body = code[start:]
body = body[: body.index("\nvoid loop()")] if "\nvoid loop()" in body else body


def pos(needle):
    i = body.find(needle)
    if i < 0:
        print(f"FAIL: setup() has no `{needle}`", file=sys.stderr)
        sys.exit(1)
    return i


guard = pos("ExternalTime.status().bound()")
for later in ("Time.isValid()", "Time.now()", "Clock::isTrusted()", "Wire.",
              "ab1805.withFOUT(WKP).setup()"):
    i = body.find(later)
    if i >= 0 and i < guard:
        print(f"FAIL: `{later}` runs before the binding-delete guard in setup()", file=sys.stderr)
        sys.exit(1)
pos("ab1805.withFOUT(WKP).setup()")
print("binding_delete_guard_order_test: guard precedes the Time reads and ab1805.setup()")
