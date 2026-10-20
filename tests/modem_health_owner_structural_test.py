#!/usr/bin/env python3
"""WO-2026-10-19-004: power/ModemHealth is the only writer of the four standby-latch
fields; the call sites use it; the new files carry no retained-memory keyword."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "src"
OWNER_CPP = SRC / "power" / "ModemHealth.cpp"
OWNER_HDR = SRC / "power" / "ModemHealth.h"
FIELDS = "modemUnstable|modemStandbySuppressed|modemUnstableReason|modemConnectTimeoutCount"
WRITE = re.compile(
    rf"(?:\b(?:{FIELDS})\b\s*(?:=(?!=)|\+\+|--|[-+*/|&^%]=|<<=|>>=)"
    rf"|(?:\+\+|--)\s*[\w.>-]*\b(?:{FIELDS})\b"
    rf"|&\s*[\w.>-]*\b(?:{FIELDS})\b)")
DECLARATION = re.compile(rf"^\s*(?:bool|uint8_t)\s+(?:{FIELDS})\s*=")


def fail(msg):
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def strip_comments_and_strings(code):
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.S)
    code = re.sub(r"//[^\n]*", "", code)
    return re.sub(r'"(?:\\.|[^"\\\n])*"', '""', code)


def writes(code):
    return [line.strip() for line in strip_comments_and_strings(code).splitlines()
            if WRITE.search(line) and not DECLARATION.match(line)]


# The detector itself: writes are flagged, reads are not.
for probe in ("session.modemUnstable = true;", "session.modemConnectTimeoutCount++;",
              "++session.modemConnectTimeoutCount;", "session.modemUnstableReason |= 1;",
              "uint8_t *p = &session.modemStandbySuppressed;", "modemUnstable=false;"):
    if not writes(probe):
        fail(f"detector missed a write: {probe}")
for probe in ("if (session.modemUnstable == false) {", "x = session.modemUnstableReason;",
              "const bool s = !session.modemStandbySuppressed;",
              "bool modemUnstable = false;             // RAM-only", "// session.modemUnstable = true;",
              'Log.info("session.modemUnstable = true");'):
    if writes(probe):
        fail(f"detector flagged a read, declaration or comment: {probe}")

for path in sorted(SRC.rglob("*")):
    if path.suffix not in (".cpp", ".h") or path == OWNER_CPP:
        continue
    hits = writes(path.read_text())
    if hits:
        fail(f"{path.relative_to(SRC)} writes a latch field outside ModemHealth.cpp: {hits[0]}")

owner = OWNER_CPP.read_text()
if len(writes(owner)) < 8:
    fail("ModemHealth.cpp no longer writes the latch fields (detector or owner broken)")
for name in ("markModemUnstable", "maybeClearModemUnstable", "noteConnectTimeout",
             "noteConnectSuccess", "clearAtDailyClose"):
    if len(re.findall(rf"^void {name}\(", owner, re.M)) != 1:
        fail(f"ModemHealth.cpp must define {name} once")
    if len(re.findall(rf"\b{name}\(", OWNER_HDR.read_text())) != 1:
        fail(f"ModemHealth.h must declare {name} once")

for path in (OWNER_CPP, OWNER_HDR):
    if re.search(r"retained", path.read_text(), re.I):
        fail(f"{path.name} must not mention retained")

# The equivalence test is frozen output only: no git, no base-commit override, no old-code variant.
TESTS = Path(__file__).resolve().parent
for name in ("modem_health_equivalence_test.sh", "modem_health_equivalence_test.cpp"):
    hit = re.search(r"\bgit\b|MODEM_HEALTH_BASE|MH_OLD", (TESTS / name).read_text())
    if hit:
        fail(f"{name} must not contain {hit.group(0)!r}")
header = [l for l in (TESTS / "fixtures" / "modem_health_transcript.txt").read_text().splitlines()
          if l.startswith("# ")]
if not any(re.search(r"\b[0-9a-f]{40}\b", l) for l in header):
    fail("fixtures/modem_health_transcript.txt header lacks a 40-hex-character commit hash")

# The thresholds and the enum have one home; the Connect duplicate is gone.
tail = {p: p.read_text() for p in SRC.rglob("*") if p.suffix in (".cpp", ".h")}
for name, home in (("MODEM_UNSTABLE_SLOW_TEARDOWN_MS", OWNER_HDR),
                   ("MODEM_UNSTABLE_RECOVERY_TEARDOWN_MS", OWNER_CPP),
                   ("MODEM_UNSTABLE_CONNECT_TIMEOUT_THRESHOLD", OWNER_CPP)):
    for path, text in tail.items():
        defined = re.search(rf"constexpr\s+[\w ]+\b{name}\s*=", text)
        if bool(defined) != (path == home):
            fail(f"{name} must be defined only in {home.name} ({path.relative_to(SRC)})")
    if "= 10000UL" not in OWNER_HDR.read_text() or "= 5000UL" not in owner \
            or "THRESHOLD = 2;" not in owner:
        fail("threshold values changed")
for path, text in tail.items():
    if re.search(r"\bmarkModemUnstableFromConnectTimeout\b|\benum ModemUnstableReason\b", text) \
            and path != OWNER_HDR:
        fail(f"{path.relative_to(SRC)} still holds the old duplicate or enum")

# Each call site uses the owner exactly once and includes its header.
for rel, call in (("state/State_Connect.cpp", "noteConnectTimeout"),
                  ("state/State_Connect.cpp", "noteConnectSuccess"),
                  ("state/State_Report.cpp", "clearAtDailyClose")):
    text = strip_comments_and_strings((SRC / rel).read_text())
    if len(re.findall(rf"\b{call}\(\);", text)) != 1:
        fail(f"{rel} must call {call}() exactly once")
for rel in ("state/State_Connect.cpp", "state/State_Report.cpp", "state/State_Sleep.cpp"):
    if '#include "power/ModemHealth.h"' not in (SRC / rel).read_text():
        fail(f"{rel} must include power/ModemHealth.h")

print("modem_health_owner_structural_test: passed")
