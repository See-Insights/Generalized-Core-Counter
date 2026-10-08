// WO-2026-10-08-002 fix A: sensor creation reads the sensor store
// (SensorSettings, /usr/sensor.dat), the field the ledgers write, and not the
// legacy sysStatus.sensorType. Part of tests/sensor_type_source_test.sh.
//
// The real SensorManager::initializeFromConfig() body, the real SensorType enum
// and the real LED-power block of setup() are extracted from src/. The real
// SensorDefinitions.h is included. The two stores are stubbed and are
// deliberately set to different values.

#include <cstdint>
#include <cstdio>

#define SENSORFACTORY_H  // the real header pulls in Particle; the enum is extracted

#include "sensor_type.inc"
#include "sensors/SensorDefinitions.h"

namespace {

int failures = 0;

struct TestLog {
  template <typename... Args>
  void info(const char *, Args...) {}
  template <typename... Args>
  void error(const char *, Args...) {}
} Log;

uint8_t sysStatusType = 0;
uint8_t sensorStoreType = 1;

namespace SystemConfig {
[[maybe_unused]] uint8_t get_sensorType() { return sysStatusType; }
namespace SensorSettings {
uint8_t get_sensorType() { return sensorStoreType; }
} // namespace SensorSettings
} // namespace SystemConfig

struct ISensor {
  bool initializeHardware() { return true; }
  const char *getSensorType() { return "stub"; }
  bool isReady() { return true; }
  bool usesInterrupt() { return true; }
};
ISensor stubSensor;

int createdType = -1;
struct SensorFactory {
  static ISensor *createSensor(SensorType type) {
    createdType = (int)type;
    return type == SensorType::PIR ? &stubSensor : nullptr;
  }
};

struct SensorManager {
  ISensor *_sensor = nullptr;
  void setSensor(ISensor *s) { _sensor = s; }
  void initializeFromConfig();
};

#include "initialize_from_config.inc"

const int ledPower = 1;
const int HIGH = 1;
const int LOW = 0;
int ledLevel = -1;
void pinMode(int, int) {}
void digitalWrite(int, int level) { ledLevel = level; }
const int OUTPUT = 1;

void ledPowerBlock() {
  pinMode(ledPower, OUTPUT);
#include "led_block.inc"
}

void check(const char *name, bool ok) {
  printf("  %s %s\n", ok ? "ok  " : "FAIL", name);
  if (!ok) {
    failures++;
  }
}

} // namespace

int main() {
  // Sensor store says PIR (1); the legacy sysStatus field says pressure (0).
  sysStatusType = 0;
  sensorStoreType = 1;

  SensorManager manager;
  createdType = -1;
  manager.initializeFromConfig();
  check("creation asks the factory for the sensor-store type (PIR)", createdType == 1);
  check("creation builds PIR when sysStatus.sensorType differs", manager._sensor == &stubSensor);

  ledLevel = -1;
  ledPowerBlock();
  check("setup's definition lookup uses the sensor-store value (PIR, LED low)", ledLevel == LOW);

  // The other way round: the legacy field says PIR, the store says pressure.
  sysStatusType = 1;
  sensorStoreType = 0;
  createdType = -1;
  SensorManager second;
  second.initializeFromConfig();
  check("creation follows the store, not sysStatus (store 0 -> factory 0)", createdType == 0);
  check("no sensor is built for a store value of 0", second._sensor == nullptr);
  ledLevel = -1;
  ledPowerBlock();
  check("setup's LED lookup follows the store (pressure, LED high)", ledLevel == HIGH);

  if (failures != 0) {
    printf("\nFAILED: %d check(s)\n", failures);
    return 1;
  }
  printf("\nsensor type source test passed\n");
  return 0;
}
