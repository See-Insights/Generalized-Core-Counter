// WO-2026-10-15-001: PIRSensor::resetTotals() against recording CurrentReadings fakes.
// argv[1] = file listing the central resetEverything() sensor-total setters, one per line.
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "sensors/PIRSensor.h"

std::vector<std::string> g_calls;
void (*g_isr)() = nullptr;
unsigned long g_millis = 0;
const pin_t intPin = 13, disableModule = 12, ledPower = 11;

namespace CurrentReadings {
bool get_occupied() { return false; }
uint32_t get_totalOccupiedSeconds() { return 0; }
uint16_t get_hourlyCount() { return 0; }
uint16_t get_dailyCount() { return 0; }
void set_hourlyCount(uint16_t v) { g_calls.push_back("set_hourlyCount(" + std::to_string(v) + ")"); }
void set_dailyCount(uint16_t v) { g_calls.push_back("set_dailyCount(" + std::to_string(v) + ")"); }
void set_occupied(bool v) { g_calls.push_back(std::string("set_occupied(") + (v ? "true" : "false") + ")"); }
void set_lastOccupancyEvent(uint32_t v) { g_calls.push_back("set_lastOccupancyEvent(" + std::to_string(v) + ")"); }
void set_occupancyStartTime(time_t v) { g_calls.push_back("set_occupancyStartTime(" + std::to_string(v) + ")"); }
void set_totalOccupiedSeconds(uint32_t v) { g_calls.push_back("set_totalOccupiedSeconds(" + std::to_string(v) + ")"); }
}  // namespace CurrentReadings

static int failures = 0;
static void check(const char *name, bool ok) {
  std::printf("%s: %s\n", ok ? "ok" : "FAIL", name);
  if (!ok) ++failures;
}
static void dump(const char *label) {
  std::printf("%s:", label);
  for (const auto &c : g_calls) std::printf(" %s", c.c_str());
  std::printf("\n");
}

int main(int argc, char **argv) {
  std::vector<std::string> central;
  std::ifstream in(argc > 1 ? argv[1] : "");
  for (std::string l; std::getline(in, l);) central.push_back(l);

  PIRSensor &pir = PIRSensor::instance();
  ISensor &sensor = pir;
  pir.initializeHardware();

  g_calls.clear();
  sensor.resetTotals(ResetPeriod::Hourly);
  dump("Hourly");
  check("Hourly records exactly [hourly 0]", g_calls == std::vector<std::string>{"set_hourlyCount(0)"});

  g_calls.clear();
  sensor.resetTotals(ResetPeriod::Daily);
  dump("Daily");
  check("Daily records the six setters in order",
        g_calls == std::vector<std::string>{"set_hourlyCount(0)", "set_dailyCount(0)", "set_occupied(false)",
                                            "set_lastOccupancyEvent(0)", "set_occupancyStartTime(0)",
                                            "set_totalOccupiedSeconds(0)"});
  check("central list has six setters", central.size() == 6);
  check("Daily equals central resetEverything() order", g_calls == central);

  for (ResetPeriod p : {ResetPeriod::Hourly, ResetPeriod::Daily}) {
    g_millis += 1000;
    g_isr();
    sensor.resetTotals(p);
    check(p == ResetPeriod::Hourly ? "ISR-pending survives Hourly reset" : "ISR-pending survives Daily reset",
          pir.loop());
  }

  struct Plain : ISensor {  // takes every default
    bool loop() override { return false; }
    SensorData getData() const override { return SensorData(); }
    const char *getSensorType() const override { return "X"; }
    bool isReady() const override { return true; }
    void reset() override {}
    bool initializeHardware() override { return true; }
  } plain;
  g_calls.clear();
  plain.resetTotals(ResetPeriod::Daily);
  check("default resetTotals does nothing", g_calls.empty());

  if (failures) {
    std::printf("pir_sensor_reset_totals_test FAILED (%d)\n", failures);
    return 1;
  }
  std::printf("pir_sensor_reset_totals_test passed\n");
  return 0;
}
