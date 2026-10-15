// WO-2026-10-15-002: the daily and hourly resets, end to end on the host. Real PIRSensor and real
// activeSensor(), plus dailyCleanup(), the hourly block of handleReportingState(), the central
// currentStatusData::resetEverything(bool) and its CurrentReadings wrapper (extracted verbatim by the
// .sh), all recording into one list of setter calls.
#include <cstdio>
#include <string>
#include <vector>
struct { void info(const char *, ...) {} void error(const char *, ...) {} } Log;
#include "persist/CurrentReadings.h"
#include "persist/SystemConfig.h"
#include "sensors/SensorFactory.h"

std::vector<std::string> g_calls;
void (*g_isr)() = nullptr;
unsigned long g_millis = 0;
const pin_t intPin = 13, disableModule = 12, ledPower = 11;

static std::string num(long v) { return std::to_string(v); }
struct Rec {
  void set_lastCountTime(time_t v) { g_calls.push_back("set_lastCountTime(" + num(v) + ")"); }
  void set_resetCount(uint8_t v) { g_calls.push_back("set_resetCount(" + num(v) + ")"); }
  void set_hourlyCount(uint16_t v) { g_calls.push_back("set_hourlyCount(" + num(v) + ")"); }
  void set_dailyCount(uint16_t v) { g_calls.push_back("set_dailyCount(" + num(v) + ")"); }
  void set_occupied(bool v) { g_calls.push_back(std::string("set_occupied(") + (v ? "true" : "false") + ")"); }
  void set_lastOccupancyEvent(uint32_t v) { g_calls.push_back("set_lastOccupancyEvent(" + num(v) + ")"); }
  void set_occupancyStartTime(time_t v) { g_calls.push_back("set_occupancyStartTime(" + num(v) + ")"); }
  void set_totalOccupiedSeconds(uint32_t v) { g_calls.push_back("set_totalOccupiedSeconds(" + num(v) + ")"); }
};
struct currentStatusData : Rec { void resetEverything(bool includeSensorTotals); };
static currentStatusData current;
static Rec sysStatus, facade;
static bool isClockTrusted() { return true; }

namespace CurrentReadings {
bool get_occupied() { return false; }
uint32_t get_totalOccupiedSeconds() { return 0; }
uint16_t get_hourlyCount() { return 0; }
uint16_t get_dailyCount() { return 0; }
void set_hourlyCount(uint16_t v) { facade.set_hourlyCount(v); }
void set_dailyCount(uint16_t v) { facade.set_dailyCount(v); }
void set_occupied(bool v) { facade.set_occupied(v); }
void set_lastOccupancyEvent(uint32_t v) { facade.set_lastOccupancyEvent(v); }
void set_occupancyStartTime(time_t v) { facade.set_occupancyStartTime(v); }
void set_totalOccupiedSeconds(uint32_t v) { facade.set_totalOccupiedSeconds(v); }
}  // namespace CurrentReadings

#include "gen_central_reset.inc"
namespace CurrentReadings {
#include "gen_wrapper.inc"
}

static uint8_t g_mode = SystemConfig::COUNTING;
static uint8_t g_sensorType = 1;
namespace SystemConfig {
uint8_t get_sensorMode() { return g_mode; }
namespace SensorSettings { uint8_t get_sensorType() { return g_sensorType; } }
}  // namespace SystemConfig

struct SensorManager { static SensorManager &instance() { static SensorManager s; return s; }
  ISensor *_sensor = nullptr; ISensor *activeSensor(); };
#include "gen_active_sensor.inc"

static bool g_connected = false;
static int g_resyncs = 0;
struct { bool connected() { return g_connected; } } Particle;
enum PublishFlags { PRIVATE, PUBLIC };
static bool publishDiagnosticSafe(const char *, const char *, PublishFlags) { return true; }
static void requestClockResync(const char *) { ++g_resyncs; }
#include "gen_daily_cleanup.inc"

static void hourlyReset() {  // the hourly block of handleReportingState()
#include "gen_hourly_block.inc"
}

struct Spy : ISensor {  // records the hook and nothing else
  bool loop() override { return false; }
  SensorData getData() const override { return SensorData(); }
  const char *getSensorType() const override { return "SPY"; }
  bool isReady() const override { return true; }
  void reset() override {}
  bool initializeHardware() override { return true; }
  void resetTotals(ResetPeriod p) override { g_calls.push_back(p == ResetPeriod::Daily ? "hook(Daily)" : "hook(Hourly)"); }
};

