#!/bin/zsh
set -euo pipefail

# WO-2026-10-21-003 (Sleep S-4): moving the sleep plan and the nightly heap guard out of
# handleSleepingState() into sleepPlan() and nightlyHeapGuard() changes no behaviour.
# tests/sleep_plan_equivalence_test.cpp runs one scenario table (one pass each) through (a) the handler's
# plan block, hibernateAttempt call and guard block extracted by marker text from the base State_Sleep.cpp
# and (b) the current handler's call lines with sleepPlan() and nightlyHeapGuard(), extracted by marker text
# from the current file, against the same fakes (tests/stubs/sleep_teardown/SleepExecutionFakes.h). The
# release build runs the full table; one extra compile with -DCONNECTIVITY_FAILSAFE_TEST_MODE=1 runs only
# the small test-mode matrix (that flag changes the code under test). Both transcripts, joined, must equal,
# byte for byte, fixtures/sleep_plan_transcript.txt ('# ' header lines stripped first). The extracted copies
# the harness compiled are read back and compared with a fresh extraction: COPY_MISMATCH on drift. Step (a)
# needs the base commit named in the fixture header. Mutants of a private copy of State_Sleep.cpp must each
# change the transcript.
# SLEEP_PLAN_SRC_ROOT tests another src/ root; SLEEP_PLAN_TAMPER_COPY=1 demonstrates COPY_MISMATCH.
# '--regenerate' rewrites the fixture from the base blocks (a).

tests_dir="${0:A:h}"
repo_root="${tests_dir:h}"
src_root="${SLEEP_PLAN_SRC_ROOT:-$repo_root/src}"
fixture="$tests_dir/fixtures/sleep_plan_transcript.txt"
stubs="$tests_dir/stubs/sleep_teardown"
work="$(mktemp -d "${TMPDIR:-/tmp}/sleep_plan_equivalence.XXXXXX")"
trap 'rm -rf "$work"' EXIT

base="$(sed -n 's/^# Base commit: //p' "$fixture" 2>/dev/null || true)"
base="${base:-3efd9d8}"
base_src="$work/base_State_Sleep.cpp"
have_base=0
if git -C "$repo_root" show "${base}:src/state/State_Sleep.cpp" > "$base_src" 2>/dev/null; then have_base=1; fi

cat > "$work/extract.py" <<'PY'
import sys

kind, path, outdir = sys.argv[1:4]
text = open(path).read()
END = "  // The sleep configuration is built fresh"


def span(start, end, keep_end=False):
    if text.count(start) != 1:
        sys.exit(f"MARKER_NOT_UNIQUE: {start!r} x{text.count(start)} in {path}")
    i = text.find(start)
    j = text.find(end, i + len(start))
    if j < 0:
        sys.exit(f"MARKER_MISSING: {end!r} in {path}")
    return text[i:j + len(end)] if keep_end else text[i:j]


out = {}
if kind == "base":
    out["region"] = span("  // WO-2026-09-19 Step 3b: the behavioral core of this step.", END)
else:
    out["funcs"] = span("struct SleepPlan {", "struct WakeContext {")
    out["region"] = span("  SleepPlan plan;\n", END)
for k, v in out.items():
    open(f"{outdir}/{kind}_{k}.inc", "w").write(v)
PY

extract() {
  # $1 = base|cur, $2 = source file
  python3 "$work/extract.py" "$1" "$2" "$work"
}

if (( have_base )); then extract base "$base_src"; fi
extract cur "$src_root/state/State_Sleep.cpp"
if [[ "${SLEEP_PLAN_TAMPER_COPY:-}" == 1 ]]; then echo "// drift" >> "$work/cur_funcs.inc"; fi

build_run() {
  # $1 = test-mode flag (0|1), $2 = mode (a|b), $3 = output, $4 = work dir holding the .inc copies
  local tm="$1" mode="$2" out="$3" inc="$4"
  c++ -std=c++17 -Wall -Wextra -Wno-unused-parameter -Werror -DWiring_Cellular=1 -DHAL_PLATFORM_CELLULAR=1 \
    -DCONNECTIVITY_FAILSAFE_TEST_MODE="$tm" -I"$stubs" -I"$src_root" \
    -DBASE_REGION_INC="\"$inc/base_region.inc\"" -DCUR_FUNCS_INC="\"$inc/cur_funcs.inc\"" \
    -DCUR_REGION_INC="\"$inc/cur_region.inc\"" "$tests_dir/sleep_plan_equivalence_test.cpp" -o "$out.bin"
  "$out.bin" "$mode" > "$out"
}

