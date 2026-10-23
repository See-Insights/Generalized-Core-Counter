#!/bin/zsh
set -euo pipefail

# WO-2026-10-21-002 (Sleep S-3): moving the cloud-operations gate out of handleSleepingState() into
# cloudGatePass() (with its ten fields in the cloudGate struct) changes no behaviour.
# tests/sleep_cloud_gate_equivalence_test.cpp runs one scenario table through (a) the handler head
# (entry resets, CONNECTED+OPEN abort, firmware and occupancy exits, the inline gate block) extracted by
# marker text from the base State_Sleep.cpp and (b) the current handler head and cloudGatePass(),
# extracted by marker text from the current file, against the same fakes
# (tests/stubs/sleep_teardown/SleepExecutionFakes.h). Both transcripts, with the gate trace logs
# compiled out and in, must equal, byte for byte, fixtures/sleep_cloud_gate_transcript.txt ('# ' header
# lines stripped first). The extracted copies the harness compiled are read back and compared with a
# fresh extraction: COPY_MISMATCH on drift. Step (a) needs the base commit named in the fixture header.
# Mutants of a private copy of State_Sleep.cpp must each change the transcript.
# SLEEP_GATE_SRC_ROOT tests another src/ root; SLEEP_GATE_TAMPER_COPY=1 demonstrates COPY_MISMATCH.
# '--regenerate' rewrites the fixture from the base blocks (a).

tests_dir="${0:A:h}"
repo_root="${tests_dir:h}"
src_root="${SLEEP_GATE_SRC_ROOT:-$repo_root/src}"
fixture="$tests_dir/fixtures/sleep_cloud_gate_transcript.txt"
stubs="$tests_dir/stubs/sleep_teardown"
work="$(mktemp -d "${TMPDIR:-/tmp}/sleep_cloud_gate_equivalence.XXXXXX")"
trap 'rm -rf "$work"' EXIT

base="$(sed -n 's/^# Base commit: //p' "$fixture" 2>/dev/null || true)"
base="${base:-9578709}"
base_src="$work/base_State_Sleep.cpp"
have_base=0
if git -C "$repo_root" show "${base}:src/state/State_Sleep.cpp" > "$base_src" 2>/dev/null; then have_base=1; fi

cat > "$work/extract.py" <<'PY'
import re
import sys

kind, path, outdir = sys.argv[1:4]
text = open(path).read()


def span(start, end, keep_end=True):
    if text.count(start) != 1:
        sys.exit(f"MARKER_NOT_UNIQUE: {start!r} x{text.count(start)} in {path}")
    i = text.find(start)
    j = text.find(end, i + len(start))
    if j < 0:
        sys.exit(f"MARKER_MISSING: {end!r} in {path}")
    return text[i:j + len(end)] if keep_end else text[i:j]


out = {}
out["helpers"] = span("unsigned long computeCloudSyncTimeoutMs(uint16_t queueDepth) {", '    return "unknown";\n  }\n}\n')
region = span("  bool enteredState = (state != oldState);\n", "  if (SleepTeardown::run(", keep_end=False)
FIELDS = ("operationsCompleteLogged", "gateWasBlocking", "lastGateStatusLogMs", "lastGateBlockerMask",
          "lastGateBlockerBeforeRelease", "gateReleaseReasonKnown", "cloudSyncStartMs", "cloudSyncBudgetMs",
          "cloudSyncMaxQueueDepth", "lastQueueDepth")
if kind == "base":
    # The ten function statics become file-scope variables so the harness can read them after a pass.
    hoisted = []
    for name in FIELDS:
        m = re.findall(rf"^  (static [^\n]*\b{name} = [^\n]*;[^\n]*)\n", region, re.M)
        if len(m) != 1:
            sys.exit(f"STATIC_NOT_FOUND_ONCE: {name} in {path}")
        hoisted.append(m[0])
        region = region.replace(f"  {m[0]}\n", "", 1)
    out["hoist"] = "\n".join(hoisted) + "\n"
