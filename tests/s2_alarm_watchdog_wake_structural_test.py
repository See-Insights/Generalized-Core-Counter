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
    fs = strip(files["fs"])
    hc = strip(files["hc"])

    for name in ("clearAb1805StaleAlarmInterrupts", "interruptAtTime", "setWDT", "stopWDT", "resumeWDT",
                 "ab1805WakeReasonName", "getWakeReason", "updateWakeReason"):
        for label, t in files.items():
            if label != "raw" and re.search(r"\b" + name + r"\b", strip(t)):
                p.append(f"{name} still referenced in {label}")

    # S3: no caller touches the AB1805 object any more.
    for label in ("gcc", "sleep", "err", "hc", "fs"):
        t = strip(files[label])
        for m in re.finditer(r"\bab1805\.(\w+)", t):
            p.append(f"ab1805.{m.group(1)} in {label}")

    # Reason 20 is judged from persisted evidence, not the retained struct, and a reason-30-only wake.
    if "in.wakeReasonIsHibernateWake = in.resetReasonIsPowerManagement;" not in hc:
        p.append("reason 30 alone must be the successful-HIBERNATE-wake arm")
    if re.search(r"retained\w*", hc) and "PIN_RESET" in hc:
        pass
    setup = gcc[gcc.index("classifyWake(reason)"):]
    block = setup[:setup.index("SystemConfig::flushNow();") + 25]
    if "isInferredWatchdog(" not in block or "get_powerOffRequested()" not in block:
        p.append("reason-20 classification must use isInferredWatchdog with the persisted byte")
    if re.search(r"\bretained\w*", block):
        p.append("retained must not be reason-20 evidence")
    if not 0 <= block.find("isInferredWatchdog(") < block.find("set_powerOffRequested(0)"):
        p.append("the byte must be read before it is cleared")
    if gcc.count("set_powerOffRequested(0)") != 1:
        p.append("the byte must be cleared exactly once, at boot")
    if not re.search(r"set_powerOffRequested\(0\);\s*SystemConfig::flushNow\(\);", gcc):
        p.append("the boot clear must be flushed immediately (forced consumption)")
    if 'ab1805WakeReason\\":\\"N/A\\"' not in files["gcc"]:
        p.append("the status payload must keep ab1805WakeReason with the constant N/A")
    if "if (reason == RESET_REASON_PIN_RESET)" not in block:
        p.append("reason 20 gate missing")

    # The byte is persisted before the deep power-down and the flush, never after.
    case3 = err[err.index("case 3:"):]
    case3 = case3[:case3.index("break;")]
    order = [case3.find(s) for s in ("set_powerOffRequested(HibernateWakeDiagnostics::kPowerOffSentinel)", "flushAllBeforeReset()", "powerOffForRecovery()")]
    if min(order) < 0 or order != sorted(order):
        p.append("State_Error case 3 must write the sentinel, then flush, then power off")
    if not re.search(r"powerOffForRecovery\(\);\s*RecoveryState::set_powerOffRequested\(0\);\s*SystemConfig::flushNow\(\);\s*System\.reset\(RESET_CAUSE_ERROR_STATE_SOFT\);", case3):
        p.append("State_Error case 3 must clear the byte and flush after a returning power-off")
    if not re.search(r"isInferredWatchdog\(RecoveryState::get_powerOffRequested\(\)\)", block):
        p.append("the reader must take the evidence byte only (no stage/time arguments)")
    s3 = fs[fs.index("persistConnectivityFailsafeState(3, now, true)"):]
    s3 = s3[:s3.index("System.reset(")]
    order = [s3.find(s) for s in ("set_powerOffRequested(HibernateWakeDiagnostics::kPowerOffSentinel)", "flushAllBeforeReset()", "powerOffForRecovery()")]
    if min(order) < 0 or order != sorted(order):
        p.append("failsafe stage 3 must write the sentinel, then flush, then power off")
    if not re.search(r"powerOffForRecovery\(\);\s*RecoveryState::set_powerOffRequested\(0\);\s*RecoveryState::set_connectivityRecoveryStage\(2\);\s*SystemConfig::flushNow\(\);\s*System\.reset\(RESET_CAUSE_CONNECTIVITY_FAILSAFE\);", fs):
        p.append("the failsafe caller must clear the byte, downgrade stage 3 and flush after a returning power-off")

    # The chip watchdog: init+start in setup, refreshed in the main loop, stopped before and started after every sleep.
    if gcc.count("ExternalWatchdog.refresh()") != 1:
        p.append("ExternalWatchdog.refresh() must appear exactly once in the main loop")
    setup_all = gcc[gcc.index("void setup()"):gcc.index("void loop()")]
    if gcc.count("ExternalWatchdog.init(") != 1 or gcc.count("ExternalWatchdog.start()") != 2:
        p.append("ExternalWatchdog.init once and start twice (setup init, boot-storm restart) expected")
    init_i = setup_all.find("ExternalWatchdog.init(")
    bind_i = setup_all.find("ExternalRtc::ensureBound(")
    storm_i = setup_all.find("System.sleep(bootStormSleep)")
    if not (0 <= bind_i < init_i < storm_i):
        p.append("ExternalWatchdog.init must follow the RTC bind and precede the boot-storm sleep")
    if not re.search(r"ExternalWatchdog\.stop\(\);\s*System\.sleep\(bootStormSleep\);\s*ExternalWatchdog\.start\(\);", setup_all):
        p.append("the boot-storm System.sleep must be bracketed by ExternalWatchdog.stop()/start()")
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
    "fs": read("power/ConnectivityFailsafe.cpp"),
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


