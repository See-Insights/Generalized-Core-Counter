/*
 * Project: Generalized-Core-Counter
 * Description: Generalized IoT device core for outdoor counting and occupancy tracking.
 * Supports multiple sensor types (PIR, ultrasonic, gesture detection, etc.) with
 * flexible operating modes (counting vs occupancy) and power modes (connected vs low-power).
 * Designed for remote deployment with robust error handling and cloud configuration.
 * 
 * Author: Charles McClelland
 * 
 * Date: 12/10/2025
 * License: MIT
 * Repo: https://github.com/chipmc/Generalized-Core-Counter
 */

// Version History (high level):
// v1.x - Initial gesture sensor implementation and refinements
// v2.x - Generalized sensor architecture with ISensor interface,
//        counting vs occupancy modes, PIR support, improved error handling
// v3.0 - Switched to Particle Ledger-based configuration with
//        product defaults and per-device overrides

// Include Particle Device OS APIs
#include "Particle.h"

// Global configuration (includes DEBUG_SERIAL define)
#include "Config.h"

// Firmware version recognized by Particle Product firmware management
// Bump this integer whenever you cut a new production release.
PRODUCT_VERSION(3);
#include "AB1805_RK.h"
#include "Cloud.h"
#include "LocalTimeRK.h"
#include "MyPersistentData.h"
#include "Particle_Functions.h"
#include "PublishQueuePosixRK.h"
#include "SensorManager.h"
#include "device_pinout.h"
#include "ISensor.h"
#include "SensorFactory.h"
#include "SensorDefinitions.h"
#include "Version.h"
#include "StateMachine.h"
#include "StateHandlers.h"
#include "ProjectConfig.h"

// Forward declarations in case Version.h is not picked up correctly
// by the build system in this translation unit.
extern const char* FIRMWARE_VERSION;
extern const char* FIRMWARE_RELEASE_NOTES;

/*
 * Architectural overview
 * ----------------------
 * - State machine: setup()/loop() implement a simple state machine
 *   (INITIALIZATION, CONNECTING, IDLE, SLEEPING, REPORTING, ERROR)
 *   that drives sensing, reporting, and power management.
 * - Sensor abstraction: ISensor + SensorFactory + SensorManager allow
 *   different physical sensors (PIR, ultrasonic, etc.) behind one API.
 * - Cloud configuration: the Cloud singleton uses Particle Ledger to
 *   merge product defaults (default-settings) with per-device overrides
 *   (device-settings), then applies the merged config to persistent data.
 * - Data publishing: publishData() builds a JSON payload and sends it
 *   via PublishQueuePosix (webhook) and also updates the device-data
 *   ledger for Console visibility.
 * - Connectivity: compile-time macros (Wiring_WiFi / Wiring_Cellular)
 *   select WiFi vs. cellular for radio control; Particle.connect() is
 *   used to bring up the cloud session on both.
 */

// Forward declarations
static void appWatchdogHandler(); // Application watchdog handler
void publishData();           // Publish the data to the cloud
void userSwitchISR();         // Interrupt for the user switch
void sensorISR();             // Interrupt for legacy tire-counting sensor
void countSignalTimerISR();   // Timer ISR to turn off BLUE LED
void dailyCleanup();          // Reset daily counters and housekeeping
void UbidotsHandler(const char *event, const char *data); // Webhook response handler
void publishStartupStatus();  // One-time status summary at boot
bool publishDiagnosticSafe(const char* eventName, const char* data, PublishFlags flags = PRIVATE); // Safe diagnostic publish with queue guard

// One-shot software timer to keep BLUE_LED on long enough
// to be visible for each count or PIR-triggered wake event.
Timer countSignalTimer(1000, countSignalTimerISR, true);

// Sleep configuration
SystemSleepConfiguration config; // Sleep 2.0 configuration
void outOfMemoryHandler(system_event_t event, int param);
LocalTimeConvert conv; // For converting UTC time to local time
AB1805 ab1805(Wire);   // AB1805 RTC / Watchdog

