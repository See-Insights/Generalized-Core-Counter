#!/usr/bin/env python3
"""WO-2026-11-03-005: flushAllBeforeReset() at the deliberate-reset sites.

1. What it flushes: the REAL function body (src/state/State_Error.cpp) is compiled against a fake backend that
   records the flush events of the current store (C), sysStatus (S), sensor store (N) and the publish queue (Q).
   All four must fire, stores before the queue.
2. Where it is called: seven production sites (thrash tier 3, Error soft, Error hard, nightly heap guard,
   sleep exhausted, failsafe stage 2, failsafe stage 3), each immediately before its reset or power-down and after
   its own record writes. The OOM reset must not call it.
3. Mutations of private text copies: each must be detected.

Nothing under src/ is written; mutants live in memory or in a temp dir.
"""
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src"
CALL = "flushAllBeforeReset();"
FILES = {
    "error": "state/State_Error.cpp",
    "thrash": "ThrashGuard.cpp",
    "sleep": "state/State_Sleep.cpp",
    "failsafe": "power/ConnectivityFailsafe.cpp",
    "main": "Generalized-Core-Counter.cpp",
}

# (name, file key, terminator that ends the site, text that must come before the flush)
SITES = [
    ("thrash tier 3", "thrash", "System.reset(RESET_CAUSE_THRASH_TIER3);", "RecoveryState::raiseAlert(18);  // Alert 18: Severe thrash"),
    ("error soft", "error", "System.reset(RESET_CAUSE_ERROR_STATE_SOFT);", "Executing soft reset from ERROR_STATE"),
    ("error hard", "error", "ab1805.deepPowerDown();", "Executing deep power down from ERROR_STATE"),
    ("heap guard", "sleep", "System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);", "PowerDiagnostics::flushDiagBatch();"),
    ("sleep exhausted", "sleep", "System.reset(RESET_CAUSE_SLEEP_ATTEMPTS_EXHAUSTED);", "PowerDiagnostics::flushDiagBatch();"),
    ("failsafe stage 2", "failsafe", "System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);", "persistConnectivityFailsafeState(2, now, true);"),
    ("failsafe stage 3", "failsafe", "ab1805.deepPowerDown();\n  RecoveryState::set_powerOffRequested(0);", "persistConnectivityFailsafeState(3, now, true);"),
]


def load():
    return {k: (SRC / v).read_text() for k, v in FILES.items()}


def site_problems(texts):
    """Returns a list of problems; empty when every structural rule holds."""
    out = []
    for name, key, term, before in SITES:
        text = texts[key]
        if text.count(term) != 1:
            out.append(f"{name}: terminator {term!r} is not unique")
            continue
        at = text.index(term)
        head = text[:at].rstrip()
        if not head.endswith(CALL):
            out.append(f"{name}: {CALL} is not immediately before the reset/power-down")
            continue
        flush_at = len(head) - len(CALL)
        pos = text.rfind(before, 0, flush_at)
        if pos == -1 or flush_at - pos > 600:
            out.append(f"{name}: {before!r} must be written shortly before the flush")
    fs = texts["failsafe"]
    persist = re.search(r"^void persistConnectivityFailsafeState\(.*?^\}\n", fs, re.S | re.M)
    if not persist or "flushNow" in persist.group(0) or CALL in persist.group(0):
        out.append("failsafe: persistConnectivityFailsafeState() must only record; the flush follows it at the reset sites")
    oom = re.search(r"if \(outOfMemory >= 0\) \{.*?\n  \}\n", texts["main"], re.S)
    if not oom or "System.reset(RESET_CAUSE_OUT_OF_MEMORY);" not in oom.group(0):
        out.append("OOM block not found")
    elif "flushAllBeforeReset" in oom.group(0):
        out.append("OOM must not flush (immediate reset)")
    if CALL in texts["main"]:
        out.append("Generalized-Core-Counter.cpp must not call flushAllBeforeReset()")
    total = sum(t.count(CALL) for k, t in texts.items() if k != "main")
    if total != len(SITES):
        out.append(f"expected {len(SITES)} call sites, found {total}")
    return out


FAKES = r"""
#include <cassert>
#include <string>
#include <vector>
std::vector<std::string> ev;
namespace CurrentReadings { void flushNow() { ev.push_back("C"); } }
namespace SystemConfig { void flushNow() { ev.push_back("S"); }
  namespace SensorSettings { void flushNow() { ev.push_back("N"); } } }
struct FakeQueue { void writeQueueToFiles() { ev.push_back("Q"); } static FakeQueue &instance() { static FakeQueue q; return q; } };
using PublishQueuePosix = FakeQueue;
"""
DRIVER = r"""
int main() {
  flushAllBeforeReset();
  const std::vector<std::string> want = {@WANT@};
  assert(ev == want);
  return 0;
}
"""
FUNC_RE = re.compile(r"^void flushAllBeforeReset\(\) \{.*?^\}\n", re.S | re.M)


