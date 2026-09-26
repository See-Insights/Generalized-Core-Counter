/**
 * @file PublishDeliveryGate.cpp
 * @brief Device-side adapter for the bounded delivery wait.
 *
 * @details See PublishDeliveryGate.h. The decision itself lives in
 *          PublishDeliveryBudget.cpp, which is free of Particle and the publish
 *          queue so it can be host-tested directly; this file only supplies the
 *          live inputs and does the logging.
 *
 *          Threading: called from the application thread only (the IDLE sleep
 *          gate, the IDLE connectivity ceiling and the SLEEPING_STATE
 *          cloud-sync gate, all inside `loop()`).
 */

#include "Particle.h"

#include "cloud/PublishDeliveryGate.h"

#include "PublishQueuePosixRK.h"
#include "cloud/PublishDeliveryBudget.h"

namespace PublishDeliveryGate {

bool publishInFlight() {
  return PublishQueuePosix::instance().getPublishInFlight();
}

bool queuePermitsSleep() {
  PublishDeliveryBudget::Inputs in;
  in.cloudConnected = Particle.connected();
  in.queueSleepSafe = PublishQueuePosix::instance().getCanSleep();
  in.publishInFlight = PublishQueuePosix::instance().getPublishInFlight();
  in.queuedEvents = PublishQueuePosix::instance().getNumEvents();
  in.nowMs = millis();

  const PublishDeliveryBudget::Verdict verdict = PublishDeliveryBudget::evaluate(in);

  if (verdict.expiryFirstObserved) {
    // WO-2026-09-25-001 Stage 5 decision 6: every budget expiry is logged, with
    // the queued count, the elapsed time and whether a publish was in flight.
    // A device that starts sleeping with events queued must say so in a plain
    // USB capture - `sleptWithQueued` in the status event counts how often.
    Log.info("DeliveryBudget: expired budget=%lums elapsed=%lums qn=%u inflight=%d",
             PublishDeliveryBudget::budgetMs(),
             verdict.elapsedMs,
             (unsigned)in.queuedEvents,
             in.publishInFlight ? 1 : 0);
  }

  return verdict.sleepPermitted;
}

} // namespace PublishDeliveryGate
