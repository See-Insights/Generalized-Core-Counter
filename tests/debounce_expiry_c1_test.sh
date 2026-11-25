#!/bin/zsh
# WO-2026-11-03-004 item 19 C1 - one millis-based debounce expiry.
#
# The REAL debounceRemainingMs() (with occupancyDebounceMs()) is extracted from
# State_Common.h and driven against fakes: expiry depends on millis() and
# lastOccupancyEvent only (never the wall clock), the zero sentinel re-arms,
# and the boundary is expired at exactly the debounce. Call-site checks prove
# the awake Idle/Modes paths, the wake close and the sleep plan all use it.
# Mutations of the predicate and of each call site must each be caught.
set -euo pipefail

repo_root="${0:A:h:h}"
work="$repo_root/build-tmp/wo-2026-11-03-004-stage6/c1_test"
mkdir -p "$work"
src="$repo_root/src/state"

extract_pred() {
  awk '/^inline uint32_t occupancyDebounceMs\(\)/ {a=1} a {print} a && /^inline uint32_t debounceRemainingMs/ {b=1} b && /^}/ {exit}' "$1"
}

run_predicate() {
  local common="$1"
  extract_pred "$common" > "$work/pred.inc"
  [[ -s "$work/pred.inc" ]] || { print -u2 "EXTRACT_FAILED"; return 2; }
  cat > "$work/t.cpp" <<'CPP'
#include <cstdint>
#include <cstdio>
static uint32_t nowMs = 0, lastEvt = 0, set1 = 0;
static uint32_t millis() { return nowMs; }
namespace CurrentReadings {
  uint32_t get_lastOccupancyEvent() { return lastEvt; }
  void set_lastOccupancyEvent(uint32_t v) { lastEvt = v; }
}
namespace SystemConfig { namespace SensorSettings { uint32_t get_sensorSetting1() { return set1; } } }
namespace Config { uint32_t occupancyDebounceMsForRuntime() { return set1 == 0 ? 3600000 : set1; } }
#include "pred.inc"
static int fails = 0;
static void expect(const char* n, uint32_t got, uint32_t want) {
  if (got != want) { std::printf("FAIL %s got=%u want=%u\n", n, got, want); fails++; }
}
int main() {
  set1 = 60000; nowMs = 1000000; lastEvt = 1000000;
  expect("fresh", debounceRemainingMs(), 60000);
  nowMs = 1000000 + 59999; expect("1ms left", debounceRemainingMs(), 1);
  nowMs = 1000000 + 60000; expect("boundary expired", debounceRemainingMs(), 0);
  nowMs = 1000000 + 90000; expect("past expiry", debounceRemainingMs(), 0);
  set1 = 0; nowMs = 1000000 + 3599000; expect("runtime default", debounceRemainingMs(), 1000);
  set1 = 60000; lastEvt = 0; nowMs = 5000;
  expect("sentinel re-arms", debounceRemainingMs(), 60000);
  expect("sentinel written", lastEvt, 5000);
  lastEvt = 4294967000u; nowMs = 100; // millis() wrap: elapsed 396
  expect("wrap", debounceRemainingMs(), 60000 - 396);
  return fails ? 1 : 0;
}
CPP
  c++ -std=c++17 -I"$work" -o "$work/t" "$work/t.cpp" && "$work/t"
}

run_predicate "$src/State_Common.h" || { print -u2 "FAILED: predicate behaviour"; exit 1; }

# --- call sites -------------------------------------------------------------
check_has() { grep -qF -- "$2" "$1" || { print -u2 "FAILED: $1 lacks: $2"; return 1; } }
check_sites() {
  local d="$1"
  check_has "$d/State_Idle.cpp" "if (debounceRemainingMs() == 0) {" &&
  check_has "$d/State_Modes.cpp" "if (debounceRemainingMs() == 0) {" &&
  check_has "$d/State_Modes.cpp" "OCCUPANCY && (signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0" &&
  check_has "$d/State_Sleep.cpp" "const uint32_t remaining = debounceRemainingMs();" &&
  check_has "$d/State_Sleep.cpp" "(int)(remaining / 1000UL) + 1"
}
check_sites "$src" || exit 1

# The wall-clock LED deadline must no longer decide an OCCUPANCY wake close.
if grep -n "OCCUPANCY" "$src/State_Modes.cpp" | grep -q "signalLEDTimeRemaining"; then
  print -u2 "FAILED: OCCUPANCY wake close still uses signalLEDTimeRemaining"; exit 1
fi

# --- mutations ---------------------------------------------------------------
mutate() {  # name file old new
  local name="$1" file="$2" old="$3" new="$4"
  local m="$work/mut"; rm -rf "$m"; mkdir -p "$m"
  cp "$src/State_Common.h" "$src/State_Idle.cpp" "$src/State_Modes.cpp" "$src/State_Sleep.cpp" "$m/"
  python3 - "$m/$file" "$old" "$new" <<'PY'
import sys
p, old, new = sys.argv[1:]
s = open(p).read()
if s.count(old) != 1:
    print("MUTATION_NOT_APPLIED:", old, s.count(old)); sys.exit(3)
open(p, "w").write(s.replace(old, new))
PY
  local caught=0
  run_predicate "$m/State_Common.h" >/dev/null 2>&1 || caught=1
  check_sites "$m" >/dev/null 2>&1 || caught=1
  if grep -n "OCCUPANCY" "$m/State_Modes.cpp" | grep -q "signalLEDTimeRemaining"; then caught=1; fi
  if (( caught )); then print "caught: $name"; else print -u2 "NOT CAUGHT: $name"; exit 1; fi
}

mutate "no expiry clamp" State_Common.h "return elapsed >= debounceMs ? 0 : debounceMs - elapsed;" "return debounceMs - elapsed;"
mutate "off by the margin" State_Common.h "debounceMs - elapsed;" "debounceMs - elapsed + 1000;"
mutate "sentinel not handled" State_Common.h "if (last == 0) {" "if (false) {"
mutate "wall clock close" State_Modes.cpp "pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0" "pieceUntrusted && CurrentReadings::get_occupied())) && signalLEDTimeRemaining() == 0"
mutate "awake idle on subtraction" State_Idle.cpp "if (debounceRemainingMs() == 0) {" "if (millis() - CurrentReadings::get_lastOccupancyEvent() > 1) {"
mutate "plan on RTC deadline" State_Sleep.cpp "const uint32_t remaining = debounceRemainingMs();" "const uint32_t remaining = signalLEDTimeRemaining() * 1000UL;"
mutate "plan margin dropped" State_Sleep.cpp "(int)(remaining / 1000UL) + 1" "(int)(remaining / 1000UL)"

print "debounce_expiry_c1_test: predicate and call sites correct, all mutations detected"
