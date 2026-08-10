// Host-side regression test for the AB1805 wake-reason classification logic
// in setup() in src/Generalized-Core-Counter.cpp (WO-2026-08-10-001, item 1,
// corrected per the Stage 7 review documented in the WO's "Status" section).
// The real setup() cannot be compiled standalone on the host - it pulls in
// Particle.h, Wire, the AB1805_RK/PublishQueuePosixRK/StorageHelperRK
// libraries, and a large amount of application state - so this test lifts the
// exact classification decision (the block between "AB1805 WATCHDOG
// WAKE-REASON CLASSIFICATION" and ab1805.setWDT()) into a small,
// dependency-free function (classifyPinResetWatchdog(), below) that is a
// byte-for-byte mirror of that block's control flow: it calls
// ab1805.getWakeReason() only - no updateWakeReason() call - exactly like the
// corrected source. Keeping it in sync with the source is a manual
// responsibility; a code reviewer/CI addition could diff the two, but that is
// out of scope for this WO.
//
// AB1805::WakeReason's enumerators are copied verbatim from
// lib/AB1805_RK/src/AB1805_RK.h (UNKNOWN, WATCHDOG, DEEP_POWER_DOWN,
// COUNTDOWN_TIMER, ALARM) since that header itself requires Particle.h and
// cannot be included on the host.
//
// FakeAb1805 below models the real AB1805_RK library's behavior closely
// enough to reproduce the actual bug this WO's corrective pass fixes: its
// updateWakeReason() mirrors lib/AB1805_RK/src/AB1805_RK.cpp's real
// updateWakeReason() - status-register bits are checked WDT-first, then
// TIMER, then ALARM (see AB1805_RK.cpp lines ~184-201), and each classified
// bit is *destructively cleared* from the register on read, exactly like the
// real clearRegisterBit(REG_STATUS, REG_STATUS_WDT) etc. calls. FakeAb1805's
// setup() mirrors AB1805::setup() calling updateWakeReason() internally
// exactly once, on successful "chip detection".

#include <cassert>
#include <cstdio>
#include <string>

namespace {

// Mirrors AB1805::WakeReason (lib/AB1805_RK/src/AB1805_RK.h) verbatim.
enum class WakeReason {
  UNKNOWN,
  WATCHDOG,
  DEEP_POWER_DOWN,
  COUNTDOWN_TIMER,
  ALARM
};

// Test double standing in for the real `AB1805 ab1805` global, modeling its
// status-register semantics closely enough to reproduce the destructive-clear
// bug: each of statusWdt/statusTim/statusAlm represents a status-register bit
// that, once classified by updateWakeReason(), is cleared - just like the
// real chip's clearRegisterBit() calls. Multiple bits may be set at once
// (e.g. a watchdog reset that happens to coincide with a pending countdown
// timer expiry), mirroring the real hardware condition that caused the bug.
struct FakeAb1805 {
  bool chipDetected = true;
  bool statusWdt = false;
  bool statusTim = false;
  bool statusAlm = false;
  WakeReason wakeReason = WakeReason::UNKNOWN;

  // Mirrors AB1805::updateWakeReason()'s real if/else-if priority chain
  // (WDT checked first, then TIMER, then ALARM - lib/AB1805_RK/src/AB1805_RK.cpp
  // lines ~184-201) and its destructive clear-on-read of whichever bit it
  // classifies.
  bool updateWakeReason() {
    if (statusWdt) {
      wakeReason = WakeReason::WATCHDOG;
      statusWdt = false; // destructive clear, exactly like the real chip
    } else if (statusTim) {
      wakeReason = WakeReason::COUNTDOWN_TIMER;
      statusTim = false;
    } else if (statusAlm) {
      wakeReason = WakeReason::ALARM;
      statusAlm = false;
    }
    return true;
  }

  // Mirrors AB1805::setup() calling updateWakeReason() internally exactly
  // once, only on successful chip detection (lib/AB1805_RK/src/AB1805_RK.cpp
  // line ~28-29: `if (detectChip()) { updateWakeReason(); }`).
  void setup() {
    if (chipDetected) {
      updateWakeReason();
    }
    // If chip detection fails, wakeReason stays at its default UNKNOWN -
    // there is no separate success/fail flag exposed by the real library.
  }

