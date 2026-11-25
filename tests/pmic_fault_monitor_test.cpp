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
  pmic.readPowerOnCallCount = 0;
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
                                        /*powerSource=*/0, /*chargeDisableConfigVerified=*/false, /*vcell=*/3.9f);
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
                                        /*powerSource=*/0, /*chargeDisableConfigVerified=*/false, /*vcell=*/3.9f);
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
                                      /*powerSource=*/0, /*chargeDisableConfigVerified=*/false, /*vcell=*/3.9f);

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
                                          /*powerSource=*/0, /*chargeDisableConfigVerified=*/false, /*vcell=*/3.9f);

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
                                        /*powerSource=*/0, /*chargeDisableConfigVerified=*/false, /*vcell=*/3.9f);
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
                                        /*powerSource=*/0, /*chargeDisableConfigVerified=*/true, /*vcell=*/3.9f);
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
                                      /*powerSource=*/0, /*chargeDisableConfigVerified=*/false, /*vcell=*/3.9f);
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
                                        /*powerSource=*/0, /*chargeDisableConfigVerified=*/false, /*vcell=*/3.9f);
    pmic.faultCallCount = 0;
    pmic.faultRegQueue[0] = pmic.faultRegQueue[1] = 0x20;
    g_testMillis += 10;
  }
  for (int i = 0; i < 2; i++) {
    PmicFaultMonitor::pollAndRemediate(pmic, /*safeToCharge=*/true, /*battState=*/0,
                                        /*powerSource=*/0, /*chargeDisableConfigVerified=*/false, /*vcell=*/3.9f);
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
                                      /*powerSource=*/0, /*chargeDisableConfigVerified=*/true, /*vcell=*/3.9f);
  assert(pmic.enableChargingCallCount == enableBefore); // still frozen
  assert(pmic.setWatchdogCallCount == 0); // watchdog only set on actual resume
  assert(pmic.disableChargingCallCount == 1); // sequence not abandoned/restarted

  pmic.faultCallCount = 0;
  pmic.faultRegQueue[0] = pmic.faultRegQueue[1] = 0x00;
  PmicFaultMonitor::pollAndRemediate(pmic, /*safeToCharge=*/true, /*battState=*/0,
                                      /*powerSource=*/0, /*chargeDisableConfigVerified=*/false, /*vcell=*/3.9f);
  assert(pmic.enableChargingCallCount == enableBefore + 1); // resumed, completed exactly once
  assert(pmic.setWatchdogCallCount == 1); // watchdog armed on level-2 resume
  assert(pmic.disableChargingCallCount == 1);
}

// --- WO-2026-11-02-001 (Amendment 1): fault-free USB "Not Charging" latch.
// Driven through the production pollAndRemediate(); its latch state is
// function-local static, so each scenario walks fresh windows sequentially. ---

constexpr unsigned long kH = 3600000UL;
constexpr unsigned long kSettleGap = 25UL * kH; // > the 24 h attempt limit and the 1 h cooldown
constexpr uint8_t kUsbNotCharging = 0x44;       // REG08: VBUS=USB host, Not Charging, power good

struct LatchInput {
  uint8_t status = kUsbNotCharging;
  uint8_t fault = 0;
  float vcell = 3.75f;
  bool safe = true;
  int source = 2; // USB host
};

// One pollAndRemediate() at time t. True if this poll fired the latch
// (raised alert 22 and requested a cycle).
bool latchPoll(PMIC &pmic, unsigned long t, const LatchInput &in = LatchInput()) {
  g_testMillis = t;
  pmic.faultRegQueue[0] = in.fault;
  pmic.faultRegQueue[1] = in.fault;
  pmic.faultCallCount = 0;
  pmic.systemStatusValue = in.status;
  const int raisedBefore = testCurrent.raiseAlertCallCount;
  PmicFaultMonitor::pollAndRemediate(pmic, in.safe, /*battState=*/0, in.source,
                                      /*chargeDisableConfigVerified=*/false, in.vcell);
  return testCurrent.raiseAlertCallCount > raisedBefore && testCurrent.lastRaisedAlertCode == 22;
}

