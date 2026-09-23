#include "Particle.h"
#include "time/Clock.h"

#include "../Config.h"
#include "persist/SystemConfig.h"
#include "LocalTimeRK.h"            // LocalTimeConvert, LocalTime::instance()
#include "time/LocalTimeCache.h"    // Cached LocalTimeRK conversions
#include "time/ClockTrust.h"        // Sync-recency resync gate and trust signal (WO-2026-08-29-002)
#include "cloud/Cloud.h"            // Cloud::instance().requestStatusPublish()
#include "AB1805_RK.h"              // ab1805.setRtcFromSystem()

extern AB1805 ab1805; // WO-2026-08-24: defined once, in Generalized-Core-Counter.cpp

static bool isWithinOpenHoursForHour(uint8_t hour, uint8_t openHour, uint8_t closeHour) {
  if (openHour < closeHour) {
    // Simple daytime window, e.g. 6 -> 22
    return (hour >= openHour) && (hour < closeHour);
  } else if (openHour > closeHour) {
    // Overnight window, e.g. 20 -> 6
    return (hour >= openHour) || (hour < closeHour);
  } else {
    // openHour == closeHour: treat as always open
    return true;
  }
}

static int secondsUntilNextOpenForSeconds(uint32_t secondsOfDay,
                                          uint8_t openHour,
                                          uint8_t closeHour,
                                          bool openNow) {
  uint32_t openSec = (uint8_t)openHour * 3600;
  uint32_t closeSec = (uint8_t)closeHour * 3600;

  // Normalize: if we're currently within opening hours, next open is tomorrow
  if (openNow) {
    return (int)((24 * 3600UL - secondsOfDay) + openSec);
  }

  if (openHour < closeHour) {
    // Simple daytime window, closed before open or after close
    if (secondsOfDay < openSec) {
      // Before opening today
      return (int)(openSec - secondsOfDay);
    } else {
      // After closing, next open is tomorrow
      return (int)((24 * 3600UL - secondsOfDay) + openSec);
    }
  } else if (openHour > closeHour) {
    // Overnight window; closed between closeHour and openHour
    if (secondsOfDay < openSec && secondsOfDay >= closeSec) {
      // During the closed gap today
      return (int)(openSec - secondsOfDay);
    } else {
      // Otherwise next open is later today or tomorrow, but openNow
      // was already false so this path will generally be rare; fall back to 1 hour
      return 3600;
    }
  } else {
    // openHour == closeHour: always open; should not normally reach here
    return 3600;
  }
}


// Helper to determine whether current *local* time is within park open hours.
// Local time is derived from LocalTimeRK using the configured timezone.
// If time is not yet valid, we treat it as "open" so the device can start
// sensing while it acquires time and configuration.
bool isWithinOpenHours() {
  if (!Time.isValid()) {
    return true;
  }

  if (!Config::isValid(false)) {
    return true;
  }

  uint8_t openHour = SystemConfig::get_openTime();
  uint8_t closeHour = SystemConfig::get_closeTime();
  const LocalTimeCache::LocalTimeSnapshot &snapshot = LocalTimeCache::getLocalTimeSnapshot();
  const uint8_t hour = snapshot.localHour;
  const bool openNow = isWithinOpenHoursForHour(hour, openHour, closeHour);

  return openNow;
}

bool isWithinOpenHoursAt(time_t epoch) {
  if (!Time.isValid() || !Config::isValid(false)) {
    return true;
  }

  LocalTimeConvert converter;
  converter.withConfig(LocalTime::instance().getConfig()).withTime(epoch).convert();
  const uint8_t localHour = (uint8_t)(converter.getLocalTimeHMS().toSeconds() / 3600);
  return isWithinOpenHoursForHour(
      localHour, SystemConfig::get_openTime(), SystemConfig::get_closeTime());
}

