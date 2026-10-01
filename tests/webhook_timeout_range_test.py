#!/usr/bin/env python3
"""WO-2026-09-29-002 step 4: one webhook-timeout range, honoured by both checks.

Before this work order the configuration-time check accepted 1000..60000 ms
while the runtime check discarded anything outside 5000..120000 ms, so a
webhook timeout of 1000 ms was accepted by the cloud and then silently
replaced with 20000 ms at runtime. The range now lives in exactly one place
(SystemConfig::kWebhookTimeoutMinMs / kWebhookTimeoutMaxMs).

This test does not restate the bounds. It lifts the two real predicate
expressions out of the shipping sources, compiles them against the real
header, and asserts that a value the configuration check accepts is a value
the runtime check actually uses - checked at both edges and just outside
them (4999, 5000, 120000, 120001).
"""

import os
import re
import subprocess
import sys
import tempfile

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(REPO, "src")
HEADER = os.path.join(SRC, "persist", "SystemConfig.h")
CONFIG_APPLY = os.path.join(SRC, "cloud", "ConfigApply.cpp")
MAIN = os.path.join(SRC, "Generalized-Core-Counter.cpp")

BOUNDARIES = [4999, 5000, 120000, 120001]

failures = []


def fail(message):
    failures.append(message)


def read(path):
    with open(path, "r", encoding="utf-8") as handle:
        return handle.read()


def extract_config_bounds(text):
    """The validateRange() call that guards webhookTimeoutMs."""
    match = re.search(
        r"validateRange\(\s*webhookTimeout\s*,\s*(.+?)\s*,\s*(.+?)\s*,\s*\"webhookTimeoutMs\"\s*\)",
        text,
        re.DOTALL,
    )
    if not match:
        fail("ConfigApply.cpp: could not find the validateRange() call for webhookTimeoutMs")
        return None
    return (" ".join(match.group(1).split()), " ".join(match.group(2).split()))


def extract_runtime_bounds(text):
    """The guard that decides whether the stored timeout is used as-is."""
    anchor = text.find("SystemConfig::get_webhookTimeoutMs()")
    if anchor < 0:
        fail("Generalized-Core-Counter.cpp: no runtime read of get_webhookTimeoutMs()")
        return None
    match = re.search(
        r"if\s*\(\s*timeoutMs\s*<\s*(.+?)\s*\|\|\s*timeoutMs\s*>\s*(.+?)\s*\)",
        text[anchor:anchor + 600],
        re.DOTALL,
    )
    if not match:
        fail("Generalized-Core-Counter.cpp: could not find the runtime timeoutMs range guard")
        return None
    return (" ".join(match.group(1).split()), " ".join(match.group(2).split()))


def build_and_run(config_bounds, runtime_bounds):
    """Compile the two real expressions against the real header and compare."""
    program = """
#include "SystemConfig.h"
#include <cstdio>

// validateRange() in src/cloud/ConfigApply.cpp is inclusive:
//     if (value < min || value > max) return false;
static bool configAccepts(int value) {
    return !(value < (%s) || value > (%s));
}

// The runtime guard replaces the stored value with the 20000 ms fallback
// whenever it trips, so "uses the configured value" is its negation.
static bool runtimeUsesConfiguredValue(unsigned long timeoutMs) {
    return !(timeoutMs < (%s) || timeoutMs > (%s));
}

int main() {
    const long samples[] = {%s};
    for (unsigned i = 0; i < sizeof(samples) / sizeof(samples[0]); ++i) {
        printf("%%ld %%d %%d\\n", samples[i], configAccepts((int)samples[i]) ? 1 : 0,
               runtimeUsesConfiguredValue((unsigned long)samples[i]) ? 1 : 0);
    }
    return 0;
}
""" % (
        config_bounds[0],
        config_bounds[1],
        runtime_bounds[0],
        runtime_bounds[1],
        ", ".join("%dL" % value for value in BOUNDARIES),
    )

    with tempfile.TemporaryDirectory() as workdir:
        source = os.path.join(workdir, "webhook_timeout_range_harness.cpp")
        binary = os.path.join(workdir, "harness")
        with open(source, "w", encoding="utf-8") as handle:
            handle.write(program)
        compile_result = subprocess.run(
            ["g++", "-std=c++17", "-Wall", "-Werror", "-I", os.path.dirname(HEADER),
             source, "-o", binary],
            capture_output=True,
            text=True,
        )
        if compile_result.returncode != 0:
            fail("harness failed to compile:\n" + compile_result.stderr.strip())
            return None
        run_result = subprocess.run([binary], capture_output=True, text=True)
        if run_result.returncode != 0:
            fail("harness failed to run:\n" + run_result.stderr.strip())
            return None
        return run_result.stdout


def main():
    header = read(HEADER)
    config_text = read(CONFIG_APPLY)
    main_text = read(MAIN)

    # 1. The range is defined once, in the namespace that owns webhookTimeoutMs.
    for name in ("kWebhookTimeoutMinMs", "kWebhookTimeoutMaxMs"):
        definitions = re.findall(r"constexpr\s+uint32_t\s+" + name, header)
        if len(definitions) != 1:
            fail("SystemConfig.h: expected exactly one definition of %s, found %d"
                 % (name, len(definitions)))
        elsewhere = [
            os.path.relpath(os.path.join(root, filename), REPO)
            for root, _, filenames in os.walk(SRC)
            for filename in filenames
            if filename.endswith((".h", ".cpp"))
            and os.path.join(root, filename) != HEADER
            and re.search(r"constexpr\s+uint32_t\s+" + name, read(os.path.join(root, filename)))
        ]
        if elsewhere:
            fail("%s is redefined outside SystemConfig.h: %s" % (name, ", ".join(elsewhere)))

    # 2. Both call sites reference it by name rather than by a bare literal.
    config_bounds = extract_config_bounds(config_text)
    runtime_bounds = extract_runtime_bounds(main_text)
    if config_bounds is None or runtime_bounds is None:
        report()
        return

    for label, bounds in (("ConfigApply.cpp", config_bounds), ("Generalized-Core-Counter.cpp", runtime_bounds)):
        for bound in bounds:
            if "kWebhookTimeout" not in bound:
                fail("%s uses the literal bound '%s' instead of SystemConfig::kWebhookTimeout*Ms"
                     % (label, bound))

    # 3. The two real expressions agree at and around both edges.
    output = build_and_run(config_bounds, runtime_bounds)
    if output is None:
        report()
        return

    expected_accept = {4999: False, 5000: True, 120000: True, 120001: False}
    seen = set()
    for line in output.strip().splitlines():
        value_text, accepted_text, used_text = line.split()
        value = int(value_text)
        accepted = accepted_text == "1"
        used = used_text == "1"
        seen.add(value)
        if accepted != used:
            fail("%d ms: config check %s it but runtime check %s it - "
                 "an accepted value must be the value used"
                 % (value, "accepts" if accepted else "rejects",
                    "uses" if used else "discards"))
        if accepted != expected_accept[value]:
            fail("%d ms: expected the shared range to %s it, but it is %s"
                 % (value, "accept" if expected_accept[value] else "reject",
                    "accepted" if accepted else "rejected"))
    missing = sorted(set(BOUNDARIES) - seen)
    if missing:
        fail("harness did not report results for: %s" % missing)

    report()


def report():
    if failures:
        print("FAIL: webhook timeout range")
        for failure in failures:
            print("  - " + failure)
        sys.exit(1)
    print("PASS: webhook timeout range is defined once and both checks agree at "
          "4999/5000/120000/120001 ms")
    sys.exit(0)


main()