// Breaks the latch timer, lets any requested cycle run to completion
// (phase 0, then re-enable after 500 ms) and returns a start time past the
// 24 h attempt limit and the 1 h cooldown: a fresh latch window.
unsigned long freshWindow(PMIC &pmic, unsigned long t) {
  LatchInput off;
  off.status = 0x00; // no input
  latchPoll(pmic, t, off);
  latchPoll(pmic, t + 600UL, off);
  latchPoll(pmic, t + 1200UL, off);
  return t + kSettleGap;
}

// Stuck from t0: no action before 2 h, exactly one at 2 h.
void assertFiresAtTwoHours(PMIC &pmic, unsigned long t0, const LatchInput &in) {
  assert(!latchPoll(pmic, t0, in));
  assert(!latchPoll(pmic, t0 + 2 * kH - 1, in));
  assert(latchPoll(pmic, t0 + 2 * kH, in));
}

void assertNeverFires(PMIC &pmic, unsigned long t0, const LatchInput &in) {
  for (unsigned long t = t0; t <= t0 + 3 * kH; t += kH / 2) assert(!latchPoll(pmic, t, in));
}

// 2 h boundary (>=), one cycle, then the 24 h per-attempt limit while the
// condition stays continuously true.
void testLatchBoundaryAndAttemptLimit(PMIC &pmic, unsigned long t0) {
  const LatchInput stuck;
  assertFiresAtTwoHours(pmic, t0, stuck);
  const unsigned long fired = t0 + 2 * kH;
  assert(pmic.readPowerOnCallCount == 1); // REG01 read once, for the one Log.warn
  assert(!latchPoll(pmic, fired + 1, stuck)); // cycle phase 0
  assert(pmic.disableChargingCallCount == 1);
  assert(!latchPoll(pmic, fired + 600UL, stuck)); // re-enable completes the cycle
  assert(pmic.enableChargingCallCount == 1);
  assert(!latchPoll(pmic, fired + 2 * kH, stuck)); // cooldown over, 24 h limit holds
  assert(!latchPoll(pmic, fired + 24 * kH - 1, stuck));
  assert(latchPoll(pmic, fired + 24 * kH, stuck));
  assert(pmic.readPowerOnCallCount == 2);
}

// Voltage clause is only < 4.05 V: no lower bound (a deeply discharged cell
// still qualifies), 4.05 V excluded.
void testLatchVoltageThreshold(PMIC &pmic, unsigned long &t) {
  LatchInput in;
  in.vcell = 4.049f;
  assertFiresAtTwoHours(pmic, t, in);
  t = freshWindow(pmic, t + 2 * kH + 1);
  in.vcell = 2.0f;
  assertFiresAtTwoHours(pmic, t, in);
  t = freshWindow(pmic, t + 2 * kH + 1);
  in.vcell = 4.05f;
  assertNeverFires(pmic, t, in);
  t = freshWindow(pmic, t + 3 * kH + 1);
}

// USB gating: corrected USB source AND VBUS 1/2. VBUS=2 (adapter) qualifies;
// raw VBUS=USB with a non-USB corrected source (VIN, battery) never does.
void testLatchUsbGating(PMIC &pmic, unsigned long &t) {
  LatchInput in;
  in.status = 0x84; // VBUS=adapter
  assertFiresAtTwoHours(pmic, t, in);
  t = freshWindow(pmic, t + 2 * kH + 1);
  in.status = kUsbNotCharging;
  in.source = 3; // USB adapter
  assertFiresAtTwoHours(pmic, t, in);
  t = freshWindow(pmic, t + 2 * kH + 1);
  for (int source : {1, 5, -1}) { // VIN, battery, unknown
    in.source = source;
    assertNeverFires(pmic, t, in);
    t = freshWindow(pmic, t + 3 * kH + 1);
  }
}

