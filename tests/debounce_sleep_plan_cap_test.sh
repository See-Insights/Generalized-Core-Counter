#!/bin/zsh
# WO-2026-11-11-003 Amendment 2 - the occupied branch of the sleep plan
# (State_Sleep.cpp) with the debounce already expired (remaining == 0) and a
# scheduled wake longer than the debounce caps the wake at the debounce from
# Config::occupancyDebounceMsForRuntime(), in whole seconds rounded up
# (1..1000 ms -> 1 s, 60000 -> 60, 300000 -> 300), never a local fallback.
# The real block is extracted from State_Sleep.cpp and the real wrapper and
# expiry predicate from State_Common.h. DEBOUNCE_SLEEP_SRC / DEBOUNCE_COMMON_SRC
# point at mutant copies.
set -euo pipefail

repo_root="${0:A:h:h}"
src="${DEBOUNCE_SLEEP_SRC:-$repo_root/src/state/State_Sleep.cpp}"
common="${DEBOUNCE_COMMON_SRC:-$repo_root/src/state/State_Common.h}"
work="$(mktemp -d "${TMPDIR:-/tmp}/debounce_sleep_plan.XXXXXX")"
trap 'rm -rf "$work"' EXIT

python3 - "$src" "$common" "$work" <<'PY'
import sys
src, common, out = sys.argv[1:]
s = open(src).read()
a = s.find("// Occupied - wake for debounce timeout check")
e = s.find('sleepReason = "debounce";', a)
if a < 0 or e < 0 or s.count("// Occupied - wake for debounce timeout check") != 1:
    sys.exit("EXTRACT_FAILED: sleep plan occupied branch")
open(f"{out}/plan.inc", "w").write(s[a:e + len('sleepReason = "debounce";')] + "\n")
c = open(common).read()
i = c.index("inline uint32_t occupancyDebounceMs() {")
j = c.index("\n}\n", c.index("inline uint32_t debounceRemainingMs() {", i)) + 3
open(f"{out}/pred.inc", "w").write(c[i:j])
PY

cat > "$work/t.cpp" <<'CPP'
#include <cstdint>
#include <cstdio>
static uint32_t set1 = 0, nowMs = 0, lastEvt = 0;
static bool isValid = true;
static uint32_t millis() { return nowMs; }
namespace CurrentReadings {
  uint32_t get_lastOccupancyEvent() { return lastEvt; }
  void set_lastOccupancyEvent(uint32_t v) { lastEvt = v; }
}
namespace SystemConfig { namespace SensorSettings { uint32_t get_sensorSetting1() { return set1; } } }
namespace Config {
  uint32_t occupancyDebounceMsForRuntime() {
    if (!isValid) return 300000;
    return set1 == 0 ? 300000 : set1;
  }
}
#include "pred.inc"
static int plan(int wakeInSeconds) {
  const char* sleepReason = "";
  {
#include "plan.inc"
  }
  return wakeInSeconds;
}
static int fails = 0;
static void check(uint32_t s1, bool valid, int scheduled, int want) {
  set1 = s1; isValid = valid;
  nowMs = 100000000; lastEvt = 1;  // long expired: remaining == 0
  const int got = plan(scheduled);
  if (got != want) {
    std::printf("FAIL setting1=%u valid=%d scheduled=%d: wake=%d s (want %d)\n", s1, (int)valid, scheduled, got, want);
    fails++;
  }
}
int main() {
  check(0, true, 3600, 300);
  check(1, true, 3600, 1);        // sub-second rounds up to 1 s
  check(999, true, 3600, 1);
  check(1000, true, 3600, 1);
  check(1001, true, 3600, 2);
  check(60000, true, 3600, 60);
  check(300000, true, 3600, 300);
  check(60000, false, 3600, 300); // invalid config: the helper's default
  check(300000, true, 120, 120);  // an earlier scheduled wake is kept
  return fails ? 1 : 0;
}
CPP
c++ -std=c++17 -I"$work" -o "$work/t" "$work/t.cpp"
"$work/t" || { print -u2 "FAILED: sleep plan debounce cap"; exit 1; }
print "PASS: the sleep plan caps the wake at the helper's debounce"