else:
    out["gate"] = span("struct {\n  bool operationsCompleteLogged", "  return false;\n}\n")
out["region"] = region
for k, v in out.items():
    open(f"{outdir}/{kind}_{k}.inc", "w").write(v)
PY

extract() {
  # $1 = base|cur, $2 = source file
  python3 "$work/extract.py" "$1" "$2" "$work"
}

if (( have_base )); then extract base "$base_src"; fi
extract cur "$src_root/state/State_Sleep.cpp"
if [[ "${SLEEP_GATE_TAMPER_COPY:-}" == 1 ]]; then echo "// drift" >> "$work/cur_gate.inc"; fi

build_run() {
  # $1 = gate trace (0|1), $2 = mode (a|b), $3 = output, $4 = work dir holding the .inc copies
  local trace="$1" mode="$2" out="$3" inc="$4"
  c++ -std=c++17 -Wall -Wextra -Wno-unused-parameter -Werror -DWiring_Cellular=1 -DHAL_PLATFORM_CELLULAR=1 \
    -DENABLE_GATE_TRACE="$trace" -I"$stubs" -I"$src_root" \
    -DBASE_HELPERS_INC="\"$inc/base_helpers.inc\"" -DBASE_HOIST_INC="\"$inc/base_hoist.inc\"" \
    -DBASE_REGION_INC="\"$inc/base_region.inc\"" -DCUR_HELPERS_INC="\"$inc/cur_helpers.inc\"" \
    -DCUR_GATE_INC="\"$inc/cur_gate.inc\"" -DCUR_REGION_INC="\"$inc/cur_region.inc\"" \
    "$tests_dir/sleep_cloud_gate_equivalence_test.cpp" -o "$out.bin"
  "$out.bin" "$mode" > "$out"
}

run_all() {
  # $1 = mode, $2 = output prefix, $3 = work dir holding the .inc copies
  build_run 1 "$1" "$2.trace1" "$3"
  build_run 0 "$1" "$2.trace0" "$3"
  cat "$2.trace1" "$2.trace0" > "$2"
}

if [[ "${1:-}" == "--regenerate" ]]; then
  (( have_base )) || { echo "NO_BASE_COMMIT: $base"; exit 1; }
  run_all a "$work/regen.txt" "$work"
  python3 - "$base" "$work/regen.txt" "$fixture" <<'PY'
import hashlib
import re
import sys

base, body_path, fixture = sys.argv[1:4]
body = open(body_path).read()
counts = re.findall(r"^SCENARIOS (\d+) PASSES (\d+) REACHED_RUN (\d+) HELD (\d+) EXITED (\d+) DISTINCT_EVENTS (\d+)$", body, re.M)
scen = sum(int(c[0]) for c in counts)
passes = sum(int(c[1]) for c in counts)
head = f"""# Frozen output of the pre-change cloud-operations gate (handler head and inline gate block): WO-2026-10-21-002 (Sleep S-3).
# Base commit: {base}
# Scenarios: {scen} (gate trace on {counts[0][0]} + gate trace off {counts[1][0]}); passes: {passes}
# Per-build summary (scenarios passes REACHED_RUN HELD EXITED distinct-events): trace-on {' '.join(counts[0])}; trace-off {' '.join(counts[1])}
# Body sha256: {hashlib.sha256(body.encode()).hexdigest()}
# Generated once by extracting the handler head by marker text from {base}:src/state/State_Sleep.cpp (git show),
# wrapping it in tests/sleep_cloud_gate_equivalence_test.cpp against tests/stubs/sleep_teardown, and running mode a.
"""
open(fixture, "w").write(head + body)
PY
  echo "fixture written: $fixture"
  exit 0
fi

grep -v '^# ' "$fixture" > "$work/expected.txt"
python3 - "$fixture" "$work/expected.txt" <<'PY'
import hashlib
import re
import sys

fixture, body_path = sys.argv[1:3]
head = open(fixture).read()
body = open(body_path).read()
want = re.search(r"^# Body sha256: (\w+)$", head, re.M).group(1)
if hashlib.sha256(body.encode()).hexdigest() != want:
    sys.exit("FIXTURE_HEADER_SHA_MISMATCH")
