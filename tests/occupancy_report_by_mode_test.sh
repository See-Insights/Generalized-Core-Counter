#!/bin/zsh
set -euo pipefail

# WO-2026-10-07-004: every occupancy change (start or end) reports right away
# when the mode in use is INTERMITTENT_KEEP_ALIVE or CONNECTED, and in no other
# mode (INTERMITTENT, DISCONNECTED and a battery-downgraded KEEP_ALIVE wait for
# the scheduled report). A change is reported once, on every Report path.
#
# The blocks of the real sources that make the decision are extracted
# byte-for-byte (see occupancy_report_by_mode_test.cpp for the list and the
# host limits). A separate step re-reads src/ and fails if any extracted block
# differs from it. The blocks of handleSleepingState() and
# handleReportingState() are emitted in their real source order, so a block
# moved in src/ changes the behaviour the test drives.
# Set OCC_REPORT_SRC_ROOT to a copy of src/ to test a mutant; the clean run
# below then rebuilds the test against deliberately broken copies and each one
# must make the *targeted* checks FAIL at run time (a mutant that does not
# compile is a broken mutation, not a caught one).

repo_root="${0:A:h:h}"
work="$(mktemp -d "${TMPDIR:-/tmp}/occupancy_report_by_mode.XXXXXX")"
trap 'rm -rf "$work"' EXIT

