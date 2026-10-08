// WO-2026-10-08-002 ruling 1: sensor.type is validated to exactly 1 in
// ConfigApply. Part of tests/sensor_type_source_test.sh.
//
// The real sensor.type block from Cloud::applySensorConfig() and the real
// Cloud::validateRange() are extracted from src/cloud/ConfigApply.cpp and run
// against a stub sensor store.

#include <cstdint>
#include <cstdio>

namespace {

int failures = 0;

struct TestLog {
  template <typename... Args>
  void info(const char *, Args...) {}
  template <typename... Args>
  void warn(const char *, Args...) {}
} Log;

uint8_t storedType = 1;
int injectedValue = 0;

namespace SystemConfig {
namespace SensorSettings {
uint8_t get_sensorType() { return storedType; }
void set_sensorType(uint8_t value) { storedType = value; }
} // namespace SensorSettings
} // namespace SystemConfig

struct Cloud {
  template <typename T>
  bool validateRange(T value, T min, T max, const char *name);
  bool applyType(bool &changed);
};

bool getMergedIntValue(int, int, const char *, int &out) {
  out = injectedValue;
  return true;
}

#include "validate_range.inc"

bool Cloud::applyType(bool &changed) {
  bool success = true;
  int sensorType = 0;
  int defaultSensor = 0;
  int deviceSensor = 0;
#include "type_block.inc"
  return success;
}

template bool Cloud::validateRange<int>(int, int, int, const char *);

bool apply(int value, bool &changed) {
  injectedValue = value;
  changed = false;
  Cloud cloud;
  return cloud.applyType(changed);
}

void check(const char *name, bool ok) {
  printf("  %s %s\n", ok ? "ok  " : "FAIL", name);
  if (!ok) {
    failures++;
  }
}

} // namespace

int main() {
  bool changed = false;

  storedType = 1;
  check("sensor.type 1 is accepted", apply(1, changed));

  const int rejected[] = {0, 2, 255};
  for (int value : rejected) {
    storedType = 1;
    char name[64];
    snprintf(name, sizeof name, "sensor.type %d is rejected, store unchanged", value);
    bool ok = !apply(value, changed);
    check(name, ok && storedType == 1 && !changed);
  }

  if (failures != 0) {
    printf("\nFAILED: %d check(s)\n", failures);
    return 1;
  }
  printf("\nsensor type validation test passed\n");
  return 0;
}
