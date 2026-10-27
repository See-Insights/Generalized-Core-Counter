// WO-2026-10-21-004 (Sleep S-5): equivalence of the pre-sleep summary and Wi-Fi-off guard extraction.
// (a) is the base handler's summary block, Wi-Fi guard block, four guard statics and entry resets,
// extracted by marker text from the base State_Sleep.cpp (base::); (b) is the current handler's two call
// lines and entry resets, with endOfCycleSummary(), wifiGuard and wifiOffGuardPass() extracted by marker text
// from the current file (cur::). Both run one scenario table against the shared fakes
// (tests/stubs/sleep_teardown). Mode "a" writes the frozen fixture, mode "b" must equal it.
// Builds (all -DWiring_Cellular=1 -DHAL_PLATFORM_CELLULAR=1): release (the full single-pass table, guard
// compiled out as on Boron), -DDEV_BUILD=1 (the awake-ceiling allowance) and -DWiring_WiFi=1 (the guard).
#include <cmath>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include "SleepExecutionFakes.h"

#ifndef DEV_BUILD
#define DEV_BUILD 0
#endif
#if Wiring_WiFi
#define BUILD_NAME "wifi"
#elif DEV_BUILD
#define BUILD_NAME "devbuild"
#else
#define BUILD_NAME "release"
#endif

struct In {
  unsigned long now = 61000;
  bool standby = false, cloud = false, radio = false, verbose = false, entered = false;
  int seedRetries = -1;
};
struct Sc {
  std::string name;
  uint32_t wake = 1000, budget = 0, connDur = 0, svc = 2500, td = 3100;
  Observability::WakeCycleStats::ConnectAttemptType attempt = Observability::WakeCycleStats::ConnectAttemptType::NONE;
  Observability::WakeCycleStats::ConnectResult result = Observability::WakeCycleStats::ConnectResult::SUCCESS;
  uint16_t qBefore = 2, qAfter = 3;
  int qNow = 5;
  float soc = 87.5f;
  int batt = 1;
  long lastConn = 1700000000L, preLast = 0;
  std::vector<In> passes{In()};
};
static In cur_in;
static long cfgLastConn = 0;
static bool reachedSleep = false;

bool SystemConfig::get_verboseMode() { return cur_in.verbose; }
time_t SystemConfig::get_lastConnection() { return (time_t)cfgLastConn; }

namespace base {
#if Wiring_WiFi
#include BASE_STATICS_INC
#endif
static void pass(bool entered, bool useNetworkStandby) {
  if (entered) {
#include BASE_RESET_INC
  }
  reachedSleep = false;
#include BASE_REGION_INC
  reachedSleep = true;
}
#if Wiring_WiFi
static void seed(uint8_t n) { wifiOffGuardRetries = n; }
static void snap(char *b, size_t n) {
  snprintf(b, n, "guard active=%d start=%lu last=%lu retries=%u", (int)wifiOffGuardActive, wifiOffGuardStartMs,
           wifiOffGuardLastRetryMs, (unsigned)wifiOffGuardRetries);
}
#endif
}  // namespace base

namespace cur {
namespace {
#include CUR_FUNCS_INC
}  // namespace
static void pass(bool entered, bool useNetworkStandby) {
  if (entered) {
#include CUR_RESET_INC
  }
  reachedSleep = false;
#include CUR_REGION_INC
  reachedSleep = true;
}
#if Wiring_WiFi
static void seed(uint8_t n) { wifiGuard.wifiOffGuardRetries = n; }
static void snap(char *b, size_t n) {
  snprintf(b, n, "guard active=%d start=%lu last=%lu retries=%u", (int)wifiGuard.wifiOffGuardActive,
           wifiGuard.wifiOffGuardStartMs, wifiGuard.wifiOffGuardLastRetryMs, (unsigned)wifiGuard.wifiOffGuardRetries);
}
#endif
}  // namespace cur

static std::vector<Sc> table;
static std::set<std::string> distinctEvents;
static int scenarios = 0, passesRun = 0, nWait = 0, nContinue = 0;

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

static Sc &add(const std::string &name) {
  table.push_back(Sc());
  table.back().name = name;
  return table.back();
}