extract_and_build() {
  # $1 = src root, $2 = work dir, $3 = binary
  local src_root="$1" dir="$2" binary="$3"
  mkdir -p "$dir"
  python3 - "$src_root" "$dir" > "$dir/flags.txt" <<'PY'
import glob
import json
import re
import sys

src_root, out_dir = sys.argv[1], sys.argv[2]


def read(rel):
    return open(f"{src_root}/{rel}").read()


def scrub(text):
    """Blank out comments and string/char literals, preserving offsets."""
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == '/' and i + 1 < n and text[i + 1] == '/':
            while i < n and text[i] != '\n':
                out[i] = ' '
                i += 1
        elif c == '/' and i + 1 < n and text[i + 1] == '*':
            while i < n and not (text[i] == '*' and i + 1 < n and text[i + 1] == '/'):
                if text[i] != '\n':
                    out[i] = ' '
                i += 1
            for j in range(i, min(i + 2, n)):
                out[j] = ' '
            i += 2
        elif c in '"\'':
            quote = c
            out[i] = ' '
            i += 1
            while i < n and text[i] != quote:
                if text[i] == '\\':
                    out[i] = ' '
                    i += 1
                if i < n and text[i] != '\n':
                    out[i] = ' '
                i += 1
            if i < n:
                out[i] = ' '
                i += 1
        else:
            i += 1
    return "".join(out)


def die(msg):
    sys.exit("EXTRACT_FAILED: " + msg)


class Src:
    def __init__(self, rel):
        self.rel = rel
        self.text = read(rel)
        self.scrubbed = scrub(self.text)

    def find(self, needle, start=0, required=True):
        idx = self.text.find(needle, start)
        if idx < 0 and required:
            die(f"{needle!r} not found in {self.rel}")
        return idx

    def find_code(self, needle, start=0):
        """First occurrence outside comments and string literals."""
        idx = self.scrubbed.find(needle, start)
        if idx < 0:
            die(f"{needle!r} not found as code in {self.rel}")
        return idx

    def close_of(self, open_idx):
        depth = 0
        for i in range(open_idx, len(self.scrubbed)):
            if self.scrubbed[i] == '{':
                depth += 1
            elif self.scrubbed[i] == '}':
                depth -= 1
                if depth == 0:
                    return i
        die(f"unbalanced braces in {self.rel}")

    def open_after(self, idx):
        o = self.scrubbed.find('{', idx)
        if o < 0:
            die(f"no opening brace after offset {idx} in {self.rel}")
        return o

    def stmt(self, needle, start=0, with_else=False, semicolon=False):
        """Offsets of the brace statement beginning at needle, plus its else branch."""
        s = self.find(needle, start)
        end = self.close_of(self.open_after(s))
        if with_else:
            tail = re.match(r"\s*else\s*\{", self.scrubbed[end + 1:])
            if tail:
                end = self.close_of(end + 1 + tail.end() - 1)
        if semicolon and self.scrubbed[end + 1:end + 2] == ';':
            end += 1
        return s, end


manifest = []


def emit(name, src, start, end):
    """Write src.text[start..end] (inclusive) byte-for-byte as one include file."""
    with open(f"{out_dir}/{name}", "w") as f:
        f.write(src.text[start:end + 1])
    manifest.append([name, src.rel, start, end + 1])


def write(name, body):
    open(f"{out_dir}/{name}", "w").write(body + "\n")


common = Src("state/State_Common.h")
modes = Src("state/State_Modes.cpp")
idle = Src("state/State_Idle.cpp")
sleep = Src("state/State_Sleep.cpp")
report = Src("state/State_Report.cpp")

# --- State_Common.h: the predicate and the real close/debounce helpers --------
s, e = common.stmt("inline bool reportsOccupancyChangesNow() {")
emit("extracted_predicate.inc", common, s, e)
s, e = common.stmt("struct OccupancyCloseResult {", semicolon=True)
emit("extracted_close_1.inc", common, s, e)
s, e = common.stmt("inline uint32_t occupancyDebounceMs() {")
emit("extracted_close_2.inc", common, s, e)
s, e = common.stmt("inline OccupancyCloseResult closeOccupancySessionSafely(const char *path, time_t closeAt = Time.now()) {")
emit("extracted_close_3.inc", common, s, e)

# --- State_Modes.cpp: the main-loop handler (unmodified; the test stubs SensorManager)
s, e = modes.stmt("void updateOccupancyState() {")
emit("extracted_modes_update.inc", modes, s, e)
s, e = modes.stmt("void handleOccupancyMode() {")
emit("extracted_modes_handle.inc", modes, s, e)

# --- State_Idle.cpp: the head of handleIdleState(), through the consumer ------
fn = idle.find("void handleIdleState() {")
fn_open = idle.open_after(fn)
occ_start, occ_end = idle.stmt("if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY) {", fn_open)
head_end = occ_end
consumer = re.match(r"\s*(//[^\n]*\n\s*)*if \(session\.occupancyChangeTriggered\) \{", idle.text[occ_end + 1:])
idle_consumer_present = 0
if consumer:
    c_start = occ_end + 1 + consumer.end() - len("if (session.occupancyChangeTriggered) {")
    head_end = idle.close_of(idle.open_after(c_start))
    idle_consumer_present = 1
emit("extracted_idle_head.inc", idle, fn_open + 1, head_end)

# Order: the consumer follows the occupancy block and precedes the park-closed sleep.
idle_order_ok = 0
if idle_consumer_present:
    park = idle.find('"park closed"', fn_open)
    idle_order_ok = int(occ_end < c_start < park)

# --- State_Sleep.cpp: the blocks of handleSleepingState(), in source order -----
sfn = sleep.find("void handleSleepingState() {")
decl = sleep.find("static bool disconnectRequested = false;", sfn)
reset_end = sleep.find('Log.warn("SLEEP: disconnectRequested latched without timestamp', decl)
teardown_site = sleep.find("disconnectRequested = true;", decl)
wake_site = sleep.find("bool pirWake = (wakePin == intPin);", sfn)

sleep_segments = []
pend_idx = sleep.find("if (session.occupancyChangeTriggered", sfn, required=False)
sleep_consumer_present = int(pend_idx >= 0)
sleep_consumer_gated = 0
sleep_below_reset = 0
if pend_idx >= 0:
    ps, pe = sleep.stmt("if (session.occupancyChangeTriggered", sfn)
    emit("extracted_sleep_pending.inc", sleep, ps, pe)
    sleep_consumer_gated = int("!disconnectRequested" in sleep.text[ps:ps + 80])
    sleep_below_reset = int(pend_idx > reset_end)
    sleep_segments.append((ps, '#include "extracted_sleep_pending.inc"'))
gate_site = sleep.find("if (Particle.connected() && !disconnectRequested) {", decl)
sleep_segments.append((gate_site, "if (!modelGate()) return;  // model: the cloud-ops gate wait"))
sleep_segments.append((teardown_site, "if (!modelTeardown()) return;  // model: teardown request or wait"))
sleep_segments.append((wake_site, "modelSleep();  // model: System.sleep() returns"))

s, e = sleep.stmt("if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY && signalLEDTimeRemaining() == 0 && signalLEDStatus()) {", sfn)
emit("extracted_sleep_wake_end.inc", sleep, s, e)
sleep_segments.append((s, '#include "extracted_sleep_wake_end.inc"'))
pir = sleep.find('thrashGuard.markProgress("PIR_WAKE_OCCUPANCY")', sfn)
s, e = sleep.stmt("if (!CurrentReadings::get_occupied()) {", pir, with_else=True)
emit("extracted_sleep_wake_start.inc", sleep, s, e)
sleep_segments.append((s, 'if (pirWake) {\n#include "extracted_sleep_wake_start.inc"\n}'))
s, e = sleep.stmt("if (timerWake) {", sfn)
emit("extracted_sleep_timer.inc", sleep, s, e)
sleep_segments.append((s, '#include "extracted_sleep_timer.inc"'))
write("sleep_pass.inc", "\n".join(code for _, code in sorted(sleep_segments)))

# Before every standby-teardown request, every System.sleep() and every
# sleep-or-suppress decision.
decision_offsets = [
    sleep.find_code("System.sleep(", sfn),
    sleep.find_code("Connectivity::request", sfn),
    sleep.find('"sleep-timer-occupied-suppress-report"', sfn),
    sleep.find('"sleep-pir-return-to-sleep"', sfn),
    sleep.find('"sleep-timer-report"', sfn),
    sleep.find('closeOccupancySessionSafely("sleep")', sfn),
]
sleep_order_ok = int(pend_idx >= 0 and all(pend_idx < f for f in decision_offsets))

# --- State_Report.cpp: the blocks of handleReportingState(), in source order ---
rfn = report.find("void handleReportingState() {")
pub = report.find("publishData(due ? boundary - 1 : 0);", rfn)
take = re.compile(
    r"const bool occupancyChangeTriggered = session\.occupancyChangeTriggered;"
    r"(?:[ \t]*\n[ \t]*session\.occupancyChangeTriggered = false;)?").search(report.text, rfn)
if not take:
    die("State_Report.cpp lost its entry take of occupancyChangeTriggered")
take_s, take_e = take.start(), take.end() - 1
emit("extracted_report_take.inc", report, take_s, take_e)
alert_s, alert_e = report.stmt("if (Clock::openness() == Clock::Openness::Open && !session.suppressAlert40ThisSession) {", rfn)
emit("extracted_report_alert40.inc", report, alert_s, alert_e)
svc_s = report.find("bool serviceRequestTriggered = session.serviceRequestTriggered;", rfn)
_, svc_e = report.stmt("if (serviceRequestTriggered) {", svc_s)
emit("extracted_report_service_take.inc", report, svc_s, svc_e)
cfg_s, cfg_e = report.stmt("if (!Config::isValid(true)) {", rfn)
emit("extracted_report_config_invalid.inc", report, cfg_s, cfg_e)

group_s, group_e = report.stmt("if (!Particle.connected()) {", rfn)
chain_s = report.find("if (serviceRequestTriggered) {", svc_e + 1)
if not (group_s < chain_s < group_e):
    die("the service-request/occupancy chain is no longer inside the not-connected branch")
chain_e = report.close_of(report.open_after(chain_s))
tail = re.match(r"\s*else if \((?:session\.)?occupancyChangeTriggered\) \{", report.scrubbed[chain_e + 1:])
if not tail:
    die("the occupancy branch no longer follows the service-request branch")
chain_e = report.close_of(chain_e + 1 + tail.end() - 1)
emit("extracted_report_chain.inc", report, chain_s, chain_e)

m = re.match(r"\s*else\s*\{", report.scrubbed[group_e + 1:])
if not m:
    die("handleReportingState() lost its else branch for the connected case")
else_open = group_e + 1 + m.end() - 1
else_close = report.close_of(else_open)
if '"already connected"' not in report.text[else_open:else_close]:
    die("the connected branch no longer ends in 'already connected'")
emit("extracted_report_connected.inc", report, else_open + 1, else_close - 1)

group = (
    "if (!Particle.connected()) {\n"
    '#include "extracted_report_chain.inc"\n'
    "  else {\n"
    '    transitionTo(IDLE_STATE, "not aligned");  // model: the rest of the chain\n'
    "  }\n"
    "} else {\n"
    '#include "extracted_report_connected.inc"\n'
    "}"
)
report_segments = [
    (pub, "modelPublish();  // model: publishData()"),
    (take_s, '#include "extracted_report_take.inc"'),
    (alert_s, '#include "extracted_report_alert40.inc"'),
    (svc_s, '#include "extracted_report_service_take.inc"'),
    (cfg_s, '#include "extracted_report_config_invalid.inc"'),
    (group_s, group),
]
write("report_pass.inc", "\n".join(code for _, code in sorted(report_segments)))
report_take_ok = int(pub < take_s < min(alert_s, svc_s, cfg_s, group_s))

# --- Whole-tree checks --------------------------------------------------------
call_sites = 0
old_expr = 0
for path in glob.glob(f"{src_root}/state/*.cpp"):
    t = open(path).read()
    call_sites += len(re.findall(r"reportNow\s*=\s*reportsOccupancyChangesNow\(\)\s*;", t))
    old_expr += len(re.findall(r"reportNow\s*=\s*\(?\s*PowerManager::instance\(\)\.effectiveConnectionMode\(\)", t))

json.dump(manifest, open(f"{out_dir}/manifest.json", "w"))

print(f"-DSLEEP_CONSUMER_PRESENT={sleep_consumer_present}")
print(f"-DSLEEP_CONSUMER_BELOW_RESET={sleep_below_reset}")
print(f"-DSLEEP_CONSUMER_GATED={sleep_consumer_gated}")
print(f"-DSLEEP_CONSUMER_BEFORE_SLEEP_DECISIONS={sleep_order_ok}")
print(f"-DIDLE_CONSUMER_PRESENT={idle_consumer_present}")
print(f"-DIDLE_CONSUMER_AFTER_OCCUPANCY_BLOCK={idle_order_ok}")
print(f"-DREPORT_TAKE_AFTER_PUBLISH_BEFORE_EXITS={report_take_ok}")
print(f"-DPREDICATE_CALL_SITES={call_sites}")
print(f"-DOLD_REPORTNOW_EXPR_REMAINING={old_expr}")
PY

  # Independent check: every extracted block equals the src/ text it came from.
  python3 - "$src_root" "$dir" <<'PY'
import json
import sys

src_root, out_dir = sys.argv[1], sys.argv[2]
bad = 0
for name, rel, start, end in json.load(open(f"{out_dir}/manifest.json")):
    src = open(f"{src_root}/{rel}").read()
    block = open(f"{out_dir}/{name}").read()
    if block != src[start:end] or block not in src:
        print(f"COPY_MISMATCH: {name} differs from {rel}[{start}:{end}]", file=sys.stderr)
        bad += 1
sys.exit(1 if bad else 0)
PY

  clang++ -std=c++17 -Wall -Wextra -pedantic -Wno-tautological-constant-out-of-range-compare -Wno-format \
    $(cat "$dir/flags.txt") \
    -I"$dir" \
    -I"$repo_root/tests/stubs/connection_mode_overrides" \
    -I"$repo_root/tests/stubs/persistence_backend" \
    -I"$src_root" \
    "$repo_root/tests/occupancy_report_by_mode_test.cpp" \
    "$src_root/MyPersistentData.cpp" \
    "$src_root/power/BatteryAuthorityCommand.cpp" \
    "$src_root/power/PowerManager.cpp" \
    "$src_root/power/BatteryAuthority.cpp" \
    "$src_root/reporting/BatteryTierGuard.cpp" \
    "$src_root/power/PowerTier.cpp" \
    "$src_root/power/BatteryHealth.cpp" \
    -o "$binary"
}

