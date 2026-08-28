// WO-2026-08-25-001 Amendment C, Decision C3 (Blocker E) host test: exercises
// the actual PRODUCTION src/power/PmicFaultMonitor.cpp::pollAndRemediate(),
// not a reimplementation, against a test-controllable PMIC stub. Covers all
// three C3 fixes:
//   1. thermallyCorrelatedFault must be classified per CHRG_FAULT code, not
//      by whether NTC_FAULT happens to also be nonzero on the same read - a
//      simultaneous independent input fault (0x01) must still be logged/
//      alerted/counted toward remediation even when NTC_FAULT is nonzero.
//   2. The no-fault branch must not abandon (zero out) an in-progress
//      remediation cycle that already disabled charging in phase 0 - it must
//      let that cycle finish (re-enable charging) before clearing counters.
//   3. The fault register is read TWICE and OR'd - a fault bit present on
//      only the first (or only the second) read must still be observed.
//
// Stage 7 R5a fix (this file's scenarios 4-5): the no-fault branch's
// advanceChargeCyclePhase() call previously had NO safeToCharge gate, so it
// could call pmic.enableCharging() on a genuinely unsafe (hot) pack once the
// phase-1 wait elapsed - contradicting PmicFaultMonitor.h's "remediation here
// always yields to safeToCharge == false" contract. The fix freezes (does not
// advance, does not abandon) an in-progress cycle while unsafe, and
// resumes/completes it once safe again. Scenarios 4 (level 1) and 5 (level 2)
// are the permanent regression tests for this - scenario 4 is Codex's
// decisive Stage 7 reproduction made permanent.

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "Particle.h"
#include "power/PmicFaultMonitor.h"
#include "power/PowerPlatform.h"
#include "MyPersistentData.h"

// Definition for the extern declared in tests/stubs/.../power/PowerPlatform.h.
PowerPlatform::TestPowerPlatformState PowerPlatform::testPowerPlatformState;

// Definitions for the two functions PmicFaultMonitor.cpp declares extern
// (real definitions live in src/sensors/SensorManager.cpp, which this host
// test does not link - it is out of scope for the PMIC fault-handling fixes
// under test here).
void boundedBatterySettleDelay(unsigned long) {}
unsigned long currentWakeAwakeMs() { return 0UL; }

