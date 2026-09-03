#pragma once

/**
 * @file HibernateWakeDiagnostics.h
 * @brief Pure, dependency-free hibernate-wake gate classification and
 *        forensic-event payload construction (WO-2026-08-29-001).
 *
 * @details The hibernate wake-validation gate in
 *          `Generalized-Core-Counter.cpp`'s `setup()` (guarded by
 *          `if (retainedHibernatePending)`) requires ALL of:
 *            1. `reason == RESET_REASON_POWER_MANAGEMENT`
 *            2. `wakeReason == AB1805::WakeReason::ALARM`
 *            3. `rtcReadOk`
 *            4. `retainedHibernateRtcBefore > 0`
 *            5. `retainedHibernateRequestedSleep > 0`
 *            6. `rtcTime >= retainedHibernateRtcBefore`
 *          before it will compute `startupHibernateActualSleepSec` /
 *          `startupHibernateSleepErrorSec` and set
 *          `startupHibernateStatusReady`. When the gate fails, nothing
 *          previously reached the cloud, and serial cannot be trusted to
 *          capture the `Log.info` line on this boot (USB CDC has not
 *          re-enumerated yet) - see the Work Order for the full incident
 *          history.
 *
 *          THIS HEADER DOES NOT CHANGE THE GATE. `classifyGateArm()` mirrors
 *          the exact same boolean short-circuit order as the real `if` in
 *          `setup()`, purely to identify *which* condition failed for
 *          reporting - it must never be used to decide `startupHibernate*`
 *          state itself. No Particle/AB1805 dependency, so this compiles
 *          and tests on the host, the same pattern used by
 *          `power/ChargeInhibitPolicy.h` and `time/ClockTrust.h`.
 */

#ifndef __HIBERNATE_WAKE_DIAGNOSTICS_H
#define __HIBERNATE_WAKE_DIAGNOSTICS_H

#include <stdint.h>
#include <stdio.h>