src_root="${OCC_REPORT_SRC_ROOT:-$repo_root/src}"

# --- Clean run ----------------------------------------------------------------
extract_and_build "$src_root" "$work/clean" "$work/clean_test"
"$work/clean_test"

[[ -n "${OCC_REPORT_SKIP_MUTATIONS:-}" ]] && exit 0

# --- Mutation runs ------------------------------------------------------------
mutate_and_expect_failure() {
  # $1 = name, $2 = regex the failing checks must match, $3 = rel file,
  # then pairs of: old literal, new literal (each applied once, in order)
  local name="$1" expect="$2" rel="$3"
  shift 3
  local dir="$work/mut_${name// /_}"
  mkdir -p "$dir"
  cp -R "$src_root" "$dir/src"
  python3 - "$src_root/$rel" "$dir/src/$rel" "$@" <<'PY'
import sys

src, dst = sys.argv[1:3]
pairs = sys.argv[3:]
text = open(src).read()
for old, new in zip(pairs[0::2], pairs[1::2]):
    if text.count(old) < 1:
        sys.exit(f"MUTATION_FAILED: {old!r} not found in {src}")
    text = text.replace(old, new, 1)
open(dst, "w").write(text)
PY
  if ! extract_and_build "$dir/src" "$dir/w" "$dir/mutant" 2>"$dir/build.log"; then
    echo "FAIL (mutation harness): mutant '$name' did not build - the mutation is malformed" >&2
    cat "$dir/build.log" >&2
    exit 1
  fi
  if "$dir/mutant" >"$dir/run.log" 2>&1; then
    echo "FAIL (mutation): '$name' was NOT detected - the test has a blind spot" >&2
    exit 1
  fi
  if ! grep -Eq "^FAIL: .*($expect)" "$dir/run.log"; then
    echo "FAIL (mutation): '$name' failed, but not in its targeted check /$expect/" >&2
    head -5 "$dir/run.log" >&2
    exit 1
  fi
  echo "OK (mutation): '$name' caught by /$expect/ ($(grep -c '^FAIL:' "$dir/run.log") failed checks, e.g. $(grep -m1 -E "^FAIL: .*($expect)" "$dir/run.log" | cut -c1-110))"
}

