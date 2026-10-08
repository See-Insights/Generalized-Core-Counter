#!/bin/zsh
# WO-2026-10-04-001 item B - a restart during an occupancy session does not
# lose that session's minutes.
#
# Three defects, one fix:
#
#  B1  `lastOccupancyEvent` is a millis() value but it is PERSISTED, so after a
#      restart the debounce compared this boot's millis() against the previous
#      boot's. setup() now clears it to 0, which the existing zero sentinel in
#      State_Idle/State_Modes re-arms from.
#  B2  `closeOccupancySessionSafely()` treated an untrusted clock as an invalid
#      session and cleared `occupied`/`occupancyStartTime` anyway, throwing the
#      session away and signalling an unoccupied transition that never
#      happened. It now keeps the session open, credits nothing, and re-arms
#      the debounce so the close is retried one debounce later.
#  B3  A session open across a restart was credited from its (pre-restart)
#      start to the close time, including all the time the device was off. The
#      clock is never trusted during setup() (Clock::isTrusted() needs a cloud
#      sync observed this boot), so a RAM-only flag set in setup() makes the
#      FIRST TRUSTED CLOSE of that session cap `closeAt` at
#      min(closeAt, bootEpoch, anchor + debounce), anchor = max(start,
#      lastReport) CAPTURED ONCE IN setup(). Capturing it at boot matters: a
#      report sent later in the boot, before the clock is trusted, advances
#      lastReport and would otherwise move the cap (Stage 7 finding). Over-credit
#      is then bounded by one debounce period however long the device was off.
#
# Behavioural test: the REAL closeOccupancySessionSafely(), occupancyDebounceMs()
# and updateOccupancyState() are extracted verbatim from the sources and driven
# against fakes. Part 1 is the one thing that cannot be driven - setup()'s boot
# clear - and is an exact-text check.
set -euo pipefail

repo_root="${0:A:h:h}"
common_src="$repo_root/src/state/State_Common.h"
modes_src="$repo_root/src/state/State_Modes.cpp"
app_src="$repo_root/src/Generalized-Core-Counter.cpp"
generated="${TMPDIR:-/tmp}/occupancy_session_restart_test.cpp"
binary="${TMPDIR:-/tmp}/occupancy_session_restart_test"

