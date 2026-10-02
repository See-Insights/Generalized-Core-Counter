#!/usr/bin/env python3
"""WO-2026-10-02-001 item B: failsafe stage 1 (radio reset) is retired.

Structural, because the thing being asserted is the ABSENCE of a code path
and there is no host harness that can observe something that is not there.
The checks are made against the real supervisor body, extracted verbatim.

Also covers the two-line test-mode build fix that B carries, because the
failsafe test-mode image is how the bench proves "no stage-1 line".
"""

import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP = os.path.join(REPO, "src", "Generalized-Core-Counter.cpp")
SRC = os.path.join(REPO, "src")
FS_TEST_CPP = os.path.join(REPO, "src", "diagnostics", "ConnectivityFailsafeTest.cpp")
FS_TEST_H = os.path.join(REPO, "src", "diagnostics", "ConnectivityFailsafeTest.h")

failures = []


def check(ok, message):
    if ok:
        print("  ok   %s" % message)
    else:
        print("  FAIL %s" % message)
        failures.append(message)


def extract_function(path, signature):
    lines = open(path).readlines()
    start = None
    for i, line in enumerate(lines):
        if line.startswith(signature):
            start = i
            break
    if start is None:
        sys.exit("EXTRACT_FAILED: %s not found in %s" % (signature, path))
    depth = 0
    opened = False
    for i in range(start, len(lines)):
        for ch in lines[i]:
            if ch == "{":
                depth += 1
                opened = True
            elif ch == "}":
                depth -= 1
        if opened and depth == 0:
            return "".join(lines[start:i + 1])
    sys.exit("EXTRACT_FAILED: closing brace not found for %s" % signature)


def main():
    supervisor = extract_function(APP, "void connectivityFailsafeSupervisor() {")
    print("--- The supervisor has no radio-reset path ---")

    check("requestFullDisconnectAndRadioOff" not in supervisor,
          "no Connectivity::requestFullDisconnectAndRadioOff() in the supervisor")
    check("transitionTo(CONNECTING_STATE" not in supervisor,
          "no transition back into CONNECTING_STATE from the supervisor")
    check("radio-reset" not in supervisor,
          "no radio-reset action log line")
    check("nextStage == 1" not in supervisor,
          "no 'nextStage == 1' branch")
    check("persistConnectivityFailsafeState(1," not in supervisor,
          "stage 1 is never persisted as an action")
    check("stage=1" not in supervisor,
          "no stage=1 log line")

    print("")
    print("--- The first action is stage 2 ---")

    m = re.search(r"const uint8_t nextStage\s*=\s*([^;]+);", supervisor)
    check(m is not None, "the supervisor computes nextStage")
    if m:
        expr = " ".join(m.group(1).split())
        print("       nextStage = %s" % expr)
        # Evaluate the real expression for each reachable currentStage.
        py = expr.replace("(uint8_t)", "").replace("currentStage", "cs")
        tern = re.fullmatch(r"\s*(.+?)\s*\?\s*(.+?)\s*:\s*(.+?)\s*", py)
        if tern:
            py = "(%s) if (%s) else (%s)" % (tern.group(2), tern.group(1), tern.group(3))
        mapping = {}
        for cs in (0, 1, 2):
            mapping[cs] = eval(py, {}, {"cs": cs})
        check(mapping[0] == 2, "with no stage recorded (0) the next action is stage 2")
        check(mapping[1] == 2, "a stage 1 persisted by older firmware progresses to stage 2")
        check(mapping[2] == 3, "stage 2 still escalates to stage 3")
        check(1 not in mapping.values(),
              "failsafeStage never newly takes 1")

    check("if (nextStage == 2) {" in supervisor,
          "stage 2 (System.reset) is still the system-reset action")
    check("RESET_CAUSE_CONNECTIVITY_FAILSAFE" in supervisor,
          "stage 2 still resets with RESET_CAUSE_CONNECTIVITY_FAILSAFE (code 2)")
    check("ab1805.deepPowerDown()" in supervisor,
          "stage 3 is still the AB1805 deep power-down")

    print("")
    print("--- What became unused is gone ---")

    hits = []
    for root, _dirs, files in os.walk(SRC):
        for name in files:
            if not name.endswith((".cpp", ".h")):
                continue
            path = os.path.join(root, name)
            for n, line in enumerate(open(path).readlines(), 1):
                if re.search(r"\bBREADCRUMB_CONNECTIVITY_FAILSAFE\b", line) and \
                        not line.lstrip().startswith("//"):
                    hits.append("%s:%d" % (os.path.relpath(path, REPO), n))
    check(not hits,
          "BREADCRUMB_CONNECTIVITY_FAILSAFE (9) is gone from src/ (found: %s)" % ", ".join(hits))

    app_text = open(APP).read()
    check("BREADCRUMB_CONNECTIVITY_FAILSAFE_HARD = 10" in app_text,
          "breadcrumb 10 (CONN_FAILSAFE_HARD) is unchanged")
    check("BREADCRUMB_APP_WATCHDOG_RESET = 8" in app_text,
          "the surrounding breadcrumb numbering is unchanged")

    print("")
    print("--- The failsafe test-mode build fix (2 lines) ---")

    cpp_head = open(FS_TEST_CPP).read()
    check('#include "../Config.h"' in cpp_head,
          "ConnectivityFailsafeTest.cpp includes \"../Config.h\" (not the bare form, "
          "which collides with Device OS's own Config.h)")
    check('#include "Config.h"\n' not in cpp_head,
          "ConnectivityFailsafeTest.cpp no longer includes the bare \"Config.h\"")
    check('#include "cloud/BatteryBackoffPolicy.h"' in open(FS_TEST_H).read(),
          "ConnectivityFailsafeTest.h includes cloud/BatteryBackoffPolicy.h (for BatteryTier)")

    print("")
    if failures:
        print("FAILED: %d check(s)" % len(failures))
        return 1
    print("connectivity_failsafe_stage1_retired_test passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
