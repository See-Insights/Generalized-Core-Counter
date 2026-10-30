// WO-2026-10-22-001 (Sleep S-6): equivalence of the wake-event extraction.
// (a) is the base handler's post-wake region (from the LED comment after the wake unpack, through the final
// transitionTo), extracted by marker text from the base State_Sleep.cpp (base::); (b) is the current
// postWakeRouting() from State_Sleep.cpp and processSleepWakeEvent() from State_Modes.cpp, extracted by marker
// text from the current files (cur::). Both run one single-pass scenario table against the shared fakes
// (tests/stubs/sleep_teardown) and the fakes below. Mode "a" writes the frozen fixture, mode "b" must equal it.
// Frozen per pass: every transition with its reason, breadcrumb, BatteryAuthority call, count/occupancy write,
// LED call, log line and sensor/measure call, in order, plus the persisted fields afterwards.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include "SleepExecutionFakes.h"

#define BUILD_NAME "release"

struct WakeScenario {
  std::string name;
  bool pir = false, btn = false, timer = false;
  uint8_t sensorMode = SystemConfig::COUNTING;
  uint8_t connMode = SystemConfig::INTERMITTENT;
  bool open = true, connected = true, lowBattery = false, clockTrusted = true, serialUp = false;
  int openness = 0;  // 0 Open, 1 Closed, 2 Unknown
  bool occupied = false, ledOn = false;
  uint32_t ledRemaining = 0, setting1 = 0;
  long lastReport = 7300;
  uint16_t hourly = 4, daily = 9;
  bool closeValid = true, closeStillOpen = false;
  bool sensorReady = true;
  int vcellState = 0, trust = 0;
};
static WakeScenario cur_in;
static long persistedLastCount = 0, persistedOccStart = 0;
static uint16_t persistedHourly = 0, persistedDaily = 0;
static bool persistedOccupied = false;
static uint32_t persistedLastOccEvent = 0;
static bool ledOnNow = false;
static uint32_t ledRemainingNow = 0;

// ----- Application surface the two functions collaborate with ------------------------------------------------
struct TimeStub {
  time_t now() { return (time_t)10000; }
};
inline TimeStub Time;
inline bool isClockTrusted() { return cur_in.clockTrusted; }
inline bool isWithinOpenHours() { return cur_in.open; }

namespace Clock {
enum class Openness { Open, Closed, Unknown };
inline Openness openness() { return (Openness)cur_in.openness; }
}  // namespace Clock
namespace Config {
inline uint16_t reportingIntervalSecForRuntime() { return 3600; }
inline uint32_t occupancyDebounceMsForRuntime() { return 300000UL; }
}  // namespace Config

// The real predicates of src/state/State_Common.h, over the same fakes.
inline bool reportDueThisInterval() {
  const time_t interval = Config::reportingIntervalSecForRuntime();
  const time_t lastReport = SystemConfig::get_lastReport();
  return lastReport == 0 || Time.now() / interval != lastReport / interval;
}
// WO-2026-10-29-002: the after-close gate is a deliberate change from the S-6 base. This table freezes the base
// behaviour of the move, so the gate is held off here; tests/occupancy_ends_at_close_test.sh drives it for real.
inline bool occupancyIgnoredWhenClosed() { return false; }
inline bool reportsOccupancyChangesNow() {
  const uint8_t mode = PowerManager::instance().effectiveConnectionMode();
  return mode == SystemConfig::INTERMITTENT_KEEP_ALIVE || mode == SystemConfig::CONNECTED;
}

struct OccupancyCloseResult {
  bool valid = false;
  uint32_t sessionSeconds = 0;
  uint32_t totalSeconds = 0;
  bool stillOpen = false;
};
inline OccupancyCloseResult closeOccupancySessionSafely(const char *path, time_t closeAt = Time.now()) {
  OccupancyCloseResult r;
  r.valid = cur_in.closeValid;
  r.stillOpen = cur_in.closeStillOpen;
  r.sessionSeconds = 120;
  r.totalSeconds = 4000;
  sim::emit("CLOSE %s at=%ld valid=%d stillOpen=%d", path, (long)closeAt, (int)r.valid, (int)r.stillOpen);
  if (r.valid && !r.stillOpen) persistedOccupied = false;
  return r;
}
inline void logOccupiedEvent(const char *reason, uint32_t ledSeconds, bool report, bool reset = false) {
  sim::emit("OCCEVT occupied reason=%s led=%lus report=%d reset=%d", reason, (unsigned long)ledSeconds, (int)report, (int)reset);
}
inline void logUnoccupiedEvent(const char *reason, uint32_t sessionSeconds, uint32_t totalSeconds, bool report) {
  sim::emit("OCCEVT unoccupied reason=%s session=%lus total=%lus report=%d", reason, (unsigned long)sessionSeconds,
            (unsigned long)totalSeconds, (int)report);
}

