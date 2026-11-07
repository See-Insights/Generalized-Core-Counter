// WO-2026-11-05-005 (S3): the timed power-off at failsafe stage 3 and the ERROR_STATE hard recovery.
//
// Driven by tests/s3_timed_power_off_test.sh, which extracts the REAL powerOffForRecovery() helper, the REAL
// connectivityFailsafeSupervisor() body and the REAL State_Error.cpp "case 3:" block, and compiles them here
// against fakes that record every call in order. System.sleep() never returns when it succeeds (the board is
// dark), so the fake throws PowerCut for success and returns an error otherwise.

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>

#ifndef POWER_OFF_CONST_H
#error "POWER_OFF_CONST_H must hold the real kPowerOffRecoverySec line"
#endif
#ifndef HELPER_BODY_H
#error "HELPER_BODY_H must name the extracted helper"
#endif

enum BatteryTier { TIER_HEALTHY, TIER_CONSERVING, TIER_CRITICAL, TIER_SURVIVAL };
enum { FIRMWARE_UPDATE_STATE = 99 };
enum { BREADCRUMB_CONNECTIVITY_FAILSAFE_HARD = 1, RESET_CAUSE_CONNECTIVITY_FAILSAFE = 2, RESET_CAUSE_ERROR_STATE_SOFT = 3 };
enum { SYSTEM_ERROR_NONE = 0, SYSTEM_ERROR_ABORTED = -170, SYSTEM_ERROR_NOT_SUPPORTED = -160 };

namespace {
std::vector<std::string> ev;
constexpr int kPowerCut = 1; // not a Device OS result: System.sleep() never returns
int sleepError = kPowerCut;
struct PowerCut {};   // the rail was cut: nothing after System.sleep() ever runs
struct ActionStop {}; // System.reset(): the board restarts
std::string fmt(const char *f, ...) {
  char b[160];
  va_list a;
  va_start(a, f);
  vsnprintf(b, sizeof b, f, a);
  va_end(a);
  return b;
}
} // namespace

enum class SystemSleepMode { POWER_OFF, HIBERNATE };
constexpr int FALLING = 2, WAKEUP_PIN = 1, BUTTON_PIN = 2; // so a GPIO-wake mutant compiles and fails on its behaviour
struct SystemSleepConfiguration {
  SystemSleepMode m = SystemSleepMode::HIBERNATE;
  unsigned long ms = 0;
  int gpios = 0;
  SystemSleepConfiguration &mode(SystemSleepMode v) { m = v; return *this; }
  SystemSleepConfiguration &duration(unsigned long v) { ms = v; return *this; }
  SystemSleepConfiguration &gpio(int, int) { ++gpios; return *this; }
};
struct SystemSleepResult {
  int e;
  int error() const { return e; }
};

namespace Clock {
enum class Openness : uint8_t { Open, Closed, Unknown };
inline bool isTimeValid() { return true; }
inline Openness openness() { return Openness::Open; }
} // namespace Clock

namespace ConnectivityPolicy {
constexpr time_t CONNECTIVITY_FAILSAFE_STALE_SEC = 3 * 3600;
constexpr time_t CONNECTIVITY_FAILSAFE_COOLDOWN_SEC = 6 * 3600;
} // namespace ConnectivityPolicy

namespace RecoveryState {
inline uint8_t get_connectivityRecoveryStage() { return 2; }
inline time_t get_lastConnectivityRecoveryAction() { return 100; }
inline void set_connectivityRecoveryStage(uint8_t v) { ev.push_back(fmt("stage=%u", (unsigned)v)); }
inline void set_powerOffRequested(uint8_t v) { ev.push_back(fmt("byte=0x%02X", (unsigned)v)); }
inline int get_alertCode() { return 15; }
} // namespace RecoveryState

namespace SystemConfig {
enum { CONNECTED, DISCONNECTED };
inline int get_openTime() { return 6; }
inline time_t get_lastConnection() { return 1000; }
inline void flushNow() { ev.push_back("flushNow"); }
} // namespace SystemConfig

namespace DailyBoundary {
inline time_t todayAt(int) { return 2000; }
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
  SystemSleepResult sleep(const SystemSleepConfiguration &c) {
    ev.push_back(fmt("sleep mode=%s ms=%lu gpios=%d", c.m == SystemSleepMode::POWER_OFF ? "POWER_OFF" : "other", c.ms, c.gpios));
    if (sleepError == kPowerCut) {
      throw PowerCut{};
    }
    return {sleepError};
  }
  void reset(int cause) { ev.push_back(fmt("reset=%d", cause)); throw ActionStop{}; }
} System;

struct ExternalWatchdogFake {
  int stop() { ev.push_back("wd.stop"); return SYSTEM_ERROR_NONE; }
  int start() { ev.push_back("wd.start"); return SYSTEM_ERROR_NONE; }
} ExternalWatchdog;

struct TimeFake { time_t now() { return 10 * 86400; } } Time;

struct LogFake {
  void info(const char *f, ...) { ev.push_back(std::string("log:") + f); }
  void error(const char *f, ...) { ev.push_back(std::string("log:") + f); }
} Log;