sleep_pending='  if (session.occupancyChangeTriggered && !disconnectRequested) {
    cloudSyncStartMs = 0; // leaving mid-gate: a later return starts its gate cleanly
    transitionTo(REPORTING_STATE, "occupancy change pending");
    return;
  }
'
idle_pending='  if (session.occupancyChangeTriggered) {
    transitionTo(REPORTING_STATE, "occupancy change pending");
    return;
  }
'
entry_take='  const bool occupancyChangeTriggered = session.occupancyChangeTriggered;
  session.occupancyChangeTriggered = false;
'
predicate_old="mode == SystemConfig::INTERMITTENT_KEEP_ALIVE || mode == SystemConfig::CONNECTED"

# Round 1's mutations, re-run (the already-connected clear is gone; its mutation
# now removes the entry clear, which the connected case depends on).
mutate_and_expect_failure "predicate drops CONNECTED" "." "state/State_Common.h" \
  "$predicate_old" "mode == SystemConfig::INTERMITTENT_KEEP_ALIVE"
mutate_and_expect_failure "predicate adds INTERMITTENT" "." "state/State_Common.h" \
  "$predicate_old" "$predicate_old || mode == SystemConfig::INTERMITTENT"
mutate_and_expect_failure "predicate adds DISCONNECTED" "." "state/State_Common.h" \
  "$predicate_old" "mode != SystemConfig::INTERMITTENT"