// System Health Variables
int outOfMemory = -1; // Set by outOfMemoryHandler when heap is exhausted

// ********** State Machine **********
char stateNames[7][16] = {"Initialize", "Error",     "Idle",
                          "Sleeping",   "Connecting", "Reporting",
                          "FirmwareUpdate"};
State state = INITIALIZATION_STATE;
State oldState = INITIALIZATION_STATE;

// ********** Global Flags **********
volatile bool userSwitchDetected = false;
volatile bool sensorDetect = false; // Flag for sensor interrupt
bool dataInFlight =
    false; // Flag for whether we are waiting for a response from the webhook

// Track first-connection queue behaviour for observability
bool firstConnectionObserved = false;
bool firstConnectionQueueDrainedLogged = false;

bool hibernateDisabledForSession = false;

// Track when we connected to enforce max connected time in LOW_POWER/DISCONNECTED modes
unsigned long connectedStartMs = 0;

// Suppress alert 40 (webhook timeout) after waking from overnight closed-hours hibernate
bool suppressAlert40ThisSession = false;

// ********** Timing **********
const int wakeBoundary = 1 * 3600;          // Reporting boundary (1 hour)
const unsigned long resetWait = 30000;      // Error state dwell before reset
const unsigned long maxConnectAttemptMs = 5UL * 60UL * 1000UL; // Max time to spend trying to connect per wake

