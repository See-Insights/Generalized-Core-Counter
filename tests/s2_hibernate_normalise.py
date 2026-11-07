"""v50 S2 (WO-2026-11-05-004, Amendment 4 F6): base comparison of hibernateAttempt() with only the approved S2 changes normalised.

normalise(text, is_base) rewrites hibernateAttempt() with indentation and blank lines dropped (the S2 block lost four
nesting levels). When is_base is true the approved S2 substitutions below are applied to the base text so it becomes the
expected S2 form; the current text is never rewritten, so any other change to the function shows as a difference.
"""

SUBSTITUTIONS = [
    # The AB1805 stale-interrupt clear and interruptAtTime() pre-arm (and their else branches) are gone.
    ("if (clearAb1805StaleAlarmInterrupts()) {\nconst time_t wakeTime = rtcNow + (time_t)wakeInSeconds;\nif (ab1805.interruptAtTime(wakeTime)) {\n", ""),
    ("} else {\nLog.warn(\"HIBERNATE bypass: failed to program AB1805 alarm - falling back to ULTRA_LOW_POWER\");\n}\n"
     "} else {\nLog.warn(\"HIBERNATE bypass: failed stale interrupt cleanup - falling back to ULTRA_LOW_POWER\");\n}\n", ""),
    # Device OS arms the chip alarm from duration().
    (".gpio(BUTTON_PIN, FALLING);\n", ".gpio(BUTTON_PIN, FALLING)\n.duration((uint32_t)wakeInSeconds * 1000UL);\n"),
    # The comment that described the AB1805 pre-arm.
    ("// WO-2026-09-16 Step 2: HibernateCycle now owns the retained\n// hibernate-cycle fields; wakeTime itself is used above for\n"
     "// ab1805.interruptAtTime() only - nothing reads it back on\n// wake (confirmed dead as a retained field), so it is not\n// passed through here.\n",
     "// HibernateCycle owns the retained hibernate-cycle fields; Device OS\n// arms the chip alarm itself from duration().\n"),
]

# The chip watchdog replaces the AB1805 one at every sleep site (hibernate and ULP/STOP): (old, new, expected count in the base).
WATCHDOG = [
    ("const bool watchdogStopped = ab1805.stopWDT();", "const bool watchdogStopped = (ExternalWatchdog.stop() == SYSTEM_ERROR_NONE);", 2),
    ("ab1805.resumeWDT();", "ExternalWatchdog.start();", 3),
]


def _span(t):
    i = t.index("hibernateAttempt(")
    i = t.rfind("\n", 0, t.rfind("void", 0, i) + 1)
    return i, t.index("\n}\n", i) + 3


def normalise(t, is_base):
    i, j = _span(t)
    body = "\n".join(l.strip() for l in t[i:j].splitlines() if l.strip()) + "\n"
    if is_base:
        for old, new in SUBSTITUTIONS:
            if body.count(old) != 1:
                raise SystemExit(f"S2 substitution does not apply to the base exactly once: {old[:60]!r}")
            body = body.replace(old, new)
    t = t[:i] + body + t[j:]
    if is_base:
        for old, new, count in WATCHDOG:
            if t.count(old) != count:
                raise SystemExit(f"S2 watchdog substitution expected {count}x in the base: {old!r}")
            t = t.replace(old, new)
    return t


def normalise_block(block, is_base, indent_free, stops, resumes):
    """For tests that already cut a block out of the file: the same normalisation on that block.
    indent_free: also drop indentation/blank lines (the hibernate block lost four nesting levels);
    stops/resumes: how many AB1805 watchdog calls the base block holds (ExternalWatchdog replaces them)."""
    if indent_free:
        block = "\n".join(l.strip() for l in block.splitlines() if l.strip()) + "\n"
    if is_base:
        if indent_free:
            for old, new in SUBSTITUTIONS:
                if block.count(old) != 1:
                    raise SystemExit(f"S2 substitution does not apply to the base exactly once: {old[:60]!r}")
                block = block.replace(old, new)
        for (old, new, _), count in zip(WATCHDOG, (stops, resumes)):
            if block.count(old) != count:
                raise SystemExit(f"S2 watchdog substitution expected {count}x in the base block: {old!r}")
            block = block.replace(old, new)
    return block
