// WO-2026-11-05-007 (I21): the carry predicate, the write-back stamp, the
// composition with the current-boot trust, and the hibernate-verdict input.
// The real inline functions are called; nothing is mirrored except the one-line
// `isClockTrusted()` composition (its source text is checked by the .sh).
#include <stdint.h>
#include <stdio.h>

#include "time/ClockTrust.h"
#include "time/HibernateWakeDiagnostics.h"

using ClockTrust::isCarryEligible;
using ClockTrust::carryStamp;

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static const int64_t kNow = 1792000000;
static const int64_t kAge = 24 * 3600;

// timeOk, externalOk, now, last, lastPlausible, faultSeen
static bool carry(int64_t last, bool timeOk = true, bool extOk = true, bool lastPlausible = true, bool fault = false) {
  return isCarryEligible(timeOk, extOk, kNow, last, lastPlausible, fault);
}

static void predicateTable() {
  CHECK(carry(kNow - 600));
  CHECK(!carry(kNow - 600, false));                 // Time invalid / now implausible
  CHECK(!carry(kNow - 600, true, false));           // external clock unreadable or invalid
  CHECK(!carry(kNow - 600, true, true, false));     // lastSync implausible
  CHECK(!carry(kNow - 600, true, true, true, true));// oscillator fault seen
  CHECK(!carry(0));                                 // never synced / revoked
  CHECK(!carry(kNow + 1));                          // stamp in the future
  CHECK(carry(kNow));                               // age 0
  CHECK(carry(kNow - (kAge - 1)));                  // N-1
  CHECK(!carry(kNow - kAge));                       // N
  CHECK(!carry(kNow - (kAge + 1)));                 // N+1
  CHECK(!carry(1700000000, true, true, false));     // stale and below the plausible range
}

static void writeBackStamp() {
  const long T = ClockTrust::kMaxCarryCorrectionSec;
  CHECK(T == 60);
  CHECK(carryStamp(true, true, T - 1, kNow) == kNow);
  CHECK(carryStamp(true, true, T, kNow) == kNow);
  CHECK(carryStamp(true, true, T + 1, kNow) == 0);
  CHECK(carryStamp(true, true, -T, kNow) == kNow);
  CHECK(carryStamp(true, true, -T - 1, kNow) == 0);
  CHECK(carryStamp(true, true, 0, kNow) == kNow);        // a clean small correction renews
  CHECK(carryStamp(true, false, -1, kNow) == 0);         // unreadable pre-write value: the -1 sentinel is not a measurement
  CHECK(carryStamp(true, false, 0, kNow) == 0);
  CHECK(carryStamp(false, true, 0, kNow) == 0);          // failed write
}

// isClockTrusted() = current-boot trust || carry (source text checked by the .sh).
static bool trusted(bool timeValid, uint32_t syncedLastMs, bool carryOk) {
  return ClockTrust::isTrustedWrapAware(timeValid, 100000, syncedLastMs, 0, 0) || carryOk;
}

static void composition() {
  CHECK(trusted(true, 0, carry(kNow - 600)));                    // carry only, before any sync
  CHECK(trusted(true, 50000, false));                            // current-boot sync alone: unchanged
  CHECK(!trusted(true, 0, false));                               // neither
  CHECK(!trusted(true, 0, carry(kNow - 600, true, true, true, true)));  // the latch withdraws the carry mid-boot
  CHECK(trusted(true, 50000, carry(0)));                         // a revoke leaves the current boot trusted
}

static void verdictInput() {
  using HibernateWakeDiagnostics::GateArm;
  using HibernateWakeDiagnostics::isRtcArm;
  CHECK(isRtcArm(GateArm::kRtcRead));
  CHECK(isRtcArm(GateArm::kRtcBeforeZero));
  CHECK(isRtcArm(GateArm::kRtcOrder));
  CHECK(!isRtcArm(GateArm::kNone));                     // a passing wake behaves as before
  CHECK(!isRtcArm(GateArm::kResetReason));              // a non-hibernate boot has no verdict (kNone) too
  CHECK(!isRtcArm(GateArm::kWakeReason));
  CHECK(!isRtcArm(GateArm::kRequestedZero));
}

int main() {
  predicateTable();
  writeBackStamp();
  composition();
  verdictInput();
  if (fails) { printf("clock_trust_carry_test: %d failure(s)\n", fails); return 1; }
  printf("clock_trust_carry_test passed\n");
  return 0;
}