// ===== WO-2026-08-29-002: monotonic-gated resync, RTC write-back, and =====
// ===== sync-recency trust signal                                     =====
//
// Root cause recap (see the Work Order's Investigation Findings 3 and 5):
// `Time.isValid()` is true as soon as `ab1805.setup()` seeds system time
// from the RTC, whether or not the RTC itself is correct. The only path
// that ever wrote the RTC in this build, `AB1805::loop()`, is latched by a
// per-boot `timeSet` member: it can fire once, as a no-op, before any real
// correction ever lands in system time, after which it silently blocks
// every later correction for that boot. `AB1805::setRtcFromSystem()`
// (`lib/AB1805_RK`, not modified here - vendored) exists for exactly this
// purpose but had zero callers.
//
// Design (2nd review fix): the RTC write-back is now a pure function of
// OBSERVED `Particle.timeSyncedLast()` state, never of request/pending
// bookkeeping. The system thread runs in this project (default since
// Device OS 6.2.0, confirmed against system/src/system_mode.cpp and the
// SYSTEM_THREAD() macro's own compiler warning), so a sync request and its
// completion are asynchronous and can race arbitrarily against loop()'s
// checks - including completing before the very next loop() iteration.
// Tying the RTC write to a `clockResyncPending`/`syncTimeDone()` branch (the
// 1st review fix's design) meant a fast completion could be observed via
// `syncTimeDone()==true` before `Particle.timeSyncedLast()` had actually
// advanced, get misclassified as "did not complete", and permanently lose
// its only path to `ab1805.setRtcFromSystem()` for that sync - silently
// reintroducing the exact bug this Work Order exists to fix, while logs
// looked healthy. The fix: track the `timeSyncedLast()` value the RTC was
// last written for (`lastRtcWriteSyncedLastMs`); on every
// `checkClockResync()` call, if `timeSyncedLast()` is nonzero and differs
// from that tracked value, a sync has genuinely advanced - write the RTC,
// stamp `lastTimeSync`, and update the tracked value - regardless of
// whether *this app* requested it. This also means a Device-OS-initiated
// sync at cloud handshake (which this app never explicitly requested) now
// correctly drives an RTC write too, closing a gap the request-tracking
// design ignored entirely.
//
// `clockResyncPending` is dropped entirely for this reason: once the RTC
// write-back no longer depends on knowing whether a specific request
// completed, the only remaining job for pending-state bookkeeping was
// pacing repeat requests - and `ClockTrust::canRetryResyncNow()` already
// does that directly from `clockResyncLastAttemptMs`, with no window where
// a stuck/racy "pending" flag could block a later request that should have
// been allowed to fire.
namespace {
uint32_t lastRtcWriteSyncedLastMs = 0; // Finding 1: only updated after a CONFIRMED successful RTC write
// Round 6 (second follow-up): split from a single "last attempt of any
// kind" timestamp into a FAILURE-only pair, so the retry floor
// (canRetryRtcWriteNow()) can never withhold a distinct new sync value
// that arrives shortly after a SUCCESSFUL write - only a repeat attempt of
// the exact value that already failed is paced. See
// ClockTrust::shouldAttemptRtcWriteNow()'s doc comment for the full
// rationale.
uint32_t lastRtcWriteFailedAttemptMs = 0; // millis() at the last FAILED write attempt, or 0 if none yet
uint32_t lastRtcWriteFailedSyncMs = 0; // the sync value lastRtcWriteFailedAttemptMs was recorded for
uint32_t clockResyncLastAttemptMs = 0;

// Finding 2 (Round 4 review): non-blocking cache for Particle.timeSyncedLast().
// See observedTimeSyncedLastMs() below for the full design writeup.
uint32_t cachedTimeSyncedLastMs = 0;

// Finding 4 (Round 4 review): millis() full-wrap tracking. lastLoopMs/
// millisWrapTrackerInitialized let tickMillisWrapTracker() detect a wrap
// (nowMs < the previous sample) by comparing consecutive samples;
// millisWrapGeneration counts how many wraps have been observed so far this
// boot. syncCaptureWrapGeneration/lastSeenSyncRawMs record the generation
// and raw value in effect the last time Particle.timeSyncedLast() (via
// observedTimeSyncedLastMs()) was observed to change, so a later comparison
// can tell whether a full wrap has occurred since that specific sync was
// captured - see ClockTrust::shouldResyncWrapAware()/isTrustedWrapAware().
uint32_t millisWrapGeneration = 0;
uint32_t lastLoopMs = 0;
bool millisWrapTrackerInitialized = false;
uint32_t syncCaptureWrapGeneration = 0;
uint32_t lastSeenSyncRawMs = 0;

// WO-2026-08-31-004 Amendment A-2: signed seconds the RTC was corrected by
// at the most recent CONFIRMED successful checkClockResync() write -
// Time.now() at write time minus the RTC's own pre-write reading. Positive
// means the RTC was behind; negative means it was ahead. -1 is the same
// "no confirmed sync yet this boot" sentinel convention reportedSyncAgeMs()/
// syncAgeSec already use in the cloud status payload - a real correction of
// exactly -1 second is possible but rare, and this field is diagnostic
// telemetry, not a control-path signal.
long lastRtcCorrectionSec = -1;
} // namespace

