#!/bin/zsh
set -euo pipefail

# WO-2026-10-02-001 item C: ConnSummary tells "modem off" from "searching",
# and nothing else changes - the label differs, every timing and decision
# stays exactly as in v31.
#
# Part 1 is BEHAVIOURAL against the real source: the real ConnAcquirePhase
# enum, connAcquirePhaseLabel() and classifyConnAcquirePhase() are extracted
# verbatim from src/state/State_Connect.cpp and compiled on the host, so the
# truth table below is the production one, not a copy of it.
#
# Part 1b is BEHAVIOURAL too: the real phase-accounting / phase-change block
# and the real cloud-recovery block are extracted verbatim and driven tick by
# tick, so recovery timing is measured on production code.
#
# Part 2 is structural: MODEM_OFF must be diagnostics only - no decision path
# may read it, and its time must count into connPhaseCellMs.

repo_root="${0:A:h:h}"
work="$TMPDIR/conn_phase_modem_off_test"
connect_src="$repo_root/src/state/State_Connect.cpp"

rm -rf "$work"
mkdir -p "$work"

cat > "$work/extract.py" <<'PY'
import sys

src, outdir = sys.argv[1], sys.argv[2]
lines = open(src).readlines()


def block(start_pred):
    start = None
    for i, line in enumerate(lines):
        if start_pred(line):
            start = i
            break
    if start is None:
        sys.exit("EXTRACT_FAILED: start not found")
    depth = 0
    opened = False
    for i in range(start, len(lines)):
        for ch in lines[i]:
            if ch == '{':
                depth += 1
            elif ch == '}':
                depth -= 1
        if depth > 0:
            opened = True
        if opened and depth == 0:
            return "".join(lines[start:i + 1])
    sys.exit("EXTRACT_FAILED: end not found")


def span(start_text, end_text):
    start = end = None
    for i, line in enumerate(lines):
        if start is None:
            if line.rstrip("\n") == start_text:
                start = i
        elif line.rstrip("\n") == end_text:
            end = i
            break
    if start is None or end is None:
        sys.exit("EXTRACT_FAILED: span %r .. %r not found" % (start_text, end_text))
    return "".join(lines[start:end + 1])


def pick(prefix):
    out = [l for l in lines if l.strip().startswith(prefix)]
    if not out:
        sys.exit("EXTRACT_FAILED: %r not found" % prefix)
    return "".join(l.strip() + "\n" for l in out)


enum = block(lambda l: l.startswith("enum class ConnAcquirePhase"))
label = block(lambda l: l.startswith("const char *connAcquirePhaseLabel("))
classify = block(lambda l: l.startswith("ConnAcquirePhase classifyConnAcquirePhase("))

for name, text in (("enum", enum), ("label", label), ("classify", classify)):
    if not text.strip():
        sys.exit("EXTRACT_FAILED: %s empty" % name)

# The phase-accounting lambda, the timing-phase normalisation and the
# phase-change block, verbatim and contiguous.
phase_block = span("  const unsigned long phaseNowMs = millis();",
                   "  lastConnPhase = currentConnPhase;")

# The cloud-recovery decision block, verbatim.
recovery = block(lambda l: l.strip() == "if (state == CONNECTING_STATE && connectRequested && !cloudConnected &&")

constants = pick("constexpr unsigned long CLOUD_RECOVER_STAGE")

with open(outdir + "/real.inc", "w") as f:
    f.write(enum)
    f.write("\n")
    f.write(label)
    f.write("\n")
    f.write(classify)
with open(outdir + "/constants.inc", "w") as f:
    f.write(constants)
with open(outdir + "/phase_block.inc", "w") as f:
    f.write(phase_block)
with open(outdir + "/recovery_block.inc", "w") as f:
    f.write(recovery)
print("Extracted the enum, connAcquirePhaseLabel(), classifyConnAcquirePhase(), "
      "the phase-change block and the cloud-recovery block verbatim from %s" % src)
PY

cat > "$work/main.cpp" <<'EOF'
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>

#define ENABLE_CONNECT_TRACE 0

#include "real.inc"

