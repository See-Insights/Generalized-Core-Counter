// WO-2026-10-20-003 (Stage 6): equivalence of the sleep teardown move.
// (a) is the teardown block that the .sh extracts by marker text from the base
// State_Sleep.cpp (BASE_INC, wrapped below as base::run), (b) is the real
// state/SleepTeardown.cpp. Both run the same scenario table against the same
// fakes (tests/stubs/sleep_teardown); each prints the same transcript. Mode "a"
// writes the frozen fixture, mode "b" is compared to it.
// Build with -DWiring_Cellular=1 -DHAL_PLATFORM_CELLULAR=1 (cellular) or without (Wi-Fi).
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include "Particle.h"
#include "state/State_Common.h"
#include "power/Connectivity.h"
#include "power/ModemHealth.h"
#include "persist/PowerConfig.h"
#include "cloud/BatteryBackoffPolicy.h"
#include "observability/WakeCycleStats.h"
#include "ThrashGuard.h"

// ---- fakes behind the real config / alert / modem-health headers ----
static bool cfgVerbose = false;
static uint16_t cfgCloudSec = 0, cfgModemSec = 0;
static uint8_t cfgTier = 0;
static bool cfgOccupied = false;
bool SystemConfig::get_verboseMode() { return cfgVerbose; }
uint16_t SystemConfig::get_cloudDisconnectBudgetSec() { return cfgCloudSec; }
uint16_t SystemConfig::get_modemOffBudgetSec() { return cfgModemSec; }
uint8_t PowerConfig::get_currentBatteryTier() { return cfgTier; }
bool CurrentReadings::get_occupied() { return cfgOccupied; }
void RecoveryState::raiseAlert(int8_t value) { sim::emit("ALERT %d", (int)value); }
void markModemUnstable(ModemUnstableReason reason, unsigned long elapsedMs) {
  sim::emit("MODEM markUnstable reason=%d elapsed=%lu", (int)reason, elapsedMs);
}
void maybeClearModemUnstable(unsigned long teardownElapsedMs) {
  sim::emit("MODEM maybeClear elapsed=%lu", teardownElapsedMs);
}

// ---- (a) the base block, verbatim, in a thin wrapper ----
namespace base {
namespace {
#include BASE_LABELS_INC
}
static bool cloudDisconnectCompleteLogged = false, radioOffCompleteLogged = false, disconnectRequested = false;
static unsigned long disconnectRequestStartMs = 0, cloudDisconnectStartMs = 0, cloudDisconnectElapsedMs = 0,
                     modemOffElapsedMs = 0, lastDisconnectProgressMs = 0, lastGateProgressMs = 0;
static bool fellThrough = false;

static void enter() {
  cloudDisconnectCompleteLogged = radioOffCompleteLogged = disconnectRequested = false;
  disconnectRequestStartMs = cloudDisconnectStartMs = cloudDisconnectElapsedMs = modemOffElapsedMs =
      lastDisconnectProgressMs = lastGateProgressMs = 0;
}

static void guardLostTimestamp() {
#include BASE_SAFETY_INC
}

static void run(bool useNetworkStandbyRequested, bool useNetworkStandbyEffective,
                bool &forceModemOffAfterStandbyFailure, int &standbySleepFailureError) {
  (void)useNetworkStandbyRequested;
  (void)useNetworkStandbyEffective;
  (void)forceModemOffAfterStandbyFailure;
  (void)standbySleepFailureError;
  fellThrough = false;
#include BASE_BLOCK_INC
  fellThrough = true;
}
}  // namespace base

// ---- (b) the new module, compiled into this translation unit ----
#include "state/SleepTeardown.cpp"

struct Scenario {
  bool req, eff, force;
  int cloud0, modem0;  // modem0: 0 on, 1 in between, 2 off
  long cloudDelay, modemDelay, spont;
  int flicker;
  uint16_t cloudSec, modemSec;
  bool verbose, occupied;
  int mode, tier;
  unsigned long start;
  int err;
};

struct Fields {
  bool cdc, rol, dr;
  unsigned long drs, cds, cde, moe, ldp, lgp;
};

