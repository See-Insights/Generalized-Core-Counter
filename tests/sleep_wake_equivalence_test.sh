#!/bin/zsh
set -euo pipefail

# WO-2026-10-22-001 (Sleep S-6): moving the post-wake routing into postWakeRouting() (State_Sleep.cpp) and the
# wake-event processing into processSleepWakeEvent() (State_Modes.cpp) changes no behaviour.
# tests/sleep_wake_equivalence_test.cpp runs one single-pass scenario table through (a) the handler's post-wake
# region extracted by marker text from the base State_Sleep.cpp, and (b) the current postWakeRouting() and
# processSleepWakeEvent(), extracted by marker text from the current files, against the same fakes
# (tests/stubs/sleep_teardown). One release build. The transcript must equal, byte for byte,
# fixtures/sleep_wake_transcript.txt ('# ' header lines stripped first). The extracted copies the harness
# compiled are read back and compared with a fresh extraction: COPY_MISMATCH on drift. Step (a) needs the base
# commit named in the fixture header. Mutants of private copies of State_Sleep.cpp and State_Modes.cpp must
# each change the transcript.
# SLEEP_WAKE_SRC_ROOT tests another src/ root; SLEEP_WAKE_TAMPER_COPY=1 demonstrates COPY_MISMATCH.
# '--regenerate' rewrites the fixture from the base region (a).

tests_dir="${0:A:h}"
repo_root="${tests_dir:h}"
src_root="${SLEEP_WAKE_SRC_ROOT:-$repo_root/src}"
fixture="$tests_dir/fixtures/sleep_wake_transcript.txt"
stubs="$tests_dir/stubs/sleep_teardown"
work="$(mktemp -d "${TMPDIR:-/tmp}/sleep_wake_equivalence.XXXXXX")"
trap 'rm -rf "$work"' EXIT

base="$(sed -n 's/^# Base commit: //p' "$fixture" 2>/dev/null || true)"
base="${base:-e6d3e69}"
base_src="$work/base_State_Sleep.cpp"
have_base=0
if git -C "$repo_root" show "${base}:src/state/State_Sleep.cpp" > "$base_src" 2>/dev/null; then have_base=1; fi

cat > "$work/extract.py" <<'PY'
import sys

kind, sleep_path, modes_path, outdir = sys.argv[1:5]
ROUTE_END = '    transitionTo(IDLE_STATE, "sleep-wake-idle");\n  }\n'


def span(path, start, end, keep_end=True):
    text = open(path).read()
    if text.count(start) != 1:
        sys.exit(f"MARKER_NOT_UNIQUE: {start!r} x{text.count(start)} in {path}")
    i = text.find(start)
    j = text.find(end, i + len(start))
    if j < 0:
        sys.exit(f"MARKER_MISSING: {end!r} in {path}")
    return text[i:j + len(end)] if keep_end else text[i:j]


out = {}
if kind == "base":
    out["region"] = span(sleep_path, "  // LED will be turned on with proper timeout by PIR wake processing below\n", ROUTE_END)
else:
    out["route"] = span(sleep_path, "static void postWakeRouting(", ROUTE_END + "}\n")
    out["event"] = span(modes_path, "bool processSleepWakeEvent(bool pirWake) {", "  return false;\n}\n")
for k, v in out.items():
    open(f"{outdir}/{kind}_{k}.inc", "w").write(v)
PY

extract() {
  # $1 = base|cur, $2 = State_Sleep.cpp, $3 = State_Modes.cpp, $4 = output dir
  python3 "$work/extract.py" "$1" "$2" "$3" "$4"
}

if (( have_base )); then extract base "$base_src" "$base_src" "$work"; fi
extract cur "$src_root/state/State_Sleep.cpp" "$src_root/state/State_Modes.cpp" "$work"
if [[ "${SLEEP_WAKE_TAMPER_COPY:-}" == 1 ]]; then echo "// drift" >> "$work/cur_route.inc"; fi

build_run() {
  # $1 = mode (a|b), $2 = output, $3 = work dir holding the .inc copies
  local mode="$1" out="$2" inc="$3"
  c++ -std=c++17 -Wall -Wextra -Wno-unused-parameter -Werror -DWiring_Cellular=1 -DHAL_PLATFORM_CELLULAR=1 \
    -I"$stubs" -I"$src_root" \
    -DBASE_REGION_INC="\"$inc/base_region.inc\"" -DCUR_EVENT_INC="\"$inc/cur_event.inc\"" \
    -DCUR_ROUTE_INC="\"$inc/cur_route.inc\"" \
    "$tests_dir/sleep_wake_equivalence_test.cpp" -o "$out.bin"
  "$out.bin" "$mode" > "$out"
}

if [[ "${1:-}" == "--regenerate" ]]; then
  (( have_base )) || { echo "NO_BASE_COMMIT: $base"; exit 1; }
  build_run a "$work/regen.txt" "$work"
  python3 - "$base" "$work/regen.txt" "$fixture" <<'PY'
