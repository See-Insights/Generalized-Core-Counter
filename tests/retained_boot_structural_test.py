#!/usr/bin/env python3
"""
WO-2026-10-17-001 (Step 6 P-0) - structural pins for the retained boot state.

  1. persist/RetainedBoot.h declares one struct whose members are `magic`,
     `version`, then exactly the nine former retained variables.
  2. main defines one `retained RetainedBoot` and binds every old name to a
     member; no old-style `retained <type> <name>` definition remains.
  3. `validateRetainedBoot(retainedBoot);` is the first statement of setup(),
     so it precedes every read of any member, including the reads inside
     ensureRetainedLoopForensicsInitialized().
  4. No other source file touches the struct or any of the nine names.
  5. The boot-storm lines mirrored by tests/retained_boot_test.cpp are intact.
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC = REPO_ROOT / "src"
MAIN = SRC / "Generalized-Core-Counter.cpp"
HEADER = SRC / "persist" / "RetainedBoot.h"

NINE = [
    "bootStormCount", "bootInProgress", "bootStormWindowStart", "bootStormLastResetReason",
    "bootStormLastResetReasonData", "bootStormTripCount", "bootStormAlertPending",
    "appBreadcrumb", "appBreadcrumbMs",
]


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def strip_comments(code: str) -> str:
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)
    return re.sub(r"//[^\n]*", "", code)


def function_body(code: str, signature: str) -> str:
    start = code.find(signature)
    if start < 0:
        fail(f"{signature} not found")
    i = code.index("{", start)
    depth = 0
    for j in range(i, len(code)):
        depth += {"{": 1, "}": -1}.get(code[j], 0)
        if depth == 0:
            return code[i + 1:j]
    fail(f"unbalanced braces in {signature}")


def main() -> None:
    header = strip_comments(HEADER.read_text())
    struct = re.search(r"struct\s+RetainedBoot\s*\{(.*?)\};", header, re.DOTALL)
    if not struct:
        fail("RetainedBoot struct not found")
    members = []
    for decl in struct.group(1).split(";"):
        decl = decl.strip()
        if decl:
            members += [n.strip() for n in decl.split(None, 1)[1].split(",")]
    if members[:2] != ["magic", "version"] or sorted(members[2:]) != sorted(NINE):
        fail(f"struct members must be magic, version, then the nine variables; got {members}")
    if not re.search(r"uint32_t\s+magic;\s*uint8_t\s+version;", struct.group(1)):
        fail("magic must be uint32_t and version uint8_t")

    main_code = strip_comments(MAIN.read_text())
    if len(re.findall(r"\bretained\s+RetainedBoot\s+retainedBoot\b", main_code)) != 1:
        fail("main must define exactly one `retained RetainedBoot retainedBoot`")
    for name in NINE:
        if re.search(rf"\bretained\s+[\w:]+\s+{name}\b", main_code):
            fail(f"{name} is still defined as a separate retained variable")
        if not re.search(rf"&\s*{name}\s*=\s*retainedBoot\.{name}\b", main_code):
            fail(f"{name} is not bound to retainedBoot.{name}")

    setup = function_body(main_code, "void setup()")
    first = setup.strip().split(";")[0].strip()
    if first != "validateRetainedBoot(retainedBoot)":
        fail(f"first statement of setup() is `{first}`, not the retained-boot check")
    check_at = setup.index("validateRetainedBoot(retainedBoot)")
    reads = NINE + ["retainedBoot", "ensureRetainedLoopForensicsInitialized", "setAppBreadcrumb"]
    for name in reads:
        m = re.search(rf"\b{name}\b", setup)
        if m and m.start() < check_at:
            fail(f"{name} is used in setup() before the retained-boot check")
    if not re.search(r"\bensureRetainedLoopForensicsInitialized\s*\(", setup):
        fail("setup() no longer calls ensureRetainedLoopForensicsInitialized()")

    for path in sorted(SRC.rglob("*")):
        if path.suffix in (".cpp", ".h") and path.is_file() and path not in (MAIN, HEADER):
            code = strip_comments(path.read_text())
            for name in NINE + ["retainedBoot", "RetainedBoot"]:
                if re.search(rf"\b{name}\b", code):
                    fail(f"{path.relative_to(REPO_ROOT)} uses {name}")

    # Lines mirrored by tests/retained_boot_test.cpp (bootStormTrips).
    for line in (
        "const bool previousBootFailedEarly = bootInProgress;",
        "if (previousBootFailedEarly && qualifiesForStormCount) {\n    if (bootStormCount < 255) {",
        "if (bootStormCount >= 6 && previousBootFailedEarly && qualifiesForStormCount) {",
        "bootStormCount = (previousBootFailedEarly && qualifiesForStormCount) ? 1 : 0;",
        "case RESET_REASON_NONE:", "case RESET_REASON_PANIC:", "case RESET_REASON_WATCHDOG:", "case RESET_REASON_UNKNOWN:",
    ):
        if setup.count(line) < 1 or (line.startswith("if") and setup.count(line) != 1):
            fail(f"boot-storm line changed (update the mirror too): {line!r}")

    print("PASS retained_boot_structural_test")


if __name__ == "__main__":
    main()
