// WO-2026-10-08-001 item C: the failsafe counts only overdue expected
// connections. Part 1 of tests/failsafe_cadence_rule_test.sh.
//
// This test compiles the REAL cadence-rule block of
// connectivityFailsafeSupervisor(). The shell driver extracts the block from
// src/Generalized-Core-Counter.cpp, checks it byte-for-byte against the source
// (COPY_MISMATCH on any difference) and writes it to CADENCE_RULE_BLOCK_H,
// which is #included below inside decide(). Nothing here re-implements the
// formula: a mutation in src/ changes the compiled block and so the results.
//
// Injected by the driver:
//   CADENCE_RULE_BLOCK_H   path of the extracted block (empty when src has none)
//   REAL_STALE_SEC         production stale threshold (ConnectivityPolicy.h)
//   TEST_STALE_SEC         test-mode stale threshold (ConnectivityPolicy.h)
//   CONNECTIVITY_FAILSAFE_TEST_MODE  0 or 1, as in the firmware build
//
// The cadence comes from the real ReportingPolicyResolver::resolve(); only the
// runtime input gathering (resolveRuntime) is stubbed.

#include <cstdint>
#include <cstdio>
#include <ctime>

#include "cloud/BatteryBackoffPolicy.h"
#include "reporting/ReportingPolicy.h"

#ifndef CADENCE_RULE_BLOCK_H
#error "CADENCE_RULE_BLOCK_H must be injected"
#endif
#ifndef REAL_STALE_SEC
#error "REAL_STALE_SEC must be injected"
#endif
#ifndef TEST_STALE_SEC
#error "TEST_STALE_SEC must be injected"
#endif
#ifndef CONNECTIVITY_FAILSAFE_TEST_MODE
#error "CONNECTIVITY_FAILSAFE_TEST_MODE must be injected"
#endif

namespace ConnectivityPolicy {
#if CONNECTIVITY_FAILSAFE_TEST_MODE
constexpr time_t CONNECTIVITY_FAILSAFE_STALE_SEC = TEST_STALE_SEC;
#else
constexpr time_t CONNECTIVITY_FAILSAFE_STALE_SEC = REAL_STALE_SEC;
#endif
} // namespace ConnectivityPolicy

namespace {
BatteryTier stubTier = TIER_HEALTHY;
uint32_t stubConfiguredSec = 3600;
} // namespace

namespace ReportingPolicyResolver {
ReportingPolicy resolveRuntime(float, time_t nowEpoch) {
  ReportingPolicyInputs inputs;
  inputs.configuredIntervalSec = stubConfiguredSec;
  inputs.batteryTier = stubTier;
  inputs.batteryMultiplier = BatteryBackoff::intervalMultiplier(stubTier);
  inputs.nowEpoch = nowEpoch;
  inputs.clockTrusted = true;
  inputs.windowOpen = true;
  inputs.alignmentToleranceSec = 30;
  return resolve(inputs);
}
} // namespace ReportingPolicyResolver

struct PowerManagerStub {
  static PowerManagerStub &instance() {
    static PowerManagerStub p;
    return p;
  }
  float soc() const { return 50.0f; }
};
#define PowerManager PowerManagerStub

namespace {

int failures = 0;
bool escalates = false;

// Runs the real block with the given age, after the supervisor's own stale
// return (its text is located in src/ by the driver's order check). The block's
// `return;` leaves escalates false; falling out of it means the supervisor goes
// on to escalate.
void decide(time_t now, time_t connectionAgeSec) {
  escalates = false;
  (void)now;
  if (connectionAgeSec < ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC) {
    return;
  }
#include CADENCE_RULE_BLOCK_H
  escalates = true;
}

#if !CONNECTIVITY_FAILSAFE_TEST_MODE
constexpr time_t H = 3600;
#endif
constexpr time_t NOW = 1000000;

void check(const char *name, bool got, bool want) {
  if (got == want) {
    printf("  ok   %s\n", name);
  } else {
    printf("  FAIL %s (escalates=%d, want %d)\n", name, (int)got, (int)want);
    failures++;
  }
}

bool escalatesAt(time_t age) {
  decide(NOW, age);
  return escalates;
}

#if !CONNECTIVITY_FAILSAFE_TEST_MODE
void expectThreshold(const char *label, time_t threshold) {
  char name[160];
  snprintf(name, sizeof(name), "%s: no escalation 1 s before %ld s", label, (long)threshold);
  check(name, escalatesAt(threshold - 1), false);
  snprintf(name, sizeof(name), "%s: escalates at %ld s", label, (long)threshold);
  check(name, escalatesAt(threshold), true);
}

time_t cadenceFor(BatteryTier tier, uint32_t configuredSec) {
  stubTier = tier;
  stubConfiguredSec = configuredSec;
  return (time_t)ReportingPolicyResolver::resolveRuntime(0.0f, NOW).effectiveIntervalSec;
}
#endif

} // namespace

int main() {
  printf("--- Part 1: real cadence-rule block (stale=%ld build=%s) ---\n",
         (long)ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC,
         CONNECTIVITY_FAILSAFE_TEST_MODE ? "test-mode" : "production");

#if CONNECTIVITY_FAILSAFE_TEST_MODE
  // The test build has no cadence rule: the threshold is plain STALE_SEC even
  // with a 4 h cadence.
  stubTier = TIER_CRITICAL;
  check("test build: escalates at plain STALE_SEC despite a 4 h cadence",
        escalatesAt(ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC), true);
#else
  const time_t cadence1h = cadenceFor(TIER_HEALTHY, 3600);
  check("cadence is 1 h on a healthy 3600 s interval", cadence1h == H, true);
  expectThreshold("cadence 1 h", 3 * H);

  const time_t cadence4h = cadenceFor(TIER_CRITICAL, 3600);
  check("cadence is 4 h at CRITICAL on a 3600 s interval", cadence4h == 4 * H, true);
  check("4 h cadence: no escalation at 3 h", escalatesAt(3 * H), false);
  check("4 h cadence: no escalation at 5 h", escalatesAt(5 * H), false);
  check("4 h cadence: no escalation at 6 h 59 m", escalatesAt(7 * H - 60), false);
  expectThreshold("cadence 4 h", 7 * H);

  // Cadence exactly 3 h (CONSERVING x2 on a 5400 s interval): threshold is 6 h.
  const time_t cadence3h = cadenceFor(TIER_CONSERVING, 5400);
  check("cadence is 3 h at CONSERVING on a 5400 s interval", cadence3h == 3 * H, true);
  expectThreshold("cadence 3 h", 6 * H);

  // Just under 3 h the rule does not apply: the threshold stays 3 h.
  const time_t cadenceUnder = cadenceFor(TIER_HEALTHY, 3 * 3600 - 60);
  check("cadence is 2 h 59 m on a healthy 10740 s interval", cadenceUnder == 3 * H - 60, true);
  expectThreshold("cadence 2 h 59 m", 3 * H);
#endif

  if (failures != 0) {
    printf("\nFAILED: %d check(s)\n", failures);
    return 1;
  }
  printf("\nPart 1 passed\n");
  return 0;
}
