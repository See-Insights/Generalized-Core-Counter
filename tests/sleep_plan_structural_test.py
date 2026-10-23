#!/usr/bin/env python3
"""WO-2026-10-21-003 (Sleep S-4): the sleep plan and the nightly heap guard are two file-local functions in
the anonymous namespace of State_Sleep.cpp (sleepPlan() and nightlyHeapGuard(), with struct SleepPlan),
placed after cloudGatePass() and before struct WakeContext. The handler calls plan -> hibernateAttempt ->
guard, in that order, once each. The plan has no function statics. No new src/ file, SleepTeardown.*, the S-2
functions, cloudGatePass() and the includes unchanged, no `retained` added, and the moved blocks are
byte-identical to the base apart from `return true;`, the final `plan = ...;`/`return false;` and the
handler's call and unpack lines. Each check is first shown to fire on a probe that breaks it. The base is the
commit in the transcript fixture header; without it in the tree the base comparisons are skipped and said."""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "src"
SLEEP = SRC / "state" / "State_Sleep.cpp"
FIXTURE = ROOT / "tests" / "fixtures" / "sleep_plan_transcript.txt"
PLAN_SIG = "bool sleepPlan(SleepPlan &plan) {"
GUARD_SIG = "bool nightlyHeapGuard(bool overnightFallbackSleep) {"
STRUCT = "struct SleepPlan { int wakeInSeconds; const char *sleepReason; bool overnightFallbackSleep; };\n"
PLAN_CALL = ("  SleepPlan plan;\n  if (sleepPlan(plan)) return;\n  int wakeInSeconds = plan.wakeInSeconds;\n"
             "  const char *sleepReason = plan.sleepReason;\n  const bool overnightFallbackSleep = plan.overnightFallbackSleep;\n")
HIB = "  hibernateAttempt(overnightFallbackSleep, wakeInSeconds);\n"
GUARD_CALL = "  if (nightlyHeapGuard(overnightFallbackSleep)) return;\n"
PLAN_TAIL = "  plan = {wakeInSeconds, sleepReason, overnightFallbackSleep};\n  return false;\n"
PLAN_START = "  // WO-2026-09-19 Step 3b: the behavioral core of this step."
GUARD_START = "  // Before a long overnight\n"
END = "  // The sleep configuration is built fresh"


def fail(msg):
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def strip(code):
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.S)
    code = re.sub(r"//[^\n]*", "", code)
    return re.sub(r'"(?:\\.|[^"\\\n])*"', '""', code)


