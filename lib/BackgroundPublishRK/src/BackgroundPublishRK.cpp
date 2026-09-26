#include "Particle.h"

// #include <spark_wiring_thread.h>

// Repository: https://github.com/rickkas7/BackgroundPublishRK
// License: MIT

#include "BackgroundPublishRK.h"

#include <atomic>

// BENCH ONLY: observe the cloud-status transition without changing publishing.
// Zero means this build has not observed a connected transition yet.
static Logger benchLog("app.pubq");
static std::atomic<unsigned long> benchConnectedAtMs{0};
static unsigned long benchAttemptId = 0; // Only the publish worker increments this.

static void benchCloudStatus(system_event_t event, int param) {
    if (event == cloud_status && param == cloud_status_connected) {
        const unsigned long now = millis();
        benchConnectedAtMs.store(now);
        benchLog.trace("PubqConnected: ms=%lu", now);
    } else if (event == cloud_status && param == cloud_status_disconnected) {
        benchConnectedAtMs.store(0);
    }
}

static uint32_t benchEventHash(const char *name, const char *data) {
    // FNV-1a of name + NUL + data; recompute from delivered payloads, not a unique ID.
    uint32_t hash = 2166136261u;
    for (const unsigned char *s = (const unsigned char *)name; *s; ++s) {
        hash = (hash ^ *s) * 16777619u;
    }
    hash = hash * 16777619u; // NUL separator.
    for (const unsigned char *s = (const unsigned char *)data; *s; ++s) {
        hash = (hash ^ *s) * 16777619u;
    }
    return hash;
}

BackgroundPublishRK *BackgroundPublishRK::_instance;

BackgroundPublishRK::BackgroundPublishRK() {
}

BackgroundPublishRK::~BackgroundPublishRK()
{
    stop();
}

BackgroundPublishRK &BackgroundPublishRK::instance() {
    if (!_instance) {
        _instance = new BackgroundPublishRK();
    }
    return *_instance;
}

void BackgroundPublishRK::start()
{
    if(!thread)
    {
        os_mutex_create(&mutex);
        System.on(cloud_status, benchCloudStatus);
        // Normally setup starts this worker before any connection exists.
        if (Particle.connected()) {
            benchConnectedAtMs.store(millis());
        }

        // use OS_THREAD_PRIORITY_DEFAULT so that application, system, and
        // background publish thread will all run at the same priority and
        // be able to preempt each other
        thread = new Thread("BackgroundPublishRK",
            [this]() { thread_f(); },
            OS_THREAD_PRIORITY_DEFAULT);
    }
}

void BackgroundPublishRK::stop()
{
    if(thread)
    {
        state = BACKGROUND_PUBLISH_STOP;
        thread->dispose();
        delete thread;
        thread = NULL;
    }
}

void BackgroundPublishRK::thread_f()
{
    while(true)
    {
        while(state == BACKGROUND_PUBLISH_IDLE)
        {
            // yield to rest of system while we wait
            // a condition variable would be ideal but doesn't look like
            // std::condition_variable is supported
            delay(1);
        }

        if(state == BACKGROUND_PUBLISH_STOP)
        {
            return;
        }

        // temporarily acquire the lock
        // this allows a calling thread to block the publish thread if it needs
        // additional synchronization around a publish request and acts as a
        // memory barrier around publish arguments to ensure all updates
        // are complete
        lock();
        unlock();

        // kick off the publish
        // WITH_ACK does not work as expected from a background thread
        // use the Future<bool> object directly as its default wait
        // (used by WITH_ACK) short-circuits when not called from the
        // main application thread
        const unsigned long attemptId = ++benchAttemptId;
        const uint32_t eventHash = benchEventHash(event_name, event_data);
        const unsigned long connectedMs = benchConnectedAtMs.load();
        const unsigned long attemptMs = millis();
        // Capture connection age at Particle.publish(), not at Future completion.
        // The app-thread cloud-status callback may lag the system transition.
        const unsigned long sinceConnectMs = connectedMs ? attemptMs - connectedMs : 0;
        auto ok = Particle.publish(event_name, event_data, event_flags);

        // then wait for publish to complete
        while(!ok.isDone() && state != BACKGROUND_PUBLISH_STOP)
        {
            // yield to rest of system while we wait
            delay(1);
        }

        const bool done = ok.isDone();
        const bool succeeded = ok.isSucceeded();
        const bool explicitAck = (event_flags.value() & WITH_ACK.value()) != 0;
        const char *ack = !explicitAck ? "not-observed" :
            (succeeded ? "confirmed" : "unconfirmed");
        const int error = done && !succeeded ? (int)ok.error().type() : 0;
        // One result line per actual Particle.publish() attempt. Future success
        // without explicit WITH_ACK does not establish that a cloud ACK arrived.
        benchLog.info("PubqAttempt: id=%lu h=%08lx e=%s f=%02x future=%s ack=%s err=%d cObs=%lu ck=%d dur=%lu",
            attemptId, (unsigned long)eventHash, event_name, (unsigned)event_flags.value(),
            !done ? "pending" : (succeeded ? "ok" : "failed"), ack, error,
            sinceConnectMs, connectedMs ? 1 : 0, millis() - attemptMs);

        if(completed_cb)
        {
            completed_cb(ok.isSucceeded(),
                event_name,
                event_data,
                event_context);
        }

        WITH_LOCK(*this)
        {
            if(state == BACKGROUND_PUBLISH_STOP)
            {
                return;
            }
            event_context = NULL;
            completed_cb = NULL;
            state = BACKGROUND_PUBLISH_IDLE;
        }
    }
}

bool BackgroundPublishRK::publish(const char *name, const char *data, PublishFlags flags, PublishCompletedCallback cb, const void *context)
{
    // protect against separate threads trying to publish at the same time
    WITH_LOCK(*this)

    // check currently in idle state and ready to accept publish request
    if(!thread || state != BACKGROUND_PUBLISH_IDLE)
    {
        return false;
    }

    // event name is required to publish
    // all other arguments may be be left out or defaulted
    if(!name)
    {
        return false;
    }

    // have the lock and thread is currently idle
    // safe to prepare publish request
    strncpy(event_name, name, sizeof(event_name));
    event_name[sizeof(event_name)-1] = '\0'; // ensure null termination

    if(data)
    {
        strncpy(event_data, data, sizeof(event_data));
        event_data[sizeof(event_data)-1] = '\0'; // ensure null termination
    }
    else
    {
        event_data[0] = '\0'; // null terminate at start for no event data
    }

    completed_cb = cb;
    event_context = context;
    event_flags = flags;
    state = BACKGROUND_PUBLISH_REQUESTED;

    return true;
}
