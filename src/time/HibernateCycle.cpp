#include "Particle.h"
#include "time/HibernateCycle.h"

#include "AB1805_RK.h"

// Defined in Generalized-Core-Counter.cpp - not Boron/hibernate-specific
// (the AB1805 chip is used on every supported platform), so it stays
// defined in one place rather than duplicated here.
extern const char *ab1805WakeReasonName(AB1805::WakeReason reason);

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

WakeVerdict classifyWake(int osResetReason, AB1805 &rtc) {
  WakeVerdict v{};

  // ===== PIN_RESET / AB1805 WATCHDOG CONFIRMATION =====
  // Device OS reports an external MCU reset (e.g. the carrier board's
  // AB1805 watchdog firing via the reset pin) as PIN_RESET, not WATCHDOG -
  // so the only way to confirm the AB1805 was the cause is to ask it
  // directly, via the same getWakeReason() getter the hibernate gate below
  // also uses.
  //
  // IMPORTANT: this relies on AB1805::getWakeReason() being a plain getter
  // (returns a cached member, see AB1805_RK.h) onto the result of the ONE
  // destructive updateWakeReason() call AB1805::setup() already made
  // earlier in setup() - updateWakeReason() itself clears the status bit
  // it classifies, so a second call to IT (not getWakeReason()) would
  // silently overwrite a correct WATCHDOG classification on a combined-bit
  // status read. Calling getWakeReason() here and again below for the
  // hibernate gate is safe and idempotent because neither call is
  // updateWakeReason() - both just read the one already-classified result.
  if (osResetReason == RESET_REASON_PIN_RESET) {
    v.pinResetChecked = true;
    const AB1805::WakeReason pinResetWakeReason = rtc.getWakeReason();
    v.pinResetWakeReasonName = ab1805WakeReasonName(pinResetWakeReason);

    if (pinResetWakeReason == AB1805::WakeReason::WATCHDOG) {
      v.pinResetConfirmedWatchdog = true;
    } else if (pinResetWakeReason == AB1805::WakeReason::UNKNOWN) {
      // Explicitly inconclusive - do NOT treat UNKNOWN as "not the AB1805".
      v.pinResetWakeReasonUnknown = true;
    }
  }

  // ===== RTC READ - every boot, independent of the above =====
  time_t rtcTime = 0;
  v.rtcReadOk = rtc.getRtcAsTime(rtcTime);
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
    const AB1805::WakeReason wakeReason = rtc.getWakeReason();
    v.wakeReasonName = ab1805WakeReasonName(wakeReason);

    HibernateWakeDiagnostics::GateInputs &in = v.gateInputs;
    in.resetReasonIsPowerManagement = (osResetReason == RESET_REASON_POWER_MANAGEMENT);
    in.wakeReasonIsAlarm = (wakeReason == AB1805::WakeReason::ALARM);
    in.rtcReadOk = v.rtcReadOk;
    in.rtcBefore = (int64_t)retainedHibernateRtcBefore;
    in.requestedSleepSec = retainedHibernateRequestedSleep;
    in.rtcAtWake = v.rtcAtWake;

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
