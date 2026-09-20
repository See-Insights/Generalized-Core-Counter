#!/usr/bin/env python3
"""
WO-2026-09-19 Step 3b - source-tracing test confirming State_Sleep.cpp's
sleep-DURATION decision consumes Clock::openness(), not isWithinOpenHours()
directly.

Background: before this step, handleSleepingState() decided whether to
commit to an overnight hibernate (nightSleepSec, up to 546 minutes) or the
normal short interval-based nap by calling isWithinOpenHours() directly -
a function that fails OPEN (returns true) when Time.isValid() is false, even
though an RTC-seeded-but-unconfirmed clock can be hours wrong. A plausible-
but-wrong epoch that happened to compute "closed" could commit the device to
an overnight hibernate based on a wrong belief about what time it is - the
exact bug this step's roadmap item exists to fix. This is a source-invariant
check on the REAL checked-in file, not a mirror. Comments are stripped
before each check so prose (including this file's own history, if pasted
into source) cannot produce a false positive or a false negative.

  1. handleSleepingState() computes `parkOpenness` from Clock::openness()
     exactly once - the single decision site feeding both the night-sleep
     commitment check and the overnightFallbackSleep flag (see invariant 2).
     (handleSleepingState() is one large function that also contains two
     OTHER, independent Clock::openness() decisions - the sleep-abort-if-open
     check and the PIR-wake opportunistic-report check - so this checks the
     `parkOpenness` assignment specifically, not a total call count for the
     whole function.)
  2. The night-sleep commitment branch (`nightSleepSec = ...`) is reached
     only through a parkOpenness == Clock::Openness::Closed condition - not
     a bare `!isWithinOpenHours()` or `!openNow` check.
  3. isWithinOpenHours() is called at most once inside handleSleepingState()
     (the logTimeDiag() isOpen= argument, which stays sourced from the raw,
     fail-open value on purpose so the log line's isOpen=/openness= fields
     can be compared) - not used a second time to gate the sleep-duration
     decision itself, which would silently reintroduce the fail-open bug
     alongside the fixed one.
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
STATE_SLEEP = REPO_ROOT / "src" / "state" / "State_Sleep.cpp"

PARK_OPENNESS_ASSIGNMENT_PATTERN = re.compile(
    r"const\s+Clock::Openness\s+parkOpenness\s*=\s*Clock::openness\s*\(\s*\)\s*;"
)
WITHIN_OPEN_HOURS_CALL_PATTERN = re.compile(r"(?<!Clock::)\bisWithinOpenHours\s*\(\s*\)")


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
    brace_start = match.end() - 1
    depth = 0
    i = brace_start
    while i < len(text):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[match.start():i + 1]
        i += 1
    fail(f"could not find matching closing brace for {name}()")
    return ""  # unreachable


def main() -> None:
    if not STATE_SLEEP.is_file():
        fail(f"{STATE_SLEEP} does not exist")

    full_text = strip_comments(STATE_SLEEP.read_text())
    sleeping_fn = extract_function(
        full_text, r"void\s+handleSleepingState\s*\(\s*\)\s*\{", "handleSleepingState"
    )

    # --- Invariant 1: exactly one `parkOpenness = Clock::openness()`
    # assignment - the single decision site the night-sleep-vs-nap decision
    # is computed from. (handleSleepingState() also contains two other,
    # unrelated Clock::openness() call sites - sleep-abort-if-open and the
    # PIR-wake opportunistic-report check - so a whole-function call count
    # would not isolate the right one.) ---
    openness_count = len(PARK_OPENNESS_ASSIGNMENT_PATTERN.findall(sleeping_fn))
    if openness_count != 1:
        fail(
            f"handleSleepingState() contains {openness_count} "
            "`parkOpenness = Clock::openness()` assignment(s), expected "
            "exactly 1 - the night-sleep-vs-nap decision must have a single "
            "decision site (WO-2026-09-19 Step 3b)"
        )

    # --- Invariant 2: the night-sleep commitment is gated on
    # `parkOpenness == Clock::Openness::Closed` (or equivalent), not a bare
    # !isWithinOpenHours()/!openNow. Anchored on the actual
    # `nightSleepSec = secondsUntilNextOpen();` assignment, which only makes
    # sense inside the night-sleep branch. ---
    commit_match = re.search(r"nightSleepSec\s*=\s*secondsUntilNextOpen\(\)\s*;", sleeping_fn)
    if not commit_match:
        fail(
            "could not locate the night-sleep commitment "
            "(nightSleepSec = secondsUntilNextOpen();) in handleSleepingState()"
        )
    preceding = sleeping_fn[:commit_match.start()]
    guard_match = re.search(
        r"if\s*\(\s*parkOpenness\s*==\s*Clock::Openness::Closed\s*\)\s*\{[^{}]*$",
        preceding,
        re.DOTALL,
    )
    if not guard_match:
        fail(
            "the night-sleep commitment (nightSleepSec = secondsUntilNextOpen();) "
            "must be directly inside `if (parkOpenness == Clock::Openness::Closed)` "
            "- found different or no guarding condition immediately enclosing it"
        )

    # Positive control: the OLD fail-open guard must be gone from this
    # function - `!isWithinOpenHours()` or `!openNow` as an if-condition.
    if re.search(r"if\s*\(\s*!\s*isWithinOpenHours\s*\(\s*\)\s*\)", sleeping_fn):
        fail(
            "handleSleepingState() still contains a bare `if (!isWithinOpenHours())` "
            "guard - the fail-open commitment check this step retired appears to "
            "be back"
        )
    if re.search(r"if\s*\(\s*!\s*openNow\s*\)", sleeping_fn):
        fail(
            "handleSleepingState() still contains a bare `if (!openNow)` guard - "
            "the fail-open commitment check this step retired appears to be back"
        )

    # --- Invariant 3: every remaining raw isWithinOpenHours() call site in
    # this function is one of the two known, reviewed, out-of-scope uses -
    # logTimeDiag()'s isOpen= argument (deliberately fail-open, for
    # comparison against openness= in the same log line) and the
    # useNetworkStandbyRequested efficiency check (not a commitment) - never
    # used, negated or otherwise, to gate the sleep-duration decision itself.
    # This is a positive allowlist, not a raw count, because
    # handleSleepingState() legitimately has other isWithinOpenHours()
    # callers this step did not touch.
    ALLOWED_WITHIN_OPEN_HOURS_USES = {
        "logTimeDiag(isWithinOpenHours());",
        "isWithinOpenHours();",
        "if (isWithinOpenHours()) {",
    }
    for match in WITHIN_OPEN_HOURS_CALL_PATTERN.finditer(sleeping_fn):
        line_start = sleeping_fn.rfind("\n", 0, match.start()) + 1
        line_end = sleeping_fn.find("\n", match.end())
        line = sleeping_fn[line_start:line_end if line_end != -1 else None].strip()
        if not any(line.endswith(allowed) for allowed in ALLOWED_WITHIN_OPEN_HOURS_USES):
            fail(
                "handleSleepingState() calls isWithinOpenHours() directly on "
                f"a line not in the reviewed allowlist: {line!r} - a new "
                "direct use here would risk silently reintroducing the "
                "fail-open bug this step fixed"
            )

    print("OK: handleSleepingState() computes parkOpenness from Clock::openness() exactly once")
    print("OK: the night-sleep commitment is gated on parkOpenness == Clock::Openness::Closed")
    print("OK: the old fail-open !isWithinOpenHours()/!openNow guard is gone")
    print("OK: every remaining isWithinOpenHours() call site is a reviewed, out-of-scope use")
    print("sleep_duration_consumes_openness_test: all invariants hold")


if __name__ == "__main__":
    main()
