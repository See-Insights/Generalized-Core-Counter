#!/usr/bin/env python3
"""WO-2026-10-21-004 (Sleep S-5): the pre-sleep cycle summary and the Wi-Fi radio-off guard are two file-local
functions in the anonymous namespace of State_Sleep.cpp (endOfCycleSummary() and, under Wiring_WiFi,
wifiOffGuardPass()), placed between cloudGatePass() and struct SleepPlan. The guard's four fields live in one
anonymous struct instance, wifiGuard, declared once under Wiring_WiFi in the anonymous namespace. Only
wifiOffGuardPass() and the handler's entry reset write the fields. No new src/ file, SleepTeardown.*, the
S-2, S-3 and S-4 functions and the includes unchanged, no `retained` added, and the moved blocks are
byte-identical to the base apart from the wifiGuard. prefix, `return true;`, the added `return false;`, the
block braces and the handler's two call lines. Each check is first shown to fire on a probe that breaks it.
The base is the commit in the transcript fixture header; without it in the tree the base comparisons are
skipped and said."""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# S-5's four entry resets are the only handler lines allowed to name wifiGuard; the prefix is mapped back to the
# base's statics for those four lines alone, so any other wifiGuard. text in the handler still differs.
WIFI_RESETS = re.compile(r"^(    )wifiGuard\.(wifiOffGuardActive = false;|wifiOffGuardStartMs = 0;|wifiOffGuardLastRetryMs = 0;|wifiOffGuardRetries = 0;)$", re.M)
ROUTE_CALL = "  postWakeRouting(wake.pirWake, wake.buttonWake, wake.timerWake, wake.wakeReasonLabel);\n"
ROUTE_BASE = "  bool pirWake = wake.pirWake;\n"
SRC = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "src"
SLEEP = SRC / "state" / "State_Sleep.cpp"
FIXTURE = ROOT / "tests" / "fixtures" / "sleep_preflight_transcript.txt"
FIELDS = {
    "wifiOffGuardActive": ("bool", "false"),
    "wifiOffGuardStartMs": ("unsigned long", "0"),
    "wifiOffGuardLastRetryMs": ("unsigned long", "0"),
    "wifiOffGuardRetries": ("uint8_t", "0"),
}
NAMES = "|".join(FIELDS)
SUMMARY_SIG = "void endOfCycleSummary(bool useNetworkStandby) {"
GUARD_SIG = "bool wifiOffGuardPass(bool useNetworkStandby) {"
SUMMARY_CALL = "  endOfCycleSummary(useNetworkStandby);\n"
WIFI_CALL = "#if Wiring_WiFi\n  if (wifiOffGuardPass(useNetworkStandby)) return;\n#endif\n"
WIFI_STATICS = ("#if Wiring_WiFi\n  static bool wifiOffGuardActive = false;\n  static unsigned long wifiOffGuardStartMs = 0;\n"
                "  static unsigned long wifiOffGuardLastRetryMs = 0;\n  static uint8_t wifiOffGuardRetries = 0;\n#endif\n")
SUMMARY_START = "  {\n    measure.batteryState(BatterySampleContext::PreSleep);"
SUMMARY_END = "  // On WiFi platforms"
WIFI_START = "#if Wiring_WiFi\n  if (!useNetworkStandby) {"
WIFI_END = "#endif\n  \n  WakeContext wake;"
WRITE = re.compile(rf"\bwifiGuard\s*\.\s*(?:{NAMES})\b\s*(?:=(?!=)|\+\+|--|[-+*/|&^%]=|<<=|>>=)"
                   rf"|(?:\+\+|--)\s*wifiGuard\s*\.\s*(?:{NAMES})\b|&\s*wifiGuard\s*\.\s*(?:{NAMES})\b")


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


def retained_count(text):
    return len(re.findall(r"\bretained\b", strip(text)))