inline void signalLED(bool on, uint32_t ms = 0) {
  sim::emit("LED on=%d ms=%lu", (int)on, (unsigned long)ms);
  ledOnNow = on;
  ledRemainingNow = (on && ms) ? ms : 0;
}
inline bool signalLEDStatus() { return ledOnNow; }
inline uint32_t signalLEDTimeRemaining() { return ledRemainingNow; }

inline void logWakeSummary(const char *reason) {
  sim::emit("WAKESUMMARY %s open=%d ready=%d occ=%d led=%lus", reason, (int)isWithinOpenHours(),
            (int)SensorManager::instance().isSensorReady(), (int)persistedOccupied, (unsigned long)(signalLEDTimeRemaining() / 1000UL));
}

namespace BatteryAuthority {
struct Verdict {
  uint8_t tier;
  BatteryHealth::SocTrust trust;
  SensorManager::VcellSampleState vcellState;
};
inline Verdict evaluate(float soc, SensorManager::VcellSampleState vs, float vcell, BatteryHealth::SocTrust trust, uint8_t previousTier) {
  sim::emit("BATAUTH evaluate soc=%.1f vcellState=%d vcell=%.2f trust=%d prev=%u", (double)soc, (int)vs, (double)vcell, (int)trust,
            (unsigned)previousTier);
  return {previousTier, trust, vs};
}
inline uint8_t currentTier() { return 2; }
inline void commit(const Verdict &v, float soc) {
  sim::emit("BATAUTH commit tier=%u trust=%d state=%d soc=%.1f", (unsigned)v.tier, (int)v.trust, (int)v.vcellState, (double)soc);
}
}  // namespace BatteryAuthority

namespace CurrentReadings {
uint16_t get_hourlyCount() { return persistedHourly; }
void set_hourlyCount(uint16_t v) { sim::emit("SET hourlyCount=%u", (unsigned)v); persistedHourly = v; }
uint16_t get_dailyCount() { return persistedDaily; }
void set_dailyCount(uint16_t v) { sim::emit("SET dailyCount=%u", (unsigned)v); persistedDaily = v; }
time_t get_lastCountTime() { return (time_t)persistedLastCount; }
void set_lastCountTime(time_t v) { sim::emit("SET lastCountTime=%ld", (long)v); persistedLastCount = (long)v; }
bool get_occupied() { return persistedOccupied; }
void set_occupied(bool v) { sim::emit("SET occupied=%d", (int)v); persistedOccupied = v; }
uint32_t get_lastOccupancyEvent() { return persistedLastOccEvent; }
void set_lastOccupancyEvent(uint32_t v) { sim::emit("SET lastOccupancyEvent=%lu", (unsigned long)v); persistedLastOccEvent = v; }
time_t get_occupancyStartTime() { return (time_t)persistedOccStart; }
void set_occupancyStartTime(time_t v) { sim::emit("SET occupancyStartTime=%ld", (long)v); persistedOccStart = (long)v; }
}  // namespace CurrentReadings
namespace SystemConfig {
uint8_t get_sensorMode() { return cur_in.sensorMode; }
time_t get_lastReport() { return (time_t)cur_in.lastReport; }
namespace SensorSettings {
uint32_t get_sensorSetting1() { return cur_in.setting1; }
}  // namespace SensorSettings
}  // namespace SystemConfig
namespace PowerConfig {
bool get_lowBatteryMode() { return cur_in.lowBattery; }
}  // namespace PowerConfig

namespace base {
static void pass(bool pirWake, bool buttonWake, bool timerWake, const char *wakeReasonLabel) {
#include BASE_REGION_INC
}
}  // namespace base

namespace cur {
#include CUR_EVENT_INC
#include CUR_ROUTE_INC
}  // namespace cur