static unsigned long expectedBudgetMs(const Scenario &s) {
  unsigned cloud = s.cloudSec, modem = s.modemSec;
  if (cloud < 5 || cloud > 120) cloud = 15;
  if (modem < 5 || modem > 120) modem = 30;
  unsigned long ms = (unsigned long)(modem > cloud ? modem : cloud) * 1000UL;
  if (s.eff) {
    ms = (unsigned long)cloud * 1000UL;
    if (ms > 5000UL) ms = 5000UL;
  }
  return ms;
}

static std::set<std::string> distinctEvents;
static int totalPasses = 0, nWait = 0, nReady = 0, nError = 0, scenarios = 0;

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

static void runScenario(const Scenario &s, bool useBase, int index) {
  sim::world = sim::World();
  sim::world.cloud = s.cloud0 != 0;
  sim::world.radioOn = s.modem0 == 0;
  sim::world.modemOff = s.modem0 == 2;
  sim::world.cloudDelay = s.cloudDelay;
  sim::world.modemDelay = s.modemDelay;
  sim::world.flickerAfter = s.flicker;
  sim::now = s.start;
  if (s.spont >= 0 && s.cloud0 != 0) sim::world.cloudDropAt = (long)s.start + s.spont;
  cfgVerbose = s.verbose;
  cfgCloudSec = s.cloudSec;
  cfgModemSec = s.modemSec;
  cfgTier = (uint8_t)s.tier;
  cfgOccupied = s.occupied;
  sim::connectionMode = (uint8_t)s.mode;
  session = SessionState();
  sim::events.clear();
  sim::transitions = 0;

  bool force = s.force;
  int err = s.err;
  if (useBase) {
    base::enter();
  } else {
    SleepTeardown::enter();
  }

  printf("S%d req=%d eff=%d force=%d cloud0=%d modem0=%d cd=%ld md=%ld sp=%ld fl=%d bud=%u/%u v=%d occ=%d mode=%d tier=%d start=%lu\n",
         index, s.req, s.eff, s.force, s.cloud0, s.modem0, s.cloudDelay, s.modemDelay, s.spont, s.flicker,
         s.cloudSec, s.modemSec, s.verbose, s.occupied, s.mode, s.tier, s.start);

  const unsigned long B = expectedBudgetMs(s);
  std::set<unsigned long> offsets = {0, 1, 400, 999, 1000, 1001, 1002, 2500, 4999, 5000, 5001, 9999, 10000, 10001,
                                     B - 1, B, B + 1, B + 1002};
  int pass = 0;
  for (unsigned long off : offsets) {
    sim::now = s.start + off;
    sim::reads = sim::Reads();
    sim::events.clear();
    const int transBefore = sim::transitions;
    const char *result;
    Fields f;
    if (useBase) {
      base::guardLostTimestamp();
      base::run(s.req, s.eff, force, err);
      result = base::fellThrough ? "READY" : (sim::transitions != transBefore ? "ERROR" : "WAIT");
      f = {base::cloudDisconnectCompleteLogged, base::radioOffCompleteLogged, base::disconnectRequested,
           base::disconnectRequestStartMs, base::cloudDisconnectStartMs, base::cloudDisconnectElapsedMs,
           base::modemOffElapsedMs, base::lastDisconnectProgressMs, base::lastGateProgressMs};
    } else {
      SleepTeardown::guardLostTimestamp();
      const SleepTeardown::Result r = SleepTeardown::run(s.req, s.eff, force, err);
      if (r == SleepTeardown::Result::READY) setAppBreadcrumb(23);
      result = r == SleepTeardown::Result::READY ? "READY" : (r == SleepTeardown::Result::ERROR ? "ERROR" : "WAIT");
      const auto &t = SleepTeardown::s;
      f = {t.cloudDisconnectCompleteLogged, t.radioOffCompleteLogged, t.disconnectRequested,
           t.disconnectRequestStartMs, t.cloudDisconnectStartMs, t.cloudDisconnectElapsedMs,
           t.modemOffElapsedMs, t.lastDisconnectProgressMs, t.lastGateProgressMs};
    }
    printf("  p%d t=%lu %s f=%d%d%d,%lu,%lu,%lu,%lu,%lu,%lu force=%d err=%d sess=%lu,%u,%u,%u rd=%d,%d,%d,%d,%d,%d\n",
           pass, sim::now, result, f.cdc, f.rol, f.dr, f.drs, f.cds, f.cde, f.moe, f.ldp, f.lgp, force, err,
           session.lastTeardownEndMs, (unsigned)session.lastCloudDisconnectElapsedMs,
           (unsigned)session.lastModemOffElapsedMs, (unsigned)session.lastTotalTeardownElapsedMs,
           sim::reads.connected, sim::reads.isOff, sim::reads.isOn, sim::reads.millis, sim::reads.process,
           sim::reads.watchdog);
    for (const auto &e : sim::events) {
      printf("    %s\n", e.c_str());
      distinctEvents.insert(normalize(e));
    }
    pass++;
    totalPasses++;
    if (strcmp(result, "READY") == 0) { nReady++; break; }
    if (strcmp(result, "ERROR") == 0) { nError++; break; }
    nWait++;
  }
}

