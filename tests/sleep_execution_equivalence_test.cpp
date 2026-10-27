// WO-2026-10-21-001 (Sleep S-2): equivalence of the sleep-execution refactor.
// (a) is the base handler's standby decision, hibernate block and ULP/STOP cascade, extracted
// by marker text from the base State_Sleep.cpp (wrapped below as base::), (b) is the current
// effectiveStandby / hibernateAttempt / sleepAndResume and the handler's call-site lines,
// extracted by marker text from the current file (cur::). Both run one scenario table against
// the same fakes (tests/stubs/sleep_teardown/SleepExecutionFakes.h); mode "a" writes the frozen
// fixture, mode "b" must equal it. Build with -DWiring_Cellular=1 -DHAL_PLATFORM_CELLULAR=1
// (cellular) or without (Wi-Fi), plus -DENABLE_SLEEP_TRACE=1 to compile the trace branch.
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include "SleepExecutionFakes.h"

static bool cfgOccupied = false;
bool SystemConfig::get_verboseMode() { return sim::exec.verbose; }
bool SystemConfig::get_serialConnected() { return sim::exec.serialCfg; }
bool CurrentReadings::get_occupied() { return cfgOccupied; }
void RecoveryState::raiseAlert(int8_t value) { sim::emit("ALERT %d", (int)value); }

namespace base {
namespace {
#include BASE_DRAIN_INC
}
static bool preserveStandbyAfterNormalReturn = false, forceModemOffAfterStandbyFailure = false;
static int standbySleepFailureError = SYSTEM_ERROR_NONE;
struct WakeContext { bool pirWake, buttonWake, timerWake; const char *wakeReasonLabel; };
static bool fell = false;

static bool effectiveStandby(bool useNetworkStandbyRequested) {
#include BASE_STANDBY_INC
  return useNetworkStandbyEffective;
}
static void hibernateAttempt(bool overnightFallbackSleep, int wakeInSeconds) {
#include BASE_HIB_INC
}
static void cascade(int wakeInSeconds, bool useNetworkStandby, const char *sleepReason, WakeContext &wake) {
  fell = false;
#include BASE_ULP_INC
  wake = {pirWake, buttonWake, timerWake, wakeReasonLabel};
  fell = true;
}
}  // namespace base

namespace cur {
namespace {
#include CUR_DRAIN_INC
#include CUR_FUNCS_INC
}  // namespace
// Records the arguments the handler hands to postWakeRouting(), the post-wake routing's only input.
static bool routedPir = false, routedButton = false, routedTimer = false;
static const char *routedLabel = nullptr;
static void postWakeRouting(bool pirWake, bool buttonWake, bool timerWake, const char *wakeReasonLabel) {
  routedPir = pirWake;
  routedButton = buttonWake;
  routedTimer = timerWake;
  routedLabel = wakeReasonLabel;
}
}  // namespace cur

struct Outcome { bool fell, pir, btn, timer; const char *label; bool eff; bool preserve, force; int err; };

static void driveBase(bool req, bool overnight, int wakeInSeconds, const char *reason, Outcome &o) {
  const bool eff = base::effectiveStandby(req);
  const bool useNetworkStandby = eff;
  if (SleepTeardown::run(req, eff, base::forceModemOffAfterStandbyFailure, base::standbySleepFailureError) !=
      SleepTeardown::Result::READY) { return; }
  base::hibernateAttempt(overnight, wakeInSeconds);
  base::WakeContext w{};
  base::cascade(wakeInSeconds, useNetworkStandby, reason, w);
  o = {base::fell, w.pirWake, w.buttonWake, w.timerWake, w.wakeReasonLabel, eff,
       base::preserveStandbyAfterNormalReturn, base::forceModemOffAfterStandbyFailure,
       base::standbySleepFailureError};
}

static void driveCur(bool useNetworkStandbyRequested, bool overnightFallbackSleep, int wakeInSeconds,
                     const char *sleepReason, Outcome &o) {
  using namespace cur;
#include CUR_CALL_EFFECTIVE_INC
  bool useNetworkStandby = useNetworkStandbyEffective;
#include CUR_CALL_TEARDOWN_INC
#include CUR_CALL_HIBERNATE_INC
#include CUR_CALL_SLEEP_INC
#include CUR_CALL_ROUTE_INC
  o = {true, routedPir, routedButton, routedTimer, routedLabel, useNetworkStandbyEffective,
       standbyReturn.preserveAfterNormalReturn, standbyReturn.forceModemOff, standbyReturn.error};
}

