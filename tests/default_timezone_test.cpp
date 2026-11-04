// WO-2026-11-02-004 Amendment 2: the default time zone is the 42131 product
// default, fits the persisted buffer, and is accepted by the real setter.
// Links the real src/MyPersistentData.cpp. Part of default_timezone_test.sh.

#include "Particle.h"
#include "StorageHelperRK.h"

#include "Config.h"
#include "persist/SystemConfig.h"

#include "MyPersistentData.h"

#include <cstdio>
#include <cstring>
#include <string>

namespace StorageHelperRK {
std::vector<FlushRecord> flushCalls;
std::vector<size_t> validateSizes;
int initializeCalls = 0;
}  // namespace StorageHelperRK

// src/MyPersistentData.cpp forward-declares these; supply host definitions.
bool publishDiagnosticSafe(const char *, const char *, PublishFlags) { return true; }
bool isClockTrusted() { return true; }
namespace Clock {
bool isTimeValid() { return true; }
}

static int failures = 0;
#define CHECK(cond, msg)                                   \
  do {                                                     \
    if (!(cond)) {                                         \
      std::fprintf(stderr, "FAIL: %s\n", msg);             \
      ++failures;                                          \
    }                                                      \
  } while (0)

int main() {
  const char *tz = Config::DEFAULT_TIMEZONE;
  CHECK(std::string(tz) == "EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00",
        "DEFAULT_TIMEZONE is the 42131 product default");
  CHECK(std::strlen(tz) == 38, "default is 38 characters");
  CHECK(std::strlen(tz) < sizeof(sysStatusData::SysData::timeZoneStr),
        "default fits timeZoneStr including the NUL");

  SystemConfig::setup();
  CHECK(SystemConfig::set_timeZoneStr(tz), "set_timeZoneStr accepts the default");
  CHECK(std::string(SystemConfig::get_timeZoneStrCStr()) == tz,
        "stored value round-trips unchanged");

  if (failures) return 1;
  std::puts("default_timezone_test passed");
  return 0;
}
