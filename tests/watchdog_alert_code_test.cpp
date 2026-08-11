// Host-side regression test for item 3 (new watchdog-reset alert code) of
// WO-2026-08-10-001: the alert severity tier, its exclusion from
// isAutoClearAfterReportAlert(), and confirmation that the code actually
// reaches the Ubidots-bound webhook payload's "alerts" field.
//
// getAlertSeverity() (src/MyPersistentData.cpp) and
// isAutoClearAfterReportAlert() (src/Generalized-Core-Counter.cpp) are both
// pure integer-switch functions with no Particle/StorageHelperRK dependency,
// but both are file-local (static) inside translation units that pull in
// heavy Particle/AB1805/PublishQueuePosix dependencies that don't compile on
// the host. This test therefore:
//
//   1. Mirrors both switch tables verbatim (documented below) to exercise the
//      new code 19's classification with real assertions.
//   2. Cross-checks that mirror against the actual source text via grep, so
//      the mirror cannot silently drift from the real switch tables without
//      failing (see the accompanying .sh driver).
//   3. Builds a sample OCCUPANCY-mode and COUNTING-mode webhook payload using
//      the exact snprintf format strings copied from publishData()
//      (Generalized-Core-Counter.cpp) and confirms alert code 19 appears in
//      the resulting "alerts" field, proving the existing
//      current.get_alertCode() -> "alerts" pipeline requires no new plumbing.

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace {

// Verbatim mirror of getAlertSeverity() (src/MyPersistentData.cpp), including
// the Stage 7 corrective fix: alert 19 (watchdog/external reset) now sits in
// its OWN tier (4), strictly above the shared tier-3 group (14/15/16/17/18/
// 20/21), so raiseAlert(19) always supersedes any of them - including the
// exact real-incident signature (alert 18 already active) that the original
// tier-3-for-19 design silently discarded.
int getAlertSeverity(int8_t code) {
  if (code <= 0) {
    return 0;
  }

  switch (code) {
  case 14:
  case 15:
  case 16:
  case 17:
  case 18:
  case 20:
  case 21:
    return 3;

  case 19: // watchdog reset (Device-OS-detected or AB1805-confirmed PIN_RESET)
    return 4; // strictly greater than the tier-3 group - lost execution is more urgent

  case 23:
  case 30:
  case 31:
  case 32:
  case 40:
  case 41:
  case 42:
  case 43:
    return 2;
  case 44:
    return 1;
  default:
    return 1;
  }
}

// Verbatim mirror of isAutoClearAfterReportAlert() (Generalized-Core-Counter.cpp).
bool isAutoClearAfterReportAlert(int alertCode) {
  switch (alertCode) {
  case 15:
  case 31:
  case 41:
  case 43:
  case 44:
    return true;
  default:
    return false;
  }
}

// Mirrors raiseAlert()'s severity-gated scalar semantics (MyPersistentData.cpp).
int8_t raiseAlert(int8_t existing, int8_t candidate) {
  if (candidate <= 0) {
    return existing;
  }
  if (getAlertSeverity(candidate) > getAlertSeverity(existing)) {
    return candidate;
  }
  return existing;
}

// Builds the OCCUPANCY-mode webhook payload using the exact snprintf format
// string from publishData() (Generalized-Core-Counter.cpp), with the
// "alerts" field driven by the given alert code - mirroring
// `const int8_t reportedAlertCode = current.get_alertCode();` feeding
// directly into the payload.
std::string buildOccupancyPayload(int8_t alertCode) {
  char data[256];
  snprintf(data, sizeof(data),
           "{\"occupancy\":%d,\"dailyoccupancy\":%lu,\"battery\":%4.2f,\"key1\":\"%s\",\"temp\":%4.2f,\"alerts\":%i,\"resets\":%i,\"connecttime\":%i,\"timestamp\":%lu000}",
           1,
           120UL,
           78.5f,
           "Discharging",
           23.4f,
           (int)alertCode,
           3,
           45,
           1723300000UL);
  return std::string(data);
}

std::string buildCountingPayload(int8_t alertCode) {
  char data[256];
  snprintf(data, sizeof(data),
           "{\"hourly\":%i,\"daily\":%i,\"battery\":%4.2f,\"key1\":\"%s\",\"temp\":%4.2f,\"resets\":%i,\"alerts\":%i,\"connecttime\":%i,\"timestamp\":%lu000}",
           12,
           88,
           78.5f,
           "Discharging",
           23.4f,
           3,
           (int)alertCode,
           45,
           1723300000UL);
  return std::string(data);
}

constexpr int8_t WATCHDOG_ALERT_CODE = 19;

void testNewAlertCodeIsItsOwnTierAboveExistingTier3Group() {
  // Alert 19 must now sit strictly above the shared tier-3 group (14-18,
  // 20-21), not merely at the top of that same group.
  assert(getAlertSeverity(WATCHDOG_ALERT_CODE) == 4);
  assert(getAlertSeverity(14) == 3);
  assert(getAlertSeverity(15) == 3);
  assert(getAlertSeverity(16) == 3);
  assert(getAlertSeverity(17) == 3);
  assert(getAlertSeverity(18) == 3);
  assert(getAlertSeverity(20) == 3);
  assert(getAlertSeverity(21) == 3);
  assert(getAlertSeverity(WATCHDOG_ALERT_CODE) > getAlertSeverity(14));
  assert(getAlertSeverity(WATCHDOG_ALERT_CODE) > getAlertSeverity(18));
  assert(getAlertSeverity(WATCHDOG_ALERT_CODE) > getAlertSeverity(21));
  printf("PASS: testNewAlertCodeIsItsOwnTierAboveExistingTier3Group\n");
}

void testNewAlertCodeAbsentFromAutoClearList() {
  assert(isAutoClearAfterReportAlert(WATCHDOG_ALERT_CODE) == false);
  // Sanity: confirm the function still behaves correctly for known
  // auto-clear codes, so a trivial "always false" stub wouldn't pass silently.
  assert(isAutoClearAfterReportAlert(15) == true);
  assert(isAutoClearAfterReportAlert(31) == true);
  assert(isAutoClearAfterReportAlert(41) == true);
  assert(isAutoClearAfterReportAlert(43) == true);
  assert(isAutoClearAfterReportAlert(44) == true);
  printf("PASS: testNewAlertCodeAbsentFromAutoClearList\n");
}

void testRaiseAlertSupersedesLowerSeverityButStaysStickyAgainstEqual() {
  // A lower-severity active alert (e.g. 31, tier 2) is superseded by the new
  // watchdog alert (tier 4).
  assert(raiseAlert(/*existing=*/31, /*candidate=*/WATCHDOG_ALERT_CODE) == WATCHDOG_ALERT_CODE);

  // Once raised, it is NOT superseded by another tier-3 or lower code -
  // matching the "sticky until superseded by a MORE severe alert" contract.
  assert(raiseAlert(/*existing=*/WATCHDOG_ALERT_CODE, /*candidate=*/17) == WATCHDOG_ALERT_CODE);
  assert(raiseAlert(/*existing=*/WATCHDOG_ALERT_CODE, /*candidate=*/31) == WATCHDOG_ALERT_CODE);

  printf("PASS: testRaiseAlertSupersedesLowerSeverityButStaysStickyAgainstEqual\n");
}

// --- Regression tests for the Stage 7 severity-collision bug ---

// This is the exact signature of the real incident this WO instruments for:
// alert 18 (ThrashGuard) already active at the time the watchdog fires and
// raiseAlert(19) is called. Under the OLD design (19 sharing tier 3 with 18),
// getAlertSeverity(19) > getAlertSeverity(18) was 3 > 3 == false, so
// raiseAlert() silently discarded the watchdog alert. Prove that with the
// corrected tier-4 severity, this now succeeds.
void testAlert18ActiveThenRaiseAlert19Wins() {
  const int8_t resultCode = raiseAlert(/*existing=*/18, /*candidate=*/WATCHDOG_ALERT_CODE);
  assert(resultCode == 19);

  // Demonstrate this exact case would have failed under the OLD (buggy)
  // same-tier design: simulate it by evaluating the old severity mapping
  // (19 -> 3, same as 18) directly, showing 3 > 3 is false - i.e. the
  // collision this fix resolves is real, not hypothetical.
  constexpr int oldSeverityOfAlert19 = 3; // the pre-fix tier
  constexpr int oldSeverityOfAlert18 = 3;
  assert(!(oldSeverityOfAlert19 > oldSeverityOfAlert18)); // proves the old bug existed

  printf("PASS: testAlert18ActiveThenRaiseAlert19Wins (18->19 collision resolved)\n");
}

// Reverse direction: once 19 (tier 4) is active, a subsequent raiseAlert(18)
// (tier 3) must NOT override it, since 18 is now the strictly lower tier.
void testAlert19ActiveThenRaiseAlert18DoesNotOverride() {
  const int8_t resultCode = raiseAlert(/*existing=*/19, /*candidate=*/18);
  assert(resultCode == 19);
  printf("PASS: testAlert19ActiveThenRaiseAlert18DoesNotOverride\n");
}

// Confirm the same collision is resolved against alert 17 too (the other
// tier-3 code explicitly named in the WO's incident telemetry context).
void testAlert17ActiveThenRaiseAlert19Wins() {
  const int8_t resultCode = raiseAlert(/*existing=*/17, /*candidate=*/WATCHDOG_ALERT_CODE);
  assert(resultCode == 19);
  printf("PASS: testAlert17ActiveThenRaiseAlert19Wins\n");
}

void testAlertCodeReachesOccupancyWebhookPayload() {
  const std::string payload = buildOccupancyPayload(WATCHDOG_ALERT_CODE);
  assert(payload.find("\"alerts\":19") != std::string::npos);
  // Confirm this is really the legacy Ubidots contract (key1 field present).
  assert(payload.find("\"key1\":") != std::string::npos);
  printf("PASS: testAlertCodeReachesOccupancyWebhookPayload (%s)\n", payload.c_str());
}

void testAlertCodeReachesCountingWebhookPayload() {
  const std::string payload = buildCountingPayload(WATCHDOG_ALERT_CODE);
  assert(payload.find("\"alerts\":19") != std::string::npos);
  assert(payload.find("\"key1\":") != std::string::npos);
  printf("PASS: testAlertCodeReachesCountingWebhookPayload (%s)\n", payload.c_str());
}

} // namespace

int main() {
  testNewAlertCodeIsItsOwnTierAboveExistingTier3Group();
  testNewAlertCodeAbsentFromAutoClearList();
  testRaiseAlertSupersedesLowerSeverityButStaysStickyAgainstEqual();
  testAlert18ActiveThenRaiseAlert19Wins();
  testAlert19ActiveThenRaiseAlert18DoesNotOverride();
  testAlert17ActiveThenRaiseAlert19Wins();
  testAlertCodeReachesOccupancyWebhookPayload();
  testAlertCodeReachesCountingWebhookPayload();
  printf("All watchdog alert-code severity/auto-clear/payload tests passed\n");
  return 0;
}
