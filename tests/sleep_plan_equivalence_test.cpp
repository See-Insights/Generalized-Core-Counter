// WO-2026-10-21-003 (Sleep S-4): equivalence of the sleep-plan and nightly-heap-guard extraction.
// (a) is the base handler's plan block, hibernateAttempt call and heap-guard block, extracted by marker
// text from the base State_Sleep.cpp (base::); (b) is the current handler's plan call, unpack lines,
// hibernateAttempt call and guard call, with sleepPlan() and nightlyHeapGuard() extracted by marker text
// from the current file (cur::). Both run one scenario table, one pass each, against the shared fakes
// (tests/stubs/sleep_teardown/SleepExecutionFakes.h). Mode "a" writes the frozen fixture, mode "b" must
// equal it. Built with -DWiring_Cellular=1 -DHAL_PLATFORM_CELLULAR=1; the release build runs the full table,
// a build with -DCONNECTIVITY_FAILSAFE_TEST_MODE=1 runs only the small test-mode matrix.
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <functional>
#include <set>
#include <string>
#include "SleepExecutionFakes.h"
#include "Config.h"

#if CONNECTIVITY_FAILSAFE_TEST_MODE
#define BUILD_NAME "testmode"
#else
#define BUILD_NAME "release"
#endif

enum FailsafeDeferReason { FAILSAFE_DEFER_CLOSED_HOURS_LONG_SLEEP };

struct In {
  int open = 0;  // 0 Open, 1 Closed, 2 Unknown
  bool trusted = true, due = false, ledOn = false, occupied = false, claim = true;
  long now = 1700000000L;
  int untilOpen = 28800, mode = 2;  // mode: 0 COUNTING, 1 OCCUPANCY, 2 other
  uint16_t interval = 3600;
  uint32_t ledRem = 0, set1 = 0;
  unsigned long heap = 1000000UL;
};
static In in;
static int stage = 0;  // 1 = left in the plan, 2 = left in the guard, 3 = ran to the end
static int hibCalls = 0, hibWake = 0, hibOvernight = 0;

struct TimeStub { time_t now() { sim::emit("CALL Time.now"); return (time_t)in.now; } };
inline TimeStub Time;
namespace Clock {
enum class Openness { Open, Closed, Unknown };
inline Openness openness() {
  sim::emit("CALL Clock::openness");
  return in.open == 0 ? Openness::Open : in.open == 1 ? Openness::Closed : Openness::Unknown;
}
inline bool isTrusted() { sim::emit("CALL Clock::isTrusted"); return in.trusted; }
}  // namespace Clock
namespace DailyBoundary {
struct Result { bool due; time_t boundary; uint8_t close; };
inline Result check(time_t t) { sim::emit("CALL DailyBoundary::check %ld", (long)t); return {in.due, 0, 0}; }
}  // namespace DailyBoundary
namespace Config { uint16_t reportingIntervalSecForRuntime() { sim::emit("CALL reportingIntervalSecForRuntime"); return in.interval; } }
namespace SystemConfig {
uint8_t get_sensorMode() { return (uint8_t)in.mode; }
namespace SensorSettings { uint32_t get_sensorSetting1() { sim::emit("CALL get_sensorSetting1"); return in.set1; } }
}  // namespace SystemConfig
bool CurrentReadings::get_occupied() { return in.occupied; }
inline bool isWithinOpenHours() { sim::emit("CALL isWithinOpenHours"); return in.open != 1; }
inline void logTimeDiag(bool isOpen) { sim::emit("CALL logTimeDiag isOpen=%d", (int)isOpen); }
inline int secondsUntilNextOpen() { sim::emit("CALL secondsUntilNextOpen"); return in.untilOpen; }
inline void signalLED(bool on) { sim::emit("CALL signalLED %d", (int)on); }
inline bool signalLEDStatus() { sim::emit("CALL signalLEDStatus"); return in.ledOn; }
inline uint32_t signalLEDTimeRemaining() { sim::emit("CALL signalLEDTimeRemaining"); return in.ledRem; }
inline bool claimFailsafeDeferLog(FailsafeDeferReason) { sim::emit("CALL claimFailsafeDeferLog"); return in.claim; }
inline const char *failsafeDeferReasonName(FailsafeDeferReason) { return "closed-hours-long-sleep"; }
inline void hibernateAttempt(bool overnight, int wake) {
  sim::emit("CALL hibernateAttempt overnight=%d wake=%d", (int)overnight, wake);
  hibCalls++; hibWake = wake; hibOvernight = overnight; stage = 2;
}

static const char *finalReason = "";
static int finalWake = 0;
static bool finalOvernight = false;

namespace base {
static void pass() {
#include BASE_REGION_INC
  finalReason = sleepReason; finalWake = wakeInSeconds; finalOvernight = overnightFallbackSleep;
  stage = 3;
}
}  // namespace base

