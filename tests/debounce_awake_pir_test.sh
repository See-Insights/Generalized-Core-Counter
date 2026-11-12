#!/bin/zsh
# WO-2026-11-11-003 Amendment 1 - with sensor.setting1 == 0 every PIR path uses
# Config::occupancyDebounceMsForRuntime() (300000 ms): the LED window and the
# logged debounce seconds. The four real blocks (awake first detection, awake
# already-occupied re-arm, sleep first detection, sleep already-occupied) are
# extracted byte-for-byte from State_Modes.cpp and run against fakes.
# Milliseconds end to end (Amendment 2): the LED window, the debounce expiry
# (through the real occupancyDebounceMs() wrapper) and the logged seconds all
# derive from the one helper value. The fake helper mirrors Config.cpp: 300000
# when the config is invalid or setting1 is 0, otherwise the stored setting1.
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

block("awake_first", "// V3.23: Set LED with debounce timeout", 'logOccupiedEvent("pir", ')
block("awake_again", "// Already occupied - reset LED timeout on new motion", 'logOccupiedEvent("pir", ', "\n}")  # closes the verboseMode if
block("sleep_first", "// Keep LED on for the debounce window", 'logOccupiedEvent("pir-wake", ')
block("sleep_again", "// Already occupied - motion detected during debounce", 'logOccupiedEvent("pir-wake", ')

# The real wrapper and expiry predicate (what State_Modes.cpp:133 and State_Idle.cpp:49 call).
c = open(src.rsplit("/", 1)[0] + "/State_Common.h").read()
a = c.index("inline uint32_t occupancyDebounceMs() {")
b = c.index("\n}\n", c.index("inline uint32_t debounceRemainingMs() {", a)) + 3
open(f"{out}/pred.inc", "w").write(c[a:b])
PY

cat > "$work/t.cpp" <<'CPP'
#include <cstdint>
#include <cstdio>
static uint32_t set1 = 0, ledMs = 0, loggedSec = 0, nowMs = 0, lastEvt = 0;
static bool isValid = true;
static uint32_t millis() { return nowMs; }
namespace CurrentReadings {
  uint32_t get_lastOccupancyEvent() { return lastEvt; }
  void set_lastOccupancyEvent(uint32_t v) { lastEvt = v; }
}
namespace SystemConfig {
  namespace SensorSettings { uint32_t get_sensorSetting1() { return set1; } }
  bool get_verboseMode() { return true; }
}
namespace Config {
  uint32_t occupancyDebounceMsForRuntime() {
    if (!isValid) return 300000;
    return set1 == 0 ? 300000 : set1;
  }
}
struct LogT { template <class... A> void info(const char*, A...) {} template <class... A> void warn(const char*, A...) {} } Log;
static void signalLED(bool, uint32_t ms) { ledMs = ms; }
static bool reportsOccupancyChangesNow() { return false; }
static void logOccupiedEvent(const char*, uint32_t sec, bool, bool = false) { loggedSec = sec; }
#include "pred.inc"
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
static void check(const char* n, void (*f)(), uint32_t s1, bool valid) {
  set1 = s1; isValid = valid;
  const uint32_t wantMs = (!valid || s1 == 0) ? 300000u : s1;
  ledMs = loggedSec = 0xFFFFFFFFu;
  f();
  if (ledMs != wantMs || loggedSec != wantMs / 1000u) {
    std::printf("FAIL %s setting1=%u valid=%d: led=%u ms (want %u), logged=%u s (want %u)\n",
                n, s1, (int)valid, ledMs, wantMs, loggedSec, wantMs / 1000u);
    fails++;
  }
  nowMs = 1000000; lastEvt = 1000000;  // just armed: remaining is the whole debounce
  const uint32_t rem = debounceRemainingMs();
  if (rem != wantMs) {
    std::printf("FAIL %s setting1=%u valid=%d: debounceRemainingMs=%u (want %u)\n", n, s1, (int)valid, rem, wantMs);
    fails++;
  }
}
int main() {
  static const uint32_t values[] = {0, 1, 999, 1000, 60000, 300000};
  static const struct { const char* n; void (*f)(); } paths[] = {
    {"awake first detection", awakeFirst}, {"awake already occupied", awakeAgain},
    {"sleep first detection", sleepFirst}, {"sleep already occupied", sleepAgain}};
  for (const auto& p : paths) {
    for (uint32_t v : values) check(p.n, p.f, v, true);
    check(p.n, p.f, 60000, false);  // invalid config with setting1 = 60000
  }
  return fails ? 1 : 0;
}
CPP
c++ -std=c++17 -I"$work" -o "$work/t" "$work/t.cpp"
"$work/t" || { print -u2 "FAILED: PIR debounce is not one millisecond value everywhere"; exit 1; }
print "PASS: all PIR paths, the LED window and the expiry share one millisecond debounce"