struct Sc {
  int prior = 0;     // 0 none, 1 preserved after a normal standby return, 2 pending forced-off (err -170)
  bool req = true, connectOk = true, cellOff = false, unstable = false, tdClears = false, tdWait = false;
  int hib = 0;       // 0 not overnight, 1 not eligible, 2 pin low, 3 stale clear fails, 4 alarm fails, 5/6 returns ok/err
  int ulp = 0;       // 0 timer, 1 button, 2 sensor pin, 3 err then STOP ok, 4 err, err, timer-only ok, 5 all fail
  bool sensorValid = true, serialCfg = false, serialPre = false, verbose = false, wdtOk = true, second = false;
  int serialUpAfter = 0;
};

static std::set<std::string> distinctEvents;
static int totalPasses = 0, nWoke = 0, nReset = 0, scenarios = 0;

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

// A run of 3+ identical blocks of up to 4 lines (the serial re-wait loop) prints once, then "... xN".
static void printEvents() {
  const auto &ev = sim::events;
  size_t i = 0;
  while (i < ev.size()) {
    bool done = false;
    for (size_t p = 1; p <= 4 && !done; p++) {
      size_t reps = 1;
      while (i + (reps + 1) * p <= ev.size() && std::equal(ev.begin() + i, ev.begin() + i + p, ev.begin() + i + reps * p)) reps++;
      if (reps >= 3) {
        for (size_t k = 0; k < p; k++) printf("    %s\n", ev[i + k].c_str());
        printf("    ... x%zu\n", reps);
        i += reps * p;
        done = true;
      }
    }
    if (!done) printf("    %s\n", ev[i++].c_str());
  }
  for (const auto &e : ev) distinctEvents.insert(normalize(e));
}

static void script(const Sc &s, bool first) {
  auto &x = sim::exec;
  x.sleeps.clear();
  x.nextSleep = 0;
  x.reconnecting = false;
  x.serialPolls = 0;
  const int hib = first ? s.hib : 0, ulp = first ? s.ulp : 0;
  if (hib >= 5) x.sleeps.push_back({hib == 5 ? 0 : -170, PIN_INVALID});
  const pin_t sensorPin = x.sensorPin;
  switch (ulp) {
    case 0: x.sleeps.push_back({0, PIN_INVALID}); break;
    case 1: x.sleeps.push_back({0, BUTTON_PIN}); break;
    case 2: x.sleeps.push_back({0, sensorPin}); break;
    case 3: x.sleeps.push_back({-170, PIN_INVALID}); x.sleeps.push_back({0, sensorPin}); break;
    case 4: x.sleeps.push_back({-170, PIN_INVALID}); x.sleeps.push_back({-170, PIN_INVALID});
            x.sleeps.push_back({0, PIN_INVALID}); break;
    default: for (int i = 0; i < 3; i++) x.sleeps.push_back({-170, PIN_INVALID});
  }
  x.hibEligible = hib >= 2;
  x.hibPinHigh = hib != 2;
  x.staleClearOk = hib != 3;
  x.alarmOk = hib != 4;
}

static void runScenario(const Sc &s, bool useBase, int index) {
  sim::world = sim::World();
  sim::world.modemOff = s.cellOff;
  sim::world.radioOn = !s.cellOff;
  session = SessionState();
  session.modemUnstable = s.unstable;
  Observability::cycleStats().connect_result = s.connectOk ? Observability::WakeCycleStats::ConnectResult::SUCCESS
                                                           : Observability::WakeCycleStats::ConnectResult::FAILED;
  sim::exec = sim::Exec();
  sim::exec.sensorPin = s.sensorValid ? 9 : PIN_INVALID;
  sim::exec.verbose = s.verbose;
  sim::exec.serialCfg = s.serialCfg;
  sim::exec.serialPre = s.serialPre;
  sim::exec.serialUpAfter = s.serialUpAfter;
  sim::exec.wdtStopOk = s.wdtOk;
  sim::exec.tdClears = s.tdClears;
  sim::exec.tdWait = s.tdWait;
  cfgOccupied = index % 2 == 1;
  const bool preserve = s.prior == 1, force = s.prior == 2;
  base::preserveStandbyAfterNormalReturn = preserve;
  base::forceModemOffAfterStandbyFailure = force;
  base::standbySleepFailureError = force ? -170 : SYSTEM_ERROR_NONE;
  cur::standbyReturn.preserveAfterNormalReturn = preserve;
  cur::standbyReturn.forceModemOff = force;
  cur::standbyReturn.error = force ? -170 : SYSTEM_ERROR_NONE;

  printf("S%d prior=%d req=%d conn=%d off=%d unst=%d td=%d hib=%d ulp=%d sv=%d ser=%d/%d/%d vb=%d wd=%d\n", index,
         s.prior, s.req, s.connectOk, s.cellOff, s.unstable, s.tdClears + 2 * s.tdWait, s.hib, s.ulp, s.sensorValid, s.serialCfg,
         s.serialPre, s.serialUpAfter, s.verbose, s.wdtOk);
  for (int pass = 0; pass < (s.second ? 2 : 1); pass++) {
    sim::now = 1000;
    sim::events.clear();
    sim::transitions = 0;
    script(s, pass == 0);
    const int wakeIn = index % 3 == 0 ? 900 : 3600;
    const bool overnight = pass == 0 && s.hib > 0;
    Outcome o{};
    o.fell = false;
    if (useBase) {
      driveBase(s.req, overnight, wakeIn, "scheduled", o);
    } else {
      driveCur(s.req, overnight, wakeIn, "scheduled", o);
    }
    if (!o.fell) {
      o.eff = false;
      o.preserve = useBase ? base::preserveStandbyAfterNormalReturn : cur::standbyReturn.preserveAfterNormalReturn;
      o.force = useBase ? base::forceModemOffAfterStandbyFailure : cur::standbyReturn.forceModemOff;
      o.err = useBase ? base::standbySleepFailureError : cur::standbyReturn.error;
    }
    printf("  p%d wakeIn=%d overnight=%d t=%lu\n", pass, wakeIn, overnight, sim::now);
    printEvents();
    printf("    RET fell=%d pir=%d btn=%d timer=%d label=%s transitions=%d\n", o.fell, o.pir, o.btn, o.timer,
           o.fell ? o.label : "-", sim::transitions);
    printf("    FIELDS eff=%d preserve=%d force=%d err=%d\n", o.eff, o.preserve, o.force, o.err);
    totalPasses++;
    (o.fell ? nWoke : nReset)++;
  }
  scenarios++;
}

