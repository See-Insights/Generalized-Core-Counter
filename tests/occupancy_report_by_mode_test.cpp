// WO-2026-10-07-004 - occupancy changes report by mode.
//
// Every occupancy change (start or end) is reported right away when the mode
// in use is INTERMITTENT_KEEP_ALIVE or CONNECTED, and in no other mode. A
// change is reported once: Report takes the flag on entry, so no path through
// Report, Connect or the next Idle/Sleep pass can report it a second time.
//
// Real code under test, extracted byte-for-byte by the .sh (which asserts that
// every extracted block equals the src/ text) or linked:
//   - reportsOccupancyChangesNow(), closeOccupancySessionSafely() and
//     occupancyDebounceMs()                 from src/state/State_Common.h
//   - handleOccupancyMode() and updateOccupancyState()  (main-loop handler)
//                                            from src/state/State_Modes.cpp
//   - the head of handleIdleState(), through the pending-change consumer
//                                            from src/state/State_Idle.cpp
//   - from handleSleepingState(): the pending-change check, the wake end
//     block, the wake start block and the timer-wake block
//                                            from src/state/State_Sleep.cpp
//   - from handleReportingState(): the entry take of the flag, the alert-40
//     block, the service-request take, the config-invalid exit, the first two
//     branches of the not-connected chain and the connected branch
//                                            from src/state/State_Report.cpp
//   - src/MyPersistentData.cpp and src/power/PowerManager.cpp, so the mode in
//     use (including the battery downgrade) is the real derivation.
//
// Host limits: handleIdleState(), handleSleepingState() and
// handleReportingState() cannot run whole on the host. The .sh sorts the
// extracted blocks by their position in the real function and emits them in
// that order (sleep_pass.inc, report_pass.inc), so moving a block in src/
// changes what the passes below do. Only the glue between the blocks is
// modelled here: the publish call, the teardown request, the sleep itself, and
// the tail of the not-connected chain. Each model is marked "model".

#include "Particle.h"
#include "StorageHelperRK.h"

#include "MyPersistentData.h"
#include "persist/CurrentReadings.h"
#include "persist/PowerConfig.h"
#include "persist/RecoveryState.h"
#include "persist/SystemConfig.h"
#include "power/ConnectivityPolicy.h"
#include "power/PowerManager.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace StorageHelperRK {
std::vector<FlushRecord> flushCalls;
std::vector<size_t> validateSizes;
int initializeCalls = 0;
}  // namespace StorageHelperRK

bool publishDiagnosticSafe(const char *, const char *, PublishFlags) { return true; }
bool isClockTrusted() { return true; }
namespace Clock {
enum class Openness { Open, Closed, Unknown };
bool trusted = true;
Openness opennessValue = Openness::Open;
bool isTimeValid() { return true; }
bool isTrusted() { return trusted; }
Openness openness() { return opennessValue; }
}  // namespace Clock

namespace Config {
bool configValid = true;
uint32_t occupancyDebounceMsForRuntime() { return 300000UL; }
bool isValid(bool) { return configValid; }
}  // namespace Config

namespace {
int failures = 0;
}
#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << "FAIL: " << (msg) << " (" #cond ")\n";                      \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

// ----- Application surface the extracted code collaborates with ---------------

struct SessionState {
  time_t occupancySessionBootAnchor = 0;
  bool occupancyChangeTriggered = false;
  bool serviceRequestTriggered = false;
  bool suppressAlert40ThisSession = false;
  bool awaitingWebhookResponse = false;
};
SessionState session;

enum State { IDLE_STATE, REPORTING_STATE, SLEEPING_STATE, CONNECTING_STATE, ERROR_STATE };
State state = IDLE_STATE;
State oldState = IDLE_STATE;
std::vector<std::string> reasons;
void transitionTo(State next, const char *reason) {
  state = next;
  reasons.push_back(reason);
}

bool ledOn = false;
uint32_t ledRemainingSec = 0;
void signalLED(bool on, uint32_t = 0) { ledOn = on; }
void signalLEDUpdate() {}
bool signalLEDStatus() { return ledOn; }
uint32_t signalLEDTimeRemaining() { return ledRemainingSec; }
void setLoopStage(int) {}
void publishStateTransition() {}
void setAppBreadcrumb(int) {}
const int LOOP_STAGE_IDLE_PROCESSING = 5;