scen = int(re.search(r"^# Scenarios: (\d+)", head, re.M).group(1))
if sum(int(n) for n in re.findall(r"^SCENARIOS (\d+) ", body, re.M)) != scen:
    sys.exit("FIXTURE_HEADER_SCENARIO_COUNT_MISMATCH")
PY

run_all b "$work/new.txt" "$work"

# What the harness compiled must be what the file says: re-extract and compare every copy.
mkdir -p "$work/fresh"
extract_fresh() { python3 "$work/extract.py" "$1" "$2" "$work/fresh"; }
extract_fresh cur "$src_root/state/State_Sleep.cpp"
(( have_base )) && extract_fresh base "$base_src"
for f in "$work"/fresh/*.inc; do
  cmp -s "$f" "$work/${f:t}" || { echo "COPY_MISMATCH: ${f:t} compiled by the harness differs from the file"; exit 1; }
done

diff -u "$work/expected.txt" "$work/new.txt" > "$work/new.diff" || { head -60 "$work/new.diff"; echo "EQUIVALENCE_FAILED: State_Sleep.cpp differs from the frozen transcript"; exit 1; }

if (( have_base )); then
  run_all a "$work/old.txt" "$work"
  diff -u "$work/expected.txt" "$work/old.txt" > "$work/old.diff" || { head -60 "$work/old.diff"; echo "FIXTURE_NOT_FROM_BASE: the base blocks differ from the fixture ($base)"; exit 1; }
else
  echo "BASE_CHECK_SKIPPED: commit $base is not in this tree; only the fixture comparison ran"
fi

# The transcript is not trivial: every branch the moved code has shows up, and so do all outcomes.
for line in "ALERT 43" "ALERT 44" "ALERT 42" "ALERT 40" "CLOUD logLedgerSleepTimeoutState" \
            "LOG WARN SLEEP: Publish queue not empty - raising alert 43" \
            "LOG WARN SLEEP: OTA updates pending - raising alert 42" \
            "LOG WARN SLEEP: Webhook response not received - raising alert 40" \
            "LOG WARN GateFail: reason=queue" "LOG WARN GateFail: reason=ledger" "LOG WARN GateFail: reason=webhook" \
            "LOG WARN GateFail: reason=update" "LOG INFO GateRelease: wait=" "reason=unknown" "LOG INFO Gate: ok wait=" \
            "LOG INFO GateBlock: wait=" "reason=queue+ledger+webhook+update" "reason=none q=" "LOG INFO GateDetail: wait=" \
            "LOG INFO GateDetailState:" "LOG INFO GateReleaseDetail:" "LOG INFO Gate: q=" \
            "LOG INFO SLEEP: Cloud operations gate passed - ready to disconnect" "LOG INFO SLEEP: disconnect context" \
            "CYCLE markServiceEnd" "PROGRESS CLOUD_OPS_WAIT" "PROGRESS QUEUE_DRAIN" \
            "TRANSITION 3 firmware update in progress" "TRANSITION 4 occupancy change pending" \
            "TRANSITION 2 sleep-abort-open-hours" "CALL ensureSensorEnabled SLEEP abort: CONNECTED+OPEN" \
            "CALL publishStateTransition" "CRUMB 3" "RET reachedRun=1" "RET reachedRun=0 transitions=1" \
            "RET reachedRun=0 transitions=0" "process=1 watchdog=1" "FIELDS ops=1" "budget=70000" "budget=120000" "budget=37000"; do
  grep -qF -- "$line" "$work/new.txt" || { echo "LOG_LINE_NEVER_EXERCISED: $line"; exit 1; }
done

mutate_and_expect_diff() {
  # $1 = label, then old/new pairs applied to a private copy of State_Sleep.cpp
  local m="$work/mut_${1// /_}"; shift
  local label="${m:t}"
  mkdir -p "$m"
  cp "$src_root/state/State_Sleep.cpp" "$m/State_Sleep.cpp"
  python3 - "$m/State_Sleep.cpp" "$@" <<'PY'
import sys

path, pairs = sys.argv[1], sys.argv[2:]
text = open(path).read()
for old, new in zip(pairs[0::2], pairs[1::2]):
    if text.count(old) != 1:
        sys.exit(f"MUTATION_NOT_APPLIED: {old!r} x{text.count(old)}")
    text = text.replace(old, new)
open(path, "w").write(text)
PY
  cp "$work"/base_*.inc "$m/" 2>/dev/null || true
  python3 "$work/extract.py" cur "$m/State_Sleep.cpp" "$m"
  run_all b "$m/new.txt" "$m" > "$m/build.log" 2>&1 || { echo "MUTANT_DID_NOT_BUILD: $label"; cat "$m/build.log"; exit 1; }
  if diff -q "$work/expected.txt" "$m/new.txt" > /dev/null; then echo "MUTANT_SURVIVED: $label"; exit 1; fi
  echo "caught: $label"
}

mutate_and_expect_diff "alert 44 raised before 43" \
  "      if (!queueEmpty) {
        Log.warn(\"SLEEP: Publish queue not empty - raising alert 43\");
        RecoveryState::raiseAlert(43); // Queue drainage failure (highest priority)
      } else if (!ledgersSynced) {" \
  "      if (!ledgersSynced) {" \
  "        RecoveryState::raiseAlert(44); // Ledger sync timeout before sleep (minor - config already applied)
      } else if (!updatesChecked) {" \
  "        RecoveryState::raiseAlert(44); // Ledger sync timeout before sleep (minor - config already applied)
      } else if (!queueEmpty) {
        Log.warn(\"SLEEP: Publish queue not empty - raising alert 43\");
        RecoveryState::raiseAlert(43);
      } else if (!updatesChecked) {"
mutate_and_expect_diff "budget edge off by one" \
  "if (elapsedMs < cloudGate.cloudSyncBudgetMs) {" "if (elapsedMs <= cloudGate.cloudSyncBudgetMs) {"
mutate_and_expect_diff "lastQueueDepth progress update skipped" \
  "      cloudGate.lastQueueDepth = queueDepth;
" ""
mutate_and_expect_diff "occupancy exit keeps the gate timer" \
  "  if (session.occupancyChangeTriggered && !SleepTeardown::disconnectRequested()) {
    cloudGate.cloudSyncStartMs = 0; // leaving mid-gate: a later return starts its gate cleanly
" "  if (session.occupancyChangeTriggered && !SleepTeardown::disconnectRequested()) {
"
mutate_and_expect_diff "firmware exit keeps the gate timer" \
  "  if (!SleepTeardown::disconnectRequested() && firmwareUpdateInProgress) {
    cloudGate.cloudSyncStartMs = 0; // leaving mid-gate: a later return starts its gate cleanly
" "  if (!SleepTeardown::disconnectRequested() && firmwareUpdateInProgress) {
"
mutate_and_expect_diff "abort keeps the gate timer" \
  "    ensureSensorEnabled(\"SLEEP abort: CONNECTED+OPEN\");
    cloudGate.cloudSyncStartMs = 0;" "    ensureSensorEnabled(\"SLEEP abort: CONNECTED+OPEN\");"
mutate_and_expect_diff "entry reset of the release reason dropped" \
  "    cloudGate.gateReleaseReasonKnown = false;
    SleepTeardown::enter();" "    SleepTeardown::enter();"
mutate_and_expect_diff "ledger-timeout floor dropped" \
  "      cloudGate.cloudSyncBudgetMs = ConnectivityPolicy::OUTPUT_LEDGER_SYNC_TIMEOUT_MS;" "      cloudGate.cloudSyncBudgetMs = cloudGate.cloudSyncBudgetMs;"
mutate_and_expect_diff "markServiceEnd dropped" \
  "      Observability::cycleStats().markServiceEnd(millis());
" ""

echo "sleep_cloud_gate_equivalence_test: both transcripts match the frozen pre-change output, all mutations detected"
