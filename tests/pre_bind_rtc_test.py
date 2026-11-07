#!/usr/bin/env python3
"""v50 S1b: host test of src/time/PreBindRtc.h (real header, real Clock::isPlausibleEpoch body), structural
checks on setup(), and mutations. Each mutation is applied to a copy in a temp dir and must FAIL.
"""
import re, subprocess, sys, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src"
MAIN = SRC / "Generalized-Core-Counter.cpp"
HDR = SRC / "time" / "PreBindRtc.h"
CLOCK = SRC / "time" / "Clock.cpp"
TEST = ROOT / "tests" / "pre_bind_rtc_test.cpp"


def strip(code):
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.S)
    return re.sub(r"//[^\n]*", "", code)


def plausible_body():
    m = re.search(r"bool isPlausibleEpoch\(time_t epoch\) \{.*?\n\}\n", CLOCK.read_text(), re.S)
    assert m, "isPlausibleEpoch not found in Clock.cpp"
    return m.group(0)


def structural(main_text, hdr_text):
    errs = []
    m, h = strip(main_text), strip(hdr_text)
    call = re.search(r"if \(!Am18x5\.status\(\)\.bound\(\)[^\n]*PreBindRtc::run\(ab1805, Wire\)[^\n]*", m)
    if not call:
        errs.append("setup() does not gate PreBindRtc::run(ab1805, Wire) on !Am18x5.status().bound()")
    elif m.count("PreBindRtc::run") != 1 or not (m.index("PreBindRtc::run") < m.index("ExternalRtc::ensureBound(Am18x5")):
        errs.append("PreBindRtc::run must be called once, before ExternalRtc::ensureBound")
    elif "reset" in call.group(0).lower():
        errs.append("a pre-bind failure must not reset")
    if not re.search(r"\.enable\(", m) and "ensureBound" not in m:
        errs.append("enable path missing")
    if "std::lock_guard<W> hold(wire);" not in h or h.index("lock_guard") > h.index("getRtcAsTime") or h.index("lock_guard") > h.index("readRegister") or h.index("lock_guard") > h.index("writeRegister"):
        errs.append("Wire lock is not held across the time read and the CONTROL1 read-modify-write")
    if "wire.begin()" not in h or "!wire.isEnabled()" not in h:
        errs.append("Wire.begin() when not enabled is missing")
    if "exit(" in h or "reset(" in h:
        errs.append("header must not reset")
    if "TwoWire" in h or "0x69" in h:
        errs.append("header must use the library's address")
    return errs


def build_and_run(hdr_text):
    with tempfile.TemporaryDirectory() as d:
        (Path(d) / "time").mkdir()
        (Path(d) / "time" / "PreBindRtc.h").write_text(hdr_text)
        (Path(d) / "plausible.inc").write_text(plausible_body())
        exe = Path(d) / "t"
        r = subprocess.run(["c++", "-std=c++17", "-Wall", "-Wextra", "-I", d, f'-DCLOCK_PLAUSIBLE_INC="{d}/plausible.inc"', str(TEST), "-o", str(exe)],
                           capture_output=True, text=True)
        if r.returncode:
            return False, r.stderr
        r = subprocess.run([str(exe)], capture_output=True, text=True)
        return r.returncode == 0, r.stdout


