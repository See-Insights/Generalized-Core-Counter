// WO-2026-10-21-002 (Sleep S-3): equivalence of the cloud-operations gate extraction.
// (a) is the base handler's head (entry resets, CONNECTED+OPEN abort, firmware and occupancy exits,
// the inline gate block), extracted by marker text from the base State_Sleep.cpp (base::), (b) is the
// current handler head and cloudGatePass() with the cloudGate struct, extracted by marker text from
// the current file (cur::). Both run one scenario table against the shared fakes
// (tests/stubs/sleep_teardown/SleepExecutionFakes.h). Mode "a" writes the frozen fixture, mode "b"
// must equal it. Build with -DWiring_Cellular=1 -DHAL_PLATFORM_CELLULAR=1 and -DENABLE_GATE_TRACE=0|1.
#include <cstdio>
#include <cstring>
#include <functional>
#include <set>
#include <string>
#include "SleepExecutionFakes.h"

#ifndef ENABLE_GATE_TRACE
#define ENABLE_GATE_TRACE 0
#endif

bool SystemConfig::get_verboseMode() { return sim::exec.verbose; }
bool SystemConfig::get_serialConnected() { return false; }
static bool cfgOccupied = false;
bool CurrentReadings::get_occupied() { return cfgOccupied; }
void RecoveryState::raiseAlert(int8_t value) { sim::emit("ALERT %d", (int)value); }

State state = SLEEPING_STATE, oldState = SLEEPING_STATE;
volatile bool firmwareUpdateInProgress = false;
static int opennessVerdict = 0;  // 0 Open, 1 Closed, 2 Unknown
static bool reachedRun = false;
inline bool isWithinOpenHours() { return opennessVerdict != 1; }
inline void publishStateTransition() { sim::emit("CALL publishStateTransition"); }
inline void ensureSensorEnabled(const char *why) { sim::emit("CALL ensureSensorEnabled %s", why); }
namespace Clock {
enum class Openness { Open, Closed, Unknown };
inline Openness openness() { return opennessVerdict == 0 ? Openness::Open : opennessVerdict == 1 ? Openness::Closed : Openness::Unknown; }
}  // namespace Clock

namespace base {
namespace {
#include BASE_HELPERS_INC
#include BASE_HOIST_INC
bool effectiveStandby(bool requested) { return requested; }
}  // namespace
static void pass() {
#include BASE_REGION_INC
  (void)useNetworkStandbyRequested;
  reachedRun = true;
}
}  // namespace base

namespace cur {
namespace {
#include CUR_HELPERS_INC
#include CUR_GATE_INC
bool effectiveStandby(bool requested) { return requested; }
}  // namespace
static void pass() {
#include CUR_REGION_INC
  (void)useNetworkStandbyRequested;
  reachedRun = true;
}
}  // namespace cur

struct In {
  bool conn = true, qEmpty = true, cfg = true, out = true, upd = false, web = false, fw = false, occ = false;
  bool disc = false, vb = false, occupied = false, enter = false;
  int depth = 0, mode = 3, open = 1;
};
struct Step { unsigned long t; In in; };
struct Sc { std::string name; std::vector<Step> steps; };

struct Builder {
  Sc sc;
  In cur;
  explicit Builder(const char *name, std::function<void(In &)> init = {}) {
    sc.name = name;
    if (init) init(cur);
  }
  Builder &at(unsigned long t, std::function<void(In &)> f = {}) {
    if (f) f(cur);
    sc.steps.push_back({t, cur});
    cur.enter = false;
    return *this;
  }
};

static std::vector<Sc> table;
static std::set<std::string> distinctEvents;
static int totalPasses = 0, nReached = 0, nHeld = 0, nExited = 0, scenarios = 0;
static const unsigned long T0 = 100000;

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