def function_body(text):
    m = FUNC_RE.search(text)
    return m.group(0) if m else None


def flush_run(body, tmp):
    """Compile the function body against the fakes; True when events are exactly C,S,N,Q in order."""
    if body is None:
        return False
    cpp = Path(tmp) / "flush.cpp"
    cpp.write_text(FAKES + body + DRIVER.replace("@WANT@", '"C", "S", "N", "Q"'))
    exe = Path(tmp) / "flush"
    built = subprocess.run(["clang++", "-std=c++17", "-Wall", "-Wextra", str(cpp), "-o", str(exe)], capture_output=True)
    if built.returncode != 0:
        return False
    return subprocess.run([str(exe)], capture_output=True).returncode == 0


failures = 0


def check(ok, msg):
    global failures
    print(("PASS: " if ok else "FAIL: ") + msg)
    if not ok:
        failures += 1


texts = load()
body = function_body(texts["error"])
check(body is not None, "flushAllBeforeReset() is defined in State_Error.cpp")
check(f"void flushAllBeforeReset();" in (SRC / "state/StateMachine.h").read_text(),
      "flushAllBeforeReset() is declared in state/StateMachine.h (seen by every reset site)")

with tempfile.TemporaryDirectory() as tmp:
    check(flush_run(body, tmp), "it flushes C, S, N, Q, in that order (stores before the queue)")
    flush_lines = {"C": "CurrentReadings::flushNow();", "S": "SystemConfig::flushNow();",
                   "N": "SystemConfig::SensorSettings::flushNow();", "Q": "PublishQueuePosix::instance().writeQueueToFiles();"}
    for k, line in flush_lines.items():
        mutant = body.replace("  " + line + "\n", "", 1)
        check(mutant != body and not flush_run(mutant, tmp), f"mutation caught: drop the {k} flush")
    lines = body.splitlines(keepends=True)
    q = next(i for i, l in enumerate(lines) if "writeQueueToFiles" in l)
    c = next(i for i, l in enumerate(lines) if "CurrentReadings::flushNow" in l)
    lines[q], lines[c] = lines[c], lines[q]
    check(not flush_run("".join(lines), tmp), "mutation caught: queue flushed before the stores")
    check("new " not in body and "String" not in body and "malloc" not in body, "the body allocates nothing")

check(not site_problems(texts), "all seven sites call flushAllBeforeReset() immediately before the reset, after their record writes")
for p in site_problems(texts):
    print("      " + p)
check("flushAllBeforeReset" not in re.search(r"if \(outOfMemory >= 0\) \{.*?\n  \}\n", texts["main"], re.S).group(0),
      "OOM structural exclusion: the out-of-memory reset does not flush")

# Mutations of the call sites.
for name, key, term, before in SITES:
    m = dict(texts)
    t = m[key]
    at = t.index(term)
    head = t[:at].rstrip()
    start = len(head) - len(CALL)
    m[key] = t[:start] + t[at:]
    check(bool(site_problems(m)), f"mutation caught: call removed at {name}")

# Add a call at the OOM site.
m = dict(texts)
m["main"] = m["main"].replace("    System.reset(RESET_CAUSE_OUT_OF_MEMORY);", "    flushAllBeforeReset();\n    System.reset(RESET_CAUSE_OUT_OF_MEMORY);", 1)
check(m["main"] != texts["main"] and bool(site_problems(m)), "mutation caught: flush added at the OOM site")

# Failsafe: the flush before the stage record (record moved after the flush).
for stage in (2, 3):
    rec = f"persistConnectivityFailsafeState({stage}, now, true);"
    m = dict(texts)
    fs = m["failsafe"]
    fs = fs.replace(rec + "\n", "", 1)
    fs = fs.replace(f"  {CALL}\n  ab1805.deepPowerDown();", f"  {CALL}\n  {rec}\n  ab1805.deepPowerDown();", 1) if stage == 3 else \
        fs.replace(f"    {CALL}\n    System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);", f"    {CALL}\n    {rec}\n    System.reset(RESET_CAUSE_CONNECTIVITY_FAILSAFE);", 1)
    check(m["failsafe"] != fs and bool(site_problems({**m, "failsafe": fs})), f"mutation caught: failsafe stage {stage} flushes before its record")

# The persist helper regaining its own flush (flush-then-record order for stage 2/3 would follow).
fs = texts["failsafe"].replace("    }\n  }\n}\n\nconst char *failsafeDeferReasonNameLocal",
                               "    }\n  }\n  SystemConfig::flushNow();\n}\n\nconst char *failsafeDeferReasonNameLocal", 1)
check(fs != texts["failsafe"] and bool(site_problems({**texts, "failsafe": fs})), "mutation caught: persistConnectivityFailsafeState() flushes on its own again")

if failures:
    print(f"\nFAILED: {failures} check(s)")
    sys.exit(1)
print("\ndeliberate_reset_flush_test: ok")