static int failures = 0;

static void check(const char *name, ConnAcquirePhase got, const char *want) {
  const char *label = connAcquirePhaseLabel(got);
  if (std::strcmp(label, want) == 0) {
    std::printf("  ok   %-56s -> %s\n", name, label);
  } else {
    std::printf("  FAIL %-56s -> %s, want %s\n", name, label, want);
    failures++;
  }
}

// ---- host stand-ins for the firmware environment the block runs in ----

static unsigned long g_nowMs = 0;
static unsigned long millis() { return g_nowMs; }

struct LogStub {
  void info(const char *, ...) {}
  void warn(const char *, ...) {}
};
static LogStub Log;

static const int CONNECTING_STATE = 1;
static int state = CONNECTING_STATE;
static bool connectRequested = true;

// The statics handleConnectingState() owns, with the values its state-entry
// reset gives them.
static bool connPhaseInitialized = false;
static ConnAcquirePhase lastConnPhase = ConnAcquirePhase::CELLULAR_ACQUIRE;
static unsigned long phaseStartMs = 0;
static uint32_t connPhaseCellMs = 0;
static uint32_t connPhaseNetMs = 0;
static uint32_t connPhaseCloudMs = 0;
static uint8_t cloudRecoverStage = 0;
static uint8_t cloudRecoverCount = 0;
static bool cloudRecoverStage1Done = false;
static bool cloudRecoverStage2Done = false;
static unsigned long lastConnectHeartbeatMs = 0;
static unsigned long lastConnDiagLogMs = 0;

#include "constants.inc"

static long stage1AtMs = -1;
static long stage2AtMs = -1;
static void requestCloudAcquireStage1Recovery() { stage1AtMs = (long)g_nowMs; }
static void requestCloudAcquireStage2Recovery() { stage2AtMs = (long)g_nowMs; }

static void resetConnect() {
  g_nowMs = 0;
  state = CONNECTING_STATE;
  connectRequested = true;
  connPhaseInitialized = false;
  lastConnPhase = ConnAcquirePhase::CELLULAR_ACQUIRE;
  phaseStartMs = 0;
  connPhaseCellMs = 0;
  connPhaseNetMs = 0;
  connPhaseCloudMs = 0;
  cloudRecoverStage = 0;
  cloudRecoverCount = 0;
  cloudRecoverStage1Done = false;
  cloudRecoverStage2Done = false;
  lastConnectHeartbeatMs = 0;
  lastConnDiagLogMs = 0;
  stage1AtMs = -1;
  stage2AtMs = -1;
}

struct Step {
  bool cellularReady;
  bool cellularOn;
  bool networkReady;
  bool cloudConnected;
  unsigned long atMs;
};

// One pass through the production phase-accounting and cloud-recovery code.
static void tick(const Step &s) {
  g_nowMs = s.atMs;
  const unsigned long elapsedMs = s.atMs;
  (void)elapsedMs;
  const bool cloudConnected = s.cloudConnected;
  const ConnAcquirePhase currentConnPhase = classifyConnAcquirePhase(
      s.cellularReady, s.cellularOn, s.networkReady, cloudConnected);
#include "phase_block.inc"
  (void)cloudAcquireElapsedMs;
#include "recovery_block.inc"
}

struct Result {
  long stage1AtMs;
  long stage2AtMs;
  uint32_t cellMs;
  uint32_t netMs;
  uint32_t cloudMs;
  const char *lastLabel;
};

// forceModemOn reproduces v31's classifier, which never reported MODEM_OFF.
static Result run(const Step *steps, int count, bool forceModemOn) {
  resetConnect();
  for (int i = 0; i < count; i++) {
    Step s = steps[i];
    if (forceModemOn) {
      s.cellularOn = true;
    }
    tick(s);
  }
  Result r = {stage1AtMs, stage2AtMs, connPhaseCellMs, connPhaseNetMs,
              connPhaseCloudMs, connAcquirePhaseLabel(lastConnPhase)};
  return r;
}

