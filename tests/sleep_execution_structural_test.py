#!/usr/bin/env python3
"""
WO-2026-10-21-001 (Sleep S-2) - structural invariants of the sleep-execution refactor, checked on
the REAL src/state/State_Sleep.cpp (comments stripped before every textual check).

  1. The three standby-return fields are declared once, as members of one struct in the anonymous
     namespace; the old handler-local names are gone from State_Sleep.cpp.
  2. Only effectiveStandby(), sleepAndResume() and the SleepTeardown::run(...) call arguments
     mention `standbyReturn`.
  3. Every SystemSleepConfiguration in src/ is still a fresh local.
  4. (needs the base commit) SleepTeardown.{h,cpp} are unchanged, no file was added under src/, no
     line mentioning `retained` was added anywhere in src/, and the moved blocks are byte-identical to
     the base apart from the listed edits (field renames, `return;` -> `return true;`, the added
     `wake = {...};` and `return false;`).
The base commit is the one recorded in the equivalence fixture header. Without history (a scratch
copy) the checks of group 4 are skipped and said so.
"""
import difflib
import os
import re
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(os.environ.get("SLEEP_EXEC_STRUCT_ROOT", Path(__file__).resolve().parent.parent))
SRC = REPO_ROOT / "src"
SLEEP = SRC / "state" / "State_Sleep.cpp"
FIXTURE = Path(__file__).resolve().parent / "fixtures" / "sleep_execution_transcript.txt"
OLD_NAMES = ("preserveStandbyAfterNormalReturn", "forceModemOffAfterStandbyFailure", "standbySleepFailureError")
RENAMES = (("standbyReturn.preserveAfterNormalReturn", "preserveStandbyAfterNormalReturn"),
           ("standbyReturn.forceModemOff", "forceModemOffAfterStandbyFailure"),
           ("standbyReturn.error", "standbySleepFailureError"))


def fail(msg):
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def strip_comments(code):
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)
    return re.sub(r"//[^\n]*", "", code)


def git(*args):
    return subprocess.run(["git", "-C", str(REPO_ROOT), *args], capture_output=True, text=True)


def function_span(lines, signature):
    """(start, end) line indexes of the col-0 function whose first line starts with `signature`."""
    starts = [i for i, l in enumerate(lines) if l.startswith(signature)]
    if len(starts) != 1:
        fail(f"expected exactly one definition of {signature!r}, found {len(starts)}")
    for j in range(starts[0] + 1, len(lines)):
        if lines[j] == "}":
            return starts[0], j
    fail(f"no closing brace for {signature!r}")


raw = SLEEP.read_text()
code = strip_comments(raw)
code_lines = code.split("\n")

# ---- 1. one declaration, in one anonymous-namespace struct ----
decl = re.findall(r"struct\s*\{[^{}]*\}\s*standbyReturn\s*;", code)
if len(decl) != 1:
    fail(f"standbyReturn must be declared once as an unnamed struct, found {len(decl)} declarations")
for field in ("preserveAfterNormalReturn", "forceModemOff", "error"):
    if len(re.findall(r"\b%s\b\s*=\s*[^;]*;\s*(?=[^{}]*\}\s*standbyReturn)" % field, decl[0])) != 1:
        fail(f"standbyReturn must declare {field} exactly once")
ns_open = raw.find("namespace {")
ns_close = raw.find("} // namespace")
decl_pos = raw.find(decl[0])
if not (0 <= ns_open < decl_pos < ns_close):
    fail("standbyReturn must be declared inside the anonymous namespace")
if any(n in code for n in OLD_NAMES):
    fail("State_Sleep.cpp still uses an old handler-local standby-return name")
print("PASS: the three standby-return fields are declared once, in one anonymous-namespace struct")

# ---- 2. only the two functions and the teardown arguments mention it ----
allowed = set()
for sig in ("bool effectiveStandby(", "bool sleepAndResume("):
    s, e = function_span(code_lines, sig)
    allowed.update(range(s, e + 1))
decl_line = code[:code.find(decl[0])].count("\n")
allowed.update(range(decl_line, decl_line + decl[0].count("\n") + 1))
teardown_args = ["SleepTeardown::run(useNetworkStandbyRequested, useNetworkStandbyEffective, "
                 "standbyReturn.forceModemOff, standbyReturn.error)"]
mentions = [i for i, l in enumerate(code_lines) if "standbyReturn" in l]
for i in mentions:
    if i in allowed:
        continue
    if teardown_args[0] in code_lines[i]:
        continue
    fail(f"State_Sleep.cpp line {i + 1} mentions standbyReturn outside effectiveStandby(), sleepAndResume() "
         f"and the teardown call: {code_lines[i].strip()}")
if sum(teardown_args[0] in l for l in code_lines) != 1:
    fail("the SleepTeardown::run(...) call must pass standbyReturn.forceModemOff and standbyReturn.error, once")
for path in sorted(SRC.rglob("*")):
    if path != SLEEP and path.suffix in (".cpp", ".h") and "standbyReturn" in strip_comments(path.read_text()):
        fail(f"{path.relative_to(REPO_ROOT)} mentions standbyReturn")
print("PASS: only effectiveStandby(), sleepAndResume() and the teardown call arguments mention standbyReturn")

