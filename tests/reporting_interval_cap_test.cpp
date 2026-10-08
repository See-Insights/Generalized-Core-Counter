// WO-2026-10-08-001 item T: the reportingIntervalSec range in ConfigApply.
// Part of tests/reporting_interval_cap_test.sh.
//
// The real block from Cloud::applyTimingConfig() and the real
// Cloud::validateRange() are extracted from src/cloud/ConfigApply.cpp and run
// against a stub store with the real uint16_t width.

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

uint16_t storedInterval = 3600;
int injectedValue = 0;

namespace SystemConfig {
uint16_t get_reportingInterval() { return storedInterval; }
void set_reportingInterval(uint16_t value) { storedInterval = value; }
} // namespace SystemConfig

struct Cloud {
  template <typename T>
  bool validateRange(T value, T min, T max, const char *name);
  bool applyInterval(bool &changed);
};

bool getMergedIntValue(int, int, const char *, int &out) {
  out = injectedValue;
  return true;
}

#include "validate_range.inc"

bool Cloud::applyInterval(bool &changed) {
  bool success = true;
  int reportingInterval = 0;
  int defaultTiming = 0;
  int deviceTiming = 0;
#include "interval_block.inc"
  return success;
}

template bool Cloud::validateRange<int>(int, int, int, const char *);

// Returns whether the apply as a whole succeeded.
bool apply(int value, bool &changed) {
  injectedValue = value;
  changed = false;
  Cloud cloud;
  return cloud.applyInterval(changed);
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

  storedInterval = 3600;
  check("65535 is accepted", apply(65535, changed) && changed);
  check("65535 is stored without wrapping", storedInterval == 65535);

  storedInterval = 3600;
  check("65536 is rejected", !apply(65536, changed));
  check("65536 leaves the stored interval unchanged", storedInterval == 3600 && !changed);

  check("86400 is rejected", !apply(86400, changed));
  check("86400 leaves the stored interval unchanged", storedInterval == 3600 && !changed);

  check("300 is still accepted", apply(300, changed) && storedInterval == 300);
  check("299 is still rejected", !apply(299, changed) && storedInterval == 300);

  if (failures != 0) {
    printf("\nFAILED: %d check(s)\n", failures);
    return 1;
  }
  printf("\nreporting interval cap test passed\n");
  return 0;
}
