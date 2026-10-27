#!/usr/bin/env python3
"""WO-2026-10-21-002 (Sleep S-3): the cloud-operations gate lives in cloudGatePass() in State_Sleep.cpp and
its ten fields in one anonymous struct instance, cloudGate, declared once in the anonymous namespace.
Only cloudGatePass() and the listed reset sites in handleSleepingState() write the fields. No new src/
file, SleepTeardown.* and the S-2 functions unchanged, no `retained` added, and the moved block is
byte-identical to the base apart from the cloudGate. prefix, `return true;` and the added `return false;`.
Each check is first shown to fire on a probe that breaks it. The base is the commit in the transcript
fixture header; without it in the tree the base comparisons are skipped and said."""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# S-5's four entry resets are the only handler lines allowed to name wifiGuard; the prefix is mapped back to the
# base's statics for those four lines alone, so any other wifiGuard. text in the handler still differs.
WIFI_RESETS = re.compile(r"^(    )wifiGuard\.(wifiOffGuardActive = false;|wifiOffGuardStartMs = 0;|wifiOffGuardLastRetryMs = 0;|wifiOffGuardRetries = 0;)$", re.M)
SRC = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "src"
SLEEP = SRC / "state" / "State_Sleep.cpp"
FIXTURE = ROOT / "tests" / "fixtures" / "sleep_cloud_gate_transcript.txt"
FIELDS = {
    "operationsCompleteLogged": ("bool", "false"),
    "gateWasBlocking": ("bool", "false"),
    "lastGateStatusLogMs": ("unsigned long", "0"),
    "lastGateBlockerMask": ("uint8_t", "kGateBlockerUninitialized"),
    "lastGateBlockerBeforeRelease": ("uint8_t", "0"),
    "gateReleaseReasonKnown": ("bool", "false"),
    "cloudSyncStartMs": ("unsigned long", "0"),
    "cloudSyncBudgetMs": ("unsigned long", "0"),
    "cloudSyncMaxQueueDepth": ("uint16_t", "0"),
    "lastQueueDepth": ("uint16_t", "0xFFFF"),
}
# Writes the handler may keep: field -> count (the entry resets, the abort and the two priority exits).
HANDLER_WRITES = {
    "operationsCompleteLogged": 1, "gateWasBlocking": 1, "lastGateStatusLogMs": 1, "lastGateBlockerMask": 1,
    "lastGateBlockerBeforeRelease": 1, "gateReleaseReasonKnown": 1, "cloudSyncStartMs": 3,
    "cloudSyncBudgetMs": 1, "cloudSyncMaxQueueDepth": 1, "lastQueueDepth": 1,
}
NAMES = "|".join(FIELDS)
CALL = "  if (cloudGatePass(useNetworkStandbyEffective)) return;\n"
# WO-2026-10-21-004 (Sleep S-5): the handler head's only other allowed differences are the removal of the four
# Wi-Fi guard statics and their entry resets naming the file-level wifiGuard struct's fields.
WIFI_STATICS = ("#if Wiring_WiFi\n  static bool wifiOffGuardActive = false;\n  static unsigned long wifiOffGuardStartMs = 0;\n"
                "  static unsigned long wifiOffGuardLastRetryMs = 0;\n  static uint8_t wifiOffGuardRetries = 0;\n#endif\n")
