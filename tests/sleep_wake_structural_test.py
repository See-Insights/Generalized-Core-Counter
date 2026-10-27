#!/usr/bin/env python3
"""WO-2026-10-22-001 (Sleep S-6): the wake-event processing is `bool processSleepWakeEvent(bool pirWake)`, one
owner at the end of State_Modes.cpp (after handleOccupancyMode() and updateOccupancyState()), declared once in
StateHandlers.h, called once from the file-local `static void postWakeRouting(...)` in State_Sleep.cpp. That
function sits after `} // namespace` and before handleSleepingState(), which calls it once as its last
statement. Neither the handler nor postWakeRouting() writes an occupancy or counting field, closes an
occupancy session or drives the LED. No new src/ file, SleepTeardown.*, the S-2..S-5 functions, logWakeSummary()
and the includes unchanged, no `retained` added, and the moved blocks are byte-identical to the base apart from
`return true;` and the final `return false;`. Each check is first shown to fire on a probe that breaks it. The
base is the commit in the transcript fixture header; without it in the tree the base comparisons are skipped
and said."""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "src"
SLEEP = SRC / "state" / "State_Sleep.cpp"
MODES = SRC / "state" / "State_Modes.cpp"
HEADER = SRC / "state" / "StateHandlers.h"
FIXTURE = ROOT / "tests" / "fixtures" / "sleep_wake_transcript.txt"

EVENT_SIG = "bool processSleepWakeEvent(bool pirWake) {"
EVENT_DECL = "bool processSleepWakeEvent(bool pirWake);"
ROUTE_SIG = "static void postWakeRouting(bool pirWake, bool buttonWake, bool timerWake, const char *wakeReasonLabel) {"
ROUTE_CALL = "  postWakeRouting(wake.pirWake, wake.buttonWake, wake.timerWake, wake.wakeReasonLabel);\n"
EVENT_CALL = "    if (processSleepWakeEvent(pirWake)) return;\n"
HANDLER_SIG = "void handleSleepingState()"
EVENT_TAIL = "  return false;\n"
UNPACK = ("  bool pirWake = wake.pirWake;\n  bool buttonWake = wake.buttonWake;\n  bool timerWake = wake.timerWake;\n"
          "  const char *wakeReasonLabel = wake.wakeReasonLabel;\n  \n")
BASE_REGION_START = "  // LED will be turned on with proper timeout by PIR wake processing below\n"
BASE_REGION_END = '    transitionTo(IDLE_STATE, "sleep-wake-idle");\n  }\n'
BASE_EVENT_START = "    // Check if LED timeout expired FIRST (before processing new PIR wake)\n"
OWNED_WRITES = re.compile(r"CurrentReadings::set_(?:hourlyCount|dailyCount|lastCountTime|occupied|occupancyStartTime|lastOccupancyEvent)\b"
                          r"|\bcloseOccupancySessionSafely\b|\bsignalLED\s*\(\s*true")


def fail(msg):
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


_LEX = re.compile(r"/\*.*?\*/|//[^\n]*|(\"(?:\\.|[^\"\\\n])*\")|('(?:\\.|[^'\\\n])*')", re.S)


def strip(code):
    """One left-to-right pass, so a `//` inside a string is not a comment and a quote inside a comment is not a string."""
    return _LEX.sub(lambda m: '""' if m.group(1) else (m.group(2) or ""), code)


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