def problems_in(code):
    """Structure problems of one State_Sleep.cpp text (base-independent)."""
    out = []
    plain = strip(code)
    ns_end = code.find("} // namespace\n")
    pos = {name: code.find(sig) for name, sig in (("summary", SUMMARY_SIG), ("guard", GUARD_SIG),
                                                  ("cloud", "bool cloudGatePass("), ("plan", "struct SleepPlan {"),
                                                  ("wake", "struct WakeContext {"))}
    for name, sig in (("endOfCycleSummary()", SUMMARY_SIG), ("wifiOffGuardPass()", GUARD_SIG)):
        if code.count(sig) != 1:
            return [f"{name} must be defined exactly once as `{sig}`"]
    decls = [m.start() for m in re.finditer(r"\}\s*wifiGuard\s*;", code)]
    if len(decls) != 1:
        return ["wifiGuard must be declared once"]
    sdecl = decls[0]
    s_open = code.rfind("struct {", 0, sdecl)
    if s_open < 0 or code.count("struct {", s_open, sdecl) != 1 or code.rfind("#if Wiring_WiFi\n", 0, s_open) != s_open - len("#if Wiring_WiFi\n"):
        return ["wifiGuard must be one anonymous struct directly under `#if Wiring_WiFi`"]
    if not (0 <= pos["cloud"] < pos["summary"] < s_open < sdecl < pos["guard"] < pos["plan"] < pos["wake"] < ns_end):
        out.append("endOfCycleSummary(), wifiGuard and wifiOffGuardPass() must sit in the anonymous namespace, between "
                   "cloudGatePass() and struct SleepPlan, in that order")
    wifi_end = code.find("#endif\n", pos["guard"])
    if not (pos["guard"] < wifi_end < pos["plan"]) or code[wifi_end + len("#endif\n"):].startswith("#") or \
            code.count("#if", s_open, wifi_end) != 0 or code.find("#endif", sdecl, pos["guard"]) >= 0:
        out.append("wifiGuard and wifiOffGuardPass() must share one `#if Wiring_WiFi` ... `#endif` that ends before struct SleepPlan")
    struct_text = code[s_open:sdecl]
    for name, (typ, init) in FIELDS.items():
        if len(re.findall(rf"\b{name}\b", strip(struct_text))) != 1 or \
                not re.search(rf"^\s*{re.escape(typ)} {name} = {re.escape(init)};", struct_text, re.M):
            out.append(f"the struct must declare {name} once as `{typ} {name} = {init};`")
    if len(re.findall(r"\b(?:bool|unsigned long|uint8_t)\s+(?:%s)\b\s*=" % NAMES, plain)) != len(FIELDS):
        out.append("a guard field is declared outside the wifiGuard struct")
    if re.search(r"\bstatic\b[^;\n]*\b(?:%s)\b" % NAMES, plain):
        out.append("a guard field is still a static")
    summary = body_of(plain, SUMMARY_SIG)
    guard = body_of(plain, GUARD_SIG)
    handler = body_of(plain, "void handleSleepingState()")
    if summary is None or guard is None or handler is None:
        return out + ["endOfCycleSummary(), wifiOffGuardPass() and handleSleepingState() must each be defined once"]
    if re.search(r"\bstatic\b", summary) or re.search(r"\bstatic\b", guard):
        out.append("the two functions must have no function statics")
    # Both functions are file-local: defined and called only here, from the handler.
    if plain.count("endOfCycleSummary(") != 2 or plain.count("wifiOffGuardPass(") != 2:
        out.append("endOfCycleSummary() and wifiOffGuardPass() are each defined once and called once, from the handler only")
    if re.search(r"\bextern\b[^;\n]*(?:endOfCycleSummary|wifiOffGuardPass|wifiGuard)", plain):
        out.append("the S-5 functions and wifiGuard must not be extern")
    if not re.search(r"^  endOfCycleSummary\(useNetworkStandby\);$", handler, re.M):
        out.append("the handler must call `endOfCycleSummary(useNetworkStandby);`")
    if code.count(WIFI_CALL) != 1:
        out.append("the handler must call `if (wifiOffGuardPass(useNetworkStandby)) return;` once, inside `#if Wiring_WiFi`")
    else:
        h = code[code.find("void handleSleepingState()"):]
        if SUMMARY_CALL not in h or not h.index(SUMMARY_CALL) < h.index(WIFI_CALL) < h.index("  WakeContext wake;"):
            out.append("the handler's order must be summary -> guard -> sleepAndResume")
    # Writes: only wifiOffGuardPass() and the handler's entry reset; no other mention of wifiGuard.
    rest = plain.replace(guard, "", 1).replace(handler, "", 1)
    if re.search(r"\bwifiGuard\b", re.sub(r"\}\s*wifiGuard\s*;", "", rest, count=1)):
        out.append("wifiGuard is used outside wifiOffGuardPass() and handleSleepingState()")
    reset = re.search(r"if \(enteredState\) \{\n    cloudGate\.cloudSyncBudgetMs.*?\n  \}\n", handler, re.S)
    counts = {}
    for line in handler.splitlines():
        if re.search(rf"\bwifiGuard\b", line):
            m = re.search(rf"\bwifiGuard\s*\.\s*({NAMES})\b", line)
            if not m or not WRITE.search(line):
                out.append(f"the handler reads or aliases the guard: {line.strip()}")
            else:
                counts[m.group(1)] = counts.get(m.group(1), 0) + 1
    if counts != {n: 1 for n in FIELDS}:
        out.append(f"the handler's guard writes must be exactly the four entry resets; found {counts}")
    if reset is None or len(WRITE.findall(reset.group(0))) != len(FIELDS):
        out.append("the four guard writes in the handler must sit in the `if (enteredState)` block")
    return out