int main(int argc, char **argv) {
  const bool useBase = argc > 1 && strcmp(argv[1], "a") == 0;
  const bool cellular = HAL_PLATFORM_CELLULAR != 0;
  printf("PLATFORM %s\n", cellular ? "cellular" : "wifi");
  const int priors = cellular ? 3 : 1;
  std::vector<Sc> table;
  for (int hib = 1; hib <= 6; hib++)  // hibernate edges (hib=0 is the ULP table's first row)
    for (int prior = 0; prior < priors; prior++)
      for (int req = 0; req < 2; req++) {
        Sc s; s.hib = hib; s.prior = prior; s.req = req; table.push_back(s);
      }
  for (int ulp = 0; ulp <= 5; ulp++)  // ULP / STOP cascade outcomes; a cellular standby pass is followed by a second
    for (int prior = 0; prior < priors; prior++)
      for (int req = 0; req < 2; req++) {
        Sc s; s.ulp = ulp; s.prior = prior; s.req = req; s.second = cellular && req; table.push_back(s);
      }
  if (cellular) {
    for (int prior = 0; prior < 3; prior++)  // inputs of the effective-standby decision
      for (int bits = 1; bits < 8; bits++) {
        Sc s; s.prior = prior; s.connectOk = bits & 1; s.cellOff = bits & 2; s.unstable = bits & 4;
        table.push_back(s);
      }
    for (int ulp = 0; ulp <= 3; ulp++) { Sc s; s.sensorValid = false; s.ulp = ulp; table.push_back(s); }  // no wake sensor
    for (int pre = 0; pre < 2; pre++)  // serial: connected beforehand or not, re-enumerates at once / late / never
      for (int up : {0, 2, -1}) { Sc s; s.serialCfg = true; s.serialPre = pre; s.serialUpAfter = up; table.push_back(s); }
    for (int ulp : {0, 3}) { Sc s; s.verbose = true; s.prior = 1; s.ulp = ulp; table.push_back(s); }
    { Sc s; s.wdtOk = false; s.hib = 5; table.push_back(s); }
    { Sc s; s.wdtOk = false; s.ulp = 4; table.push_back(s); }
    for (int ulp : {0, 3}) { Sc s; s.prior = 2; s.tdClears = true; s.ulp = ulp; s.second = true; table.push_back(s); }
    for (int why = 0; why < 4; why++) {  // teardown still waiting: the handler returns before the sleep, what do the fields hold?
      Sc s; s.prior = 1; s.tdWait = true; s.req = why != 1; s.unstable = why == 2; s.cellOff = why == 3; table.push_back(s);
    }
  }

  int index = 0;
  for (const Sc &s : table) runScenario(s, useBase, index++);
  printf("SCENARIOS %d PASSES %d WOKE %d RETURNED %d DISTINCT_EVENTS %d\n", scenarios, totalPasses, nWoke, nReset,
         (int)distinctEvents.size());
  for (const auto &e : distinctEvents) printf("EVENT %s\n", e.c_str());
  return 0;
}