namespace cur {
namespace {
#include CUR_FUNCS_INC
}  // namespace
static void pass() {
#include CUR_REGION_INC
  finalReason = sleepReason; finalWake = wakeInSeconds; finalOvernight = overnightFallbackSleep;
  stage = 3;
}
}  // namespace cur

struct Sc { std::string name; In in; int expectWake = -1; };
static std::vector<Sc> table;
static std::set<std::string> distinctEvents;
static int scenarios = 0, nExitPlan = 0, nExitGuard = 0, nContinue = 0, nExpectMismatch = 0;

static std::string normalize(const std::string &line) {
  std::string out;
  for (char c : line) {
    if (c >= '0' && c <= '9') {
      if (out.empty() || out.back() != '#') out += '#';
    } else {
      out += c;
    }
  }
  return out;
}

static void add(const std::string &name, std::function<void(In &)> f = {}, int expectWake = -1) {
  Sc s{name, In(), expectWake};
  if (f) f(s.in);
  table.push_back(s);
}

static void runScenario(const Sc &s, bool useBase, int index) {
  printf("S%d %s\n", index, s.name.c_str());
  sim::events.clear();
  sim::transitions = 0;
  in = s.in;
  stage = 0; hibCalls = 0; hibWake = -1; hibOvernight = -1;
  finalReason = ""; finalWake = -1; finalOvernight = false;
  sim::exec.freeHeap = in.heap;
  printf("  in open=%d trusted=%d due=%d now=%ld until=%d interval=%u mode=%d led=%u/%d occ=%d set1=%u heap=%lu claim=%d\n",
         in.open, (int)in.trusted, (int)in.due, in.now, in.untilOpen, (unsigned)in.interval, in.mode, (unsigned)in.ledRem,
         (int)in.ledOn, (int)in.occupied, (unsigned)in.set1, in.heap, (int)in.claim);
  if (useBase) base::pass(); else cur::pass();
  for (const auto &e : sim::events) {
    printf("    %s\n", e.c_str());
    distinctEvents.insert(normalize(e));
  }
  const char *where = stage == 0 ? "PLAN_EXIT" : stage == 2 ? "GUARD_EXIT" : "CONTINUE";
  printf("  OUT %s transitions=%d hibernateCalls=%d hibernate=%d/%d", where, sim::transitions, hibCalls, hibWake, hibOvernight);
  if (stage == 3) printf(" plan wake=%d reason=%s overnight=%d", finalWake, finalReason, (int)finalOvernight);
  printf("\n");
  if (s.expectWake >= 0 && (stage != 3 || finalWake != s.expectWake || strcmp(finalReason, "debounce") != 0)) {
    printf("  EXPECT_MISMATCH want wake=%d reason=debounce\n", s.expectWake);
    nExpectMismatch++;
  }
  if (stage == 0) nExitPlan++; else if (stage == 2) nExitGuard++; else nContinue++;
  scenarios++;
}