  WakeReason getWakeReason() const { return wakeReason; }
};

// Mirrors ab1805WakeReasonName() (Generalized-Core-Counter.cpp).
const char *wakeReasonName(WakeReason reason) {
  switch (reason) {
  case WakeReason::WATCHDOG:
    return "WATCHDOG";
  case WakeReason::DEEP_POWER_DOWN:
    return "DEEP_POWER_DOWN";
  case WakeReason::COUNTDOWN_TIMER:
    return "COUNTDOWN_TIMER";
  case WakeReason::ALARM:
    return "ALARM";
  case WakeReason::UNKNOWN:
  default:
    return "UNKNOWN";
  }
}

// A stand-in for Device OS's RESET_REASON_PIN_RESET constant (its actual
// numeric value, 20, is irrelevant here - only equality with `reason` below
// matters, exactly as in the real setup()).
constexpr int RESET_REASON_PIN_RESET = 20;
constexpr int RESET_REASON_WATCHDOG = 60;
constexpr int RESET_REASON_USER = 10;

struct ClassificationResult {
  bool pinResetChecked = false;
  std::string wakeReasonName = "N/A";
  bool ab1805ConfirmedWatchdog = false;
};

// Byte-for-byte mirror of the CORRECTED classification block in setup()
// (between ab1805.setup() and ab1805.setWDT() - see Generalized-Core-Counter.cpp,
// "AB1805 WATCHDOG WAKE-REASON CLASSIFICATION"). Calls ab1805.getWakeReason()
// ONLY - no updateWakeReason() call - reusing whatever ab1805.setup() already
// classified from its single internal read.
ClassificationResult classifyPinResetWatchdog(int reason, FakeAb1805 &ab1805) {
  ClassificationResult result;

  if (reason != RESET_REASON_PIN_RESET) {
    return result;
  }

  result.pinResetChecked = true;
  const WakeReason pinResetWakeReason = ab1805.getWakeReason();
  result.wakeReasonName = wakeReasonName(pinResetWakeReason);

  if (pinResetWakeReason == WakeReason::WATCHDOG) {
    result.ab1805ConfirmedWatchdog = true;
  } else if (pinResetWakeReason == WakeReason::UNKNOWN) {
    // Explicitly inconclusive - do NOT treat UNKNOWN as "not the AB1805".
    result.ab1805ConfirmedWatchdog = false;
  } else {
    result.ab1805ConfirmedWatchdog = false;
  }

  return result;
}

// Reproduces the ORIGINAL (buggy) classification block from the first
// implementation pass: it called ab1805.updateWakeReason() a SECOND time
// (after AB1805::setup() already called it once internally) before reading
// getWakeReason(). This function exists solely so the test below can prove
// the old design actually misclassifies combined-bit status reads - it must
// NOT be reintroduced into the real source.
ClassificationResult classifyPinResetWatchdogBuggyDoubleRead(int reason, FakeAb1805 &ab1805) {
  ClassificationResult result;

  if (reason != RESET_REASON_PIN_RESET) {
    return result;
  }

  result.pinResetChecked = true;
  ab1805.updateWakeReason(); // the redundant second read - this is the bug
  const WakeReason pinResetWakeReason = ab1805.getWakeReason();
  result.wakeReasonName = wakeReasonName(pinResetWakeReason);

  if (pinResetWakeReason == WakeReason::WATCHDOG) {
    result.ab1805ConfirmedWatchdog = true;
  } else {
    result.ab1805ConfirmedWatchdog = false;
  }

  return result;
}

// Mirrors the combined watchdogClassified condition and the "must not
// overwrite persisted last-watchdog fields unless classified" rule from
// setup()'s "WATCHDOG RESET PERSISTENCE / FORENSICS" block.
bool shouldPersistWatchdogForensics(bool watchdogResetDetected, bool ab1805ConfirmedWatchdog) {
  return watchdogResetDetected || ab1805ConfirmedWatchdog;
}

void testPinResetWithConfirmedAb1805Watchdog() {
  FakeAb1805 ab1805;
  ab1805.statusWdt = true;
  ab1805.setup(); // AB1805::setup() classifies WATCHDOG and clears the bit

  const ClassificationResult result = classifyPinResetWatchdog(RESET_REASON_PIN_RESET, ab1805);

  assert(result.pinResetChecked);
  assert(result.wakeReasonName == "WATCHDOG");
  assert(result.ab1805ConfirmedWatchdog == true);
  // Should participate in watchdog persistence/forensics even though the OS
  // reason was PIN_RESET, not WATCHDOG.
  assert(shouldPersistWatchdogForensics(/*watchdogResetDetected=*/false, result.ab1805ConfirmedWatchdog) == true);

  printf("PASS: testPinResetWithConfirmedAb1805Watchdog\n");
}

void testPinResetWithUnknownIsInconclusiveNotNegative() {
  FakeAb1805 ab1805;
  // No status bits set at all - setup()'s internal read leaves UNKNOWN.
  ab1805.setup();

  const ClassificationResult result = classifyPinResetWatchdog(RESET_REASON_PIN_RESET, ab1805);

  assert(result.pinResetChecked);
  assert(result.wakeReasonName == "UNKNOWN");
  // Must NOT be silently misclassified as a confirmed watchdog...
  assert(result.ab1805ConfirmedWatchdog == false);
  // ...but critically, "not confirmed" here means "inconclusive", which must
  // not overwrite a previously persisted watchdog's forensic fields (i.e. it
  // must also not itself trigger a fresh persistence write when there is no
  // separate Device-OS-detected watchdog condition active).
  assert(shouldPersistWatchdogForensics(/*watchdogResetDetected=*/false, result.ab1805ConfirmedWatchdog) == false);

  printf("PASS: testPinResetWithUnknownIsInconclusiveNotNegative\n");
}

void testChipDetectionFailureLeavesUnknownInconclusive() {
  // No clean success/fail signal is plumbed through (see WO decision on
  // startupAb1805WakeReadOk) - a failed chip detection simply leaves
  // wakeReason at its default UNKNOWN, which is already handled as
  // inconclusive above. This test documents that this is intentional and
  // correct, not a gap.
  FakeAb1805 ab1805;
  ab1805.chipDetected = false;
  ab1805.statusWdt = true; // even if a bit happens to be set, it's never read
  ab1805.setup();

  const ClassificationResult result = classifyPinResetWatchdog(RESET_REASON_PIN_RESET, ab1805);

  assert(result.pinResetChecked);
  assert(result.wakeReasonName == "UNKNOWN");
  assert(result.ab1805ConfirmedWatchdog == false);
  assert(shouldPersistWatchdogForensics(/*watchdogResetDetected=*/false, result.ab1805ConfirmedWatchdog) == false);

  printf("PASS: testChipDetectionFailureLeavesUnknownInconclusive\n");
}

void testPinResetWithNonWatchdogReasonIsNotConfirmed() {
  FakeAb1805 ab1805;
  ab1805.statusAlm = true;
  ab1805.setup();

  const ClassificationResult result = classifyPinResetWatchdog(RESET_REASON_PIN_RESET, ab1805);

  assert(result.pinResetChecked);
  assert(result.wakeReasonName == "ALARM");
  assert(result.ab1805ConfirmedWatchdog == false);

  printf("PASS: testPinResetWithNonWatchdogReasonIsNotConfirmed\n");
}

void testNonPinResetReasonSkipsAb1805CheckEntirely() {
  FakeAb1805 ab1805;
  ab1805.statusWdt = true;
  ab1805.setup();

  // A plain RESET_REASON_USER boot should never consult the AB1805 at all -
  // the check is strictly gated on RESET_REASON_PIN_RESET.
  const ClassificationResult result = classifyPinResetWatchdog(RESET_REASON_USER, ab1805);
  assert(result.pinResetChecked == false);
  assert(result.ab1805ConfirmedWatchdog == false);

  printf("PASS: testNonPinResetReasonSkipsAb1805CheckEntirely\n");
}

void testDeviceOsWatchdogPathStillPersistsIndependently() {
  // Device-OS-detected watchdog (reason == RESET_REASON_WATCHDOG) must still
  // trigger persistence/forensics regardless of AB1805 state (which is never
  // even consulted for this reason value).
  assert(shouldPersistWatchdogForensics(/*watchdogResetDetected=*/true, /*ab1805ConfirmedWatchdog=*/false) == true);
  (void)RESET_REASON_WATCHDOG;
  printf("PASS: testDeviceOsWatchdogPathStillPersistsIndependently\n");
}

// --- Regression tests for the Stage 7 double-read bug (WO corrective pass) ---

// Combined WDT+TIMER status bits, exactly the scenario the WO's Status
// section calls out: setup()'s single internal updateWakeReason() call
// correctly prioritizes WDT and clears it. The CORRECTED classification
// (getWakeReason() only) must still report WATCHDOG. The BUGGY double-read
// classification must be shown to instead fall through to COUNTDOWN_TIMER,
// proving this is a real regression test, not just an assertion of intent.
void testCombinedWdtAndTimerBitsClassifyAsWatchdog() {
  FakeAb1805 fixedAb1805;
  fixedAb1805.statusWdt = true;
  fixedAb1805.statusTim = true;
  fixedAb1805.setup(); // single internal read: WDT wins, WDT bit cleared, TIM bit still set

  const ClassificationResult fixedResult = classifyPinResetWatchdog(RESET_REASON_PIN_RESET, fixedAb1805);
  assert(fixedResult.wakeReasonName == "WATCHDOG");
  assert(fixedResult.ab1805ConfirmedWatchdog == true);

  // Now prove the OLD design actually breaks on this exact input: a fresh
  // FakeAb1805 in the same combined-bit state, but run through the buggy
  // double-read classifier.
  FakeAb1805 buggyAb1805;
  buggyAb1805.statusWdt = true;
  buggyAb1805.statusTim = true;
  buggyAb1805.setup(); // AB1805::setup()'s internal call: same as above

  const ClassificationResult buggyResult = classifyPinResetWatchdogBuggyDoubleRead(RESET_REASON_PIN_RESET, buggyAb1805);
  // The redundant second updateWakeReason() call re-reads the register: WDT
  // bit is already cleared, so it now falls through to TIMER, silently
  // overwriting the correct WATCHDOG classification.
  assert(buggyResult.wakeReasonName == "COUNTDOWN_TIMER");
  assert(buggyResult.ab1805ConfirmedWatchdog == false);

  printf("PASS: testCombinedWdtAndTimerBitsClassifyAsWatchdog (and buggy double-read regression confirmed)\n");
}

// Same proof for WDT+ALARM combined bits.
void testCombinedWdtAndAlarmBitsClassifyAsWatchdog() {
  FakeAb1805 fixedAb1805;
  fixedAb1805.statusWdt = true;
  fixedAb1805.statusAlm = true;
  fixedAb1805.setup();

  const ClassificationResult fixedResult = classifyPinResetWatchdog(RESET_REASON_PIN_RESET, fixedAb1805);
  assert(fixedResult.wakeReasonName == "WATCHDOG");
  assert(fixedResult.ab1805ConfirmedWatchdog == true);

  FakeAb1805 buggyAb1805;
  buggyAb1805.statusWdt = true;
  buggyAb1805.statusAlm = true;
  buggyAb1805.setup();

  const ClassificationResult buggyResult = classifyPinResetWatchdogBuggyDoubleRead(RESET_REASON_PIN_RESET, buggyAb1805);
  assert(buggyResult.wakeReasonName == "ALARM");
  assert(buggyResult.ab1805ConfirmedWatchdog == false);

  printf("PASS: testCombinedWdtAndAlarmBitsClassifyAsWatchdog (and buggy double-read regression confirmed)\n");
}

} // namespace

int main() {
  testPinResetWithConfirmedAb1805Watchdog();
  testPinResetWithUnknownIsInconclusiveNotNegative();
  testChipDetectionFailureLeavesUnknownInconclusive();
  testPinResetWithNonWatchdogReasonIsNotConfirmed();
  testNonPinResetReasonSkipsAb1805CheckEntirely();
  testDeviceOsWatchdogPathStillPersistsIndependently();
  testCombinedWdtAndTimerBitsClassifyAsWatchdog();
  testCombinedWdtAndAlarmBitsClassifyAsWatchdog();
  printf("All AB1805 watchdog classification tests passed\n");
  return 0;
}
