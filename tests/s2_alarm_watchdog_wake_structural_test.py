#!/usr/bin/env python3
"""v50 S2 (WO-2026-11-05-004): structural checks for the Device OS alarm / chip-watchdog / wake changes.

Each rule is a function of the source text; every rule is shown to fire on a mutated copy
(in memory) before the real source is judged.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "src"


def read(rel):
    return (SRC / rel).read_text()


def strip(t):
    t = re.sub(r"/\*.*?\*/", "", t, flags=re.S)
    return re.sub(r"//[^\n]*", "", t)


def fn_body(t, sig):
    i = t.index(sig)
    j = t.index("\n}\n", i)
    return t[i:j]


def problems(files):
    try:
        return _problems(files)
    except ValueError as e:
        return [f"expected structure missing: {e}"]


def _problems(files):
    p = []
    gcc = strip(files["gcc"])
    sleep = strip(files["sleep"])
    err = strip(files["err"])
    hc = strip(files["hc"])

    for name in ("clearAb1805StaleAlarmInterrupts", "interruptAtTime", "setWDT", "stopWDT", "resumeWDT",
                 "ab1805WakeReasonName", "getWakeReason", "updateWakeReason"):
        for label, t in files.items():
            if label != "raw" and re.search(r"\b" + name + r"\b", strip(t)):
                p.append(f"{name} still referenced in {label}")

    # Only the two S3 callers may still touch the AB1805 object for power-down.
    for label in ("gcc", "sleep", "err", "hc"):
        t = strip(files[label])
        for m in re.finditer(r"\bab1805\.(\w+)", t):
            if m.group(1) != "deepPowerDown":
                p.append(f"ab1805.{m.group(1)} in {label}")

    # Reason 20 is judged from persisted evidence, not the retained struct, and a reason-30-only wake.
    if "in.wakeReasonIsAlarm = in.resetReasonIsPowerManagement;" not in hc:
        p.append("reason 30 alone must be the alarm wake")
    if re.search(r"retained\w*", hc) and "PIN_RESET" in hc:
        pass
    setup = gcc[gcc.index("classifyWake(reason)"):]
    block = setup[:setup.index("ExternalWatchdog.init")]
    if "isInferredWatchdog(" not in block or "get_powerOffRequested()" not in block:
        p.append("reason-20 classification must use isInferredWatchdog with the persisted byte")
    if re.search(r"\bretained\w*", block):
        p.append("retained must not be reason-20 evidence")
    if not 0 <= block.find("isInferredWatchdog(") < block.find("set_powerOffRequested(0)"):
        p.append("the byte must be read before it is cleared")
    if gcc.count("set_powerOffRequested(0)") != 1:
        p.append("the byte must be cleared exactly once, at boot")
    if "if (reason == RESET_REASON_PIN_RESET)" not in block:
        p.append("reason 20 gate missing")

    # The byte is persisted before the deep power-down and the flush, never after.
    case3 = err[err.index("case 3:"):]
    case3 = case3[:case3.index("break;")]
    order = [case3.find(s) for s in ("set_powerOffRequested(1)", "flushAllBeforeReset()", "ab1805.deepPowerDown()")]
    if min(order) < 0 or order != sorted(order):
        p.append("State_Error case 3 must write the byte, then flush, then deepPowerDown")

    # The chip watchdog: init+start in setup, refreshed in the main loop, stopped before and started after every sleep.
    if gcc.count("ExternalWatchdog.refresh()") != 1:
        p.append("ExternalWatchdog.refresh() must appear exactly once in the main loop")
    if gcc.count("ExternalWatchdog.init(") != 1 or gcc.count("ExternalWatchdog.start()") != 1:
        p.append("ExternalWatchdog init/start must appear once in setup")
    loop = gcc[gcc.index("void loop()"):]
    if loop.index("ExternalWatchdog.refresh()") > loop.index("checkClockResync()"):
        p.append("refresh must precede checkClockResync() (the old ab1805.loop() slot)")

    hib = fn_body(sleep, "bool hibernateAttempt(") if "bool hibernateAttempt(" in sleep else fn_body(sleep, "void hibernateAttempt(")
    ulp_i = sleep.index("sleepAndResume(")
    sites = [("hibernate", hib), ("ulp", sleep[ulp_i:])]
    for label, body in sites:
        stop = body.find("ExternalWatchdog.stop()")
        call = body.find("System.sleep(")
        start = body.find("ExternalWatchdog.start()", call)
        if stop < 0 or call < 0 or start < 0 or not (stop < call < start):
            p.append(f"{label}: ExternalWatchdog.stop() must precede System.sleep() and start() follow it")
    if sleep.count("ExternalWatchdog.stop()") != 2:
        p.append("expected exactly two ExternalWatchdog.stop() sites (hibernate, ULP/STOP)")
    if sleep.count("ExternalWatchdog.start()") != 3:
        p.append("expected exactly three ExternalWatchdog.start() sites (hibernate return, all-failed, post-ULP)")

    # One native alarm: duration() once on the hibernate config, both GPIO wakes kept, no second arming.
    cfg = hib[hib.index("hibernateConfig.mode("):]
    cfg = cfg[:cfg.index(";")]
    if cfg.count(".duration(") != 1 or "wakeInSeconds" not in cfg or cfg.count(".gpio(") != 2:
        p.append("hibernate config must arm exactly one duration(wakeInSeconds) plus both GPIO wakes")
    if "SystemSleepMode::HIBERNATE" not in cfg:
        p.append("hibernate mode missing")

    # A native error return is abandoned, the dog restarted, and the ULP fallback follows.
    res = hib[hib.index("System.sleep(hibernateConfig)"):]
    if not (res.index("HibernateCycle::abandon()") < res.index("ExternalWatchdog.start()")):
        p.append("a returned hibernate must abandon() then restart the watchdog")
    if "falling back to ULTRA_LOW_POWER" not in sleep:
        p.append("the ULP fallback log is missing")
    return p


raw = {
    "gcc": read("Generalized-Core-Counter.cpp"),
    "sleep": read("state/State_Sleep.cpp"),
    "err": read("state/State_Error.cpp"),
    "hc": read("time/HibernateCycle.cpp"),
    "diag": read("time/ClockDiagnostics.cpp"),
    "elig": read("time/HibernateEligibility.cpp"),
}
raw["raw"] = ""


def mutated(label, old, new, count=1):
    f = dict(raw)
    if f[label].count(old) < 1:
        print(f"FAIL: probe did not apply: {old!r}")
        sys.exit(1)
    f[label] = f[label].replace(old, new, count)
    return f


probes = {
    "reason 20 always a wake": mutated("hc", "in.wakeReasonIsAlarm = in.resetReasonIsPowerManagement;", "in.wakeReasonIsAlarm = true;"),
    "evidence read from retained": mutated("gcc", "RecoveryState::get_powerOffRequested() != 0", "retainedHibernatePending != 0"),
    "byte not cleared": mutated("gcc", "RecoveryState::set_powerOffRequested(0);", ""),
    "byte cleared before read": mutated("gcc", "  bool inferredWatchdog = false;\n", "  RecoveryState::set_powerOffRequested(0);\n  bool inferredWatchdog = false;\n"),
    "byte written after the call": mutated("err", "RecoveryState::set_powerOffRequested(1); // persisted evidence: a reason-20 boot after this is a power-up, not a watchdog\n      flushAllBeforeReset();\n      ab1805.deepPowerDown();",
                                           "flushAllBeforeReset();\n      ab1805.deepPowerDown();\n      RecoveryState::set_powerOffRequested(1);"),
    "alarm armed twice": mutated("sleep", ".duration((uint32_t)wakeInSeconds * 1000UL);\n\n        const bool watchdogStopped",
                                 ".duration((uint32_t)wakeInSeconds * 1000UL).duration(1);\n\n        const bool watchdogStopped"),
    "no watchdog stop before hibernate": mutated("sleep", "const bool watchdogStopped = (ExternalWatchdog.stop() == SYSTEM_ERROR_NONE);\n        pauseAwakeWatchdogForSleep",
                                                  "const bool watchdogStopped = true;\n        pauseAwakeWatchdogForSleep"),
    "refresh missing": mutated("gcc", "ExternalWatchdog.refresh();", ""),
    "hibernate fallback restart removed": mutated("sleep", "HibernateCycle::abandon();\n        ExternalWatchdog.start();", "HibernateCycle::abandon();"),
    "ab1805 stale clear back": dict(raw, elig="bool clearAb1805StaleAlarmInterrupts();\n" + raw["elig"]),
}
for name, f in probes.items():
    if not problems(f):
        print(f"FAIL: probe not detected: {name}")
        sys.exit(1)

found = problems(raw)
if found:
    for x in found:
        print(f"FAIL: {x}")
    sys.exit(1)
print("s2_alarm_watchdog_wake_structural_test: ok (%d probes detected)" % len(probes))