// Each clause breaking once (at +1 h) restarts the timer: no action at the
// original 2 h mark, one action 2 h after the clause is restored.
void testLatchEachClauseBreakResetsTimer(PMIC &pmic, unsigned long &t) {
  for (int clause = 0; clause < 13; clause++) {
    LatchInput bad;
    switch (clause) {
      case 0: bad.status = 0x04; break;  // no VBUS
      case 1: bad.status = 0xC4; break;  // OTG is not input
      case 2: bad.status = 0x40; break;  // power not good
      case 3: bad.status = 0x54; break;  // Pre-charge
      case 4: bad.status = 0x64; break;  // Fast
      case 5: bad.status = 0x74; break;  // Done
      case 6: bad.status = 0x46; break;  // thermal regulation
      case 7: bad.fault = 0x10; break;   // CHRG_FAULT
      case 8: bad.fault = 0x08; break;   // BAT_FAULT
      case 9: bad.vcell = 4.05f; break;
      case 10: bad.safe = false; break;
      case 11: bad.source = 1; break;    // corrected source VIN
      case 12: bad.status = 0x84; bad.source = 5; break; // VBUS adapter, source battery
    }
    const LatchInput good;
    assert(!latchPoll(pmic, t, good));
    assert(!latchPoll(pmic, t + kH, bad));
    assert(!latchPoll(pmic, t + kH + 1, good));
    assert(!latchPoll(pmic, t + 2 * kH, good));
    assert(!latchPoll(pmic, t + 3 * kH, good));
    assert(latchPoll(pmic, t + 3 * kH + 1, good));
    t = freshWindow(pmic, t + 3 * kH + 2);
  }
}

// millis() wrap-around: the timer straddles the unsigned long wrap (host
// unsigned long is 64-bit, so this wraps at 2^64; the device wraps at 2^32 -
// both rely on the same modular unsigned subtraction).
void testLatchAcrossMillisWrap(PMIC &pmic) {
  const unsigned long t0 = ~0UL - kH / 2; // 30 min before the wrap
  assertFiresAtTwoHours(pmic, t0, LatchInput());
  assert(t0 + 2 * kH < t0); // the firing time really is past the wrap
}

void testLatchScenario() {
  PMIC pmic;
  resetPmic(pmic);
  resetCurrent();
  unsigned long t = 1000UL;
  testLatchBoundaryAndAttemptLimit(pmic, t);
  t = freshWindow(pmic, t + 26 * kH + 1);
  testLatchVoltageThreshold(pmic, t);
  testLatchUsbGating(pmic, t);
  testLatchEachClauseBreakResetsTimer(pmic, t);
  testLatchAcrossMillisWrap(pmic);
}

// Alert 22 (not 21) is raised; it stays while Not Charging persists and is
// cleared by the existing "charging resumed, codes 20-23" clear once the
// charger reports Pre-charge, Fast or Done.
void testLatchAlert22ClearsWhenChargingResumes() {
  PMIC pmic;
  resetPmic(pmic);
  resetCurrent();
  unsigned long t = 1000UL;
  const LatchInput stuck;
  for (uint8_t resumed : {0x54, 0x64, 0x74}) { // Pre-charge, Fast, Done
    const int raisedBefore = testCurrent.raiseAlertCallCount;
    assertFiresAtTwoHours(pmic, t, stuck);
    assert(testCurrent.alertCodeValue == 22 && testCurrent.lastRaisedAlertCode == 22);
    assert(testCurrent.raiseAlertCallCount == raisedBefore + 1);
    const unsigned long fired = t + 2 * kH;
    latchPoll(pmic, fired + 1, stuck);      // phase 0
    latchPoll(pmic, fired + 600UL, stuck);  // re-enable
    latchPoll(pmic, fired + 1200UL, stuck); // still Not Charging
    assert(testCurrent.alertCodeValue == 22);
    LatchInput charging;
    charging.status = resumed;
    latchPoll(pmic, fired + 1800UL, charging);
    assert(testCurrent.alertCodeValue == 0);
    assert(testCurrent.raiseAlertCallCount == raisedBefore + 1);
    t = fired + kSettleGap;
  }
  assert(pmic.disableChargingCallCount == 3 && pmic.enableChargingCallCount == 3);
}

// The legacy alert-21 clear is unchanged: cleared on any no-fault poll
// whose charge status is not Fast (2) - including Not Charging - and the
// latch never raises or holds 21.
void testAlert21ClearUnchanged() {
  PMIC pmic;
  resetPmic(pmic);
  resetCurrent();
  LatchInput in;
  in.source = 1; // keep the latch out of it
  unsigned long t = 1000UL;
  in.status = 0x64; // Fast keeps 21
  testCurrent.alertCodeValue = 21;
  latchPoll(pmic, t++, in);
  assert(testCurrent.alertCodeValue == 21);
  for (uint8_t status : {0x44, 0x54, 0x74}) { // Not Charging, Pre-charge, Done clear 21
    testCurrent.alertCodeValue = 21;
    in.status = status;
    latchPoll(pmic, t++, in);
    assert(testCurrent.alertCodeValue == 0);
  }
  assert(testCurrent.raiseAlertCallCount == 0);
}