extract_braced_block() {
  local file="$1"
  local marker="$2"
  local out
  out=$(awk -v marker="$marker" '
    !active && index($0, marker) { active = 1 }
    active {
      print
      opens = gsub(/\{/, "{")
      closes = gsub(/\}/, "}")
      depth += opens - closes
      if (seen_open && depth == 0) exit
      if (opens > 0) seen_open = 1
    }
  ' "$file")
  if [[ -z "$out" ]]; then
    print -u2 "EXTRACT_FAILED: $file has no block starting with: $marker"
    exit 1
  fi
  print -r -- "$out"
}

# --- Part 1: the boot clear (B1) and the cross-boot flag (B3) -----------------
#
# setup() runs once, before any state handler, and cannot be driven on the host
# without standing up the whole application. These two statements must be in
# setup(), immediately after the persistent data loads and BEFORE anything can
# read lastOccupancyEvent or close a session.

setup_block=$(extract_braced_block "$app_src" "void setup() {")

for required in \
  "session.occupancySessionBootAnchor = CurrentReadings::get_occupied()" \
  "? std::max(CurrentReadings::get_occupancyStartTime(), SystemConfig::get_lastReport()) : 0;" \
  "CurrentReadings::set_lastOccupancyEvent(0);"
do
  if ! print -r -- "$setup_block" | grep -qF -- "$required"; then
    print -u2 "FAILED: setup() is missing: $required"
    exit 1
  fi
done

# Ordering: both must come after CurrentReadings::setup() (the load) ...
load_at=$(print -r -- "$setup_block" | grep -nF 'CurrentReadings::setup();' | head -1 | cut -d: -f1)
flag_at=$(print -r -- "$setup_block" | grep -nF 'session.occupancySessionBootAnchor = CurrentReadings::get_occupied()' | head -1 | cut -d: -f1)
clear_at=$(print -r -- "$setup_block" | grep -nF 'CurrentReadings::set_lastOccupancyEvent(0);' | head -1 | cut -d: -f1)

if (( load_at >= flag_at )); then
  print -u2 "FAILED: the cross-boot flag is sampled before the persistent data is loaded"
  exit 1
fi
# ... and the flag must be sampled BEFORE the clear, since the clear would
# otherwise be free to change what the flag observes.
if (( flag_at >= clear_at )); then
  print -u2 "FAILED: lastOccupancyEvent is cleared before the open-session flag is sampled"
  exit 1
fi

# The flag is RAM-only: it lives in SessionState, which is explicitly not
# persisted and not retained. No new persisted field is allowed by this WO.
grep -qF 'time_t occupancySessionBootAnchor = 0;' "$repo_root/src/state/StateMachine.h" || {
  print -u2 "FAILED: occupancySessionBootAnchor is not a SessionState (RAM-only) field"; exit 1; }
if grep -qF 'occupancySessionBootAnchor' "$repo_root/src/MyPersistentData.h"; then
  print -u2 "FAILED: occupancySessionBootAnchor must not become a persisted field"
  exit 1
fi
if grep -rqE 'retained[^;]*occupancySessionBootAnchor' "$repo_root/src"; then
  print -u2 "FAILED: occupancySessionBootAnchor must not be retained"
  exit 1
fi

# --- Part 2: drive the real close and the real debounce ----------------------
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

// ----- Fake Device OS / application surface ----------------------------------

static unsigned long g_millis = 0;
unsigned long millis() { return g_millis; }

static time_t g_now = 0;
struct FakeTime {
  time_t now() const { return g_now; }
};
FakeTime Time;

struct FakeLog {
  std::vector<std::string> lines;
  void warn(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    lines.push_back(buf);
  }
  void info(const char *fmt, ...) { (void)fmt; }
  void trace(const char *fmt, ...) { (void)fmt; }
  int count(const char *needle) const {
    int n = 0;
    for (const std::string &l : lines) {
      if (l.find(needle) != std::string::npos) ++n;
    }
    return n;
  }
};
FakeLog Log;

namespace CurrentReadings {
bool occupied = false;
time_t occupancyStartTime = 0;
uint32_t totalOccupiedSeconds = 0;
uint32_t lastOccupancyEvent = 0;
bool get_occupied() { return occupied; }
void set_occupied(bool v) { occupied = v; }
time_t get_occupancyStartTime() { return occupancyStartTime; }
void set_occupancyStartTime(time_t v) { occupancyStartTime = v; }
uint32_t get_totalOccupiedSeconds() { return totalOccupiedSeconds; }
void set_totalOccupiedSeconds(uint32_t v) { totalOccupiedSeconds = v; }
uint32_t get_lastOccupancyEvent() { return lastOccupancyEvent; }
void set_lastOccupancyEvent(uint32_t v) { lastOccupancyEvent = v; }
}  // namespace CurrentReadings

namespace RecoveryState {
int8_t alertCode = 0;
int8_t get_alertCode() { return alertCode; }
}  // namespace RecoveryState

namespace Clock {
bool trusted = true;
bool isTrusted() { return trusted; }
}  // namespace Clock

namespace SystemConfig {
enum ConnectionMode { CONNECTED = 0, DISCONNECTED_KEEP_ALIVE = 2, INTERMITTENT_KEEP_ALIVE = 3 };
uint8_t connectionMode = INTERMITTENT_KEEP_ALIVE;
uint8_t get_configuredConnectionMode() { return connectionMode; }
time_t lastReport = 0;
time_t get_lastReport() { return lastReport; }
namespace SensorSettings {
uint32_t sensorSetting1 = 0;
uint32_t get_sensorSetting1() { return sensorSetting1; }
}  // namespace SensorSettings
}  // namespace SystemConfig

// This test never sets lowBatteryMode, so the mode in use equals the configured one.
struct PowerManager {
  static PowerManager &instance() {
    static PowerManager pm;
    return pm;
  }
  uint8_t effectiveConnectionMode() const { return SystemConfig::get_configuredConnectionMode(); }
};

namespace Config {
uint32_t occupancyDebounceMsForRuntime() { return 300000UL; }  // the shipped default, 5 minutes
}  // namespace Config

struct SessionState {
  time_t occupancySessionBootAnchor = 0;
  bool occupancyChangeTriggered = false;
};
SessionState session;

// updateOccupancyState()'s collaborators.
enum State { IDLE_STATE, REPORTING_STATE, SLEEPING_STATE };
State state = IDLE_STATE;
int transitionCount = 0;
void transitionTo(State next, const char *reason) {
  (void)reason;
  state = next;
  ++transitionCount;
}
bool ledOn = false;
void signalLED(bool on) { ledOn = on; }
int unoccupiedEvents = 0;
uint32_t lastLoggedSessionSeconds = 0;
void logUnoccupiedEvent(const char *why, uint32_t sessionSeconds, uint32_t totalSeconds, bool reportNow) {
  (void)why;
  (void)totalSeconds;
  (void)reportNow;
  ++unoccupiedEvents;
  lastLoggedSessionSeconds = sessionSeconds;
}
CPP

  echo ""
  echo "// ===== Extracted verbatim from src/state/State_Common.h ====="
  extract_braced_block "$common_src" "inline bool reportsOccupancyChangesNow() {"
  echo ""
  extract_braced_block "$common_src" "struct OccupancyCloseResult {"
  echo ""
  extract_braced_block "$common_src" "inline uint32_t occupancyDebounceMs() {"
  echo ""
  extract_braced_block "$common_src" \
    "inline OccupancyCloseResult closeOccupancySessionSafely(const char *path, time_t closeAt = Time.now()) {"

  echo ""
  echo "// ===== Extracted verbatim from src/state/State_Modes.cpp ====="
  extract_braced_block "$modes_src" "void updateOccupancyState() {"

  cat <<'CPP'

// ----- Test driver ------------------------------------------------------------

const uint32_t kDebounceMs = 300000UL;   // Config::occupancyDebounceMsForRuntime()
const time_t kDebounceSec = 300;

// A device that has been occupied since `startEpoch` and has just rebooted:
// setup() cleared lastOccupancyEvent and captured the cross-boot anchor.
void bootMidSession(time_t startEpoch, time_t bootEpoch, unsigned long sinceBootMs, time_t lastReport) {
  Log.lines.clear();
  CurrentReadings::occupied = true;
  CurrentReadings::occupancyStartTime = startEpoch;
  CurrentReadings::totalOccupiedSeconds = 0;
  CurrentReadings::lastOccupancyEvent = 0;        // setup()'s clear (B1)
  SystemConfig::lastReport = lastReport;
  SystemConfig::SensorSettings::sensorSetting1 = 0;
  RecoveryState::alertCode = 0;
  Clock::trusted = true;
  session.occupancySessionBootAnchor = std::max(startEpoch, lastReport);  // setup()'s anchor (B3)
  session.occupancyChangeTriggered = false;
  state = IDLE_STATE;
  transitionCount = 0;
  unoccupiedEvents = 0;
  g_millis = sinceBootMs;
  g_now = bootEpoch + (time_t)(sinceBootMs / 1000UL);
}

// --- 1. The debounce period is the one the debounce checks use.
void testDebounceSourceIsShared() {
  SystemConfig::SensorSettings::sensorSetting1 = 0;
  assert(occupancyDebounceMs() == kDebounceMs);
  SystemConfig::SensorSettings::sensorSetting1 = 90000UL;
  assert(occupancyDebounceMs() == 90000UL);
  SystemConfig::SensorSettings::sensorSetting1 = 0;
}

// --- 2. B3: a mid-session restart credits up to the moment this boot started,
//        when that is earlier than anchor + debounce. The old code credited
//        through to the close time, counting the power-off as occupancy.
void testRestartCreditsToBootTimeWhenEarlierThanTheCap() {
  // Occupied since 1000. The device restarted at 1100 (100 s of real session)
  // and the first trusted close happens 50 s into the new boot.
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/1100, /*sinceBootMs=*/50000, /*lastReport=*/0);
  // anchor = max(1000, 0) = 1000, anchor + debounce = 1300; bootEpoch = 1100.
  const OccupancyCloseResult r = closeOccupancySessionSafely("test");
  assert(r.valid);
  assert(!r.stillOpen);
  assert(r.sessionSeconds == 100);        // 1000 -> 1100, the boot moment
  assert(r.totalSeconds == 100);
  assert(!CurrentReadings::occupied);     // a trusted close really closes
  assert(CurrentReadings::occupancyStartTime == 0);
  assert(Log.count("OccAnom") == 0);
  // The flag is one-shot: a later session in the same boot is not capped.
  assert(session.occupancySessionBootAnchor == 0);
}