namespace {

void resetPmic(PMIC &pmic) {
  pmic.faultRegQueue[0] = 0;
  pmic.faultRegQueue[1] = 0;
  pmic.faultCallCount = 0;
  pmic.systemStatusValue = 0;
  pmic.disableChargingCallCount = 0;
  pmic.enableChargingCallCount = 0;
  pmic.setWatchdogCallCount = 0;
}

void resetCurrent() {
  testCurrent.alertCodeValue = 0;
  testCurrent.lastAlertTimeValue = 0;
  testCurrent.internalTempCValue = 20.0f;
  testCurrent.batteryStateValue = 0;
  testCurrent.raiseAlertCallCount = 0;
  testCurrent.lastRaisedAlertCode = -1;
}

// --- Fix 1: per-fault-class classification, not per-register-read. ---

void testIndependentInputFaultIsNotSuppressedByCoincidentNtcFault() {
  PMIC pmic;
  resetPmic(pmic);
  resetCurrent();
  g_testMillis = 4000000UL; // bypass the 1h remediation cooldown gate

  // REG09 fault byte: NTC_FAULT bits(2:0) = 0x01 (nonzero, e.g. TS-network
  // condition), CHRG_FAULT bits(5:4) = 0x01 (Input fault) -> 0b00010001 = 0x11.
  // These are two independent conditions on the same register read.
  //
  // safeToCharge=false (as if ChargeInhibit had it disabled for an unrelated
  // reason) is the exact condition under which the old code's blanket
  // thermallyCorrelatedFault (true whenever NTC != 0) would have suppressed
  // remediation for this independent Input fault, freezing
  // consecutiveFaults/remediation state forever regardless of how many times
  // it recurs. With the fix, an Input fault (chargeFault==0x01) is NOT
  // thermally correlated merely because NTC_FAULT is also nonzero, so
  // remediation proceeds and escalates normally.
  for (int i = 0; i < 3; i++) {
    pmic.faultRegQueue[0] = pmic.faultRegQueue[1] = 0x11;
    pmic.faultCallCount = 0;
    PmicFaultMonitor::pollAndRemediate(pmic, /*safeToCharge=*/false, /*battState=*/0,
                                        /*powerSource=*/0, /*chargeDisableConfigVerified=*/false);
    g_testMillis += 10;
  }

  // NTC_FAULT alone still raises alert 20 unconditionally either way (that
  // part is unaffected by this fix) - the OBSERVABLE effect this test must
  // catch is whether the independent, non-thermal Input fault was still
  // allowed to escalate remediation. Under the pre-fix blanket
  // classification this never reaches phase 0 (pmic.disableCharging() is
  // never called, no matter how many consecutive faults occur) because
  // `(!safeToCharge && thermallyCorrelatedFault)` is unconditionally true
  // whenever NTC_FAULT != 0, freezing remediation state forever.
  assert(testCurrent.lastRaisedAlertCode == 20);
  assert(pmic.disableChargingCallCount >= 1);
}

// --- Fix 2: no-fault branch must not abandon in-progress remediation. ---

void testNoFaultBranchCompletesInProgressRemediationBeforeClearing() {
  PMIC pmic;
  resetPmic(pmic);
  resetCurrent();

  // Bypass the 1h remediation cooldown gate (`now - lastRemediationAttempt >
  // kRemediationCooldownMs`) on the very first fault - lastRemediationAttempt
  // starts at 0, so millis() must already exceed the cooldown window.
  g_testMillis = 4000000UL;

  // Drive 3 consecutive CHRG_FAULT=0x03 (safety timer) reads with
  // safeToCharge=true so remediation is free to run (not suppressed) and
  // escalates: fault 2 arms level 1 (sets remediationInProgress/phase=0 for
  // the NEXT call); fault 3 is the call that actually runs phase 0
  // (pmic.disableCharging()) via advanceChargeCyclePhase().
  pmic.faultRegQueue[0] = pmic.faultRegQueue[1] = 0x30; // CHRG_FAULT=0x03 << 4
  for (int i = 0; i < 3; i++) {
    PmicFaultMonitor::pollAndRemediate(pmic, /*safeToCharge=*/true, /*battState=*/0,
                                        /*powerSource=*/0, /*chargeDisableConfigVerified=*/false);
    // Only reset the fault-read cursor for the next call (so getFault() is
    // read fresh) - NOT the cumulative disable/enable call counters, which
    // this test needs to observe across the whole sequence.
    pmic.faultCallCount = 0;
    pmic.faultRegQueue[0] = pmic.faultRegQueue[1] = 0x30;
    g_testMillis += 10;
  }
  // After 3 consecutive faults, remediation should have escalated and run
  // phase 0 (disableCharging()) at least once.
  assert(pmic.disableChargingCallCount >= 1);
  const int disableCountAfterFaults = pmic.disableChargingCallCount;
  const int enableCountAfterFaults = pmic.enableChargingCallCount;

  // Advance past the phase-1 wait window (500ms for level 1) so this next
  // call is the one that WOULD complete the cycle (call enableCharging()) -
  // and the fault clears (no CHRG_FAULT on this read) at exactly that
  // moment. The buggy version zeroed remediationInProgress/ActiveLevel/Phase
  // here unconditionally the instant CHRG_FAULT read back clear, stranding
  // charging disabled with no software owner left to finish re-enabling it.
  g_testMillis += 600UL;
  pmic.faultCallCount = 0;
  pmic.faultRegQueue[0] = pmic.faultRegQueue[1] = 0x00; // no fault
  PmicFaultMonitor::pollAndRemediate(pmic, /*safeToCharge=*/true, /*battState=*/0,
                                      /*powerSource=*/0, /*chargeDisableConfigVerified=*/false);

  // The fix must have advanced the in-progress cycle's phase (calling
  // enableCharging() to complete it) rather than merely dropping the state -
  // disableChargingCallCount must NOT have grown (no new phase-0 disable) and
  // enableChargingCallCount MUST have grown (phase-1 completion happened).
  assert(pmic.disableChargingCallCount == disableCountAfterFaults);
  assert(pmic.enableChargingCallCount > enableCountAfterFaults);
}

// --- Fix 3: read the fault register twice and OR the results. ---

void testFaultRegisterIsReadTwiceAndOred() {
  PMIC pmic;
  resetPmic(pmic);
  resetCurrent();

  // First read reports nothing; second read reports an Input fault. A
  // single-read implementation observing only faultRegQueue[0] would see 0
  // and miss it entirely.
  pmic.faultRegQueue[0] = 0x00;
  pmic.faultRegQueue[1] = 0x10; // CHRG_FAULT=Input (0x01 << 4)

  const PmicFaultMonitor::Registers regs =
      PmicFaultMonitor::pollAndRemediate(pmic, /*safeToCharge=*/true, /*battState=*/0,
                                          /*powerSource=*/0, /*chargeDisableConfigVerified=*/false);

  assert(pmic.faultCallCount == 2); // both reads actually happened
  assert(((regs.faultReg & 0x30) >> 4) == 0x01); // OR'd result carries the second read's fault
}

// --- WO-2026-08-25-001 Stage 7 R5a fix: no-fault branch must FREEZE (not
// advance, not abandon) an in-progress remediation cycle while
// safeToCharge==false, and resume/complete it once safeToCharge becomes true
// again. This is Codex's decisive Stage 7 blocker: the no-fault branch
// previously called advanceChargeCyclePhase() with no safeToCharge gate at
// all, so a clear-fault poll during a genuinely unsafe (hot) interval would
// call pmic.enableCharging() unconditionally once the phase-1 wait elapsed -
// directly contradicting this module's own "remediation here always yields
// to safeToCharge == false" contract (PmicFaultMonitor.h). ---

// Drives 3 consecutive CHRG_FAULT=0x03 (safety timer, non-thermal) reads with
// safeToCharge=true so remediation escalates to level 1 and runs phase 0
// (pmic.disableCharging()), leaving the cycle parked in phase 1 waiting to
// re-enable - mirrors testNoFaultBranchCompletesInProgressRemediationBeforeClearing's setup.
void driveToLevel1Phase1(PMIC &pmic) {
  pmic.faultRegQueue[0] = pmic.faultRegQueue[1] = 0x30; // CHRG_FAULT=0x03 << 4
  for (int i = 0; i < 3; i++) {
    PmicFaultMonitor::pollAndRemediate(pmic, /*safeToCharge=*/true, /*battState=*/0,
                                        /*powerSource=*/0, /*chargeDisableConfigVerified=*/false);
    pmic.faultCallCount = 0;
    pmic.faultRegQueue[0] = pmic.faultRegQueue[1] = 0x30;
    g_testMillis += 10;
  }
}

void testNoFaultBranchFreezesLevel1WhileUnsafeThenResumes() {
  PMIC pmic;
  resetPmic(pmic);
  resetCurrent();
  g_testMillis = 4000000UL; // bypass the 1h remediation cooldown gate

  driveToLevel1Phase1(pmic);
  assert(pmic.disableChargingCallCount == 1); // phase 0 ran exactly once
  const int enableBefore = pmic.enableChargingCallCount; // 0: not yet re-enabled

  // Advance past the phase-1 wait window (500ms for level 1) so the next
  // no-fault poll is the one that WOULD complete the cycle, if allowed.
  g_testMillis += 600UL;

  // Codex's exact reproduction: clear fault reads, safeToCharge=false,
  // chargeDisableConfigVerified=true (as if a coupled temperature read this
  // same cycle is genuinely hot). Repeat several polls to prove the freeze
  // holds - not just on the first unsafe poll - and that the in-progress
  // sequence (remediationInProgress/ActiveLevel/Phase) is NOT abandoned:
  // if it had been zeroed, disableChargingCallCount would never move again
  // on its own (no new faults are being fed here), so it staying at 1 while
  // enableChargingCallCount also stays put is the observable proof the
  // phase-1 state is still parked, not reset to phase 0 or dropped.
  for (int i = 0; i < 3; i++) {
    pmic.faultCallCount = 0;
    pmic.faultRegQueue[0] = pmic.faultRegQueue[1] = 0x00; // clear fault reads
    PmicFaultMonitor::pollAndRemediate(pmic, /*safeToCharge=*/false, /*battState=*/0,
                                        /*powerSource=*/0, /*chargeDisableConfigVerified=*/true);
    // Requirement 1: PMIC must NOT be enabled while unsafe.
    assert(pmic.enableChargingCallCount == enableBefore);
    // Requirement 3: sequence retains its owner - no phantom restart of
    // phase 0 (which would only happen if the state had been zeroed and a
    // brand-new cycle re-armed and re-ran disableCharging()).
    assert(pmic.disableChargingCallCount == 1);
    g_testMillis += 10;
  }

  // Requirement 2: once safe again, the frozen sequence RESUMES and
  // completes - charging is enabled exactly once, not zero and not twice.
  pmic.faultCallCount = 0;
  pmic.faultRegQueue[0] = pmic.faultRegQueue[1] = 0x00;
  PmicFaultMonitor::pollAndRemediate(pmic, /*safeToCharge=*/true, /*battState=*/0,
                                      /*powerSource=*/0, /*chargeDisableConfigVerified=*/false);
  assert(pmic.enableChargingCallCount == enableBefore + 1);
  assert(pmic.disableChargingCallCount == 1); // still no phantom re-arm/restart
}

// Requirement 4: level 2 (1000ms wait, watchdog on resume) behaves the same.
// Drives 2 thermally-correlated (CHRG_FAULT=0x02) frozen faults while unsafe
// (consecutiveFaults reaches 2 without arming any level - the frozen branch
// skips escalation entirely), then 2 more with safeToCharge=true so
// consecutiveFaults reaches 3, escalating straight to level 2 and running
// phase 0.
void driveToLevel2Phase1(PMIC &pmic) {
  pmic.faultRegQueue[0] = pmic.faultRegQueue[1] = 0x20; // CHRG_FAULT=0x02 << 4 (thermal)
  for (int i = 0; i < 2; i++) {
    PmicFaultMonitor::pollAndRemediate(pmic, /*safeToCharge=*/false, /*battState=*/0,
                                        /*powerSource=*/0, /*chargeDisableConfigVerified=*/false);
    pmic.faultCallCount = 0;
    pmic.faultRegQueue[0] = pmic.faultRegQueue[1] = 0x20;
    g_testMillis += 10;
  }
  for (int i = 0; i < 2; i++) {
    PmicFaultMonitor::pollAndRemediate(pmic, /*safeToCharge=*/true, /*battState=*/0,
                                        /*powerSource=*/0, /*chargeDisableConfigVerified=*/false);
    pmic.faultCallCount = 0;
    pmic.faultRegQueue[0] = pmic.faultRegQueue[1] = 0x20;
    g_testMillis += 10;
  }
}

void testNoFaultBranchFreezesLevel2WhileUnsafeThenResumesWithWatchdog() {
  PMIC pmic;
  resetPmic(pmic);
  resetCurrent();
  g_testMillis = 4000000UL; // bypass the 1h remediation cooldown gate

  driveToLevel2Phase1(pmic);
  assert(pmic.disableChargingCallCount == 1); // phase 0 ran exactly once
  const int enableBefore = pmic.enableChargingCallCount; // 0

  // Advance past the phase-1 wait window (1000ms for level 2).
  g_testMillis += 1100UL;

  pmic.faultCallCount = 0;
  pmic.faultRegQueue[0] = pmic.faultRegQueue[1] = 0x00; // clear fault reads
  PmicFaultMonitor::pollAndRemediate(pmic, /*safeToCharge=*/false, /*battState=*/0,
                                      /*powerSource=*/0, /*chargeDisableConfigVerified=*/true);
  assert(pmic.enableChargingCallCount == enableBefore); // still frozen
  assert(pmic.setWatchdogCallCount == 0); // watchdog only set on actual resume
  assert(pmic.disableChargingCallCount == 1); // sequence not abandoned/restarted

  pmic.faultCallCount = 0;
  pmic.faultRegQueue[0] = pmic.faultRegQueue[1] = 0x00;
  PmicFaultMonitor::pollAndRemediate(pmic, /*safeToCharge=*/true, /*battState=*/0,
                                      /*powerSource=*/0, /*chargeDisableConfigVerified=*/false);
  assert(pmic.enableChargingCallCount == enableBefore + 1); // resumed, completed exactly once
  assert(pmic.setWatchdogCallCount == 1); // watchdog armed on level-2 resume
  assert(pmic.disableChargingCallCount == 1);
}

} // namespace

// pollAndRemediate() uses function-local `static` remediation state that
// persists across every call within one process (by design - it models
// escalation across a device's uptime). Each scenario below therefore runs
// in its OWN process (selected by argv[1]) so its statics start fresh,
// rather than sharing accumulated state with the other scenarios.
int main(int argc, char **argv) {
  const int scenario = (argc > 1) ? atoi(argv[1]) : 0;
  switch (scenario) {
    case 1:
      testIndependentInputFaultIsNotSuppressedByCoincidentNtcFault();
      break;
    case 2:
      testNoFaultBranchCompletesInProgressRemediationBeforeClearing();
      break;
    case 3:
      testFaultRegisterIsReadTwiceAndOred();
      break;
    case 4:
      testNoFaultBranchFreezesLevel1WhileUnsafeThenResumes();
      break;
    case 5:
      testNoFaultBranchFreezesLevel2WhileUnsafeThenResumesWithWatchdog();
      break;
    default:
      fprintf(stderr, "usage: %s <1|2|3|4|5>\n", argv[0]);
      return 2;
  }
  printf("pmic_fault_monitor_test: scenario %d passed\n", scenario);
  return 0;
}