bool sensorEvent = false;
struct SensorManager {
  static SensorManager &instance() {
    static SensorManager s;
    return s;
  }
  bool loop() { return sensorEvent; }
};

void logOccupiedEvent(const char *, uint32_t, bool, bool = false) {}
void logUnoccupiedEvent(const char *, uint32_t, uint32_t, bool) {}

#include "extracted_predicate.inc"
#include "extracted_close_1.inc"
#include "extracted_close_2.inc"
#include "extracted_close_3.inc"
#include "extracted_modes_update.inc"
#include "extracted_modes_handle.inc"

void idleHead() {
#include "extracted_idle_head.inc"
}

// ----- Sleep: the real order of handleSleepingState() --------------------------
enum WakeKind { WAKE_TIMER, WAKE_PIR, WAKE_OTHER };
WakeKind wakeKind = WAKE_TIMER;
bool reportDue = false;
bool reportDueThisInterval() { return reportDue; }
bool disconnectRequested = false;
bool teardownWanted = false;
bool sleepReached = false;
bool wakeFellThrough = false;

// model: the teardown request/wait. The real code requests the disconnect and
// returns; on a later pass it completes the teardown and resets the flag
// before it sleeps.
bool modelTeardown() {
  if (!disconnectRequested && teardownWanted) {
    disconnectRequested = true;
    return false;
  }
  disconnectRequested = false;
  return true;
}
// model: System.sleep() returning to the wake processing.
void modelSleep() { sleepReached = true; }

// model: the cloud-ops gate ("Only wait for cloud operations *before*
// requesting disconnect"). Like the real gate it starts its timer on the first
// pass, waits while under budget, and gives up (resetting the timer) once the
// budget has elapsed.
unsigned long cloudSyncStartMs = 0;
bool gateWanted = false;
bool gateTimedOut = false;
unsigned long gateElapsedMs = 0;
const unsigned long kGateBudgetMs = 30000;
bool modelGate() {
  if (!gateWanted || disconnectRequested) return true;
  if (cloudSyncStartMs == 0) cloudSyncStartMs = millis();
  gateElapsedMs = millis() - cloudSyncStartMs;
  if (gateElapsedMs < kGateBudgetMs) return false;
  gateTimedOut = true;
  cloudSyncStartMs = 0;
  return true;
}

void sleepPass() {
  sleepReached = false;
  wakeFellThrough = false;
  const bool pirWake = (wakeKind == WAKE_PIR);
  const bool timerWake = (wakeKind == WAKE_TIMER);
  (void)pirWake;
  (void)timerWake;
#include "sleep_pass.inc"
  wakeFellThrough = true;
}

// ----- Report: the real order of handleReportingState() ------------------------
int publishCount = 0;
bool flagAtPublish = false;
// model: publishData() queues the payload from the current readings.
void modelPublish() {
  ++publishCount;
  flagAtPublish = session.occupancyChangeTriggered;
}

void reportPass() {
  const time_t now = Time.now();
  bool forceConnectForLongTermWebhook = false;
  (void)forceConnectForLongTermWebhook;
#include "report_pass.inc"
}

// One main-loop pass: the state switch, then the centralized sensor handler.
void loopOnce() {
  oldState = state;
  switch (state) {
    case IDLE_STATE: idleHead(); break;
    case REPORTING_STATE: reportPass(); break;
    case SLEEPING_STATE: sleepPass(); break;
    default: break;
  }
  handleOccupancyMode();
}