def problems_in(sleep, modes, header):
    """Structure problems of the three current files (base-independent)."""
    out = []
    if header.count(EVENT_DECL) != 1:
        out.append(f"StateHandlers.h must declare `{EVENT_DECL}` exactly once")
    if modes.count(EVENT_SIG) != 1 or sleep.count("processSleepWakeEvent(") != 1:
        return out + [f"State_Modes.cpp must define `{EVENT_SIG}` exactly once and State_Sleep.cpp must call it exactly once"]
    plain_modes = strip(modes)
    e = plain_modes.find("bool processSleepWakeEvent(bool pirWake) {")
    if e < 0 or plain_modes[close_of(plain_modes, plain_modes.find("{", e)) + 1:].strip():
        out.append("processSleepWakeEvent() must be the last definition in State_Modes.cpp")
    for owner in ("void handleOccupancyMode() {", "void updateOccupancyState() {"):
        if not 0 <= plain_modes.find(owner) < e:
            out.append(f"processSleepWakeEvent() must come after `{owner[:-2]}`")
    if re.search(r"\bstatic\b", body_of(plain_modes, "bool processSleepWakeEvent(bool pirWake)") or ""):
        out.append("processSleepWakeEvent() must have no function statics")
    if sleep.count(ROUTE_SIG) != 1 or sleep.count("postWakeRouting(") != 2:
        return out + ["postWakeRouting() must be defined once as `static void postWakeRouting(bool, bool, bool, const char *)` "
                      "and called once"]
    ns_end = sleep.find("} // namespace\n")
    route_at, handler_at = sleep.find(ROUTE_SIG), sleep.find(HANDLER_SIG)
    if not 0 <= ns_end < route_at < handler_at:
        out.append("postWakeRouting() must sit after `} // namespace` and before handleSleepingState()")
    plain = strip(sleep)
    route = body_of(plain, strip(ROUTE_SIG))
    handler = body_of(plain, HANDLER_SIG)
    if route is None or handler is None:
        return out + ["postWakeRouting() and handleSleepingState() must each be defined once"]
    if route.count("processSleepWakeEvent(") != 1 or not re.search(r"if \(processSleepWakeEvent\(pirWake\)\) return;", route):
        out.append("postWakeRouting() must call `if (processSleepWakeEvent(pirWake)) return;` once")
    if "postWakeRouting(" not in handler or not strip(ROUTE_CALL).strip() in handler:
        out.append("the handler must call postWakeRouting(wake.pirWake, wake.buttonWake, wake.timerWake, wake.wakeReasonLabel)")
    if not handler.rstrip().endswith(strip(ROUTE_CALL).strip()):
        out.append("the postWakeRouting() call must be the handler's last statement")
    for where, text in (("State_Sleep.cpp", plain),):
        hit = OWNED_WRITES.search(text)
        if hit:
            out.append(f"{where} must not write occupancy or counting state, close a session, or switch the LED on "
                       f"(found `{hit.group(0)}`); that belongs to processSleepWakeEvent()")
    return out


sleep, modes, header = SLEEP.read_text(), MODES.read_text(), HEADER.read_text()