int main(int argc, char **argv) {
  const bool useBase = argc > 1 && strcmp(argv[1], "a") == 0;
  printf("BUILD %s\n", BUILD_NAME);

#if CONNECTIVITY_FAILSAFE_TEST_MODE
  // The failsafe test-mode cap on the closed sleep (60 s): below, at and above it, and the defer log claim.
  for (int until : {59, 60, 61, 3600}) {
    add("testmode closed until=" + std::to_string(until), [=](In &i) { i.open = 1; i.untilOpen = until; });
  }
  add("testmode closed over the clamp", [](In &i) { i.open = 1; i.untilOpen = 40000; });
  add("testmode closed claim refused", [](In &i) { i.open = 1; i.untilOpen = 3600; i.claim = false; });
  add("testmode open is not capped", [](In &i) { i.open = 0; i.untilOpen = 3600; });
#else
  const unsigned long RESET = ConnectivityPolicy::NIGHTLY_HEAP_RESET_THRESHOLD_BYTES;
  const unsigned long WARN = ConnectivityPolicy::NIGHTLY_HEAP_WARN_THRESHOLD_BYTES;
  const long T = 1700000000L - (1700000000L % 3600);  // an hour boundary
  // Openness x close due x trusted clock.
  for (int open : {0, 1, 2}) for (int due : {0, 1}) for (int tr : {1, 0}) {
    add("openness=" + std::to_string(open) + " due=" + std::to_string(due) + " trusted=" + std::to_string(tr),
        [=](In &i) { i.open = open; i.due = due; i.trusted = tr; });
  }
  // Night duration: <= 0, normal, around the 546-minute clamp.
  for (int until : {-5, 0, 1, 28800, 32759, 32760, 32761, 32762, 40000, 86400}) {
    add("night until=" + std::to_string(until), [=](In &i) { i.open = 1; i.untilOpen = until; });
  }
  // Interval 0 or non-zero, and the alignment offsets.
  for (int interval : {0, 60, 3600, 65535}) for (long off : {0L, 1L, 3599L}) {
    add("interval=" + std::to_string(interval) + " offset=" + std::to_string(off),
        [=](In &i) { i.interval = (uint16_t)interval; i.now = T + off; });
  }
  add("interval=0 untrusted", [](In &i) { i.interval = 0; i.trusted = false; });
  add("interval=0 closed", [](In &i) { i.interval = 0; i.open = 1; });
  add("interval=1800 unknown clock", [](In &i) { i.interval = 1800; i.open = 2; i.trusted = false; });
  add("interval=1800 unknown but trusted", [](In &i) { i.interval = 1800; i.open = 2; i.now = T + 5; });
  // COUNTING: the LED remaining or not, on or off, open or closed.
  for (int open : {0, 1, 2}) for (uint32_t rem : {0u, 3u}) for (int on : {0, 1}) {
    add("counting open=" + std::to_string(open) + " ledRem=" + std::to_string(rem) + " ledOn=" + std::to_string(on),
        [=](In &i) { i.mode = 0; i.open = open; i.ledRem = rem; i.ledOn = on; });
  }
  add("counting close due wins over the LED", [](In &i) { i.mode = 0; i.open = 1; i.due = true; i.ledRem = 3; });
  // OCCUPANCY: occupied or not, the debounce below, at and above the wake time, and setting 0.
  for (int occ : {0, 1}) for (uint32_t set1 : {0u, 999u, 1000u, 5000u, 3589000u, 3590000u, 3591000u, 3592000u, 7200000u}) {
    if (!occ && set1 > 5000u) continue;
    add("occupancy occupied=" + std::to_string(occ) + " set1=" + std::to_string(set1),
        [=](In &i) { i.mode = 1; i.occupied = occ; i.set1 = set1; i.now = T + 10; });
  }
  for (uint32_t set1 : {0u, 5000u, 50000000u}) {
    add("occupancy closed set1=" + std::to_string(set1), [=](In &i) { i.mode = 1; i.open = 1; i.occupied = true; i.set1 = set1; });
  }
  add("occupancy unknown untrusted", [](In &i) { i.mode = 1; i.open = 2; i.trusted = false; i.occupied = true; i.set1 = 5000; i.interval = 900; });
  add("occupancy closed unoccupied", [](In &i) { i.mode = 1; i.open = 1; i.occupied = false; });
  // The nightly heap guard: at and around the reset and warn thresholds, overnight or not.
  for (unsigned long heap : {0UL, RESET - 1, RESET, RESET + 1, WARN - 1, WARN, WARN + 1, 1000000UL}) {
    add("heap overnight " + std::to_string(heap), [=](In &i) { i.open = 1; i.heap = heap; });
  }
  for (unsigned long heap : {0UL, RESET, WARN}) {
    add("heap daytime " + std::to_string(heap), [=](In &i) { i.open = 0; i.heap = heap; });
  }
  add("heap unknown clock", [](In &i) { i.open = 2; i.trusted = false; i.heap = 0; });
  // WO-2026-10-25-001: an occupied OCCUPANCY device sleeps the LED time left + 1, or the full debounce when none is
  // left, never past the reporting boundary wake. The boundary wake is set through the untrusted-clock interval
  // (wake = interval) so it can sit below 2 s. Expected values come from the rule, not from the plan block.
  // The base blocks have no such rule, so mode "a" does not run these.
  if (!useBase) {
    const uint32_t DEBOUNCE_SEC = 300;
    for (uint32_t rem : {300u, 120u, 5u, 1u, 0u}) for (int near : {1, 0}) {
      const int boundary = near ? (rem ? (int)rem : 100) : (int)rem + 1000;
      const int want = rem ? std::min((int)rem + 1, boundary) : std::min((int)DEBOUNCE_SEC, boundary);
      add("debounce-left occupied ledRem=" + std::to_string(rem) + " boundary=" + std::to_string(boundary),
          [=](In &i) { i.mode = 1; i.occupied = true; i.set1 = DEBOUNCE_SEC * 1000; i.ledRem = rem; i.trusted = false; i.interval = (uint16_t)boundary; },
          want);
    }
  }
#endif

  int index = 0;
  for (const Sc &s : table) runScenario(s, useBase, index++);
  if (nExpectMismatch) printf("EXPECT_MISMATCHES %d\n", nExpectMismatch);
  printf("SCENARIOS %d PASSES %d PLAN_EXIT %d GUARD_EXIT %d CONTINUE %d DISTINCT_EVENTS %d\n", scenarios, scenarios, nExitPlan,
         nExitGuard, nContinue, (int)distinctEvents.size());
  for (const auto &e : distinctEvents) printf("EVENT %s\n", e.c_str());
  return 0;
}
