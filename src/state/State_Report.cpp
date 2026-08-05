#include "state/State_Common.h"
#include "../Config.h"
#include "cloud/Cloud.h"
#include "power/ConnectivityPolicy.h"
#include "time/LocalTimeCache.h"
#include "LocalTimeRK.h"
#include "MyPersistentData.h"
#include "PublishQueuePosixRK.h"
#include "sensors/SensorManager.h"
#include "device_pinout.h"
#include "sensors/SensorDefinitions.h"
#include "reporting/ReportingPolicy.h"

// NOTE:
// This file was split from StateHandlers.cpp as a mechanical refactor.
// No behavioral changes were made.

namespace {

constexpr unsigned long MODEM_UNSTABLE_RECONNECT_DEFER_MS = 30000UL;

} // namespace

// REPORTING_STATE: Build and send periodic report
void handleReportingState() {
  if (state != oldState) {
    publishStateTransition();
  }

  time_t now = Time.now();
  // If this is the first report after a calendar *local* day boundary,
  // run the daily cleanup once to reset daily counters and housekeeping.
  if (Time.isValid()) {
    time_t lastReport = sysStatus.get_lastReport();
    if (lastReport != 0) {
      const LocalTimeCache::LocalTimeSnapshot &snapshot = LocalTimeCache::getLocalTimeSnapshot();
      LocalTimeConvert convLast;
      convLast.withConfig(LocalTime::instance().getConfig()).withTime(lastReport).convert();

      LocalTimeYMD ymdNow = snapshot.localYmd;
      LocalTimeYMD ymdLast = convLast.getLocalTimeYMD();

      if (ymdNow.getYear() != ymdLast.getYear() ||
          ymdNow.getMonth() != ymdLast.getMonth() ||
          ymdNow.getDay() != ymdLast.getDay()) {
        Log.info("New local day detected (last=%04d-%02d-%02d, current=%04d-%02d-%02d) - running dailyCleanup",
                 ymdLast.getYear(), ymdLast.getMonth(), ymdLast.getDay(),
                 ymdNow.getYear(), ymdNow.getMonth(), ymdNow.getDay());
        dailyCleanup();
        sysStatus.set_lastDailyCleanup(now);
      }
    }
  }

  // Read battery state BEFORE connectivity decision so SoC-tiered
  // logic below uses fresh data, not stale values from a previous
  // cycle or from during an active radio session.
  measure.loop();         // Take sensor measurements for reporting
  measure.batteryState(); // Update battery SoC/state and enclosure temperature

  publishData(); // Queue hourly report; actual send depends on connectivity policy

  // This timestamp is the authoritative application-report generation time.
  // Transport acceptance and successful delivery have separate diagnostics.
  sysStatus.set_lastReport(now);

  // After each hourly report, reset the hourly counter so
  // the next report contains only the counts for that hour.
  if (sysStatus.get_sensorMode() == COUNTING) {
    current.set_hourlyCount(0);
  }

  // Long-term webhook supervision
  // Requirement: when >3 hours have passed without a webhook response, take
  // escalating corrective action, but only during OPEN hours and with backoff
  // to prevent thrashing.
  bool forceConnectForLongTermWebhook = false;
  if (Time.isValid() && isWithinOpenHours() && !session.suppressAlert40ThisSession) {
    time_t lastHook = sysStatus.get_lastHookResponse();
    if (lastHook != 0) {
      const long ageSec = (long)(now - lastHook);

      // Only evaluate long-term health when we're not already in the middle of
      // the short-term response window.
      if (!session.awaitingWebhookResponse) {
        if (ageSec > ConnectivityPolicy::WEBHOOK_LONGTERM_ALERT40_SEC) {
          if (current.get_alertCode() != 40) {
            Log.info("No successful webhook response for >3 hours during OPEN hours (age=%ld sec) - raising alert 40",
                     ageSec);
          }
          current.raiseAlert(40);

          // Corrective action stage 1: periodically force a connection attempt
          // so we can validate the integration path during open hours.
          // Backoff: do not force connect more often than every 30 minutes.
          time_t lastConn = sysStatus.get_lastConnection();
          if (lastConn == 0 || (now - lastConn) > ConnectivityPolicy::WEBHOOK_LONGTERM_FORCE_CONNECT_MIN_INTERVAL_SEC) {
            forceConnectForLongTermWebhook = true;
          }
        }

        // Corrective action stage 2: if we've been failing for a long time and
        // we have connected recently but still aren't seeing hook responses,
        // escalate via ERROR_STATE (soft reset policy applies there).
        // Backoff: at most once every 3 hours.
        if (ageSec > ConnectivityPolicy::WEBHOOK_LONGTERM_ESCALATE_TO_ERROR_SEC && current.get_alertCode() == 40) {
          time_t lastConn = sysStatus.get_lastConnection();
          bool connectedRecently = (lastConn != 0 && (now - lastConn) < ConnectivityPolicy::WEBHOOK_LONGTERM_CONNECTED_RECENTLY_SEC);
          time_t lastEscalation = current.get_lastAlertTime();
          bool cooldownPassed = (lastEscalation == 0 || (now - lastEscalation) > ConnectivityPolicy::WEBHOOK_LONGTERM_ESCALATION_COOLDOWN_SEC);
          if (connectedRecently && cooldownPassed) {
            Log.warn("Webhook long-term failure persists (age=%ld sec) - escalating to ERROR_STATE (backoff ok)", ageSec);
            // Repurpose lastAlertTime as our escalation timestamp for alert 40.
            current.set_lastAlertTime(now);
            transitionTo(ERROR_STATE, "webhook long-term failure");
            return;
          }
        }
      }
    }
  }

  // ********** Connectivity Decision with Battery-Aware Back-off **********
  // Instead of connecting on every report, implement progressive back-off
  // based on battery tier and connection history to extend operational life
  // in remote solar deployments with poor charging conditions.
  bool serviceRequestTriggered = session.serviceRequestTriggered;
  if (serviceRequestTriggered) {
    session.serviceRequestTriggered = false;
  }

  if (!Config::isValid(true)) {
    Log.warn("REPORTING: configuration invalid - forcing CONNECTING_STATE");
    transitionTo(CONNECTING_STATE, "config invalid");
    return;
  }
  
  if (!Particle.connected()) {
    float currentSoC = current.get_stateOfCharge();
    const ReportingPolicy reportingPolicy =
        ReportingPolicyResolver::resolveRuntime(currentSoC, now);
    applyBatteryAwareConnectionModePolicy(currentSoC, reportingPolicy.batteryTier);

    const char *tierName = ReportingPolicyResolver::batteryTierName(
        reportingPolicy.batteryTier);
  #if !ENABLE_CONNECT_DECISION_TRACE
    (void)tierName;
  #endif
    bool deferAutoConnectForUnstableModem = false;
    unsigned long reconnectDeferRemainingMs = 0;
    if (session.modemUnstable && session.lastTeardownEndMs != 0) {
      unsigned long sinceTeardownMs = millis() - session.lastTeardownEndMs;
      if (sinceTeardownMs < MODEM_UNSTABLE_RECONNECT_DEFER_MS) {
        deferAutoConnectForUnstableModem = true;
        reconnectDeferRemainingMs = MODEM_UNSTABLE_RECONNECT_DEFER_MS - sinceTeardownMs;
      }
    }
    
    // Check if occupied in low-power mode - need to return to sleep after reporting
    // to wake periodically and check debounce timeout
    if (current.get_occupied() && sysStatus.get_connectionMode() != CONNECTED) {
#if ENABLE_CONNECT_DECISION_TRACE
      Log.info("REPORTING: Occupied in low-power mode - will return to sleep after report tier=%s",
               tierName);
#endif
      session.returnToSleepAfterReport = true;
    }
    
    // Check if report was triggered by occupancy state change
    if (serviceRequestTriggered) {
#if ENABLE_CONNECT_DECISION_TRACE
      Log.info("REPORTING: Immediate connection (service request) - bypassing alignment tier=%s",
               tierName);
#endif
      transitionTo(CONNECTING_STATE, "service request");
    } else if (session.occupancyChangeTriggered) {
      session.occupancyChangeTriggered = false;  // Clear flag after processing
#if ENABLE_CONNECT_DECISION_TRACE
      Log.info("REPORTING: Immediate connection (occupancy change) - bypassing alignment tier=%s",
               tierName);
#endif
      transitionTo(CONNECTING_STATE, "occupancy change");
    } else if (forceConnectForLongTermWebhook) {
#if ENABLE_CONNECT_DECISION_TRACE
      Log.info("REPORTING: Forcing connection due to long-term webhook health (OPEN hours)");
#endif
      transitionTo(CONNECTING_STATE, "webhook health check");
    } else if (sysStatus.get_connectionMode() == INTERMITTENT_KEEP_ALIVE) {
      // In INTERMITTENT_KEEP_ALIVE mode, connect immediately for all reports
      if (deferAutoConnectForUnstableModem) {
        Log.warn("MODEM_POLICY: reconnect deferred reason=unstable_modem remaining=%lu ms trigger=keep_alive",
                 reconnectDeferRemainingMs);
        transitionTo(IDLE_STATE, "modem unstable");
      } else {
#if ENABLE_CONNECT_DECISION_TRACE
        Log.info("REPORTING: Immediate connection (KEEP_ALIVE mode) - bypassing alignment tier=%s",
                 tierName);
#endif
        transitionTo(CONNECTING_STATE, "keep alive mode");
      }
    } else if (reportingPolicy.cadenceDue) {
      if (deferAutoConnectForUnstableModem) {
        Log.warn("MODEM_POLICY: reconnect deferred reason=unstable_modem remaining=%lu ms trigger=aligned",
                 reconnectDeferRemainingMs);
        transitionTo(IDLE_STATE, "modem unstable aligned");
      } else {
#if ENABLE_CONNECT_DECISION_TRACE
        Log.info("REPORTING: Connection due - boundary aligned tier=%s interval=%lus (base=%lu x %u)",
                 tierName,
                 (unsigned long)reportingPolicy.effectiveIntervalSec,
                 (unsigned long)reportingPolicy.configuredIntervalSec,
                 (unsigned)reportingPolicy.batteryMultiplier);
#endif
        transitionTo(CONNECTING_STATE, "boundary aligned");
      }
    } else {
      const time_t nextBoundary = reportingPolicy.nextReportEpoch > now
          ? reportingPolicy.nextReportEpoch - now
          : 0;
#if ENABLE_CONNECT_DECISION_TRACE
      Log.info("REPORTING: Connection deferred - cadence closed tier=%s interval=%lus next_in=%lus window=%d",
               tierName,
               (unsigned long)reportingPolicy.effectiveIntervalSec,
               (unsigned long)nextBoundary,
               reportingPolicy.windowOpen ? 1 : 0);
    #else
      (void)nextBoundary;
#endif
      transitionTo(IDLE_STATE, "not aligned");
    }
  } else {
    transitionTo(IDLE_STATE, "already connected");
  }

  // If a webhook supervision alert (40) has been raised, we leave the
  // state machine to proceed with its normal connection behaviour. The
  // error supervisor can still evaluate alert 40 via resolveErrorAction,
  // but we no longer override the connection decision here, so the
  // device can continue to attempt hourly connections.
  if (current.get_alertCode() == 40) {
    Log.info("Alert 40 active after report - continuing normal state flow (no immediate ERROR_STATE)");
  }
}
