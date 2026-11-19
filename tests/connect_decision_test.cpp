// WO-2026-11-19-001: the one connect decision, power/ConnectDecision, on the host.
//
// The real src/power/ConnectDecision.cpp is linked; nothing here re-implements it.
//
// "char:"    the REPORTING chain as it was at eb398b8, with failureCount = 0:
//            - the rows of docs/reviews/2026-11-19-one-place-map.md section 1.3 that the
//              decision owns (R2-R5, R7, R9, R10), each with its "Outcome today" string.
//              R6/R8 (the 30 s defer) are deleted; R0, R1 and R1b stay in State_Report.cpp
//              and are not inputs (their exits are driven by occupancy_report_by_mode_test).
//            - every combination of the five triggers and four modes against oldChain(),
//              a transcription of State_Report.cpp:220-282 at eb398b8 without the defer arms.
// "seq:"     the hold is 1, 2, 4, 6 and 6 h after failures 1-5, and never more than 6 h.
// "hold:"    inside the hold every trigger but the service request waits; once it has
//            passed each gives its own CONNECT.
// "service:" a service request connects during a hold.
// "reset:"   with failureCount = 0 there is no hold, whatever the time since the failure.
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "persist/SystemConfig.h"
#include "power/ConnectDecision.h"
#include "power/ConnectivityPolicy.h"

using ConnectDecision::Action;
using ConnectDecision::Inputs;
using ConnectDecision::Result;