/**
 * @brief Non-blocking substitute for Particle.timeSyncedLast() (Finding 2,
 *        Round 4 review).
 *
 * @details Particle.timeSyncedLast() backs onto spark_sync_time_last(),
 *          which is wrapped in SYSTEM_THREAD_CONTEXT_SYNC - when called from
 *          the application thread (as here; the system thread IS started in
 *          this project, confirmed against system/src/system_mode.cpp and
 *          the SYSTEM_THREAD() macro's own compiler warning), that macro
 *          blocks the CALLING thread on future->get() until the system
 *          thread actually processes the call - which, while a cloud
 *          connection is being established, can take as long as the
 *          connection itself does. The vendored, unmodified
 *          lib/AB1805_RK/src/AB1805_RK.cpp:51-53 documents this exact hazard
 *          in a comment and guards its own call to this same API behind
 *          Particle.connected(). Before this fix, checkClockResync() called
 *          Particle.timeSyncedLast() unconditionally every loop() - ahead of
 *          serviceAwakeWatchdog() - and isClockTrusted() reached it from
 *          publishStartupStatus() during setup(); a slow cellular connect
 *          could delay the watchdog refresh past the 60s app / 124s AB1805
 *          watchdog budget, risking a watchdog-reset/boot-storm.
 *
 *          DESIGN TENSION (must be stated explicitly, not papered over): the
 *          obvious guard - only call the real API when Particle.connected()
 *          - changes what callers see while disconnected. Chosen resolution:
 *          cache the real value while connected; return the cached snapshot
 *          while disconnected, instead of a synthesized "unknown"/0 value.
 *
 *          BEHAVIORAL CONSEQUENCE: while disconnected, isClockTrusted()/
 *          checkClockResync()/logTimeDiag()/the status payload see a frozen
 *          snapshot of the last-known sync timestamp rather than a live
 *          read. This is always an accurate reflection of "has a sync
 *          completed, and when" for two reasons: (a) a sync cannot complete
 *          while disconnected anyway, so there is nothing newer to miss, and
 *          (b) trust still correctly DEGRADES the longer the device stays
 *          disconnected, because elapsedMs()/ClockTrust::isTrustedWrapAware()
 *          compute
 *          age from millis() (which keeps advancing) against this fixed
 *          cached timestamp - the clock does not get stuck falsely
 *          "trusted" while offline. A device that has never connected this
 *          boot has cachedTimeSyncedLastMs == 0 (its initial value), which
 *          reads identically to what the real API would report in that case
 *          (Particle.connected() would be false and the real API would
 *          never even be called, but the semantic answer - "no confirmed
 *          sync this boot" - is the same).
 */
uint32_t observedTimeSyncedLastMs() {
  if (Particle.connected()) {
    cachedTimeSyncedLastMs = Particle.timeSyncedLast();
  }
  return cachedTimeSyncedLastMs;
}

