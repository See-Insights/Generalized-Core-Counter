// WO-2026-10-25-001: logWakeSummary() prints the LED time left in seconds. signalLEDTimeRemaining() already returns
// seconds, so with 150 s left the log must read led=150s (the old code divided by 1000 again and printed led=0s).
// The real function is included by marker extraction (wake_summary_led_test.sh).
#include <cstdarg>
#include <cstdio>
#include <cstring>

static char logged[256];
static unsigned long ledRemainingSec = 150;

struct LogStub {
  void info(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(logged, sizeof(logged), fmt, ap);
    va_end(ap);
  }
} Log;
struct SensorStub { bool isSensorReady() { return true; } };
struct SensorManager { static SensorStub &instance() { static SensorStub s; return s; } };
namespace CurrentReadings { inline bool get_occupied() { return true; } }
inline bool isWithinOpenHours() { return true; }
inline unsigned long signalLEDTimeRemaining() { return ledRemainingSec; }

#include WAKE_SUMMARY_INC

static int check(unsigned long sec, const char *want) {
  ledRemainingSec = sec;
  logWakeSummary("timer");
  if (!strstr(logged, want)) {
    printf("FAIL: with %lu s left the log was \"%s\", wanted to contain \"%s\"\n", sec, logged, want);
    return 1;
  }
  return 0;
}

int main() {
  int bad = 0;
  bad += check(150, "led=150s");
  bad += check(0, "led=0s");
  bad += check(3600, "led=3600s");
  if (!bad) printf("wake_summary_led_test: ok (%s)\n", logged);
  return bad ? 1 : 0;
}