static void expectEq(const char *name, long got, long want) {
  if (got == want) {
    std::printf("  ok   %-56s -> %ld\n", name, got);
  } else {
    std::printf("  FAIL %-56s -> %ld, want %ld\n", name, got, want);
    failures++;
  }
}

static void expectLabel(const char *name, const char *got, const char *want) {
  if (std::strcmp(got, want) == 0) {
    std::printf("  ok   %-56s -> %s\n", name, got);
  } else {
    std::printf("  FAIL %-56s -> %s, want %s\n", name, got, want);
    failures++;
  }
}

// off = modem not powered; searching = powered but not ready;
// cloudAcq = cellular and network ready, cloud not connected.
static Step off(unsigned long atMs) { return Step{false, false, false, false, atMs}; }
static Step searching(unsigned long atMs) { return Step{false, true, false, false, atMs}; }
static Step netAcq(unsigned long atMs) { return Step{true, true, false, false, atMs}; }
static Step cloudAcq(unsigned long atMs) { return Step{true, true, true, false, atMs}; }
static Step connected(unsigned long atMs) { return Step{true, true, true, true, atMs}; }

static void compareWithV31(const char *name, const Step *steps, int count) {
  const Result fixed = run(steps, count, false);
  const Result v31 = run(steps, count, true);
  char buf[128];
  std::snprintf(buf, sizeof(buf), "%s: stage1 fires when v31 does", name);
  expectEq(buf, fixed.stage1AtMs, v31.stage1AtMs);
  std::snprintf(buf, sizeof(buf), "%s: stage2 fires when v31 does", name);
  expectEq(buf, fixed.stage2AtMs, v31.stage2AtMs);
  std::snprintf(buf, sizeof(buf), "%s: connPhaseCellMs matches v31", name);
  expectEq(buf, (long)fixed.cellMs, (long)v31.cellMs);
  std::snprintf(buf, sizeof(buf), "%s: connPhaseNetMs matches v31", name);
  expectEq(buf, (long)fixed.netMs, (long)v31.netMs);
  std::snprintf(buf, sizeof(buf), "%s: connPhaseCloudMs matches v31", name);
  expectEq(buf, (long)fixed.cloudMs, (long)v31.cloudMs);
}