WRITE = re.compile(rf"\bcloudGate\s*\.\s*(?:{NAMES})\b\s*(?:=(?!=)|\+\+|--|[-+*/|&^%]=|<<=|>>=)"
                   rf"|(?:\+\+|--)\s*cloudGate\s*\.\s*(?:{NAMES})\b|&\s*cloudGate\s*\.\s*(?:{NAMES})\b")


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
    ns_start = code.find("namespace {")
    ns_end = code.find("} // namespace\n")
    decls = [m.start() for m in re.finditer(r"\}\s*cloudGate\s*;", code)]
    if len(decls) != 1 or not (0 <= ns_start < decls[0] < ns_end):
        return ["cloudGate must be declared once, in the anonymous namespace"]
    s_open = code.rfind("struct {", 0, decls[0])
    if s_open < 0 or code.count("struct {", s_open, decls[0]) != 1:
        return ["cloudGate must be one anonymous struct"]
    struct_text = code[code.rfind("struct {", 0, code.find("} cloudGate;") + 1):code.find("} cloudGate;")]
    for name, (typ, init) in FIELDS.items():
        if len(re.findall(rf"\b{name}\b", strip(struct_text))) != 1 or \
                not re.search(rf"^\s*{re.escape(typ)} {name} = {re.escape(init)};", struct_text, re.M):
            out.append(f"the struct must declare {name} once as `{typ} {name} = {init};`")
    if len(re.findall(r"\b(?:bool|unsigned long|uint8_t|uint16_t)\s+(?:%s)\b\s*=" % NAMES, plain)) != len(FIELDS):
        out.append("a gate field is declared outside the cloudGate struct")
    if re.search(r"\bstatic\b[^;\n]*\b(?:%s)\b" % NAMES, plain):
        out.append("a gate field is still a static")
    gate = body_of(plain, "bool cloudGatePass(bool useNetworkStandbyEffective)")
    handler = body_of(plain, "void handleSleepingState()")
    if gate is None or handler is None:
        return out + ["cloudGatePass() and handleSleepingState() must each be defined once"]
    if code.find("bool cloudGatePass(") > ns_end or code.find("bool cloudGatePass(") > code.find("struct WakeContext"):
        out.append("cloudGatePass() must sit in the anonymous namespace, before struct WakeContext")
    if plain.count("cloudGatePass(") != 2 or "if (cloudGatePass(useNetworkStandbyEffective)) return;" not in handler:
        out.append("the handler must call `if (cloudGatePass(useNetworkStandbyEffective)) return;` once")
    # Writes: nowhere but cloudGatePass() and the listed handler reset sites.
    outside = plain.replace(gate, "", 1).replace(handler, "", 1)
    if WRITE.search(outside):
        out.append("a gate field is written outside cloudGatePass() and handleSleepingState()")
    counts = {}
    for line in handler.splitlines():
        for m in re.finditer(rf"\bcloudGate\s*\.\s*({NAMES})\b", line):
            if WRITE.search(line):
                counts[m.group(1)] = counts.get(m.group(1), 0) + 1
    if counts != HANDLER_WRITES:
        out.append(f"the handler's gate writes must be exactly the reset sites {HANDLER_WRITES}; found {counts}")
    for line in handler.splitlines():
        if re.search(rf"\bcloudGate\s*\.\s*(?:{NAMES})\b", line) and not WRITE.search(line):
            out.append(f"the handler reads a gate field: {line.strip()}")
    return out


def normalized_block(code):
    """The current gate function body and handler head, mapped back to the base's inline text."""
    gate = body_of(code, "bool cloudGatePass(bool useNetworkStandbyEffective)")
    gate = gate.lstrip("\n")
    if not gate.endswith("  return false;\n"):
        return None, None
    gate = gate[:-len("  return false;\n")].replace("return true;", "return;").replace("cloudGate.", "")
    handler = body_of(code, "void handleSleepingState()")
    head = handler[handler.index("  bool enteredState = (state != oldState);\n"):handler.index("  if (SleepTeardown::run(")]
    if head.count(CALL) != 1:
        return None, None
    return gate, WIFI_RESETS.sub(r"\1\2", head.replace(CALL, "@@GATE@@").replace("cloudGate.", ""))


def base_pieces(base_text):
    handler = body_of(base_text, "void handleSleepingState()")
    head = handler[handler.index("  bool enteredState = (state != oldState);\n"):handler.index("  if (SleepTeardown::run(")]
    start = "  // Only wait for cloud operations *before* requesting disconnect.\n"
    i = head.index(start)
    block = head[i:].rstrip("\n") + "\n"
    head = head[:i] + "@@GATE@@\n"
    for name, (typ, init) in FIELDS.items():
        decl = [ln for ln in head.splitlines(True) if re.match(rf"  static .*\b{name} = ", ln)]
        if len(decl) != 1:
            return None, None
        head = head.replace(decl[0], "", 1)
    if head.count(WIFI_STATICS) != 1:
        return None, None
    return block, head.replace(WIFI_STATICS, "", 1)


# ---- probes: every check fires on a text that breaks it -------------------------------------------------
sleep = SLEEP.read_text()
probes = {
    "a write outside the allowed places": sleep.replace("void handleSleepingState() {", "void probeFn() { cloudGate.lastQueueDepth = 1; }\nvoid handleSleepingState() {", 1),
    "a write in the handler beyond the reset sites": sleep.replace("  SleepTeardown::guardLostTimestamp();\n", "  SleepTeardown::guardLostTimestamp();\n  cloudGate.gateWasBlocking = true;\n", 1),
    "a handler read of a gate field": sleep.replace("  SleepTeardown::guardLostTimestamp();\n", "  SleepTeardown::guardLostTimestamp();\n  if (cloudGate.gateWasBlocking) {}\n", 1),
    "a dropped reset site": sleep.replace("    cloudGate.cloudSyncStartMs = 0; // leaving mid-gate: a later return starts its gate cleanly\n", "", 1),
    "a field redeclared as a static": sleep.replace("void handleSleepingState() {", "static bool gateWasBlocking = false;\nvoid handleSleepingState() {", 1),
    "a second struct instance": sleep.replace("} cloudGate;", "} cloudGate;\nstruct { bool gateWasBlocking = false; } cloudGate2;", 1),
    "a changed initial value": sleep.replace("uint16_t lastQueueDepth = 0xFFFF;", "uint16_t lastQueueDepth = 0;", 1),
    "the gate call removed": sleep.replace(CALL, "", 1),
}
for name, text in probes.items():
    if text == sleep:
        fail(f"probe did not apply: {name}")
    if not problems_in(text):
        fail(f"probe not detected: {name}")