void setup() {
  // Wait for serial connection when DEBUG_SERIAL is enabled
#ifdef DEBUG_SERIAL
  waitFor(Serial.isConnected, 10000);
  delay(1000);
#endif

  Log.info("===== Firmware Version %s =====", FIRMWARE_VERSION);
  Log.info("===== Release Notes: %s =====", FIRMWARE_RELEASE_NOTES);
  
  System.on(out_of_memory,
            outOfMemoryHandler); // Enabling an out of memory handler is a good
                                 // safety tip. If we run out of memory a
                                 // System.reset() is done.

  // Application watchdog: reset if loop() doesn't execute within 60 seconds.
  // This catches state machine hangs, blocking operations, and cellular/cloud
  // stalls that exceed our non-blocking design intent. The AB1805 hardware
  // watchdog (124s) provides ultimate backstop if this software watchdog fails.
  static ApplicationWatchdog appWatchdog(60000, appWatchdogHandler, 1536);
  Log.info("Application watchdog enabled: 60s timeout");

  // Subscribe to the Ubidots integration response event so we can track
  // successful webhook deliveries and update lastHookResponse.
  {
    char responseTopic[125];
    String deviceID = System.deviceID();
    deviceID.toCharArray(responseTopic, sizeof(responseTopic));
    Particle.subscribe(responseTopic, UbidotsHandler);
  }

  // Configure network stack but keep radio OFF at startup.
  // In SEMI_AUTOMATIC mode we explicitly control when the radio is turned on
  // by calling Particle.connect() from CONNECTING_STATE.
#if Wiring_WiFi
  Log.info("Platform connectivity: WiFi (radio off until CONNECTING_STATE)");
  WiFi.disconnect();
  WiFi.off();
#elif Wiring_Cellular
  Log.info("Platform connectivity: Cellular (radio off until CONNECTING_STATE)");
  Cellular.disconnect();
  Cellular.off();
#else
  Log.info("Platform connectivity: default (Particle.connect only)");
  // Fallback: rely on Particle.connect() in CONNECTING_STATE
#endif

  Particle_Functions::instance().setup(); // Initialize the Particle functions

  initializePinModes(); // Initialize the pin modes

  sysStatus.setup();    // Initialize persistent storage
  sensorConfig.setup(); // Initialize the sensor configuration
  current.setup();      // Initialize the current status data

  // Testing: clear sticky sleep-failure alert to avoid reset/deep-power loops.
  if (current.get_alertCode() == 16) {
    Log.info("Clearing alert 16 on boot");
    current.set_alertCode(0);
    current.set_lastAlertTime(0);
  }

  // Track how often the device has been resetting so the error supervisor
  // can apply backoffs and avoid permanent reset loops. Only count resets
  // that are likely to be recoverable by firmware (pin/user/watchdog).
  switch (System.resetReason()) {
  case RESET_REASON_PIN_RESET:
  case RESET_REASON_USER:
  case RESET_REASON_WATCHDOG:
    sysStatus.set_resetCount(sysStatus.get_resetCount() + 1);
    break;
  case RESET_REASON_UPDATE:
    // After OTA firmware update, force connection to reload
    // configuration from ledger. This ensures device-settings
    // (operatingMode, etc.) override any stale FRAM values.
    Log.info("OTA update detected - forcing connection to reload config");
    state = CONNECTING_STATE;
    break;
  case RESET_REASON_POWER_MANAGEMENT:
    // Waking from sleep. If current local hour matches opening hour (e.g., 07:00),
    // we likely just woke from overnight closed-hours hibernate.
    // Suppress alert 40 this session since 8+ hours without webhook responses
    // during closed hours is expected, not an error.
    // Error escalation/hard resets during other hours (08:00-22:00) will still
    // correctly trigger alert 40 if there's a real connectivity issue.
    // Note: Local time calculation happens after timezone setup, so we check this
    // later in setup() rather than here where timezone isn't configured yet.
    break;
  default:
    break;
  }

  // Ensure sensor-board LED power default matches configured sensor type
  pinMode(ledPower, OUTPUT);
  SensorType configuredType = static_cast<SensorType>(sysStatus.get_sensorType());
  const SensorDefinition* sensorDef = SensorDefinitions::getDefinition(configuredType);
  if (sensorDef && sensorDef->ledDefaultOn) {
    digitalWrite(ledPower, HIGH);
  } else {
    digitalWrite(ledPower, LOW);
  }

  // Configure publish queue to retain ~30+ days of hourly reports
  // across all supported platforms (P2, Boron, Argon). With an
  // hourly reporting interval, 800 file-backed events provide
  // headroom over the 720 events needed for a full 30 days.
  PublishQueuePosix::instance()
      .withFileQueueSize(800)
      .setup(); // Initialize the publish queue

  // Initialize AB1805 RTC and watchdog
  const bool timeValidBeforeRtc = Time.isValid();
  ab1805.withFOUT(WKP).setup();                // Initialize AB1805 RTC - WKP is D10 on Photon2
  ab1805.setWDT(AB1805::WATCHDOG_MAX_SECONDS); // Enable watchdog

  time_t rtcTime = 0;
  const bool rtcReadOk = ab1805.getRtcAsTime(rtcTime);
  const bool timeValidAfterRtc = Time.isValid();
  if (!timeValidBeforeRtc && timeValidAfterRtc) {
    if (rtcReadOk) {
      Log.info("RTC restored system time: %s (rtc=%s)",
               Time.timeStr().c_str(),
               Time.format(rtcTime, TIME_FORMAT_DEFAULT).c_str());
    } else {
      Log.info("RTC restored system time: %s (rtc read failed)",
               Time.timeStr().c_str());
    }
  } else if (!timeValidAfterRtc) {
    Log.warn("RTC did not restore time (rtcSet=%s rtcReadOk=%s)",
             ab1805.isRTCSet() ? "true" : "false",
             rtcReadOk ? "true" : "false");
  }

  Cloud::instance().setup(); // Initialize the cloud functions

  // Enqueue a one-time status snapshot so the cloud can see
  // firmware version, reset reason, and any outstanding alert
  // soon after the first successful connection.
  publishStartupStatus();

  // ===== TIME AND TIMEZONE CONFIGURATION =====
  // Setup local time from persisted timezone string (POSIX TZ format).
  // This must be configured before we can make any open/close hour decisions.
  String tz = sysStatus.get_timeZoneStr();
  if (tz.length() == 0) {
    tz = "SGT-8"; // Fallback default
    sysStatus.set_timeZoneStr(tz.c_str());
  }
  LocalTime::instance().withConfig(LocalTimePosixTimezone(tz.c_str()));

  // Validate time and configure local time converter
  if (!Time.isValid()) {
    Log.info("Time is invalid - %s so connecting", Time.timeStr().c_str());
    state = CONNECTING_STATE;
  } else {
    Log.info("Time is valid - %s", Time.timeStr().c_str());
    
    // Now that time is valid, configure local time converter for timezone-aware operations
    conv.withCurrentTime().convert();
    Log.info("Timezone: %s, Local time: %s", tz.c_str(), conv.format(TIME_FORMAT_DEFAULT).c_str());
    Log.info("Open hours %02u:00-%02u:00, currently: %s",
             sysStatus.get_openTime(), sysStatus.get_closeTime(),
             isWithinOpenHours() ? "OPEN" : "CLOSED");

    // Check if waking from overnight hibernate - suppress alert 40 since
    // 8+ hours without webhook during closed hours is expected, not an error.
    if (System.resetReason() == RESET_REASON_POWER_MANAGEMENT) {
      uint8_t localHour = (uint8_t)(conv.getLocalTimeHMS().toSeconds() / 3600);
      if (localHour == sysStatus.get_openTime()) {
        Log.info("Wake from overnight hibernate at opening hour - suppressing alert 40");
        suppressAlert40ThisSession = true;
      }
    }

    // In CONNECTED operating mode, always connect on boot to reload
    // configuration from ledger and prevent stuck-in-IDLE power drain.
    if (sysStatus.get_operatingMode() == CONNECTED) {
      Log.info("CONNECTED mode - connecting on boot to reload config");
      state = CONNECTING_STATE;
    }
  }

  Log.info("Sensor ready at startup: %s", SensorManager::instance().isSensorReady() ? "true" : "false");

  // ===== SENSOR ABSTRACTION LAYER =====
  // Initialize the sensor based on configuration using *local* time. This
  // runs after timezone configuration so open/close checks are correct.
  Log.info("Initial operatingMode: %d (%s)", sysStatus.get_operatingMode(),
           sysStatus.get_operatingMode() == 0 ? "CONNECTED" :
           sysStatus.get_operatingMode() == 1 ? "LOW_POWER" : "DISCONNECTED");

  if (!SensorManager::instance().isSensorReady()) {
    if (isWithinOpenHours()) {
      Log.info("Initializing sensor after timezone setup");
      SensorManager::instance().initializeFromConfig();

      if (!SensorManager::instance().isSensorReady()) {
        Log.error("Sensor failed to initialize after timezone setup; connecting to report error");
        state = CONNECTING_STATE;
      }
    } else {
      Log.info("Outside opening hours at startup; sensor will remain powered down");
      // Ensure carrier sensor power rails are actually turned off even if
      // we skipped sensor initialization while closed.
      Log.info("Startup CLOSED: forcing sensor power down before sleep");
      SensorManager::instance().onEnterSleep();
      Log.info("Sensor ready after startup power-down: %s", SensorManager::instance().isSensorReady() ? "true" : "false");
    }
  }
  // ===================================

  attachInterrupt(BUTTON_PIN, userSwitchISR,
                  FALLING); // We may need to monitor the user switch to change
                            // behaviours / modes

  if (state == INITIALIZATION_STATE)
    state = IDLE_STATE; // Default to IDLE; CONNECTING only when explicitly requested
  Log.info("Startup complete");
  digitalWrite(BLUE_LED, LOW); // Signal the end of startup
}