# ---- probes: every check fires on a text that breaks it -------------------------------------------------
event_body = body_of(modes, EVENT_SIG)
probes = {
    "the declaration removed": (sleep, modes, header.replace(EVENT_DECL, "", 1)),
    "the declaration twice": (sleep, modes, header.replace(EVENT_DECL, EVENT_DECL + "\n" + EVENT_DECL, 1)),
    "a function after the event owner": (sleep, modes + "\nvoid trailingHelper() {}\n", header),
    "the event owner before updateOccupancyState": (sleep, modes.replace("void updateOccupancyState() {", "void updateOccupancyState0() {", 1), header),
    "the event owner defined in State_Sleep.cpp": (sleep + "\n" + EVENT_SIG + "\n  return false;\n}\n", modes, header),
    "a function static in the event owner": (sleep, modes.replace(EVENT_SIG, EVENT_SIG + "\n    static int hits = 0;", 1), header),
    "postWakeRouting not static": (sleep.replace(ROUTE_SIG, ROUTE_SIG[len("static "):], 1), modes, header),
    "postWakeRouting inside the namespace": (sleep.replace("} // namespace\n\n" + ROUTE_SIG, ROUTE_SIG + "\n}\n} // namespace\n\nstatic void other() {", 1), modes, header),
    "postWakeRouting behind the handler": (sleep.replace(ROUTE_SIG, "static void postWakeRouting0(bool pirWake, bool buttonWake, bool timerWake, const char *wakeReasonLabel) {", 1)
                                           + "\n" + ROUTE_SIG + "\n}\n", modes, header),
    "the event call removed": (sleep.replace(EVENT_CALL, "", 1), modes, header),
    "the event call from the handler": (sleep.replace(ROUTE_CALL, ROUTE_CALL + "  processSleepWakeEvent(false);\n", 1), modes, header),
    "the routing call not last": (sleep.replace(ROUTE_CALL, ROUTE_CALL + "  Log.info(\"x\");\n", 1), modes, header),
    "a count write in postWakeRouting": (sleep.replace(EVENT_CALL, EVENT_CALL + "    CurrentReadings::set_hourlyCount(0);\n", 1), modes, header),
    "an occupancy writer elsewhere in State_Sleep.cpp": (sleep.replace("} // namespace\n\n" + ROUTE_SIG, "static void rogueOccupancyOwner() { CurrentReadings::set_occupied(false); }\n} // namespace\n\n" + ROUTE_SIG, 1), modes, header),
    "an occupancy writer hidden after a // in a string": (sleep.replace("} // namespace\n\n" + ROUTE_SIG, 'static void rogueOccupancyOwner() { Log.info("//"); CurrentReadings::set_occupied(false); }\n} // namespace\n\n' + ROUTE_SIG, 1), modes, header),
    "an occupancy write in the handler": (sleep.replace(ROUTE_CALL, "  CurrentReadings::set_occupied(false);\n" + ROUTE_CALL, 1), modes, header),
    "a session close in the handler": (sleep.replace(ROUTE_CALL, '  closeOccupancySessionSafely("x");\n' + ROUTE_CALL, 1), modes, header),
    "an LED write in postWakeRouting": (sleep.replace(EVENT_CALL, EVENT_CALL + "    signalLED(true, 1000);\n", 1), modes, header),
}
for name, args in probes.items():
    if args == (sleep, modes, header):
        fail(f"probe did not apply: {name}")
    if not problems_in(*args):
        fail(f"probe not detected: {name}")
problems = problems_in(sleep, modes, header)
if problems:
    fail(problems[0])

head = FIXTURE.read_text()
m = re.search(r"^# Base commit: (\w+)$", head, re.M)
base = m.group(1) if m else "e6d3e69"
base_sleep = git_show(base, "src/state/State_Sleep.cpp")
if base_sleep is None:
    print(f"BASE_CHECK_SKIPPED: commit {base} is not in this tree; only the structure checks ran")
    print("sleep_wake_structural_test: ok")
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

# `retained` is not added anywhere in src/.
base_retained = sum(len(re.findall(r"\bretained\b", strip(git_show(base, rel) or "")))
                    for rel in ls if rel.endswith((".cpp", ".h", ".ino")))
cur_retained = sum(len(re.findall(r"\bretained\b", strip(p.read_text())))
                   for p in sorted(SRC.rglob("*")) if p.suffix in (".cpp", ".h", ".ino"))
if cur_retained != base_retained:
    fail("a `retained` keyword or identifier was added in src/")

# The S-2 functions, logWakeSummary() and the includes are unchanged; State_Modes.cpp is unchanged above the new
# owner apart from its three added includes.
tail_of = lambda t: t[t.index("struct WakeContext {"):t.index("} // namespace\n", t.index("struct WakeContext {"))]
prefix_of = lambda t: t[:t.index("namespace {")]
log_of = lambda t: t[t.index("void logWakeSummary("):t.index("\n}\n", t.index("void logWakeSummary(")) + 3]
# WO-2026-10-25-001: the one intended change, the led= value is already in seconds; mapped back for the compare.
LED_NEW = "(unsigned long)signalLEDTimeRemaining());"
LED_BASE = "(unsigned long)(signalLEDTimeRemaining() / 1000UL));"
if log_of(sleep).count(LED_NEW) != 1:
    fail("logWakeSummary() must print signalLEDTimeRemaining() (seconds) without a further division")