namespace HibernateWakeDiagnostics {

/// Identifies which of the gate's conditions (in the same order the real
/// `if` at Generalized-Core-Counter.cpp evaluates them) first failed.
/// `kNone` means every condition passed - the gate succeeded.
enum class GateArm {
  kNone = 0,
  kResetReason,    // OS reset reason was not RESET_REASON_POWER_MANAGEMENT
  kWakeReason,     // AB1805 wake reason was not ALARM
  kRtcRead,        // ab1805.getRtcAsTime() failed
  kRtcBeforeZero,  // retainedHibernateRtcBefore was not > 0
  kRequestedZero,  // retainedHibernateRequestedSleep was not > 0
  kRtcOrder,       // rtcTime on wake was earlier than retainedHibernateRtcBefore
};

/// Raw inputs to the gate, exactly as read in setup(). The caller resolves
/// `resetReasonIsPowerManagement` and `wakeReasonIsAlarm` from Device-OS/
/// AB1805 types before calling in, so this header never needs to know
/// those enums.
struct GateInputs {
  bool resetReasonIsPowerManagement;
  bool wakeReasonIsAlarm;
  bool rtcReadOk;
  int64_t rtcBefore;
  uint32_t requestedSleepSec;
  int64_t rtcAtWake;
};

/// Classifies which gate arm failed, mirroring the exact short-circuit
/// order of the real `if` condition. Pure, no I/O.
///
/// ORDERING CONTRACT (must match production exactly - see rationale below):
///   1. `resetReasonIsPowerManagement` false -> `GateArm::kResetReason`
///   2. `wakeReasonIsAlarm`            false -> `GateArm::kWakeReason`
///   3. `rtcReadOk`                    false -> `GateArm::kRtcRead`
///   4. `rtcBefore > 0`                false -> `GateArm::kRtcBeforeZero`
///   5. `requestedSleepSec > 0`        false -> `GateArm::kRequestedZero`
///   6. `rtcAtWake >= rtcBefore`       false -> `GateArm::kRtcOrder`
///      (all six pass)                       -> `GateArm::kNone`
///
/// This order MUST match, position for position, the short-circuit `&&`
/// chain of the real gate `if` in `setup()` at
/// `Generalized-Core-Counter.cpp` (search for
/// `if (reason == RESET_REASON_POWER_MANAGEMENT &&`, currently around
/// line 1221). When more than one condition is false simultaneously, C++
/// `&&` short-circuits left-to-right and only ever evaluates/reports the
/// FIRST false one - so if this function's internal order ever diverges
/// from the real gate's (e.g. an accidental reordering of two `if`s
/// below), it would report the WRONG arm whenever two or more conditions
/// fail on the same boot. That is not a cosmetic bug: identifying which
/// arm the real gate failed on, correctly, is the entire purpose of this
/// Work Order - a forensic event that names the wrong arm is actively
/// misleading, worse than reporting nothing. Any change to either the
/// real gate's condition order or this list must change both together,
/// and a reviewer should be able to check this doc comment against the
/// two code sites directly, rather than re-deriving intended order from
/// either implementation.
inline GateArm classifyGateArm(const GateInputs &in) {
  if (!in.resetReasonIsPowerManagement) {
    return GateArm::kResetReason;
  }
  if (!in.wakeReasonIsAlarm) {
    return GateArm::kWakeReason;
  }
  if (!in.rtcReadOk) {
    return GateArm::kRtcRead;
  }
  if (!(in.rtcBefore > 0)) {
    return GateArm::kRtcBeforeZero;
  }
  if (!(in.requestedSleepSec > 0)) {
    return GateArm::kRequestedZero;
  }
  if (!(in.rtcAtWake >= in.rtcBefore)) {
    return GateArm::kRtcOrder;
  }
  return GateArm::kNone;
}

inline const char *gateArmName(GateArm arm) {
  switch (arm) {
  case GateArm::kNone:
    return "none";
  case GateArm::kResetReason:
    return "reset_reason";
  case GateArm::kWakeReason:
    return "wake_reason";
  case GateArm::kRtcRead:
    return "rtc_read";
  case GateArm::kRtcBeforeZero:
    return "rtc_before_zero";
  case GateArm::kRequestedZero:
    return "requested_zero";
  case GateArm::kRtcOrder:
    return "rtc_order";
  }
  return "unknown";
}

/// Fields for the "hibernate_wake" queued forensic event. Covers both
/// outcomes uniformly (WO requirement 2): `arm == kNone` is success,
/// anything else identifies the failing condition. `actualSleepSec`/
/// `sleepErrorSec` are only well-defined when `arm == kNone` - `buildEventFields()`
/// is the single place that computes and zeroes them appropriately; callers
/// must build this struct through `buildEventFields()` rather than filling
/// it in by hand (buildEventPayload() does not re-check `arm` before
/// formatting them, by design - it always reports whatever the struct
/// holds).
struct EventFields {
  GateArm arm;
  int osResetReason;
  const char *wakeReasonName;
  bool rtcReadOk;
  uint32_t requestedSleepSec;
  int64_t rtcBefore;
  int64_t rtcAtWake;
  uint32_t hibernateCount;
  uint32_t actualSleepSec;
  int32_t sleepErrorSec;
};

/// Builds the EventFields for the "hibernate_wake" event from the classified
/// gate outcome. This is the ONLY place actual/error are combined with the
/// classification: when the gate passed (`arm == kNone`), it reports the
/// already-computed `actualSleepSecOnSuccess`/`sleepErrorSecOnSuccess` (the
/// same values setup() derived from `rtcTime`/`retainedHibernateRtcBefore`
/// and stored in `startupHibernateActualSleepSec`/
/// `startupHibernateSleepErrorSec` for the status payload - passed in
/// verbatim here, not recomputed, so the event and the status payload are
/// guaranteed to agree). When the gate failed, it zeroes both fields rather
/// than report a duration derived from rtc values that never qualified.
/// Called by production immediately after classifyGateArm() so both
/// outcomes run through this single, host-tested function instead of a
/// hand-duplicated branch at the call site.
inline EventFields buildEventFields(const GateInputs &in, int osResetReason,
                                     const char *wakeReasonName, uint32_t hibernateCount,
                                     uint32_t actualSleepSecOnSuccess,
                                     int32_t sleepErrorSecOnSuccess) {
  EventFields f{};
  f.arm = classifyGateArm(in);
  f.osResetReason = osResetReason;
  f.wakeReasonName = wakeReasonName;
  f.rtcReadOk = in.rtcReadOk;
  f.requestedSleepSec = in.requestedSleepSec;
  f.rtcBefore = in.rtcBefore;
  f.rtcAtWake = in.rtcAtWake;
  f.hibernateCount = hibernateCount;
  if (f.arm == GateArm::kNone) {
    f.actualSleepSec = actualSleepSecOnSuccess;
    f.sleepErrorSec = sleepErrorSecOnSuccess;
  } else {
    // Gate failed - rtcAtWake/rtcBefore did not both qualify, so a duration
    // derived from them would not be meaningful. Report 0/0 rather than
    // fabricate a duration.
    f.actualSleepSec = 0;
    f.sleepErrorSec = 0;
  }
  return f;
}

/// Builds the bounded JSON payload for the "hibernate_wake" queued event,
/// mirroring the snprintf-into-fixed-buffer pattern of
/// publishWatchdogForensics(). Returns whatever snprintf() returns (the
/// number of bytes that would have been written, excluding the NUL,
/// possibly >= bufferSize on truncation) so a caller can detect truncation
/// exactly the way the watchdog forensic publisher's snprintf call does.
inline int buildEventPayload(char *buffer, size_t bufferSize, const EventFields &f) {
  const bool ok = f.arm == GateArm::kNone;
  return snprintf(
      buffer, bufferSize,
      "{\"result\":\"%s\",\"gateArm\":\"%s\",\"osReason\":%d,\"wakeReason\":\"%s\","
      "\"rtcOk\":%d,\"req\":%lu,\"rtcBefore\":%lld,\"rtcAt\":%lld,\"actual\":%lu,"
      "\"err\":%ld,\"count\":%lu}",
      ok ? "ok" : "fail", gateArmName(f.arm), f.osResetReason, f.wakeReasonName,
      f.rtcReadOk ? 1 : 0, (unsigned long)f.requestedSleepSec, (long long)f.rtcBefore,
      (long long)f.rtcAtWake, (unsigned long)f.actualSleepSec, (long)f.sleepErrorSec,
      (unsigned long)f.hibernateCount);
}

} // namespace HibernateWakeDiagnostics

#endif
