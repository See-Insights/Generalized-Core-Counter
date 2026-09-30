#!/usr/bin/env python3
"""
WO-2026-09-30-001 - the device never commits to night sleep while the daily
close is still due.

Evidence (Dev-14, 2026-09-29/30): a `connect-timeout` at 22:01:59 routed
straight to closed-hours hibernate, so no closing report ran and the count
never reset. The "is the close due" test existed only inside
`handleReportingState()`; the night-sleep commitment in
`handleSleepingState()` never asked it. Every route into SLEEPING_STATE that
does not pass through a report after the boundary reaches that one
commitment, so the guard belongs there.

This is a source-invariant check on the REAL checked-in file, not a mirror.
Comments are stripped before each check so prose cannot produce a false
positive or a false negative.

  1. `handleSleepingState()`'s `parkOpenness == Clock::Openness::Closed`
     branch calls `DailyBoundary::check(` and, when the close is due,
     `transitionTo(REPORTING_STATE, "close due before night sleep")` and
     returns.
  2. That guard comes BEFORE any night-sleep work - before
     `SensorManager::instance().onEnterSleep()` and before
     `secondsUntilNextOpen()` - so the device has not powered sensors down
     or computed a hibernate duration by the time it turns around.
  3. `State_Sleep.cpp` includes the `DailyBoundary` owner rather than
     re-deriving the due-test locally.
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
STATE_SLEEP = REPO_ROOT / "src" / "state" / "State_Sleep.cpp"

TRANSITION = 'transitionTo(REPORTING_STATE, "close due before night sleep")'


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def strip_comments(code: str) -> str:
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)
    code = re.sub(r"//[^\n]*", "", code)
    return code


def extract_function(text: str, signature_pattern: str, name: str) -> str:
    match = re.search(signature_pattern, text)
    if not match:
        fail(f"could not locate {name}() in {STATE_SLEEP.name}")
    depth = 0
    for index in range(match.end() - 1, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[match.start():index + 1]
    fail(f"could not find matching closing brace for {name}()")
    return ""  # unreachable


def extract_closed_branch(sleeping_fn: str) -> str:
    match = re.search(
        r"if\s*\(\s*parkOpenness\s*==\s*Clock::Openness::Closed\s*\)\s*\{", sleeping_fn
    )
    if not match:
        fail(
            "could not locate the night-sleep commitment branch "
            "(if (parkOpenness == Clock::Openness::Closed)) in handleSleepingState()"
        )
    depth = 0
    for index in range(match.end() - 1, len(sleeping_fn)):
        if sleeping_fn[index] == "{":
            depth += 1
        elif sleeping_fn[index] == "}":
            depth -= 1
            if depth == 0:
                return sleeping_fn[match.end():index]
    fail("could not find the closing brace of the night-sleep commitment branch")
    return ""  # unreachable


def main() -> None:
    if not STATE_SLEEP.is_file():
        fail(f"{STATE_SLEEP} does not exist")

    full_text = strip_comments(STATE_SLEEP.read_text())

    # --- Invariant 3: the owner is included, not reimplemented. ---
    if '#include "time/DailyBoundary.h"' not in STATE_SLEEP.read_text():
        fail(
            "State_Sleep.cpp must include time/DailyBoundary.h - the night-sleep "
            "commitment has to ask the same due-test the report asks, not its own copy"
        )
    if "localTodayAt" in full_text or "get_lastDailyCleanup()" in full_text:
        fail(
            "State_Sleep.cpp must not re-derive the daily close test locally - "
            "it belongs to DailyBoundary (WO-2026-09-30-001)"
        )

    sleeping_fn = extract_function(
        full_text, r"void\s+handleSleepingState\s*\(\s*\)\s*\{", "handleSleepingState"
    )
    closed_branch = extract_closed_branch(sleeping_fn)

    # --- Invariant 1: the Closed branch asks DailyBoundary and turns around. ---
    check_call = closed_branch.find("DailyBoundary::check(")
    if check_call == -1:
        fail(
            "the parkOpenness == Clock::Openness::Closed branch must call "
            "DailyBoundary::check(...) - without it the device hibernates for the "
            "night with the daily close still pending (WO-2026-09-30-001)"
        )
    transition = closed_branch.find(TRANSITION)
    if transition == -1:
        fail(
            "the Closed branch must transition to REPORTING_STATE with the reason "
            f'"close due before night sleep" - missing {TRANSITION}'
        )
    guard = re.search(
        r"if\s*\(\s*DailyBoundary::check\s*\([^;]*\)\s*\.\s*due\s*\)\s*\{\s*"
        + re.escape(TRANSITION)
        + r"\s*;\s*return\s*;\s*\}",
        closed_branch,
    )
    if not guard:
        fail(
            "the close-due turnaround must be a guarded early return: "
            "if (DailyBoundary::check(...).due) { transitionTo(REPORTING_STATE, "
            '"close due before night sleep"); return; }'
        )

    # --- Invariant 2: it happens before any night-sleep work. ---
    enter_sleep = closed_branch.find("SensorManager::instance().onEnterSleep()")
    if enter_sleep == -1:
        fail("could not locate SensorManager::instance().onEnterSleep() in the Closed branch")
    next_open = closed_branch.find("secondsUntilNextOpen()")
    if next_open == -1:
        fail("could not locate secondsUntilNextOpen() in the Closed branch")
    if not (guard.end() <= enter_sleep and guard.end() <= next_open):
        fail(
            "the close-due check must run BEFORE any night-sleep work - before "
            "onEnterSleep() powers the sensors down and before secondsUntilNextOpen() "
            "computes a hibernate duration"
        )

    print("OK: the Closed branch asks DailyBoundary::check() before committing to night sleep")
    print('OK: when the close is due it transitions to REPORTING_STATE ("close due before night sleep") and returns')
    print("OK: the guard precedes onEnterSleep() and secondsUntilNextOpen()")
    print("OK: State_Sleep.cpp uses the DailyBoundary owner rather than a local copy of the due-test")
    print("close_before_night_sleep_test: all invariants hold")


if __name__ == "__main__":
    main()
