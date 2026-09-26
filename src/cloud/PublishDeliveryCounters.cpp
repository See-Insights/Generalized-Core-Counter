/**
 * @file PublishDeliveryCounters.cpp
 * @brief Retained-RAM storage and accounting for the publish delivery counters.
 *
 * @details See PublishDeliveryCounters.h for the counter definitions and for
 *          exactly what survives a hibernate, a reset and a power loss.
 *
 *          Threading: `noteAttempt()` and `noteResult()` are invoked from
 *          `PublishQueuePosix::stateWait()` / `statePublishWait()`, both of
 *          which run on the application thread inside
 *          `PublishQueuePosix::instance().loop()`. They are deliberately NOT
 *          wired to `BackgroundPublishRK`'s own completion callback, which runs
 *          on the background publish thread - keeping every write on one thread
 *          is what makes the plain (unlocked, non-atomic) increments below
 *          correct. `noteQueuedAtSleep()` is called from the sleep path, also on
 *          the application thread.
 */

#include "Particle.h"
#include "cloud/PublishDeliveryCounters.h"

namespace {

// 'PDC' + version nibble. Changing the struct layout must change kRetainedVersion.
constexpr uint32_t kRetainedMagic = 0x50444301UL;
// Version 2 (WO-2026-09-25-001 Stage 5 decisions 6 and 7) added sleptWithQueued,
// abandoned and the outstanding-attempt flag. A device carrying a version-1
// block re-zeroes on the first boot of this build, which is the intended
// behaviour for a layout change.
constexpr uint16_t kRetainedVersion = 2;

struct RetainedPublishDelivery {
  uint32_t magic;
  uint16_t version;
  uint16_t attempted;
  uint16_t acknowledged;
  uint16_t failed;
  uint16_t retried;
  uint16_t queuedAtSleep;
  uint16_t sleptWithQueued;
  uint16_t abandoned;
  // Set when an attempt completed without an acknowledgment, so the next
  // attempt (which the queue will make for the same, still-queued event) is
  // counted as a retry. Retained alongside the counters so a retry that
  // happens on the far side of a hibernate is still counted.
  uint8_t retryPending;
  // Set from noteAttempt() until noteResult() arrives. If the device resets
  // while it is set, that attempt's outcome was lost with the volatile Future,
  // so begin() counts it as abandoned.
  uint8_t attemptOutstanding;
};

void bump(uint16_t &counter) {
  if (counter < PublishDeliveryCounters::kCounterMax) {
    counter++;
  }
}

} // namespace

// Separate retained block - no SysData/CurrentData/SensorData layout is
// involved. Same ownership pattern as HibernateCycle.cpp's retained fields.
retained RetainedPublishDelivery retainedPublishDelivery = {};

namespace PublishDeliveryCounters {

void begin() {
  if (retainedPublishDelivery.magic != kRetainedMagic ||
      retainedPublishDelivery.version != kRetainedVersion) {
    retainedPublishDelivery = RetainedPublishDelivery{};
    retainedPublishDelivery.magic = kRetainedMagic;
    retainedPublishDelivery.version = kRetainedVersion;
    return;
  }

  // WO-2026-09-25-001 Stage 5 decision 7 (Stage 7 finding P2): an attempt that
  // was outstanding when the device reset lost its Future and its callback, so
  // its outcome will never arrive. Count it once, here, so that
  // `attempted == acknowledged + failed + abandoned` still holds. The event
  // itself is still queued - the queue only removes it on an acknowledgment -
  // so the dispatch that follows this boot is a re-send and must be counted as
  // a retry.
  if (retainedPublishDelivery.attemptOutstanding) {
    bump(retainedPublishDelivery.abandoned);
    retainedPublishDelivery.attemptOutstanding = 0;
    retainedPublishDelivery.retryPending = 1;
  }
}

void noteAttempt() {
  if (retainedPublishDelivery.retryPending) {
    bump(retainedPublishDelivery.retried);
    retainedPublishDelivery.retryPending = 0;
  }
  bump(retainedPublishDelivery.attempted);
  retainedPublishDelivery.attemptOutstanding = 1;
}

void noteResult(bool acknowledged) {
  retainedPublishDelivery.attemptOutstanding = 0;
  if (acknowledged) {
    bump(retainedPublishDelivery.acknowledged);
    retainedPublishDelivery.retryPending = 0;
  } else {
    bump(retainedPublishDelivery.failed);
    // The queue keeps a failed event queued and re-dispatches it after
    // waitAfterFailure, so the next attempt is a retry.
    retainedPublishDelivery.retryPending = 1;
  }
}

void noteQueuedAtSleep(uint16_t queuedEvents) {
  retainedPublishDelivery.queuedAtSleep = queuedEvents;
}

void noteSleptWithQueued() {
  bump(retainedPublishDelivery.sleptWithQueued);
}

Snapshot snapshot() {
  Snapshot out;
  out.attempted = retainedPublishDelivery.attempted;
  out.acknowledged = retainedPublishDelivery.acknowledged;
  out.failed = retainedPublishDelivery.failed;
  out.retried = retainedPublishDelivery.retried;
  out.queuedAtSleep = retainedPublishDelivery.queuedAtSleep;
  out.sleptWithQueued = retainedPublishDelivery.sleptWithQueued;
  out.abandoned = retainedPublishDelivery.abandoned;
  return out;
}

} // namespace PublishDeliveryCounters