static unsigned lcg = 12345;
static unsigned draw(unsigned n) {
  lcg = lcg * 1103515245u + 12345u;
  return (lcg >> 16) % n;
}

int main(int argc, char **argv) {
  const bool useBase = argc > 1 && strcmp(argv[1], "a") == 0;
#if Wiring_Cellular
  printf("PLATFORM cellular\n");
  const int worlds[][2] = {{1, 0}, {0, 0}, {0, 1}, {0, 2}, {1, 1}};
  const int nWorlds = 5, nVariants = 4;
#else
  printf("PLATFORM wifi\n");
  const int worlds[][2] = {{1, 0}, {0, 0}, {0, 2}};
  const int nWorlds = 3, nVariants = 3;
#endif
  const long cloudDelays[] = {0, 400, 3000, 6000, -1};
  const long modemDelays[] = {0, 1500, 8000, -1};
  const uint16_t budgets[][2] = {{0, 0}, {5, 5}, {10, 40}, {200, 3}, {3, 200}};
  const long sponts[] = {-1, -1, 2000};
  const int standby[][2] = {{0, 0}, {1, 0}, {1, 1}};

  int index = 0;
  for (const auto &sb : standby)
    for (int force = 0; force < 2; force++)
      for (int w = 0; w < nWorlds; w++)
        for (int v = 0; v < nVariants; v++) {
          Scenario s;
          s.req = sb[0];
          s.eff = sb[1];
          s.force = force;
          s.cloud0 = worlds[w][0];
          s.modem0 = worlds[w][1];
          s.err = force ? -170 : 0;
          s.flicker = 0;
          if (v == 0) {
            s.cloudDelay = 0; s.modemDelay = 0; s.spont = -1; s.cloudSec = 0; s.modemSec = 0;
            s.start = 0; s.verbose = false;
          } else if (v == 1) {
            // Teardown completes exactly 10000 ms after the request: the slow-teardown edge.
            s.cloudDelay = 10000; s.modemDelay = 10000; s.spont = -1; s.cloudSec = 0; s.modemSec = 0;
            s.start = 1000 + draw(50000); s.verbose = false;
          } else {
            s.cloudDelay = cloudDelays[draw(5)];
            s.modemDelay = modemDelays[draw(4)];
            s.spont = sponts[draw(3)];
            const auto &b = budgets[draw(5)];
            s.cloudSec = b[0];
            s.modemSec = b[1];
            s.start = 1000 + draw(50000);
            s.verbose = draw(2) != 0;
            // The last variant makes the cloud read as connected after the first 1-3 reads of a
            // pass, which is the only way to reach the pre-sleep gate's blocked branches.
            if (v == nVariants - 1) s.flicker = 1 + (index % 3);
          }
          s.occupied = draw(2) != 0;
          s.mode = (int)draw(5);
          s.tier = (int)draw(5);
          runScenario(s, useBase, index++);
          scenarios++;
        }
  printf("SCENARIOS %d PASSES %d WAIT %d READY %d ERROR %d DISTINCT_EVENTS %d\n", scenarios, totalPasses, nWait,
         nReady, nError, (int)distinctEvents.size());
  for (const auto &e : distinctEvents) printf("EVENT %s\n", e.c_str());
  return 0;
}
