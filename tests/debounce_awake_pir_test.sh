#!/bin/zsh
# WO-2026-11-11-003 Amendment 1 - with sensor.setting1 == 0 every PIR path uses
# Config::occupancyDebounceMsForRuntime() (300000 ms): the LED window and the
# logged debounce seconds. The four real blocks (awake first detection, awake
# already-occupied re-arm, sleep first detection, sleep already-occupied) are
# extracted byte-for-byte from State_Modes.cpp and run against fakes.
# A nonzero setting1 must keep its value (truncated to whole seconds awake).
# Set DEBOUNCE_AWAKE_SRC to a copy of State_Modes.cpp to test a mutant.
set -euo pipefail

repo_root="${0:A:h:h}"
src="${DEBOUNCE_AWAKE_SRC:-$repo_root/src/state/State_Modes.cpp}"
work="$(mktemp -d "${TMPDIR:-/tmp}/debounce_awake_pir.XXXXXX")"
trap 'rm -rf "$work"' EXIT

python3 - "$src" "$work" <<'PY'
import sys
src, out = sys.argv[1:]
lines = open(src).read().split("\n")

def block(name, start, end, tail=""):
    s = [i for i, l in enumerate(lines) if start in l]
    if len(s) != 1:
        sys.exit(f"EXTRACT_FAILED: {name} start {start!r} x{len(s)}")
    for j in range(s[0], len(lines)):
        if end in lines[j]:
            open(f"{out}/{name}.inc", "w").write("\n".join(lines[s[0]:j + 1]) + tail + "\n")
            return
    sys.exit(f"EXTRACT_FAILED: {name} end {end!r}")

block("awake_first", "// V3.23: Set LED with debounce timeout", 'logOccupiedEvent("pir", debounceSeconds, reportNow);')
block("awake_again", "// Already occupied - reset LED timeout on new motion", 'logOccupiedEvent("pir", debounceSeconds, false, true);', "\n}")  # closes the verboseMode if
block("sleep_first", "// Keep LED on for the debounce window", 'logOccupiedEvent("pir-wake", debounceMs / 1000UL, reportNow);')
block("sleep_again", "// Already occupied - motion detected during debounce", 'logOccupiedEvent("pir-wake", debounceMs / 1000UL, false, true);')
PY

cat > "$work/t.cpp" <<'CPP'
#include <cstdint>
#include <cstdio>
static uint32_t set1 = 0, ledMs = 0, loggedSec = 0;
namespace SystemConfig {
  namespace SensorSettings { uint32_t get_sensorSetting1() { return set1; } }
  bool get_verboseMode() { return true; }
}
namespace Config { uint32_t occupancyDebounceMsForRuntime() { return 300000; } }
struct LogT { template <class... A> void info(const char*, A...) {} template <class... A> void warn(const char*, A...) {} } Log;
static void signalLED(bool, uint32_t ms) { ledMs = ms; }
static bool reportsOccupancyChangesNow() { return false; }
static void logOccupiedEvent(const char*, uint32_t sec, bool, bool = false) { loggedSec = sec; }
static void awakeFirst() {
#include "awake_first.inc"
}
static void awakeAgain() {
#include "awake_again.inc"
}
static void sleepFirst() {
#include "sleep_first.inc"
}
static void sleepAgain() {
#include "sleep_again.inc"
}
static int fails = 0;
static void check(const char* n, void (*f)(), uint32_t s1, uint32_t wantMs, uint32_t wantSec) {
  set1 = s1; ledMs = loggedSec = 0xFFFFFFFFu;
  f();
  if (ledMs != wantMs || loggedSec != wantSec) {
    std::printf("FAIL %s setting1=%u: led=%u ms (want %u), logged=%u s (want %u)\n",
                n, s1, ledMs, wantMs, loggedSec, wantSec);
    fails++;
  }
}
int main() {
  check("awake first detection", awakeFirst, 0, 300000, 300);
  check("awake already occupied", awakeAgain, 0, 300000, 300);
  check("sleep first detection", sleepFirst, 0, 300000, 300);
  check("sleep already occupied", sleepAgain, 0, 300000, 300);
  check("awake first explicit", awakeFirst, 60000, 60000, 60);
  check("awake again explicit", awakeAgain, 60000, 60000, 60);
  check("awake first truncates", awakeFirst, 1500, 1000, 1);
  check("sleep first explicit", sleepFirst, 60000, 60000, 60);
  return fails ? 1 : 0;
}
CPP
c++ -std=c++17 -I"$work" -o "$work/t" "$work/t.cpp"
"$work/t" || { print -u2 "FAILED: PIR debounce with setting1 == 0"; exit 1; }
print "PASS: all PIR paths use the 300000 ms runtime fallback when setting1 is 0"
