#pragma once

/**
 * @file HibernateWakeDiagnostics.h
 * @brief Pure, dependency-free data types and forensic-event payload
 *        construction for the hibernate wake-validation gate
 *        (WO-2026-08-29-001; WO-2026-09-16 Step 2).
 *
 * @details The gate's six-condition classification itself is owned by
 *          `time/HibernateCycle.cpp`'s `classifyWake()` - this header does
 *          not classify anything anymore. What it still owns is the shared,
 *          Particle-free layer both `HibernateCycle` and
 *          `Generalized-Core-Counter.cpp` need: `GateInputs` (the six raw
 *          conditions), `GateArm` (which one failed, or none), and the
 *          `EventFields`/`buildEventFields()`/`buildEventPayload()` trio
 *          that renders a classification into the "hibernate_wake" cloud
 *          event. When the gate fails, nothing previously reached the
 *          cloud, and serial cannot be trusted to capture the `Log.info`
 *          line on this boot (USB CDC has not re-enumerated yet) - see the
 *          Work Order for the full incident history.
 *
 *          No Particle/AB1805 dependency, so this compiles and tests on the
 *          host, the same pattern used by `power/ChargeInhibitPolicy.h` and
 *          `time/ClockTrust.h`.
 */

#ifndef __HIBERNATE_WAKE_DIAGNOSTICS_H
#define __HIBERNATE_WAKE_DIAGNOSTICS_H

#include <stdint.h>
#include <stdio.h>

namespace HibernateWakeDiagnostics {

/// Identifies which of the gate's six conditions (in the order
/// `HibernateCycle::classifyWake()` evaluates them) first failed. `kNone`
/// means every condition passed - the gate succeeded.
enum class GateArm {
  kNone = 0,
  kResetReason,    // OS reset reason was not RESET_REASON_POWER_MANAGEMENT
  kWakeReason,     // AB1805 wake reason was not ALARM
  kRtcRead,        // ab1805.getRtcAsTime() failed
  kRtcBeforeZero,  // retainedHibernateRtcBefore was not > 0
  kRequestedZero,  // retainedHibernateRequestedSleep was not > 0
  kRtcOrder,       // rtcTime on wake was earlier than retainedHibernateRtcBefore
};

/// Raw inputs to the gate, exactly as `HibernateCycle::classifyWake()`
/// reads them. The caller resolves `resetReasonIsPowerManagement` and
/// `wakeReasonIsAlarm` from Device-OS/AB1805 types before populating this,
/// so this header never needs to know those enums.
struct GateInputs {
  bool resetReasonIsPowerManagement;
  bool wakeReasonIsAlarm;
  bool rtcReadOk;
  int64_t rtcBefore;
  uint32_t requestedSleepSec;
  int64_t rtcAtWake;
};

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

/// Builds the EventFields for the "hibernate_wake" event from an already-
/// classified gate outcome (`arm` - computed once, by `HibernateCycle`'s
/// `classifyWake()`, and passed in rather than re-derived here; this
/// function used to call a `classifyGateArm()` in this same header to get
/// it, which meant the classification ran twice per boot for no reason -
/// removed in WO-2026-09-16 Step 2). This is the ONLY place actual/error
/// are combined with the classification: when the gate passed
/// (`arm == kNone`), it reports the already-computed
/// `actualSleepSecOnSuccess`/`sleepErrorSecOnSuccess` (the same values
/// `classifyWake()` derived and the caller stored in
/// `startupHibernateActualSleepSec`/`startupHibernateSleepErrorSec` for the
/// status payload - passed in verbatim here, not recomputed, so the event
/// and the status payload are guaranteed to agree). When the gate failed,
/// it zeroes both fields rather than report a duration derived from rtc
/// values that never qualified.
inline EventFields buildEventFields(GateArm arm, const GateInputs &in, int osResetReason,
                                     const char *wakeReasonName, uint32_t hibernateCount,
                                     uint32_t actualSleepSecOnSuccess,
                                     int32_t sleepErrorSecOnSuccess) {
  EventFields f{};
  f.arm = arm;
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
