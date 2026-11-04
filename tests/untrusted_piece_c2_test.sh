#!/bin/zsh
# WO-2026-11-03-006 item 19 C2 - an untrusted session piece is credited, closed
# and reported on millis().
#
# The REAL closeOccupancySessionSafely(), debounceRemainingMs() and
# updateOccupancyState() are extracted from the sources and driven against
# fakes: the Dev-09 replay (23-24 Oct), the reboot straddler (post-boot part
# only), a piece that keeps millis() when trust returns, a trusted piece that is
# unchanged, a fresh piece after each close (both piece fields cleared at close
# and set by every open path), and no wall-start check on a piece. The piece-open and boot sites
# (not drivable without the whole application) are exact-text checks. Every
# mutation in the WO must fail this test.
set -euo pipefail

repo_root="${0:A:h:h}"
work="$repo_root/build-tmp/wo-2026-11-03-006-stage6-r2/c2_test"
mkdir -p "$work"
src="$repo_root/src"

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

# --- structural checks (the sites that cannot be driven on the host) ----------
check_structure() {  # $1 = src root
  local r="$1" modes="$1/state/State_Modes.cpp" app="$1/Generalized-Core-Counter.cpp"
  local open_pair
  open_pair=$'session.occupancyStartMs = millis();\n'
  python3 - "$modes" "$app" "$r/state/StateMachine.h" "$r/state/State_Report.cpp" <<'PY'
import re, sys
modes, app, sm, report = (open(p).read() for p in sys.argv[1:5])
def need(cond, msg):
    if not cond:
        print("FAILED:", msg); sys.exit(1)
# Both sites that open a session record the millis() start and the trust, right after the epoch start.
pat = re.compile(r"set_occupied\(true\);\n(?:\s*//.*\n)*\s*CurrentReadings::set_occupancyStartTime\(Time\.now\(\)\);\n(?:\s*//.*\n)*\s*session\.occupancyStartMs = millis\(\);\n\s*session\.pieceUntrusted = !Clock::isTrusted\(\);\n")
need(len(pat.findall(modes)) == 2, "both piece-open sites must set occupancyStartMs = millis() and pieceUntrusted = !Clock::isTrusted()")
# The wake-close must not hang on the LED when an untrusted piece is open (the C1 verdict's LED-gate gap).
need("OCCUPANCY && (signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0" in modes, "the wake close must also fire for an open untrusted piece")
# The reboot straddler: untrusted piece starting at boot (millis() 0), no B3 anchor for it.
need("session.pieceUntrusted = CurrentReadings::get_occupied() && !Clock::isTrusted();" in app, "setup() must mark an open session at an untrusted boot as an untrusted piece")
need("session.occupancyStartMs = millis();  // straddler" in app, "the boot restore must set occupancyStartMs = millis()")
# The close==24 boundary reopen is a trusted piece: it sets both fields.
need(re.search(r"set_occupancyStartTime\(std::max\(boundary, originalSessionStart\)\);\n\s*session\.occupancyStartMs = millis\(\); session\.pieceUntrusted = !Clock::isTrusted\(\);", report) is not None, "the boundary reopen must set occupancyStartMs = millis() and pieceUntrusted = !Clock::isTrusted()")
need("session.occupancySessionBootAnchor = CurrentReadings::get_occupied() && !session.pieceUntrusted" in app, "the B3 boot anchor must stay for a trusted boot only")
need("pre-boot part dropped" in app, "setup() must log the dropped straddler part")
# RAM only.
need("uint32_t occupancyStartMs" in sm and "bool pieceUntrusted" in sm, "piece fields must be SessionState (RAM) members")
PY
}
check_structure "$src" || exit 1
for f in MyPersistentData.h device_pinout.cpp; do
  if grep -qE 'occupancyStartMs|pieceUntrusted' "$src/$f"; then print -u2 "FAILED: piece fields leaked into $f"; exit 1; fi
done

