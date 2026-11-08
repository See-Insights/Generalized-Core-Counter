#!/bin/zsh
# WO-2026-11-05-007 (I21): clock trust survives a restart and is withdrawn on evidence.
#
# 1. clock_trust_carry_test.cpp: the real predicate, write-back stamp and verdict arms.
# 2. The C2 regression (a piece opened while trusted, trust then withdrawn) against
#    the REAL closeOccupancySessionSafely() extracted from State_Common.h.
# 3. Exact-text checks of the call sites that cannot be driven on the host
#    (Clock.cpp, the boot verdict site).
# 4. Mutations (run on a scratch copy of src/): each of the nine in the WO must fail.
set -euo pipefail

repo_root="${0:A:h:h}"
work="$repo_root/build-tmp/wo-2026-11-05-007-stage6/carry_test"
mkdir -p "$work"
trap 'rm -rf "$work/mut"' EXIT

extract_braced_block() {
  awk -v marker="$2" '
    !active && index($0, marker) { active = 1 }
    active {
      print
      opens = gsub(/\{/, "{"); closes = gsub(/\}/, "}")
      depth += opens - closes
      if (seen_open && depth == 0) exit
      if (opens > 0) seen_open = 1
    }' "$1"
}

run_unit() {  # $1 = src root
  c++ -std=c++17 -Wall -Wextra -pedantic -I"$1" "$repo_root/tests/clock_trust_carry_test.cpp" -o "$work/unit" && "$work/unit"
}

run_c2() {  # $1 = src root
  local common="$1/state/State_Common.h" out="$work/c2.cpp"
  {
    cat <<'CPP'
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
static unsigned long g_millis = 0;
unsigned long millis() { return g_millis; }
static time_t g_now = 0;
struct FakeTime { time_t now() const { return g_now; } };
FakeTime Time;
struct FakeLog { void warn(const char *, ...) {} void info(const char *, ...) {} };
FakeLog Log;
namespace CurrentReadings {
bool occupied = false; time_t start = 0; uint32_t total = 0; uint32_t lastEvt = 0;
bool get_occupied() { return occupied; }
void set_occupied(bool v) { occupied = v; }
time_t get_occupancyStartTime() { return start; }
void set_occupancyStartTime(time_t v) { start = v; }
uint32_t get_totalOccupiedSeconds() { return total; }
void set_totalOccupiedSeconds(uint32_t v) { total = v; }
uint32_t get_lastOccupancyEvent() { return lastEvt; }
void set_lastOccupancyEvent(uint32_t v) { lastEvt = v; }
}
namespace RecoveryState { int8_t get_alertCode() { return 0; } }
namespace Clock {
bool trusted = false;
bool isTrusted() { return trusted; }
enum class Openness { Open, Closed, Unknown };
Openness openness() { return Openness::Open; }
}
namespace DailyBoundary { struct Result { bool due; time_t boundary; uint8_t close; }; Result check(time_t) { return {false, 0, 0}; } }
namespace SystemConfig { namespace SensorSettings { uint32_t get_sensorSetting1() { return 300000; } } }
namespace Config { uint32_t occupancyDebounceMsForRuntime() { return 300000UL; } }
struct SessionState { time_t occupancySessionBootAnchor = 0; uint32_t occupancyStartMs = 0; bool pieceUntrusted = false; };
SessionState session;
CPP
    extract_braced_block "$common" "inline bool occupancyIgnoredWhenClosed() {"
    extract_braced_block "$common" "struct OccupancyCloseResult {"
    extract_braced_block "$common" "inline uint32_t occupancyDebounceMs() {"
    extract_braced_block "$common" "inline OccupancyCloseResult closeOccupancySessionSafely(const char *path, time_t closeAt = Time.now()) {"
    cat <<'CPP'

// Carried trust opens the piece as a trusted piece (pieceUntrusted = !Clock::isTrusted() = false); trust is then withdrawn.
int main() {
  g_millis = 600000; g_now = 1792000000;
  Clock::trusted = true;                                  // trust carried from the persisted lastTimeSync
  CurrentReadings::occupied = true; CurrentReadings::start = g_now - 120;
  session.occupancyStartMs = g_millis; session.pieceUntrusted = !Clock::isTrusted();
  assert(!session.pieceUntrusted);
  Clock::trusted = false;                                 // withdrawn (latch, large correction or failed verdict)
  g_millis += 400000;
  OccupancyCloseResult r = closeOccupancySessionSafely("c2", g_now);
  assert(r.stillOpen && !r.valid);                        // B2: kept open, nothing credited
  assert(CurrentReadings::occupied && CurrentReadings::total == 0);
  assert(CurrentReadings::lastEvt == g_millis);           // debounce re-armed
  Clock::trusted = true;                                  // trust returns: the trusted piece closes on the wall clock
  g_now += 400;
  r = closeOccupancySessionSafely("c2", g_now);
  assert(!r.stillOpen && r.valid && !CurrentReadings::occupied);
  assert(CurrentReadings::total == 520);
  std::printf("c2 regression passed\n");
  return 0;
}
CPP
  } > "$out"
  c++ -std=c++17 -I"$1" -o "$work/c2" "$out" && "$work/c2"
}