/**
 * @brief Finding 4 (Round 4 review): call once per loop() with the current
 *        millis() to detect a full 32-bit rollover (~49.7 days).
 *
 * @details A wrap is detected purely by comparing consecutive samples
 *          (nowMs < the previous sample) - this only requires being called
 *          more often than once per wrap period, which loop() trivially
 *          satisfies. Deliberately NOT reset on HIBERNATE wake by anything
 *          here: it doesn't need to be - millisWrapGeneration/lastLoopMs are
 *          plain RAM globals, so a HIBERNATE wake (a fresh boot) already
 *          resets them to 0 along with everything else in this translation
 *          unit's static storage.
 */
void tickMillisWrapTracker(uint32_t nowMs) {
  if (millisWrapTrackerInitialized && nowMs < lastLoopMs) {
    ++millisWrapGeneration;
  }
  lastLoopMs = nowMs;
  millisWrapTrackerInitialized = true;
}

/**
 * @brief Requests a Device OS time sync, paced by the retry floor.
 *
 * @details Fire-and-forget: `Particle.syncTime()` queues the request on the
 *          system thread. Completion is NOT tracked here - see
 *          `checkClockResync()`'s observed-state write-back below, which
 *          fires independently of who asked for the sync (this function,
 *          `dailyCleanup()`, or Device OS's own cloud-handshake sync that
 *          this app never explicitly requests).
 */
void requestClockResync(const char *reason) {
  if (!Particle.connected()) {
    return;
  }
  clockResyncLastAttemptMs = millis();
  Log.info("ClockResync: requesting sync (%s)", reason ? reason : "unspecified");
  Particle.syncTime();
}

