#!/bin/zsh
set -euo pipefail

# WO-2026-10-21-001 (Sleep S-2), re-frozen for v50 S2 (WO-2026-11-05-004): tests/sleep_execution_equivalence_test.cpp
# runs one scenario table through the current functions and the handler's call-site lines, extracted by
# marker text from the current State_Sleep.cpp, against the fakes (tests/stubs/sleep_teardown/SleepExecutionFakes.h),
# for cellular and Wi-Fi. The transcript must equal, byte for byte, the frozen fixtures/sleep_execution_transcript.txt
# ('# ' header lines stripped first). The extracted copies the harness compiled are read back and compared with a
# fresh extraction: COPY_MISMATCH on drift. S2 changed the hibernate block on purpose (native duration alarm,
# ExternalWatchdog), so the pre-change base blocks are no longer run. Mutants of a private copy of State_Sleep.cpp
# must each change the transcript. SLEEP_EXECUTION_SRC_ROOT tests another src/ root; SLEEP_EXECUTION_TAMPER_COPY=1
# demonstrates COPY_MISMATCH. '--regenerate' rewrites the fixture from the current source.

tests_dir="${0:A:h}"
repo_root="${tests_dir:h}"
src_root="${SLEEP_EXECUTION_SRC_ROOT:-$repo_root/src}"
fixture="$tests_dir/fixtures/sleep_execution_transcript.txt"
stubs="$tests_dir/stubs/sleep_teardown"
work="$(mktemp -d "${TMPDIR:-/tmp}/sleep_execution_equivalence.XXXXXX")"
trap 'rm -rf "$work"' EXIT

cat > "$work/extract.py" <<'PY'
import sys

kind, path, outdir = sys.argv[1:4]
text = open(path).read()


def span(start, end, whole_line_end=False, keep_end=True):
    if text.count(start) != 1:
        sys.exit(f"MARKER_NOT_UNIQUE: {start!r} x{text.count(start)} in {path}")
    i = text.find(start)
    j = text.find(end, i + len(start) if end != start else i)
    if j < 0:
        sys.exit(f"MARKER_MISSING: {end!r} in {path}")
    return text[i:j + len(end)] if keep_end else text[i:j]


def line(start):
    return span(start, "\n")


out = {"drain": span("void drainSerialBeforeSleep() {", "\n}\n")}
out["funcs"] = span("struct WakeContext {", "\n} // namespace\n", keep_end=False) + "\n"
out["call_effective"] = line("  bool useNetworkStandbyEffective = effectiveStandby(")
out["call_teardown"] = line("  if (SleepTeardown::run(")
out["call_hibernate"] = line("  hibernateAttempt(")
out["call_sleep"] = span("  WakeContext wake;\n", "  if (sleepAndResume(wakeInSeconds, useNetworkStandby, sleepReason, wake)) return;\n")
out["call_route"] = line("  postWakeRouting(")
for k, v in out.items():
    open(f"{outdir}/{kind}_{k}.inc", "w").write(v)
PY

# Writes the base_*.inc / cur_*.inc copies the harness compiles.
extract() {
  # $1 = base|cur, $2 = source file
  python3 "$work/extract.py" "$1" "$2" "$work"
}

extract cur "$src_root/state/State_Sleep.cpp"
if [[ "${SLEEP_EXECUTION_TAMPER_COPY:-}" == 1 ]]; then echo "// drift" >> "$work/cur_funcs.inc"; fi

