#!/usr/bin/env python3
"""
WO-2026-11-09-001 - no preprocessor guard on a Device OS enumerator.

`#ifdef RESET_REASON_PANIC` is false when the name is an enumerator (Device OS
declares the reset reasons in an enum, not as macros), so the guarded code is
compiled out. This parses the REAL system_defs.h for every enumerator name and
fails on any #ifdef / #ifndef / #if / #elif in src/ (not lib/) that names one.
Inside the boot-storm qualifying switch in setup() any RESET_REASON_* guard is
also rejected, declared or not: RESET_REASON_USER_APPLICATION is not in Device OS.
(Two other guards on that undeclared name remain elsewhere in the main file;
they are not enumerators and are out of scope here - see the WO-2026-11-09-001
report.)

Backslash-continued directives are joined into one logical line (reported at
its starting line) before matching.

Documented limits (a guardrail, not a parser; F3 of the Stage 7 round 1 verdict):
- a directive split in the middle of a token by a backslash-newline
  (`#if defined(RESET_REASON_PAN\` then `IC)`, or `#i\` then `f defined(...)`) is not
  recognised: continued lines are joined with a space, where the compiler joins them
  with nothing (Stage 7 round 2, finding 2; deliberately constructed, so documented);
- macro aliases are not followed (`#define ALIAS RESET_REASON_PANIC`, `#if ALIAS`);
- an enumerator name inside a quoted directive argument is a false positive;
- only enumerators declared in Device OS system_defs.h are covered, not those of
  other HAL or system headers;
- brace counting and the storm-switch span check are not string-aware (comments
  are blanked, string and character literals are not).

DEVICE_OS_ROOT selects the header set (default: the toolchain copy of the version in project.properties; the test fails if the header tree is any other version).
"""
import os
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC = REPO_ROOT / "src"
DIRECTIVE = re.compile(r"^\s*#\s*(ifdef|ifndef|if|elif)\b(.*)$")


def blank_comments(text):
    return re.sub(r"/\*.*?\*/|//[^\n]*", lambda m: re.sub(r"[^\n]", " ", m.group(0)), text, flags=re.S)


def enumerators(defs_text):
    names = set()
    for body in re.findall(r"\benum\b[^{;]*\{(.*?)\}", blank_comments(defs_text), flags=re.S):
        body = re.sub(r"^\s*#.*$", "", body, flags=re.M)
        names.update(re.findall(r"(?:^|,)\s*([A-Za-z_]\w*)\s*(?:=[^,]*)?", body))
    return names


def storm_switch_lines(text):
    start = text.find("bool qualifiesForStormCount")
    if start < 0:
        return range(0)
    depth, i = 0, text.index("{", text.index("switch", start))
    for j in range(i, len(text)):
        depth += {"{": 1, "}": -1}.get(text[j], 0)
        if depth == 0:
            return range(text.count("\n", 0, start) + 1, text.count("\n", 0, j) + 2)
    return range(0)


def guarded_hits(path, names):
    hits = []
    text = blank_comments(path.read_text(errors="replace"))
    storm = storm_switch_lines(text)  # uses the original physical line numbers
    lines = text.splitlines()
    logical, idx = [], 0
    while idx < len(lines):
        number, line = idx + 1, lines[idx]
        while line.rstrip().endswith("\\") and idx + 1 < len(lines):
            idx += 1
            line = line.rstrip()[:-1] + " " + lines[idx]
        logical.append((number, line, idx + 1))
        idx += 1
    for number, line, last in logical:
        m = DIRECTIVE.match(line)
        if not m:
            continue
        for word in re.findall(r"\w+", m.group(2)):
            if word in names or (any(n in storm for n in range(number, last + 1)) and word.startswith("RESET_REASON_")):
                hits.append((number, line.strip()))
                break
    return hits


def main():
    ship = next((l.split("=", 1)[1].strip() for l in (REPO_ROOT / "project.properties").read_text().splitlines()
                 if l.startswith("deviceOS=")), "")  # the Device OS version we ship
    root = Path(os.environ.get("DEVICE_OS_ROOT", str(Path.home() / ".particle/toolchains/deviceOS" / ship)))
    ver_h = root / "system" / "inc" / "system_version.h"
    have = re.search(r"^#define SYSTEM_VERSION SYSTEM_VERSION_v(\w+)", ver_h.read_text(), flags=re.M) if ver_h.is_file() else None
    if not ship or not have or have.group(1) != ship.replace(".", ""):
        print(f"FAIL: Device OS headers under {root} are v{have.group(1) if have else '?'} but project.properties ships {ship}", file=sys.stderr)
        return 1
    defs = root / "services" / "inc" / "system_defs.h"
    if not defs.is_file():
        print(f"FAIL: missing {defs}", file=sys.stderr)
        return 1
    names = enumerators(defs.read_text())
    if not {"RESET_REASON_PANIC", "RESET_REASON_UNKNOWN", "RESET_REASON_WATCHDOG"} <= names:
        print("FAIL: enumerator parse of system_defs.h found no reset reasons", file=sys.stderr)
        return 1

    failures = []
    for path in sorted(SRC.rglob("*")):
        if path.suffix in {".cpp", ".h", ".c", ".hpp", ".ino"}:
            for number, line in guarded_hits(path, names):
                failures.append(f"{path.relative_to(REPO_ROOT)}:{number}: {line}")
    if failures:
        print("FAIL: preprocessor guard on a Device OS enumerator (always false):", file=sys.stderr)
        for f in failures:
            print("  " + f, file=sys.stderr)
        return 1
    print(f"PASS reset_reason_guard_structural_test ({len(names)} enumerators)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