static int failures = 0;
#define CHECK(label, cond)                                         \
  do {                                                             \
    if (!(cond)) {                                                 \
      printf("FAIL %s: %s\n", label, #cond);                       \
      failures++;                                                  \
    }                                                              \
  } while (0)

static const uint32_t kHour = 60UL * 60UL * 1000UL;
static const uint8_t kModes[] = {SystemConfig::CONNECTED, SystemConfig::INTERMITTENT, SystemConfig::DISCONNECTED,
                                 SystemConfig::INTERMITTENT_KEEP_ALIVE};
static const char *const kModeNames[] = {"CONNECTED", "INTERMITTENT", "DISCONNECTED", "KEEP_ALIVE"};

static bool is(const Result &r, Action action, const char *reason) {
  return r.action == action && r.reason != nullptr && strcmp(r.reason, reason) == 0;
}

// State_Report.cpp:220-282 at eb398b8, branch for branch, without the two defer arms (R6, R8).
struct Outcome {
  bool connect;
  const char *reason;
};
static Outcome oldChain(bool serviceRequestTriggered, bool occupancyChangeTriggered, bool due,
                        bool forceConnectForLongTermWebhook, uint8_t effectiveConnectionMode, bool cadenceDue) {
  if (serviceRequestTriggered) {
    return {true, "service request"};
  } else if (occupancyChangeTriggered) {
    return {true, "occupancy change"};
  } else if (due) {
    return {true, "daily close"};
  } else if (forceConnectForLongTermWebhook) {
    return {true, "webhook health check"};
  } else if (effectiveConnectionMode == SystemConfig::INTERMITTENT_KEEP_ALIVE) {
    return {true, "keep alive mode"};
  } else if (cadenceDue) {
    return {true, "boundary aligned"};
  } else {
    return {false, "not aligned"};
  }
}

static Inputs fromBits(unsigned bits, uint8_t mode) {
  Inputs in;
  in.serviceRequest = bits & 1;
  in.occupancyChange = bits & 2;
  in.closeDue = bits & 4;
  in.webhookHealthDue = bits & 8;
  in.cadenceDue = bits & 16;
  in.effectiveMode = mode;
  return in;
}

static void testMapRows() {
  struct Row {
    const char *id;
    unsigned bits;  // serviceRequest 1, occupancyChange 2, closeDue 4, webhookHealthDue 8, cadenceDue 16
    uint8_t mode;
    Action action;
    const char *outcome;  // the map's "Outcome today" reason string
  };
  const uint8_t C = SystemConfig::CONNECTED, I = SystemConfig::INTERMITTENT, D = SystemConfig::DISCONNECTED,
                K = SystemConfig::INTERMITTENT_KEEP_ALIVE;
  const Row rows[] = {
      {"R2 service request, KEEP_ALIVE", 1, K, Action::CONNECT, "service request"},
      {"R2 service request, DISCONNECTED", 1, D, Action::CONNECT, "service request"},
      {"R2 service request, CONNECTED", 1, C, Action::CONNECT, "service request"},
      {"R2 service request, INTERMITTENT", 1, I, Action::CONNECT, "service request"},
      {"R3 occupancy change, KEEP_ALIVE", 2, K, Action::CONNECT, "occupancy change"},
      {"R3 occupancy change, CONNECTED", 2, C, Action::CONNECT, "occupancy change"},
      {"R4 daily close, KEEP_ALIVE", 4, K, Action::CONNECT, "daily close"},
      {"R4 daily close, INTERMITTENT", 4, I, Action::CONNECT, "daily close"},
      {"R4 daily close, DISCONNECTED", 4, D, Action::CONNECT, "daily close"},
      {"R4 daily close, CONNECTED", 4, C, Action::CONNECT, "daily close"},
      {"R5 webhook health, KEEP_ALIVE", 8, K, Action::CONNECT, "webhook health check"},
      {"R5 webhook health, INTERMITTENT", 8, I, Action::CONNECT, "webhook health check"},
      {"R5 webhook health, DISCONNECTED", 8, D, Action::CONNECT, "webhook health check"},
      {"R5 webhook health, CONNECTED", 8, C, Action::CONNECT, "webhook health check"},
      {"R7 any other report, KEEP_ALIVE", 0, K, Action::CONNECT, "keep alive mode"},
      {"R7 cadence due, KEEP_ALIVE", 16, K, Action::CONNECT, "keep alive mode"},
      {"R9 cadence due, CONNECTED", 16, C, Action::CONNECT, "boundary aligned"},
      {"R9 cadence due, INTERMITTENT", 16, I, Action::CONNECT, "boundary aligned"},
      {"R9 cadence due, DISCONNECTED", 16, D, Action::CONNECT, "boundary aligned"},
      {"R10 cadence not due, CONNECTED", 0, C, Action::WAIT, "not aligned"},
      {"R10 cadence not due, INTERMITTENT", 0, I, Action::WAIT, "not aligned"},
      {"R10 cadence not due, DISCONNECTED", 0, D, Action::WAIT, "not aligned"},
      // first match wins: the order of the chain
      {"R2 before R3", 1 | 2, K, Action::CONNECT, "service request"},
      {"R3 before R4", 2 | 4, K, Action::CONNECT, "occupancy change"},
      {"R4 before R5", 4 | 8, I, Action::CONNECT, "daily close"},
      {"R5 before R7", 8, K, Action::CONNECT, "webhook health check"},
      {"R5 before R9", 8 | 16, I, Action::CONNECT, "webhook health check"},
      {"R4 before R9", 4 | 16, I, Action::CONNECT, "daily close"},
  };
  for (const Row &row : rows) {
    const Result r = ConnectDecision::decide(fromBits(row.bits, row.mode));
    char label[160];
    snprintf(label, sizeof(label), "char: %s", row.id);
    CHECK(label, is(r, row.action, row.outcome));
    CHECK(label, r.holdRemainingMs == 0);
  }
}

static void testEveryCombinationMatchesTheOldChain() {
  const uint32_t ages[] = {0, 1, kHour - 1, kHour, 6 * kHour, UINT32_MAX};
  for (unsigned m = 0; m < 4; m++) {
    for (unsigned bits = 0; bits < 32; bits++) {
      const Outcome want = oldChain(bits & 1, bits & 2, bits & 4, bits & 8, kModes[m], bits & 16);
      for (uint32_t age : ages) {
        Inputs in = fromBits(bits, kModes[m]);
        in.msSinceLastFailure = age;
        const Result r = ConnectDecision::decide(in);
        char label[160];
        snprintf(label, sizeof(label), "char: old chain, mode=%s triggers=0x%02x age=%lu", kModeNames[m], bits,
                 (unsigned long)age);
        CHECK(label, is(r, want.connect ? Action::CONNECT : Action::WAIT, want.reason));
        snprintf(label, sizeof(label), "reset: no hold at failureCount 0, mode=%s triggers=0x%02x age=%lu",
                 kModeNames[m], bits, (unsigned long)age);
        CHECK(label, r.holdRemainingMs == 0 && strcmp(r.reason, "connect hold") != 0);
      }
    }
  }
}

static void testHoldSequence() {
  CHECK("seq: the policy constants are 1 h and 6 h",
        ConnectivityPolicy::HOLD_BASE_MS == kHour && ConnectivityPolicy::HOLD_MAX_MS == 6 * kHour);
  CHECK("seq: no failure, no hold", ConnectDecision::holdMs(0) == 0);
  CHECK("seq: failure 1 holds 1 h", ConnectDecision::holdMs(1) == 1 * kHour);
  CHECK("seq: failure 2 holds 2 h", ConnectDecision::holdMs(2) == 2 * kHour);
  CHECK("seq: failure 3 holds 4 h", ConnectDecision::holdMs(3) == 4 * kHour);
  CHECK("seq: failure 4 holds 6 h", ConnectDecision::holdMs(4) == 6 * kHour);
  CHECK("seq: failure 5 holds 6 h", ConnectDecision::holdMs(5) == 6 * kHour);
  for (unsigned n = 6; n <= 255; n++) {
    char label[160];
    snprintf(label, sizeof(label), "seq: failure %u holds 6 h", n);
    CHECK(label, ConnectDecision::holdMs((uint8_t)n) == 6 * kHour);
  }
}

struct Trigger {
  const char *name;    // the CONNECT reason it gives once the hold has passed
  unsigned bits;
  uint8_t mode;
};
static const Trigger kHeld[] = {
    {"occupancy change", 2, SystemConfig::INTERMITTENT_KEEP_ALIVE},
    {"occupancy change", 2, SystemConfig::CONNECTED},
    {"daily close", 4, SystemConfig::INTERMITTENT},
    {"webhook health check", 8, SystemConfig::INTERMITTENT},
    {"keep alive mode", 0, SystemConfig::INTERMITTENT_KEEP_ALIVE},
    {"boundary aligned", 16, SystemConfig::INTERMITTENT},
};
static const uint32_t kHoldHours[] = {0, 1, 2, 4, 6, 6};  // by failure count

static void testHoldRows() {
  for (uint8_t n = 1; n <= 5; n++) {
    const uint32_t hold = kHoldHours[n] * kHour;
    const uint32_t inside[] = {0, 1, hold / 2, hold - 1};
    const uint32_t passed[] = {hold, hold + 1, 2 * hold, UINT32_MAX};
    for (const Trigger &t : kHeld) {
      char label[160];
      for (uint32_t age : inside) {
        Inputs in = fromBits(t.bits, t.mode);
        in.failureCount = n;
        in.msSinceLastFailure = age;
        const Result r = ConnectDecision::decide(in);
        snprintf(label, sizeof(label), "hold: %s waits, failures=%u age=%lu", t.name, n, (unsigned long)age);
        CHECK(label, is(r, Action::WAIT, "connect hold"));
        CHECK(label, r.holdRemainingMs == hold - age);
        CHECK(label, r.trigger != nullptr && strcmp(r.trigger, t.name) == 0);
      }
      for (uint32_t age : passed) {
        Inputs in = fromBits(t.bits, t.mode);
        in.failureCount = n;
        in.msSinceLastFailure = age;
        const Result r = ConnectDecision::decide(in);
        snprintf(label, sizeof(label), "hold: %s connects once the hold has passed, failures=%u age=%lu", t.name, n,
                 (unsigned long)age);
        CHECK(label, is(r, Action::CONNECT, t.name));
        CHECK(label, r.holdRemainingMs == 0);
      }
    }

    // Rule 1 comes first: with nothing due the pass still answers "connect hold", then "not aligned".
    Inputs idle = fromBits(0, SystemConfig::INTERMITTENT);
    idle.failureCount = n;
    idle.msSinceLastFailure = hold - 1;
    Result r = ConnectDecision::decide(idle);
    CHECK("hold: nothing due waits on the hold", is(r, Action::WAIT, "connect hold") && r.holdRemainingMs == 1 &&
                                                   r.trigger != nullptr && strcmp(r.trigger, "none") == 0);
    idle.msSinceLastFailure = hold;
    r = ConnectDecision::decide(idle);
    CHECK("hold: nothing due is not aligned once the hold has passed",
          is(r, Action::WAIT, "not aligned") && r.holdRemainingMs == 0);
  }
}

static void testServiceRequestIsExempt() {
  for (uint8_t n = 1; n <= 5; n++) {
    const uint32_t hold = kHoldHours[n] * kHour;
    const uint32_t ages[] = {0, 1, hold / 2, hold - 1, hold};
    for (unsigned m = 0; m < 4; m++) {
      for (unsigned bits = 1; bits < 32; bits += 2) {  // every combination that includes the service request
        for (uint32_t age : ages) {
          Inputs in = fromBits(bits, kModes[m]);
          in.failureCount = n;
          in.msSinceLastFailure = age;
          const Result r = ConnectDecision::decide(in);
          char label[160];
          snprintf(label, sizeof(label), "service: connects during a hold, failures=%u mode=%s triggers=0x%02x age=%lu",
                   n, kModeNames[m], bits, (unsigned long)age);
          CHECK(label, is(r, Action::CONNECT, "service request") && r.holdRemainingMs == 0);
        }
      }
    }
  }
}

int main() {
  testMapRows();
  testEveryCombinationMatchesTheOldChain();
  testHoldSequence();
  testHoldRows();
  testServiceRequestIsExempt();
  if (failures) {
    printf("connect_decision_test: %d FAILED\n", failures);
    return 1;
  }
  printf("connect_decision_test: all checks passed\n");
  return 0;
}