using Calls = std::vector<std::string>;
static int failures = 0;
static void check(const char *name, bool ok) { std::printf("%s: %s\n", ok ? "ok" : "FAIL", name); failures += !ok; }
static void dump(const char *label) {
  std::printf("%s:", label);
  for (const auto &c : g_calls) std::printf(" %s", c.c_str());
  std::printf("\n");
}
template <typename F> static Calls record(const char *label, F f) {
  g_calls.clear();
  f();
  dump(label);
  return g_calls;
}

int main() {
  PIRSensor &pir = PIRSensor::instance();
  Spy spy;
  const Calls today = {"set_lastCountTime(1000)", "set_resetCount(0)", "set_hourlyCount(0)", "set_dailyCount(0)",
                       "set_occupied(false)", "set_lastOccupancyEvent(0)", "set_occupancyStartTime(0)",
                       "set_totalOccupiedSeconds(0)"};

  check("resetEverything() default clears all eight, in order", record("resetEverything()", [] { CurrentReadings::resetEverything(); }) == today);
  check("resetEverything(true) clears all eight, in order", record("resetEverything(true)", [] { CurrentReadings::resetEverything(true); }) == today);
  check("resetEverything(false) stamps lastCountTime and clears resetCount only",
        record("resetEverything(false)", [] { CurrentReadings::resetEverything(false); }) == Calls(today.begin(), today.begin() + 2));

  for (bool connected : {false, true}) {
    g_connected = connected;
    g_resyncs = 0;
    SensorManager::instance()._sensor = &pir;
    check(connected ? "daily (connected, PIR): exactly today's eight setters in today's order"
                    : "daily (not connected, PIR): exactly today's eight setters in today's order",
          record("daily PIR", [] { dailyCleanup(); }) == today);
    check("the daily clock resync request still follows the connection", g_resyncs == (connected ? 1 : 0));
    SensorManager::instance()._sensor = &spy;
    check("daily with a sensor of its own: central two, then its Daily hook, nothing else",
          record("daily spy", [] { dailyCleanup(); }) == Calls{"set_lastCountTime(1000)", "set_resetCount(0)", "hook(Daily)"});
  }
  g_connected = false;

  SensorManager::instance()._sensor = nullptr;
  for (uint8_t type : {(uint8_t)1, (uint8_t)99}) {  // configured PIR; an unimplemented type falls back to PIR
    g_sensorType = type;
    check(type == 1 ? "null sensor (closed boot), PIR configured: Daily reaches PIR's hook"
                    : "null sensor, unimplemented type: Daily reaches PIR's hook",
          record("daily null", [] { dailyCleanup(); }) == today);
    g_mode = SystemConfig::COUNTING;
    check(type == 1 ? "null sensor, PIR configured: Hourly reaches PIR's hook"
                    : "null sensor, unimplemented type: Hourly reaches PIR's hook",
          record("hourly null COUNTING", [] { hourlyReset(); }) == Calls{"set_hourlyCount(0)"});
  }
  g_sensorType = 1;

  for (ISensor *s : {static_cast<ISensor *>(&pir), static_cast<ISensor *>(nullptr)}) {
    SensorManager::instance()._sensor = s;
    g_mode = SystemConfig::COUNTING;
    check("hourly COUNTING clears only hourlyCount", record("hourly COUNTING", [] { hourlyReset(); }) == Calls{"set_hourlyCount(0)"});
    g_mode = SystemConfig::OCCUPANCY;
    check("hourly OCCUPANCY clears nothing", record("hourly OCCUPANCY", [] { hourlyReset(); }).empty());
  }
  SensorManager::instance()._sensor = &spy;
  g_mode = SystemConfig::COUNTING;
  check("hourly COUNTING calls the sensor's Hourly hook once", record("hourly spy", [] { hourlyReset(); }) == Calls{"hook(Hourly)"});
  g_mode = SystemConfig::OCCUPANCY;
  check("hourly OCCUPANCY does not call the hook", record("hourly spy OCCUPANCY", [] { hourlyReset(); }).empty());

  std::printf(failures ? "reset_wiring_test FAILED\n" : "reset_wiring_test passed\n");
  return failures != 0;
}
