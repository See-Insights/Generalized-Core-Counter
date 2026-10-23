#pragma once
// Host fakes for the sleep-execution equivalence harness (WO-2026-10-21-001): what the
// hibernate attempt, the ULP/STOP cascade and the standby decision call. Every call
// that changes the world goes to sim::events; the inputs are scripted in sim::exec.
#include <ctime>
#include <utility>
#include "Particle.h"
#include "state/State_Common.h"
#include "ResetCause.h"
#include "ThrashGuard.h"
#include "observability/WakeCycleStats.h"
#include "persist/CurrentReadings.h"
#include "persist/SystemConfig.h"
#include "persist/RecoveryState.h"
#include "power/ConnectivityPolicy.h"

#define PLATFORM_BORON 13
#define PLATFORM_ID PLATFORM_BORON
#define ALLOW_BLOCKING_SERIAL_WAITS 0
#define ENABLE_DIAGNOSTICS_PUBLISH_MODE 1
#ifndef CONNECTIVITY_FAILSAFE_TEST_MODE
#define CONNECTIVITY_FAILSAFE_TEST_MODE 0
#endif
#define Wiring_Watchdog 1
#ifndef ENABLE_SLEEP_TRACE
#define ENABLE_SLEEP_TRACE 0
#endif
#ifndef HAL_PLATFORM_CELLULAR
#define HAL_PLATFORM_CELLULAR 0
#endif

typedef uint16_t pin_t;
constexpr pin_t PIN_INVALID = 0xFFFF, BUTTON_PIN = 4, WAKEUP_PIN = 7;
enum InterruptMode { RISING = 1, FALLING = 2 };
enum PinMode { INPUT = 0, INPUT_PULLUP = 2 };
enum { LOW = 0, HIGH = 1 };
constexpr int NETWORK_INTERFACE_CELLULAR = 3;
enum class SystemSleepMode { ULTRA_LOW_POWER, STOP, HIBERNATE };
enum class SystemSleepNetworkFlag { INACTIVE_STANDBY };

struct SensorWakeSource { pin_t pin; InterruptMode edge; };

namespace sim {
struct Exec {
  struct Sleep { int err; pin_t pin; };
  bool hibEligible = false, hibPinHigh = true, staleClearOk = true, alarmOk = true, wdtStopOk = true;
  std::vector<Sleep> sleeps;
  size_t nextSleep = 0;
  pin_t sensorPin = 9;
  bool verbose = false, serialCfg = false, serialPre = false, reconnecting = false;
  int serialUpAfter = 0, serialPolls = 0;  // after wake: connected from poll N+1; -1 = never
  bool tdClears = false;  // the teardown fake clears the failure latch, as the real one does
  bool tdWait = false;    // the teardown fake is still waiting, so the handler returns before the sleep
  unsigned long freeHeap = 0;  // System.freeMemory() for the nightly heap guard (WO-2026-10-21-003)
};
// Inputs of the cloud-operations gate (WO-2026-10-21-002), scripted per pass by the gate runner.
struct Gate {
  bool queueEmpty = true, cfgSynced = true, outSynced = true, updatesPending = false, disconnectRequested = false;
  int queueDepth = 0;
};
inline Gate gate;
inline Exec exec;
}  // namespace sim

struct SystemSleepConfiguration {
  int m = -1, netIf = -1, netFlag = -1;
  long dur = -1;
  std::string gpios;
  SystemSleepConfiguration &mode(SystemSleepMode x) { m = (int)x; return *this; }
  SystemSleepConfiguration &gpio(pin_t p, InterruptMode e) {
    gpios += (gpios.empty() ? "" : ",") + std::to_string(p) + (e == FALLING ? "F" : "R");
    return *this;
  }
  SystemSleepConfiguration &network(int i, SystemSleepNetworkFlag f) { netIf = i; netFlag = (int)f; return *this; }
  SystemSleepConfiguration &duration(uint32_t ms) { dur = (long)ms; return *this; }
  std::string str() const {
    static const char *names[] = {"ULP", "STOP", "HIBERNATE"};
    return std::string("mode=") + (m < 0 ? "none" : names[m]) + " gpio=[" + gpios + "] net=" +
           (netIf < 0 ? std::string("none") : std::to_string(netIf) + ":" + std::to_string(netFlag)) + " dur=" +
           (dur < 0 ? std::string("none") : std::to_string(dur));
  }
};
struct SystemSleepResult {
  int e = 0;
  pin_t pin = PIN_INVALID;
  int error() const { return e; }
  pin_t wakeupPin() const { return pin; }
};
struct SystemStub {
  bool updatesPending() { return sim::gate.updatesPending; }
  SystemSleepResult sleep(const SystemSleepConfiguration &c) {
    auto &x = sim::exec;
    const sim::Exec::Sleep s = x.nextSleep < x.sleeps.size() ? x.sleeps[x.nextSleep++] : sim::Exec::Sleep{-1, PIN_INVALID};
    sim::emit("SLEEP %s -> err=%d pin=%u", c.str().c_str(), s.err, (unsigned)s.pin);
    sim::now += (s.err == 0 && s.pin == PIN_INVALID && c.dur > 0) ? (unsigned long)c.dur : 7UL;
    return SystemSleepResult{s.err, s.pin};
  }
  void reset(uint32_t cause) { sim::emit("RESET cause=%u", (unsigned)cause); }
  unsigned long freeMemory() { return sim::exec.freeHeap; }
};
inline SystemStub System;