int main() {
  std::printf("--- Part 1: classifyConnAcquirePhase(ready, isOn, net, cloud) ---\n");

  check("modem not ready and isOn() false",
        classifyConnAcquirePhase(false, false, false, false), "MODEM_OFF");
  check("modem not ready and isOn() false, network up",
        classifyConnAcquirePhase(false, false, true, false), "MODEM_OFF");
  check("modem not ready and isOn() true (searching)",
        classifyConnAcquirePhase(false, true, false, false), "CELLULAR_ACQUIRE");
  check("modem ready, network not ready",
        classifyConnAcquirePhase(true, true, false, false), "NETWORK_ACQUIRE");
  check("modem and network ready, cloud not connected",
        classifyConnAcquirePhase(true, true, true, false), "CLOUD_ACQUIRE");
  check("cloud connected wins over everything",
        classifyConnAcquirePhase(false, false, false, true), "CONNECTED");

  std::printf("\n--- Part 1b: timing is v31's; only the label is new ---\n");

  // 1. The Stage 7 reproduction: modem off at 0 s, powered but not ready at
  //    50 s, cloud acquisition at 70 s. v31 fires recovery stage 1 at 70 s
  //    because MODEM_OFF -> CELLULAR_ACQUIRE is not a timing boundary.
  {
    const Step steps[] = {off(0), searching(50000), cloudAcq(70000)};
    const Result r = run(steps, 3, false);
    expectEq("reproduction: recovery stage 1 fires at 70 s", r.stage1AtMs, 70000);
    const Result v31 = run(steps, 3, true);
    expectEq("reproduction: v31 fires at the same instant", v31.stage1AtMs, 70000);
  }

  // 2. v31 equivalence over sequences that mix MODEM_OFF and CELLULAR_ACQUIRE
  //    in any order before CLOUD_ACQUIRE.
  {
    const Step a[] = {off(0), searching(50000), cloudAcq(70000),
                      cloudAcq(140000), connected(150000)};
    compareWithV31("off then searching", a, 5);

    const Step b[] = {searching(0), off(20000), searching(40000), off(55000),
                      cloudAcq(70000), cloudAcq(130000), cloudAcq(200000),
                      connected(210000)};
    compareWithV31("searching/off alternating", b, 8);

    const Step c[] = {off(0), off(10000), off(30000), netAcq(45000),
                      cloudAcq(60000), cloudAcq(125000), connected(130000)};
    compareWithV31("off throughout, then network then cloud", c, 7);

    const Step d[] = {searching(0), searching(30000), off(61000),
                      cloudAcq(90000), cloudAcq(155000), cloudAcq(215000),
                      connected(220000)};
    compareWithV31("searching then off then cloud", d, 7);

    const Step e[] = {off(0), cloudAcq(65000), off(80000), searching(95000),
                      cloudAcq(110000), cloudAcq(175000), connected(240000)};
    compareWithV31("cloud, back to off, then cloud again", e, 7);
  }

  // 3. The label still distinguishes the two.
  {
    const Step steps[] = {off(0), off(10000)};
    const Result r = run(steps, 2, false);
    expectLabel("after a MODEM_OFF interval, last phase is MODEM_OFF",
                r.lastLabel, "MODEM_OFF");

    const Step mixed[] = {off(0), searching(10000)};
    const Result m = run(mixed, 2, false);
    expectLabel("after the modem powers up, last phase is CELLULAR_ACQUIRE",
                m.lastLabel, "CELLULAR_ACQUIRE");

    const Step back[] = {searching(0), off(10000)};
    const Result b = run(back, 2, false);
    expectLabel("when the modem drops out again, last phase is MODEM_OFF",
                b.lastLabel, "MODEM_OFF");
    expectEq("a label-only change flushes nothing into connPhaseCellMs",
             (long)b.cellMs, 0);
  }

  if (failures != 0) {
    std::printf("\nFAILED: %d check(s)\n", failures);
    return 1;
  }
  std::printf("\nParts 1 and 1b passed\n");
  return 0;
}
EOF

build_and_run() {
  local src="$1"
  local outdir="$2"
  mkdir -p "$outdir"
  python3 "$work/extract.py" "$src" "$outdir"
  clang++ -std=c++17 -Wall -Wextra -pedantic -I"$outdir" -I"$work" "$work/main.cpp" -o "$outdir/bin"
  "$outdir/bin"
}

build_and_run "$connect_src" "$work/real"

echo ""
echo "--- Part 2: MODEM_OFF is diagnostics only ---"

fail() {
  echo "FAILED: $1" >&2
  exit 1
}

# Every reference to the phase, anywhere in src/, must be one of the five
# diagnostics-only sites. A decision that branches on it fails here.
cat > "$work/scan.py" <<'SCAN'
import os
import re
import sys

root = sys.argv[1]
allowed = (
    re.compile(r"^MODEM_OFF,$"),                            # the enum value
    re.compile(r"^case ConnAcquirePhase::MODEM_OFF:$"),     # label + phase accounting
    re.compile(r'^return "MODEM_OFF";$'),                   # the label text
    re.compile(r"^return cellularOn \? ConnAcquirePhase::CELLULAR_ACQUIRE"
               r" : ConnAcquirePhase::MODEM_OFF;$"),        # the classifier
    re.compile(r"^auto timingPhase = \[\]\(ConnAcquirePhase phase\) \{ return phase"
               r" == ConnAcquirePhase::MODEM_OFF \? ConnAcquirePhase::CELLULAR_ACQUIRE"
               r" : phase; \};$"),                          # timing normalisation
)
interesting = re.compile(r"ConnAcquirePhase::MODEM_OFF"
                         r"|^\s*MODEM_OFF,\s*$"
                         r'|^\s*return "MODEM_OFF";\s*$')