build_run() {
  # $1 = platform (cellular|wifi), $2 = mode (a|b), $3 = output, $4 = work dir holding the .inc copies
  local platform="$1" mode="$2" out="$3" inc="$4" defs=()
  [[ "$platform" == cellular ]] && defs=(-DWiring_Cellular=1 -DHAL_PLATFORM_CELLULAR=1 -DENABLE_SLEEP_TRACE=1) || defs=(-DWiring_WiFi=1)
  c++ -std=c++17 -Wall -Wextra -Wno-unused-parameter -Werror "${defs[@]}" -I"$stubs" -I"$src_root" \
    -DCUR_DRAIN_INC="\"$inc/cur_drain.inc\"" -DCUR_FUNCS_INC="\"$inc/cur_funcs.inc\"" \
    -DCUR_CALL_EFFECTIVE_INC="\"$inc/cur_call_effective.inc\"" -DCUR_CALL_TEARDOWN_INC="\"$inc/cur_call_teardown.inc\"" \
    -DCUR_CALL_HIBERNATE_INC="\"$inc/cur_call_hibernate.inc\"" -DCUR_CALL_SLEEP_INC="\"$inc/cur_call_sleep.inc\"" -DCUR_CALL_ROUTE_INC="\"$inc/cur_call_route.inc\"" \
    "$tests_dir/sleep_execution_equivalence_test.cpp" -o "$out.bin"
  "$out.bin" "$mode" > "$out"
}

run_all() {
  # $1 = mode, $2 = output prefix, $3 = work dir holding the .inc copies
  build_run cellular "$1" "$2.cellular" "$3"
  build_run wifi "$1" "$2.wifi" "$3"
  cat "$2.cellular" "$2.wifi" > "$2"
}

if [[ "${1:-}" == "--regenerate" ]]; then
  run_all b "$work/regen.txt" "$work"
  python3 - "$work/regen.txt" "$fixture" <<'PY'
import hashlib
import re
import sys