static void load(const In &in, bool entering) {
  sim::world = sim::World();
  sim::world.cloud = in.conn;
  sim::world.radioOn = true;
  sim::connectionMode = (uint8_t)in.mode;
  sim::exec.verbose = in.vb;
  sim::gate.queueEmpty = in.qEmpty;
  sim::gate.cfgSynced = in.cfg;
  sim::gate.outSynced = in.out;
  sim::gate.updatesPending = in.upd;
  sim::gate.disconnectRequested = in.disc;
  sim::gate.queueDepth = in.depth;
  session.awaitingWebhookResponse = in.web;
  session.occupancyChangeTriggered = in.occ;
  firmwareUpdateInProgress = in.fw;
  cfgOccupied = in.occupied;
  opennessVerdict = in.open;
  state = SLEEPING_STATE;
  oldState = entering ? IDLE_STATE : SLEEPING_STATE;
}

static void printFields(bool useBase) {
  if (useBase) {
    printf("    FIELDS ops=%d blocking=%d statusLog=%lu mask=%u before=%u known=%d start=%lu budget=%lu maxq=%u lastq=%u\n",
           base::operationsCompleteLogged, base::gateWasBlocking, base::lastGateStatusLogMs, base::lastGateBlockerMask,
           base::lastGateBlockerBeforeRelease, base::gateReleaseReasonKnown, base::cloudSyncStartMs,
           base::cloudSyncBudgetMs, base::cloudSyncMaxQueueDepth, base::lastQueueDepth);
  } else {
    const auto &g = cur::cloudGate;
    printf("    FIELDS ops=%d blocking=%d statusLog=%lu mask=%u before=%u known=%d start=%lu budget=%lu maxq=%u lastq=%u\n",
           g.operationsCompleteLogged, g.gateWasBlocking, g.lastGateStatusLogMs, g.lastGateBlockerMask,
           g.lastGateBlockerBeforeRelease, g.gateReleaseReasonKnown, g.cloudSyncStartMs, g.cloudSyncBudgetMs,
           g.cloudSyncMaxQueueDepth, g.lastQueueDepth);
  }
}

static void runScenario(const Sc &s, bool useBase, int index) {
  printf("S%d %s\n", index, s.name.c_str());
  bool first = true;
  int p = 0;
  for (const Step &st : s.steps) {
    sim::events.clear();
    sim::transitions = 0;
    sim::reads = sim::Reads();
    sim::now = T0 + st.t;
    reachedRun = false;
    load(st.in, first || st.in.enter);
    first = false;
    if (useBase) base::pass(); else cur::pass();
    printf("  p%d t=%lu conn=%d q=%d/%d cfg=%d out=%d upd=%d web=%d fw=%d occ=%d disc=%d mode=%d open=%d vb=%d ent=%d\n", p,
           (unsigned long)st.t, st.in.conn, st.in.qEmpty, st.in.depth, st.in.cfg, st.in.out, st.in.upd, st.in.web, st.in.fw,
           st.in.occ, st.in.disc, st.in.mode, st.in.open, st.in.vb, (int)(p == 0 || st.in.enter));
    for (const auto &e : sim::events) {
      printf("    %s\n", e.c_str());
      distinctEvents.insert(normalize(e));
    }
    printf("    RET reachedRun=%d transitions=%d process=%d watchdog=%d\n", reachedRun, sim::transitions, sim::reads.process,
           sim::reads.watchdog);
    printFields(useBase);
    totalPasses++;
    if (reachedRun) nReached++;
    else if (sim::transitions) nExited++;
    else nHeld++;
    p++;
  }
  scenarios++;
}

static void add(Builder &b) { table.push_back(b.sc); }