/**
 * @brief Per-loop monotonic-gated resync request and recurring, observed-
 *        state-driven RTC write-back.
 *
 * @details Item 6 (request side): the elapsed-time gate
 *          (`ClockTrust::shouldResyncWrapAware()`) is driven by
 *          `millis()`/`Particle.timeSyncedLast()` - both monotonic - never
 *          by wall clock, so a wrong wall clock cannot postpone its own
 *          repair. `Particle.timeSyncedLast()` resets to 0 on every fresh
 *          boot, including every HIBERNATE wake, so the gate is eligible on
 *          the first check after such a wake with no hibernate-specific
 *          detection needed; within a boot that stays up without
 *          hibernating, it recurs every <=24h. Eligible is not the same as
 *          guaranteed: a request is only issued when `Particle.connected()`,
 *          so a device that wakes with a wrong clock and sleeps again before
 *          connecting is never repaired here. Closing that loop requires
 *          waiting on sync COMPLETION in the sleep gate and is
 *          WO-2026-08-31-002's scope, not this one's. Repeat requests are
 *          paced by `ClockTrust::canRetryResyncNow()`
 *          (`kMinResyncRetryIntervalMs`).
 *
 *          Item 7 (write-back side, 2nd review fix): fires whenever
 *          `Particle.timeSyncedLast()` is nonzero and has advanced past the
 *          value the RTC was last written for - a pure function of observed
 *          sync state, decoupled from request/pending bookkeeping (see the
 *          design note above). This is safe to stamp `lastTimeSync` at this
 *          point, because `timeSyncedLast()` having just advanced means the
 *          clock is trusted by definition (matches
 *          `ClockTrust::isTrustedWrapAware()`).
 *          Fires on a recurring basis every time the sync advances, not
 *          gated by `AB1805::loop()`'s per-boot `timeSet` latch, and picks
 *          up syncs regardless of whether this app, `dailyCleanup()`, or
 *          Device OS's own cloud-handshake time sync caused them.
 *
 *          Finding 1 fix: `lastTimeSync` is stamped with `Time.now()` here,
 *          at observed-sync-advance time, not at the moment any request was
 *          issued (the request-time stamp used the very clock under
 *          suspicion).
 *
 *          Finding 1 fix, Round 4: `lastRtcWriteSyncedLastMs` is now updated
 *          ONLY after `ab1805.setRtcFromSystem()` returns true. Previously it
 *          was updated unconditionally before the call, so a failed I2C
 *          write was permanently treated as done - the sync was never
 *          retried, and because the cloud sync itself had succeeded, the
 *          *request*-side gate (`shouldResync()`) also saw a recent sync and
 *          would not ask again for up to 24h, while a HIBERNATE could
 *          re-seed system time from the still-wrong RTC much sooner. Now, on
 *          a failed write, the tracked value is left unchanged, so the very
 *          next `checkClockResync()` call (still observing the same
 *          unwritten `lastSyncMs`) retries the write - recurring, not a
 *          one-shot.
 *
 *          Round 5 cleanup (Stage 7 finding 5): the recurring retry above
 *          is now paced by `ClockTrust::canRetryRtcWriteNow()`
 *          (`kMinRtcWriteRetryIntervalMs`), a monotonic millis()-based
 *          floor separate from the request-side retry floor. Without this,
 *          a PERMANENT AB1805/Wire fault would retry `setRtcFromSystem()`
 *          (a locked I2C transaction) on every single main-loop pass -
 *          potentially hundreds of times per second on a bus shared with
 *          PMIC/battery work, with matching log volume and watchdog
 *          pressure. This only paces the FAILED-write path: a confirmed
 *          successful write advances `lastRtcWriteSyncedLastMs`, which
 *          already blocks any further attempt for that sync value via the
 *          gate condition itself, so the success path's cadence is
 *          unaffected.
 *
 *          Round 6 (second follow-up) fix: the Round 5 implementation
 *          above keyed the floor to `lastRtcWriteAttemptMs`, a timestamp
 *          recorded before EVERY attempt including successes - so a
 *          DISTINCT new sync value arriving within `kMinRtcWriteRetryIntervalMs`
 *          of a successful write was wrongly withheld from the RTC too,
 *          which made the two paragraphs above false in that case. Fixed
 *          by splitting the tracked state into `lastRtcWriteFailedAttemptMs`/
 *          `lastRtcWriteFailedSyncMs`, captured ONLY in the `else` (failed)
 *          branch below, so `ClockTrust::shouldAttemptRtcWriteNow()` applies
 *          the floor only when retrying the exact value that already
 *          failed - never to a value that has not yet been attempted.
 *
 *          Finding 2 fix, Round 4: uses `observedTimeSyncedLastMs()`, not
 *          `Particle.timeSyncedLast()` directly, so this can never block
 *          past a watchdog boundary during a slow cellular connect.
 *
 *          Finding 4 fix, Round 4: the request- and write-back-side gates
 *          use `ClockTrust::shouldResyncWrapAware()`/wrap-generation
 *          tracking rather than the plain millis()-diff gate, so an ancient
 *          sync can't be misread as fresh after a full millis() rollover
 *          (~49.7 days) with no intervening confirmed sync.
 *
 *          Finding 8 (Round 4 review, accepted/documented, not "fixed"):
 *          the vendored `AB1805::loop()` (lib/AB1805_RK, untouched) has its
 *          own one-shot RTC write, gated on
 *          `!timeSet && Time.isValid() && Particle.connected() &&
 *          Particle.timeSyncedLast() != 0` - the SAME condition ("a sync has
 *          completed") this function's write-back reacts to, and
 *          `ab1805.loop()` runs immediately before `checkClockResync()` in
 *          `loop()`. So the FIRST time a sync completes each boot, BOTH
 *          fire, writing the same correct value to the RTC twice. This is a
 *          harmless, idempotent duplicate write (same value, immediate
 *          succession), not a correctness bug - it costs one extra I2C
 *          transaction once per boot. There is no accessor into the
 *          vendored `timeSet` latch to suppress it without modifying
 *          `lib/AB1805_RK`, which is out of scope. "Exactly once" elsewhere
 *          in this codebase (tests, comments) refers to THIS function's own
 *          idempotency across repeated calls with an unchanged
 *          `timeSyncedLast()` value, not to the total count of physical RTC
 *          writes across both this function and the vendored one-shot.
 */