run_all() {
  # $1 = mode, $2 = output prefix, $3 = work dir holding the .inc copies
  build_run 0 "$1" "$2.release" "$3"
  build_run 1 "$1" "$2.testmode" "$3"
  cat "$2.release" "$2.testmode" > "$2"
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
counts = re.findall(r"^SCENARIOS (\d+) PASSES (\d+) PLAN_EXIT (\d+) GUARD_EXIT (\d+) CONTINUE (\d+) DISTINCT_EVENTS (\d+)$", body, re.M)
scen = sum(int(c[0]) for c in counts)
passes = sum(int(c[1]) for c in counts)
head = f"""# Frozen output of the pre-change sleep plan and nightly heap guard (handler plan block, hibernateAttempt call, guard block): WO-2026-10-21-003 (Sleep S-4).
# Base commit: {base}
# Scenarios: {scen} (release {counts[0][0]} + test-mode matrix {counts[1][0]}); passes: {passes}
# Per-build summary (scenarios passes PLAN_EXIT GUARD_EXIT CONTINUE distinct-events): release {' '.join(counts[0])}; test-mode {' '.join(counts[1])}
# Body sha256: {hashlib.sha256(body.encode()).hexdigest()}
# Generated once by extracting the plan and guard blocks by marker text from {base}:src/state/State_Sleep.cpp (git show),
# wrapping them in tests/sleep_plan_equivalence_test.cpp against tests/stubs/sleep_teardown, and running mode a.
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

# The transcript is not trivial: every branch of the moved code shows up, and so do all outcomes.
for line in "OUT PLAN_EXIT" "OUT GUARD_EXIT" "OUT CONTINUE" "TRANSITION 4 close due before night sleep" \
            "TRANSITION 2 sleep-counting-led-defer" "SENSOR onEnterSleep" "CALL signalLED 0" "CALL signalLEDStatus" \
            "CALL signalLEDTimeRemaining" "CALL logTimeDiag isOpen=1" "CALL logTimeDiag isOpen=0" \
            "CALL secondsUntilNextOpen" "CALL DailyBoundary::check" "CALL Clock::isTrusted" "CALL get_sensorSetting1" \
            "LOG INFO Clamping night sleep duration to max supported 32760 seconds (requested=32761)" \
            "LOG INFO COUNTING: Deferring sleep - LED remaining=3 sec" \
            "LOG INFO FailsafeDefer: reason=closed-hours-long-sleep open=0 sleep=" \
            "LOG INFO FailsafeTest: cap closed sleep 3600s->60s" \
            "LOG INFO ModemTeardown: radioOn=1 point=heap-guard-reset" \
            "resetting for clean next-day start" "monitoring only" "POWER flushDiagBatch" "RESET cause=4" \
            "CALL hibernateAttempt overnight=1 wake=28800" "CALL hibernateAttempt overnight=0 wake=3601" \
            "CALL hibernateAttempt overnight=0 wake=3600" "CALL hibernateAttempt overnight=1 wake=32760" \
            "reason=closed" "reason=debounce" "reason=scheduled" "wake=0 reason=debounce" "wake=5 reason=debounce"; do
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

mutate_and_expect_diff "close-due guard dropped" "if (DailyBoundary::check(Time.now()).due) {" "if (false) {"
mutate_and_expect_diff "clamp at 545 minutes" "const int MAX_SLEEP_SEC = 546 * 60;" "const int MAX_SLEEP_SEC = 545 * 60;"
mutate_and_expect_diff "plus-one margin dropped" "wakeInSeconds = aligned + 1;" "wakeInSeconds = aligned;"
mutate_and_expect_diff "heap reset at the warn threshold" \
  "if (freeHeap <= ConnectivityPolicy::NIGHTLY_HEAP_RESET_THRESHOLD_BYTES) {" \
  "if (freeHeap <= ConnectivityPolicy::NIGHTLY_HEAP_WARN_THRESHOLD_BYTES) {"
mutate_and_expect_diff "debounce divisor" "(setting1Raw / 1000) : 60;" "(setting1Raw / 100) : 60;"
mutate_and_expect_diff "LED off after onEnterSleep dropped" "    SensorManager::instance().onEnterSleep();
    signalLED(false);
" "    SensorManager::instance().onEnterSleep();
"
mutate_and_expect_diff "LED-defer transition dropped" '      transitionTo(IDLE_STATE, "sleep-counting-led-defer");
      return true;' "      return true;"
mutate_and_expect_diff "flush before the reset dropped" "      PowerDiagnostics::flushDiagBatch();
#endif
      System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);" "#endif
      System.reset(RESET_CAUSE_SLEEP_HEAP_GUARD);"
mutate_and_expect_diff "test-mode cap dropped" "nightSleepSec = (int)ConnectivityPolicy::CONNECTIVITY_FAILSAFE_TEST_MAX_CLOSED_SLEEP_SEC;" "nightSleepSec = nightSleepSec;"
mutate_and_expect_diff "guard called before the hibernate attempt" "  hibernateAttempt(overnightFallbackSleep, wakeInSeconds);

  if (nightlyHeapGuard(overnightFallbackSleep)) return;
" "  if (nightlyHeapGuard(overnightFallbackSleep)) return;

  hibernateAttempt(overnightFallbackSleep, wakeInSeconds);
"

echo "sleep_plan_equivalence_test: both transcripts match the frozen pre-change output, all mutations detected"