bad = []
seen = 0
for dirpath, _dirs, files in os.walk(root):
    for name in files:
        if not name.endswith((".cpp", ".h")):
            continue
        path = os.path.join(dirpath, name)
        for n, line in enumerate(open(path).readlines(), 1):
            if not interesting.search(line):
                continue
            text = line.strip()
            if text.startswith("//") or text.startswith("*"):
                continue
            seen += 1
            if not any(p.match(text) for p in allowed):
                bad.append("%s:%d: %s" % (os.path.relpath(path, root), n, text))

if seen == 0:
    print("MODEM_OFF is not referenced anywhere in src/")
    sys.exit(1)
for b in bad:
    print(b)
sys.exit(1 if bad else 0)
SCAN

python3 "$work/scan.py" "$repo_root/src" || \
  fail "MODEM_OFF is read outside the enum, the classifier, the label, the phase accounting and the timing normalisation"
echo "OK: MODEM_OFF appears only in the enum, the classifier, the label, the phase accounting and the timing normalisation"

# Its time must count into connPhaseCellMs, so ConnSummary's fields are unchanged.
accounting=$(python3 - "$connect_src" <<'PY'
import sys
text = open(sys.argv[1]).read()
start = text.index("auto addPhaseElapsed")
end = text.index("};", start)
print(text[start:end])
PY
)
if ! print -r -- "$accounting" | grep -A2 "case ConnAcquirePhase::MODEM_OFF:" | grep -q "connPhaseCellMs"; then
  fail "MODEM_OFF time must count into connPhaseCellMs"
fi
echo "OK: MODEM_OFF time counts into connPhaseCellMs (ConnSummary fields unchanged)"

# The decision that reads a phase still reads CLOUD_ACQUIRE only.
decisions=$(grep -n "currentConnPhase ==" "$connect_src" || true)
if print -r -- "$decisions" | grep -q "MODEM_OFF"; then
  fail "a decision branches on MODEM_OFF"
fi
if ! print -r -- "$decisions" | grep -q "ConnAcquirePhase::CLOUD_ACQUIRE"; then
  fail "the CLOUD_ACQUIRE decision path is missing"
fi
echo "OK: the only phase a decision reads is still CLOUD_ACQUIRE"

# isOn() must be sampled next to ready(), under the cellular guard.
sample=$(grep -n -A6 "bool cellularReady = true;" "$connect_src")
for needle in "bool cellularOn = true;" "#if Wiring_Cellular" "cellularReady = Cellular.ready();" "cellularOn = Cellular.isOn();"; do
  print -r -- "$sample" | grep -q "$needle" || \
    fail "Cellular.isOn() must be sampled next to Cellular.ready() under #if Wiring_Cellular (missing: $needle)"
done
echo "OK: Cellular.isOn() is sampled next to Cellular.ready(), under #if Wiring_Cellular"

echo ""
echo "--- Part 3: mutation - a MODEM_OFF boundary must be caught ---"

# Round 1's behaviour, on a copy: a MODEM_OFF <-> CELLULAR_ACQUIRE change is a
# timing boundary again. The source file itself is never written to.
mkdir -p "$work/mutant"
python3 - "$connect_src" "$work/mutant/State_Connect.cpp" <<'PY'
import sys
src, dst = sys.argv[1], sys.argv[2]
text = open(src).read()
needle = "timingPhase(currentConnPhase) != timingPhase(lastConnPhase)"
if needle not in text:
    sys.exit("MUTATION_FAILED: timing normalisation not found in the phase-change block")
open(dst, "w").write(text.replace(needle, "currentConnPhase != lastConnPhase", 1))
PY

if build_and_run "$work/mutant/State_Connect.cpp" "$work/mutant-out" > "$work/mutant.log" 2>&1; then
  cat "$work/mutant.log"
  fail "the mutant passed: a MODEM_OFF timing boundary is not detected"
fi
grep -q "FAIL" "$work/mutant.log" || \
  fail "the mutant failed for the wrong reason (no behavioural FAIL): see $work/mutant.log"
echo "OK: restoring round 1's boundary behaviour fails the timing checks"
grep "FAIL" "$work/mutant.log" | head -3 | sed 's/^/     /'

rm -rf "$work"
echo ""
echo "conn_phase_modem_off_test (real-source behaviour + structural + mutation) passed"
