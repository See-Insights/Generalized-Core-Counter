// tests/publish_with_ack_queue_test.cpp
//
// WO-2026-09-25-001 (fix B) host mirror for the two delivery guarantees that
// PublishQueuePosixRK.cpp cannot be compiled on the host to demonstrate (it
// pulls in Particle, SequentialFileRK and a POSIX flash file system):
//
//   1. Every queued send is dispatched with explicit WITH_ACK, with NO_ACK
//      cleared first, whatever flags the event was enqueued with - including
//      events already persisted by an older, PRIVATE-only build.
//   2. A queue entry is removed only after its publish's Future succeeded.
//      A failed/unacknowledged attempt leaves the event queued, and the queue
//      does not report itself sleep-safe while it is still there.
//
// The companion zsh script cross-checks this mirror against the real source so
// it cannot silently drift. Stage 7 mutations this file is built to catch:
//   (i)   drop WITH_ACK from the dispatch flags
//   (ii)  remove the queue entry on dispatch or on the publish call returning
//   (iii) report canSleep == true while an unacknowledged event is queued

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <string>

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

// ---------------------------------------------------------------------------
// Mirror of the Device OS publish flag constants (system/inc/system_cloud.h:178-181)
// ---------------------------------------------------------------------------
constexpr uint32_t kPublic = 0x00;
constexpr uint32_t kPrivate = 0x01;
constexpr uint32_t kNoAck = 0x02;
constexpr uint32_t kWithAck = 0x08;

// Mirror of PublishQueuePosix::stateWait()'s dispatch normalization:
//   const PublishFlags sendFlags = (curEvent->flags & ~PublishFlags(NO_ACK)) | WITH_ACK;
// NO_ACK takes precedence over WITH_ACK in Device OS
// (communication/src/publisher.cpp:49-57), so it has to be cleared, not just
// OR-ed over.
uint32_t normalizeSendFlags(uint32_t queuedFlags) {
    return (queuedFlags & ~kNoAck) | kWithAck;
}

// ---------------------------------------------------------------------------
// Mirror of the queue's publish state machine, reduced to the removal decision
// ---------------------------------------------------------------------------
class QueueMirror {
public:
    void enqueue(const std::string &name, uint32_t flags) {
        events_.push_back({name, flags});
        canSleep_ = false;
    }

    // Mirror of stateWait(): take the head event, dispatch it with normalized
    // flags, and mark the queue not sleep-safe for the duration.
    bool dispatch() {
        if (inFlight_) {
            return false;
        }
        if (events_.empty()) {
            canSleep_ = true; // stateWait(): "No events, can sleep"
            return false;
        }
        inFlight_ = true;
        publishComplete_ = false;
        publishSuccess_ = false;
        canSleep_ = false; // stateWait() sets canSleep = false on dispatch
        lastSendFlags_ = normalizeSendFlags(events_.front().flags);
        attempts_++;
        return true;
    }

    // Mirror of BackgroundPublishRK's worker completing the Future, then
    // PublishQueuePosix::publishCompleteCallback().
    void completeAttempt(bool futureSucceeded) {
        assert(inFlight_);
        publishComplete_ = true;
        publishSuccess_ = futureSucceeded;
    }

    // Mirror of statePublishWait(): removal ONLY on publishSuccess.
    void serviceCompletion() {
        if (!publishComplete_) {
            return; // statePublishWait() early-returns; nothing is removed
        }
        if (publishSuccess_) {
            events_.pop_front();
        }
        // else: the event stays queued (file left in place / RAM event
        // push_front-ed and re-persisted), and is retried after waitAfterFailure.
        inFlight_ = false;
        publishComplete_ = false;
        canSleep_ = events_.empty();
    }

    size_t depth() const { return events_.size(); }
    bool canSleep() const { return canSleep_; }
    uint32_t lastSendFlags() const { return lastSendFlags_; }
    unsigned attempts() const { return attempts_; }
    std::string headName() const { return events_.empty() ? std::string() : events_.front().name; }

private:
    struct Event {
        std::string name;
        uint32_t flags;
    };

    std::deque<Event> events_;
    bool inFlight_ = false;
    bool publishComplete_ = false;
    bool publishSuccess_ = false;
    bool canSleep_ = false;
    uint32_t lastSendFlags_ = 0;
    unsigned attempts_ = 0;
};

// --- (i) explicit WITH_ACK on every queued send -----------------------------

