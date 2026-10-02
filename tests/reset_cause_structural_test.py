#!/usr/bin/env python3
"""
WO-2026-10-01-001 item C - after any firmware-issued reset, the next startup
status must name the code that issued it.

Evidence: PCKL1 sent `resetReason=140 resetReasonData=0` after an unexplained
hour of silence. Reason 140 is `System.reset()`, and six call sites in `src/`
can issue it; nothing recorded which one did. Device OS 6.4.1's
`System.reset(uint32_t data)` hands `data` to the next boot as
`System.resetReasonData()`, which the startup status already publishes.

This is a structural test. The behavior it guards (a value surviving a reset
into the next boot) lives in Device OS, so a host harness cannot execute it;
what can be pinned is the shape:

  1. Every `System.reset(` call in `src/` passes a cause code - a bare
     `System.reset()` fails.
  2. The codes are distinct (no two sites share one) and non-zero, because 0 is
     reserved to mean "the reset came from outside our src/ reset sites".
  3. Every code used is declared in the ResetCause enum, and the enum's own
     values are distinct and non-zero.
  4. The out-of-memory site in `loop()` resets directly with
     `RESET_CAUSE_OUT_OF_MEMORY` (7) - WO-2026-10-02-003 item B.
  5. The startup status payload still carries `resetReasonData`.

Comments are stripped before each check so prose mentioning `System.reset()`
cannot produce a false positive or a false negative.
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC = REPO_ROOT / "src"
RESET_CAUSE_HEADER = SRC / "ResetCause.h"
MAIN = SRC / "Generalized-Core-Counter.cpp"

EXPECTED_SITE_COUNT = 7

# WO-2026-10-02-003 item B: the out-of-memory reset must carry its own code.
OOM_CAUSE_NAME = "RESET_CAUSE_OUT_OF_MEMORY"
OOM_CAUSE_VALUE = 7


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def strip_comments(code: str) -> str:
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)
    code = re.sub(r"//[^\n]*", "", code)
    return code


def source_files():
    for path in sorted(SRC.rglob("*")):
        if path.suffix in (".cpp", ".h") and path.is_file():
            yield path


def main() -> None:
    if not RESET_CAUSE_HEADER.exists():
        fail(f"{RESET_CAUSE_HEADER.relative_to(REPO_ROOT)} is missing")

    # --- Enum: distinct, non-zero -------------------------------------------
    header = strip_comments(RESET_CAUSE_HEADER.read_text())
    enum_body = re.search(r"enum\s+ResetCause\s*:[^{]*\{(.*?)\}", header, re.DOTALL)
    if not enum_body:
        fail("ResetCause.h does not declare an `enum ResetCause`")
    declared = dict(
        (name, int(value))
        for name, value in re.findall(r"(RESET_CAUSE_\w+)\s*=\s*(\d+)", enum_body.group(1))
    )
    if not declared:
        fail("the ResetCause enum declares no explicitly-valued cause codes")
    zero_valued = [name for name, value in declared.items() if value == 0]
    if zero_valued:
        fail(
            f"reset cause code(s) {sorted(zero_valued)} are 0; 0 is reserved for "
            "resets issued outside src/"
        )
    seen = {}
    for name, value in declared.items():
        if value in seen:
            fail(f"reset cause codes {seen[value]} and {name} share the value {value}")
        seen[value] = name

    # --- Call sites: every one carries a distinct code -----------------------
    sites = []
    for path in source_files():
        code = strip_comments(path.read_text())
        for match in re.finditer(r"System\.reset\s*\(([^;)]*)\)", code):
            argument = match.group(1).strip()
            line = code[: match.start()].count("\n") + 1
            where = f"{path.relative_to(REPO_ROOT)}:{line}"
            if not argument:
                fail(
                    f"{where} calls System.reset() with no cause code; every firmware-issued "
                    "reset must name itself (WO-2026-10-01-001 item C)"
                )
            sites.append((where, argument))

    if len(sites) != EXPECTED_SITE_COUNT:
        fail(
            f"expected {EXPECTED_SITE_COUNT} System.reset() call sites in src/, found "
            f"{len(sites)}: {[s[0] for s in sites]}. Add the new site's code to the "
            "ResetCause enum and update this test."
        )

    used = {}
    for where, argument in sites:
        if argument not in declared:
            fail(f"{where} passes '{argument}', which is not a ResetCause enum value")
        if argument in used:
            fail(
                f"{where} reuses the cause code '{argument}' already used by {used[argument]}; "
                "codes must be distinct so a reset can be attributed"
            )
        used[argument] = where

    # --- The out-of-memory site carries code 7 -------------------------------
    if declared.get(OOM_CAUSE_NAME) != OOM_CAUSE_VALUE:
        fail(
            f"ResetCause.h must declare {OOM_CAUSE_NAME} = {OOM_CAUSE_VALUE} "
            f"(found {declared.get(OOM_CAUSE_NAME)!r})"
        )
    if OOM_CAUSE_NAME not in used:
        fail(
            f"no System.reset() site passes {OOM_CAUSE_NAME}; the out-of-memory "
            "handler's reset must name itself (WO-2026-10-02-003 item B)"
        )

    # --- The startup status still publishes the code -------------------------
    main_src = strip_comments(MAIN.read_text())
    if "System.resetReasonData()" not in main_src:
        fail("the startup path no longer reads System.resetReasonData()")
    if not re.search(r'\\"resetReasonData\\":', main_src):
        fail("the startup status payload no longer carries the resetReasonData field")

    print(
        f"Reset cause structural test passed ({len(sites)} call sites, distinct non-zero "
        "codes, resetReasonData still published)"
    )


if __name__ == "__main__":
    main()