void loop() {
  // Main state machine driving sensing, reporting, power management
  switch (state) {
  case IDLE_STATE:
    handleIdleState();
    break;

  case SLEEPING_STATE:
    handleSleepingState();
    break;

  case REPORTING_STATE:
    handleReportingState();
    break;

  case CONNECTING_STATE:
    handleConnectingState();
    break;

  case FIRMWARE_UPDATE_STATE:
    handleFirmwareUpdateState();
    break;

  case ERROR_STATE:
    handleErrorState();
    break;
  }

  ab1805.loop(); // Keeps the RTC synchronized with the device clock

  // Housekeeping for each transit of the main loop
  current.loop();
  sysStatus.loop();
  sensorConfig.loop();

  // Service deferred cloud work (ledger status publishes, etc.)
  Cloud::instance().loop();

  // Service outgoing publish queue
  PublishQueuePosix::instance().loop();

  // If an out-of-memory event occurred, go to error state
  if (outOfMemory >= 0) {
    Log.info("Resetting due to low memory");
    // Out-of-memory is treated as a critical alert; only overwrite any
    // existing alert if this is more severe.
    current.raiseAlert(14);
    state = ERROR_STATE;
  }

  // If the user switch is pressed, force a connection to drain queue.
  if (userSwitchDetected) {
    Log.info("User switch pressed - connecting to drain queue");
    userSwitchDetected = false;
    state = CONNECTING_STATE;
  }

  // ********** Centralized sensor event handling **********
  // Service sensor interrupts regardless of current state. This ensures
  // counts are captured even during long-running operations like cellular
  // connection attempts (which can take minutes) or firmware updates.
  // SCHEDULED mode is time-based (handled in IDLE only), not interrupt-driven.
  uint8_t countingMode = sysStatus.get_countingMode();
  if (countingMode == COUNTING) {
    handleCountingMode();  // Count each sensor event
  } else if (countingMode == OCCUPANCY) {
    handleOccupancyMode(); // Track occupied/unoccupied state
  }

} // End of loop

