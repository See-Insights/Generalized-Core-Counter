#pragma once

/**
 * @file PublishDeliveryGate.h
 * @brief Device-side adapter for the bounded delivery wait
 *        (WO-2026-09-25-001, Stage 5 decision 6 / acceptance criterion 11).
 *
 * @details Collects the live inputs - `Particle.connected()`, the publish
 *          queue's own `getCanSleep()` / `getPublishInFlight()` / `getNumEvents()`
 *          - feeds them to `PublishDeliveryBudget::evaluate()`, logs each budget
 *          expiry once, and returns the single queue term used by every gate
 *          that can put the device to sleep or tear the cloud connection down:
 *
 *          - the low-power IDLE sleep gate (`State_Idle.cpp`)
 *          - the IDLE connectivity ceiling (`State_Idle.cpp`)
 *          - the SLEEPING_STATE cloud-sync gate (`State_Sleep.cpp`)
 *
 *          Keeping the three sites on one function is what makes the rule
 *          consistent; before this, IDLE's unconditional queue condition made
 *          the SLEEPING_STATE timeout unreachable (Stage 7 finding P1).
 */

#ifndef __PUBLISH_DELIVERY_GATE_H
#define __PUBLISH_DELIVERY_GATE_H

namespace PublishDeliveryGate {

/**
 * @brief The queue term for a sleep/teardown gate.
 *
 * @return true when the publish queue is empty, or the delivery budget has
 *         expired and no publish attempt is in flight. Never returns true while
 *         an attempt is outstanding with events still queued.
 *
 * Logs one INFO line per budget expiry with the queued count, the elapsed time
 * and whether a publish was in flight.
 */
bool queuePermitsSleep();

/**
 * @brief True while the publish queue is waiting on a publish future.
 *
 * Thin pass-through to `PublishQueuePosix::getPublishInFlight()` so callers do
 * not have to include the queue header just to honour "never abandon an
 * in-flight publish".
 */
bool publishInFlight();

} // namespace PublishDeliveryGate

#endif // __PUBLISH_DELIVERY_GATE_H