def moved_init():
    init = "  // Device OS keeps the chip watchdog running across a reset; init() re-arms it.\n  if (ExternalWatchdog.init(WatchdogConfiguration().timeout(124000)) != SYSTEM_ERROR_NONE || ExternalWatchdog.start() != SYSTEM_ERROR_NONE) {\n    Log.warn(\"ExternalWatchdog: init/start failed\");\n  }\n"
    anchor = "  // Wait for serial connection only in explicit DEV builds\n"
    f = mutated("gcc", init, "")
    f["gcc"] = f["gcc"].replace(anchor, init + anchor, 1)
    return f


probes = {
    "reason 20 always a wake": mutated("hc", "in.wakeReasonIsHibernateWake = in.resetReasonIsPowerManagement;", "in.wakeReasonIsHibernateWake = true;"),
    "evidence read from retained": mutated("gcc", "isInferredWatchdog(RecoveryState::get_powerOffRequested())", "isInferredWatchdog(retainedHibernatePending)"),
    "stage-3 sentinel write removed": mutated("fs", "RecoveryState::set_powerOffRequested(HibernateWakeDiagnostics::kPowerOffSentinel);", ""),
    "reader takes stage again": mutated("gcc", "isInferredWatchdog(RecoveryState::get_powerOffRequested())", "isInferredWatchdog(RecoveryState::get_powerOffRequested(), 3)"),
    "storm sleep stop removed": mutated("gcc", "ExternalWatchdog.stop();\n    System.sleep(bootStormSleep);", "System.sleep(bootStormSleep);"),
    "storm sleep start removed": mutated("gcc", "System.sleep(bootStormSleep);\n    ExternalWatchdog.start();", "System.sleep(bootStormSleep);"),
    "init moved after the storm block": moved_init(),
    "duplicate init re-added": mutated("gcc", "  RecoveryState::set_powerOffRequested(0);\n", "  RecoveryState::set_powerOffRequested(0);\n  ExternalWatchdog.init(WatchdogConfiguration().timeout(124000));\n"),
    "byte not cleared": mutated("gcc", "RecoveryState::set_powerOffRequested(0);", ""),
    "byte cleared before read": mutated("gcc", "  bool inferredWatchdog = false;\n", "  RecoveryState::set_powerOffRequested(0);\n  bool inferredWatchdog = false;\n"),
    "byte written after the call": mutated("err", "RecoveryState::set_powerOffRequested(HibernateWakeDiagnostics::kPowerOffSentinel); // persisted evidence: a reason-20 boot after this is a power-up, not a watchdog\n      flushAllBeforeReset();\n      powerOffForRecovery();",
                                           "flushAllBeforeReset();\n      powerOffForRecovery();\n      RecoveryState::set_powerOffRequested(HibernateWakeDiagnostics::kPowerOffSentinel);"),
    "boot clear not flushed": mutated("gcc", "  SystemConfig::flushNow(); // consume the evidence durably before anything can reload it\n", ""),
    "no clear after a returning error-path power-down": mutated("err", "      RecoveryState::set_powerOffRequested(0); SystemConfig::flushNow();", ""),
    "failsafe stage 3 not downgraded on return": mutated("fs", "RecoveryState::set_connectivityRecoveryStage(2); SystemConfig::flushNow();", ""),
    "ab1805WakeReason dropped from the payload": mutated("gcc", '\\"ab1805WakeReason\\":\\"N/A\\",', ""),
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
