// WO-2026-10-14-002: lifecycle of the real PIRSensor (src/sensors/PIRSensor.h/.cpp)
// against recording fakes (tests/stubs/pir_lifecycle/Particle.h). The start operation
// is called by its interface name, initializeHardware().
#include <cstdio>
#include <string>
#include <vector>

#include "sensors/PIRSensor.h"

std::vector<std::string> g_calls;
void (*g_isr)() = nullptr;
unsigned long g_millis = 0;

const pin_t intPin = 13;
const pin_t disableModule = 12;
const pin_t ledPower = 11;

// PIRSensor::reportFields() reads these; the lifecycle test never calls it.
namespace CurrentReadings {
bool get_occupied() { return false; }
uint32_t get_totalOccupiedSeconds() { return 0; }
uint16_t get_hourlyCount() { return 0; }
uint16_t get_dailyCount() { return 0; }
}  // namespace CurrentReadings

namespace {
int failures = 0;

void check(const char *name, bool ok) {
  std::printf("%s: %s\n", ok ? "ok" : "FAIL", name);
  if (!ok) ++failures;
}

void dump(const char *label) {
  std::printf("%s:\n", label);
  for (const auto &c : g_calls) std::printf("  %s\n", c.c_str());
}

bool sameCalls(const std::vector<std::string> &want) { return g_calls == want; }
}  // namespace
int main() {
  PIRSensor &pir = PIRSensor::instance();

  g_calls.clear();
  bool ok = pir.initializeHardware();
  dump("initializeHardware");
  check("initializeHardware returns true", ok);
  check("initializeHardware call order",
        sameCalls({"pinMode(intPin,INPUT_PULLDOWN)", "pinMode(disableModule,OUTPUT)",
                   "pinMode(ledPower,OUTPUT)", "digitalWrite(disableModule,LOW)",
                   "digitalWrite(ledPower,LOW)", "attachInterrupt(intPin,RISING)"}));
  check("ready after initializeHardware", pir.isReady());
  check("ISR captured", g_isr != nullptr);

  g_millis = 1000;
  check("no motion before ISR", !pir.loop());
  g_isr();
  check("ISR sets motion flag: loop() reports motion", pir.loop());
  check("getData reports new data", pir.getData().hasNewData);

  g_calls.clear();
  pir.onSleep();
  dump("onSleep");
  check("onSleep call order",
        sameCalls({"detachInterrupt(intPin)", "digitalWrite(disableModule,HIGH)",
                   "digitalWrite(ledPower,HIGH)"}));
  check("not ready after onSleep", !pir.isReady());

  g_calls.clear();
  ok = pir.onWake();
  dump("onWake");
  check("onWake returns true", ok);
  check("onWake call order",
        sameCalls({"pinMode(disableModule,OUTPUT)", "pinMode(ledPower,OUTPUT)",
                   "pinMode(intPin,INPUT_PULLDOWN)", "digitalWrite(disableModule,LOW)",
                   "digitalWrite(ledPower,LOW)", "attachInterrupt(intPin,RISING)"}));
  check("ready after onWake", pir.isReady());

  g_millis = 2000;
  g_isr();
  check("ISR re-attached on wake sets motion flag", pir.loop());

  if (failures) {
    std::printf("pir_sensor_lifecycle_test FAILED (%d)\n", failures);
    return 1;
  }
  std::printf("pir_sensor_lifecycle_test passed\n");
  return 0;
}
