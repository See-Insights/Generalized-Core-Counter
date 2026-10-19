// WO-2026-10-09-003: the modem standby latch clears once a day.
//
// The real daily-close block of State_Report.cpp (the `if (due) { ... }` that
// follows the occupancy flag take), the real latch owner power/ModemHealth.{h,cpp}
// (markModemUnstable(), maybeClearModemUnstable(), clearAtDailyClose(), the
// thresholds) and the real teardown trip check of State_Sleep.cpp are
// extracted byte-for-byte by the .sh wrapper into the .inc files included
// below. Only the Particle/SystemConfig surface is stubbed.
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>

struct SessionState {
  bool modemUnstable = false;
  bool modemStandbySuppressed = false;
  uint8_t modemUnstableReason = 0;
  uint8_t modemConnectTimeoutCount = 0;
  unsigned long lastTeardownEndMs = 0;
};
static SessionState session;

static std::vector<std::string> logs;
struct LogStub {
  void add(const char *fmt, va_list ap) {
    char buf[256];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    logs.push_back(buf);
  }
  void info(const char *fmt, ...) { va_list ap; va_start(ap, fmt); add(fmt, ap); va_end(ap); }
  void warn(const char *fmt, ...) { va_list ap; va_start(ap, fmt); add(fmt, ap); va_end(ap); }
};
static LogStub Log;

namespace Observability {
struct WakeCycleStats {
  enum class ConnectResult { NOT_ATTEMPTED, SUCCESS };
  ConnectResult connect_result = ConnectResult::SUCCESS;
};
static WakeCycleStats &cycleStats() {
  static WakeCycleStats s;
  return s;
}
}  // namespace Observability

namespace SystemConfig {
static time_t lastDailyCleanup = 0;
static void set_lastDailyCleanup(time_t t) { lastDailyCleanup = t; }
}  // namespace SystemConfig
namespace CurrentReadings {
static void set_occupied(bool) {}
static void set_occupancyStartTime(time_t) {}
}  // namespace CurrentReadings

#include "gen_sleep_defs.inc"

static int cleanupCalls = 0;
static void dailyCleanup() { cleanupCalls++; }

static void dailyCloseBlock(bool due) {
  const uint8_t close = 6;
  const bool wasOccupied = false;
  const time_t boundary = 1000;
  const time_t originalSessionStart = 0;
  const time_t now = 2000;
#include "gen_report_block.inc"
  (void)boundary; (void)originalSessionStart; (void)close; (void)wasOccupied;
}

static void teardownVerdict(unsigned long teardownElapsedMs) {
#include "gen_trip_check.inc"
}

static int failures = 0;
#define CHECK(name, cond) do { if (!(cond)) { printf("FAIL %s: %s\n", name, #cond); failures++; } } while (0)

static int countLogs(const char *line) {
  int n = 0;
  for (auto &l : logs) if (l == line) n++;
  return n;
}

static const char *kLine = "MODEM_POLICY: standby re-allowed at daily close; night modem-off decides";

static void latch() {
  session = SessionState();
  session.modemUnstable = true;
  session.modemStandbySuppressed = true;
  session.modemUnstableReason = 1;
  session.modemConnectTimeoutCount = 2;
  session.lastTeardownEndMs = 777;
  logs.clear();
}

int main() {
  // latched at the boundary: all four fields clear, one line, nothing else touched
  latch();
  dailyCloseBlock(true);
  CHECK("latched:modemUnstable", session.modemUnstable == false);
  CHECK("latched:standbySuppressed", session.modemStandbySuppressed == false);
  CHECK("latched:reason", session.modemUnstableReason == 0);
  CHECK("latched:timeoutCount", session.modemConnectTimeoutCount == 0);
  CHECK("latched:logOnce", countLogs(kLine) == 1);
  CHECK("latched:logTotal", logs.size() == 1);
  CHECK("latched:lastTeardownEndMs", session.lastTeardownEndMs == 777);

  // not latched at the boundary: nothing logged, fields as they were
  session = SessionState();
  session.modemStandbySuppressed = true;
  session.modemUnstableReason = 2;
  session.modemConnectTimeoutCount = 1;
  logs.clear();
  dailyCloseBlock(true);
  CHECK("unlatched:noLog", logs.empty());
  CHECK("unlatched:suppressedKept", session.modemStandbySuppressed == true);
  CHECK("unlatched:reasonKept", session.modemUnstableReason == 2);
  CHECK("unlatched:countKept", session.modemConnectTimeoutCount == 1);

  // due false: no clear, no log
  latch();
  dailyCloseBlock(false);
  CHECK("notdue:stillUnstable", session.modemUnstable == true);
  CHECK("notdue:stillSuppressed", session.modemStandbySuppressed == true);
  CHECK("notdue:reasonKept", session.modemUnstableReason == 1);
  CHECK("notdue:countKept", session.modemConnectTimeoutCount == 2);
  CHECK("notdue:noLog", logs.empty());

  // re-trip: after a clear, a slow teardown trips again through the existing check
  latch();
  dailyCloseBlock(true);
  logs.clear();
  teardownVerdict(12000UL);
  CHECK("retrip:unstable", session.modemUnstable == true);
  CHECK("retrip:suppressed", session.modemStandbySuppressed == true);
  CHECK("retrip:reason", session.modemUnstableReason == 1);
  CHECK("retrip:warn", countLogs("MODEM_POLICY: standby temporarily disabled reason=unstable_modem") == 1);

  // a healthy night (<= 5 s) after a clear leaves standby allowed
  latch();
  dailyCloseBlock(true);
  logs.clear();
  teardownVerdict(3000UL);
  CHECK("healthy:unstable", session.modemUnstable == false);
  CHECK("healthy:suppressed", session.modemStandbySuppressed == false);

  if (failures) {
    printf("standby_latch_daily_clear_test: %d FAILED\n", failures);
    return 1;
  }
  printf("standby_latch_daily_clear_test: all checks passed\n");
  return 0;
}
