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
  if (sysStatus.get_sensorMode() == COUNTING) {
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

  // ********** Connectivity Decision with Battery-Aware Back-off **********
  // Instead of connecting on every report, implement progressive back-off
  // based on battery tier and connection history to extend operational life
  // in remote solar deployments with poor charging conditions.
  
  if (!Particle.connected()) {
    // ********** Auto-Cycling Test Mode **********
    // Automatically cycle through test scenarios on each report:
    // 0: 80% (HEALTHY), 1: 60% (CONSERVING), 2: 40% (CRITICAL), 3: 25% (SURVIVAL), 4: Real battery
    uint8_t scenarioIndex = sysStatus.get_testScenarioIndex();
    if (scenarioIndex != 0xFF) {
      const float testBatteryValues[] = {80.0f, 60.0f, 40.0f, 25.0f, -1.0f};
      const char* scenarioNames[] = {"HEALTHY tier test", "CONSERVING tier test", "CRITICAL tier test", "SURVIVAL tier test", "Real battery"};
      
      if (scenarioIndex < 5) {
        float batteryOverride = testBatteryValues[scenarioIndex];
        sysStatus.set_testBatteryOverride(batteryOverride);
        Log.info("AUTO-TEST: Scenario %d - %s (battery=%.1f%%)", scenarioIndex, scenarioNames[scenarioIndex], (double)batteryOverride);
        
        // Advance to next scenario for next cycle
        scenarioIndex++;
        if (scenarioIndex >= 5) {
          scenarioIndex = 0xFF;  // Done with all scenarios
          Log.info("AUTO-TEST: Completed all scenarios - disabling auto-test mode");
        }
        sysStatus.set_testScenarioIndex(scenarioIndex);
      }
    }
    
    // Calculate current battery tier with hysteresis to prevent thrashing
    // Use test override if set (>= 0), otherwise use actual battery level
    float testBatteryOverride = sysStatus.get_testBatteryOverride();
    float currentSoC = (testBatteryOverride >= 0.0f) ? testBatteryOverride : current.get_stateOfCharge();
    
    if (testBatteryOverride >= 0.0f && sysStatus.get_testScenarioIndex() == 0xFF) {
      Log.info("TEST MODE: Using battery override = %.1f%%", (double)testBatteryOverride);
    }
    
    BatteryTier newTier = Cloud::calculateBatteryTier(currentSoC);
    uint8_t prevTierValue = sysStatus.get_currentBatteryTier();
    
    // Log tier transitions for field diagnostics
    if (newTier != prevTierValue) {
      const char* tierNames[] = {"HEALTHY", "CONSERVING", "CRITICAL", "SURVIVAL"};
      const char* prevName = (prevTierValue < 4) ? tierNames[prevTierValue] : "UNKNOWN";
      const char* newName = tierNames[newTier];
      Log.info("Battery tier transition: %s → %s (SoC=%.1f%%)", prevName, newName, (double)currentSoC);
      sysStatus.set_currentBatteryTier(static_cast<uint8_t>(newTier));
    }
    
    // Calculate effective interval based on tier multiplier only
    // Connection timing is boundary-aligned, not elapsed-time based
    uint16_t baseInterval = sysStatus.get_reportingInterval();
    uint16_t tierMultiplier = Cloud::getIntervalMultiplier(newTier);
    uint32_t effectiveInterval = (uint32_t)baseInterval * tierMultiplier;
    
    // Check if current time is aligned to the effective interval boundary
    // Allow 30 second tolerance for timer jitter and processing overhead
    time_t offset = now % effectiveInterval;
    bool isAligned = (offset <= 30) || (offset >= effectiveInterval - 30);
    
    const char* tierName = (newTier == TIER_HEALTHY ? "HEALTHY" : 
                            newTier == TIER_CONSERVING ? "CONSERVING" :
                            newTier == TIER_CRITICAL ? "CRITICAL" : "SURVIVAL");
    
    if (isAligned) {
      Log.info("REPORTING: Connection due - boundary aligned tier=%s interval=%us (base=%u x %u) offset=%lus",
               tierName,
               (unsigned)effectiveInterval,
               (unsigned)baseInterval,
               (unsigned)tierMultiplier,
               (unsigned long)offset);
      state = CONNECTING_STATE;
    } else {
      time_t nextBoundary = effectiveInterval - offset;
      Log.info("REPORTING: Connection deferred - not aligned tier=%s interval=%us offset=%lus next_in=%lus",
               tierName,
               (unsigned)effectiveInterval,
               (unsigned long)offset,
               (unsigned long)nextBoundary);
      state = IDLE_STATE;
    }
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