int state = 0;
unsigned long millis() { return 1000; }
unsigned long resetTimer = 0;
unsigned long resetWait = 0;
const char *batteryTierShortName(BatteryTier) { return "H"; }
bool activeConnectAttemptWithinBudget() { return false; }
void setAppBreadcrumb(int) {}
BatteryTier currentBatteryTierForFailsafe() { return TIER_HEALTHY; }
bool connectivityFailsafeHasExternalPower() { return true; }
uint32_t connectivityFailsafeJitterSec(uint8_t) { return 0; }
void persistConnectivityFailsafeState(uint8_t stage, time_t, bool) { ev.push_back(fmt("persist=%u", (unsigned)stage)); }
void flushAllBeforeReset() { ev.push_back("flush"); }

#include "HibernateWakeDiagnostics.h"
#include POWER_OFF_CONST_H
#include HELPER_BODY_H
#include SUPERVISOR_BODY_H

void errorCase3() {
  switch (3) {
#include CASE3_BODY_H
  }
}

namespace {
int failures = 0;
using Events = std::vector<std::string>;

std::string join(const Events &e) {
  std::string s;
  for (const auto &x : e) s += "\n      " + x;
  return s;
}

template <typename F> bool run(int err, F f) {
  ev.clear();
  sleepError = err;
  bool cut = false;
  try { f(); } catch (const PowerCut &) { cut = true; } catch (const ActionStop &) {}
  return cut;
}

void expect(const char *what, const Events &want) {
  if (ev == want) {
    printf("  ok   %s\n", what);
  } else {
    printf("  FAIL %s\n    want:%s\n    got:%s\n", what, join(want).c_str(), join(ev).c_str());
    failures++;
  }
}

const std::string kSleep = fmt("sleep mode=POWER_OFF ms=%lu gpios=0", (unsigned long)kPowerOffRecoverySec * 1000UL);
const Events kWdThenSleep = {"wd.stop", kSleep};            // power cut: stopped, never restarted
const Events kReturned = {"wd.stop", kSleep, "wd.start"};   // any return: restarted straight after
const std::string kHelperLog = "log:PowerOff: System.sleep returned err=%d";
const std::string kStage3Log = "log:Failsafe: stage=3 action=deep-powerdown age=%lds";
const std::string kErrorLog = "log:Executing deep power down from ERROR_STATE (alert=%d)";
} // namespace

int main() {
  printf("--- helper: POWER_OFF, the real off time, RTC wake only ---\n");
  run(kPowerCut, powerOffForRecovery);
  expect("power cut: watchdog stopped, one POWER_OFF sleep of kPowerOffRecoverySec, no GPIO wake, nothing after", kWdThenSleep);
  if (kPowerOffRecoverySec == 0) { printf("  FAIL off time is zero\n"); failures++; }
  for (int err : {SYSTEM_ERROR_NONE, SYSTEM_ERROR_ABORTED, SYSTEM_ERROR_NOT_SUPPORTED}) {
    run(err, powerOffForRecovery);
    Events want = kReturned;
    want.push_back(kHelperLog);
    expect(err == SYSTEM_ERROR_NONE ? "returned success: watchdog restarted, logs and returns (never resets inside)"
                                    : "error: watchdog restarted, logs and returns (never resets inside)", want);
  }

  printf("--- failsafe stage 3 ---\n");
  const Events stage3Pre = {"persist=3", kStage3Log, "flush"};
  Events stage3Cut = stage3Pre;
  stage3Cut.insert(stage3Cut.end(), kWdThenSleep.begin(), kWdThenSleep.end());
  run(kPowerCut, connectivityFailsafeSupervisor);
  expect("power cut: record, log, flush, then exactly one sleep; nothing follows", stage3Cut);
  for (int err : {SYSTEM_ERROR_NONE, SYSTEM_ERROR_ABORTED, SYSTEM_ERROR_NOT_SUPPORTED}) {
    run(err, connectivityFailsafeSupervisor);
    Events want = stage3Pre;
    want.insert(want.end(), kReturned.begin(), kReturned.end());
    for (const char *s : {kHelperLog.c_str(), "byte=0x00", "stage=2", "flushNow"}) want.push_back(s);
    want.push_back("reset=2");
    expect("return: clear evidence, downgrade, flush, then System.reset(failsafe cause)", want);
  }

  printf("--- ERROR_STATE hard recovery ---\n");
  const Events errPre = {kErrorLog, "byte=0xA5", "flush"};
  Events errCut = errPre;
  errCut.insert(errCut.end(), kWdThenSleep.begin(), kWdThenSleep.end());
  run(kPowerCut, errorCase3);
  expect("power cut: log, evidence, flush, then exactly one sleep; nothing follows", errCut);
  for (int err : {SYSTEM_ERROR_NONE, SYSTEM_ERROR_ABORTED, SYSTEM_ERROR_NOT_SUPPORTED}) {
    run(err, errorCase3);
    Events want = errPre;
    want.insert(want.end(), kReturned.begin(), kReturned.end());
    for (const char *s : {kHelperLog.c_str(), "byte=0x00", "flushNow"}) want.push_back(s);
    want.push_back("reset=3");
    expect("return: clear evidence, flush, then System.reset(ERROR_STATE_SOFT)", want);
  }

  if (failures != 0) {
    printf("\nFAILED: %d check(s)\n", failures);
    return 1;
  }
  printf("\ns3_timed_power_off cases passed\n");
  return 0;
}
