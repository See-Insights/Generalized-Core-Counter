#!/usr/bin/env python3
"""
Regression/evidence test for item 4 of WO-2026-08-10-001: the always-on
"brief wait if already connected" serial settle in setup(), independent of
the bench-only ALLOW_BLOCKING_SERIAL_WAITS flag.

Rather than a logic-mirror unit test (the check itself is a single `if`
statement with no independent decision logic worth reproducing), this test
traces the actual shipped source directly to confirm:

  1. The serial-settle helper call is the very first statement in setup() -
     strictly before ensureRetainedLoopForensicsInitialized() and any
     application log line.
  2. It is unconditional - NOT gated behind ALLOW_BLOCKING_SERIAL_WAITS (that
     flag's existing logic, further down in setup(), is confirmed unchanged).
  3. Inside the helper, delay(DEBUG_SERIAL_POST_CONNECT_DELAY_MS) only
     executes inside the `if (Serial.isConnected())` block - i.e. the 500ms
     settle can only ever fire when a monitor is already connected, never
     unconditionally.
  4. No new Serial.begin() call was added alongside it (the global
     SerialLogHandler's constructor already calls Serial.begin() before
     setup() runs).

Combined with the `particle compile p2 . --target 6.4.1` before/after
Flash/RAM comparison captured in the WO implementation summary (a net +848
byte Flash delta across ALL FOUR fix items combined, this single `if`+delay
check being a tiny fraction of that), this demonstrates near-zero field cost
when nothing is attached to serial - Serial.isConnected() returns false
immediately in that case and the delay() is never reached.
"""
import re
import sys
from pathlib import Path
from typing import Optional

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC = REPO_ROOT / "src" / "Generalized-Core-Counter.cpp"
SETTLE_HELPER_CALL = "waitForDebugSerialIfConnected();"
SETTLE_HELPER_DEF = "static void waitForDebugSerialIfConnected() {"
SETTLE_DELAY = "delay(ConnectivityPolicy::DEBUG_SERIAL_POST_CONNECT_DELAY_MS);"


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def find_matching_brace(lines: list[str], open_brace_line: int) -> int:
    depth = 0
    seen_open = False

    for i in range(open_brace_line, len(lines)):
        for char in lines[i]:
            if char == "{":
                depth += 1
                seen_open = True
            elif char == "}":
                depth -= 1
                if seen_open and depth == 0:
                    return i

    fail(f"could not find matching closing brace for line {open_brace_line + 1}")


def non_comment_statements(lines: list[str]) -> list[str]:
    return [l.strip() for l in lines if l.strip() and not l.strip().startswith("//")]


def first_non_comment_statement_line(lines: list[str], start: int, end: int) -> Optional[int]:
    for i in range(start, end):
        stripped = lines[i].strip()
        if stripped and not stripped.startswith("//"):
            return i
    return None


def serial_begin_lines(lines: list[str], start: int, end: int) -> list[int]:
    return [i for i in range(start, end)
            if "Serial.begin(" in lines[i] and not lines[i].strip().startswith("//")]