// WO-2026-11-11-001 item 3: 20 raised by an NTC fault alone, and 23 raised by
// BAT_FAULT alone, never touch consecutiveFaults, so they clear on their own
// no-fault predicates.
void testAlert20FromNtcAloneClearsWhenNtcClears() {
  PMIC pmic;
  resetPmic(pmic);
  resetCurrent();
  LatchInput in;
  in.source = 1; // keep the latch out of it
  in.status = 0x54;
  in.fault = 0x01; // NTC fault only
  unsigned long t = 1000UL;
  latchPoll(pmic, t++, in);
  assert(testCurrent.alertCodeValue == 20);
  latchPoll(pmic, t++, in);
  assert(testCurrent.alertCodeValue == 20); // still faulted: stays
  in.fault = 0;
  latchPoll(pmic, t++, in);
  assert(testCurrent.alertCodeValue == 0);
  assert(testCurrent.lastAlertTimeValue == 0);
}

void testAlert23FromBatFaultAloneClearsWhenBatFaultClears() {
  PMIC pmic;
  resetPmic(pmic);
  resetCurrent();
  LatchInput in;
  in.source = 1;
  in.status = 0x54;
  in.fault = 0x08; // BAT_FAULT only
  unsigned long t = 1000UL;
  latchPoll(pmic, t++, in);
  assert(testCurrent.alertCodeValue == 23);
  latchPoll(pmic, t++, in);
  assert(testCurrent.alertCodeValue == 23);
  in.fault = 0;
  latchPoll(pmic, t++, in);
  assert(testCurrent.alertCodeValue == 0);
  assert(testCurrent.lastAlertTimeValue == 0);
}

// WO-2026-11-11-001 Amendment 1 (2a): a thermal CHRG_FAULT 20 must outlive the
// first clear poll while its remediation cycle is unfinished, and clear only
// after the cycle completes and one further clean poll confirms recovery.
void testAlert20ThermalHeldThroughRemediationCycle() {
  PMIC pmic;
  resetPmic(pmic);
  resetCurrent();
  LatchInput fault;
  fault.source = 1; // keep the latch out of it
  fault.status = 0x54;
  fault.fault = 0x20; // thermal shutdown
  LatchInput clean = fault;
  clean.fault = 0;
  unsigned long t = 4000000UL; // past the remediation cooldown
  latchPoll(pmic, t++, fault);
  assert(testCurrent.alertCodeValue == 20);
  latchPoll(pmic, t++, fault); // arms level 1
  latchPoll(pmic, t++, fault); // phase 0
  assert(pmic.disableChargingCallCount == 1 && pmic.enableChargingCallCount == 0);
  latchPoll(pmic, t++, clean); // first clear poll, cycle still waiting to re-enable
  assert(pmic.enableChargingCallCount == 0);
  assert(testCurrent.alertCodeValue == 20);
  t += 600UL;
  latchPoll(pmic, t++, clean); // completes the cycle
  assert(pmic.enableChargingCallCount == 1);
  assert(testCurrent.alertCodeValue == 20);
  latchPoll(pmic, t++, clean); // one further clean poll
  assert(testCurrent.alertCodeValue == 0);
}

// Amendment 1 (2b): a combined CHRG+NTC poll raises 20; the next poll showing
// only the NTC fault must not clear it via the "charging resumed" clear.
void testAlert20HeldWhileNtcActiveAfterCombinedFault() {
  // Any nonzero NTC_FAULT code (bits 2:0, kPmicNtcFaultMask 0x07) must hold 20;
  // three representative codes so a guard that protects only one code fails.
  const uint8_t ntcCodes[] = {0x01, 0x05, 0x07};
  for (uint8_t ntc : ntcCodes) {
    PMIC pmic;
    resetPmic(pmic);
    resetCurrent();
    LatchInput in;
    in.source = 1;
    in.status = 0x54;
    in.fault = 0x20 | ntc; // thermal CHRG_FAULT + NTC fault
    unsigned long t = 4000000UL;
    latchPoll(pmic, t++, in);
    assert(testCurrent.alertCodeValue == 20);
    in.fault = ntc; // NTC only
    latchPoll(pmic, t++, in);
    assert(testCurrent.alertCodeValue == 20);
    in.fault = 0;
    latchPoll(pmic, t++, in);
    assert(testCurrent.alertCodeValue == 0);
  }
}