// --- 3. B3: the cap bites when the device was off for longer than a debounce.
void testRestartIsCappedAtAnchorPlusOneDebounce() {
  // Occupied since 1000, the device was off for a while and booted at 2000.
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/2000, /*sinceBootMs=*/50000, /*lastReport=*/0);
  const OccupancyCloseResult r = closeOccupancySessionSafely("test");
  assert(r.valid);
  assert(r.sessionSeconds == (uint32_t)kDebounceSec);   // 1000 -> 1300
  assert(Log.count("OccAnom") == 0);
}

// --- 4. B-anchor: lastReport later than the session start moves the anchor,
//        because a report proves the session was still open at that moment.
void testLastReportMovesTheAnchor() {
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/9000, /*sinceBootMs=*/50000, /*lastReport=*/5000);
  // anchor = max(1000, 5000) = 5000, so the credit runs 1000 -> 5300.
  const OccupancyCloseResult r = closeOccupancySessionSafely("test");
  assert(r.valid);
  assert(r.sessionSeconds == (uint32_t)(5000 + kDebounceSec - 1000));
  assert(r.sessionSeconds == 4300);

  // A lastReport EARLIER than the start must not move the anchor backwards.
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/9000, /*sinceBootMs=*/50000, /*lastReport=*/400);
  const OccupancyCloseResult r2 = closeOccupancySessionSafely("test");
  assert(r2.valid);
  assert(r2.sessionSeconds == (uint32_t)kDebounceSec);  // anchor stayed at 1000
}