static void runScenario(const Sc &s, bool useBase, int index) {
  using Stats = Observability::WakeCycleStats;
  printf("S%d %s\n", index, s.name.c_str());
  Observability::cycleStats() = Stats();
  Stats &st = Observability::cycleStats();
  st.wake_start_ms = s.wake;
  st.connect_attempt_type = s.attempt;
  st.connect_budget_ms = s.budget;
  st.connect_duration_ms = s.connDur;
  st.connect_result = s.result;
  st.service_duration_ms = s.svc;
  st.teardown_duration_ms = s.td;
  st.publish_queue_depth_before_connect = s.qBefore;
  st.publish_queue_depth_after_connect = s.qAfter;
  st.last_success_epoch = (time_t)s.preLast;
  st.total_awake_ms = s.wake == 0 ? 4000000u : 0;
  sim::gate.queueDepth = s.qNow;
  sim::soc = s.soc;
  sim::battState = (uint8_t)s.batt;
  cfgLastConn = s.lastConn;
  printf("  stats wake=%u budget=%u conn=%u svc=%u td=%u attempt=%d result=%d q=%u/%u qNow=%d soc=%.2f batt=%d last=%ld preLast=%ld\n",
         s.wake, s.budget, s.connDur, s.svc, s.td, (int)s.attempt, (int)s.result, (unsigned)s.qBefore, (unsigned)s.qAfter, s.qNow,
         (double)s.soc, s.batt, s.lastConn, s.preLast);
  int pi = 0;
  for (const In &p : s.passes) {
    sim::events.clear();
    cur_in = p;
    sim::now = p.now;
    sim::world.cloud = p.cloud;
    sim::world.radioOn = p.radio;
    printf("  P%d in now=%lu standby=%d cloud=%d radio=%d verbose=%d entered=%d\n", pi, p.now, (int)p.standby, (int)p.cloud,
           (int)p.radio, (int)p.verbose, (int)p.entered);
#if Wiring_WiFi
    if (p.seedRetries >= 0) {
      if (useBase) base::seed((uint8_t)p.seedRetries); else cur::seed((uint8_t)p.seedRetries);
      printf("    seed retries=%d\n", p.seedRetries);
    }
#endif
    if (useBase) base::pass(p.entered, p.standby); else cur::pass(p.entered, p.standby);
    for (const auto &e : sim::events) {
      printf("    %s\n", e.c_str());
      distinctEvents.insert(normalize(e));
    }
    char g[160] = "guard n/a";
#if Wiring_WiFi
    if (useBase) base::snap(g, sizeof g); else cur::snap(g, sizeof g);
#endif
    printf("  OUT %s awake=%u soc=%u chg=%u %s\n", reachedSleep ? "CONTINUE" : "WAIT", st.total_awake_ms,
           (unsigned)st.battery_soc_tenths, (unsigned)st.is_charging, g);
    if (reachedSleep) nContinue++; else nWait++;
    passesRun++;
    pi++;
  }
  scenarios++;
}