// WO-2026-11-11-001 Stage 6 r3: the consecutive-fault counter saturates, so a
// cycle frozen under a persistent thermal fault for 256+ polls cannot wrap it
// to zero and let guard 2a clear alert 20 before charging is re-enabled.
void testAlert20HeldWhenFrozenFaultCounterWouldWrap() {
  PMIC pmic;
  resetPmic(pmic);
  resetCurrent();
  LatchInput fault;
  fault.source = 1;
  fault.status = 0x54;
  fault.fault = 0x20; // thermal shutdown
  LatchInput clean = fault;
  clean.fault = 0;
  unsigned long t = 4000000UL;
  latchPoll(pmic, t++, fault);
  latchPoll(pmic, t++, fault); // arms level 1
  latchPoll(pmic, t++, fault); // phase 0: charging disabled
  assert(pmic.disableChargingCallCount == 1 && pmic.enableChargingCallCount == 0);
  fault.safe = false;
  clean.safe = false;
  t += 600UL;
  for (int i = 0; i < 509; i++) { // 3 + 509 faults wraps a uint8_t counter to 0
    latchPoll(pmic, t++, fault); // frozen under the persistent fault
    assert(testCurrent.alertCodeValue == 20);
    assert(pmic.disableChargingCallCount == 1 && pmic.enableChargingCallCount == 0);
  }
  latchPoll(pmic, t++, clean); // CHRG clears, cycle still frozen
  assert(testCurrent.alertCodeValue == 20);
  assert(pmic.disableChargingCallCount == 1 && pmic.enableChargingCallCount == 0);
  clean.safe = true;
  latchPoll(pmic, t++, clean); // cycle completes
  assert(pmic.enableChargingCallCount == 1);
  assert(testCurrent.alertCodeValue == 20);
  latchPoll(pmic, t++, clean);
  assert(testCurrent.alertCodeValue == 0);
}

// WO-2026-11-11-001 item 4: the 6 h no-gain alert 21 is skipped on a full pack
// (vcell >= 4.05 V); 4.04 V still fires. Driven through trackAndReport().
bool stuckFastChargeFires(float vcell) {
  resetCurrent();
  PmicFaultMonitor::Registers regs;
  regs.chargeStatus = 0;
  g_testMillis = 1000UL;
  PmicFaultMonitor::trackAndReport(regs, 0, 50.0f, vcell, 50.0f); // resets the window
  regs.chargeStatus = 2;
  PmicFaultMonitor::trackAndReport(regs, 0, 50.0f, vcell, 50.0f); // window starts
  g_testMillis = 1000UL + 6UL * kH + 1UL;
  PmicFaultMonitor::trackAndReport(regs, 0, 50.0f, vcell, 50.0f);
  return testCurrent.alertCodeValue == 21;
}

void testAlert21SkippedOnFullPack() {
  assert(stuckFastChargeFires(4.04f));
  assert(!stuckFastChargeFires(4.05f));
  assert(!stuckFastChargeFires(4.16f));
}

// A latch request inside the 1 h cooldown after a remediation attempt does
// not start a second cycle: an immediate charge-fault reset that reads back
// clear leaves the mature latch timer running, so without the cooldown the
// latch would cycle charging on that same poll.
void testLatchHonoursRemediationCooldown() {
  PMIC pmic;
  resetPmic(pmic);
  resetCurrent();
  const unsigned long t0 = 1000UL;
  const LatchInput stuck;
  assert(!latchPoll(pmic, t0, stuck));
  assert(!latchPoll(pmic, t0 + kH, stuck));

  // At the 2 h mark: CHRG_FAULT on the first read pair, clear after the
  // immediate reset's re-read (stub returns faultRegQueue[1] from then on).
  const unsigned long reset = t0 + 2 * kH;
  g_testMillis = reset;
  pmic.faultRegQueue[0] = 0x10;
  pmic.faultRegQueue[1] = 0x00;
  pmic.faultCallCount = 0;
  pmic.systemStatusValue = kUsbNotCharging;
  PmicFaultMonitor::pollAndRemediate(pmic, true, 0, 2, false, 3.75f);
  assert(pmic.faultCallCount == 4); // immediate reset ran and re-read
  assert(pmic.disableChargingCallCount == 1 && pmic.enableChargingCallCount == 1);
  assert(testCurrent.raiseAlertCallCount == 0);

  assert(!latchPoll(pmic, reset + 1, stuck));
  assert(!latchPoll(pmic, reset + kH / 2, stuck));
  assert(!latchPoll(pmic, reset + kH, stuck)); // cooldown is "more than 1 h"
  assert(pmic.disableChargingCallCount == 1); // no second cycle started
  assert(latchPoll(pmic, reset + kH + 1, stuck));
  assert(!latchPoll(pmic, reset + kH + 2, stuck));
  assert(pmic.disableChargingCallCount == 2);
}