# --- behavioural driver --------------------------------------------------------
build_and_run() {  # $1 = State_Common.h $2 = State_Modes.cpp
  local out="$work/driver.cpp"
  {
    cat <<'CPP'
#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>
static unsigned long g_millis = 0;
unsigned long millis() { return g_millis; }
static time_t g_now = 0;
struct FakeTime { time_t now() const { return g_now; } };
FakeTime Time;
struct FakeLog {
  std::vector<std::string> lines;
  void add(const char *fmt, va_list ap) { char b[256]; vsnprintf(b, sizeof(b), fmt, ap); lines.push_back(b); }
  void warn(const char *fmt, ...) { va_list ap; va_start(ap, fmt); add(fmt, ap); va_end(ap); }
  void info(const char *fmt, ...) { va_list ap; va_start(ap, fmt); add(fmt, ap); va_end(ap); }
  int count(const char *n) const { int c = 0; for (auto &l : lines) if (l.find(n) != std::string::npos) ++c; return c; }
};
FakeLog Log;
namespace CurrentReadings {
bool occupied = false; time_t occupancyStartTime = 0; uint32_t total = 0; uint32_t lastEvt = 0;
bool get_occupied() { return occupied; }
void set_occupied(bool v) { occupied = v; }
time_t get_occupancyStartTime() { return occupancyStartTime; }
void set_occupancyStartTime(time_t v) { occupancyStartTime = v; }
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
Openness open = Openness::Unknown;
Openness openness() { return open; }
}
namespace DailyBoundary {
struct Result { bool due; time_t boundary; uint8_t close; };
time_t boundary = 0;
Result check(time_t) { return {false, boundary, 0}; }
}
namespace SystemConfig {
enum ConnectionMode { CONNECTED = 0, DISCONNECTED_KEEP_ALIVE = 2, INTERMITTENT_KEEP_ALIVE = 3 };
uint8_t connectionMode = INTERMITTENT_KEEP_ALIVE;
uint8_t get_configuredConnectionMode() { return connectionMode; }
namespace SensorSettings { uint32_t set1 = 300000; uint32_t get_sensorSetting1() { return set1; } }
}
struct PowerManager {
  static PowerManager &instance() { static PowerManager pm; return pm; }
  uint8_t effectiveConnectionMode() const { return SystemConfig::get_configuredConnectionMode(); }
};
namespace Config { uint32_t occupancyDebounceMsForRuntime() { return 300000UL; } }
struct SessionState {
  time_t occupancySessionBootAnchor = 0;
  bool occupancyChangeTriggered = false;
  uint32_t occupancyStartMs = 0;
  bool pieceUntrusted = false;
};
SessionState session;
enum State { IDLE_STATE, REPORTING_STATE };
State state = IDLE_STATE;
int reports = 0;
void transitionTo(State next, const char *) { state = next; ++reports; }
void signalLED(bool) {}
int unoccupied = 0;
void logUnoccupiedEvent(const char *, uint32_t, uint32_t, bool) { ++unoccupied; }
CPP
    extract_braced_block "$1" "inline bool reportsOccupancyChangesNow() {"
    extract_braced_block "$1" "inline bool occupancyIgnoredWhenClosed() {"
    extract_braced_block "$1" "struct OccupancyCloseResult {"
    extract_braced_block "$1" "inline uint32_t occupancyDebounceMs() {"
    extract_braced_block "$1" "inline uint32_t debounceRemainingMs() {"
    extract_braced_block "$1" "inline OccupancyCloseResult closeOccupancySessionSafely(const char *path, time_t closeAt = Time.now()) {"
    extract_braced_block "$2" "void updateOccupancyState() {"
    cat <<'CPP'

// The two piece-open sites, as the (text-checked) source writes them.
void pir() {
  if (!CurrentReadings::occupied) {
    CurrentReadings::occupied = true;
    CurrentReadings::occupancyStartTime = g_now;
    session.occupancyStartMs = millis();
    session.pieceUntrusted = !Clock::isTrusted();
  }
  CurrentReadings::lastEvt = millis();
}
// The close==24 boundary reopen in State_Report.cpp (text-checked): a trusted piece.
void boundaryReopen() {
  CurrentReadings::set_occupied(true);
  CurrentReadings::set_occupancyStartTime(g_now);
  session.occupancyStartMs = millis(); session.pieceUntrusted = !Clock::isTrusted();
}
// One loop pass per second of real time; epoch and millis() advance together.
void run(long seconds) { for (long i = 0; i < seconds; ++i) { g_now += 1; g_millis += 1000; updateOccupancyState(); state = IDLE_STATE; } }
void reset() {  // a reboot with a persisted open session: setup() clears lastEvt, millis() restarts
  g_millis = 0; CurrentReadings::lastEvt = 0;
  session = SessionState();
  session.pieceUntrusted = CurrentReadings::occupied && !Clock::isTrusted();
}
void world(uint32_t set1) {
  Log.lines.clear(); CurrentReadings::occupied = false; CurrentReadings::occupancyStartTime = 0; CurrentReadings::total = 0;
  CurrentReadings::lastEvt = 0; session = SessionState(); Clock::trusted = false; Clock::open = Clock::Openness::Unknown;
  DailyBoundary::boundary = 0; SystemConfig::SensorSettings::set1 = set1; g_millis = 7000000; g_now = 1792000000;
  state = IDLE_STATE; reports = 0; unoccupied = 0;
}
// A pulsed session: PIR every 30 s for `pulses` pulses, then the debounce runs out.
void session_of(int pulses) { for (int i = 0; i < pulses; ++i) { pir(); run(30); } }

// Dev-09 timeline: a session every 20 min; ten PIR pulses 30 s apart (270 s) then a 300 s debounce = 570 s.
void gap(long total, long used) { run(total - used); }

void testControlCreditsEachPieceItsOwnOnTime() {
  world(300000);
  for (int k = 0; k < 10; ++k) {
    session_of(10);
    gap(1200, 300);
    assert(!CurrentReadings::occupied);
    assert(CurrentReadings::total == (uint32_t)(570 * (k + 1)));
  }
  assert(unoccupied == 10 && reports == 10);            // every close raises the occupancy-change report
  assert(Log.count("Occ: untrusted piece credit=570") == 10);
}

// The reset at 10:40:55 (334 s into the first session): the straddler credits its post-boot part only.
void testRebootStraddlerCreditsPostBootOnly() {
  world(300000);
  session_of(10);                                        // pulses to 270 s, then 30 s more
  // reboot 300 s into the piece (persisted open, untrusted clock): millis() restarts
  reset();
  run(1000);
  assert(!CurrentReadings::occupied);
  // no PIR after the reboot: the debounce re-arms at the first loop pass (1 s) and runs 300 s => 301 s, not 570 s
  assert(CurrentReadings::total == 301);
  assert(Log.count("Occ: untrusted piece credit=301") == 1);

  // The 236 s case (only with a 60 s debounce, not the 300 s Dev-14 setting): 60 s debounce, PIR pulses continue 176 s after the reboot.
  world(60000);
  for (int i = 0; i < 11; ++i) { pir(); run(30); }       // 0..330 s, session open
  reset();
  for (int i = 0; i < 6; ++i) { run(i == 0 ? 26 : 30); pir(); }   // post-boot PIRs at 26..176 s
  run(1000);
  assert(CurrentReadings::total == 236);                 // 176 + 60; 536 would be a one-debounce fallback
}

void testDev09Replay() {
  // Control: ten pieces close by 14:00 (the eleventh opens at 13:55:21), each its own 570 s.
  world(300000);
  for (int k = 0; k < 10; ++k) { session_of(10); gap(1200, 300); }
  assert(CurrentReadings::total == 5700);                 // pre-boundary checkpoint (14:00): 95.0 min; v45 credited 205 min
  {  // the eleventh piece (opened 13:55:21) at the close report after trust returns past the boundary: full 570 s
    session_of(10);
    Clock::trusted = true; Clock::open = Clock::Openness::Closed; DailyBoundary::boundary = g_now - 9;
    const OccupancyCloseResult r = closeOccupancySessionSafely("daily-cleanup", DailyBoundary::boundary);
    assert(r.valid && r.sessionSeconds == 570);
    assert(CurrentReadings::total == 6270);               // 104.5 min (payload 104)
    Clock::open = Clock::Openness::Unknown;
  }
  // Fault pass: reset 334 s into the first piece (after its last pulse at 270 s); the other nine are whole.
  world(300000);
  session_of(10);                                         // 300 s: pulses to 270 s, piece open
  run(34);                                                // 334 s: the reset
  reset();
  gap(1200 - 334, 0);                                     // no PIR after the reboot: the straddler closes post-boot
  for (int k = 1; k < 10; ++k) { session_of(10); gap(1200, 300); }
  assert(CurrentReadings::total == 301 + 9 * 570);        // pre-boundary: 5,431 s = 90.5 min, never the 205 of the merged span
  assert(Log.count("Occ: untrusted piece credit=301") == 1);
  assert(unoccupied == 10);
  session_of(10);                                         // the eleventh piece, closed at the report after the boundary
  Clock::trusted = true; Clock::open = Clock::Openness::Closed; DailyBoundary::boundary = g_now - 9;
  assert(closeOccupancySessionSafely("daily-cleanup", DailyBoundary::boundary).sessionSeconds == 570);
  assert(CurrentReadings::total == 6001);                 // 100.0 min (payload 100)
  Clock::open = Clock::Openness::Unknown;
}

// Trust returns while the piece is open: the wall clock jumps, the credit stays on millis().
void testPieceOpenedUntrustedKeepsMillisWhenTrustReturns() {
  world(300000);
  pir(); run(60); pir();
  Clock::trusted = true; g_now += 36000;                 // the sync step
  run(1000);
  assert(!CurrentReadings::occupied);
  assert(CurrentReadings::total == 360);                 // 60 + 300, not 36360
  assert(session.pieceUntrusted == false);
  // ... and the next piece, opened trusted, is a wall piece: the same code credits its wall span.
  pir(); run(60); pir(); g_now += 100; run(1000);
  assert(CurrentReadings::total == 360 + 460);
}

// A piece opened trusted is unchanged: it credits the wall span (a wall step shows through).
void testTrustedPieceUnchanged() {
  world(300000);
  Clock::trusted = true;
  pir(); run(60); pir(); g_now += 100; run(1000);
  assert(!CurrentReadings::occupied);
  assert(CurrentReadings::total == 460);                 // wall: 60 + 100 + 300; millis would say 360
  assert(Log.count("untrusted piece") == 0);
}

// Close clears both piece fields; the boundary reopen (a trusted piece) and a fresh open set them.
void testClosedPieceClearsFieldsAndReopenSetsThem() {
  world(300000);
  pir(); run(400);
  assert(!CurrentReadings::occupied && !session.pieceUntrusted && session.occupancyStartMs == 0);
  Clock::trusted = true; g_millis += 12345;
  boundaryReopen();
  assert(CurrentReadings::occupied && !session.pieceUntrusted && session.occupancyStartMs == g_millis);
  CurrentReadings::lastEvt = g_millis;
  run(400);                                              // trusted: credits the wall span 0..300 + 100 elapsed
  assert(!CurrentReadings::occupied && CurrentReadings::total == 300 + 300);
  assert(Log.count("untrusted piece credit") == 1);      // only the first, untrusted piece used millis
}

// Wall-start checks belong to the trusted wall path only (a piece is wholly on millis()).
void testPieceIgnoresWallStartChecks() {
  world(300000);
  pir(); run(60); pir();
  Clock::trusted = true; g_now -= 36000;                 // ten-hour backward correction
  run(1000);
  assert(!CurrentReadings::occupied && CurrentReadings::total == 360);
  world(300000);
  pir(); run(60); pir();
  CurrentReadings::occupancyStartTime = 0;               // zero wall start
  Clock::trusted = true;
  run(1000);
  assert(!CurrentReadings::occupied && CurrentReadings::total == 360);
  // A trusted piece with a backward step is still discarded; so is a trusted zero start.
  world(300000);
  Clock::trusted = true;
  pir(); run(60); pir(); g_now -= 36000; run(1000);
  assert(!CurrentReadings::occupied && CurrentReadings::total == 0 && Log.count("OccAnom") == 1);
  world(300000);
  Clock::trusted = true;
  pir(); CurrentReadings::occupancyStartTime = 0; run(1000);
  assert(!CurrentReadings::occupied && CurrentReadings::total == 0 && Log.count("OccAnom") == 1);
}

// A fresh piece after each close: its start is its own (millis and flag are not carried over).
void testNextPieceStartsClean() {
  world(300000);
  pir(); const uint32_t before = session.occupancyStartMs; run(400);
  assert(!CurrentReadings::occupied && CurrentReadings::total == 300);
  run(5000);
  pir();
  assert(session.occupancyStartMs > before + 5000000UL);
  run(400);
  assert(CurrentReadings::total == 600);                 // not 300 + 5300
}

// A mode that does not report on occupancy changes still credits, without a REPORTING transition.
void testNoReportInQuietModes() {
  world(300000);
  SystemConfig::connectionMode = SystemConfig::DISCONNECTED_KEEP_ALIVE;
  pir(); run(400);
  assert(CurrentReadings::total == 300 && reports == 0 && unoccupied == 1);
  SystemConfig::connectionMode = SystemConfig::INTERMITTENT_KEEP_ALIVE;
}

// Trust returned mid-piece and the day's boundary is due: the explicit-closeAt close still credits on-time.
void testBoundaryCloseOfAnUntrustedPiece() {
  world(300000);
  pir(); run(100);
  Clock::trusted = true; Clock::open = Clock::Openness::Closed; DailyBoundary::boundary = g_now - 50;
  const OccupancyCloseResult r = closeOccupancySessionSafely("daily-cleanup", DailyBoundary::boundary);
  assert(r.valid && !r.stillOpen && !CurrentReadings::occupied);
  assert(r.sessionSeconds == 300);                       // measured (last PIR to expiry), not the wall span
  Clock::open = Clock::Openness::Unknown;
}

int main() {
  testDev09Replay();
  testControlCreditsEachPieceItsOwnOnTime();
  testRebootStraddlerCreditsPostBootOnly();
  testPieceOpenedUntrustedKeepsMillisWhenTrustReturns();
  testTrustedPieceUnchanged();
  testClosedPieceClearsFieldsAndReopenSetsThem();
  testPieceIgnoresWallStartChecks();
  testNextPieceStartsClean();
  testNoReportInQuietModes();
  testBoundaryCloseOfAnUntrustedPiece();
  std::cout << "driver ok\n";
  return 0;
}
CPP
  } > "$out"
  clang++ -std=c++17 -w -o "$work/driver" "$out" && "$work/driver"
}
build_and_run "$src/state/State_Common.h" "$src/state/State_Modes.cpp"
print "driver and structure ok"

