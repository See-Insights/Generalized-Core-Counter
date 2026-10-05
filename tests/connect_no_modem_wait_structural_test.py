#!/usr/bin/env python3
"""WO-2026-10-04-001 item A1: the main loop never waits on the modem.

`Cellular.RSSI()` is a synchronous modem transaction - it takes the modem
client lock and runs AT commands (Device OS `cellular_hal.cpp`,
`sara_ncp_client.cpp`). Before v37, State_Connect.cpp read the signal at the
start of every connect attempt, every 30 s during acquisition, and again on
the timeout path, so a stuck modem could hold CONNECTING_STATE past the 60 s
MCU watchdog. v37 keeps exactly one read, after `Particle.connected()`.

This is a structural test because the property is "unreachable", not "returns
X": it is about which code paths can touch the modem at all. The two
behavioural halves of item A are covered by
tests/connection_signal_validity_test.sh (the surviving read's rounding) and
tests/sleep_gate_modem_off_test.sh (the sleep gate).
"""

import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CONNECT = os.path.join(REPO, "src", "state", "State_Connect.cpp")

HELPER_DEF = "void sampleConnectionSignal(int &strengthPct, int &qualityPct, bool &valid) {"

# Every way State_Connect.cpp can reach the modem for a signal reading.
MODEM_READ_RE = re.compile(r"Cellular\.RSSI\s*\(|WiFi\.RSSI\s*\(|sampleConnectionSignal\s*\(")

failures = []


def check(ok, message):
    print(("  ok   " if ok else "  FAIL ") + message)
    if not ok:
        failures.append(message)


def blank(match):
    """Replace a span with spaces, preserving newlines and so line numbers."""
    return re.sub(r"[^\n]", " ", match.group(0))


def strip_comments(text):
    """Blank out comments.

    Prose that names Cellular.RSSI() - including the comment explaining why
    this very read was removed - must not count as a call site.
    """
    return re.sub(r"/\*.*?\*/|//[^\n]*", blank, text, flags=re.S)


def strip_helper(text):
    """Blank out the sampleConnectionSignal() definition.

    The helper itself must keep calling Cellular.RSSI() - the point of item A1
    is where it is CALLED from, not that the read disappears.
    """
    start = text.index(HELPER_DEF)
    depth = 0
    seen_open = False
    i = start
    while i < len(text):
        if text[i] == "{":
            depth += 1
            seen_open = True
        elif text[i] == "}":
            depth -= 1
            if seen_open and depth == 0:
                break
        i += 1
    end = i + 1
    blanked = re.sub(r"[^\n]", " ", text[start:end])
    return text[:start] + blanked + text[end:]


def line_of(text, index):
    return text.count("\n", 0, index) + 1


def braced_block(text, marker):
    """The source of the braced block introduced by `marker`, and its offset."""
    start = text.index(marker)
    depth = 0
    seen_open = False
    i = start
    while i < len(text):
        if text[i] == "{":
            depth += 1
            seen_open = True
        elif text[i] == "}":
            depth -= 1
            if seen_open and depth == 0:
                break
        i += 1
    return text[start:i + 1], start


def main():
    raw = open(CONNECT).read()
    check(HELPER_DEF in raw, "sampleConnectionSignal() is still the single signal-reading helper")
    text = strip_helper(strip_comments(raw))

    print("")
    print("--- exactly one modem read, and it is after the cloud connects ---")

    reads = list(MODEM_READ_RE.finditer(text))
    check(len(reads) == 1,
          "State_Connect.cpp has exactly one signal read outside the helper (found %d: lines %s)"
          % (len(reads), [line_of(text, m.start()) for m in reads] or "none"))
    if len(reads) != 1:
        return 1

    the_read = reads[0]
    handler, handler_at = braced_block(text, "void handleConnectingState() {")
    cloud_connected_at = handler_at + handler.index("if (cloudConnected) {")
    check(the_read.start() > cloud_connected_at,
          "the surviving read at line %d is inside the `if (cloudConnected)` branch "
          "(which begins at line %d), so no signal read is reachable before "
          "Particle.connected()" % (line_of(text, the_read.start()), line_of(text, cloud_connected_at)))

    post_connect, post_connect_at = braced_block(text[cloud_connected_at:], "if (!postConnectDone) {")
    post_connect_at += cloud_connected_at
    check(post_connect_at <= the_read.start() < post_connect_at + len(post_connect),
          "the surviving read is the ConnSummary: ok sample in the post-connect block")

    print("")
    print("--- the connect-start path does not touch the modem ---")

    before_cloud = text[handler_at:cloud_connected_at]
    check(not MODEM_READ_RE.search(before_cloud),
          "nothing between the top of handleConnectingState() and `if (cloudConnected)` reads the signal "
          "(this covers both the connect-start read and the 30-second ConnDiag read)")
    check("Connect: start budget=" in before_cloud,
          "the Connect: start log line itself is unchanged")
    check("ConnDiag: " in before_cloud and "sig=na" in before_cloud,
          "the ConnDiag trace still exists and reports sig=na")
    check(not re.search(r'ConnDiag: [^"]*sig=%d/%d', text),
          "no ConnDiag variant claims a real reading")

    print("")
    print("--- the timeout path does not touch the modem ---")

    timeout_block, timeout_at = braced_block(text, "if (elapsedMs > budgetMs) {")
    check(not MODEM_READ_RE.search(timeout_block),
          "the `elapsedMs > budgetMs` block (line %d) contains no signal read"
          % line_of(text, timeout_at))
    check('ConnSummary: fail' in timeout_block and "sig=na" in timeout_block,
          "ConnSummary: fail reports sig=na")
    check(not re.search(r'ConnSummary: fail[^"]*sig=%d/%d', text),
          "no ConnSummary: fail variant claims a real reading, so no dead branch was left behind")

    print("")
    print("--- Connect: ok reuses the one sample instead of reading again ---")

    check("Connect: ok elapsed=%lums sig=%d/%d" in post_connect,
          "Connect: ok logs the integer percentages ConnSummary already sampled")
    check("summarySigStrength" in post_connect.split("Connect: ok")[1],
          "Connect: ok's arguments come from the ConnSummary sample")
    check(not re.search(r"Connect: ok[^\"]*sig=%\.0f", text),
          "the old float-formatted Connect: ok read is gone")

    print("")
    if failures:
        print("FAILED: %d check(s)" % len(failures))
        return 1
    print("connect_no_modem_wait_structural_test passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