import hashlib
import re
import sys

base, body_path, fixture = sys.argv[1:4]
body = open(body_path).read()
m = re.search(r"^SCENARIOS (\d+) PASSES (\d+) TRANSITIONS (\d+) NONE (\d+) DISTINCT_EVENTS (\d+)$", body, re.M)
head = f"""# Frozen output of the pre-change post-wake routing (handler region after the wake unpack, with the wake-event processing inside it): WO-2026-10-22-001 (Sleep S-6).
# Base commit: {base}
# Scenarios: {m.group(1)}; passes: {m.group(2)}
# Summary (scenarios passes TRANSITIONS NONE distinct-events): {' '.join(m.groups())}
# Body sha256: {hashlib.sha256(body.encode()).hexdigest()}
# Generated once by extracting the region by marker text from {base}:src/state/State_Sleep.cpp (git show),
# wrapping it in tests/sleep_wake_equivalence_test.cpp against tests/stubs/sleep_teardown, and running mode a.
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
if int(re.search(r"^SCENARIOS (\d+) ", body, re.M).group(1)) != scen:
    sys.exit("FIXTURE_HEADER_SCENARIO_COUNT_MISMATCH")
PY

build_run b "$work/new.txt" "$work"

# What the harness compiled must be what the files say: re-extract and compare every copy.
mkdir -p "$work/fresh"
extract cur "$src_root/state/State_Sleep.cpp" "$src_root/state/State_Modes.cpp" "$work/fresh"
(( have_base )) && extract base "$base_src" "$base_src" "$work/fresh"
for f in "$work"/fresh/*.inc; do
  cmp -s "$f" "$work/${f:t}" || { echo "COPY_MISMATCH: ${f:t} compiled by the harness differs from the file"; exit 1; }
done

diff -u "$work/expected.txt" "$work/new.txt" > "$work/new.diff" || { head -60 "$work/new.diff"; echo "EQUIVALENCE_FAILED: the wake code differs from the frozen transcript"; exit 1; }

if (( have_base )); then
  build_run a "$work/old.txt" "$work"
  diff -u "$work/expected.txt" "$work/old.txt" > "$work/old.diff" || { head -60 "$work/old.diff"; echo "FIXTURE_NOT_FROM_BASE: the base region differs from the fixture ($base)"; exit 1; }
else
  echo "BASE_CHECK_SKIPPED: commit $base is not in this tree; only the fixture comparison ran"
fi

# The transcript is not trivial: every branch of the moved code shows up.
for line in "BUILD release" "TRANSITION 4 sleep-wake-button" "TRANSITION 4 sleep-timer-report" "TRANSITION 0 sleep-timer-occupied-suppress-report" \
            "TRANSITION 5 sleep-open-hours-reconnect" "TRANSITION 4 sleep-pir-overdue-report" "TRANSITION 0 sleep-pir-return-to-sleep" \
            "TRANSITION 2 sleep-wake-idle" "TRANSITION 4 sleep-occupancy-debounce-report" "TRANSITION 4 sleep-pir-occupancy-report" \
            "CRUMB 5" "SENSOR onExitSleep" "SENSOR initializeFromConfig" "MEASURE batteryState 3" \
            "BATAUTH evaluate soc=41.5 vcellState=0" "BATAUTH evaluate soc=41.5 vcellState=2" "BATAUTH commit tier=2 trust=0" \
            "WAKESUMMARY BUTTON" "WAKESUMMARY PIR" "WAKESUMMARY TIMER" "SET hourlyCount=5" "SET hourlyCount=0" "SET dailyCount=0" \
            "SET lastCountTime=10000" "SET lastCountTime=0" "SET occupied=1" "SET occupancyStartTime=10000" "SET lastOccupancyEvent=61000" \
            "LED on=0 ms=0" "LED on=1 ms=1000" "LED on=1 ms=300000" "LED on=1 ms=45000" "LED on=1 ms=20000" \
            "CLOSE sleep at=10000 valid=1 stillOpen=0" "CLOSE sleep at=10000 valid=0 stillOpen=0" "CLOSE sleep at=10000 valid=1 stillOpen=1" \
            "LOG INFO Count detected from PIR wake - Hourly: 5, Daily: 10" "SERIAL Count detected from PIR wake" "PROGRESS PIR_WAKE_OCCUPANCY" \
            "OCCEVT occupied reason=pir-wake led=300s report=1 reset=0" "OCCEVT occupied reason=pir-wake led=300s report=0 reset=0" \
            "OCCEVT occupied reason=pir-wake led=45s report=0 reset=1" "OCCEVT unoccupied reason=debounce session=120s total=4000s report=1" \
            "OCCEVT unoccupied reason=debounce session=120s total=4000s report=0" "OUT trans=1 serviceRequest=1" "occChange=1" \
            "REASON sleep-wake-button 5"; do
  grep -qF -- "$line" "$work/new.txt" || { echo "LOG_LINE_NEVER_EXERCISED: $line"; exit 1; }
done

mutate_and_expect_diff() {
  # $1 = label, $2 = sleep|modes, then old/new pairs applied to a private copy of that file
  local m="$work/mut_${1// /_}" which="$2"
  shift 2
  local label="${m:t}" file
  [[ "$which" == sleep ]] && file=State_Sleep.cpp || file=State_Modes.cpp
  mkdir -p "$m"
  cp "$src_root/state/State_Sleep.cpp" "$m/State_Sleep.cpp"
  cp "$src_root/state/State_Modes.cpp" "$m/State_Modes.cpp"
  python3 - "$m/$file" "$@" <<'PY'
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
  python3 "$work/extract.py" cur "$m/State_Sleep.cpp" "$m/State_Modes.cpp" "$m"
  build_run b "$m/new.txt" "$m" > "$m/build.log" 2>&1 || { echo "MUTANT_DID_NOT_BUILD: $label"; cat "$m/build.log"; exit 1; }
  if diff -q "$work/expected.txt" "$m/new.txt" > /dev/null; then echo "MUTANT_SURVIVED: $label"; exit 1; fi
  echo "caught: $label"
}

LED_CLOSE='    if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY && signalLEDTimeRemaining() == 0 && signalLEDStatus()) {'
# The PIR event processed before the LED-timeout close (a pair of literal replacements).
mutate_and_expect_diff "PIR event processed before the LED-timeout close" modes \
  "$LED_CLOSE" "    if (false) {" \
  "        CurrentReadings::set_lastOccupancyEvent(millis());
      }
    }
  return false;
}" "        CurrentReadings::set_lastOccupancyEvent(millis());
      }
    }
$LED_CLOSE
      const bool reportNow = reportsOccupancyChangesNow();
      const OccupancyCloseResult closeResult = closeOccupancySessionSafely(\"sleep\");
      signalLED(false);
      if (reportNow && !closeResult.stillOpen) {
        session.occupancyChangeTriggered = true;
        transitionTo(REPORTING_STATE, \"sleep-occupancy-debounce-report\");
        return true;
      }
    }
  return false;
}"
mutate_and_expect_diff "breadcrumb 5 dropped" modes "            setAppBreadcrumb(5);
" ""
mutate_and_expect_diff "battery refresh skipped on the occupancy PIR wake" modes "        if (PowerConfig::get_lowBatteryMode()) {" "        if (false) {"
mutate_and_expect_diff "CONNECTED PIR wake routed back to sleep" sleep \
  "if (pirWake && PowerManager::instance().effectiveConnectionMode() != SystemConfig::CONNECTED) {" "if (pirWake) {"
mutate_and_expect_diff "timer-wake suppress condition dropped" sleep "INTERMITTENT_KEEP_ALIVE && !reportDueThisInterval()) {
        transitionTo(SLEEPING_STATE, \"sleep-timer-occupied-suppress-report\");" "INTERMITTENT_KEEP_ALIVE && false) {
        transitionTo(SLEEPING_STATE, \"sleep-timer-occupied-suppress-report\");"
mutate_and_expect_diff "stray occupancy write in the routing" sleep "    // Timer wake = scheduled report. No checks, no gates.
" "    CurrentReadings::set_occupied(false);
    // Timer wake = scheduled report. No checks, no gates.
"
mutate_and_expect_diff "wake-event result ignored" sleep "if (processSleepWakeEvent(pirWake)) return;" "processSleepWakeEvent(pirWake);"
mutate_and_expect_diff "wake summary dropped before the reconnect" sleep "        logWakeSummary(wakeReasonLabel);
        transitionTo(CONNECTING_STATE" "        transitionTo(CONNECTING_STATE"
mutate_and_expect_diff "opportunistic report needs Open, not not-Closed" sleep "if (pirWake && Clock::openness() != Clock::Openness::Closed) {" "if (pirWake && Clock::openness() == Clock::Openness::Open) {"
mutate_and_expect_diff "overdue report counts a never-reported device" sleep "if (lastReport > 0 && (now - lastReport) >= intervalSec) {" "if (lastReport >= 0 && (now - lastReport) >= intervalSec) {"
mutate_and_expect_diff "last count time recorded from an untrusted clock" modes $'        CurrentReadings::set_lastCountTime(isClockTrusted() ? Time.now() : 0);\n        signalLED' $'        CurrentReadings::set_lastCountTime(Time.now());\n        signalLED'
mutate_and_expect_diff "button wake skips the service request" sleep "    session.serviceRequestTriggered = true;
" ""

echo "sleep_wake_equivalence_test: the transcript matches the frozen pre-change output, all mutations detected"
