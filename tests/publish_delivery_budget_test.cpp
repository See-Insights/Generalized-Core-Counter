// tests/publish_delivery_budget_test.cpp
//
// WO-2026-09-25-001, Stage 5 decision 6 / acceptance criterion 11:
// the bounded delivery wait.
//
// This compiles the REAL decision module (src/cloud/PublishDeliveryBudget.cpp)
// together with the REAL policy constant (src/power/ConnectivityPolicy.h), so
// these are behavioural checks against shipped code, not a mirror. The rule
// under test is:
//
//     sleep is allowed when the queue is empty
//     OR (the delivery budget has expired AND no publish is in flight)
//
// Stage 7 finding P1 this file is built to catch: a connected device with a
// queue that never drains stayed connected for a simulated hour, because IDLE
// refused to enter SLEEPING_STATE while the queue was non-empty and so never
// reached that state's own timeout.

#include <cstdio>

#include "cloud/PublishDeliveryBudget.h"
#include "power/ConnectivityPolicy.h"

namespace {

int failures = 0;

void expectEq(const char *what, unsigned long actual, unsigned long expected) {
    if (actual != expected) {
        std::printf("FAIL: %s: got %lu, expected %lu\n", what, actual, expected);
        failures++;
    }
}

void expectTrue(const char *what, bool value) {
    if (!value) {
        std::printf("FAIL: %s: expected true\n", what);
        failures++;
    }
}

void expectFalse(const char *what, bool value) {
    if (value) {
        std::printf("FAIL: %s: expected false\n", what);
        failures++;
    }
}

using PublishDeliveryBudget::Inputs;
using PublishDeliveryBudget::Verdict;

// A device that is cloud-connected with an undrainable queue and nothing
// currently in flight (for example, inside the queue's 30 s retry backoff).
Inputs stuckQueue(unsigned long nowMs, bool inFlight = false) {
    Inputs in;
    in.cloudConnected = true;
    in.queueSleepSafe = false;
    in.publishInFlight = inFlight;
    in.queuedEvents = 3;
    in.nowMs = nowMs;
    return in;
}

Inputs drainedQueue(unsigned long nowMs) {
    Inputs in;
    in.cloudConnected = true;
    in.queueSleepSafe = true;
    in.publishInFlight = false;
    in.queuedEvents = 0;
    in.nowMs = nowMs;
    return in;
}

Inputs offlineWithQueue(unsigned long nowMs) {
    Inputs in;
    in.cloudConnected = false;
    in.queueSleepSafe = false;
    in.publishInFlight = false;
    in.queuedEvents = 3;
    in.nowMs = nowMs;
    return in;
}

// --- the budget itself ------------------------------------------------------

void testBudgetIsTheShippedPolicyConstant() {
    expectEq("the budget is ConnectivityPolicy::PUBLISH_DELIVERY_BUDGET_MS",
             PublishDeliveryBudget::budgetMs(),
             ConnectivityPolicy::PUBLISH_DELIVERY_BUDGET_MS);
    expectEq("the default budget is 90 s", PublishDeliveryBudget::budgetMs(), 90000UL);
}

// --- P1: a stuck queue eventually permits sleep -----------------------------

void testStuckQueueEventuallyPermitsSleep() {
    PublishDeliveryBudget::reset();
    const unsigned long t0 = 1000UL;
    const unsigned long budget = PublishDeliveryBudget::budgetMs();

    Verdict v = PublishDeliveryBudget::evaluate(stuckQueue(t0));
    expectFalse("sleep is refused the moment the budget starts", v.sleepPermitted);
    expectFalse("the budget has not expired at t0", v.budgetExpired);
    expectEq("elapsed is 0 at the start", v.elapsedMs, 0);

    v = PublishDeliveryBudget::evaluate(stuckQueue(t0 + budget - 1));
    expectFalse("sleep is still refused one millisecond before expiry", v.sleepPermitted);
    expectEq("elapsed tracks wall clock", v.elapsedMs, budget - 1);

    v = PublishDeliveryBudget::evaluate(stuckQueue(t0 + budget));
    expectTrue("the budget expires exactly at the budget", v.budgetExpired);
    expectTrue("sleep is permitted once the budget expired with nothing in flight",
               v.sleepPermitted);

    // Stage 7's reproduction ran for a simulated hour. It must not be possible
    // to stay connected that long with a stuck queue.
    v = PublishDeliveryBudget::evaluate(stuckQueue(t0 + 3600000UL));
    expectTrue("a simulated hour of a stuck queue still permits sleep", v.sleepPermitted);
}

void testExpiryIsReportedExactlyOncePerEpisode() {
    PublishDeliveryBudget::reset();
    const unsigned long t0 = 5000UL;
    const unsigned long budget = PublishDeliveryBudget::budgetMs();

    unsigned expiries = 0;
    for (unsigned long t = t0; t <= t0 + budget + 10000UL; t += 1000UL) {
        if (PublishDeliveryBudget::evaluate(stuckQueue(t)).expiryFirstObserved) {
            expiries++;
        }
    }
    expectEq("one expiry is reported per episode, however often it is evaluated",
             expiries, 1);

    // Draining the queue ends the episode; a new backlog starts a new budget
    // and a new expiry report.
    PublishDeliveryBudget::evaluate(drainedQueue(t0 + budget + 20000UL));
    expiries = 0;
    const unsigned long t1 = t0 + budget + 30000UL;
    for (unsigned long t = t1; t <= t1 + budget + 5000UL; t += 1000UL) {
        if (PublishDeliveryBudget::evaluate(stuckQueue(t)).expiryFirstObserved) {
            expiries++;
        }
    }
    expectEq("a second backlog reports its own expiry", expiries, 1);
}

// --- an in-flight publish is never abandoned --------------------------------

void testSleepIsNeverPermittedWhileAPublishIsInFlight() {
    PublishDeliveryBudget::reset();
    const unsigned long t0 = 1000UL;
    const unsigned long budget = PublishDeliveryBudget::budgetMs();

    PublishDeliveryBudget::evaluate(stuckQueue(t0, /*inFlight=*/true));

    Verdict v = PublishDeliveryBudget::evaluate(stuckQueue(t0 + budget, /*inFlight=*/true));
    expectTrue("the budget still expires while a publish is in flight", v.budgetExpired);
    expectTrue("the expiry is still logged while a publish is in flight", v.expiryFirstObserved);
    expectFalse("an in-flight publish is never abandoned, expired budget or not",
                v.sleepPermitted);

    v = PublishDeliveryBudget::evaluate(stuckQueue(t0 + budget + 600000UL, /*inFlight=*/true));
    expectFalse("ten more minutes in flight still does not permit sleep", v.sleepPermitted);

    // Device OS completes a WITH_ACK future within its 20 s ACK timeout, and
    // the queue clears in-flight once statePublishWait() has acted on the
    // result. Sleep is permitted from that point.
    v = PublishDeliveryBudget::evaluate(stuckQueue(t0 + budget + 600001UL, /*inFlight=*/false));
    expectTrue("sleep is permitted once the attempt's result has been processed",
               v.sleepPermitted);
}

// --- episode boundaries -----------------------------------------------------

void testDrainedQueueAlwaysPermitsSleepAndCancelsTheBudget() {
    PublishDeliveryBudget::reset();
    const unsigned long budget = PublishDeliveryBudget::budgetMs();

    expectTrue("an empty queue permits sleep immediately",
               PublishDeliveryBudget::evaluate(drainedQueue(1000UL)).sleepPermitted);

    // Accumulate most of a budget, then drain: the next backlog must start
    // from zero rather than inheriting the old elapsed time.
    PublishDeliveryBudget::evaluate(stuckQueue(2000UL));
    PublishDeliveryBudget::evaluate(stuckQueue(2000UL + budget - 1000UL));
    PublishDeliveryBudget::evaluate(drainedQueue(2000UL + budget - 500UL));

    Verdict v = PublishDeliveryBudget::evaluate(stuckQueue(2000UL + budget));
    expectEq("a drained queue restarts the budget from zero", v.elapsedMs, 0);
    expectFalse("the inherited elapsed time does not carry over", v.sleepPermitted);
}

void testOfflineSleepIsPermittedAndConsumesNoBudget() {
    PublishDeliveryBudget::reset();
    const unsigned long budget = PublishDeliveryBudget::budgetMs();

    Verdict v = PublishDeliveryBudget::evaluate(offlineWithQueue(1000UL));
    expectTrue("offline with a queue still permits sleep - the events are durable",
               v.sleepPermitted);
    expectEq("offline time does not run the budget", v.elapsedMs, 0);

    // Connect, accumulate, drop the connection, reconnect: the budget restarts.
    PublishDeliveryBudget::evaluate(stuckQueue(2000UL));
    PublishDeliveryBudget::evaluate(stuckQueue(2000UL + budget - 1000UL));
    PublishDeliveryBudget::evaluate(offlineWithQueue(2000UL + budget - 500UL));

    v = PublishDeliveryBudget::evaluate(stuckQueue(2000UL + budget));
    expectEq("a dropped connection restarts the budget from zero", v.elapsedMs, 0);
    expectFalse("a reconnect does not inherit the previous connection's elapsed time",
                v.sleepPermitted);
}

void testBudgetStartAtMillisZeroStillRuns() {
    // millis() is 0 for the first millisecond after boot, and 0 is the module's
    // "not running" sentinel. The budget must still start and expire.
    PublishDeliveryBudget::reset();
    const unsigned long budget = PublishDeliveryBudget::budgetMs();

    // millis() is 0 for the first millisecond after boot; the module tracks
    // "running" with its own flag so the budget is not a millisecond short there.
    PublishDeliveryBudget::evaluate(stuckQueue(0UL));
    Verdict v = PublishDeliveryBudget::evaluate(stuckQueue(budget));
    expectTrue("a budget started at millis()==0 still expires", v.budgetExpired);
    expectTrue("a budget started at millis()==0 still permits sleep", v.sleepPermitted);
}

} // namespace

int main() {
    testBudgetIsTheShippedPolicyConstant();
    testStuckQueueEventuallyPermitsSleep();
    testExpiryIsReportedExactlyOncePerEpisode();
    testSleepIsNeverPermittedWhileAPublishIsInFlight();
    testDrainedQueueAlwaysPermitsSleepAndCancelsTheBudget();
    testOfflineSleepIsPermittedAndConsumesNoBudget();
    testBudgetStartAtMillisZeroStillRuns();

    if (failures != 0) {
        std::printf("publish_delivery_budget_test: %d failure(s)\n", failures);
        return 1;
    }
    std::printf("publish_delivery_budget_test: all checks passed\n");
    return 0;
}
