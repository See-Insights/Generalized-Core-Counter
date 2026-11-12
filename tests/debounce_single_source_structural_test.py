#!/usr/bin/env python3
"""WO-2026-11-11-003 Amendment 2: one source and one unit for the occupancy debounce.

In src/, outside Config.cpp/Config.h/MyPersistentData.cpp/ConfigApply.cpp:
 1. no read of get_sensorSetting1() feeds a debounce (the only other readers are
    the Step 0 non-consumers: a hash, a dead comparison, a log, the accessor itself);
 2. no "/ 1000" is applied to a value derived from the helper or the wrapper, except
    as a log argument, at the epoch-seconds cap and the untrusted-piece credit (a
    millisecond sum converted to seconds once) in State_Common.h, or where the sleep
    plan assigns wakeInSeconds;
 3. no literal 60 or 60000 appears as a debounce fallback.
SRC_ROOT points at a mutant copy of src/."""
import os
import re
import sys

root = os.environ.get("SRC_ROOT") or os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src")
OWNERS = {"Config.cpp", "Config.h", "MyPersistentData.cpp", "ConfigApply.cpp"}
SETTING1_READERS = {"SystemConfig.h", "MyPersistentData.h", "DeviceStatusPublisher.cpp", "Cloud.cpp"}
SOURCE = re.compile(r"occupancyDebounceMs(?:ForRuntime)?\s*\(\s*\)")


def strip(text):
    def blank(m):
        return re.sub(r"[^\n]", " ", m.group(0))
    text = re.sub(r"/\*.*?\*/", blank, text, flags=re.S)
    return re.sub(r"//[^\n]*", blank, text)


def statements(text):
    """(line, statement) pairs split on ; { }."""
    out, start, line, cur = [], 0, 1, 1
    for i, ch in enumerate(text):
        if ch == "\n":
            cur += 1
        if ch in ";{}":
            seg = text[start:i + 1]
            lead = len(seg) - len(seg.lstrip())
            out.append((line + seg[:lead].count("\n"), seg.strip()))
            start, line = i + 1, cur
    return out


def log_statement(stmt):
    return stmt.startswith(("Log.", "logOccupiedEvent(", "logUnoccupiedEvent("))


problems = []
for dirpath, _, files in os.walk(root):
    for name in sorted(files):
        if not name.endswith((".cpp", ".h")) or name in OWNERS:
            continue
        path = os.path.join(dirpath, name)
        rel = os.path.relpath(path, root)
        stmts = statements(strip(open(path, errors="replace").read()))
        tainted = set()
        for _, st in stmts:
            m = re.search(r"\b(\w+)\s*=[^=].*" + SOURCE.pattern, st, re.S)
            if m:
                tainted.add(m.group(1))
        for line, st in stmts:
            if "get_sensorSetting1" in st and name not in SETTING1_READERS:
                problems.append(f"{rel}:{line}: reads get_sensorSetting1() outside the helper")
            if re.search(r"\bdebounce\w*|\bsetting1\w*", st, re.I) and re.search(r"(?<![\w.])(60|60000)[uUlL]*(?![\w.])", st):
                problems.append(f"{rel}:{line}: literal 60/60000 near a debounce")
            divides = re.search(r"/\s*1000", st)
            if divides and not log_statement(st):
                derived = SOURCE.search(st) or any(re.search(r"\b%s\b" % re.escape(v), st) for v in tainted)
                if derived and "wakeInSeconds" not in st and not (name == "State_Common.h" and ("(time_t)" in st or "rawSessionSeconds =" in st)):
                    problems.append(f"{rel}:{line}: divides the debounce by 1000 outside a log argument")

if problems:
    print("\n".join(problems))
    print("FAILED: the occupancy debounce has a second source or unit")
    sys.exit(1)
print("PASS: every debounce consumer uses the helper, in milliseconds")