def close_of(text, open_idx):
    depth = 0
    for i in range(open_idx, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return i
    return -1


def body_of(text, header):
    i = text.find(header)
    if i < 0 or text.count(header) != 1:
        return None
    o = text.find("{", i)
    return text[o + 1:close_of(text, o)]


def git_show(commit, path):
    r = subprocess.run(["git", "-C", str(ROOT), "show", f"{commit}:{path}"], capture_output=True, text=True)
    return r.stdout if r.returncode == 0 else None


def problems_in(code):
    """Structure problems of one State_Sleep.cpp text (base-independent)."""
    out = []
    plain = strip(code)
    ns_end = code.find("} // namespace\n")
    wake = code.find("struct WakeContext {")
    cloud = code.find("bool cloudGatePass(")
    pos = {name: code.find(sig) for name, sig in (("struct", STRUCT), ("plan", PLAN_SIG), ("guard", GUARD_SIG))}
    for name, sig in (("struct SleepPlan", STRUCT), ("sleepPlan()", PLAN_SIG), ("nightlyHeapGuard()", GUARD_SIG)):
        if code.count(sig) != 1:
            return [f"{name} must be defined exactly once as `{sig.strip()}`"]
    if not (0 <= cloud < pos["struct"] < pos["plan"] < pos["guard"] < wake < ns_end):
        out.append("struct SleepPlan, sleepPlan() and nightlyHeapGuard() must sit in the anonymous namespace, "
                   "after cloudGatePass() and before struct WakeContext, in that order")
    plan = body_of(plain, PLAN_SIG)
    guard = body_of(plain, GUARD_SIG)
    handler = body_of(plain, "void handleSleepingState()")
    if plan is None or guard is None or handler is None:
        return out + ["sleepPlan(), nightlyHeapGuard() and handleSleepingState() must each be defined once"]
    if re.search(r"\bstatic\b", plan) or re.search(r"\bstatic\b", guard):
        out.append("sleepPlan() and nightlyHeapGuard() must have no function statics")
    # The handler's order: plan, hibernateAttempt, guard; each exactly once.
    calls = [("sleepPlan(", handler.count("sleepPlan(")), ("hibernateAttempt(", handler.count("hibernateAttempt(")),
             ("nightlyHeapGuard(", handler.count("nightlyHeapGuard("))]
    if any(n != 1 for _, n in calls):
        out.append(f"the handler must call sleepPlan, hibernateAttempt and nightlyHeapGuard once each; found {calls}")
    else:
        i, j, k = (handler.index(c) for c, _ in calls)
        if not i < j < k:
            out.append("the handler's call order must be sleepPlan -> hibernateAttempt -> nightlyHeapGuard")
    if plain.count("sleepPlan(") != 2 or plain.count("nightlyHeapGuard(") != 2:
        out.append("sleepPlan() and nightlyHeapGuard() are each defined once and called once, from the handler only")
    if not re.search(r"if \(sleepPlan\(plan\)\) return;", handler):
        out.append("the handler must call `if (sleepPlan(plan)) return;`")
    if not re.search(r"if \(nightlyHeapGuard\(overnightFallbackSleep\)\) return;", handler):
        out.append("the handler must call `if (nightlyHeapGuard(overnightFallbackSleep)) return;`")
    # The plan's outputs are not file-scope state.
    ns_text = plain[:plain.find("struct WakeContext {")]
    if re.search(r"^(?:int|bool|const char \*)\s+(?:wakeInSeconds|sleepReason|overnightFallbackSleep|nightSleepSec)\b", ns_text, re.M):
        out.append("the plan's outputs must not be file-scope variables")
    return out


def normalized(code):
    """The current plan and guard bodies and the handler, mapped back to the base's inline text."""
    plan = body_of(code, PLAN_SIG).lstrip("\n")
    guard = body_of(code, GUARD_SIG).lstrip("\n")
    if not plan.endswith(PLAN_TAIL) or not guard.endswith("  return false;\n"):
        return None, None, None
    plan = plan[:-len(PLAN_TAIL)].replace("return true;", "return;")
    guard = guard[:-len("  return false;\n")].replace("return true;", "return;")
    handler = body_of(code, "void handleSleepingState()")
    if handler.count(PLAN_CALL) != 1 or handler.count(GUARD_CALL) != 1:
        return None, None, None
    return plan, guard, handler.replace(PLAN_CALL, "@@PLAN@@").replace(GUARD_CALL, "@@GUARD@@")


def base_pieces(base_text):
    handler = body_of(base_text, "void handleSleepingState()")
    a, b, e = handler.index(PLAN_START), handler.index(GUARD_START), handler.index(END)
    plan = handler[a:handler.index(HIB)]
    guard = handler[b:e]
    return plan.rstrip("\n") + "\n", guard.rstrip("\n") + "\n", handler[:a] + "@@PLAN@@\n" + handler[handler.index(HIB):b] + "@@GUARD@@\n" + handler[e:]


# ---- probes: every check fires on a text that breaks it -------------------------------------------------
sleep = SLEEP.read_text()
probes = {
    "the guard call before the hibernate attempt": sleep.replace(HIB + "\n" + GUARD_CALL, GUARD_CALL + "\n" + HIB, 1),
    "a second plan call": sleep.replace(HIB, "  { SleepPlan p2; sleepPlan(p2); }\n" + HIB, 1),
    "a function static in the plan": sleep.replace("  int nightSleepSec = -1;\n  const Clock::Openness parkOpenness", "  static int nightSleepSec = -1;\n  const Clock::Openness parkOpenness", 1),
    "the plan moved behind WakeContext": sleep.replace("struct WakeContext {", "struct WakeContext0 {", 1).replace(STRUCT, "", 1) + "struct WakeContext {",
    "the guard call removed": sleep.replace(GUARD_CALL, "", 1),
    "the plan call without its early return": sleep.replace("if (sleepPlan(plan)) return;", "sleepPlan(plan);", 1),
}
for name, text in probes.items():
    if text == sleep:
        fail(f"probe did not apply: {name}")
    if not problems_in(text):
        fail(f"probe not detected: {name}")
problems = problems_in(sleep)
if problems:
    fail(problems[0])

head = FIXTURE.read_text()
m = re.search(r"^# Base commit: (\w+)$", head, re.M)
base = m.group(1) if m else "3efd9d8"
base_sleep = git_show(base, "src/state/State_Sleep.cpp")
if base_sleep is None:
    print(f"BASE_CHECK_SKIPPED: commit {base} is not in this tree; only the structure checks ran")
    print("sleep_plan_structural_test: ok")
    sys.exit(0)

# No new src/ file; SleepTeardown.* unchanged.
ls = subprocess.run(["git", "-C", str(ROOT), "ls-tree", "-r", "--name-only", base, "src"], capture_output=True, text=True).stdout.split()
if SRC == ROOT / "src":
    cur_files = sorted(str(p.relative_to(ROOT)) for p in SRC.rglob("*") if p.is_file())
    if sorted(ls) != cur_files:
        fail(f"src/ file list differs from {base}: {sorted(set(cur_files) ^ set(ls))}")
for rel in ("src/state/SleepTeardown.cpp", "src/state/SleepTeardown.h"):
    want = git_show(base, rel)
    if want is None or (ROOT / rel).read_text() != want:
        fail(f"{rel} changed")

# `retained` is not added anywhere in src/ (the base count equals the current count, comment-stripped).
base_retained = sum(len(re.findall(r"\bretained\b", strip(git_show(base, rel) or "")))
                    for rel in ls if rel.endswith((".cpp", ".h", ".ino")))
cur_retained = sum(len(re.findall(r"\bretained\b", strip(p.read_text())))
                   for p in sorted(SRC.rglob("*")) if p.suffix in (".cpp", ".h", ".ino"))
if cur_retained != base_retained:
    fail("a `retained` keyword or identifier was added in src/")

# The S-2 functions, cloudGatePass() and the includes are unchanged.
tail_of = lambda t: t[t.index("struct WakeContext {"):t.index("} // namespace\n", t.index("struct WakeContext {"))]
prefix_of = lambda t: t[:t.index("namespace {")]
cloud_of = lambda t: t[t.index("struct {\n  bool operationsCompleteLogged"):t.index("struct SleepPlan {") if "struct SleepPlan {" in t else t.index("struct WakeContext {")]
if tail_of(sleep) != tail_of(base_sleep):
    fail("the S-2 functions (struct WakeContext .. end of the anonymous namespace) differ from the base")
if prefix_of(sleep) != prefix_of(base_sleep):
    fail("the includes differ from the base")
if cloud_of(sleep).rstrip("\n") != cloud_of(base_sleep).rstrip("\n"):
    fail("the cloudGate struct or cloudGatePass() differ from the base")

# The moved blocks and the handler are byte-identical to the base apart from the listed edits.
plan_cur, guard_cur, handler_cur = normalized(sleep)
plan_base, guard_base, handler_base = base_pieces(base_sleep)
if plan_cur is None:
    fail("could not extract sleepPlan(), nightlyHeapGuard() or the handler's call lines")
if plan_cur.rstrip("\n") != plan_base.rstrip("\n"):
    fail("sleepPlan() differs from the base block beyond `return true;` and the final `plan = ...; return false;`")
if guard_cur.rstrip("\n") != guard_base.rstrip("\n"):
    fail("nightlyHeapGuard() differs from the base block beyond `return true;` and the final `return false;`")
if handler_cur != handler_base:
    fail("the handler differs from the base beyond the plan call and unpack lines and the guard call")

# The byte-diff check itself is shown to fire: one changed character inside each moved block is detected.
for old, new, idx in (("546 * 60", "545 * 60", 0), ("freeHeap <= ConnectivityPolicy::NIGHTLY_HEAP_RESET", "freeHeap < ConnectivityPolicy::NIGHTLY_HEAP_RESET", 1)):
    tampered = sleep.replace(old, new, 1)
    if tampered == sleep or normalized(tampered)[idx].rstrip("\n") == (plan_base, guard_base)[idx].rstrip("\n"):
        fail("a changed moved block was not detected")

print("sleep_plan_structural_test: ok")