problems = problems_in(sleep)
if problems:
    fail(problems[0])

# `retained` anywhere in src/ is compared with the base (comment-stripped).
def retained_count(root_text_by_file):
    return sum(len(re.findall(r"\bretained\b", strip(t))) for t in root_text_by_file)


current_files = {p: p.read_text() for p in sorted(SRC.rglob("*")) if p.suffix in (".cpp", ".h", ".ino")}
if "retained cloudGate" in sleep or re.search(r"\bretained\b[^;\n]*\bcloudGate\b", sleep):
    fail("cloudGate must not be retained")

head = FIXTURE.read_text()
m = re.search(r"^# Base commit: (\w+)$", head, re.M)
base = m.group(1) if m else "9578709"
base_sleep = git_show(base, "src/state/State_Sleep.cpp")
if base_sleep is None:
    print(f"BASE_CHECK_SKIPPED: commit {base} is not in this tree; only the structure checks ran")
    print("sleep_cloud_gate_structural_test: ok")
    sys.exit(0)

# No new src/ file; SleepTeardown.* unchanged.
ls = subprocess.run(["git", "-C", str(ROOT), "ls-tree", "-r", "--name-only", base, "src"], capture_output=True, text=True).stdout.split()
cur_files = sorted(str(p.relative_to(ROOT)) for p in SRC.rglob("*") if p.is_file()) if SRC == ROOT / "src" else None
if cur_files is not None and sorted(ls) != cur_files:
    fail(f"src/ file list differs from {base}: {sorted(set(cur_files) ^ set(ls))}")
for rel in ("src/state/SleepTeardown.cpp", "src/state/SleepTeardown.h"):
    want = git_show(base, rel)
    if want is None or (ROOT / rel).read_text() != want:
        fail(f"{rel} changed")

# `retained` is not added anywhere in src/ (the base count equals the current count).
base_retained = 0
for rel in ls:
    t = git_show(base, rel)
    if t is not None and rel.endswith((".cpp", ".h", ".ino")):
        base_retained += len(re.findall(r"\bretained\b", strip(t)))
if retained_count(current_files.values()) != base_retained:
    fail("a `retained` keyword or identifier was added in src/")

# The S-2 functions and the includes are unchanged.
def tail_of(text):
    i = text.index("struct WakeContext {")
    return text[i:text.index("} // namespace\n", i)]


def prefix_of(text):
    return text[:text.index("namespace {")]


def drain_of(text):
    i = text.index("void drainSerialBeforeSleep() {")
    return text[i:text.index("\n}\n", i)]


if tail_of(sleep) != tail_of(base_sleep):
    fail("the S-2 functions (struct WakeContext .. end of the anonymous namespace) differ from the base")
if prefix_of(sleep) != prefix_of(base_sleep) or drain_of(sleep) != drain_of(base_sleep):
    fail("the includes or drainSerialBeforeSleep differ from the base")
helpers = lambda t: t[t.index("unsigned long computeCloudSyncTimeoutMs("):t.index('    return "unknown";\n  }\n}\n')]
if helpers(sleep) != helpers(base_sleep):
    fail("computeCloudSyncTimeoutMs, the kGate* constants or gateBlockerReasonLabel moved or changed")

# The moved block and the handler head are byte-identical to the base apart from the listed edits.
gate_cur, head_cur = normalized_block(sleep)
block_base, head_base = base_pieces(base_sleep)
if gate_cur is None or block_base is None:
    fail("could not extract the gate function or the base block")
if gate_cur != block_base:
    fail("cloudGatePass() differs from the base block beyond the cloudGate. prefix, `return true;` and `return false;`")
if head_cur != head_base:
    fail("the handler head differs from the base beyond the cloudGate. prefix, the removed statics and the call")

# The check itself is shown to fire: one changed character inside the moved block is detected.
tampered = sleep.replace("elapsedMs < cloudGate.cloudSyncBudgetMs", "elapsedMs <= cloudGate.cloudSyncBudgetMs", 1)
if tampered == sleep or normalized_block(tampered)[0] == block_base:
    fail("a changed moved block was not detected")

print("sleep_cloud_gate_structural_test: ok")