def main():
    hdr, main_text = HDR.read_text(), MAIN.read_text()
    errs = structural(main_text, hdr)
    if errs:
        print("FAIL:", *errs, sep="\n  ")
        sys.exit(1)
    print("OK: structural checks")
    ok, out = build_and_run(hdr)
    if not ok:
        print("FAIL: host test\n" + out)
        sys.exit(1)
    print("OK: host test")

    def rep(a, b, t=hdr):
        assert a in t, a
        return t.replace(a, b, 1)

    hmut = {
        "wrtc-write-clears-other-bits": rep("(c1 | 0x01) & ~0x40", "0x01"),
        "seed-when-time-valid": rep(" && !Time.isValid()", ""),
        "skip-plausibility": rep(" && Clock::isPlausibleEpoch(chip)", ""),
        "wrong-register": rep("readRegister(0x10", "readRegister(0x11"),
        "wrong-register-write": rep("rtc.writeRegister(0x10, (c1 | 0x01)", "rtc.writeRegister(0x11, (c1 | 0x01)"),
        "wrong-register-clear": rep("rtc.writeRegister(0x10, c1 & ~0x01)", "rtc.writeRegister(0x11, c1 & ~0x01)"),
        "reset-on-i2c-failure": rep("const bool read = rtc.readRegister(0x10, c1);", "const bool read = rtc.readRegister(0x10, c1); if (!read) std::exit(1);"),
        "1224-ignored": rep("(c1 | 0x01) & ~0x40", "(c1 | 0x01)"),
        "1224-not-cleared-for-set-bits": rep("(c1 | 0x01) & ~0x40", "(c1 | 0x41)"),
        "wire-lock-not-held": rep("std::lock_guard<W> hold(wire);", ""),
        "lock-released-before-rmw": rep("  uint8_t c1 = 0;", "  wire.unlock();\n  uint8_t c1 = 0;").replace("std::lock_guard<W> hold(wire);", "wire.lock();"),
        "no-wire-begin": rep("if (!wire.isEnabled()) wire.begin();", ""),
        "begin-always": rep("if (!wire.isEnabled()) wire.begin();", "wire.begin();"),
        "no-wrtc-write": rep("return (read && rtc.writeRegister(0x10, (c1 | 0x01) & ~0x40)) & ok & timeOk;", "return ok && timeOk;"),
        "wrtc-not-cleared-before-read": rep("const bool ok = read && (!(c1 & 0x01) || rtc.writeRegister(0x10, c1 & ~0x01));", "const bool ok = read;"),
        "final-write-skipped-on-time-read-failure": rep("return (read && rtc", "return (read && timeOk && rtc"),
        "saved-value-not-restored": rep("(c1 | 0x01) & ~0x40", "(0x01)"),
        "saved-value-not-normalised": rep("(c1 | 0x01) & ~0x40", "c1"),
        "clear-result-ignored": rep("const bool ok = read && (!(c1 & 0x01) || rtc.writeRegister(0x10, c1 & ~0x01));", "const bool ok = read; if (c1 & 0x01) rtc.writeRegister(0x10, c1 & ~0x01);"),
        "final-write-result-ignored": rep("return (read && rtc.writeRegister(0x10, (c1 | 0x01) & ~0x40)) & ok & timeOk;", "if (read) rtc.writeRegister(0x10, (c1 | 0x01) & ~0x40); return ok && timeOk;"),
        "control-read-result-ignored": rep("const bool read = rtc.readRegister(0x10, c1);", "const bool read = (rtc.readRegister(0x10, c1), true);"),
        "time-read-result-ignored": rep("& ok & timeOk;", "& ok;"),
    }
    bad = 0
    for name, text in hmut.items():
        ok, _ = build_and_run(text)
        print(("OK" if not ok else "FAIL"), f"mutation {name} is caught")
        bad += ok
    ens = "ExternalRtc::ensureBound(Am18x5"
    call = "PreBindRtc::run(ab1805, Wire)"
    smut = {
        "run-on-bound-boot": rep("!Am18x5.status().bound() && ", "", main_text),
        "reset-on-failure": rep('Log.warn("PreBindRtc: chip access failed")', "System.reset()", main_text),
        "never-run": rep(call, "true", main_text),
    }
    # run-after-enable: move the call below ensureBound
    line = re.search(r"[^\n]*PreBindRtc::run\(ab1805, Wire\)[^\n]*\n", main_text).group(0)
    moved = main_text.replace(line, "", 1)
    i = moved.index(ens)
    j = moved.index("\n", moved.index(");", i)) + 1
    smut["run-after-enable"] = moved[:j] + line + moved[j:]
    for name, text in smut.items():
        caught = bool(structural(text, hdr))
        print(("OK" if caught else "FAIL"), f"mutation {name} is caught")
        bad += not caught
    if bad:
        sys.exit(1)
    print("pre_bind_rtc_test: all checks passed")


main()