if tail_of(sleep) != tail_of(base_sleep):
    fail("the S-2 functions (struct WakeContext .. end of the anonymous namespace) differ from the base")
if prefix_of(sleep) != prefix_of(base_sleep):
    fail("the State_Sleep.cpp includes differ from the base")
if log_of(sleep).replace(LED_NEW, LED_BASE) != log_of(base_sleep):
    fail("logWakeSummary() differs from the base")
base_modes = git_show(base, "src/state/State_Modes.cpp")
cur_modes_head = modes[:modes.index("\nbool processSleepWakeEvent(bool pirWake) {")]
added = [l for l in modes.splitlines(keepends=True) if l not in base_modes.splitlines(keepends=True)]
head_added = [l for l in cur_modes_head.splitlines(keepends=True) if l not in base_modes.splitlines(keepends=True)]
if head_added and not all(l.startswith("#include") for l in head_added):
    fail("State_Modes.cpp changed above the new owner beyond added includes")
if "".join(l for l in base_modes.splitlines(keepends=True) if not l.startswith("#include")).rstrip("\n") != \
        "".join(l for l in cur_modes_head.splitlines(keepends=True) if not l.startswith("#include")).rstrip("\n"):
    fail("State_Modes.cpp's existing functions differ from the base")


def moved_blocks(sleep_text, modes_text):
    """The route and the event owner mapped back to the base's inline region."""
    route = body_of(sleep_text, ROUTE_SIG)
    event = body_of(modes_text, EVENT_SIG)
    if route is None or event is None or route.count(EVENT_CALL) != 1 or not event.endswith(EVENT_TAIL):
        return None
    event = event[event.index(BASE_EVENT_START):-len(EVENT_TAIL)].replace("return true;", "return;")
    return route.lstrip("\n").replace(EVENT_CALL, event, 1)


# The moved blocks and the handler are byte-identical to the base apart from the listed edits.
cur_region = moved_blocks(sleep, modes)
if cur_region is None:
    fail("could not extract postWakeRouting() and processSleepWakeEvent()")
a = base_sleep.index(BASE_REGION_START)
b = base_sleep.index(BASE_REGION_END, a) + len(BASE_REGION_END)
base_region = base_sleep[a:b]
if cur_region.rstrip("\n") != base_region.rstrip("\n"):
    fail("postWakeRouting() plus processSleepWakeEvent() differ from the base region beyond `return true;`/`return false;` "
         "and the call")
handler = body_of(sleep, HANDLER_SIG)
if handler.count(ROUTE_CALL) != 1:
    fail("the handler must call postWakeRouting() once")
if not handler.rstrip().endswith("if (sleepAndResume(wakeInSeconds, useNetworkStandby, sleepReason, wake)) return;\n" + ROUTE_CALL.rstrip("\n")):
    fail("the handler must end with the sleepAndResume call followed by the postWakeRouting call")

# The byte-diff check itself is shown to fire: one changed character inside each moved block is detected.
for old, new, which in (("sleep-timer-occupied-suppress-report", "sleep-timer-occupied-suppress-reportx", "sleep"),
                        ("sleep-pir-occupancy-report", "sleep-pir-occupancy-reportx", "modes")):
    s2, m2 = (sleep.replace(old, new, 1), modes) if which == "sleep" else (sleep, modes.replace(old, new, 1))
    t = moved_blocks(s2, m2)
    if (s2, m2) == (sleep, modes) or t is None or t.rstrip("\n") == base_region.rstrip("\n"):
        fail("a changed moved block was not detected")

print("sleep_wake_structural_test: ok")