def normalized(code):
    """The current functions and handler, mapped back to the base's inline text."""
    summary = body_of(code, SUMMARY_SIG)
    guard = body_of(code, GUARD_SIG)
    handler = body_of(code, "void handleSleepingState()")
    if summary is None or guard is None or handler is None:
        return None, None, None
    guard = guard.lstrip("\n")
    if not guard.endswith("  return false;\n"):
        return None, None, None
    guard = guard[:-len("  return false;\n")].replace("return true;", "return;").replace("wifiGuard.", "")
    if handler.count(SUMMARY_CALL) != 1 or handler.count(WIFI_CALL) != 1 or handler.count(ROUTE_CALL) != 1:
        return None, None, None
    handler = handler.replace(SUMMARY_CALL, "@@SUMMARY@@\n").replace(WIFI_CALL, "#if Wiring_WiFi\n@@WIFI@@\n#endif\n")
    handler = handler.replace(ROUTE_CALL, "@@ROUTE@@\n")
    return summary.lstrip("\n"), guard, WIFI_RESETS.sub(r"\1\2", handler)


def base_pieces(base_text):
    handler = body_of(base_text, "void handleSleepingState()")
    if handler.count(WIFI_STATICS) != 1 or handler.count(SUMMARY_START) != 1 or handler.count(WIFI_START) != 1:
        return None, None, None
    s0, s1 = handler.index(SUMMARY_START), handler.index(SUMMARY_END)
    block = handler[s0:s1].rstrip("\n") + "\n"
    inner = block[len("  {\n"):-len("  }\n")]
    w0, w1 = handler.index(WIFI_START), handler.index(WIFI_END)
    wifi = handler[w0 + len("#if Wiring_WiFi\n"):w1]
    rest = handler.replace(WIFI_STATICS, "", 1)
    rest = rest.replace(handler[s0:s1], "@@SUMMARY@@\n\n", 1)
    rest = rest.replace(handler[w0:w1 + len(WIFI_END)], "#if Wiring_WiFi\n@@WIFI@@\n#endif\n  \n  WakeContext wake;", 1)
    if rest.count(ROUTE_BASE) != 1:
        return None, None, None
    rest = rest[:rest.index(ROUTE_BASE)] + "@@ROUTE@@\n"
    return inner, wifi, rest


def move_wifi_block(text):
    a = text.index("#if Wiring_WiFi\nstruct {")
    b = text.index("struct SleepPlan {")
    block = text[a:b]
    c = text.index("\n", b) + 1
    return text[:a] + text[b:c] + block + text[c:]


# ---- probes: every check fires on a text that breaks it -------------------------------------------------
sleep = SLEEP.read_text()
RESET = "    wifiGuard.wifiOffGuardRetries = 0;\n#endif\n  }"
probes = {
    "a stray write to a guard field in the handler": sleep.replace("  SleepTeardown::guardLostTimestamp();\n", "  SleepTeardown::guardLostTimestamp();\n  wifiGuard.wifiOffGuardActive = true;\n", 1),
    "a stray write in another function": sleep.replace("struct SleepPlan {", "void strayWrite() { wifiGuard.wifiOffGuardRetries++; }\nstruct SleepPlan {", 1),
    "a read of a guard field in the handler": sleep.replace("  SleepTeardown::guardLostTimestamp();\n", "  SleepTeardown::guardLostTimestamp();\n  if (wifiGuard.wifiOffGuardActive) {}\n", 1),
    "a dropped entry reset": sleep.replace(RESET, "#endif\n  }", 1),
    "a field redeclared as a static": sleep.replace("void handleSleepingState() {", "static bool wifiOffGuardActive = false;\nvoid handleSleepingState() {", 1),
    "a second struct instance": sleep.replace("} wifiGuard;", "} wifiGuard;\nstruct { bool wifiOffGuardActive = false; } wifiGuard2;", 1),
    "a changed initial value": sleep.replace("uint8_t wifiOffGuardRetries = 0;\n} wifiGuard;", "uint8_t wifiOffGuardRetries = 1;\n} wifiGuard;", 1),
    "the Wi-Fi block moved behind struct SleepPlan": move_wifi_block(sleep),
    "the summary moved before cloudGatePass": sleep.replace("bool cloudGatePass(", "void endOfCycleSummary(bool useNetworkStandby);\nbool cloudGatePass(", 1),
    "the guard call outside the Wi-Fi block": sleep.replace(WIFI_CALL, "  if (wifiOffGuardPass(useNetworkStandby)) return;\n", 1),
    "the summary call removed": sleep.replace(SUMMARY_CALL, "", 1),
    "the guard made extern": sleep.replace(GUARD_SIG, "extern " + GUARD_SIG, 1),
    "a function static in the summary": sleep.replace(SUMMARY_SIG, SUMMARY_SIG + "\n    static int probeStatic = 0;", 1),
}
for name, text in probes.items():
    if text == sleep:
        fail(f"probe did not apply: {name}")
    if not problems_in(text):
        fail(f"probe not detected: {name}")
