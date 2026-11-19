// WO-2026-11-19-001: State_Report builds the inputs, asks ConnectDecision::decide() and
// acts. The .sh extracts the real block of handleReportingState() byte-for-byte into
// gen_decision_block.inc (and fails with COPY_MISMATCH if it differs from src/); the
// real src/power/ConnectDecision.cpp is linked. Only the surface the block reads is
// stubbed: the session's two failure facts, millis(), Log, transitionTo(), the
// effective mode and the cadence verdict.
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "persist/SystemConfig.h"
#include "power/ConnectDecision.h"

enum State { NO_STATE, IDLE_STATE, CONNECTING_STATE };

struct SessionState {
  uint8_t modemConnectTimeoutCount = 0;
  uint32_t lastConnectFailMs = 0;
};
static SessionState session;

static uint32_t nowMs = 0;
static uint32_t millis() { return nowMs; }

static std::vector<std::string> logs;
struct LogStub {
  void info(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    logs.push_back(buf);
  }
};
static LogStub Log;

static State state = NO_STATE;
static std::string reason;
static int transitions = 0;
static void transitionTo(State next, const char *why) {
  state = next;
  reason = why;
  transitions++;
}

static uint8_t effectiveMode = SystemConfig::INTERMITTENT;
struct PowerManager {
  static PowerManager &instance() {
    static PowerManager p;
    return p;
  }
  uint8_t effectiveConnectionMode() const { return effectiveMode; }
};
static struct { bool cadenceDue = false; } reportingPolicy;

static void decisionBlock(bool serviceRequestTriggered, bool occupancyChangeTriggered, bool due,
                          bool forceConnectForLongTermWebhook) {
#include "gen_decision_block.inc"
}

static int failures = 0;
#define CHECK(label, cond)                                         \
  do {                                                             \
    if (!(cond)) {                                                 \
      printf("FAIL %s: %s\n", label, #cond);                       \
      failures++;                                                  \
    }                                                              \
  } while (0)

static void arm(uint8_t mode, bool cadenceDue, uint8_t failureCount, uint32_t failedAtMs, uint32_t now) {
  effectiveMode = mode;
  reportingPolicy.cadenceDue = cadenceDue;
  session.modemConnectTimeoutCount = failureCount;
  session.lastConnectFailMs = failedAtMs;
  nowMs = now;
  state = NO_STATE;
  reason.clear();
  transitions = 0;
  logs.clear();
}

static bool went(State s, const char *why) { return transitions == 1 && state == s && reason == why; }

int main() {
  const uint8_t I = SystemConfig::INTERMITTENT, K = SystemConfig::INTERMITTENT_KEEP_ALIVE;
  const uint32_t kMin = 60000UL, kHour = 60 * kMin;

  // Each input reaches the decision, and its answer is acted on.
  arm(I, false, 0, 0, 5 * kMin);
  decisionBlock(true, false, false, false);
  CHECK("wire: R2 the service request connects", went(CONNECTING_STATE, "service request") && logs.empty());
  arm(I, false, 0, 0, 5 * kMin);
  decisionBlock(false, true, false, false);
  CHECK("wire: R3 the occupancy change connects", went(CONNECTING_STATE, "occupancy change") && logs.empty());
  arm(I, false, 0, 0, 5 * kMin);
  decisionBlock(false, false, true, false);
  CHECK("wire: R4 the daily close connects", went(CONNECTING_STATE, "daily close") && logs.empty());
  arm(I, false, 0, 0, 5 * kMin);
  decisionBlock(false, false, false, true);
  CHECK("wire: R5 the webhook health check connects", went(CONNECTING_STATE, "webhook health check") && logs.empty());
  arm(K, false, 0, 0, 5 * kMin);
  decisionBlock(false, false, false, false);
  CHECK("wire: R7 the effective mode connects in keep alive", went(CONNECTING_STATE, "keep alive mode") && logs.empty());
  arm(I, true, 0, 0, 5 * kMin);
  decisionBlock(false, false, false, false);
  CHECK("wire: R9 the cadence verdict connects", went(CONNECTING_STATE, "boundary aligned") && logs.empty());
  arm(I, false, 0, 0, 5 * kMin);
  decisionBlock(false, false, false, false);
  CHECK("wire: R10 nothing due goes to Idle", went(IDLE_STATE, "not aligned") && logs.empty());

  // The failure facts: one timeout 10 min ago holds an occupancy change for the 50 min left.
  arm(K, false, 1, 5 * kHour, 5 * kHour + 10 * kMin);
  decisionBlock(false, true, false, false);
  CHECK("wire: hold: a failure 10 min ago holds the occupancy change", went(IDLE_STATE, "connect hold"));
  CHECK("wire: log: one CONNECT_HOLD line per held pass",
        logs.size() == 1 && logs[0] == "CONNECT_HOLD: remaining=3000 failures=1 trigger=occupancy change");
  arm(K, false, 1, 5 * kHour, 6 * kHour);
  decisionBlock(false, true, false, false);
  CHECK("wire: hold: the same change connects at +1 h", went(CONNECTING_STATE, "occupancy change"));
  CHECK("wire: log: no CONNECT_HOLD line once the hold has passed", logs.empty());

  // The count drives the length: three timeouts hold a keep-alive report for 4 h.
  arm(K, false, 3, 2 * kHour, 5 * kHour);
  decisionBlock(false, false, false, false);
  CHECK("wire: hold: three failures hold for 4 h", went(IDLE_STATE, "connect hold"));
  CHECK("wire: log: the line carries the count and the trigger",
        logs.size() == 1 && logs[0] == "CONNECT_HOLD: remaining=3600 failures=3 trigger=keep alive mode");
  arm(K, false, 3, 2 * kHour, 6 * kHour);
  decisionBlock(false, false, false, false);
  CHECK("wire: hold: three failures release at +4 h", went(CONNECTING_STATE, "keep alive mode") && logs.empty());

  // The clock is millis() minus the stamp, across the 49.7-day wrap.
  arm(K, false, 1, 0xFFFFF000UL, 0x00001000UL);
  decisionBlock(false, true, false, false);
  CHECK("wire: hold: the hold survives the millis() wrap", went(IDLE_STATE, "connect hold"));

  // The service request is acted on inside a hold, without a hold line.
  arm(K, false, 2, 5 * kHour, 5 * kHour + kMin);
  decisionBlock(true, true, false, false);
  CHECK("wire: service: the service request connects inside a hold",
        went(CONNECTING_STATE, "service request") && logs.empty());

  // No failure, no hold: a stamp of 0 and a young millis() hold nothing.
  arm(K, false, 0, 0, kMin);
  decisionBlock(false, true, false, false);
  CHECK("wire: reset: a count of 0 never holds", went(CONNECTING_STATE, "occupancy change") && logs.empty());

  if (failures) {
    printf("connect_hold_wiring_test: %d FAILED\n", failures);
    return 1;
  }
  printf("connect_hold_wiring_test: all checks passed\n");
  return 0;
}