// --- 5. B3 acceptance: a long power-off over-credits by at most one debounce,
//        however long the device was off.
void testLongPowerOffOverCreditsAtMostOneDebounce() {
  for (const time_t offSeconds : {(time_t)600, (time_t)3600, (time_t)36000, (time_t)864000}) {
    const time_t start = 1000;
    const time_t lastKnownOpen = start;                 // the anchor
    bootMidSession(start, /*bootEpoch=*/start + offSeconds, /*sinceBootMs=*/50000, /*lastReport=*/0);
    const OccupancyCloseResult r = closeOccupancySessionSafely("test");
    assert(r.valid);
    const time_t creditedThrough = start + (time_t)r.sessionSeconds;
    const time_t overCredit = creditedThrough - lastKnownOpen;
    assert(overCredit <= kDebounceSec);
  }
  // Ten hours off: exactly one debounce of over-credit, not ten hours of it.
  bootMidSession(1000, /*bootEpoch=*/1000 + 36000, /*sinceBootMs=*/50000, /*lastReport=*/0);
  const OccupancyCloseResult r = closeOccupancySessionSafely("test");
  assert(r.sessionSeconds == (uint32_t)kDebounceSec);
}

// --- 6. The cap applies to the FIRST trusted close only. A session that
//        started cleanly in this boot is credited in full.
void testNormalCloseIsNotCapped() {
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/1000, /*sinceBootMs=*/50000, /*lastReport=*/0);
  session.occupancySessionBootAnchor = 0;               // no session was open at boot
  CurrentReadings::occupancyStartTime = 1000;
  g_now = 1000 + 3600;                                  // a one-hour session
  const OccupancyCloseResult r = closeOccupancySessionSafely("test");
  assert(r.valid);
  assert(r.sessionSeconds == 3600);                     // the whole hour, uncapped
}

