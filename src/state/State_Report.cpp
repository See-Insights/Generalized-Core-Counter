#include "state/State_Common.h"
#include "Config.h"
#include "Cloud.h"
#include "LocalTimeRK.h"
#include "MyPersistentData.h"
#include "PublishQueuePosixRK.h"
#include "SensorManager.h"
#include "device_pinout.h"
#include "SensorDefinitions.h"

// NOTE:
// This file was split from StateHandlers.cpp as a mechanical refactor.
// No behavioral changes were made.

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
      LocalTimeConvert convNow;
      convNow.withConfig(LocalTime::instance().getConfig()).withTime(now).convert();
      LocalTimeConvert convLast;
      convLast.withConfig(LocalTime::instance().getConfig()).withTime(lastReport).convert();

      LocalTimeYMD ymdNow = convNow.getLocalTimeYMD();
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

  sysStatus.set_lastReport(now);

  // Read battery state BEFORE connectivity decision so SoC-tiered
  // logic below uses fresh data, not stale values from a previous
  // cycle or from during an active radio session.
  measure.loop();         // Take sensor measurements for reporting
  measure.batteryState(); // Update battery SoC/state and enclosure temperature

  Log.info("Enclosure temperature at report: %4.2f C", (double)current.get_internalTempC());
  publishData(); // Queue hourly report; actual send depends on connectivity policy

  // After each hourly report, reset the hourly counter so
  // the next report contains only the counts for that hour.
  if (sysStatus.get_countingMode() == COUNTING) {
    Log.info("Resetting hourlyCount after report (was %d)", current.get_hourlyCount());
    current.set_hourlyCount(0);
  }

  // Webhook supervision: if we have not seen a successful webhook
  // response in more than 6 hours, raise alert 40 so the error
  // supervisor can evaluate and potentially reset. Threshold is set
  // higher for remote/solar units in poor reception where intermittent
  // connectivity is expected.
  // Suppress alert 40 if we just woke from overnight closed-hours sleep.
  extern bool suppressAlert40ThisSession;
  if (Time.isValid() && !suppressAlert40ThisSession) {
    time_t lastHook = sysStatus.get_lastHookResponse();
    if (lastHook != 0 && (now - lastHook) > (6 * 3600L)) {
      Log.info("No successful webhook response for >6 hours (last=%ld, now=%ld) - raising alert 40",
               (long)lastHook, (long)now);
      current.raiseAlert(40);
    }
  } else if (Time.isValid() && suppressAlert40ThisSession) {
    time_t lastHook = sysStatus.get_lastHookResponse();
    if (lastHook != 0 && (now - lastHook) > (6 * 3600L)) {
      Log.info("Webhook timeout detected after power mgmt wake - suppressing alert 40 (expected behavior)");
    }
  }

  // ********** Connectivity Decision **********
  // In low-power mode, connect on every scheduled report to drain queue.
  // User can control report frequency via reportingIntervalSec.
  if (!Particle.connected()) {
    Log.info("REPORTING: Not connected - reason=SCHEDULED_REPORT transitioning to CONNECTING_STATE");
    state = CONNECTING_STATE;
  } else {
    state = IDLE_STATE;
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