static std::vector<WakeScenario> table;
static std::set<std::string> distinctEvents;
static std::map<std::string, int> reasons;
static int scenarios = 0, nTrans = 0, nNoTrans = 0;

static std::string normalize(const std::string &line) {
  std::string out;
  for (char c : line) {
    if ((c >= '0' && c <= '9') || c == '.' || c == '-') {
      if (out.empty() || out.back() != '#') out += '#';
    } else {
      out += c;
    }
  }
  return out;
}

static WakeScenario &add(const std::string &name) {
  table.push_back(WakeScenario());
  table.back().name = name;
  return table.back();
}

static void runScenario(const WakeScenario &s, bool useBase, int index) {
  cur_in = s;
  sim::events.clear();
  sim::now = 61000;
  sim::transitions = 0;
  sim::connectionMode = s.connMode;
  sim::soc = 41.5f;
  sim::world.cloud = s.connected;
  sim::exec.serialPre = s.serialUp;
  sim::wakeIn.sensorReady = s.sensorReady;
  sim::wakeIn.vcellState = s.vcellState;
  sim::wakeIn.trust = s.trust;
  session = SessionState();
  persistedHourly = s.hourly;
  persistedDaily = s.daily;
  persistedLastCount = 0;
  persistedOccupied = s.occupied;
  persistedOccStart = s.occupied ? 8000 : 0;
  persistedLastOccEvent = 0;
  ledOnNow = s.ledOn;
  ledRemainingNow = s.ledRemaining;
  const bool pir = s.pir, btn = s.btn, timer = s.timer;
  const char *label = btn ? "BUTTON" : (pir ? "PIR" : "TIMER");
  printf("S%d %s\n", index, s.name.c_str());
  printf("  in wake=%s mode=%d conn=%d open=%d connected=%d low=%d openness=%d clock=%d serial=%d occ=%d led=%d/%lu set1=%lu last=%ld "
         "hourly=%u daily=%u close=%d/%d ready=%d vcell=%d trust=%d\n",
         label, (int)s.sensorMode, (int)s.connMode, (int)s.open, (int)s.connected, (int)s.lowBattery, s.openness, (int)s.clockTrusted,
         (int)s.serialUp, (int)s.occupied, (int)s.ledOn, (unsigned long)s.ledRemaining, (unsigned long)s.setting1, s.lastReport,
         (unsigned)s.hourly, (unsigned)s.daily, (int)s.closeValid, (int)s.closeStillOpen, (int)s.sensorReady, s.vcellState, s.trust);
  if (useBase) base::pass(pir, btn, timer, label); else cur::postWakeRouting(pir, btn, timer, label);
  for (const auto &e : sim::events) {
    printf("    %s\n", e.c_str());
    distinctEvents.insert(normalize(e));
    if (e.rfind("TRANSITION ", 0) == 0) reasons[e.substr(e.find(' ', 11) + 1)]++;
  }
  printf("  OUT trans=%d serviceRequest=%d occChange=%d hourly=%u daily=%u lastCount=%ld occ=%d start=%ld lastEvt=%lu led=%d/%lu\n",
         sim::transitions, (int)session.serviceRequestTriggered, (int)session.occupancyChangeTriggered, (unsigned)persistedHourly,
         (unsigned)persistedDaily, persistedLastCount, (int)persistedOccupied, persistedOccStart, (unsigned long)persistedLastOccEvent,
         (int)ledOnNow, (unsigned long)ledRemainingNow);
  if (sim::transitions) nTrans++; else nNoTrans++;
  scenarios++;
}

