// WO-2026-10-24-001 (recovery item 18): the connectivity failsafe acts when
// openness is Unknown (an untrusted clock after its own stage-2 reset) and
// still skips when it is Closed.
//
// Driven by tests/connectivity_failsafe_unknown_openness_test.sh, which
// extracts the REAL connectivityFailsafeSupervisor() body from
// src/power/ConnectivityFailsafe.cpp into SUPERVISOR_BODY_H (production build,
// CONNECTIVITY_FAILSAFE_TEST_MODE=0) and compiles it here against small fakes.
// REAL_STALE_SEC / REAL_COOLDOWN_SEC / REAL_JITTER_MAX_SEC come from
// src/power/ConnectivityPolicy.h.

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <ctime>

#ifndef SUPERVISOR_BODY_H
#error "SUPERVISOR_BODY_H must name the extracted supervisor body"
#endif
#ifndef REAL_STALE_SEC
#error "REAL_STALE_SEC must be injected from src/power/ConnectivityPolicy.h"
#endif

enum BatteryTier { TIER_HEALTHY, TIER_CONSERVING, TIER_CRITICAL, TIER_SURVIVAL };
enum { FIRMWARE_UPDATE_STATE = 99 };
enum { BREADCRUMB_CONNECTIVITY_FAILSAFE_HARD = 1, RESET_CAUSE_CONNECTIVITY_FAILSAFE = 2 };

namespace {
enum Action { NONE, RESET, DEEP_POWER_DOWN };
Action lastActionTaken = NONE;
int actionCount = 0;
uint8_t persistedStage = 0;
time_t persistedAction = 0;
constexpr uint32_t FAKE_JITTER_SEC = 1000;
} // namespace

namespace Clock {
enum class Openness : uint8_t { Open, Closed, Unknown };
Openness fakeOpenness = Openness::Open;
inline bool isTimeValid() { return true; }
inline Openness openness() { return fakeOpenness; }
} // namespace Clock

namespace ConnectivityPolicy {
constexpr time_t CONNECTIVITY_FAILSAFE_STALE_SEC = REAL_STALE_SEC;
constexpr time_t CONNECTIVITY_FAILSAFE_COOLDOWN_SEC = REAL_COOLDOWN_SEC;
} // namespace ConnectivityPolicy

namespace RecoveryState {
uint8_t stage = 0;
time_t lastAction = 0;
inline uint8_t get_connectivityRecoveryStage() { return stage; }
inline time_t get_lastConnectivityRecoveryAction() { return lastAction; }
} // namespace RecoveryState

namespace SystemConfig {
enum { CONNECTED, DISCONNECTED };
time_t lastConnection = 0;
time_t openTimeToday = 0;
inline int get_openTime() { return 6; }
inline time_t get_lastConnection() { return lastConnection; }
} // namespace SystemConfig

namespace DailyBoundary {
inline time_t todayAt(int) { return SystemConfig::openTimeToday; }
} // namespace DailyBoundary

struct ReportingRuntime { uint32_t effectiveIntervalSec; };
namespace ReportingPolicyResolver {
inline ReportingRuntime resolveRuntime(int, time_t) { return {900}; }
} // namespace ReportingPolicyResolver

struct PowerManagerFake {
  static PowerManagerFake &instance() { static PowerManagerFake p; return p; }
  int effectiveConnectionMode() { return SystemConfig::CONNECTED; }
  int soc() { return 0; }
  bool downgradeActive() { return false; }
};
using PowerManager = PowerManagerFake;

struct SystemFake {
  bool updatesPending() { return false; }
  void reset(int) { lastActionTaken = RESET; ++actionCount; }
} System;

struct Ab1805Fake {
  void deepPowerDown() { lastActionTaken = DEEP_POWER_DOWN; ++actionCount; }
} ab1805;

struct TimeFake {
  time_t current = 0;
  time_t now() { return current; }
} Time;

struct LogFake {
  void info(const char *, ...) {}
} Log;

int state = 0;
const char *batteryTierShortName(BatteryTier) { return "H"; }
bool activeConnectAttemptWithinBudget() { return false; }
void setAppBreadcrumb(int) {}
BatteryTier currentBatteryTierForFailsafe() { return TIER_HEALTHY; }
bool connectivityFailsafeHasExternalPower() { return true; }
uint32_t connectivityFailsafeJitterSec(uint8_t) { return FAKE_JITTER_SEC; }
void persistConnectivityFailsafeState(uint8_t stage, time_t actionTime, bool) {
  persistedStage = stage;
  persistedAction = actionTime;
}

#include SUPERVISOR_BODY_H