// ********** Helper Functions **********

// ApplicationWatchdog expects a plain function pointer.
static void appWatchdogHandler() {
  System.reset();
}

// Helper to determine whether current *local* time is within park open hours.
// Local time is derived from LocalTimeRK using the configured timezone.
// If time is not yet valid, we treat it as "open" so the device can start
// sensing while it acquires time and configuration.
bool isWithinOpenHours() {
  if (!Time.isValid()) {
    return true;
  }

  uint8_t openHour = sysStatus.get_openTime();
  uint8_t closeHour = sysStatus.get_closeTime();
  // Use LocalTimeRK to convert UTC to local hour based on the
  // configured timezone (see setup()).
  LocalTimeConvert tempConv;
  tempConv.withConfig(LocalTime::instance().getConfig()).withCurrentTime().convert();
  uint8_t hour = (uint8_t)(tempConv.getLocalTimeHMS().toSeconds() / 3600);

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

// Helper to compute seconds until next park opening time (local time)
int secondsUntilNextOpen() {
  if (!Time.isValid()) {
    // Fallback: 1 hour if time is not yet valid
    return 3600;
  }

  uint8_t openHour = sysStatus.get_openTime();
  uint8_t closeHour = sysStatus.get_closeTime();

  LocalTimeConvert tempConv;
  tempConv.withConfig(LocalTime::instance().getConfig()).withCurrentTime().convert();
  uint32_t secondsOfDay = tempConv.getLocalTimeHMS().toSeconds();

  uint32_t openSec = (uint8_t)openHour * 3600;
  uint32_t closeSec = (uint8_t)closeHour * 3600;

  // Normalize: if we're currently within opening hours, next open is tomorrow
  if (isWithinOpenHours()) {
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
      // Otherwise next open is later today or tomorrow, but isWithinOpenHours()
      // was already false so this path will generally be rare; fall back to 1 hour
      return 3600;
    }
  } else {
    // openHour == closeHour: always open; should not normally reach here
    return 3600;
  }
}

/**
 * @brief Publish sensor data to Ubidots webhook and device-data ledger.
 *
 * @details
 * 1) Builds a compact JSON payload expected by the Ubidots webhook template
 *    and enqueues it via PublishQueuePosix to the "Ubidots-Parking-Hook-v1" event.
 * 2) Updates the Particle Ledger "device-data" with a richer JSON snapshot
 *    via Cloud::publishDataToLedger() for Console visibility.
 */