void testEveryQueuedSendCarriesExplicitWithAck() {
    // The app enqueues everything PRIVATE-only today (report, status, pdiag,
    // watchdog, hibernate_wake).
    expectEq("PRIVATE-only event is sent PRIVATE|WITH_ACK",
             normalizeSendFlags(kPrivate), kPrivate | kWithAck);

    // An event persisted to the file queue by an older build carries whatever
    // flags that build used; normalization happens at dispatch, so those are
    // covered too.
    expectEq("persisted PRIVATE-only event is sent PRIVATE|WITH_ACK",
             normalizeSendFlags(kPrivate), 0x09u);
    expectEq("PUBLIC event is sent with WITH_ACK too",
             normalizeSendFlags(kPublic), kWithAck);

    // NO_ACK wins over WITH_ACK in Device OS, so it must be cleared.
    expectEq("NO_ACK is cleared, not merely OR-ed over",
             normalizeSendFlags(kPrivate | kNoAck), kPrivate | kWithAck);
    expectFalse("NO_ACK is absent from the effective send flags",
                (normalizeSendFlags(kPrivate | kNoAck) & kNoAck) != 0);

    // Already-explicit WITH_ACK is idempotent.
    expectEq("WITH_ACK is idempotent",
             normalizeSendFlags(kPrivate | kWithAck), kPrivate | kWithAck);

    // And the dispatch path actually uses it.
    QueueMirror q;
    q.enqueue("report", kPrivate);
    expectTrue("dispatch accepted", q.dispatch());
    expectEq("dispatched flags carry WITH_ACK", q.lastSendFlags() & kWithAck, kWithAck);
    expectEq("dispatched flags preserve PRIVATE", q.lastSendFlags() & kPrivate, kPrivate);
}

// --- (ii) removal only after an acknowledged Future -------------------------

void testEventIsNotRemovedBeforeAcknowledgment() {
    QueueMirror q;
    q.enqueue("report", kPrivate);
    expectEq("one event queued", q.depth(), 1);

    q.dispatch();
    expectEq("dispatch alone does not remove the event", q.depth(), 1);

    q.serviceCompletion(); // Future still pending
    expectEq("a pending Future does not remove the event", q.depth(), 1);

    q.completeAttempt(true);
    q.serviceCompletion();
    expectEq("an acknowledged Future removes the event", q.depth(), 0);
}

void testFailedAttemptLeavesEventQueuedForRetry() {
    QueueMirror q;
    q.enqueue("report", kPrivate);

    q.dispatch();
    q.completeAttempt(false); // no ACK within the 20 s timeout
    q.serviceCompletion();
    expectEq("unacknowledged publish leaves the event queued", q.depth(), 1);
    expectEq("the same event is still at the head", q.headName() == "report" ? 1u : 0u, 1u);

    // Retry, this time acknowledged.
    q.dispatch();
    expectEq("the retry is a second attempt for the same event", q.attempts(), 2);
    q.completeAttempt(true);
    q.serviceCompletion();
    expectEq("the acknowledged retry removes the event", q.depth(), 0);
}

void testOrderingIsPreservedAcrossAFailure() {
    QueueMirror q;
    q.enqueue("report", kPrivate);
    q.enqueue("status", kPrivate);

    q.dispatch();
    q.completeAttempt(false);
    q.serviceCompletion();
    expectEq("both events still queued after a failure", q.depth(), 2);
    expectEq("the failed event stays at the head", q.headName() == "report" ? 1u : 0u, 1u);
}

// --- (iii) sleep safety while an unacknowledged event is queued -------------

void testQueueIsNotSleepSafeWhileAnEventIsUnacknowledged() {
    QueueMirror q;
    expectTrue("empty queue that has never dispatched can sleep after a poll",
               (q.dispatch(), q.canSleep()));

    q.enqueue("report", kPrivate);
    expectFalse("queued event makes the queue not sleep-safe", q.canSleep());

    q.dispatch();
    expectFalse("in-flight publish is not sleep-safe", q.canSleep());

    q.serviceCompletion(); // still pending
    expectFalse("pending Future is not sleep-safe", q.canSleep());

    q.completeAttempt(false);
    q.serviceCompletion();
    expectFalse("unacknowledged event left queued is not sleep-safe", q.canSleep());

    q.dispatch();
    q.completeAttempt(true);
    q.serviceCompletion();
    expectTrue("drained queue is sleep-safe", q.canSleep());
}

} // namespace

int main() {
    testEveryQueuedSendCarriesExplicitWithAck();
    testEventIsNotRemovedBeforeAcknowledgment();
    testFailedAttemptLeavesEventQueuedForRetry();
    testOrderingIsPreservedAcrossAFailure();
    testQueueIsNotSleepSafeWhileAnEventIsUnacknowledged();

    if (failures != 0) {
        std::printf("publish_with_ack_queue_test: %d failure(s)\n", failures);
        return 1;
    }
    std::printf("publish_with_ack_queue_test: all checks passed\n");
    return 0;
}