static void blockerCase(const char *tag, bool q, bool cfg, bool out, bool web, bool upd) {
  const unsigned long B = ConnectivityPolicy::CLOUD_OPS_GATE_TIMEOUT_MS;
  const unsigned long Lf = ConnectivityPolicy::OUTPUT_LEDGER_SYNC_TIMEOUT_MS;
  const unsigned long budget = (!out && Lf > B) ? Lf : B;
  auto block = [=](In &i) { i.qEmpty = !q; i.cfg = cfg; i.out = out; i.web = web; i.upd = upd; i.vb = true; };
  {  // blocked, never clears: the log thresholds, the status interval, then the timeout at budget-1, budget, budget+1
    Builder b("", [&](In &i) { block(i); });
    b.sc.name = std::string("timeout ") + tag;
    b.at(0).at(4000).at(5100).at(11000).at(12000).at(budget - 1).at(budget).at(budget + 1).at(budget + 2000);
    add(b);
  }
  {  // blocked past the log threshold, then released
    Builder b("", [&](In &i) { block(i); });
    b.sc.name = std::string("release ") + tag;
    b.at(0).at(5000).at(11000).at(16000).at(20000, [](In &i) { i.qEmpty = true; i.cfg = true; i.out = true; i.web = false; i.upd = false; })
        .at(21000);
    add(b);
  }
}