check_text() {  # $1 = src root
  python3 - "$1/time/Clock.cpp" "$1/Generalized-Core-Counter.cpp" <<'PY'
import re, sys
clock, app = (open(p).read() for p in sys.argv[1:3])
def need(cond, msg):
    if not cond:
        print("FAILED:", msg); sys.exit(1)
def body(src, sig):
    i = src.index(sig); j = src.index("{", i); d = 0
    for k in range(j, len(src)):
        d += (src[k] == "{") - (src[k] == "}")
        if d == 0: return src[i:k + 1]
    sys.exit("EXTRACT_FAILED " + sig)
trust = body(clock, "bool isClockTrusted() {")
need(re.search(r"syncCaptureWrapGeneration\) \|\| carryEligible\(\);", trust), "isClockTrusted() must be current-boot trust || carryEligible()")
elig = body(clock, "bool carryEligible() {")
need("last != 0 && ClockTrust::isCarryEligible(Time.isValid() && Clock::isPlausibleEpoch(now), ExternalRtc::isSet(), now, last, Clock::isPlausibleEpoch(last)," in elig, "carryEligible() must use the epoch now/last, both plausibility checks and the external clock")
need("ExternalRtc::oscillatorFaultSeenSinceBind().load(std::memory_order_relaxed)" in elig, "carryEligible() must read the S4 latch")
need("millis" not in elig, "the carry age must not use millis()")
cc = body(clock, "void checkClockResync() {")
need(re.match(r"void checkClockResync\(\) \{\n  if \(ExternalRtc::oscillatorFaultSeenSinceBind\(\)\.load\(std::memory_order_relaxed\) && SystemConfig::get_lastTimeSync\(\) != 0\) \{\n    SystemConfig::set_lastTimeSync\(0\); SystemConfig::flushNow\(\);", cc), "a latched fault must zero lastTimeSync and flushNow() synchronously")
need("const time_t carryStamp = (time_t)ClockTrust::carryStamp(rtcUpdated, rtcPreWriteReadOk, rtcCorrectionSec, Time.now());\n    SystemConfig::set_lastTimeSync(carryStamp);\n    if (carryStamp == 0) SystemConfig::flushNow();" in cc, "the write-back must stamp via carryStamp(rtcUpdated, rtcPreWriteReadOk, ...) and flush a revoke")
need(re.search(r"flushNow\(\);[^\n]*\n    if \(rtcUpdated\) \{", cc), "the revoke must not return before the current-boot bookkeeping")
need(cc.count("set_lastTimeSync") == 2, "lastTimeSync may be written only by the fault withdrawal and the write-back stamp")
verdict = "if (HibernateWakeDiagnostics::isRtcArm(wakeVerdict.gateArm)) SystemConfig::set_lastTimeSync(0);"
i = app.find(verdict)
need(i != -1 and 0 < app.find("SystemConfig::flushNow();", i) - i < 1500 and 0 <= app.find("classifyWake(reason);\n  " + verdict) , "a failed RTC arm must zero lastTimeSync right after classifyWake() and before the next flushNow()")
PY
}

check_all() { run_unit "$1" && run_c2 "$1" && check_text "$1"; }

check_all "$repo_root/src" || { print -u2 "FAILED: clock_trust_carry checks on the real source"; exit 1; }

# --- mutations (on a scratch copy of src/; the real tree is never touched) ----
mutate() {  # name file old new
  local name="$1" file="$2" old="$3" new="$4" m="$work/mut"
  rm -rf "$m"; mkdir -p "$m"; cp -R "$repo_root/src" "$m/src"
  python3 - "$m/src/$file" "$old" "$new" <<'PY' || { print -u2 "FAILED: mutation '$name' did not apply"; exit 1; }
import sys
p, old, new = sys.argv[1:4]
s = open(p).read()
if old not in s: sys.exit(1)
open(p, "w").write(s.replace(old, new, 1))
PY
  if check_all "$m/src" >/dev/null 2>&1; then print -u2 "FAILED: mutation '$name' was not caught"; exit 1; fi
  echo "caught: $name"
}

mutate "carry ignores the latch" time/ClockTrust.h "lastPlausible && !faultSeen && " "lastPlausible && "
mutate "carry ignores age" time/ClockTrust.h " && nowEpoch - lastSyncEpoch < (int64_t)(kMaxSyncAgeMs / 1000UL)" ""
mutate "age on millis()" time/Clock.cpp "ExternalRtc::isSet(), now, last," "ExternalRtc::isSet(), (time_t)(millis() / 1000), last,"
mutate "abs() dropped" time/ClockTrust.h " && correctionSec >= -kMaxCarryCorrectionSec" ""
mutate "-1 sentinel compared against T" time/ClockTrust.h "written && preReadOk && correctionSec" "written && correctionSec"
mutate "no flush after the revoke" time/Clock.cpp "    if (carryStamp == 0) SystemConfig::flushNow();  // I21: a revoke is flushed synchronously
" ""
mutate "revoke leaves the current boot untrusted" time/Clock.cpp "    if (carryStamp == 0) SystemConfig::flushNow();  // I21: a revoke is flushed synchronously
" "    if (carryStamp == 0) { SystemConfig::flushNow(); return; }
"
mutate "carry without plausibility checks" time/ClockTrust.h "timeOk && externalOk && lastPlausible && " "timeOk && externalOk && "
mutate "carry without plausibility checks (call site)" time/Clock.cpp "Time.isValid() && Clock::isPlausibleEpoch(now), ExternalRtc::isSet()" "Time.isValid(), ExternalRtc::isSet()"
mutate "verdict input ignored" Generalized-Core-Counter.cpp "if (HibernateWakeDiagnostics::isRtcArm(wakeVerdict.gateArm)) SystemConfig::set_lastTimeSync(0);" "if (false) SystemConfig::set_lastTimeSync(0);"
mutate "verdict arm dropped" time/HibernateWakeDiagnostics.h " || a == GateArm::kRtcOrder" ""

echo ""
echo "clock_trust_carry_test passed (unit, C2 regression, call-site text, 11 mutation checks)"
