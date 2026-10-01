#!/usr/bin/env python3
"""
WO-2026-10-01-001 item A (round 2) - structural invariants that a behavioral
harness cannot express without new mechanism.

`tests/firmware_update_dwell_test.sh` covers the behavior (handler recording,
the sleep-gate check, the dwell exits). This file pins the shape rules that go
with it:

  1. The `firmware_update` system event is registered in `setup()` and bound to
     `firmwareUpdateHandler`, and that handler only records: it contains no
     `transitionTo(`, no `Particle.`, no `System.` and no publish/queue call.
  2. Round 1's loop-level begin transition and its flags are gone: the loop
     owns no firmware-update transition any more.
  3. The sleep gate's firmware-update check sits inside `handleSleepingState()`
     and textually *before* the first teardown request in that function, so a
     download in flight is never lost to a cloud disconnect or radio-off.
  4. `handleFirmwareUpdateState()` carries the flag-clear exit and the
     no-progress exit, and none of the removed exits - the
     `System.updatesPending()` exit, the fixed absolute cap measured from
     `firmwareUpdateStartMs` alone, and the complete/failed exits to Idle.
  5. `ThrashGuard::timeoutForStateSec(FIRMWARE_UPDATE_STATE)` stays above the
     300 s no-progress window, so ThrashGuard cannot pre-empt it (Stage 5
     decision 1).

Checks run on the REAL checked-in sources with comments stripped first, so
prose cannot create a false positive or a false negative.
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
MAIN = REPO_ROOT / "src" / "Generalized-Core-Counter.cpp"
STATE_CONNECT = REPO_ROOT / "src" / "state" / "State_Connect.cpp"
STATE_SLEEP = REPO_ROOT / "src" / "state" / "State_Sleep.cpp"
THRASH_GUARD = REPO_ROOT / "src" / "ThrashGuard.cpp"

MIN_THRASH_TIMEOUT_SEC = 300

TEARDOWN_CALLS = (
    "Connectivity::requestFullDisconnectAndRadioOff(",
    "Connectivity::requestCloudDisconnectOnly(",
    "Connectivity::requestRadioPowerOff(",
)


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def strip_comments(code: str) -> str:
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)
    code = re.sub(r"//[^\n]*", "", code)
    return code


def extract_block(text: str, start_pattern: str, name: str) -> str:
    match = re.search(start_pattern, text)
    if not match:
        fail(f"could not find {name}")
    idx = text.index("{", match.start())
    depth = 0
    for pos in range(idx, len(text)):
        if text[pos] == "{":
            depth += 1
        elif text[pos] == "}":
            depth -= 1
            if depth == 0:
                return text[idx : pos + 1]
    fail(f"unterminated block for {name}")
    return ""


def main() -> None:
    main_src = strip_comments(MAIN.read_text())
    connect_src = strip_comments(STATE_CONNECT.read_text())
    sleep_src = strip_comments(STATE_SLEEP.read_text())
    thrash_src = strip_comments(THRASH_GUARD.read_text())

    # --- 1. Registration ------------------------------------------------------
    registration = re.search(
        r"System\.on\(\s*firmware_update\s*,\s*firmwareUpdateHandler\s*\)", main_src
    )
    if not registration:
        fail("setup() does not register firmwareUpdateHandler for the firmware_update event")

    setup_body = extract_block(main_src, r"\bvoid\s+setup\s*\(\s*\)\s*\{", "setup()")
    if "System.on(firmware_update,firmwareUpdateHandler)" not in re.sub(r"\s+", "", setup_body):
        fail("the firmware_update registration is not inside setup()")

    # --- 1b. The handler records only ----------------------------------------
    handler = extract_block(
        main_src, r"\bvoid\s+firmwareUpdateHandler\s*\([^)]*\)\s*\{", "firmwareUpdateHandler()"
    )
    for forbidden in ("transitionTo(", "Particle.", "System.", "publish", "Log."):
        if forbidden in handler:
            fail(
                f"firmwareUpdateHandler() must only record state, but it contains "
                f"'{forbidden}'"
            )
    for required in (
        "firmwareUpdateInProgress",
        "firmwareUpdateLastActivityMs",
        "millis()",
    ):
        if required not in handler:
            fail(f"firmwareUpdateHandler() does not record '{required}'")

    # --- 2. Round 1's loop-level begin transition is gone ---------------------
    for retired in ("firmwareUpdateBeginPending", "firmwareUpdateLastEvent"):
        if retired in main_src or retired in connect_src:
            fail(
                f"'{retired}' is round 1 state that round 2 removes; the handler now "
                "records only firmwareUpdateInProgress and the activity stamp"
            )
    loop_body = extract_block(main_src, r"\bvoid\s+loop\s*\(\s*\)\s*\{", "loop()")
    if "transitionTo(FIRMWARE_UPDATE_STATE" in loop_body:
        fail("loop() must not own a firmware-update transition; the sleep gate does")

    # --- 3. The sleep gate checks before any teardown request -----------------
    sleeping = extract_block(
        sleep_src, r"\bvoid\s+handleSleepingState\s*\(\s*\)\s*\{", "handleSleepingState()"
    )
    guard = re.search(
        r"if\s*\(\s*!\s*disconnectRequested\s*&&\s*firmwareUpdateInProgress\s*\)", sleeping
    )
    if not guard:
        fail(
            "handleSleepingState() has no `if (!disconnectRequested && "
            "firmwareUpdateInProgress)` check; a download in flight would be slept through"
        )
    guard_block = sleeping[guard.start() : guard.start() + 500]
    if 'transitionTo(FIRMWARE_UPDATE_STATE, "firmware update in progress")' not in guard_block:
        fail(
            "the sleep-gate firmware-update check does not transition to "
            "FIRMWARE_UPDATE_STATE with the expected reason"
        )
    if "return;" not in guard_block:
        fail("the sleep-gate firmware-update check does not return after transitioning")

    teardown_positions = [
        sleeping.index(call) for call in TEARDOWN_CALLS if call in sleeping
    ]
    if not teardown_positions:
        fail("handleSleepingState() has no teardown request to order the check against")
    first_teardown = min(teardown_positions)
    if guard.start() > first_teardown:
        fail(
            "the sleep-gate firmware-update check comes AFTER the first teardown request "
            "in handleSleepingState(); by then the download is already lost"
        )

    # --- 4. The dwell state's exits ------------------------------------------
    dwell = extract_block(
        connect_src,
        r"\bvoid\s+handleFirmwareUpdateState\s*\(\s*\)\s*\{",
        "handleFirmwareUpdateState()",
    )
    if "updatesPending" in dwell:
        fail(
            "handleFirmwareUpdateState() still consults System.updatesPending(); Device OS "
            "clears that flag for the whole transfer (WO problem 1)"
        )
    flat = re.sub(r"\s+", " ", dwell)
    if re.search(r"millis\(\) - firmwareUpdateStartMs\s*\)?\s*>", flat):
        fail(
            "handleFirmwareUpdateState() still applies a fixed cap measured from "
            "firmwareUpdateStartMs; the exit must be a no-progress window"
        )
    if "thrashGuard.markProgress(" not in dwell:
        fail("handleFirmwareUpdateState() does not refresh ThrashGuard on progress")
    if not re.search(r"if\s*\(\s*!\s*firmwareUpdateInProgress\s*\)", dwell):
        fail("handleFirmwareUpdateState() has no flag-clear exit")
    if 'transitionTo(SLEEPING_STATE, "firmware-update-not-in-progress")' not in dwell:
        fail("the flag-clear exit does not go to SLEEPING_STATE")
    if 'transitionTo(SLEEPING_STATE, "firmware-update-no-progress")' not in dwell:
        fail("handleFirmwareUpdateState() has no no-progress exit to SLEEPING_STATE")
    idle_exits = re.findall(r"transitionTo\(IDLE_STATE,\s*\"([^\"]+)\"", dwell)
    if idle_exits != ["firmware-update-button-exit"]:
        fail(
            "the only exit to IDLE_STATE must be the button override, but found: "
            f"{idle_exits}"
        )

    # --- 5. ThrashGuard must not pre-empt the no-progress window --------------
    timeouts = extract_block(
        thrash_src,
        r"uint32_t\s+ThrashGuard::timeoutForStateSec\s*\([^)]*\)[^{]*\{",
        "ThrashGuard::timeoutForStateSec()",
    )
    match = re.search(r"case\s+FIRMWARE_UPDATE_STATE\s*:\s*return\s+(\d+)\s*;", timeouts)
    if not match:
        fail("ThrashGuard::timeoutForStateSec() has no FIRMWARE_UPDATE_STATE case")
    timeout_sec = int(match.group(1))
    if timeout_sec <= MIN_THRASH_TIMEOUT_SEC:
        fail(
            f"ThrashGuard FIRMWARE_UPDATE_STATE timeout is {timeout_sec}s; it must exceed "
            f"{MIN_THRASH_TIMEOUT_SEC}s so it cannot pre-empt the 5-minute no-progress exit"
        )

    print(
        "Firmware update wiring structural test passed "
        f"(record-only handler, pre-teardown sleep check, ThrashGuard timeout {timeout_sec}s)"
    )


if __name__ == "__main__":
    main()