# ---- 3. every SystemSleepConfiguration is a fresh local ----
fresh = 0
for path in sorted(SRC.rglob("*")):
    if path.suffix not in (".cpp", ".h"):
        continue
    text = strip_comments(path.read_text())
    for m in re.finditer(r"[^\n]*\bSystemSleepConfiguration\b[^\n]*", text):
        if not re.fullmatch(r"\s+SystemSleepConfiguration\s+\w+;", m.group(0)):
            fail(f"{path.relative_to(REPO_ROOT)}: SystemSleepConfiguration must be a fresh local, found: {m.group(0).strip()}")
        depth = text[:m.start()].count("{") - text[:m.start()].count("}")
        if depth < 2:
            fail(f"{path.relative_to(REPO_ROOT)}: SystemSleepConfiguration declared outside a function body")
        fresh += 1
if fresh != 5:
    fail(f"expected 5 fresh SystemSleepConfiguration locals (hibernate, ULP, STOP, STOP timer-only, boot storm), found {fresh}")
print("PASS: all 5 SystemSleepConfiguration objects are fresh locals")

# ---- 4. against the base commit ----
base = None
if FIXTURE.is_file():
    m = re.search(r"^# Base commit: (\S+)$", FIXTURE.read_text(), re.M)
    base = m.group(1) if m else None
base_text = git("show", f"{base}:src/state/State_Sleep.cpp").stdout if base else ""
if not base_text:
    print(f"SKIP: base commit {base} is not available here; base comparisons not run")
    print("sleep_execution_structural_test: all available invariants hold")
    sys.exit(0)

changed = git("diff", "--name-only", base, "--", "src/state/SleepTeardown.h", "src/state/SleepTeardown.cpp").stdout.split()
if changed:
    fail(f"SleepTeardown.{{h,cpp}} must be unchanged against {base}: {changed}")
added = git("diff", "--name-status", "--diff-filter=A", base, "--", "src").stdout.split()
added += git("ls-files", "--others", "--exclude-standard", "--", "src").stdout.split()
if added:
    fail(f"no new file may appear under src/: {added}")
for name in git("diff", "--name-only", base, "--", "src").stdout.split():
    before = strip_comments(git("show", f"{base}:{name}").stdout)
    after = strip_comments((REPO_ROOT / name).read_text())
    for pattern in (r"retained", r"\.backup", r"__attribute__\s*\(\s*\(\s*section"):
        if len(re.findall(pattern, after, re.I)) > len(re.findall(pattern, before, re.I)):
            fail(f"{name}: code mentioning {pattern!r} was added (the standby-return fields must not be retained)")
print(f"PASS: SleepTeardown.{{h,cpp}} unchanged, no new file under src/, nothing retained added (base {base})")

base_lines = base_text.split("\n")


def between(lines, first, last):
    return "\n".join(lines[first:last + 1]) + "\n"


def base_block(start_marker, end_marker):
    i = base_text.find(start_marker)
    j = base_text.find(end_marker, i + len(start_marker))
    if base_text.count(start_marker) != 1 or i < 0 or j < 0:
        fail(f"base marker {start_marker!r} is not unique/present")
    return base_text[i:j + len(end_marker)]


raw_lines = raw.split("\n")
checks = []
s, e = function_span(raw_lines, "bool effectiveStandby(")
body = between(raw_lines, s + 1, e - 1)
if not body.endswith("  return useNetworkStandbyEffective;\n"):
    fail("effectiveStandby() must end with `return useNetworkStandbyEffective;`")
checks.append(("effectiveStandby", body[:-len("  return useNetworkStandbyEffective;\n")],
               base_block("  bool preserveStandbyForThisPass = false;\n", "  useNetworkStandbyEffective = false;\n#endif\n")))
s, e = function_span(raw_lines, "void hibernateAttempt(")
checks.append(("hibernateAttempt", between(raw_lines, s + 1, e - 1),
               base_block("#if PLATFORM_ID == PLATFORM_BORON\n  if (overnightFallbackSleep) {\n    time_t rtcNow", "\n  }\n#endif\n")))
s, e = function_span(raw_lines, "bool sleepAndResume(")
body = between(raw_lines, s + 1, e - 1)
for needle, repl in (("  wake = {pirWake, buttonWake, timerWake, wakeReasonLabel};\n", ""), ("  return false;\n", ""),
                     ("        return true;\n", "        return;\n")):
    if body.count(needle) != 1:
        fail(f"sleepAndResume() must contain {needle.strip()!r} exactly once, found {body.count(needle)}")
    body = body.replace(needle, repl)
checks.append(("sleepAndResume", body,
               base_block("  const SensorWakeSource sensorWake = SensorManager", '  PowerDiagnostics::logPowerState("post-wake");\n')))
for name, current, original in checks:
    for new, old in RENAMES:
        current = current.replace(new, old)
    if current != original:
        diff = "".join(list(difflib.unified_diff(original.splitlines(True), current.splitlines(True), "base", "current"))[:40])
        fail(f"{name}() differs from the base block beyond the listed edits:\n{diff}")
drain = lambda t: t[t.find("void drainSerialBeforeSleep() {"):t.find("\n}\n", t.find("void drainSerialBeforeSleep() {")) + 3]
if drain(raw) != drain(base_text):
    fail("drainSerialBeforeSleep() changed")
print("PASS: effectiveStandby(), hibernateAttempt() and sleepAndResume() are byte-identical to the base blocks "
      "apart from the field renames, `return true;` and the added `wake = {...};` / `return false;`")
print("sleep_execution_structural_test: all invariants hold")