body_path, fixture = sys.argv[1:3]
body = open(body_path).read()
counts = re.findall(r"^SCENARIOS (\d+) PASSES (\d+) WOKE (\d+) RETURNED (\d+) DISTINCT_EVENTS (\d+)$", body, re.M)
scen = sum(int(c[0]) for c in counts)
passes = sum(int(c[1]) for c in counts)
head = f"""# Frozen output of the sleep execution (standby decision, hibernate attempt, ULP/STOP cascade): WO-2026-10-21-001 (Sleep S-2), re-frozen for v50 S2 (WO-2026-11-05-004).
# Scenarios: {scen} (cellular {counts[0][0]} + wifi {counts[1][0]}); passes: {passes}
# Per-platform summary (scenarios passes WOKE RETURNED distinct-events): cellular {' '.join(counts[0])}; wifi {' '.join(counts[1])}
# Body sha256: {hashlib.sha256(body.encode()).hexdigest()}
# Generated with --regenerate from the S2 State_Sleep.cpp (native HIBERNATE duration alarm, ExternalWatchdog) against tests/stubs/sleep_teardown.
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
for f in "$work"/fresh/*.inc; do
  cmp -s "$f" "$work/${f:t}" || { echo "COPY_MISMATCH: ${f:t} compiled by the harness differs from the file"; exit 1; }
done

diff -u "$work/expected.txt" "$work/new.txt" > "$work/new.diff" || { head -60 "$work/new.diff"; echo "EQUIVALENCE_FAILED: State_Sleep.cpp differs from the frozen transcript"; exit 1; }

# The transcript is not trivial: every branch the moved code has shows up, and so do all outcomes.
for line in "HIB eligible?" "HIB armForSleep" "HIB abandon" "EXTWDT stop" "EXTWDT start" \
            "SLEEP mode=HIBERNATE gpio=[7F,4F]" "SLEEP mode=ULP gpio=[4F,9R] net=3:0 dur=3600000" \
            "SLEEP mode=ULP gpio=[4F,9R] net=none dur=900000" "SLEEP mode=STOP gpio=[4F,9R]" \
            "SLEEP mode=STOP gpio=[] net=none dur=3600000" "SLEEP mode=ULP gpio=[4F] net=3:0" \
            "TEARDOWN req=1 eff=0 force=1 err=-170" "TEARDOWN req=1 eff=1 force=0 err=0" "RESET cause=5" \
            "TRANSITION 1 sleep-all-attempts-failed" "ALERT 16" "CALL Serial.begin 9600" "CALL attachInterrupt 4" \
            "WATCHDOG pause ulp" "WATCHDOG restore hibernate" "CRUMB 24" "CRUMB 28" "CRUMB 4" "PROGRESS WAKE_FROM_SLEEP" \
            "PROGRESS SERIAL_WAIT" "POWER flushDiagBatch" "CYCLE resetOnWake" "MEASURE noteWakeFromLowPowerSleep" \
            "LOG INFO SLEEP: standby sleep returned normally; preserving modem standby" \
            "LOG WARN SLEEP: standby sleep failed; forcing modem-off reason=sleep_error err=-170" \
            "LOG WARN HIBERNATE bypass: wake pin held low" "LOG WARN HIBERNATE sleep failed err=-170" \
            "LOG ERROR ULTRA_LOW_POWER sleep failed err=-170" "LOG ERROR STOP sleep fallback failed" \
            "LOG ERROR All sleep attempts failed err=-170" "LOG WARN Serial did not reconnect within 30s" \
            "LOG INFO SleepCall: standby=1" "LOG WARN SleepCall: watchdog-stop failed" \
            "RET fell=1 pir=1 btn=0 timer=0 label=PIR" "RET fell=1 pir=0 btn=1 timer=0 label=BUTTON" \
            "RET fell=1 pir=0 btn=0 timer=1 label=TIMER" "RET fell=0" \
            "FIELDS eff=1 preserve=1 force=0 err=0" "FIELDS eff=0 preserve=0 force=1 err=-170" \
            "FIELDS eff=0 preserve=0 force=0 err=0" "... x"; do
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
  python3 "$work/extract.py" cur "$m/State_Sleep.cpp" "$m"
  run_all b "$m/new.txt" "$m" > "$m/build.log" 2>&1 || { echo "MUTANT_DID_NOT_BUILD: $label"; cat "$m/build.log"; exit 1; }
  if diff -q "$work/expected.txt" "$m/new.txt" > /dev/null; then echo "MUTANT_SURVIVED: $label"; exit 1; fi
  echo "caught: $label"
}

mutate_and_expect_diff "ULP error skips STOP with GPIO" \
  "    result = System.sleep(stopConfig);
" ""
mutate_and_expect_diff "standby error leaves forceModemOff unset" \
  "standbyReturn.forceModemOff = true;" "standbyReturn.forceModemOff = false;"
mutate_and_expect_diff "returned hibernate not abandoned" \
  "        HibernateCycle::abandon();
" ""
mutate_and_expect_diff "WAKE_FROM_SLEEP progress after the serial re-wait" \
  "  thrashGuard.markProgress(\"WAKE_FROM_SLEEP\");
" "" \
  "  Log.info(\"WakeReturn: elapsed=%lu reason=%s\"," "  thrashGuard.markProgress(\"WAKE_FROM_SLEEP\");
  Log.info(\"WakeReturn: elapsed=%lu reason=%s\","
mutate_and_expect_diff "standby preserved through an unstable modem" \
  "(!useNetworkStandbyRequested || session.modemUnstable || Cellular.isOff())" "(!useNetworkStandbyRequested || Cellular.isOff())"
mutate_and_expect_diff "PIR classified from the wrong edge" \
  "bool pirWake = sensorCanWake && (wakePin == sensorWake.pin);" "bool pirWake = sensorCanWake && (wakePin != sensorWake.pin);"
mutate_and_expect_diff "post-wake routing handed swapped pir and button" \
  "postWakeRouting(wake.pirWake, wake.buttonWake, wake.timerWake, wake.wakeReasonLabel);" "postWakeRouting(wake.buttonWake, wake.pirWake, wake.timerWake, wake.wakeReasonLabel);"

echo "sleep_execution_equivalence_test: both transcripts match the re-frozen S2 output, all mutations detected"