void publishData() {
  // Legacy Ubidots context strings describing battery state
  static const char *batteryContext[7] = {
    "Unknown", "Not Charging", "Charging",
    "Charged", "Discharging", "Fault",
    "Diconnected"
  };

  char data[256];

  // Compute the timestamp as the last second of the previous hour so the
  // webhook data aggregates correctly into hourly buckets in Ubidots.
  unsigned long timeStampValue = Time.now() - (Time.minute() * 60L + Time.second() + 1L);

  // Bounds check battery state index for safety
  uint8_t battState = current.get_batteryState();
  if (battState > 6) {
    battState = 0;
  }

  // Correct Ubidots webhook JSON structure
  snprintf(data, sizeof(data),
           "{\"hourly\":%i, \"daily\":%i, \"battery\":%4.2f,\"key1\":\"%s\", \"temp\":%4.2f, \"resets\":%i, \"alerts\":%i,\"connecttime\":%i,\"timestamp\":%lu000}",
           current.get_hourlyCount(),
           current.get_dailyCount(),
           current.get_stateOfCharge(),
           batteryContext[battState],
           current.get_internalTempC(),
           sysStatus.get_resetCount(),
           current.get_alertCode(),
           sysStatus.get_lastConnectionDuration(),
           timeStampValue);

  // Explicitly log the counts and alert code used in this report
  Log.info("Report payload: hourly=%d daily=%d alert=%d",
           (int)current.get_hourlyCount(),
           (int)current.get_dailyCount(),
           (int)current.get_alertCode());

  PublishQueuePosix::instance().publish(ProjectConfig::webhookEventName(), data, PRIVATE | WITH_ACK);
  Log.info("Ubidots Webhook: %s", data);

  // Also update device-data ledger with structured JSON snapshot
  if (!Cloud::instance().publishDataToLedger()) {
    // Data ledger publish failure; escalate via alert so the error
    // supervisor can decide on corrective action.
    current.raiseAlert(42);
  }
}

/**
 * @brief Enqueue a one-time startup status event summarizing
 *        firmware version, reset reason, and any active alert.
 *
 * This uses PublishQueuePosix so the event will be delivered
 * after the next successful cloud connection, even if called
 * before the radio is brought up.
 */
void publishStartupStatus() {
  char status[192];

  int resetReason = System.resetReason();
  uint32_t resetReasonData = System.resetReasonData();
  int8_t alertCode = current.get_alertCode();
  time_t lastAlert = current.get_lastAlertTime();

  snprintf(status, sizeof(status),
           "{\"version\":\"%s\",\"resetReason\":%d,\"resetReasonData\":%lu,\"alert\":%d,\"lastAlert\":%ld}",
           FIRMWARE_VERSION,
           resetReason,
           (unsigned long)resetReasonData,
           (int)alertCode,
           (long)lastAlert);

  PublishQueuePosix::instance().publish("status", status, PRIVATE | WITH_ACK);
  Log.info("Startup status: %s", status);
}

void UbidotsHandler(const char *event, const char *data) {
  // Handle response from Ubidots webhook (legacy integration)
  char responseString[64];
  // Response is only a single number thanks to Template
  if (!strlen(data)) { // No data in response - Error
    snprintf(responseString, sizeof(responseString), "No Data");
  } else if (atoi(data) == 200 || atoi(data) == 201) {
    snprintf(responseString, sizeof(responseString), "Response Received");
    dataInFlight =
        false; // We have received a response - so we can send another
    sysStatus.set_lastHookResponse(
        Time.now()); // Record the last successful Webhook Response

    // If a webhook supervision alert (40) was active, clear it now that
    // we have a confirmed successful response, so future reports reflect
    // the healthy state.
    if (current.get_alertCode() == 40) {
      current.set_alertCode(0);
      current.set_lastAlertTime(0);
    }
  } else {
    snprintf(responseString, sizeof(responseString),
             "Unknown response recevied %i", atoi(data));
  }
  if (sysStatus.get_verboseMode() && Particle.connected()) {
    publishDiagnosticSafe("Ubidots Hook", responseString, PRIVATE);
  }
  Log.info(responseString);
}