problems = problems_in(sleep)
if problems:
    fail(problems[0])
if retained_count(sleep + "\nretained int probe;\n") == retained_count(sleep):
    fail("the retained probe is not counted")

head = FIXTURE.read_text()
m = re.search(r"^# Base commit: (\w+)$", head, re.M)
base = m.group(1) if m else "e9475d7"
base_sleep = git_show(base, "src/state/State_Sleep.cpp")
if base_sleep is None:
    print(f"BASE_CHECK_SKIPPED: commit {base} is not in this tree; only the structure checks ran")
    print("sleep_preflight_structural_test: ok")
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
    others = [p for p in SRC.rglob("*") if p.is_file() and p != SLEEP and p.suffix in (".cpp", ".h", ".ino")]
    if any(re.search(r"\b(?:endOfCycleSummary|wifiOffGuardPass|wifiGuard)\b", p.read_text()) for p in others):
        fail("the S-5 names appear outside State_Sleep.cpp")

# `retained` is not added anywhere in src/ (the base count equals the current count, comment-stripped).
base_retained = sum(retained_count(git_show(base, rel) or "") for rel in ls if rel.endswith((".cpp", ".h", ".ino")))
cur_retained = sum(retained_count(p.read_text()) for p in sorted(SRC.rglob("*")) if p.suffix in (".cpp", ".h", ".ino"))
if cur_retained != base_retained:
    fail("a `retained` keyword or identifier was added in src/")

# The S-2 functions, the S-3 gate, the S-4 functions and the includes are unchanged.
tail_of = lambda t: t[t.index("struct WakeContext {"):t.index("} // namespace\n", t.index("struct WakeContext {"))]
prefix_of = lambda t: t[:t.index("namespace {")]
gate_of = lambda t: t[t.index("struct {\n  bool operationsCompleteLogged"):t.index(SUMMARY_SIG) if SUMMARY_SIG in t else t.index("struct SleepPlan {")]
plan_of = lambda t: t[t.index("struct SleepPlan {"):t.index("struct WakeContext {")]
if tail_of(sleep) != tail_of(base_sleep):
    fail("the S-2 functions (struct WakeContext .. end of the anonymous namespace) differ from the base")
if prefix_of(sleep) != prefix_of(base_sleep):
    fail("the includes differ from the base")
if gate_of(sleep).rstrip("\n") != gate_of(base_sleep).rstrip("\n"):
    fail("the cloudGate struct or cloudGatePass() (S-3) differ from the base")
if plan_of(sleep) != plan_of(base_sleep):
    fail("struct SleepPlan, sleepPlan(), hibernateAttempt() or nightlyHeapGuard() (S-4) differ from the base")

# The moved blocks and the handler are byte-identical to the base apart from the listed edits.
summary_cur, guard_cur, handler_cur = normalized(sleep)
summary_base, guard_base, handler_base = base_pieces(base_sleep)
if summary_cur is None or summary_base is None:
    fail("could not extract the S-5 functions, the handler's call lines or the base blocks")
if summary_cur != summary_base:
    fail("endOfCycleSummary() differs from the base block beyond the function's own braces")
if guard_cur.rstrip("\n") != guard_base.rstrip("\n"):
    fail("wifiOffGuardPass() differs from the base block beyond the wifiGuard. prefix, `return true;` and `return false;`")
if handler_cur != handler_base:
    fail("the handler differs from the base beyond the two calls, the removed statics, the wifiGuard. entry resets and the post-wake routing call")

# The byte-diff check itself is shown to fire: one changed character inside each moved block, and in the
# handler, is detected.
for old, new, which in (("30000UL; // slack", "3000UL; // slack", 0), ("WIFI_OFF_GUARD_RETRY_MS = 400UL", "WIFI_OFF_GUARD_RETRY_MS = 401UL", 1),
                        ("  SleepTeardown::guardLostTimestamp();\n", "  SleepTeardown::guardLostTimestamp();\n  probe();\n", 2)):
    tampered = sleep.replace(old, new, 1)
    cur = normalized(tampered)
    base_ref = (summary_base, guard_base, handler_base)
    if tampered == sleep or cur[which] is None or cur[which].rstrip("\n") == base_ref[which].rstrip("\n"):
        fail("a changed moved block was not detected")

print("sleep_preflight_structural_test: ok")