// --- 7. B2: an untrusted clock keeps the session open, credits nothing, and
//        re-arms the debounce. The old code cleared the session instead.
void testUntrustedClockKeepsTheSessionOpen() {
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/1100, /*sinceBootMs=*/50000, /*lastReport=*/0);
  CurrentReadings::totalOccupiedSeconds = 777;
  Clock::trusted = false;

  const OccupancyCloseResult r = closeOccupancySessionSafely("idle");
  assert(!r.valid);
  assert(r.stillOpen);
  assert(r.sessionSeconds == 0);
  assert(CurrentReadings::occupied);                              // still open
  assert(CurrentReadings::occupancyStartTime == 1000);            // start preserved
  assert(CurrentReadings::totalOccupiedSeconds == 777);           // nothing credited
  assert(CurrentReadings::lastOccupancyEvent == (uint32_t)g_millis);  // debounce re-armed
  assert(Log.count("OccAnom") == 0);                              // not an anomaly
  // The cross-boot flag survives, so the credit still happens at the first
  // close that IS trusted.
  assert(session.occupancySessionBootAnchor != 0);

  Clock::trusted = true;
  g_millis += kDebounceMs;
  g_now += (time_t)kDebounceSec;
  const OccupancyCloseResult r2 = closeOccupancySessionSafely("idle");
  assert(r2.valid);
  assert(!r2.stillOpen);
  assert(!CurrentReadings::occupied);
  assert(r2.sessionSeconds == 100);   // still capped at the boot moment, 1000 -> 1100
}

// --- 8. B2: no report storm and no repeated unoccupied transition while the
//        clock is untrusted. This drives the REAL updateOccupancyState().
void testUntrustedClockCausesNoReportStorm() {
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/1100, /*sinceBootMs=*/1000, /*lastReport=*/0);
  Clock::trusted = false;
  SystemConfig::connectionMode = SystemConfig::INTERMITTENT_KEEP_ALIVE;

  // B1: lastOccupancyEvent is 0 after the boot clear, so the first pass
  // re-arms it from THIS boot's millis() rather than expiring immediately on
  // the previous boot's value.
  updateOccupancyState();
  assert(CurrentReadings::lastOccupancyEvent == 1000);
  assert(transitionCount == 0);
  assert(CurrentReadings::occupied);

  // Run for an hour of simulated time, a loop pass every 10 s.
  for (int i = 0; i < 360; ++i) {
    g_millis += 10000;
    g_now += 10;
    updateOccupancyState();
  }
  assert(CurrentReadings::occupied);          // never thrown away
  assert(transitionCount == 0);               // no REPORTING_STATE storm
  assert(unoccupiedEvents == 0);              // no unoccupied transition signalled
  assert(Log.count("OccAnom") == 0);          // no OccAnom storm
  assert(CurrentReadings::totalOccupiedSeconds == 0);

  // Each attempt re-armed the debounce, so attempts are one debounce apart,
  // not once per loop pass.
  assert(CurrentReadings::lastOccupancyEvent > kDebounceMs);

  // Once the clock is trusted, the very next expiry closes and reports.
  Clock::trusted = true;
  g_millis += kDebounceMs + 1;
  g_now += (time_t)kDebounceSec + 1;
  updateOccupancyState();
  assert(!CurrentReadings::occupied);
  assert(unoccupiedEvents == 1);
  assert(transitionCount == 1);
  assert(state == REPORTING_STATE);
  assert(session.occupancyChangeTriggered);
  // 1000 -> the boot moment (1100), the capped credit. bootEpoch is recomputed
  // at close time as Time.now() - millis()/1000, so the integer-second floor
  // can land one second either side.
  assert(lastLoggedSessionSeconds >= 100 && lastLoggedSessionSeconds <= 101);
}

// --- 9. The pre-existing safety checks are untouched: a zero or future start
//        is still an anomaly, and is still cleared.
void testExistingAnomalyHandlingIsUnchanged() {
  bootMidSession(/*startEpoch=*/0, /*bootEpoch=*/1100, /*sinceBootMs=*/50000, /*lastReport=*/0);
  session.occupancySessionBootAnchor = 0;
  const OccupancyCloseResult zeroStart = closeOccupancySessionSafely("zero");
  assert(!zeroStart.valid);
  assert(!zeroStart.stillOpen);
  assert(!CurrentReadings::occupied);
  assert(Log.count("OccAnom") == 1);

  bootMidSession(/*startEpoch=*/0, /*bootEpoch=*/1100, /*sinceBootMs=*/50000, /*lastReport=*/0);
  session.occupancySessionBootAnchor = 0;
  CurrentReadings::occupancyStartTime = g_now + 600;   // well in the future
  const OccupancyCloseResult future = closeOccupancySessionSafely("future");
  assert(!future.valid);
  assert(!future.stillOpen);
  assert(!CurrentReadings::occupied);
  assert(Log.count("OccAnom") == 1);

  // A session longer than a day is still rejected.
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/1000, /*sinceBootMs=*/50000, /*lastReport=*/0);
  session.occupancySessionBootAnchor = 0;
  g_now = 1000 + 90000;
  const OccupancyCloseResult tooLong = closeOccupancySessionSafely("long");
  assert(!tooLong.valid);
  assert(!CurrentReadings::occupied);
  assert(Log.count("OccAnom") == 1);
}