namespace {

int failures = 0;

constexpr time_t OPENED_AT = 2000000;
constexpr time_t COOLDOWN_AND_JITTER = REAL_COOLDOWN_SEC + FAKE_JITTER_SEC;

struct Scenario {
  Clock::Openness openness;
  time_t now;
  time_t lastConnection;
  uint8_t stage;
  time_t lastAction;
};

Action run(const Scenario &s) {
  Clock::fakeOpenness = s.openness;
  Time.current = s.now;
  SystemConfig::lastConnection = s.lastConnection;
  SystemConfig::openTimeToday = OPENED_AT;
  RecoveryState::stage = s.stage;
  RecoveryState::lastAction = s.lastAction;
  lastActionTaken = NONE;
  actionCount = 0;
  persistedStage = 0;
  persistedAction = 0;
  connectivityFailsafeSupervisor();
  return lastActionTaken;
}

const char *name(Action a) {
  return a == RESET ? "System.reset" : a == DEEP_POWER_DOWN ? "deepPowerDown" : "no action";
}

void check(const char *what, const Scenario &s, Action want, uint8_t wantStage = 0) {
  const Action got = run(s);
  const bool stageOk = (want == NONE) || persistedStage == wantStage;
  if (got == want && stageOk && actionCount == (want == NONE ? 0 : 1)) {
    printf("  ok   %-64s -> %s\n", what, name(got));
  } else {
    printf("  FAIL %-64s got %s (stage %u), want %s\n", what, name(got),
           (unsigned)persistedStage, name(want));
    failures++;
  }
}

} // namespace

int main() {
  using Clock::Openness;
  printf("--- the real supervisor body (stale=%ld cooldown=%ld jitter<=%ld) ---\n",
         (long)REAL_STALE_SEC, (long)REAL_COOLDOWN_SEC, (long)REAL_JITTER_MAX_SEC);

  // The last connection is before today's opening, so the age starts at it.
  const time_t last = OPENED_AT - 3600;
  const time_t staleNow = OPENED_AT + REAL_STALE_SEC;
  const time_t stage2At = OPENED_AT + 100;
  const time_t stage3Now = stage2At + COOLDOWN_AND_JITTER;

  // 1. Unknown at stage 0: stage 2 acts.
  check("Unknown, stage 0, stale: stage 2 system reset",
        {Openness::Unknown, staleNow, last, 0, 0}, RESET, 2);

  // 2. Unknown at stage 2: the trap case.
  check("Unknown, stage 2, past cooldown + jitter: stage 3 deep power-down",
        {Openness::Unknown, stage3Now, last, 2, stage2At}, DEEP_POWER_DOWN, 3);
  check("Unknown, stage 2, one second inside the cooldown + jitter: nothing",
        {Openness::Unknown, stage3Now - 1, last, 2, stage2At}, NONE);
  check("Unknown, stage 2, one second past the cooldown, inside jitter: nothing",
        {Openness::Unknown, stage2At + REAL_COOLDOWN_SEC + 1, last, 2, stage2At}, NONE);
  check("Unknown, stage 3: nothing (no loop)",
        {Openness::Unknown, stage3Now + 86400, last, 3, stage3Now}, NONE);

  // 3. Closed: never acts.
  check("Closed, stage 0, stale: nothing",
        {Openness::Closed, staleNow, last, 0, 0}, NONE);
  check("Closed, stage 2, past cooldown + jitter: nothing",
        {Openness::Closed, stage3Now, last, 2, stage2At}, NONE);

  // 4. Open: unchanged.
  check("Open, stage 0, stale: stage 2 system reset",
        {Openness::Open, staleNow, last, 0, 0}, RESET, 2);
  check("Open, stage 2, past cooldown + jitter: stage 3 deep power-down",
        {Openness::Open, stage3Now, last, 2, stage2At}, DEEP_POWER_DOWN, 3);
  check("Open, stage 2, inside the cooldown + jitter: nothing",
        {Openness::Open, stage3Now - 1, last, 2, stage2At}, NONE);
  check("Open, stage 0, one second short of the stale threshold: nothing",
        {Openness::Open, staleNow - 1, last, 0, 0}, NONE);

  // 00:00-06:00 local: today's opening is in the future, so now <= ageBase.
  check("Unknown, before today's opening (now <= ageBase): nothing",
        {Openness::Unknown, OPENED_AT - 10, last, 0, 0}, NONE);
  check("Unknown, now == ageBase: nothing",
        {Openness::Unknown, OPENED_AT, last, 0, 0}, NONE);

  if (failures != 0) {
    printf("\nFAILED: %d check(s)\n", failures);
    return 1;
  }
  printf("\nconnectivity_failsafe_unknown_openness cases passed\n");
  return 0;
}
