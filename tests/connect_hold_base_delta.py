"""WO-2026-11-19-001: the two things this WO changes in what the Sleep S-1..S-6 structural tests pin to their base commits.

Those tests require "no new src/ file" and "SleepTeardown.* unchanged" against the commit each was written at. This WO
adds the pure connect decision (two files) and deletes four session writes from SleepTeardown.cpp: the three write-only
teardown timings, and lastTeardownEndMs, whose only reader (the 30 s reconnect defer in State_Report.cpp) is gone.
Only these deltas are allowed. teardown_base() rewrites the BASE text and never the current text, and the deleted block
must be in the base exactly once, so any other change to SleepTeardown.* or any other new file still fails those tests.
Run directly, this module checks itself.
"""

NEW_SRC_FILES = ["src/power/ConnectDecision.cpp", "src/power/ConnectDecision.h"]

TEARDOWN_DELETED = (
    "    session.lastCloudDisconnectElapsedMs = cloudDisconnectElapsedMs;\n"
    "    session.lastModemOffElapsedMs = modemOffElapsedMs;\n"
    "    session.lastTotalTeardownElapsedMs = teardownElapsedMs;\n"
    "    session.lastTeardownEndMs = millis();\n"
)


def teardown_base(rel, text):
    """The base text of a SleepTeardown file as this WO leaves it (None stays None)."""
    if text is None or not str(rel).endswith("SleepTeardown.cpp"):
        return text
    if text.count(TEARDOWN_DELETED) != 1:
        raise SystemExit("WO-2026-11-19-001: the deleted session writes are not in the base SleepTeardown.cpp exactly once")
    return text.replace(TEARDOWN_DELETED, "")


if __name__ == "__main__":
    sample = "    a();\n" + TEARDOWN_DELETED + "    b();\n"
    assert teardown_base("src/state/SleepTeardown.cpp", sample) == "    a();\n    b();\n"
    assert teardown_base("src/state/SleepTeardown.h", sample) == sample
    assert teardown_base("src/state/SleepTeardown.cpp", None) is None
    for bad in ("    a();\n    b();\n", sample + TEARDOWN_DELETED):
        try:
            teardown_base("src/state/SleepTeardown.cpp", bad)
        except SystemExit:
            continue
        raise SystemExit("FAIL: a base without exactly one deleted block must be rejected")
    print("connect_hold_base_delta: ok")
