#pragma once

/**
 * @file PublishDeliveryCounters.h
 * @brief Single owner of the publish delivery counters reported in the
 *        `status` event (WO-2026-09-25-001, Stage 5 decisions 2 and 5).
 *
 * @note  Decision 5 keeps these counters OUT of the device-status ledger
 *        payload: that payload was already measured at 831-856 of its 896-byte
 *        cap in the field, so a counter object there would have pushed ordinary
 *        cycles over the cap. They are published by `publishStartupStatus()`
 *        in src/Generalized-Core-Counter.cpp instead - see
 *        tests/status_event_payload_budget_test.py for that event's byte
 *        budget.
 *
 * @details Fix B makes every queued send go out with explicit `WITH_ACK`, so a
 *          queue entry is only removed once the cloud acknowledged it. These
 *          counters turn that guarantee into something measurable fleet-wide:
 *          without them the only record of a generated-but-undelivered event is
 *          a USB serial capture, which nine of the twelve production devices do
 *          not have (see the WO's fleet-scope table).
 *
 *          Counter definitions - all four cumulative counters count *publish
 *          attempts*, not logical events, because an event that needs three
 *          attempts is exactly the signal the bench is looking for:
 *
 *          - `attempted`     one per `Particle.publish()` actually started for a
 *                            queued event (the queue's dispatch to
 *                            BackgroundPublishRK was accepted).
 *          - `acknowledged`  one per attempt whose Future succeeded. Under fix B
 *                            every queued send carries `WITH_ACK`, so Device OS
 *                            only completes that Future on a cloud ACK
 *                            (`communication/src/publisher.cpp:88-98`).
 *          - `failed`        one per attempt whose Future failed or timed out
 *                            (20 s ACK timeout). The event stays queued.
 *          - `retried`       one per attempt that follows an earlier failed
 *                            attempt - i.e. a re-send, not a first send.
 *                            `retried` is a subset of `attempted`.
 *          - `abandoned`     one per attempt that was dispatched but whose
 *                            outcome never arrived, because the device reset
 *                            (watchdog, pin, software) or lost power while the
 *                            attempt was outstanding. Counted at the next
 *                            `begin()`, from the retained outstanding-attempt
 *                            flag. This is what keeps the accounting balanced:
 *
 *                                attempted == acknowledged + failed + abandoned
 *
 *                            outside an in-flight attempt, and below saturation
 *                            (see `kCounterMax` - once any counter reaches the
 *                            ceiling the totals are censored and the identity no
 *                            longer holds).
 *          - `queuedAtSleep` NOT cumulative: the queue depth observed at the
 *                            most recent sleep commit. It is the count of events
 *                            carried over to the next connection.
 *          - `sleptWithQueued` cumulative count of sleep commits that happened
 *                            with at least one event still queued - i.e. how
 *                            often the delivery budget expired (or a gate timed
 *                            out) rather than the queue draining. Stage 5
 *                            decision 6.
 *
 *          All seven live in retained (backup) SRAM, guarded by a magic and a
 *          version so a layout change or an uninitialized block is detected
 *          rather than published as garbage. **No `SysData`, `CurrentData` or
 *          `SensorData` layout is touched** - this is a separate retained
 *          block, exactly as `HibernateCycle.cpp` and `ThrashGuard.cpp` already
 *          do for their own retained state.
 *
 *          Survival:
 *          - HIBERNATE / ULTRA_LOW_POWER sleep and wake: **survives.** Backup
 *            SRAM is kept powered across both, which is what makes the counters
 *            usable as a per-device cumulative loss measure.
 *          - Software/watchdog/pin reset: **survives.** Backup SRAM is not
 *            cleared by a reset; `begin()` only re-zeroes the block when the
 *            magic or version does not match.
 *          - Power loss (battery disconnected / fully drained), a new flash, or
 *            a firmware build that changes `kRetainedVersion`: **does not
 *            survive** - the block re-initializes to zero on the next boot.
 *
 *          All entry points are called from the application thread only (the
 *          publish queue's `loop()` and the sleep path), so no locking is
 *          needed - see the call-site notes in `PublishDeliveryCounters.cpp`.
 */

#ifndef __PUBLISH_DELIVERY_COUNTERS_H
#define __PUBLISH_DELIVERY_COUNTERS_H

#include <stdint.h>

namespace PublishDeliveryCounters {

/// Plain value copy of the retained block, for telemetry consumers.
struct Snapshot {
  uint16_t attempted = 0;
  uint16_t acknowledged = 0;
  uint16_t failed = 0;
  uint16_t retried = 0;
  uint16_t queuedAtSleep = 0;
  uint16_t sleptWithQueued = 0;
  uint16_t abandoned = 0;
};

/// Saturating ceiling for the cumulative counters. Chosen so the status event's
/// worst case is bounded at five digits per counter - see the byte budget in
/// tests/status_event_payload_budget_test.py. At the ceiling the counters are
/// censored values: `attempted == acknowledged + failed + abandoned` no longer
/// holds, because each counter saturates independently.
constexpr uint16_t kCounterMax = 65535;

/**
 * @brief Validate (and if necessary zero) the retained block. Call once from
 *        `setup()`, before the publish queue can dispatch anything.
 *
 * Also reconciles an attempt that was outstanding when the device reset: it is
 * counted as `abandoned`, and the event's next dispatch is marked as a retry,
 * because the queue still holds that event. Without this, `attempted` stays
 * permanently ahead of `acknowledged + failed` (Stage 7 finding P2).
 */
void begin();

/**
 * @brief One publish attempt was started for a queued event.
 *
 * Also closes out a pending retry: if the previous attempt failed, this attempt
 * is by definition a re-send, so `retried` is incremented here rather than at
 * the failure, which keeps `retried` a strict subset of `attempted`. Records the
 * attempt as outstanding, so a reset before its result arrives is counted as
 * `abandoned` at the next `begin()`.
 */
void noteAttempt();

/**
 * @brief A publish attempt completed.
 *
 * @param acknowledged true when the publish Future succeeded. With fix B's
 *        explicit `WITH_ACK` that means the cloud acknowledged the event and
 *        the queue is about to remove it; false means no acknowledgment and the
 *        event stays queued for retry.
 */
void noteResult(bool acknowledged);

/**
 * @brief Record the queue depth at the moment the device commits to sleep.
 */
void noteQueuedAtSleep(uint16_t queuedEvents);

/**
 * @brief The device is committing to sleep with at least one event still queued.
 *
 * Call once per sleep commit that carries events over - Stage 5 decision 6.
 */
void noteSleptWithQueued();

/**
 * @brief Read all counters.
 */
Snapshot snapshot();

} // namespace PublishDeliveryCounters

#endif // __PUBLISH_DELIVERY_COUNTERS_H
