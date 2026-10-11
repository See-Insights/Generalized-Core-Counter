// WO-2026-10-09-001: Idle's CONNECTED branch logs TimeDiag only on Idle entry
// or when isOpen / openness / trusted / valid changes. The real head of
// handleIdleState() (through the CONNECTED branch) is #included from the
// extracted file; everything it touches is stubbed below.
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <cstring>

enum { LOOP_STAGE_IDLE_PROCESSING = 1 };
enum StateName { IDLE_STATE, SLEEPING_STATE, REPORTING_STATE };
StateName state = IDLE_STATE;
StateName oldState = IDLE_STATE;

static unsigned long g_millis = 0;
unsigned long millis() { return g_millis; }

struct TimeStub {
  bool valid = true;
  time_t now_ = 1700000000;
  bool isValid() const { return valid; }
  time_t now() const { return now_; }
} Time;

namespace Clock {
enum class Openness : uint8_t { Open, Closed, Unknown };
}
static Clock::Openness g_openness = Clock::Openness::Open;
static bool g_trusted = true;
static bool g_isOpen = true;
namespace Clock {
Openness openness() { return g_openness; }
bool isTimeValid() { return Time.isValid(); }
}
bool isClockTrusted() { return g_trusted; }
bool isWithinOpenHours() { return g_isOpen; }

static int g_timeDiagCalls = 0;
static int g_lastLoggedIsOpen = -1;
void logTimeDiag(bool isOpen) { g_timeDiagCalls++; g_lastLoggedIsOpen = isOpen; }

static const char *g_transition = nullptr;
void transitionTo(StateName next, const char *why) { state = next; g_transition = why; }
void publishStateTransition() { oldState = state; }
void setLoopStage(int) {}
void signalLEDUpdate() {}
void signalLED(bool) {}
bool signalLEDStatus() { return false; }
void ensureSensorEnabled(const char *) {}
struct Session { bool occupancyChangeTriggered = false; } session;

namespace SystemConfig {
enum SensorMode { COUNTING, OCCUPANCY, MEASUREMENT };
enum ConnectionMode { CONNECTED, INTERMITTENT_KEEP_ALIVE };
SensorMode get_sensorMode() { return COUNTING; }
}
namespace CurrentReadings {
bool get_occupied() { return false; }
uint32_t get_lastOccupancyEvent() { return 0; }
void set_lastOccupancyEvent(uint32_t) {}
}
struct PowerManagerStub {
  SystemConfig::ConnectionMode mode = SystemConfig::CONNECTED;
  SystemConfig::ConnectionMode effectiveConnectionMode() const { return mode; }
};
PowerManagerStub &powerStub() { static PowerManagerStub p; return p; }
struct PM { static PowerManagerStub &instance() { return powerStub(); } };
#define PowerManager PM
struct SensorMgr {
  static SensorMgr &instance() { static SensorMgr s; return s; }
  bool isSensorReady() { return true; }
};
#define SensorManager SensorMgr

struct OccupancyCloseResult { bool valid; bool stillOpen; uint32_t sessionSeconds; uint32_t totalSeconds; };
uint32_t occupancyDebounceMs() { return 0; }
bool reportsOccupancyChangesNow() { return false; }
OccupancyCloseResult closeOccupancySessionSafely(const char *) { return {false, false, 0, 0}; }
void logUnoccupiedEvent(const char *, uint32_t, uint32_t, bool) {}
struct TestLog { template <typename... A> void info(const char *, A...) {} template <typename... A> void warn(const char *, A...) {} } Log;

#include "extracted_idle_head.inc"

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); failures++; } } while (0)

static void reset() {
  state = IDLE_STATE; oldState = SLEEPING_STATE; // entry pass: state != oldState
  g_openness = Clock::Openness::Open; g_trusted = true; g_isOpen = true; Time.valid = true;
  g_transition = nullptr; g_timeDiagCalls = 0;
}

// Runs n passes, advancing ticking values each pass; returns TimeDiag count.
static int passes(int n) {
  const int before = g_timeDiagCalls;
  for (int i = 0; i < n; i++) {
    g_millis += 7;
    Time.now_ += 1;
    handleIdleState();
  }
  return g_timeDiagCalls - before;
}

// A static last-key persists across runs, so each scenario starts by leaving
// and re-entering Idle with a known key, then settling.
static void settle() { reset(); passes(1); }

int main() {
  settle();
  CHECK(g_timeDiagCalls == 1, "entry pass logs exactly one TimeDiag");
  CHECK(passes(1000) == 0, "1000 unchanged CONNECTED/Open passes add no TimeDiag (ticking time ignored)");

  g_openness = Clock::Openness::Unknown;
  CHECK(passes(1000) == 1, "openness Open->Unknown logs exactly one TimeDiag");
  CHECK(state == IDLE_STATE, "Unknown stays in Idle");
  g_trusted = false;
  CHECK(passes(1000) == 1, "trusted true->false logs exactly one TimeDiag");
  g_isOpen = false;
  CHECK(passes(1000) == 1, "isOpen true->false logs exactly one TimeDiag");
  CHECK(g_lastLoggedIsOpen == 0, "logTimeDiag receives the same isOpen value");
  Time.valid = false;
  CHECK(passes(1000) == 1, "valid true->false logs exactly one TimeDiag");

  // Re-entry: leave Idle and return; the entry pass logs again with an unchanged key.
  state = SLEEPING_STATE; oldState = SLEEPING_STATE;
  state = IDLE_STATE;
  CHECK(passes(1000) == 1, "re-entry logs exactly one TimeDiag on the entry pass");

  // Closed still goes to Sleep with "park closed".
  settle();
  g_openness = Clock::Openness::Closed;
  passes(1);
  CHECK(state == SLEEPING_STATE, "Closed transitions to SLEEPING_STATE");
  CHECK(g_transition && std::strcmp(g_transition, "park closed") == 0, "Closed reason is park closed");

  if (failures) { std::printf("%d failure(s)\n", failures); return 1; }
  std::printf("PASS\n");
  return 0;
}