# --- mutations: each must fail the driver or the structural checks ------------
mutate() {  # name file old new
  local name="$1" file="$2" old="$3" new="$4" m="$work/mut"
  rm -rf "$m"; mkdir -p "$m/state"
  cp "$src/state/State_Common.h" "$src/state/State_Modes.cpp" "$src/state/StateMachine.h" "$m/state/"
  cp "$src/Generalized-Core-Counter.cpp" "$m/"
  cp "$src/state/State_Report.cpp" "$m/state/"
  python3 - "$m/$file" "$old" "$new" <<'PY'
import sys
p, old, new = sys.argv[1:]
s = open(p).read()
if s.count(old) != 1:
    print("MUTATION_NOT_APPLIED:", old, s.count(old)); sys.exit(3)
open(p, "w").write(s.replace(old, new))
PY
  (( $? == 0 )) || { print -u2 "mutation did not apply: $name"; exit 1; }
  local caught=0
  check_structure "$m" >/dev/null 2>&1 || caught=1
  build_and_run "$m/state/State_Common.h" "$m/state/State_Modes.cpp" >/dev/null 2>&1 || caught=1
  if (( caught )); then print "caught: $name"; else print -u2 "NOT CAUGHT: $name"; exit 1; fi
}

mutate "keep the B2 re-arm for an untrusted piece" state/State_Common.h "const bool piece = occupied && session.pieceUntrusted;" "const bool piece = false;"
mutate "credit the wall span" state/State_Common.h "rawSessionSeconds = (CurrentReadings::get_lastOccupancyEvent() - session.occupancyStartMs + occupancyDebounceMs()) / 1000UL;" "rawSessionSeconds = (long long)Time.now() - (long long)start;"
mutate "credit from the persisted wall start" state/State_Common.h "- session.occupancyStartMs +" "- (uint32_t)CurrentReadings::get_occupancyStartTime() +"
mutate "one-debounce fallback at boot" state/State_Common.h "		durationComputable = true;
		Log.info(\"Occ: untrusted" "		if (session.occupancyStartMs == 0) rawSessionSeconds += occupancyDebounceMs() / 1000UL;
		durationComputable = true;
		Log.info(\"Occ: untrusted"
mutate "start a trusted piece on millis()" state/State_Common.h "const bool piece = occupied && session.pieceUntrusted;" "const bool piece = occupied;"
mutate "mark a trusted piece untrusted at open" state/State_Modes.cpp $'\n      session.pieceUntrusted = !Clock::isTrusted();' $'\n      session.pieceUntrusted = true;'
mutate "drop the report call" state/State_Modes.cpp "if (reportNow && !closeResult.stillOpen) {
      session.occupancyChangeTriggered = true;
      if (state == IDLE_STATE) {
        transitionTo(REPORTING_STATE, \"occupancy cleared\");" "if (false) {
      session.occupancyChangeTriggered = true;
      if (state == IDLE_STATE) {
        transitionTo(REPORTING_STATE, \"occupancy cleared\");"
mutate "reuse the old piece's start (awake open)" state/State_Modes.cpp $'\n      session.occupancyStartMs = millis();\n' $'\n'
mutate "straddler not marked at boot" Generalized-Core-Counter.cpp "session.pieceUntrusted = CurrentReadings::get_occupied() && !Clock::isTrusted();" "session.pieceUntrusted = false;"
mutate "wake close gated on the LED again" state/State_Modes.cpp "(signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied()))" "signalLEDStatus()"

mutate "close leaves the piece start stale" state/State_Common.h "session.pieceUntrusted = false; session.occupancyStartMs = 0;" "session.pieceUntrusted = false;"
mutate "close leaves the piece flag set" state/State_Common.h "session.pieceUntrusted = false; session.occupancyStartMs = 0;" "session.occupancyStartMs = 0;"
mutate "boundary reopen without the piece fields" state/State_Report.cpp "      session.occupancyStartMs = millis(); session.pieceUntrusted = !Clock::isTrusted();  // trusted piece
" ""
mutate "boot restore without occupancyStartMs" Generalized-Core-Counter.cpp " session.occupancyStartMs = millis();  // straddler" "  // straddler"
mutate "restore start==0 check for pieces" state/State_Common.h "bool invalid = !piece && (!timeValid || start == 0);" "bool invalid = (!timeValid && !piece) || start == 0;"
mutate "restore start>now+5 check for pieces" state/State_Common.h "if (!piece && timeValid && start > now + 5) {" "if (timeValid && start > now + 5) {"

print "untrusted_piece_c2_test: Dev-09 replay, straddler, trust-return, trusted-unchanged and all mutations detected"