// --- 10. A boundary-aware close (the daily cleanup) still honours its
//         explicit closeAt, and is capped the same way when it is the first
//         trusted close after a restart.
void testExplicitCloseAtStillWins() {
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/1100, /*sinceBootMs=*/50000, /*lastReport=*/0);
  session.occupancySessionBootAnchor = 0;
  const OccupancyCloseResult r = closeOccupancySessionSafely("daily-cleanup", 1050);
  assert(r.valid);
  assert(r.sessionSeconds == 50);

  // With the cross-boot flag set, a boundary later than the cap is capped.
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/2000, /*sinceBootMs=*/50000, /*lastReport=*/0);
  const OccupancyCloseResult capped = closeOccupancySessionSafely("daily-cleanup", 2040);
  assert(capped.valid);
  assert(capped.sessionSeconds == (uint32_t)kDebounceSec);

  // ... and a boundary EARLIER than the cap still wins, because the cap is a
  // ceiling, not a target.
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/2000, /*sinceBootMs=*/50000, /*lastReport=*/0);
  const OccupancyCloseResult early = closeOccupancySessionSafely("daily-cleanup", 1100);
  assert(early.valid);
  assert(early.sessionSeconds == 100);
}

// --- 11. Stage 7 regression: a report is sent after the restart but before
//         the clock is trusted. That report advances lastReport; the credit
//         must still be capped at the BOOT-TIME anchor plus one debounce.
//         Codex's reproduction: 300 s debounce, session from 1000, last report
//         and power-off at 5000, ten hours off, boot at 41000.
void testReportAfterRestartBeforeTrustDoesNotMoveTheCap() {
  const time_t start = 1000;
  const time_t lastKnownOpen = 5000;   // the last report before the power-off
  bootMidSession(start, /*bootEpoch=*/lastKnownOpen + 36000, /*sinceBootMs=*/10000, /*lastReport=*/lastKnownOpen);

  // Untrusted: a close attempt keeps the session open and credits nothing.
  Clock::trusted = false;
  const OccupancyCloseResult untrusted = closeOccupancySessionSafely("idle");
  assert(untrusted.stillOpen);
  assert(CurrentReadings::totalOccupiedSeconds == 0);

  // A scheduled report goes out after the boot, still untrusted.
  g_millis += 40000;
  g_now += 40;
  SystemConfig::lastReport = g_now;    // 41050, the State_Report.cpp lastReport write

  // The clock becomes trusted and the session closes.
  Clock::trusted = true;
  g_millis += kDebounceMs;
  g_now += (time_t)kDebounceSec;
  const OccupancyCloseResult r = closeOccupancySessionSafely("idle");
  assert(r.valid);
  assert(r.sessionSeconds == 4300);    // 1000 -> 5000 + 300, not 1000 -> 41000
  const time_t overCredit = start + (time_t)r.sessionSeconds - lastKnownOpen;
  assert(overCredit <= kDebounceSec);
}

int main() {
  testDebounceSourceIsShared();
  testRestartCreditsToBootTimeWhenEarlierThanTheCap();
  testRestartIsCappedAtAnchorPlusOneDebounce();
  testLastReportMovesTheAnchor();
  testLongPowerOffOverCreditsAtMostOneDebounce();
  testNormalCloseIsNotCapped();
  testUntrustedClockKeepsTheSessionOpen();
  testUntrustedClockCausesNoReportStorm();
  testExistingAnomalyHandlingIsUnchanged();
  testExplicitCloseAtStillWins();
  testReportAfterRestartBeforeTrustDoesNotMoveTheCap();

  std::cout << "Occupancy session restart test passed "
               "(mid-session restart credited and capped, untrusted clock keeps the session open)\n";
  return 0;
}
CPP
} > "$generated"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  "$generated" \
  -o "$binary"

"$binary"
