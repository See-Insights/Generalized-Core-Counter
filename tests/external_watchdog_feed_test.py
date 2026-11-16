#!/usr/bin/env python3
"""WO-2026-11-16-001: the loop must keep the AB1805 chip watchdog fed through its WDT register.

THIS IS A MODEL, NOT THE REAL FUNCTION. The Device OS watchdog HAL is a stub on the host, so
Am18x5::feedWatchdog() cannot run here. The model follows Device OS 6.6.0 hal/shared/am18x5.cpp:934-946:
  - no watchdog pin configured: feedWatchdog() writes the chip's WDT register;
  - a watchdog pin configured:  feedWatchdog() only toggles that GPIO.
The chip counter model: 124 s; only a register write reloads it; a GPIO edge does not (D5's wiring to
the chip's WDI pin is unverified). Bench steps 1 and 2 of the WO prove the real behaviour.

The fleet Am18x5Configuration chain and the loop() refresh are read from the real src/ file.
Each mutation is applied to a copy of the source; the check must then FAIL.
Run from anywhere; no network.
"""
import re, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MAIN = ROOT / "src" / "Generalized-Core-Counter.cpp"

COUNTER_S = 124
FEED_PERIOD_S = 30
WINDOW_S = 200


def strip(code):
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.S)
    return re.sub(r"//[^\n]*", "", code)


def fleet_has_watchdog_pin(m):
    chain = re.search(r"ExternalRtc::ensureBound\(Am18x5,(.*?)\);", m, re.S)
    if not chain:
        return None
    return ".watchdogPin(" in chain.group(1)


def simulate(pin_configured, feeds_enabled=True):
    """Returns (register_writes, feeds, expired_at_s or None) over the awake window."""
    counter, writes, feeds = COUNTER_S, 0, 0
    for t in range(1, WINDOW_S + 1):
        counter -= 1
        if counter <= 0:
            return writes, feeds, t
        if feeds_enabled and t % FEED_PERIOD_S == 0:
            feeds += 1
            if not pin_configured:  # model of feedWatchdog(): register write reloads the counter
                writes += 1
                counter = COUNTER_S
            # pin configured: a GPIO toggle, which does not reload the chip counter
    return writes, feeds, None


def check(main_text):
    errs = []
    m = strip(main_text)
    pin = fleet_has_watchdog_pin(m)
    if pin is None:
        return ["fleet Am18x5Configuration chain not found"]
    writes, feeds, expired = simulate(pin)
    if expired is not None:
        errs.append(f"the chip counter expires at {expired} s in a {WINDOW_S} s awake window")
    if writes != feeds or feeds == 0:
        errs.append(f"expected one register write per feed, got {writes} writes for {feeds} feeds")

    if "void loop()" not in m:
        return errs + ["loop() not found"]
    loop = m[m.index("void loop()"):]
    if loop.count("ExternalWatchdog.refresh()") != 1 or m.count("ExternalWatchdog.refresh()") != 1:
        errs.append("ExternalWatchdog.refresh() must appear exactly once")
    if not re.search(r"(?:const\s+)?int\s+refreshRc\s*=\s*ExternalWatchdog\.refresh\(\);", loop):
        errs.append("the refresh return code is not kept")
    if not re.search(r"static int lastRefreshRc\s*=\s*SYSTEM_ERROR_NONE;", loop):
        errs.append("the change latch lastRefreshRc is missing")
    if not re.search(r'if \(refreshRc != lastRefreshRc\) Log\.warn\("ExternalWatchdog: refresh rc=%d", refreshRc\);\s*lastRefreshRc = refreshRc;', loop):
        errs.append("the change-detecting Log.warn of the refresh rc is missing")
    return errs


def main():
    text = MAIN.read_text()
    errs = check(text)
    if errs:
        for e in errs:
            print("FAIL:", e)
        sys.exit(1)
    print("OK: fleet config feeds through the register; counter never expires; loop keeps and logs the rc")

    def rep(a, b):
        assert a in text, a
        return text.replace(a, b, 1)

    mut = {
        "pin-restored": rep(".interruptPin(WKP)", ".interruptPin(WKP).watchdogPin(WATCHDOG_DONE_PIN)"),
        "refresh-removed": rep("const int refreshRc = ExternalWatchdog.refresh();", "const int refreshRc = SYSTEM_ERROR_NONE;"),
        "rc-discarded": rep("const int refreshRc = ExternalWatchdog.refresh();", "ExternalWatchdog.refresh(); const int refreshRc = 0;"),
        "rc-logging-removed": rep('if (refreshRc != lastRefreshRc) Log.warn("ExternalWatchdog: refresh rc=%d", refreshRc);', ""),
        "rc-logged-every-loop": rep("if (refreshRc != lastRefreshRc) Log.warn", "if (refreshRc) Log.warn"),
    }
    bad = 0
    for name, t in mut.items():
        caught = bool(check(t))
        print(("OK" if caught else "FAIL"), f"mutation {name} is caught")
        bad += not caught
    # the model itself: with the pin the counter must expire at 124 s
    _, _, exp = simulate(True)
    if exp != COUNTER_S:
        print("FAIL: model, pin configured should expire at", COUNTER_S, "got", exp)
        bad += 1
    if bad:
        sys.exit(1)
    print("external_watchdog_feed_test: all checks passed")


main()