int main(int argc, char **argv) {
  const bool useBase = argc > 1 && strcmp(argv[1], "a") == 0;
  printf("GATE_TRACE %d\n", ENABLE_GATE_TRACE);
  const unsigned long B = ConnectivityPolicy::CLOUD_OPS_GATE_TIMEOUT_MS;
  const unsigned long L = ConnectivityPolicy::OUTPUT_LEDGER_SYNC_TIMEOUT_MS;

  // Every blocker combination (queue, config ledger, output ledger, webhook, update): timeout and release.
  for (int m = 1; m < 16; m++) {
    std::string tag = std::string(m & 1 ? "q" : "") + (m & 2 ? "L" : "") + (m & 4 ? "w" : "") + (m & 8 ? "u" : "");
    blockerCase(tag.c_str(), m & 1, !(m & 2), true, m & 4, m & 8);
  }
  blockerCase("outputLedger", false, true, false, false, false);
  blockerCase("outputLedger+q", true, true, false, false, false);
  blockerCase("bothLedgers", false, false, false, false, false);
  blockerCase("outputLedger+webhook", false, true, false, true, false);
  blockerCase("outputLedger+update", false, true, false, false, true);

  // Elapsed just under, at and over a raised budget: depth drives computeCloudSyncTimeoutMs.
  for (int depth : {1, 10, 30, 60, 200}) {
    unsigned long q = 2000UL + (unsigned long)depth * 1000UL + 5000UL;
    unsigned long budget = q < B ? B : (q > 120000UL ? 120000UL : q);
    Builder b("", [&](In &i) { i.qEmpty = false; i.depth = depth; i.vb = true; });
    b.sc.name = "queue depth " + std::to_string(depth);
    b.at(0).at(11000).at(budget - 1).at(budget).at(budget + 1);
    add(b);
  }
  {  // depth grows mid-wait and raises the budget, then drains (QUEUE_DRAIN progress)
    Builder b("queue grows then drains", [](In &i) { i.qEmpty = false; i.depth = 2; });
    b.at(0).at(2000, [](In &i) { i.depth = 30; }).at(B - 1).at(B).at(36999, [](In &i) { i.depth = 60; }).at(37000)
        .at(60000, [](In &i) { i.depth = 40; }).at(66999).at(67000, [](In &i) { i.depth = 41; }).at(67001, [](In &i) { i.depth = 5; })
        .at(68000).at(69000, [](In &i) { i.depth = 5; }).at(70000, [](In &i) { i.depth = 0; });
    add(b);
  }
  {  // the budget never shrinks when the queue empties while another blocker holds
    Builder b("budget stays raised", [](In &i) { i.qEmpty = false; i.depth = 30; i.vb = true; });
    b.at(0).at(1000, [](In &i) { i.qEmpty = true; i.depth = 0; i.web = true; }).at(36999).at(37000).at(37001);
    add(b);
  }
  {  // the ledger-timeout floor on top of a smaller queue-aware budget, and under a larger one
    Builder b("ledger floor over queue budget", [](In &i) { i.out = false; i.qEmpty = false; i.depth = 30; });
    b.at(0).at(L - 1).at(L).at(L + 1);
    add(b);
    Builder c("ledger floor under queue budget", [](In &i) { i.out = false; i.qEmpty = false; i.depth = 100; });
    c.at(0).at(L).at(L + 1).at(100000).at(107000).at(107001);
    add(c);
    Builder d("ledger floor appears late", [](In &i) { i.qEmpty = false; i.depth = 0; });
    d.at(0).at(B - 1, [](In &i) { i.out = false; }).at(B).at(L - 1).at(L).at(L + 1);
    add(d);
  }

  // Release with no block, with a block, and the variants of "unknown" and "reason=none".
  {
    Builder b("release no block silent", [](In &) {});
    b.at(0).at(1000).at(2000);
    add(b);
    Builder v("release no block verbose", [](In &i) { i.vb = true; i.occupied = true; i.mode = 3; });
    v.at(0).at(1000);
    add(v);
    Builder s("release after short block", [](In &i) { i.qEmpty = false; i.vb = true; });
    s.at(0).at(1000).at(2000, [](In &i) { i.qEmpty = true; }).at(3000);
    add(s);
    Builder r("release after block then fresh block", [](In &i) { i.web = true; });
    r.at(0).at(3000, [](In &i) { i.web = false; }).at(4000, [](In &i) { i.upd = true; }).at(8000, [](In &i) { i.upd = false; });
    add(r);
    Builder n("release reason none after long mask", [](In &i) { i.qEmpty = false; });
    n.at(0).at(11000).at(12000, [](In &i) { i.qEmpty = true; }).at(13000);
    add(n);
    Builder g("block, disconnect, reconnect, release unknown", [](In &i) { i.qEmpty = false; });
    g.at(0).at(2000).at(3000, [](In &i) { i.conn = false; }).at(4000, [](In &i) { i.conn = true; i.qEmpty = true; }).at(5000);
    add(g);
    Builder h("changing blockers mask log", [](In &i) { i.qEmpty = false; i.vb = true; });
    h.at(0).at(11000).at(12000, [](In &i) { i.web = true; }).at(13000, [](In &i) { i.qEmpty = true; i.web = true; })
        .at(17000, [](In &i) { i.web = false; i.upd = true; }).at(18000, [](In &i) { i.upd = false; });
    add(h);
    Builder k("status log interval", [](In &i) { i.upd = true; });
    k.at(0).at(4999).at(5000).at(9999).at(10000).at(10001).at(15000).at(20000).at(25000);
    add(k);
  }

  // Not connected, and disconnectRequested (the gate is skipped, the else branch resets).
  {
    Builder a("not connected from the start", [](In &i) { i.conn = false; i.qEmpty = false; });
    a.at(0).at(1000);
    add(a);
    Builder b("connected then dropped mid-gate", [](In &i) { i.qEmpty = false; });
    b.at(0).at(5000).at(6000, [](In &i) { i.conn = false; }).at(7000, [](In &i) { i.conn = true; }).at(8000);
    add(b);
    Builder c("disconnect requested skips the gate", [](In &i) { i.qEmpty = false; i.disc = true; });
    c.at(0).at(40000);
    add(c);
    Builder d("gate then disconnect requested", [](In &i) { i.qEmpty = false; i.web = true; });
    d.at(0).at(11000).at(12000, [](In &i) { i.disc = true; }).at(13000, [](In &i) { i.disc = false; }).at(14000);
    add(d);
    Builder e("timeout restart after not connected", [](In &i) { i.upd = true; });
    e.at(0).at(B - 1).at(B).at(B + 1, [](In &i) { i.conn = false; }).at(B + 2, [](In &i) { i.conn = true; }).at(B + 3).at(2 * B + 2);
    add(e);
    Builder f("operations logged once and again after entry", [](In &i) { i.vb = true; });
    f.at(0).at(1000).at(2000, [](In &i) { i.enter = true; }).at(3000);
    add(f);
  }

  // Priority exits mid-gate and the resets they own.
  {
    Builder a("firmware exit mid-gate", [](In &i) { i.qEmpty = false; });
    a.at(0).at(5000).at(6000, [](In &i) { i.fw = true; }).at(7000, [](In &i) { i.fw = false; }).at(8000);
    add(a);
    Builder b("firmware exit then late return", [](In &i) { i.qEmpty = false; });
    b.at(0).at(20000).at(25000, [](In &i) { i.fw = true; }).at(26000, [](In &i) { i.fw = false; }).at(B + 5000).at(B + 25999).at(B + 26000);
    add(b);
    Builder c("occupancy exit mid-gate", [](In &i) { i.qEmpty = false; });
    c.at(0).at(5000).at(6000, [](In &i) { i.occ = true; }).at(7000, [](In &i) { i.occ = false; }).at(8000);
    add(c);
    Builder d("occupancy exit then late return", [](In &i) { i.qEmpty = false; });
    d.at(0).at(20000).at(25000, [](In &i) { i.occ = true; }).at(26000, [](In &i) { i.occ = false; }).at(B + 5000).at(B + 25999).at(B + 26000);
    add(d);
    Builder e("firmware and occupancy while disconnect requested", [](In &i) { i.qEmpty = false; i.fw = true; i.occ = true; i.disc = true; });
    e.at(0).at(1000);
    add(e);
    Builder f("firmware wins over occupancy", [](In &i) { i.qEmpty = false; i.fw = true; i.occ = true; });
    f.at(0);
    add(f);
    Builder g("occupancy exit with a raised budget", [](In &i) { i.qEmpty = false; i.depth = 30; });
    g.at(0).at(10000).at(11000, [](In &i) { i.occ = true; }).at(12000, [](In &i) { i.occ = false; i.depth = 0; }).at(40000).at(41999);
    add(g);
    Builder h("CONNECTED mode open aborts", [](In &i) { i.mode = 0; i.open = 0; i.qEmpty = false; });
    h.at(0).at(1000);
    add(h);
    Builder j("CONNECTED mode abort mid-gate", [](In &i) { i.mode = 3; i.qEmpty = false; });
    j.at(0).at(20000).at(21000, [](In &i) { i.mode = 0; i.open = 0; }).at(22000, [](In &i) { i.mode = 3; i.open = 1; }).at(B + 20000).at(B + 21000);
    add(j);
    Builder k("CONNECTED mode closed does not abort", [](In &i) { i.mode = 0; i.open = 1; });
    k.at(0).at(1000);
    add(k);
    Builder l("CONNECTED mode unknown openness aborts", [](In &i) { i.mode = 0; i.open = 2; });
    l.at(0);
    add(l);
    Builder m("keep-alive open hours", [](In &i) { i.mode = 3; i.open = 0; i.qEmpty = false; });
    m.at(0).at(1000);
    add(m);
    Builder n("entry mid-gate resets the gate fields", [](In &i) { i.qEmpty = false; i.depth = 30; i.vb = true; });
    n.at(0).at(11000).at(12000, [](In &i) { i.enter = true; }).at(13000).at(B + 13000);
    add(n);
    Builder o("entry after a timeout", [](In &i) { i.web = true; });
    o.at(0).at(B).at(B + 1, [](In &i) { i.enter = true; }).at(B + 2);
    add(o);
  }

  int index = 0;
  for (const Sc &s : table) runScenario(s, useBase, index++);
  printf("SCENARIOS %d PASSES %d REACHED_RUN %d HELD %d EXITED %d DISTINCT_EVENTS %d\n", scenarios, totalPasses, nReached,
         nHeld, nExited, (int)distinctEvents.size());
  for (const auto &e : distinctEvents) printf("EVENT %s\n", e.c_str());
  return 0;
}