namespace {

constexpr uint8_t kConnected = SystemConfig::CONNECTED;
constexpr uint8_t kIntermittent = SystemConfig::INTERMITTENT;
constexpr uint8_t kDisconnected = SystemConfig::DISCONNECTED;
constexpr uint8_t kKeepAlive = SystemConfig::INTERMITTENT_KEEP_ALIVE;

struct ModeCase {
  const char *name;
  uint8_t configured;
  bool lowBattery;
  bool reportsNow;
};

const ModeCase kModes[] = {
    {"INTERMITTENT", kIntermittent, false, false},
    {"KEEP_ALIVE", kKeepAlive, false, true},
    {"CONNECTED", kConnected, false, true},
    {"DISCONNECTED", kDisconnected, false, false},
    {"downgraded KEEP_ALIVE", kKeepAlive, true, false},
};

constexpr unsigned long kDebounceMs = 300000UL;

void arm(const ModeCase &m) {
  SystemConfig::set_sensorMode(SystemConfig::OCCUPANCY);
  SystemConfig::set_connectionMode(m.configured);
  PowerConfig::set_lowBatteryMode(m.lowBattery);
  SystemConfig::set_lastHookResponse(0);
  SystemConfig::set_lastConnection(0);
  RecoveryState::set_lastAlertTime(0);
  CurrentReadings::set_occupied(false);
  CurrentReadings::set_occupancyStartTime(0);
  CurrentReadings::set_totalOccupiedSeconds(0);
  CurrentReadings::set_lastOccupancyEvent(0);
  Clock::trusted = true;
  Clock::opennessValue = Clock::Openness::Open;
  Config::configValid = true;
  Time.nowValue = 1750000000;
  millisValue = 1000000;
  session = SessionState();
  state = IDLE_STATE;
  oldState = IDLE_STATE;
  reasons.clear();
  ledOn = false;
  ledRemainingSec = 0;
  sensorEvent = false;
  sleepReached = false;
  wakeFellThrough = false;
  wakeKind = WAKE_TIMER;
  reportDue = false;
  disconnectRequested = false;
  teardownWanted = false;
  cloudSyncStartMs = 0;
  gateWanted = false;
  gateTimedOut = false;
  gateElapsedMs = 0;
  publishCount = 0;
  flagAtPublish = false;
  Particle.connectedValue = false;
}

// An open session whose debounce has just run out.
void occupiedPastDebounce(State where) {
  CurrentReadings::set_occupied(true);
  CurrentReadings::set_occupancyStartTime(Time.nowValue - 600);
  CurrentReadings::set_lastOccupancyEvent(millisValue);
  millisValue += kDebounceMs + 1000;
  Time.nowValue += (kDebounceMs + 1000) / 1000;
  ledOn = true;
  ledRemainingSec = 0;
  state = where;
  oldState = where;
}

bool wentTo(const char *reason) {
  return std::find(reasons.begin(), reasons.end(), reason) != reasons.end();
}

bool anyOccupancyReport() {
  return wentTo("occupancy change") || wentTo("occupancy cleared") || wentTo("occupancy transition") ||
         wentTo("occupancy change pending") || wentTo("sleep-occupancy-debounce-report") ||
         wentTo("sleep-pir-occupancy-report");
}

std::string label(const ModeCase &m, const std::string &what) { return std::string(m.name) + ": " + what; }

#define CHECK_MODE(m, cond, what) CHECK(cond, label(m, what))

// --- The rule table: awake path -------------------------------------------------
void testAwakeStart() {
  for (const ModeCase &m : kModes) {
    arm(m);
    sensorEvent = true;
    loopOnce();
    CHECK_MODE(m, CurrentReadings::get_occupied(), "awake start: the session opens");
    if (m.reportsNow) {
      CHECK_MODE(m, state == REPORTING_STATE && wentTo("occupancy change"), "awake start: goes to REPORTING right away");
      CHECK_MODE(m, session.occupancyChangeTriggered, "awake start: flag set for the report");
    } else {
      CHECK_MODE(m, state == IDLE_STATE && !anyOccupancyReport(), "awake start: no immediate report");
      CHECK_MODE(m, !session.occupancyChangeTriggered, "awake start: no flag set");
    }
  }
}

void testAwakeEnd() {
  for (const ModeCase &m : kModes) {
    arm(m);
    occupiedPastDebounce(IDLE_STATE);
    loopOnce();
    CHECK_MODE(m, !CurrentReadings::get_occupied(), "awake end: the session closes");
    if (m.reportsNow) {
      CHECK_MODE(m, state == REPORTING_STATE && wentTo("occupancy transition"), "awake end: goes to REPORTING right away");
    } else {
      CHECK_MODE(m, state == IDLE_STATE && !anyOccupancyReport(), "awake end: no immediate report");
      CHECK_MODE(m, !session.occupancyChangeTriggered, "awake end: no flag set");
    }
  }
}

// A change the main-loop handler latches outside Idle goes out from the first Idle pass.
void testAwakeLatchedOutsideIdle() {
  for (const ModeCase &m : kModes) {
    arm(m);
    state = CONNECTING_STATE;
    sensorEvent = true;
    loopOnce();
    sensorEvent = false;
    CHECK_MODE(m, state == CONNECTING_STATE, "latched start: no transition outside Idle");
    CHECK_MODE(m, session.occupancyChangeTriggered == m.reportsNow, "latched start: flag follows the rule");
    state = IDLE_STATE;
    reasons.clear();
    loopOnce();
    if (m.reportsNow) {
      CHECK_MODE(m, state == REPORTING_STATE && wentTo("occupancy change pending"),
                 "latched start: the first Idle pass goes to REPORTING");
    } else {
      CHECK_MODE(m, state == IDLE_STATE && !anyOccupancyReport(), "latched start: Idle does not report");
    }

    arm(m);
    occupiedPastDebounce(CONNECTING_STATE);
    loopOnce();
    CHECK_MODE(m, state == CONNECTING_STATE, "latched end: no transition outside Idle");
    CHECK_MODE(m, session.occupancyChangeTriggered == m.reportsNow, "latched end: flag follows the rule");
    state = IDLE_STATE;
    reasons.clear();
    loopOnce();
    if (m.reportsNow) {
      CHECK_MODE(m, state == REPORTING_STATE && wentTo("occupancy change pending"),
                 "latched end: the first Idle pass goes to REPORTING");
    } else {
      CHECK_MODE(m, state == IDLE_STATE && !anyOccupancyReport(), "latched end: Idle does not report");
    }
  }
}

// --- The rule table: wake-from-sleep path ---------------------------------------
// WAKE_OTHER isolates the wake blocks from the timer-wake decision below them.
void testWakeStart() {
  for (const ModeCase &m : kModes) {
    arm(m);
    state = SLEEPING_STATE;
    wakeKind = WAKE_PIR;
    sleepPass();
    CHECK_MODE(m, CurrentReadings::get_occupied(), "wake start: the session opens");
    if (m.reportsNow) {
      CHECK_MODE(m, state == REPORTING_STATE && wentTo("sleep-pir-occupancy-report"),
                 "wake start: goes to REPORTING right away");
    } else {
      CHECK_MODE(m, state == SLEEPING_STATE && wakeFellThrough && !anyOccupancyReport(), "wake start: no immediate report");
      CHECK_MODE(m, !session.occupancyChangeTriggered, "wake start: no flag set");
    }
  }
}

void testWakeEnd() {
  for (const ModeCase &m : kModes) {
    arm(m);
    occupiedPastDebounce(SLEEPING_STATE);
    wakeKind = WAKE_OTHER;
    sleepPass();
    CHECK_MODE(m, !CurrentReadings::get_occupied(), "wake end: the session closes");
    if (m.reportsNow) {
      CHECK_MODE(m, state == REPORTING_STATE && wentTo("sleep-occupancy-debounce-report"),
                 "wake end: goes to REPORTING right away");
    } else {
      CHECK_MODE(m, state == SLEEPING_STATE && wakeFellThrough && !anyOccupancyReport(), "wake end: no immediate report");
      CHECK_MODE(m, !session.occupancyChangeTriggered, "wake end: no flag set");
    }
  }
}

// --- The soak: the end is latched by the main-loop handler while SLEEPING -------
// Pass N is a timer wake whose end gate is false (the LED was switched off).
// The tail then latches the end. Pass N+1 must report before it sleeps again;
// the order of the pending check against the suppress decision is the real one.
void testSoakLatchedWhileSleeping() {
  for (const ModeCase &m : kModes) {
    arm(m);
    occupiedPastDebounce(SLEEPING_STATE);
    ledOn = false;
    wakeKind = WAKE_TIMER;
    sleepPass();
    CHECK_MODE(m, CurrentReadings::get_occupied(), "soak: the wake end gate does not fire");
    const bool suppresses = m.reportsNow && m.configured == kKeepAlive;
    CHECK_MODE(m, wentTo(suppresses ? "sleep-timer-occupied-suppress-report" : "sleep-timer-report"),
               "soak: the first wake takes its scheduled decision");
    state = SLEEPING_STATE;
    handleOccupancyMode();  // the main-loop tail, state already SLEEPING
    CHECK_MODE(m, !CurrentReadings::get_occupied(), "soak: the tail closes the session");
    CHECK_MODE(m, state == SLEEPING_STATE, "soak: the tail cannot transition");
    CHECK_MODE(m, session.occupancyChangeTriggered == m.reportsNow, "soak: flag follows the rule");

    reasons.clear();
    loopOnce();  // the next sleep-prep pass
    if (m.reportsNow) {
      CHECK_MODE(m, state == REPORTING_STATE && wentTo("occupancy change pending"),
                 "soak: the next sleep prep goes to REPORTING for the pending change");
      CHECK_MODE(m, !sleepReached && !wentTo("sleep-timer-occupied-suppress-report"),
                 "soak: no sleep-or-suppress decision was reached");
    } else {
      CHECK_MODE(m, !wentTo("occupancy change pending") && sleepReached,
                 "soak: sleep prep proceeds to sleep without an occupancy report");
    }
  }
}

// --- Mid-gate: leaving for a pending change resets the gate timer ---------------
void testPendingMidGateResetsGateTimer() {
  for (const ModeCase &m : kModes) {
    if (!m.reportsNow) continue;
    arm(m);
    state = SLEEPING_STATE;
    oldState = SLEEPING_STATE;
    gateWanted = true;
    wakeKind = WAKE_OTHER;
    sleepPass();  // pass 1: the gate starts its timer and waits
    CHECK_MODE(m, cloudSyncStartMs != 0 && !sleepReached && state == SLEEPING_STATE,
               "midgate: the first pass starts the gate and waits");
    millisValue += 20000;
    session.occupancyChangeTriggered = true;  // the main-loop tail latches a change mid-gate
    sleepPass();  // pass 2: the pending check leaves for REPORTING
    CHECK_MODE(m, state == REPORTING_STATE && wentTo("occupancy change pending"),
               "midgate: the pending change goes to REPORTING");
    CHECK_MODE(m, cloudSyncStartMs == 0, "midgate: leaving mid-gate resets the gate timer");

    // Report and Connect run; a later sleep span starts its own gate.
    session.occupancyChangeTriggered = false;
    state = SLEEPING_STATE;
    oldState = SLEEPING_STATE;
    millisValue += 60000;
    sleepPass();
    CHECK_MODE(m, gateElapsedMs == 0 && !gateTimedOut && !sleepReached,
               "midgate: the next sleep's gate gets its full budget");
  }
}

// --- Teardown: a flag latched after the teardown request waits for the wake ------
void testTeardownLatchedAfterRequest() {
  for (const ModeCase &m : kModes) {
    if (!m.reportsNow) continue;
    arm(m);
    occupiedPastDebounce(SLEEPING_STATE);
    ledOn = false;
    teardownWanted = true;
    wakeKind = WAKE_OTHER;
    sleepPass();  // pass 1: requests the teardown and returns
    CHECK_MODE(m, disconnectRequested && state == SLEEPING_STATE && !sleepReached,
               "teardown: the first pass requests teardown and returns");
    handleOccupancyMode();  // the tail latches the end after the request
    CHECK_MODE(m, session.occupancyChangeTriggered, "teardown: the flag latches after the request");

    sleepPass();  // pass 2: the teardown completes and the device sleeps
    CHECK_MODE(m, !wentTo("occupancy change pending") && state == SLEEPING_STATE,
               "teardown: the flag does not go to REPORTING on this sleep span");
    CHECK_MODE(m, sleepReached && !disconnectRequested, "teardown: the span completes and sleeps");
    CHECK_MODE(m, session.occupancyChangeTriggered, "teardown: the flag stays latched across the sleep");

    sleepPass();  // pass 3: the first pass after the wake
    CHECK_MODE(m, state == REPORTING_STATE && wentTo("occupancy change pending"),
               "teardown: the first pass after the wake goes to REPORTING");
  }
}

// --- One report per change, on every Report path ----------------------------------
struct ExitCase {
  const char *name;
  const char *reason;
  void (*setup)();
};

void setupConfigInvalid() { Config::configValid = false; }
void setupServiceRequest() { session.serviceRequestTriggered = true; }
void setupAlert40() {
  RecoveryState::raiseAlert(40);
  RecoveryState::set_lastAlertTime(0);
  SystemConfig::set_lastHookResponse(Time.nowValue - 7 * 3600);
  SystemConfig::set_lastConnection(Time.nowValue - 3600);
}

const ExitCase kExits[] = {
    {"config invalid", "config invalid", setupConfigInvalid},
    {"service request", "service request", setupServiceRequest},
    {"alert-40 escalation", "webhook long-term failure", setupAlert40},
};

void testNoRepeat() {
  const ModeCase *modes[] = {&kModes[1], &kModes[2]};
  for (const ModeCase *m : modes) {
    for (const ExitCase &e : kExits) {
      for (int followUp = 0; followUp < 2; ++followUp) {
        const std::string tag = std::string("norepeat: ") + e.name + (followUp == 0 ? " then Idle" : " then Sleep");
        arm(*m);
        session.occupancyChangeTriggered = true;
        state = REPORTING_STATE;
        e.setup();
        loopOnce();
        CHECK_MODE(*m, wentTo(e.reason), tag + ": Report takes its exit");
        CHECK_MODE(*m, publishCount == 1 && flagAtPublish, tag + ": the one payload carries the change");
        CHECK_MODE(*m, !session.occupancyChangeTriggered, tag + ": the flag is taken at entry");

        Config::configValid = true;
        reasons.clear();
        publishCount = 0;
        state = followUp == 0 ? IDLE_STATE : SLEEPING_STATE;
        wakeKind = WAKE_OTHER;
        for (int i = 0; i < 5; ++i) loopOnce();
        CHECK_MODE(*m, !anyOccupancyReport() && publishCount == 0,
                   tag + ": the next passes do not report the same change again");
      }
    }
  }
}

// --- Ruling 1: a connected device reports once, then returns to Idle ------------
void testConnectedReportsOnce() {
  const ModeCase &connected = kModes[2];
  arm(connected);
  Particle.connectedValue = true;
  session.occupancyChangeTriggered = true;

  loopOnce();
  CHECK(state == REPORTING_STATE && wentTo("occupancy change pending"), "connected-once: pending flag goes to REPORTING");
  loopOnce();
  CHECK(publishCount == 1, "connected-once: exactly one report");
  CHECK(state == IDLE_STATE && wentTo("already connected"), "connected-once: returns to Idle (already connected)");
  CHECK(!session.occupancyChangeTriggered, "connected-once: the flag is cleared");

  reasons.clear();
  for (int i = 0; i < 5; ++i) loopOnce();
  CHECK(!anyOccupancyReport() && state == IDLE_STATE, "connected-once: no second REPORTING on the next 5 passes");
  CHECK(publishCount == 1, "connected-once: still exactly one report after more passes");
}

// A change in CONNECTED while connected, end to end through the Idle head.
void testConnectedEndToEnd() {
  const ModeCase &connected = kModes[2];
  arm(connected);
  Particle.connectedValue = true;
  occupiedPastDebounce(IDLE_STATE);
  for (int i = 0; i < 6; ++i) loopOnce();
  CHECK(publishCount == 1, "connected end to end: one report for one change");
  CHECK(state == IDLE_STATE && !session.occupancyChangeTriggered, "connected end to end: settled in Idle");
}

}  // namespace