void checkClockResync() {
  const uint32_t nowMs = millis();
  tickMillisWrapTracker(nowMs);
  const uint32_t lastSyncMs = observedTimeSyncedLastMs();

  // Track the wrap generation in effect the moment this sync value was
  // first observed, so the wrap-aware gates below can tell whether a full
  // millis() rollover has happened since (Finding 4).
  if (lastSyncMs != lastSeenSyncRawMs) {
    lastSeenSyncRawMs = lastSyncMs;
    syncCaptureWrapGeneration = millisWrapGeneration;
  }

  // --- Write-back side: pure function of observed timeSyncedLast() state. ---
  // Round 5 cleanup task 2 (Stage 7 finding 5), corrected Round 6 (second
  // follow-up): a FAILED write is paced by ClockTrust::canRetryRtcWriteNow()
  // so a permanent AB1805/Wire fault cannot retry ab1805.setRtcFromSystem()
  // (a locked I2C transaction) on every single main-loop pass. This does
  // NOT affect a distinct new sync value: lastRtcWriteFailedAttemptMs/
  // lastRtcWriteFailedSyncMs are recorded ONLY in the `else` (failed)
  // branch below, never on a success and never before the attempt, so
  // ClockTrust::shouldAttemptRtcWriteNow() only applies the floor when
  // lastSyncMs is the SAME value that most recently failed - a fresh sync
  // value is admitted immediately regardless of how recently any write was
  // last attempted.
  //
  // Round 5 cleanup task 4 (Stage 7 finding 7): the gate condition itself
  // is now ClockTrust::shouldAttemptRtcWriteNow() - a pure function shared
  // with the host tests - rather than being re-derived inline, so a host
  // test can exercise the real decision logic instead of a hand-written
  // mirror that could silently drift from it.
  if (ClockTrust::shouldAttemptRtcWriteNow(lastSyncMs, lastRtcWriteSyncedLastMs,
                                            nowMs, lastRtcWriteFailedAttemptMs,
                                            lastRtcWriteFailedSyncMs)) {
    // WO-2026-08-31-004 Amendment A-2: read the RTC's own value immediately
    // before correcting it, purely to report how far off it was - this read
    // does not gate or alter the write below in any way. If the read fails,
    // the delta is reported as unavailable (-1) rather than fabricated; see
    // this file's Amendment A note (mirroring RtcSkewTest's own "do not log
    // a fabricated value" rule) for the same reasoning applied here.
    time_t rtcPreWriteValue = 0;
    const bool rtcPreWriteReadOk = ab1805.getRtcAsTime(rtcPreWriteValue);
    const bool rtcUpdated = ab1805.setRtcFromSystem();
    const long rtcCorrectionSec = rtcPreWriteReadOk
        ? (long)(Time.now() - rtcPreWriteValue)
        : -1;
    Log.info("ClockResync: sync advanced, rtcUpdated=%d epoch=%ld correctionSec=%ld",
             rtcUpdated ? 1 : 0, (long)Time.now(), rtcCorrectionSec);
    if (rtcUpdated) {
      // Finding 1: only mark this sync value as "written" once the RTC
      // write is confirmed successful, and only then stamp lastTimeSync.
      lastRtcWriteSyncedLastMs = lastSyncMs;
      SystemConfig::set_lastTimeSync(Time.now());

      // Only persist the correction for cloud reporting once the write it
      // describes is confirmed - same "confirmed, not attempted" standard
      // Finding 1 already applies to lastRtcWriteSyncedLastMs/lastTimeSync
      // above. A failed pre-write read (rtcPreWriteReadOk == false) leaves
      // the prior confirmed value in place rather than overwriting it with
      // -1, so a transient read failure cannot erase real history.
      lastRtcCorrectionSec = rtcPreWriteReadOk ? rtcCorrectionSec : lastRtcCorrectionSec;

      // Round 5 cleanup task 3 (Stage 7 finding 3): the status payload is
      // published on connect, BEFORE this corrective sync completes (see
      // State_Connect.cpp), so it correctly read trusted=false at that
      // point. Nothing previously republished it once the sync succeeded,
      // so the ledger could go a whole session without ever showing
      // trusted=true. Flag the EXISTING deferred-republish mechanism
      // (Cloud::loop() already drains pendingStatusPublish, retrying on
      // failure) rather than publishing synchronously here - this cannot
      // block or reorder anything checkClockResync() does. The same
      // republish also carries this sync's correctionSec, computed above.
      Cloud::instance().requestStatusPublish("ClockResync");
    } else {
      // Round 6 (second follow-up): record the failure-only timestamp/value
      // pair that paces JUST this value's retries - never touched on a
      // success, so a later distinct sync is never delayed by it.
      lastRtcWriteFailedAttemptMs = nowMs;
      lastRtcWriteFailedSyncMs = lastSyncMs;
      Log.warn("ClockResync: setRtcFromSystem failed (Time.isValid=%d) - will retry after retry floor",
               Time.isValid() ? 1 : 0);
    }
  }

  // --- Request side: ask for a resync when the monotonic gate is open, ---
  // --- paced by the retry floor.                                       ---
  if (Particle.connected() &&
      ClockTrust::shouldResyncWrapAware(nowMs, lastSyncMs, millisWrapGeneration, syncCaptureWrapGeneration) &&
      ClockTrust::canRetryResyncNow(nowMs, clockResyncLastAttemptMs)) {
    requestClockResync(lastSyncMs == 0 ? "no confirmed sync yet this boot" : "24h elapsed since last confirmed sync");
  }
}