// Fires the latch from t0 and completes its cycle with a Pre-charge poll
// (chargeStatus 1) while the cycle is still running. Returns the completion
// time (the cooldown start).
unsigned long fireLatchAndResumeDuringCycle(PMIC &pmic, unsigned long t0) {
  assertFiresAtTwoHours(pmic, t0, LatchInput());
  const unsigned long fired = t0 + 2 * kH;
  const int disableBefore = pmic.disableChargingCallCount;
  const int enableBefore = pmic.enableChargingCallCount;
  latchPoll(pmic, fired + 1, LatchInput()); // phase 0
  assert(pmic.disableChargingCallCount == disableBefore + 1);
  LatchInput charging;
  charging.status = 0x54; // Pre-charge, while the cycle runs
  latchPoll(pmic, fired + 600UL, charging);
  assert(pmic.enableChargingCallCount == enableBefore + 1); // cycle completed on this poll
  assert(testCurrent.alertCodeValue == 22);
  return fired + 600UL;
}

// Charging resuming while the latch's own cycle runs must not leave a
// counter behind: (i) a later Not Charging poll keeps alert 22, and (ii) a
// later first real charge fault still needs 2 consecutive faults before a
// level-1 cycle starts.
void testLatchResumeDuringCycleLeavesNoCounter() {
  PMIC pmic;
  resetPmic(pmic);
  resetCurrent();
  const LatchInput stuck;
  unsigned long done = fireLatchAndResumeDuringCycle(pmic, 1000UL);
  latchPoll(pmic, done + 600UL, stuck);
  assert(testCurrent.alertCodeValue == 22); // (i)
  LatchInput charging;
  charging.status = 0x54;
  latchPoll(pmic, done + 1200UL, charging);
  assert(testCurrent.alertCodeValue == 0); // no cycle running: cleared on the same call

  done = fireLatchAndResumeDuringCycle(pmic, freshWindow(pmic, done + 1800UL));
  LatchInput fault = stuck;
  fault.fault = 0x20; // thermal CHRG_FAULT: no immediate reset
  const int disableBefore = pmic.disableChargingCallCount;
  latchPoll(pmic, done + kH + 1, fault); // first sighting, past the cooldown
  latchPoll(pmic, done + kH + 2, fault); // second: requests level 1
  assert(pmic.disableChargingCallCount == disableBefore); // (ii)
  latchPoll(pmic, done + kH + 3, fault); // phase 0
  assert(pmic.disableChargingCallCount == disableBefore + 1);
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
    case 6:
      testLatchScenario();
      break;
    case 7:
      testLatchAlert22ClearsWhenChargingResumes();
      break;
    case 8:
      testAlert21ClearUnchanged();
      break;
    case 9:
      testLatchHonoursRemediationCooldown();
      break;
    case 10:
      testLatchResumeDuringCycleLeavesNoCounter();
      break;
    case 11:
      testAlert20FromNtcAloneClearsWhenNtcClears();
      break;
    case 12:
      testAlert23FromBatFaultAloneClearsWhenBatFaultClears();
      break;
    case 13:
      testAlert21SkippedOnFullPack();
      break;
    case 14:
      testAlert20ThermalHeldThroughRemediationCycle();
      break;
    case 15:
      testAlert20HeldWhileNtcActiveAfterCombinedFault();
      break;
    case 16:
      testAlert20HeldWhenFrozenFaultCounterWouldWrap();
      break;
    default:
      fprintf(stderr, "usage: %s <1-16>\n", argv[0]);
      return 2;
  }
  printf("pmic_fault_monitor_test: scenario %d passed\n", scenario);
  return 0;
}
