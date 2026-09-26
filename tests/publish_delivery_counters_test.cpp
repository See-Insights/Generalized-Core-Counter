// tests/publish_delivery_counters_test.cpp
//
// WO-2026-09-25-001, Stage 5 decision 2 / acceptance criterion 4.
//
// This compiles the REAL production counter module
// (src/cloud/PublishDeliveryCounters.cpp) on the host - `retained` is defined
// away on the compile line, which is the only Particle-ism in that file - so
// these are behavioural checks against shipped code, not a mirror.

#include <cstdio>

#include "cloud/PublishDeliveryCounters.h"

namespace {

int failures = 0;

void expectEq(const char *what, unsigned long actual, unsigned long expected) {
    if (actual != expected) {
        std::printf("FAIL: %s: got %lu, expected %lu\n", what, actual, expected);
        failures++;
    }
}

using PublishDeliveryCounters::Snapshot;

Snapshot reset() {
    // begin() only re-zeroes when the magic/version does not match, which is
    // exactly the hibernate/reset survival behaviour. Drain the block to a
    // known state by calling begin() on a fresh process.
    PublishDeliveryCounters::begin();
    return PublishDeliveryCounters::snapshot();
}

void testStartsAtZero() {
    const Snapshot s = reset();
    expectEq("attempted starts at 0", s.attempted, 0);
    expectEq("acknowledged starts at 0", s.acknowledged, 0);
    expectEq("failed starts at 0", s.failed, 0);
    expectEq("retried starts at 0", s.retried, 0);
    expectEq("queuedAtSleep starts at 0", s.queuedAtSleep, 0);
    expectEq("sleptWithQueued starts at 0", s.sleptWithQueued, 0);
    expectEq("abandoned starts at 0", s.abandoned, 0);
}

void testBeginIsIdempotentOnceInitialized() {
    PublishDeliveryCounters::noteAttempt();
    PublishDeliveryCounters::noteResult(true);
    // A later boot with the same layout must NOT clear the counters - that is
    // what makes them survive a hibernate wake and a reset.
    PublishDeliveryCounters::begin();
    const Snapshot s = PublishDeliveryCounters::snapshot();
    expectEq("begin() preserves an already-initialized block (attempted)", s.attempted, 1);
    expectEq("begin() preserves an already-initialized block (acknowledged)", s.acknowledged, 1);
}

void testAcknowledgedAttempt() {
    const Snapshot before = PublishDeliveryCounters::snapshot();
    PublishDeliveryCounters::noteAttempt();
    PublishDeliveryCounters::noteResult(true);
    const Snapshot after = PublishDeliveryCounters::snapshot();
    expectEq("an acknowledged send counts one attempt", after.attempted, before.attempted + 1);
    expectEq("an acknowledged send counts one acknowledgment", after.acknowledged, before.acknowledged + 1);
    expectEq("an acknowledged send counts no failure", after.failed, before.failed);
    expectEq("an acknowledged send counts no retry", after.retried, before.retried);
}

void testFailedAttemptThenRetry() {
    const Snapshot before = PublishDeliveryCounters::snapshot();

    PublishDeliveryCounters::noteAttempt();
    PublishDeliveryCounters::noteResult(false); // no ACK within the 20 s timeout
    const Snapshot afterFailure = PublishDeliveryCounters::snapshot();
    expectEq("an unacknowledged send counts one attempt", afterFailure.attempted, before.attempted + 1);
    expectEq("an unacknowledged send counts one failure", afterFailure.failed, before.failed + 1);
    expectEq("an unacknowledged send is not itself a retry", afterFailure.retried, before.retried);
    expectEq("an unacknowledged send counts no acknowledgment", afterFailure.acknowledged, before.acknowledged);

    // The queue keeps the event and re-dispatches it: the NEXT attempt is the retry.
    PublishDeliveryCounters::noteAttempt();
    const Snapshot afterRetryDispatch = PublishDeliveryCounters::snapshot();
    expectEq("the re-send counts as a retry", afterRetryDispatch.retried, before.retried + 1);
    expectEq("the re-send also counts as an attempt", afterRetryDispatch.attempted, before.attempted + 2);

    PublishDeliveryCounters::noteResult(true);
    const Snapshot afterRetryAck = PublishDeliveryCounters::snapshot();
    expectEq("the acknowledged re-send counts one acknowledgment",
             afterRetryAck.acknowledged, before.acknowledged + 1);

    // A third, first-time send must not be counted as a retry.
    PublishDeliveryCounters::noteAttempt();
    const Snapshot afterFresh = PublishDeliveryCounters::snapshot();
    expectEq("a first-time send after a success is not a retry",
             afterFresh.retried, before.retried + 1);
    PublishDeliveryCounters::noteResult(true);
}

// WO-2026-09-25-001 Stage 5 decision 7 (Stage 7 finding P2): an attempt that was
// outstanding when the device reset lost its Future and its callback, so its
// outcome never arrives. Without the `abandoned` reconciliation at boot,
// `attempted` stayed permanently ahead of `acknowledged + failed`.
void testResetDuringAnAttemptIsCountedAsAbandoned() {
    const Snapshot before = PublishDeliveryCounters::snapshot();

    PublishDeliveryCounters::noteAttempt();
    const Snapshot outstanding = PublishDeliveryCounters::snapshot();
    expectEq("the interrupted attempt was counted as an attempt",
             outstanding.attempted, before.attempted + 1);
    expectEq("nothing is abandoned while the attempt is still outstanding",
             outstanding.abandoned, before.abandoned);

    // The reset: retained SRAM survives, so begin() sees a valid block with an
    // attempt still marked outstanding.
    PublishDeliveryCounters::begin();
    const Snapshot afterBoot = PublishDeliveryCounters::snapshot();
    expectEq("a reset during an attempt counts one abandoned",
             afterBoot.abandoned, before.abandoned + 1);
    expectEq("the surviving counters are not cleared by the reconciliation",
             afterBoot.attempted, before.attempted + 1);
    expectEq("a == k + f + abandoned across the simulated reset",
             afterBoot.attempted,
             (unsigned long)afterBoot.acknowledged + (unsigned long)afterBoot.failed +
                 (unsigned long)afterBoot.abandoned);

    // A second boot must not re-abandon the same attempt.
    PublishDeliveryCounters::begin();
    expectEq("begin() does not re-abandon an already-reconciled attempt",
             PublishDeliveryCounters::snapshot().abandoned, before.abandoned + 1);

    // The queue still holds that event - it is only removed on an
    // acknowledgment - so the dispatch after the reset is a re-send.
    PublishDeliveryCounters::noteAttempt();
    expectEq("the re-send after an abandoned attempt counts as a retry",
             PublishDeliveryCounters::snapshot().retried, before.retried + 1);
    PublishDeliveryCounters::noteResult(true);
}

void testAttemptedEqualsAcknowledgedPlusFailedPlusAbandoned() {
    const Snapshot s = PublishDeliveryCounters::snapshot();
    expectEq("attempted == acknowledged + failed + abandoned once nothing is in flight",
             s.attempted,
             (unsigned long)s.acknowledged + (unsigned long)s.failed + (unsigned long)s.abandoned);
    if (s.retried > s.attempted) {
        std::printf("FAIL: retried (%u) must be a subset of attempted (%u)\n",
                    (unsigned)s.retried, (unsigned)s.attempted);
        failures++;
    }
}

void testSleptWithQueuedIsCumulative() {
    const Snapshot before = PublishDeliveryCounters::snapshot();
    PublishDeliveryCounters::noteSleptWithQueued();
    expectEq("a sleep commit that carries events over is counted",
             PublishDeliveryCounters::snapshot().sleptWithQueued, before.sleptWithQueued + 1);
    PublishDeliveryCounters::noteSleptWithQueued();
    expectEq("sleptWithQueued accumulates, unlike queuedAtSleep",
             PublishDeliveryCounters::snapshot().sleptWithQueued, before.sleptWithQueued + 2);
    // It must not disturb the delivery accounting.
    const Snapshot after = PublishDeliveryCounters::snapshot();
    expectEq("counting a carry-over does not change attempted", after.attempted, before.attempted);
    expectEq("counting a carry-over does not change acknowledged",
             after.acknowledged, before.acknowledged);
}

void testQueuedAtSleepIsASnapshotNotACumulativeCount() {
    PublishDeliveryCounters::noteQueuedAtSleep(3);
    expectEq("queuedAtSleep records the depth at sleep",
             PublishDeliveryCounters::snapshot().queuedAtSleep, 3);
    PublishDeliveryCounters::noteQueuedAtSleep(0);
    expectEq("queuedAtSleep is replaced, not accumulated",
             PublishDeliveryCounters::snapshot().queuedAtSleep, 0);
    PublishDeliveryCounters::noteQueuedAtSleep(1);
    expectEq("queuedAtSleep tracks the most recent sleep",
             PublishDeliveryCounters::snapshot().queuedAtSleep, 1);
}

void testCountersSaturateRatherThanWrap() {
    // The payload byte budget depends on the counters never exceeding five
    // digits (see tests/status_event_payload_budget_test.py). Wrapping to 0
    // would also silently reset a fleet-wide loss measurement.
    while (PublishDeliveryCounters::snapshot().attempted < PublishDeliveryCounters::kCounterMax ||
           PublishDeliveryCounters::snapshot().acknowledged < PublishDeliveryCounters::kCounterMax) {
        PublishDeliveryCounters::noteAttempt();
        PublishDeliveryCounters::noteResult(true);
    }
    const Snapshot s = PublishDeliveryCounters::snapshot();
    expectEq("attempted saturates at kCounterMax", s.attempted, PublishDeliveryCounters::kCounterMax);
    expectEq("acknowledged saturates at kCounterMax", s.acknowledged, PublishDeliveryCounters::kCounterMax);
    PublishDeliveryCounters::noteAttempt();
    expectEq("attempted stays saturated", PublishDeliveryCounters::snapshot().attempted,
             PublishDeliveryCounters::kCounterMax);
    PublishDeliveryCounters::noteResult(true);

    // Each counter saturates independently, so at the ceiling the totals are
    // censored and `a == k + f + abandoned` no longer holds. That is a declared
    // limitation, documented in PublishDeliveryCounters.h and in
    // docs/FIELD_MEANINGS_REFERENCE.md - read 65535 as ">= 65535". This
    // assertion pins the behaviour so it cannot change silently.
    const Snapshot censored = PublishDeliveryCounters::snapshot();
    if (censored.failed != 0 || censored.abandoned != 0) {
        const unsigned long sum = (unsigned long)censored.acknowledged +
                                  (unsigned long)censored.failed +
                                  (unsigned long)censored.abandoned;
        if (sum <= (unsigned long)censored.attempted) {
            std::printf("FAIL: at saturation the invariant is expected to be censored, "
                        "but a=%u >= k+f+b=%lu\n",
                        (unsigned)censored.attempted, sum);
            failures++;
        }
    }
}

} // namespace

int main() {
    testStartsAtZero();
    testBeginIsIdempotentOnceInitialized();
    testAcknowledgedAttempt();
    testFailedAttemptThenRetry();
    testResetDuringAnAttemptIsCountedAsAbandoned();
    testAttemptedEqualsAcknowledgedPlusFailedPlusAbandoned();
    testSleptWithQueuedIsCumulative();
    testQueuedAtSleepIsASnapshotNotACumulativeCount();
    testCountersSaturateRatherThanWrap();

    if (failures != 0) {
        std::printf("publish_delivery_counters_test: %d failure(s)\n", failures);
        return 1;
    }
    std::printf("publish_delivery_counters_test: all checks passed\n");
    return 0;
}