struct SerialStub {
  bool isConnected() {
    auto &x = sim::exec;
    if (!x.reconnecting) return x.serialPre;
    x.serialPolls++;
    return x.serialUpAfter >= 0 && x.serialPolls > x.serialUpAfter;
  }
  void flush() { sim::emit("CALL Serial.flush"); }
  void begin(int baud) { sim::emit("CALL Serial.begin %d", baud); sim::exec.reconnecting = true; }
};
inline SerialStub Serial;
inline void delay(unsigned long ms) { sim::emit("CALL delay %lu", ms); sim::now += ms; }
inline void pinMode(pin_t p, int mode) { sim::emit("CALL pinMode %u %d", (unsigned)p, mode); }
inline int digitalRead(pin_t) { return sim::exec.hibPinHigh ? HIGH : LOW; }
inline void userSwitchISR() {}
inline bool userSwitchDetected = true;
inline void attachInterrupt(pin_t p, void (*)(), InterruptMode e) { sim::emit("CALL attachInterrupt %u %d", (unsigned)p, (int)e); }

struct AB1805Stub {
  bool interruptAtTime(time_t t) { sim::emit("AB1805 interruptAtTime %lld", (long long)t); return sim::exec.alarmOk; }
  bool stopWDT() { sim::emit("AB1805 stopWDT"); return sim::exec.wdtStopOk; }
  bool resumeWDT() { sim::emit("AB1805 resumeWDT"); return true; }
};
inline AB1805Stub ab1805;
struct WatchdogStub { bool started() { sim::emit("CALL Watchdog.started"); return true; } };
inline WatchdogStub Watchdog;
inline void pauseAwakeWatchdogForSleep(const char *ctx) { sim::emit("WATCHDOG pause %s", ctx); }
inline void restoreAwakeWatchdogAfterWake(const char *ctx) { sim::emit("WATCHDOG restore %s", ctx); }

inline bool shouldUseBoronRtcAlarmHibernate(uint32_t wakeIn, time_t &rtcNow) {
  rtcNow = 1700000000;
  sim::emit("HIB eligible? wakeIn=%u -> %d", (unsigned)wakeIn, (int)sim::exec.hibEligible);
  return sim::exec.hibEligible;
}
inline bool clearAb1805StaleAlarmInterrupts() { sim::emit("HIB clearStale"); return sim::exec.staleClearOk; }
namespace HibernateCycle {
inline void armForSleep(time_t rtcNow, uint32_t wakeIn) { sim::emit("HIB armForSleep %lld %u", (long long)rtcNow, (unsigned)wakeIn); }
inline void abandon() { sim::emit("HIB abandon"); }
}  // namespace HibernateCycle
namespace Connectivity {
inline bool isRadioPoweredOn() { return sim::world.radioOn; }
}
namespace PowerDiagnostics {
inline void logPowerState(const char *tag) { sim::emit("POWER %s", tag); }
inline void flushDiagBatch() { sim::emit("POWER flushDiagBatch"); }
}  // namespace PowerDiagnostics
namespace AwakeCycles {
inline void recordSleepReturn(bool ok) { sim::emit("AWAKE recordSleepReturn %d", (int)ok); }
}
struct CloudStub {
  struct LedgerSyncDiagnostics {
    uint16_t pendingCount;
    bool pendingStatusPublish, pendingDeviceStatusSync, pendingDeviceDataSync, statusInflight, dataInflight, syncing, inflight;
  };
  void logLedgerSleepState() { sim::emit("CLOUD logLedgerSleepState"); }
  void logLedgerSleepTimeoutState() { sim::emit("CLOUD logLedgerSleepTimeoutState"); }
  bool areLedgersSynced() { return sim::gate.cfgSynced; }
  bool hasPendingOutputLedgerSync() { return !sim::gate.outSynced; }
  LedgerSyncDiagnostics ledgerSyncDiagnostics() {
    const bool all = sim::gate.cfgSynced && sim::gate.outSynced;
    return {(uint16_t)(all ? 0 : 3), !sim::gate.outSynced, !sim::gate.cfgSynced, !sim::gate.outSynced,
            !sim::gate.cfgSynced, !sim::gate.outSynced, !all, !sim::gate.cfgSynced};
  }
};
struct PublishQueuePosix {
  static PublishQueuePosix &instance() { static PublishQueuePosix q; return q; }
  bool getCanSleep() { return sim::gate.queueEmpty; }
  size_t getNumEvents() { return (size_t)sim::gate.queueDepth; }
};
struct CloudHolder { using LedgerSyncDiagnostics = CloudStub::LedgerSyncDiagnostics; static CloudStub &instance() { static CloudStub c; return c; } };
using Cloud = CloudHolder;
struct MeasureStub { void noteWakeFromLowPowerSleep() { sim::emit("MEASURE noteWakeFromLowPowerSleep"); } };
inline MeasureStub measure;
struct SensorStub { SensorWakeSource wakeSource() { return {sim::exec.sensorPin, RISING}; } };
struct SensorManagerStub {
  static SensorManagerStub &instance() { static SensorManagerStub s; return s; }
  void onEnterSleep() { sim::emit("SENSOR onEnterSleep"); }
  SensorStub *activeSensor() { static SensorStub s; return &s; }
};
using SensorManager = SensorManagerStub;

// SleepTeardown stands in for state/SleepTeardown: it records what it is given and, when
// scripted, clears the failure latch the way the real one does once the modem is off.
namespace SleepTeardown {
enum class Result { WAIT, READY, ERROR };
inline void enter() {}
inline void guardLostTimestamp() {}
inline bool disconnectRequested() { return sim::gate.disconnectRequested; }
inline Result run(bool req, bool eff, bool &force, int &err) {
  sim::emit("TEARDOWN req=%d eff=%d force=%d err=%d", (int)req, (int)eff, (int)force, err);
  if (sim::exec.tdClears) { force = false; err = SYSTEM_ERROR_NONE; }
  return sim::exec.tdWait ? Result::WAIT : Result::READY;
}
}  // namespace SleepTeardown