int main() {
  // The sites as found in the real sources (set by the .sh).
  CHECK(SLEEP_CONSUMER_PRESENT == 1, "sleep prep has the pending-change check");
  CHECK(SLEEP_CONSUMER_BELOW_RESET == 1,
        "the sleep check sits below the disconnectRequested declaration and its entry reset");
  CHECK(SLEEP_CONSUMER_GATED == 1, "the sleep check is gated on !disconnectRequested");
  CHECK(SLEEP_CONSUMER_BEFORE_SLEEP_DECISIONS == 1,
        "the sleep check sits before System.sleep() and every sleep-or-suppress decision");
  CHECK(IDLE_CONSUMER_PRESENT == 1, "Idle has the pending-change consumer");
  CHECK(IDLE_CONSUMER_AFTER_OCCUPANCY_BLOCK == 1,
        "the Idle consumer follows the occupancy block and precedes the park-closed sleep");
  CHECK(REPORT_TAKE_AFTER_PUBLISH_BEFORE_EXITS == 1,
        "Report takes the flag after publishData() and before the first early exit");
  CHECK(PREDICATE_CALL_SITES == 5, "the five report-now sites all call reportsOccupancyChangesNow()");
  CHECK(OLD_REPORTNOW_EXPR_REMAINING == 0, "no site keeps its own KEEP_ALIVE-only reportNow expression");

  testAwakeStart();
  testAwakeEnd();
  testAwakeLatchedOutsideIdle();
  testWakeStart();
  testWakeEnd();
  testSoakLatchedWhileSleeping();
  testTeardownLatchedAfterRequest();
  testPendingMidGateResetsGateTimer();
  testNoRepeat();
  testConnectedReportsOnce();
  testConnectedEndToEnd();

  if (failures != 0) {
    std::cerr << failures << " check(s) failed\n";
    return 1;
  }
  std::cout << "occupancy_report_by_mode_test: all checks passed\n";
  return 0;
}