/**
 * @brief Item 8: is `Time.now()` currently trustworthy enough to persist or
 *        report?
 *
 * @details Per Finding 3, `Time.isValid()` alone is NOT sufficient - it was
 *          true throughout the incident this Work Order investigates. This
 *          additionally requires a confirmed cloud time sync within the
 *          last `ClockTrust::kMaxSyncAgeMs`, using the same monotonic,
 *          non-blocking, wrap-aware signal as the resync gate above
 *          (`observedTimeSyncedLastMs()` / `syncCaptureWrapGeneration`).
 *
 *          Acceptance criteria note (Finding 5, accepted as-is - see the
 *          Work Order): this deliberately requires BOTH `Time.isValid()`
 *          AND sync recency. `Time.isValid()` is necessary but not
 *          sufficient on its own (that is the whole point of Finding 3);
 *          sync recency is the decisive term that actually distinguishes
 *          "trusted" from "not" in every real scenario this Work Order
 *          investigates - a device with `Time.isValid()==false` is
 *          untrusted regardless of sync recency (defensive; should not
 *          occur once a sync has completed), and a device with
 *          `Time.isValid()==true` is untrusted unless it ALSO has a recent
 *          confirmed sync.
 */
bool isClockTrusted() {
  return ClockTrust::isTrustedWrapAware(Time.isValid(), millis(), observedTimeSyncedLastMs(),
                                         millisWrapGeneration, syncCaptureWrapGeneration);
}

