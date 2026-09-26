/**
 * @file PublishDeliveryBudget.cpp
 * @brief Bounded delivery wait - the decision logic only.
 *
 * @details See PublishDeliveryBudget.h for the rule, the budget's start
 *          condition and why the budget is a compile-time constant.
 *
 *          This file deliberately includes nothing from Particle or the publish
 *          queue, so the host test compiles the shipped logic itself (together
 *          with the shipped policy constant) rather than a mirror of it. The
 *          device-side adapter lives in PublishDeliveryGate.cpp.
 *
 *          Threading: evaluated from the application thread only - the IDLE
 *          sleep gate, the IDLE connectivity ceiling and the SLEEPING_STATE
 *          cloud-sync gate - so the module state below needs no locking.
 */

#include "cloud/PublishDeliveryBudget.h"

#include "power/ConnectivityPolicy.h"

namespace {

/// millis() at which the budget started; only meaningful while budgetRunning.
unsigned long budgetStartMs = 0;
/// True while a budget is running. A separate flag rather than a 0 sentinel on
/// budgetStartMs, because millis() really is 0 for the first millisecond after
/// boot and the budget must not be a millisecond short there.
bool budgetRunning = false;
/// True once this episode's expiry has been reported to a caller.
bool expiryReported = false;

} // namespace

namespace PublishDeliveryBudget {

unsigned long budgetMs() {
  return ConnectivityPolicy::PUBLISH_DELIVERY_BUDGET_MS;
}

void reset() {
  budgetStartMs = 0;
  budgetRunning = false;
  expiryReported = false;
}

Verdict evaluate(const Inputs &in) {
  Verdict out;

  // The queue is empty (and nothing is being dispatched): sleep was always
  // allowed here, and the budget does not apply.
  if (in.queueSleepSafe) {
    reset();
    out.sleepPermitted = true;
    return out;
  }

  // Offline with events queued: this is the ordinary low-power case. The events
  // are durable and are flushed on the next connection, so sleep is permitted
  // and no delivery budget is consumed - only connected time counts.
  if (!in.cloudConnected) {
    reset();
    out.sleepPermitted = true;
    return out;
  }

  // Cloud-connected with a non-empty queue: the budget runs.
  if (!budgetRunning) {
    budgetStartMs = in.nowMs;
    budgetRunning = true;
    expiryReported = false;
  }

  out.elapsedMs = in.nowMs - budgetStartMs;
  out.budgetExpired = (out.elapsedMs >= budgetMs());

  if (out.budgetExpired && !expiryReported) {
    out.expiryFirstObserved = true;
    expiryReported = true;
  }

  // An outstanding publish attempt is never abandoned, expired budget or not.
  out.sleepPermitted = out.budgetExpired && !in.publishInFlight;
  return out;
}

} // namespace PublishDeliveryBudget