int main(int argc, char **argv) {
  const bool useBase = argc > 1 && strcmp(argv[1], "a") == 0;
  printf("BUILD %s\n", BUILD_NAME);
  namespace SC = SystemConfig;
  const uint8_t kModes[] = {SC::CONNECTED, SC::INTERMITTENT, SC::DISCONNECTED, SC::INTERMITTENT_KEEP_ALIVE};
  const char *kModeName[] = {"CONNECTED", "INTERMITTENT", "DISCONNECTED", "KEEP_ALIVE"};
  const char *kOpenName[] = {"open", "closed", "unknown"};
  // Last-report values at now=10000, interval 3600: 7300 same interval, 6401 earlier interval but under an
  // interval old, 6400 exactly an interval old, 0 never reported.
  const long kLast[] = {7300, 6401, 6400, 0};
  const char *kLastName[] = {"last=same-interval", "last=due-not-overdue", "last=exactly-overdue", "last=never"};
  auto mk = [&](const std::string &name, bool pir, bool btn, bool timer) -> WakeScenario & {
    WakeScenario &s = add(name);
    s.pir = pir; s.btn = btn; s.timer = timer;
    return s;
  };

  // The button wake ignores everything after its own block.
  for (uint8_t sm : {SC::COUNTING, SC::OCCUPANCY}) for (bool open : {true, false}) {
    WakeScenario &s = mk(std::string("button ") + (sm ? "occupancy" : "counting") + (open ? " open" : " closed"), false, true, false);
    s.sensorMode = sm; s.open = open; s.occupied = sm == SC::OCCUPANCY; s.connMode = SC::CONNECTED; s.connected = false;
  }

  // The timer wake: open hours x mode x connected (CONNECTED reconnects when not connected, in open hours only).
  for (bool open : {true, false}) for (int m = 0; m < 4; m++) for (bool conn : {true, false}) {
    WakeScenario &s = mk(std::string("timer ") + (open ? "open " : "closed ") + kModeName[m] + (conn ? " connected" : " disconnected"), false, false, true);
    s.open = open; s.connMode = kModes[m]; s.connected = conn;
  }
  // Timer wake in OCCUPANCY, the suppress condition: occupied x mode x report due.
  for (bool occ : {false, true}) for (int m = 0; m < 4; m++) for (long last : {7300L, 6401L}) {
    WakeScenario &s = mk(std::string("timer occupancy ") + (occ ? "occupied " : "empty ") + kModeName[m] + (last == 7300 ? " report-not-due" : " report-due"), false, false, true);
    s.sensorMode = SC::OCCUPANCY; s.occupied = occ; s.connMode = kModes[m]; s.lastReport = last;
  }
  // Timer wake in COUNTING: the LED flash that finished during sleep.
  for (bool open : {true, false}) {
    { WakeScenario &s = mk(std::string("timer counting led-expired") + (open ? " open" : " closed"), false, false, true); s.open = open; s.ledOn = true; s.ledRemaining = 0; }
    { WakeScenario &s = mk(std::string("timer counting led-running") + (open ? " open" : " closed"), false, false, true); s.open = open; s.ledOn = true; s.ledRemaining = 400; }
    { WakeScenario &s = mk(std::string("timer counting led-off") + (open ? " open" : " closed"), false, false, true); s.open = open; }
  }

  // The PIR wake in COUNTING: the synthesized count, then opportunistic reporting and the return to sleep.
  for (int o = 0; o < 3; o++) for (int m = 0; m < 4; m++) for (int l = 0; l < 4; l++) {
    if (o != 0 && l >= 2) continue;
    if (o == 0 || l < 2) {
      WakeScenario &s = mk(std::string("pir counting ") + kOpenName[o] + " " + kModeName[m] + " " + kLastName[l], true, false, false);
      s.openness = o; s.connMode = kModes[m]; s.lastReport = kLast[l];
    }
  }
  { WakeScenario &s = mk("pir counting untrusted clock", true, false, false); s.clockTrusted = false; }
  { WakeScenario &s = mk("pir counting trusted clock, serial up", true, false, false); s.serialUp = true; }
  { WakeScenario &s = mk("pir counting untrusted clock, serial up", true, false, false); s.clockTrusted = false; s.serialUp = true; }
  { WakeScenario &s = mk("pir counting closed hours", true, false, false); s.open = false; s.openness = 1; s.ledOn = true; s.ledRemaining = 0; }
  { WakeScenario &s = mk("pir counting count wraps", true, false, false); s.hourly = 65535; s.daily = 65535; }

  // The PIR wake in OCCUPANCY, the LED timeout first: occupied x LED state x mode (open).
  for (bool occ : {false, true}) for (int led = 0; led < 3; led++) for (int m = 0; m < 4; m++) {
    const char *ledName[] = {"led-expired", "led-running", "led-off"};
    WakeScenario &s = mk(std::string("pir occupancy ") + (occ ? "occupied " : "empty ") + ledName[led] + " " + kModeName[m], true, false, false);
    s.sensorMode = SC::OCCUPANCY; s.occupied = occ; s.connMode = kModes[m];
    s.ledOn = led != 2; s.ledRemaining = led == 1 ? 90000 : 0;
    s.setting1 = led == 1 ? 45000 : 0;
  }
  // The PIR wake in OCCUPANCY, closed hours, opportunistic reporting for non-KEEP_ALIVE modes.
  for (bool occ : {false, true}) for (int m = 0; m < 4; m++) {
    WakeScenario &s = mk(std::string("pir occupancy closed ") + (occ ? "occupied " : "empty ") + kModeName[m], true, false, false);
    s.sensorMode = SC::OCCUPANCY; s.occupied = occ; s.connMode = kModes[m]; s.openness = 1; s.open = false;
    s.ledOn = true; s.ledRemaining = 60000;
  }
  // The debounce window length: setting1 zero, set.
  for (int m : {1, 3}) for (uint32_t set1 : {0u, 20000u}) {
    WakeScenario &s = mk(std::string("pir occupancy empty ") + kModeName[m] + " setting1=" + std::to_string(set1), true, false, false);
    s.sensorMode = SC::OCCUPANCY; s.connMode = kModes[m]; s.setting1 = set1;
  }
  // Occupied PIR wake under KEEP_ALIVE: report due or not, by openness.
  for (int o = 0; o < 3; o++) for (long last : {7300L, 6401L}) {
    WakeScenario &s = mk(std::string("pir occupancy occupied KEEP_ALIVE ") + kOpenName[o] + (last == 7300 ? " report-not-due" : " report-due"), true, false, false);
    s.sensorMode = SC::OCCUPANCY; s.occupied = true; s.connMode = SC::INTERMITTENT_KEEP_ALIVE; s.openness = o; s.lastReport = last;
    s.ledOn = true; s.ledRemaining = 30000;
  }
  // The LED-timeout close that is not valid, or leaves the session open.
  for (bool valid : {false, true}) for (int m : {1, 3}) {
    WakeScenario &s = mk(std::string("pir occupancy led-expired ") + kModeName[m] + (valid ? " session still open" : " close not valid"), true, false, false);
    s.sensorMode = SC::OCCUPANCY; s.occupied = true; s.connMode = kModes[m]; s.ledOn = true; s.ledRemaining = 0;
    s.closeValid = valid; s.closeStillOpen = valid;
  }
  // Low-battery refresh: on the wake's open-hours block and on the OCCUPANCY PIR event.
  for (int wk = 0; wk < 3; wk++) for (int m : {0, 3}) for (int v = 0; v < 2; v++) {
    const char *wn[] = {"timer", "pir counting", "pir occupancy"};
    WakeScenario &s = mk(std::string("low battery ") + wn[wk] + " " + kModeName[m] + (v ? " vcell invalid, trust suspect" : " vcell known, trust trusted"), wk != 0, false, wk == 0);
    s.lowBattery = true; s.connMode = kModes[m]; s.vcellState = v ? 1 : 0; s.trust = v ? 1 : 0;
    s.sensorMode = wk == 2 ? SC::OCCUPANCY : SC::COUNTING;
  }
  { WakeScenario &s = mk("low battery pir occupancy occupied", true, false, false); s.lowBattery = true; s.sensorMode = SC::OCCUPANCY; s.occupied = true; s.connMode = SC::INTERMITTENT_KEEP_ALIVE; s.trust = 2; s.vcellState = 2; }
  { WakeScenario &s = mk("low battery pir occupancy closed hours", true, false, false); s.lowBattery = true; s.sensorMode = SC::OCCUPANCY; s.open = false; s.openness = 1; }
  { WakeScenario &s = mk("low battery off, open hours closed pir", true, false, false); s.open = false; }
  // The sensor stack was never initialized.
  for (bool pir : {false, true}) {
    WakeScenario &s = mk(std::string(pir ? "pir" : "timer") + " sensor not ready", pir, false, !pir);
    s.sensorReady = false;
  }
  { WakeScenario &s = mk("button sensor not ready", false, true, false); s.sensorReady = false; }

  int index = 0;
  for (const WakeScenario &s : table) runScenario(s, useBase, index++);
  printf("SCENARIOS %d PASSES %d TRANSITIONS %d NONE %d DISTINCT_EVENTS %d\n", scenarios, scenarios, nTrans, nNoTrans,
         (int)distinctEvents.size());
  for (const auto &e : distinctEvents) printf("EVENT %s\n", e.c_str());
  for (const auto &r : reasons) printf("REASON %s %d\n", r.first.c_str(), r.second);
  return 0;
}
