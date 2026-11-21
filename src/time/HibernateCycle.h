#pragma once

/**
 * @file HibernateCycle.h
 * @brief Single owner of the hibernate-sleep/wake retained-state lifecycle
 *        (WO-2026-09-16, Step 2 of the Structural Ownership Map roadmap).
 *
 * @details Before this step, five retained fields
 *          (`retainedHibernateRtcBefore`, `retainedHibernateWakeTime`,
 *          `retainedHibernateRequestedSleep`, `retainedHibernateCount`,
 *          `retainedHibernatePending`) were declared in
 *          `Generalized-Core-Counter.cpp`, armed by `State_Sleep.cpp`
 *          immediately before a HIBERNATE sleep, and read back by
 *          `Generalized-Core-Counter.cpp` on the wake boot - three files
 *          sharing one lifecycle with no single owner. This header/its
 *          `.cpp` now own all five (four - `retainedHibernateWakeTime` had
 *          no readers anywhere in the tree and was not carried forward).
 *
 *          v50 S2: `classifyWake()` reads the OS reset reason and the
 *          external RTC only; the chip's own wake flags are not used.
 *
 *          The former `HibernateWakeDiagnostics::classifyGateArm()` is not
 *          called from here - its six-condition decision is this file's own
 *          implementation now, not a separate function being wrapped.
 *          `HibernateWakeDiagnostics.h` still owns `GateInputs`/`GateArm`/
 *          `EventFields`/`buildEventFields()`/`buildEventPayload()` - the
 *          shared, Particle-free data and cloud-event-rendering types both
 *          this file and `Generalized-Core-Counter.cpp` use.
 */

#ifndef __HIBERNATE_CYCLE_H
#define __HIBERNATE_CYCLE_H

#include <cstdint>
#include <ctime>

#include "time/HibernateWakeDiagnostics.h"

namespace HibernateCycle {

/// Everything a wake boot needs to know, in one call. The hibernate-gate
/// fields are populated only when a hibernate was actually pending this boot.
struct WakeVerdict {
  // ---- RTC read outcome, populated every boot - needed both for
  // hibernate-gate classification and, independently, by the caller's own
  // "did system time just get restored" check. ----
  bool rtcReadOk = false;
  int64_t rtcAtWake = 0;

  // ---- Hibernate wake-validation gate, populated only when a hibernate
  // was pending this boot. `gateInputs` is exposed so the caller can still
  // build the `hibernate_wake` forensic event via
  // `HibernateWakeDiagnostics::buildEventFields()` without this file
  // needing to know anything about cloud events. ----
  bool hibernateGateEvaluated = false;
  HibernateWakeDiagnostics::GateArm gateArm = HibernateWakeDiagnostics::GateArm::kNone;
  HibernateWakeDiagnostics::GateInputs gateInputs{};
  const char *wakeReasonName = "UNKNOWN";
  uint32_t requestedSleepSec = 0;
  uint32_t hibernateCount = 0;
  uint32_t actualSleepSec = 0;
  int32_t sleepErrorSec = 0;
};

/// Arms the retained state for an upcoming HIBERNATE sleep. Called
/// immediately before `System.sleep(config)` with a HIBERNATE
/// configuration. Replaces the inline
/// `retainedHibernateRtcBefore/RequestedSleep/Count++/Pending` sequence
/// formerly in `State_Sleep.cpp`.
void armForSleep(time_t rtcNow, uint32_t requestedSec);

/// Un-arms a hibernate attempt without processing a wake: either the
/// HIBERNATE `System.sleep()` call returned instead of resetting the MCU
/// (the ULTRA_LOW_POWER fallback path), or the wake boot has finished
/// processing `classifyWake()`'s verdict and the pending flag should not
/// carry into the next cycle. Replaces both
/// `retainedHibernatePending = false;` sites.
void abandon();

/// Classifies this boot's wake in one call - see the struct doc above.
/// `osResetReason` is the OS reset reason (`System.resetReason()`'s
/// result, e.g. `RESET_REASON_PIN_RESET`/`RESET_REASON_POWER_MANAGEMENT`).
WakeVerdict classifyWake(int osResetReason);

/// Current hibernate count, for the one confirmed consumer outside the
/// wake-classification path: `publishStartupStatus()`'s `status` event.
uint32_t currentHibernateCount();

} // namespace HibernateCycle

#endif
