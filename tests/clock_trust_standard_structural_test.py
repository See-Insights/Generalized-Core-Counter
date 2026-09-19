#!/usr/bin/env python3
"""
WO-2026-09-19 Step 3b - structural regression test for the trust-standard
conversion: every decision site that used to gate on Clock::isTimeValid()
(an epoch merely EXISTS) now gates on Clock::isTrusted() or Clock::openness()
(this epoch can be TRUSTED), with exactly two named exceptions that are
solving a different problem and must NOT be converted.

Background: Step 3a introduced Clock::isTimeValid() as a semantics-
preserving wrapper around Time.isValid() and re-pointed ~15 raw call sites
at it - deliberately a no-op move, proven by its own structural test. Step
3b is the behaviour change 3a's seam exists for: Time.isValid() is seeded
true by ab1805.setup() from whatever the RTC holds, including a clock that
is hours wrong, so a decision resting on it rests on "an epoch exists," not
"this epoch can be trusted." This test enumerates every real (non-comment)
Clock::isTimeValid() occurrence left in the tree and requires each one to be
on the two-line allowlist below - anything else fails, whether it is a
decision site that was never converted or one that got reverted later.

This is a source-invariant check on the REAL checked-in files, not a mirror.
Comments are stripped before each check so prose (including this file's own
history, if pasted into source) cannot produce a false positive or a false
negative.

  1. Every remaining Clock::isTimeValid() call in src/ is on the two-line
     allowlist (the setup() CONNECTING_STATE gate and
     connectivityFailsafeSupervisor()'s early-return) - both deliberately
     NOT converted, for reasons documented in-place. Anything else means a
     decision site was missed, or a converted one reverted.
  2. Both allowlisted sites are still actually present (positive control -
     an allowlist that no longer matches anything would pass invariant 1
     vacuously).
  3. Each of the fourteen converted decision sites this step touched calls
     Clock::isTrusted() or Clock::openness() at its specific, expected
     location - not merely somewhere in the file.
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_ROOT = REPO_ROOT / "src"
APP_SRC = SRC_ROOT / "Generalized-Core-Counter.cpp"
STATE_COMMON = SRC_ROOT / "state" / "State_Common.h"
STATE_ERROR = SRC_ROOT / "state" / "State_Error.cpp"
STATE_IDLE = SRC_ROOT / "state" / "State_Idle.cpp"
STATE_REPORT = SRC_ROOT / "state" / "State_Report.cpp"
STATE_SLEEP = SRC_ROOT / "state" / "State_Sleep.cpp"

# The only two Clock::isTimeValid() call sites this step leaves unconverted,
# matched as exact (whitespace-trimmed) source lines after comment-stripping.
ALLOWED_ISTIMEVALID_LINES = {
    "if (!Clock::isTimeValid() || neverConfirmedSyncEver) {",
    'transitionTo(CONNECTING_STATE, !Clock::isTimeValid() ? "time invalid" : "no confirmed time sync ever (Finding 3)");',
    "if (!Clock::isTimeValid()) {",
}

# (file, expected substring, description) - each of the fourteen converted
# decision sites, checked at its specific location rather than as a
# file-wide count, since several files have more than one conversion.
CONVERTED_SITES = [
    (APP_SRC, "if (Clock::isTrusted()) {", "setup() boot-storm window (Group A)"),
    (STATE_COMMON, "const bool timeValid = Clock::isTrusted();", "closeOccupancySessionSafely (Group A)"),
    (STATE_ERROR, "if (!Clock::isTrusted()) {", "alert 40 corrective action (Group A)"),
    (STATE_REPORT, "if (Clock::isTrusted()) {", "daily-cleanup day-boundary gate (Group A)"),
    (STATE_IDLE, "if (Clock::isTrusted()) {", "MEASUREMENT-mode scheduled sampling (Group A)"),
    (STATE_SLEEP, "if (Clock::isTrusted() && intervalSec > 0) {", "reporting-boundary alignment (Group A)"),
    (STATE_IDLE, "const Clock::Openness parkOpenness = Clock::openness();", "CONNECTED-mode park-hours policy (Group B)"),
    (STATE_IDLE, "if (Clock::openness() != Clock::Openness::Closed) {", "scheduled reporting (Group B)"),
    (STATE_IDLE, "if (Clock::openness() == Clock::Openness::Open && sysStatus.get_connectionMode() == CONNECTED) {", "dead-code never-auto-sleep check (Group B)"),
    (STATE_IDLE, "const bool openHoursKeepAwakeValid = (Clock::openness() == Clock::Openness::Open);", "idle-ceiling exemption (Group B)"),
    (STATE_SLEEP, "if (sysStatus.get_connectionMode() == CONNECTED && Clock::openness() != Clock::Openness::Closed) {", "sleep-abort-if-open (Group B)"),
    (STATE_SLEEP, "const Clock::Openness parkOpenness = Clock::openness();", "night-sleep-vs-nap core decision (Group B)"),
    (STATE_SLEEP, "if (pirWake && Clock::openness() != Clock::Openness::Closed) {", "PIR-wake opportunistic reporting (Group B)"),
    (STATE_REPORT, "if (Clock::openness() == Clock::Openness::Open && !session.suppressAlert40ThisSession) {", "long-term webhook supervision (Group B)"),
]


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def strip_comments(code: str) -> str:
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)
    code = re.sub(r"//[^\n]*", "", code)
    return code


def main() -> None:
    texts = {}
    for path in {APP_SRC, STATE_COMMON, STATE_ERROR, STATE_IDLE, STATE_REPORT, STATE_SLEEP}:
        if not path.is_file():
            fail(f"{path} does not exist")
        texts[path] = strip_comments(path.read_text())

    # --- Invariant 1: every remaining Clock::isTimeValid() call is allowlisted. ---
    offenders = []
    for path, text in texts.items():
        for line in text.splitlines():
            stripped = line.strip()
            if "Clock::isTimeValid()" in stripped and stripped not in ALLOWED_ISTIMEVALID_LINES:
                offenders.append(f"{path.relative_to(REPO_ROOT)}: {stripped}")
    if offenders:
        fail(
            "Clock::isTimeValid() found outside the two-line allowlist - a "
            "decision site was either missed by Step 3b's conversion or has "
            "reverted: " + " | ".join(offenders)
        )

    # --- Invariant 2 (positive control): the allowlisted lines are still present. ---
    all_text = "\n".join(texts[APP_SRC].splitlines())
    present_lines = {line.strip() for line in all_text.splitlines()}
    missing = ALLOWED_ISTIMEVALID_LINES - present_lines
    if missing:
        fail(
            f"{APP_SRC.name} is missing expected allowlisted Clock::isTimeValid() "
            f"line(s): {missing} - update this test if these were deliberately "
            "rephrased, or investigate if they were accidentally removed"
        )

    # --- Invariant 3: each converted site is present at its specific location. ---
    for path, expected_substring, description in CONVERTED_SITES:
        if expected_substring not in texts[path]:
            fail(
                f"{path.relative_to(REPO_ROOT)} is missing the expected "
                f"converted decision site ({description}): {expected_substring!r}"
            )

    print("OK: every remaining Clock::isTimeValid() call is on the two-line allowlist")
    print("OK: both allowlisted sites are still present")
    print(f"OK: all {len(CONVERTED_SITES)} converted decision sites are present at their expected locations")
    print("clock_trust_standard_structural_test: all invariants hold")


if __name__ == "__main__":
    main()