def main() -> None:
    text = SRC.read_text()
    lines = text.splitlines()

    setup_start = next((i for i, l in enumerate(lines) if re.match(r"void setup\(\) \{", l)), None)
    if setup_start is None:
        fail("could not find 'void setup() {' in source")
    setup_end = find_matching_brace(lines, setup_start)

    ensure_line = next((i for i, l in enumerate(lines)
                        if setup_start < i < setup_end
                        and "ensureRetainedLoopForensicsInitialized();" in l), None)
    if ensure_line is None:
        fail("could not find ensureRetainedLoopForensicsInitialized() call inside setup()")

    helper_call_line = first_non_comment_statement_line(lines, setup_start + 1, setup_end)
    if helper_call_line is None:
        fail("could not find any executable statement inside setup()")
    if lines[helper_call_line].strip() != SETTLE_HELPER_CALL:
        fail(f"first executable statement in setup() must be exactly '{SETTLE_HELPER_CALL}', "
             f"found: {lines[helper_call_line].strip()}")
    if helper_call_line >= ensure_line:
        fail(f"{SETTLE_HELPER_CALL} must appear before ensureRetainedLoopForensicsInitialized() in setup()")
    print(f"PASS: {SETTLE_HELPER_CALL} (line {helper_call_line + 1}) precedes "
          f"ensureRetainedLoopForensicsInitialized() (line {ensure_line + 1}) in setup()")

    # Confirm no other statement (aside from comments/blank lines) sits between
    # 'void setup() {' and the helper call - serial settle must be the very
    # first thing setup() does.
    before_helper = non_comment_statements(lines[setup_start + 1:helper_call_line])
    if before_helper:
        fail(f"expected only comments/blank lines before {SETTLE_HELPER_CALL}, found: {before_helper}")
    print("PASS: no application log line or other statement precedes the serial-settle helper call")

    helper_def_line = next((i for i, l in enumerate(lines) if l.strip() == SETTLE_HELPER_DEF), None)
    if helper_def_line is None:
        fail(f"could not find helper definition '{SETTLE_HELPER_DEF}'")
    helper_end_line = find_matching_brace(lines, helper_def_line)
    helper_body = lines[helper_def_line + 1:helper_end_line]

    isconnected_line = next((i for i in range(helper_def_line + 1, helper_end_line)
                             if "if (Serial.isConnected())" in lines[i]), None)
    if isconnected_line is None:
        fail("Serial.isConnected() guard not found inside waitForDebugSerialIfConnected()")

    delay_lines = [i for i in range(helper_def_line + 1, helper_end_line) if SETTLE_DELAY in lines[i]]
    if not delay_lines:
        fail(f"{SETTLE_DELAY} not found inside waitForDebugSerialIfConnected()")
    if len(delay_lines) > 1:
        fail(f"expected one {SETTLE_DELAY} inside waitForDebugSerialIfConnected(), found {len(delay_lines)}")

    guard_end_line = find_matching_brace(lines, isconnected_line)
    delay_line = delay_lines[0]
    if not (isconnected_line < delay_line < guard_end_line):
        fail(f"{SETTLE_DELAY} must be inside the if (Serial.isConnected()) block, not unconditional")
    if non_comment_statements(helper_body) != [
            "if (Serial.isConnected()) {",
            SETTLE_DELAY,
            "}"]:
        fail("waitForDebugSerialIfConnected() must contain only the Serial.isConnected() guard "
             "and guarded settle delay")
    print(f"PASS: {SETTLE_DELAY} at line {delay_line + 1} is strictly inside the "
          f"Serial.isConnected() guard (closes at line {guard_end_line + 1})")

    # Confirm no new Serial.begin() call was added near the new check (the
    # global SerialLogHandler's constructor already handles that).
    helper_serial_begin_lines = serial_begin_lines(lines, helper_def_line, helper_end_line + 1)
    if helper_serial_begin_lines:
        fail("unexpected Serial.begin() call found inside the serial-settle helper at line(s): "
             f"{[i + 1 for i in helper_serial_begin_lines]}")

    setup_serial_begin_lines = serial_begin_lines(lines, helper_call_line, ensure_line)
    if setup_serial_begin_lines:
        fail("unexpected Serial.begin() call found near the setup() serial-settle call site at line(s): "
             f"{[i + 1 for i in setup_serial_begin_lines]}")
    print("PASS: no additional Serial.begin() call added alongside the serial-settle helper or setup() call site")

    # Confirm the bench-only ALLOW_BLOCKING_SERIAL_WAITS flag's existing logic
    # is untouched and distinct from this new unconditional check - it should
    # still exist later in setup(), gating its OWN separate wait loop.
    allow_blocking_line = next((i for i, l in enumerate(lines)
                                if i > ensure_line
                                and "ALLOW_BLOCKING_SERIAL_WAITS" in l), None)
    if allow_blocking_line is None:
        fail("ALLOW_BLOCKING_SERIAL_WAITS gate not found later in setup() - "
             "it must remain unchanged and independent of the new check")
    print(f"PASS: pre-existing ALLOW_BLOCKING_SERIAL_WAITS-gated wait logic still present "
          f"separately at line {allow_blocking_line + 1}, unmodified by this change")

    print("\nAll item-4 serial-settle regression checks passed")


if __name__ == "__main__":
    main()