int main(int argc, char **argv) {
  const bool useBase = argc > 1 && strcmp(argv[1], "a") == 0;
  [[maybe_unused]] typedef Observability::WakeCycleStats::ConnectAttemptType AT;
  [[maybe_unused]] typedef Observability::WakeCycleStats::ConnectResult CR;
  printf("BUILD %s\n", BUILD_NAME);

  [[maybe_unused]] const unsigned long CEIL = ConnectivityPolicy::CONNECT_BUDGET_DEEP_MS + ConnectivityPolicy::FIRMWARE_UPDATE_MAX_MS +
                             ConnectivityPolicy::OUTPUT_LEDGER_SYNC_TIMEOUT_MS +
                             (unsigned long)ConnectivityPolicy::DISCONNECT_MODEM_DEFAULT_SEC * 1000UL + 30000UL;
  [[maybe_unused]] const unsigned long W = 1000;  // the scenarios' wake start: awake = now - W

#if Wiring_WiFi
  // The Wi-Fi radio-off guard (compiled out on Boron). 5000 ms is a non-zero start so the retry timer is live.
  auto guardSc = [&](const std::string &name, std::vector<In> ps) -> Sc & {
    Sc &s = add("wifi " + name);
    s.passes = ps;
    if (!s.passes.empty()) s.passes[0].entered = true;
    return s;
  };
  auto P = [](unsigned long now, bool radio, bool cloud = false, bool standby = false, bool verbose = false) {
    In i;
    i.now = now; i.radio = radio; i.cloud = cloud; i.standby = standby; i.verbose = verbose;
    return i;
  };
  guardSc("radio already off", {P(5000, false)});
  guardSc("radio already off, cloud on", {P(5000, false, true)});
  guardSc("standby skips the guard", {P(5000, true, true, true)});
  guardSc("radio on at first pass", {P(5000, true)});
  guardSc("radio on then off, quiet", {P(5000, true), P(5100, false)});
  guardSc("radio on then off, verbose", {P(5000, true), P(5100, false, false, false, true)});
  guardSc("radio on, cloud on, two passes", {P(5000, true, true), P(5100, true, true)});
  guardSc("retry just under the interval", {P(5000, true), P(5399, true), P(5799, true)});
  guardSc("retry at the interval", {P(5000, true), P(5399, true), P(5400, true), P(5799, true), P(5800, true)});
  guardSc("retry over the interval", {P(5000, true), P(5401, true), P(5802, true)});
  guardSc("last wait pass at the limit, then give-up",
          {P(5000, true), P(5400, true), P(5800, true), P(6200, true), P(6600, true), P(7000, true), P(7001, true), P(7002, true)});
  guardSc("give-up one past the limit", {P(5000, true), P(7001, true), P(7001, false, false, false, true)});
  guardSc("give-up with cloud on", {P(5000, true, true), P(7001, true, true)});
  guardSc("radio off after give-up restarts the guard", {P(5000, true), P(7001, true), P(7100, true), P(7200, false, false, false, true)});
  guardSc("start at millis 0 retries every pass", {P(0, true), P(100, true), P(200, true)});
  {
    Sc &s = guardSc("entered again mid-guard resets it", {P(5000, true), P(5100, true), P(5200, true), P(5300, true)});
    s.passes[2].entered = true;
  }
  guardSc("radio off with the guard idle and verbose", {P(5000, false, false, false, true)});
  {
    Sc &s = guardSc("retry counter saturates at 255", {P(5000, true), P(5400, true), P(5800, true), P(6200, true)});
    s.passes[1].seedRetries = 254;
  }
  {
    Sc &s = guardSc("retry counter at 255 then give-up", {P(5000, true), P(5400, true), P(7001, true)});
    s.passes[1].seedRetries = 255;
  }
  {
    Sc &s = guardSc("retry counter 253 then give-up", {P(5000, true), P(5400, true), P(7001, true)});
    s.passes[1].seedRetries = 253;
  }
#elif DEV_BUILD
  // DEV_BUILD=1 allows the awake time to run over the ceiling; the other invariants still log.
  for (long d : {-1L, 0L, 1L, 30000L}) {
    Sc &s = add("dev awake=ceiling" + std::string(d < 0 ? "" : "+") + std::to_string(d));
    s.passes[0].now = W + CEIL + d;
  }
  { Sc &s = add("dev standby=0 cloud and radio on, over the ceiling"); s.passes[0].cloud = s.passes[0].radio = true; s.passes[0].now = W + CEIL + 5; }
  { Sc &s = add("dev connect over budget"); s.attempt = AT::DEEP; s.budget = 1000; s.connDur = 2001; }
  { Sc &s = add("dev verbose line over the ceiling"); s.passes[0].verbose = true; s.passes[0].now = W + CEIL + 5; }
#else
  // Standby on/off x cloud on/off x radio on/off, verbose.
  for (int sb : {0, 1}) for (int cl : {0, 1}) for (int rd : {0, 1}) {
    Sc &s = add("standby=" + std::to_string(sb) + " cloud=" + std::to_string(cl) + " radio=" + std::to_string(rd));
    s.passes[0].standby = sb; s.passes[0].cloud = cl; s.passes[0].radio = rd; s.passes[0].verbose = true;
  }
  add("quiet standby=0 all off");
  { Sc &s = add("quiet standby=0 cloud on radio on"); s.passes[0].cloud = s.passes[0].radio = true; }
  // Awake time against the ceiling (30 s slack included): under, at and over, and without the slack.
  for (long d : {-30001L, -30000L, -29999L, -1L, 0L, 1L, 30000L}) {
    Sc &s = add("awake=ceiling" + std::string(d < 0 ? "" : "+") + std::to_string(d));
    s.passes[0].now = W + CEIL + d;
  }
  { Sc &s = add("awake=0"); s.passes[0].now = W; }
  { Sc &s = add("wake start 0 keeps the preset awake time"); s.wake = 0; s.passes[0].verbose = true; }
  { Sc &s = add("over the ceiling and verbose"); s.passes[0].now = W + CEIL + 1; s.passes[0].verbose = true; }
  // Connect duration against budget + 1000: with and without an attempt.
  for (AT at : {AT::NONE, AT::NORMAL, AT::DEEP}) for (uint32_t cd : {30999u, 31000u, 31001u}) {
    Sc &s = add("connect attempt=" + std::to_string((int)at) + " budget=30000 dur=" + std::to_string(cd));
    s.attempt = at; s.budget = 30000; s.connDur = cd;
  }
  { Sc &s = add("connect budget 0 with a long duration"); s.attempt = AT::DEEP; s.budget = 0; s.connDur = 900000; }
  { Sc &s = add("connect none with a long duration"); s.attempt = AT::NONE; s.budget = 30000; s.connDur = 900000; }
  { Sc &s = add("connect timeout, verbose"); s.attempt = AT::DEEP; s.budget = 20000; s.connDur = 25000; s.result = CR::TIMEOUT; s.passes[0].verbose = true; }
  { Sc &s = add("connect aborted, verbose"); s.attempt = AT::NORMAL; s.budget = 20000; s.connDur = 100; s.result = CR::ABORTED; s.passes[0].verbose = true; }
  { Sc &s = add("connect all warnings together"); s.attempt = AT::NORMAL; s.budget = 100; s.connDur = 5000; s.passes[0].now = W + CEIL + 1;
    s.passes[0].cloud = s.passes[0].radio = true; s.passes[0].verbose = true; }
  // State of charge: sentinels, bounds and the +0.5 rounding.
  for (float soc : {-0.1f, 0.0f, 0.04f, 0.05f, 12.34f, 12.35f, 12.36f, 99.95f, 100.0f, 100.04f, 100.1f, NAN}) {
    Sc &s = add("soc=" + std::to_string(soc));
    s.soc = soc; s.passes[0].verbose = true;
  }
  // Charge state: unknown, and the mapping of the others.
  for (int b : {0, 1, 2, 3, 255}) {
    Sc &s = add("battState=" + std::to_string(b));
    s.batt = b; s.passes[0].verbose = true;
  }
  // Queue depth, last connection and sentinel depths.
  for (int q : {0, 65534, 65535, 65536, 70000}) {
    Sc &s = add("queue now=" + std::to_string(q));
    s.qNow = q; s.passes[0].verbose = true;
  }
  { Sc &s = add("depth sentinels before/after connect"); s.qBefore = 0xFFFF; s.qAfter = 0xFFFF; s.passes[0].verbose = true; }
  { Sc &s = add("last connection 0 keeps the preset"); s.lastConn = 0; s.preLast = 1699999999L; s.passes[0].verbose = true; }
  { Sc &s = add("last connection 0 and none preset"); s.lastConn = 0; s.preLast = 0; s.passes[0].verbose = true; }
#endif

  int index = 0;
  for (const Sc &s : table) runScenario(s, useBase, index++);
  printf("SCENARIOS %d PASSES %d WAIT %d CONTINUE %d DISTINCT_EVENTS %d\n", scenarios, passesRun, nWait, nContinue,
         (int)distinctEvents.size());
  for (const auto &e : distinctEvents) printf("EVENT %s\n", e.c_str());
  return 0;
}
