#include "Particle.h"
#include "time/HibernateCycle.h"

#include "time/ExternalRtc.h"

namespace HibernateCycle {

namespace {

// These four retained fields lived in Generalized-Core-Counter.cpp before
// this step, keeping their original identifiers (not renamed - this is a
// move, not a rewrite) so a repo-wide search for retainedHibernate* still
// finds them, now confined to this one file.
// retainedHibernateWakeTime (unused by any reader anywhere in the tree -
// confirmed by a full-tree search during this step's baseline
// investigation) was not carried forward.
retained time_t retainedHibernateRtcBefore = 0;
retained uint32_t retainedHibernateRequestedSleep = 0;
retained uint32_t retainedHibernateCount = 0;
retained bool retainedHibernatePending = false;

} // namespace

void armForSleep(time_t rtcNow, uint32_t requestedSec) {
  retainedHibernateRtcBefore = rtcNow;
  retainedHibernateRequestedSleep = requestedSec;
  retainedHibernateCount++;
  retainedHibernatePending = true;
}

void abandon() {
  retainedHibernatePending = false;
}

uint32_t currentHibernateCount() {
  return retainedHibernateCount;
}

WakeVerdict classifyWake(int osResetReason) {
  WakeVerdict v{};

  // ===== RTC READ - every boot, independent of the above =====
  time_t rtcTime = 0;
  v.rtcReadOk = ExternalRtc::get(rtcTime);
  v.rtcAtWake = (int64_t)rtcTime;

  // ===== HIBERNATE WAKE-VALIDATION GATE =====
  // This IS the classification now - not a call into a separately
  // maintained mirror. All six conditions below are evaluated directly,
  // in the same short-circuit order as before this step (WO-2026-09-14-002
  // Step 1 first consolidated a hand-written `if` chain and a mirrored
  // classifyGateArm() into one call site; this step consolidates further,
  // absorbing the classification itself so there is exactly one place this
  // logic is written, not a function elsewhere being wrapped).
  if (retainedHibernatePending) {
    v.hibernateGateEvaluated = true;
    HibernateWakeDiagnostics::GateInputs &in = v.gateInputs;
    in.resetReasonIsPowerManagement = (osResetReason == RESET_REASON_POWER_MANAGEMENT);
    in.rtcReadOk = v.rtcReadOk;
    in.rtcBefore = (int64_t)retainedHibernateRtcBefore;
    in.requestedSleepSec = retainedHibernateRequestedSleep;
    in.rtcAtWake = v.rtcAtWake;
    // Reason 30 alone passes the wake-reason arm (WO-2026-11-05-004 Amendment 3).
    in.wakeReasonIsAlarm = in.resetReasonIsPowerManagement;
    if (in.resetReasonIsPowerManagement) v.wakeReasonName = "POWER_MGMT";

    using HibernateWakeDiagnostics::GateArm;
    if (!in.resetReasonIsPowerManagement) {
      v.gateArm = GateArm::kResetReason;
    } else if (!in.wakeReasonIsAlarm) {
      v.gateArm = GateArm::kWakeReason;
    } else if (!in.rtcReadOk) {
      v.gateArm = GateArm::kRtcRead;
    } else if (!(in.rtcBefore > 0)) {
      v.gateArm = GateArm::kRtcBeforeZero;
    } else if (!(in.requestedSleepSec > 0)) {
      v.gateArm = GateArm::kRequestedZero;
    } else if (!(in.rtcAtWake >= in.rtcBefore)) {
      v.gateArm = GateArm::kRtcOrder;
    } else {
      v.gateArm = GateArm::kNone;
    }

    v.requestedSleepSec = retainedHibernateRequestedSleep;
    v.hibernateCount = retainedHibernateCount;

    if (v.gateArm == GateArm::kNone) {
      v.actualSleepSec = (uint32_t)(rtcTime - retainedHibernateRtcBefore);
      v.sleepErrorSec = (int32_t)v.actualSleepSec - (int32_t)retainedHibernateRequestedSleep;
    }
  }

  return v;
}

} // namespace HibernateCycle