/**
 * @brief Publish a state transition to the log handler.
 */
/**
 * @brief Safely publish diagnostic message through queue with depth guard.
 *
 * @details Routes low-priority diagnostic messages through PublishQueuePosix
 *          only when queue depth is below threshold, preventing displacement
 *          of critical telemetry data during tight loops or error conditions.
 *
 * @param eventName The event name for the publish.
 * @param data The event data payload.
 * @param flags Particle publish flags (e.g., PRIVATE).
 *
 * @return true if message was queued or published, false if queue was too full.
 */
bool publishDiagnosticSafe(const char* eventName, const char* data, PublishFlags flags) {
  // Guard: only add diagnostics when queue has capacity for them.
  // Reserve headroom for critical data payloads (hourly reports, alerts).
  // Threshold: allow diagnostics if queue has <10 events pending.
  const size_t DIAGNOSTIC_QUEUE_THRESHOLD = 10;
  
  size_t queueDepth = PublishQueuePosix::instance().getNumEvents();
  
  if (queueDepth >= DIAGNOSTIC_QUEUE_THRESHOLD) {
    Log.info("Diagnostic publish skipped (queue depth=%u): %s", (unsigned)queueDepth, eventName);
    return false;
  }
  
  // Queue has capacity; safe to add diagnostic message
  PublishQueuePosix::instance().publish(eventName, data, flags | WITH_ACK);
  return true;
}

void publishStateTransition() {
  char stateTransitionString[256];
  if (state == IDLE_STATE) {
    if (!Time.isValid())
      snprintf(stateTransitionString, sizeof(stateTransitionString),
               "From %s to %s with invalid time", stateNames[oldState],
               stateNames[state]);
    else
      snprintf(stateTransitionString, sizeof(stateTransitionString),
               "From %s to %s", stateNames[oldState], stateNames[state]);
  } else
    snprintf(stateTransitionString, sizeof(stateTransitionString),
             "From %s to %s", stateNames[oldState], stateNames[state]);
  oldState = state;
  Log.info(stateTransitionString);
}

// ********** Interrupt Service Routines **********
void outOfMemoryHandler(system_event_t event, int param) {
  outOfMemory = param;
}

void userSwitchISR() { userSwitchDetected = true; }

void sensorISR() {
  static bool frontTireFlag = false;
  if (frontTireFlag || sysStatus.get_sensorType() == 1) { // Counts the rear tire for pressure sensors and once for PIR (sensor type 1)
    sensorDetect = true;                                  // sets the sensor flag for the main loop
    frontTireFlag = false;
  } else
    frontTireFlag = true;
}

void countSignalTimerISR() { digitalWrite(BLUE_LED, LOW); }

/**
 * @brief Cleanup function that is run at the beginning of the day.
 *
 * @details May or may not be in connected state.  Syncs time with remote
 * service and sets low power mode. Called from Reporting State ONLY. Cleans
 * house at the beginning of a new day.
 */
void dailyCleanup() {
  if (Particle.connected()) {
    publishDiagnosticSafe("Daily Cleanup", "Running", PRIVATE);
    
    // Force time sync once per day to prevent clock drift
    Log.info("Daily time sync requested");
    Particle.syncTime();
    sysStatus.set_lastTimeSync(Time.now());
  }
  
  Log.info("Running Daily Cleanup");
  // Leave verbose mode enabled for now to aid debugging
  if (sysStatus.get_solarPowerMode() ||
      current.get_stateOfCharge() <=
          65) { // If Solar or if the battery is being discharged
    // setLowPowerMode("1");
  }
  current
      .resetEverything(); // If so, we need to Zero the counts for the new day
}
