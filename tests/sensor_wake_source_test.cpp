// WO-2026-10-14-004: wake source from the sensor. Real PIRSensor, activeSensor() and the
// wake lines of State_Sleep.cpp (extracted verbatim by the .sh) against fakes.
#include <cstdio>
#include <limits>
#include <string>
#include <vector>
struct { void error(const char *, ...) {} } Log;
#include "sensors/SensorFactory.h"
std::vector<std::string> g_calls;
void (*g_isr)() = nullptr;
unsigned long g_millis = 0;
const pin_t intPin = 13, disableModule = 12, ledPower = 11;
namespace CurrentReadings { bool get_occupied() { return false; } uint32_t get_totalOccupiedSeconds() { return 0; }
  uint16_t get_hourlyCount() { return 0; } uint16_t get_dailyCount() { return 0; } }  // reportFields() only
namespace SystemConfig { namespace SensorSettings { uint8_t get_sensorType() { return 1; } } }
struct SensorManager { static SensorManager &instance() { static SensorManager s; return s; }
  ISensor *_sensor = nullptr; ISensor *activeSensor(); };
#include "gen_active_sensor.inc"
struct NoWake : ISensor {  // takes every default, including wakeSource()
  bool loop() override { return false; } SensorData getData() const override { return SensorData(); }
  const char *getSensorType() const override { return "NONE"; } bool isReady() const override { return true; }
  void reset() override {} bool initializeHardware() override { return true; } };
struct OtherPin : NoWake { SensorWakeSource wakeSource() const override { return {7, RISING}; } };
struct SleepResult { pin_t pin; pin_t wakeupPin() const { return pin; } int error() const { return 0; } };
const int SYSTEM_ERROR_NONE = 0; struct Pass { bool pirWake; std::string reason; };
static Pass sleepPass(ISensor *sensor, pin_t gpio) {  // the wake lines of handleSleepingState()
  SensorManager::instance()._sensor = sensor;
  const pin_t BUTTON_PIN = 5;
  SleepResult result{gpio};
#include "gen_sensor_wake.inc"
#include "gen_wake_pin.inc"
#include "gen_wake_reason.inc"
#include "gen_pirwake.inc"
  return {pirWake, wakeReturnReason};
}
static int failures = 0;
static void check(const char *name, bool ok) { std::printf("%s: %s\n", ok ? "ok" : "FAIL", name); failures += !ok; }
static bool is(SensorWakeSource w, pin_t pin, InterruptMode e) { return w.pin == pin && w.edge == e; }

int main() {
  const pin_t timer = std::numeric_limits<pin_t>::max();  // wakeupPin() on a timer/RTC wake (0xFFFF)
  PIRSensor &pir = PIRSensor::instance();
  NoWake none; OtherPin other;
  check("PIR wake source is {intPin, RISING}", is(pir.wakeSource(), intPin, RISING));
  check("closed boot (null sensor, PIR configured) gives the same", is(SensorManager::instance().activeSensor()->wakeSource(), intPin, RISING));
  check("a sensor with no wake source gives PIN_INVALID", none.wakeSource().pin == PIN_INVALID);
  SensorWakeSource stored; stored.pin = timer;
  check("the wake-source pin can hold the timer-wake value 0xFFFF", stored.pin == timer);
  check("timer wake, PIR source: pirWake false, reason TIMER", !sleepPass(&pir, timer).pirWake && sleepPass(&pir, timer).reason == "TIMER");
  check("timer wake, no-source sensor: reason TIMER", sleepPass(&none, timer).reason == "TIMER");
  for (pin_t noPin : {timer, (pin_t)PIN_INVALID})
    check("no-source sensor, no-pin wake value: pirWake false", !sleepPass(&none, noPin).pirWake);
  check("PIR GPIO wake: pirWake true, reason PIR (also on a closed boot)",
        sleepPass(&pir, intPin).pirWake && sleepPass(&pir, intPin).reason == "PIR" && sleepPass(nullptr, intPin).pirWake);
  check("button wake: pirWake false, reason BUTTON", !sleepPass(&pir, 5).pirWake && sleepPass(&pir, 5).reason == "BUTTON");
  check("a sensor with another pin wakes on that pin", sleepPass(&other, 7).pirWake && !sleepPass(&other, intPin).pirWake);
  std::printf(failures ? "FAILED\n" : "sensor_wake_source_test passed\n");
  return failures != 0;
}
