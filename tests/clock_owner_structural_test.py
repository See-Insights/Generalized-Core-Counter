#!/usr/bin/env python3
"""
WO-2026-09-18 Step 3a - structural regression test for the clock module's
single ownership of "what time is it, and can it be trusted."

Background: before this step, `isWithinOpenHours()`/`isWithinOpenHoursAt()`/
`secondsUntilNextOpen()` (open-hours evaluation) and `checkClockResync()`/
`requestClockResync()`/`isClockTrusted()`/`observedTimeSyncedLastMs()`/
`reportedSyncAgeMs()` (the resync/RTC-write-back cycle and trust signal)
were all defined directly inside `Generalized-Core-Counter.cpp`, and
`src/state/*.cpp` decision sites called `Time.isValid()` - the raw Particle
API - directly, rather than through any owning module. This is the
"no single owner of what time is it" finding from
`docs/architecture-review-2026-09-03.md`, Q1 of the agreed Structural
Ownership Map roadmap.

This step RELOCATED (not redesigned) every one of those functions verbatim
into `src/time/Clock.{h,cpp}` - same global-scope names, same bodies, only
the defining file moved - and introduced exactly two NEW, namespaced
functions: `Clock::isTimeValid()` (a semantics-preserving `return
Time.isValid();` wrapper) and `Clock::isPlausibleEpoch()` (absorbing the
former `State_Sleep.cpp`-local `isRtcTimeValidForHibernate()`). Every
`src/state/*.cpp` decision site that used to call `Time.isValid()` directly
now calls `Clock::isTimeValid()` instead - the same decision, spelled
through the module that owns it.

This is a source-invariant check on the REAL checked-in files, not a
mirror. Comments are stripped before each check so prose (including this
file's own history, if pasted into source) cannot produce a false positive
or a false negative.

  1. `src/time/Clock.h`/`src/time/Clock.cpp` exist, and Clock.cpp defines
     each of the eight relocated functions exactly once; none of their
     definitions remain in `Generalized-Core-Counter.cpp`.
  2. `isRtcTimeValidForHibernate` no longer exists anywhere in `src/` -
     folded into `Clock::isPlausibleEpoch`, which `State_Sleep.cpp` calls
     exactly once.
  3. Zero raw `Time.isValid()` calls exist anywhere under `src/state/` (all
     converted to `Clock::isTimeValid()`).
  4. In `Generalized-Core-Counter.cpp`, every surviving raw `Time.isValid()`
     call is on the explicitly enumerated formatting/telemetry allowlist
     below - not a decision site. A decision site that reintroduces a bare
     `Time.isValid()` (anywhere in the file, not just the three sites this
     step converted) fails this check.
  5. `MyPersistentData.cpp` includes `time/Clock.h` rather than
     forward-declaring `isClockTrusted()` itself (the "correct direction"
     fix named explicitly by this step's dispatch).
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_ROOT = REPO_ROOT / "src"
CLOCK_H = SRC_ROOT / "time" / "Clock.h"
CLOCK_CPP = SRC_ROOT / "time" / "Clock.cpp"
APP_SRC = SRC_ROOT / "Generalized-Core-Counter.cpp"
STATE_SLEEP = SRC_ROOT / "state" / "State_Sleep.cpp"
MY_PERSISTENT_DATA_CPP = SRC_ROOT / "MyPersistentData.cpp"

# Formatting/telemetry-only Time.isValid() call sites explicitly permitted
# to remain in Generalized-Core-Counter.cpp - each feeds only a Log.info/
# Log.warn line or a purely diagnostic local, never a branch that changes
# what the device does. Matched as exact (whitespace-trimmed) source lines
# after comment-stripping, so a NEW decision site elsewhere in the file
# (even one that happens to share a substring with one of these) cannot
# hide behind this allowlist.
ALLOWED_APP_SRC_TIME_ISVALID_LINES = {
    "const bool timeValidBeforeRtc = Time.isValid();",
    "const bool timeValidAfterRtc = Time.isValid();",
    "const bool timeValid = Time.isValid();",
    "Time.isValid() ? (isWithinOpenHours() ? 1 : 0) : -1,",
}

RELOCATED_FUNCTION_SIGNATURES = {
    "isWithinOpenHours": r"bool\s+isWithinOpenHours\s*\(\s*\)\s*\{",
    "isWithinOpenHoursAt": r"bool\s+isWithinOpenHoursAt\s*\(time_t\s+\w+\)\s*\{",
    "secondsUntilNextOpen": r"int\s+secondsUntilNextOpen\s*\(\s*\)\s*\{",
    "requestClockResync": r"void\s+requestClockResync\s*\(",
    "checkClockResync": r"void\s+checkClockResync\s*\(\s*\)\s*\{",
    "isClockTrusted": r"bool\s+isClockTrusted\s*\(\s*\)\s*\{",
    "observedTimeSyncedLastMs": r"uint32_t\s+observedTimeSyncedLastMs\s*\(\s*\)\s*\{",
    "reportedSyncAgeMs": r"uint32_t\s+reportedSyncAgeMs\s*\(\s*\)\s*\{",
}


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def strip_comments(code: str) -> str:
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)
    code = re.sub(r"//[^\n]*", "", code)
    return code


def main() -> None:
    if not CLOCK_H.is_file():
        fail(f"{CLOCK_H} does not exist")
    if not CLOCK_CPP.is_file():
        fail(f"{CLOCK_CPP} does not exist")

    clock_cpp_text = strip_comments(CLOCK_CPP.read_text())
    app_text_raw = APP_SRC.read_text()
    app_text = strip_comments(app_text_raw)

    # --- Invariant 1: each relocated function is defined exactly once, in ---
    # --- Clock.cpp, and not (re)defined in Generalized-Core-Counter.cpp.  ---
    for name, pattern in RELOCATED_FUNCTION_SIGNATURES.items():
        clock_count = len(re.findall(pattern, clock_cpp_text))
        if clock_count != 1:
            fail(
                f"{name}() must be defined exactly once in {CLOCK_CPP.name}, "
                f"found {clock_count} - Step 3a relocated it there as the "
                "clock module's single owner"
            )
        app_count = len(re.findall(pattern, app_text))
        if app_count != 0:
            fail(
                f"{name}() must NOT be (re)defined in {APP_SRC.name} - it "
                f"was relocated to {CLOCK_CPP.name} by Step 3a, found "
                f"{app_count} definition(s) still in {APP_SRC.name}"
            )

    # --- Invariant 2: isRtcTimeValidForHibernate is gone; folded into ---
    # --- Clock::isPlausibleEpoch, called exactly once.                ---
    for path in SRC_ROOT.rglob("*.cpp"):
        if "isRtcTimeValidForHibernate" in strip_comments(path.read_text()):
            fail(
                f"isRtcTimeValidForHibernate must not exist anywhere in src/ "
                f"(found in {path.relative_to(REPO_ROOT)}) - it was folded "
                "into Clock::isPlausibleEpoch by Step 3a"
            )
    for path in SRC_ROOT.rglob("*.h"):
        if "isRtcTimeValidForHibernate" in strip_comments(path.read_text()):
            fail(
                f"isRtcTimeValidForHibernate must not exist anywhere in src/ "
                f"(found in {path.relative_to(REPO_ROOT)}) - it was folded "
                "into Clock::isPlausibleEpoch by Step 3a"
            )
    if "bool isPlausibleEpoch(time_t epoch) {" not in clock_cpp_text:
        fail(f"Clock::isPlausibleEpoch() must be defined in {CLOCK_CPP.name}")
    plausible_epoch_callers = len(
        re.findall(r"Clock::isPlausibleEpoch\s*\(", strip_comments(STATE_SLEEP.read_text()))
    )
    if plausible_epoch_callers != 1:
        fail(
            f"{STATE_SLEEP.name} must call Clock::isPlausibleEpoch(...) exactly "
            f"once (shouldUseBoronRtcAlarmHibernate()'s RTC-plausibility check), "
            f"found {plausible_epoch_callers}"
        )

    # --- Invariant 3: zero raw Time.isValid() calls anywhere under        ---
    # --- src/state/ - every decision site there now goes through         ---
    # --- Clock::isTimeValid() instead of the raw Particle API.           ---
    state_dir = SRC_ROOT / "state"
    offenders = []
    for path in sorted(state_dir.rglob("*")):
        if path.suffix not in (".cpp", ".h"):
            continue
        if "Time.isValid()" in strip_comments(path.read_text()):
            offenders.append(str(path.relative_to(REPO_ROOT)))
    if offenders:
        fail(
            "Time.isValid() must not appear in any file under src/state/ - "
            "found in: " + ", ".join(offenders) + " (Step 3a requires these "
            "decision sites to call Clock::isTimeValid() instead)"
        )

    # --- Invariant 4: every surviving Time.isValid() call in             ---
    # --- Generalized-Core-Counter.cpp is on the formatting/telemetry     ---
    # --- allowlist - not a decision site.                                ---
    offending_lines = []
    for line in app_text.splitlines():
        if "Time.isValid()" in line and line.strip() not in ALLOWED_APP_SRC_TIME_ISVALID_LINES:
            offending_lines.append(line.strip())
    if offending_lines:
        fail(
            f"{APP_SRC.name} contains Time.isValid() call(s) not on the "
            "formatting/telemetry allowlist (Step 3a requires every "
            "decision site to use Clock::isTimeValid() instead): "
            + " | ".join(offending_lines)
        )

    # Positive control: every allowlisted line must actually still be
    # present - otherwise this allowlist could silently go stale (e.g. a
    # rename that happens to also satisfy invariant 4 vacuously).
    present_lines = {line.strip() for line in app_text.splitlines()}
    missing_allowed = ALLOWED_APP_SRC_TIME_ISVALID_LINES - present_lines
    if missing_allowed:
        fail(
            f"{APP_SRC.name} is missing expected formatting/telemetry "
            f"Time.isValid() line(s): {missing_allowed} - update this test's "
            "allowlist if these were deliberately rephrased, or investigate "
            "if they were accidentally removed"
        )

    # --- Invariant 5: MyPersistentData.cpp uses the correct-direction     ---
    # --- include rather than forward-declaring isClockTrusted() itself.  ---
    persistent_data_text = MY_PERSISTENT_DATA_CPP.read_text()
    if "#include \"time/Clock.h\"" not in persistent_data_text:
        fail(f'{MY_PERSISTENT_DATA_CPP.name} must #include "time/Clock.h"')
    if re.search(r"^\s*bool\s+isClockTrusted\s*\(\s*\)\s*;", persistent_data_text, re.MULTILINE):
        fail(
            f"{MY_PERSISTENT_DATA_CPP.name} must not forward-declare "
            "isClockTrusted() itself - it should come from time/Clock.h "
            "(Step 3a's 'correct direction' fix)"
        )

    print(f"OK: all 8 relocated functions defined exactly once in {CLOCK_CPP.name}, none remain in {APP_SRC.name}")
    print("OK: isRtcTimeValidForHibernate is gone; Clock::isPlausibleEpoch has exactly one caller")
    print("OK: zero raw Time.isValid() calls under src/state/")
    print(f"OK: every surviving Time.isValid() call in {APP_SRC.name} is on the formatting/telemetry allowlist")
    print(f"OK: {MY_PERSISTENT_DATA_CPP.name} includes time/Clock.h instead of forward-declaring isClockTrusted()")
    print("clock_owner_structural_test: all invariants hold")


if __name__ == "__main__":
    main()
