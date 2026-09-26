#pragma once

/**
 * @file PublishDeliveryBudget.h
 * @brief Bounded delivery wait for the publish queue (WO-2026-09-25-001,
 *        Stage 5 decision 6 / acceptance criterion 11).
 *
 * @details Fix B only lets the queue remove an event once the cloud has
 *          acknowledged it. That closes the loss gap, but it also means a queue
 *          that never drains keeps the device connected forever: IDLE refuses to
 *          enter `SLEEPING_STATE` while the queue is non-empty
 *          (`State_Idle.cpp`), so the `SLEEPING_STATE` gate's own timeout is
 *          never reached at all (Stage 7 finding P1 - an exact-source
 *          reproduction stayed connected for a simulated hour).
 *
 *          The rule this module implements is:
 *
 *          > Sleep is allowed when the queue is empty **or** (the delivery
 *          > budget has expired **and** no publish is in flight).
 *
 *          **Budget start:** the first evaluation at which the device is
 *          cloud-connected *and* the publish queue is not sleep-safe (i.e. it
 *          holds at least one event, or is dispatching one). The budget is
 *          cancelled - and restarts from zero next time - as soon as the queue
 *          becomes sleep-safe or the cloud connection drops. Offline time never
 *          consumes the budget, because offline sleep is already permitted:
 *          queued events are durable and flushed on the next connection.
 *
 *          **Budget value:** `ConnectivityPolicy::PUBLISH_DELIVERY_BUDGET_MS`
 *          (90 s). It is a compile-time named constant in the existing policy
 *          header rather than a runtime setting: every runtime-configurable
 *          value on this device lives in a persisted struct, and this work order
 *          may not change a persisted layout.
 *
 *          **In flight:** supplied by the caller from
 *          `PublishQueuePosix::getPublishInFlight()` - the queue's own state,
 *          not an inference. An outstanding attempt is never abandoned by a
 *          disconnect, whatever the budget says.
 *
 *          This translation unit is deliberately free of Particle, the publish
 *          queue and logging, so `tests/publish_delivery_budget_test.cpp`
 *          exercises the shipped decision logic directly on the host. The
 *          device-side adapter that collects the inputs, applies the verdict and
 *          logs expiry lives in `PublishDeliveryGate.h/.cpp`.
 */

#ifndef __PUBLISH_DELIVERY_BUDGET_H
#define __PUBLISH_DELIVERY_BUDGET_H

#include <stddef.h>
#include <stdint.h>

namespace PublishDeliveryBudget {

/// Everything the decision depends on, gathered by the caller.
struct Inputs {
  /// Particle.connected()
  bool cloudConnected = false;
  /// PublishQueuePosix::getCanSleep() - true when the queue holds nothing and
  /// is not dispatching.
  bool queueSleepSafe = false;
  /// PublishQueuePosix::getPublishInFlight()
  bool publishInFlight = false;
  /// Number of events still queued, for the expiry log only.
  size_t queuedEvents = 0;
  /// millis()
  unsigned long nowMs = 0;
};

struct Verdict {
  /// The queue term for the sleep/teardown gates: queue empty OR (budget
  /// expired AND nothing in flight).
  bool sleepPermitted = false;
  /// True once the budget has run out for this episode, whether or not a
  /// publish is still in flight.
  bool budgetExpired = false;
  /// True on the single evaluation at which the budget expired, so the caller
  /// logs each expiry exactly once per episode.
  bool expiryFirstObserved = false;
  /// Milliseconds since the budget started (0 while it is not running).
  unsigned long elapsedMs = 0;
};

/// The configured budget, in milliseconds.
unsigned long budgetMs();

/**
 * @brief Advance the budget and return the current verdict.
 *
 * Safe to call several times per loop iteration and from several call sites:
 * the timer is keyed on wall-clock `millis()`, and `expiryFirstObserved` is
 * raised for exactly one evaluation per expiry episode.
 */
Verdict evaluate(const Inputs &in);

/// Cancel a running budget (used by the tests; the device cancels implicitly by
/// draining the queue or dropping the connection).
void reset();

} // namespace PublishDeliveryBudget

#endif // __PUBLISH_DELIVERY_BUDGET_H
