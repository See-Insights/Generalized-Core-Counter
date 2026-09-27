#!/usr/bin/env python3
"""
Structural regression test for WO-2026-09-25-001, Stage 5 decision 10: the
restoration of `WITH_ACK` on every publish-queue publish, and the Idle
handoff that comes with it.

Background: until `eda6b7e` (v3.24, 2026-02-09) every queued publish was
sent with `WITH_ACK`, so PublishQueuePosix only deleted an event once the
cloud had confirmed it. That commit dropped the flag, leaving publishes
fire-and-forget: the queue deleted events the cloud had never
acknowledged, and reports were silently lost. Nothing in the tree
prevented that flag from being dropped again - this file does.

The second half of decision 10 hands the "wait for the queue" job to the
existing bounded gate in SLEEPING_STATE. With `WITH_ACK` restored, an
event that is never acknowledged keeps `getCanSleep()` false forever, so
the old unbounded `canSleepGate` term in handleIdleState() would keep the
device awake indefinitely. Sleep's gate waits 30-120 s, then raises alert
43 and disconnects, which is bounded; Idle's was not. Decision 10 removes
Idle's gate term and leaves Sleep's alone.

This is a source-invariant check on the REAL checked-in files, not a
mirror. Comments are stripped before each check so prose (including this
docstring, if pasted into the source) cannot produce a false positive or
a false negative.

  1. Every `PublishQueuePosix::instance().publish(` call under `src/`
     passes `WITH_ACK` in its flags argument. A mutation that removes
     `WITH_ACK` from any single site fails this invariant and names the
     file and line.
  2. At least MIN_PUBLISH_SITES such calls exist, so invariant 1 cannot
     be satisfied vacuously by deleting the publishes.
  3. handleIdleState() enters the low-power sleep path on
     `if (!updatesPending) {` alone, and the `canSleepGate` local that
     used to AND the publish queue into that decision is gone from
     State_Idle.cpp. (The separate Idle connectivity ceiling later in the
     same function still consults getCanSleep(); that is outside decision
     10, so this test does not assert anything about it.)
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_ROOT = REPO_ROOT / "src"
IDLE_SOURCE = SRC_ROOT / "state" / "State_Idle.cpp"

# The six known queue publishes at the time of decision 10 (report, status,
# watchdog, hibernate_wake, publishDiagnosticSafe(), pdiag). New publish
# sites are welcome - they simply must also carry WITH_ACK.
MIN_PUBLISH_SITES = 6

PUBLISH_CALL_PATTERN = re.compile(r"PublishQueuePosix::instance\(\)\.publish\s*\(")
SLEEP_ENTRY_PATTERN = re.compile(r"if\s*\(\s*!\s*updatesPending\s*\)\s*\{")
RETIRED_GATE_PATTERN = re.compile(r"canSleepGate")
SOURCE_SUFFIXES = (".cpp", ".h", ".hpp", ".ino", ".c")


def strip_comments(code: str) -> str:
    """Remove // line comments and /* */ block comments so prose cannot
    produce a false positive or a false negative. Newlines inside block
    comments are preserved so reported line numbers stay accurate."""
    code = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), code, flags=re.DOTALL)
    code = re.sub(r"//[^\n]*", "", code)
    return code


def call_arguments(code: str, open_paren_index: int) -> str:
    """Return the text between the call's parentheses, tracking nesting so
    nested calls in the argument list do not end the scan early."""
    depth = 0
    for i in range(open_paren_index, len(code)):
        if code[i] == "(":
            depth += 1
        elif code[i] == ")":
            depth -= 1
            if depth == 0:
                return code[open_paren_index + 1:i]
    return code[open_paren_index + 1:]


def fail(msg):
    print(f"FAIL: {msg}")
    sys.exit(1)


def main():
    if not SRC_ROOT.is_dir():
        fail(f"{SRC_ROOT} does not exist")
    if not IDLE_SOURCE.is_file():
        fail(f"{IDLE_SOURCE} does not exist")

    # --- Invariants 1 and 2: every queue publish asks for an ack. ---
    sites = 0
    for path in sorted(SRC_ROOT.rglob("*")):
        if not path.is_file() or path.suffix not in SOURCE_SUFFIXES:
            continue
        code_only = strip_comments(path.read_text(errors="replace"))
        for match in PUBLISH_CALL_PATTERN.finditer(code_only):
            sites += 1
            args = call_arguments(code_only, match.end() - 1)
            if "WITH_ACK" not in args:
                line = code_only.count("\n", 0, match.start()) + 1
                fail(
                    f"{path.relative_to(REPO_ROOT)}:{line} publishes to the "
                    "queue without WITH_ACK - PublishQueuePosix would delete "
                    "the event without the cloud ever confirming it, the "
                    "eda6b7e report-loss regression (WO-2026-09-25-001 "
                    f"decision 10). Flags seen: {args.strip()!r}"
                )

    if sites < MIN_PUBLISH_SITES:
        fail(
            f"found {sites} PublishQueuePosix::instance().publish( call(s) "
            f"under src/, expected at least {MIN_PUBLISH_SITES} - the "
            "WITH_ACK invariant above must not be satisfied vacuously by "
            "publishes having been removed or renamed"
        )

    # --- Invariant 3: Idle hands the queue wait to Sleep's bounded gate. ---
    idle_code = strip_comments(IDLE_SOURCE.read_text())

    if not SLEEP_ENTRY_PATTERN.search(idle_code):
        fail(
            f"{IDLE_SOURCE.relative_to(REPO_ROOT)} has no "
            "`if (!updatesPending) {` low-power sleep entry - decision 10 "
            "requires the sleep decision to depend on updatesPending alone, "
            "with the publish-queue wait left to SLEEPING_STATE's bounded "
            "gate (30-120 s, then alert 43 and disconnect)"
        )

    if RETIRED_GATE_PATTERN.search(idle_code):
        fail(
            f"{IDLE_SOURCE.relative_to(REPO_ROOT)} still references "
            "`canSleepGate` in code - the unbounded publish-queue gate on "
            "the low-power sleep entry is back. With WITH_ACK restored, a "
            "never-acknowledged event would hold that gate closed forever "
            "and keep the device awake (WO-2026-09-25-001 decision 10)"
        )

    print(f"OK: all {sites} PublishQueuePosix::instance().publish( call(s) under src/ pass WITH_ACK")
    print("OK: handleIdleState() enters low-power sleep on !updatesPending alone; canSleepGate is gone")
    print("publish_with_ack_structural_test: all invariants hold")


if __name__ == "__main__":
    main()