/**
 * @brief Item 9 / Round 6 (Stage 7 finding 1): a wrap-aware, sentinel-
 *        bearing sync age for TELEMETRY ONLY - not a control-path signal.
 *
 * @details Stage 7 finding 1: `logTimeDiag()`'s `syncAgeMs` and the status
 *          payload's `syncAgeSec` (`DeviceStatusPublisher.cpp`) both used
 *          raw `ClockTrust::elapsedMs()`, which is correct for the TRUST
 *          DECISION (bounded by `kMaxSyncAgeMs` before it can ever be
 *          consulted again) but not for open-ended REPORTING: after a full
 *          ~49.7-day `millis()` wrap with no further confirmed sync,
 *          `isClockTrusted()` correctly goes false (via
 *          `syncCaptureWrapGeneration` mismatch), while the raw age would
 *          still wrap around to a small, deceptively "fresh-looking" number
 *          - telemetry would then contradict the trust verdict, which is
 *          exactly the confusion item 9 exists to prevent. Separately,
 *          before any sync has completed this boot
 *          (`observedTimeSyncedLastMs() == 0`), the raw age is elapsed-
 *          since-zero, i.e. uptime - a real number that means nothing as a
 *          "sync age".
 *
 *          This function reuses the EXACT SAME wrap-generation tracking
 *          the trust decision already uses (`millisWrapGeneration` /
 *          `syncCaptureWrapGeneration` - no parallel mechanism) and
 *          collapses both defective cases to one sentinel,
 *          `ClockTrust::kReportedSyncAgeUnavailableMs`, meaning "no
 *          reliable sync age is available to report right now" - which is
 *          true for both "never synced this boot" and "sync is stale
 *          beyond one full wrap": in both cases the caller should not, and
 *          per this function cannot, read a specific elapsed span out of
 *          it, and `isClockTrusted()` is independently false. Callers
 *          convert this raw-milliseconds sentinel to whatever
 *          representation fits their format (see `logTimeDiag()` for the
 *          log line and `DeviceStatusPublisher.cpp` for the JSON ledger
 *          field).
 *
 *          Deliberately NOT used by `checkClockResync()`, `isClockTrusted()`,
 *          or `ClockTrust::isTrustedWrapAware()`/`shouldResyncWrapAware()` -
 *          this function does not feed back into any control path; it only
 *          reports what those functions have already decided.
 */
uint32_t reportedSyncAgeMs() {
  return ClockTrust::wrapAwareReportedSyncAgeMs(millis(), observedTimeSyncedLastMs(),
                                                 millisWrapGeneration, syncCaptureWrapGeneration);
}

// Helper to compute seconds until next park opening time (local time)
int secondsUntilNextOpen() {
  if (!Time.isValid() || !Config::isValid(false)) {
    return Config::DEFAULT_REPORT_INTERVAL_SEC;
  }

  uint8_t openHour = SystemConfig::get_openTime();
  uint8_t closeHour = SystemConfig::get_closeTime();
  const LocalTimeCache::LocalTimeSnapshot &snapshot = LocalTimeCache::getLocalTimeSnapshot();
  const bool openNow = isWithinOpenHoursForHour(snapshot.localHour, openHour, closeHour);
  const int secondsUntil = secondsUntilNextOpenForSeconds(
      snapshot.localSecondsOfDay, openHour, closeHour, openNow);

  return secondsUntil;
}

// ===== New in Step 3a =====

namespace Clock {

bool isTimeValid() {
  return Time.isValid();
}

bool isPlausibleEpoch(time_t epoch) {
  static const time_t kEpochMin = 1704067200; // 2024-01-01 00:00:00 UTC
  static const time_t kEpochMax = 2051222400; // 2035-01-01 00:00:00 UTC
  return epoch >= kEpochMin && epoch < kEpochMax;
}

} // namespace Clock

// ===== New in Step 3b =====

namespace Clock {

bool isTrusted() {
  return isClockTrusted();
}

Openness openness() {
  // Never fail open: an untrusted clock or unloaded config both return
  // Unknown, explicitly - see this function's doc comment in Clock.h for
  // why isWithinOpenHours()'s fail-OPEN behavior is wrong for a decision
  // that commits the device to state.
  if (!isTrusted()) {
    return Openness::Unknown;
  }
  if (!Config::isValid(false)) {
    return Openness::Unknown;
  }

  uint8_t openHour = SystemConfig::get_openTime();
  uint8_t closeHour = SystemConfig::get_closeTime();
  const LocalTimeCache::LocalTimeSnapshot &snapshot = LocalTimeCache::getLocalTimeSnapshot();
  const bool openNow = isWithinOpenHoursForHour(snapshot.localHour, openHour, closeHour);
  return openNow ? Openness::Open : Openness::Closed;
}

} // namespace Clock

// ===== New in WO-2026-08-31-004 Amendment A-2 =====

namespace Clock {

long lastSyncCorrectionSec() {
  return lastRtcCorrectionSec;
}

} // namespace Clock