mutate_and_expect_failure "sleep-prep check removed" "soak:" "state/State_Sleep.cpp" \
  "$sleep_pending" ""
mutate_and_expect_failure "Idle consumer removed" "latched" "state/State_Idle.cpp" \
  "$idle_pending" ""
mutate_and_expect_failure "entry clear removed" "connected-once:" "state/State_Report.cpp" \
  "$entry_take" "  const bool occupancyChangeTriggered = session.occupancyChangeTriggered;
"

# Round 2's mutations.
mutate_and_expect_failure "flag take moved below the early exits" "norepeat:" "state/State_Report.cpp" \
  "$entry_take" "" \
  "  if (!Particle.connected()) {
" "$entry_take  if (!Particle.connected()) {
"
mutate_and_expect_failure "sleep check ignores disconnectRequested" "teardown:" "state/State_Sleep.cpp" \
  "if (session.occupancyChangeTriggered && !disconnectRequested) {" "if (session.occupancyChangeTriggered) {"
mutate_and_expect_failure "mid-gate exit keeps the gate timer" "midgate:" "state/State_Sleep.cpp" \
  "    cloudSyncStartMs = 0; // leaving mid-gate: a later return starts its gate cleanly
    transitionTo(REPORTING_STATE, \"occupancy change pending\");" \
  "    transitionTo(REPORTING_STATE, \"occupancy change pending\");"
mutate_and_expect_failure "sleep check moved below suppress" "soak:" "state/State_Sleep.cpp" \
  "$sleep_pending" "" \
  "    // For PIR wakes, check if reporting is also due (opportunistic reporting)
" "$sleep_pending    // For PIR wakes, check if reporting is also due (opportunistic reporting)
"
mutate_and_expect_failure "connected clear restored, entry take removed" "norepeat:" "state/State_Report.cpp" \
  "$entry_take" "  const bool occupancyChangeTriggered = session.occupancyChangeTriggered;
" \
  "    } else if (occupancyChangeTriggered) {
" "    } else if (occupancyChangeTriggered) {
      session.occupancyChangeTriggered = false;
" \
  '    transitionTo(IDLE_STATE, "already connected");' '    session.occupancyChangeTriggered = false;
    transitionTo(IDLE_STATE, "already connected");'

echo "occupancy_report_by_mode_test: clean run passed and all mutations detected"
