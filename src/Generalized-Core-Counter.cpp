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
void publishData();           // Publish the data to the cloud
void userSwitchISR();         // Interrupt for the user switch
void sensorISR();             // Interrupt for legacy tire-counting sensor
void countSignalTimerISR();   // Timer ISR to turn off BLUE LED
void dailyCleanup();          // Reset daily counters and housekeeping
void softDelay(uint32_t t);   // Non-blocking delay helper
void UbidotsHandler(const char *event, const char *data); // Webhook response handler
void publishStartupStatus();  // One-time status summary at boot

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

  uint32_t openSec = (uint32_t)openHour * 3600;
  uint32_t closeSec = (uint32_t)closeHour * 3600;

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
                          "Fw Update"};
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

// ********** Timing **********
const int wakeBoundary = 1 * 3600;          // Reporting boundary (1 hour)
const unsigned long stayAwakeLong = 90000;  // Stay awake after connect (ms)
const unsigned long resetWait = 30000;      // Error state dwell before reset
unsigned long stayAwakeTimeStamp = 0;       // Timestamp for stay-awake window
unsigned long stayAwake = 0;                // How long to remain awake (ms)
const unsigned long maxOnlineWorkMs = 5UL * 60UL * 1000UL; // Max time to stay online per low-power connection
unsigned long onlineWorkStartMs = 0;       // When we first became cloud-connected this cycle
bool forceSleepThisCycle = false;          // Force sleep even if publish queue is not yet empty
bool lastLowPowerMode = false;             // Tracks previous lowPowerMode to detect transitions
bool hibernateDisabledForSession = false; // Disable HIBERNATE after first failure

void setup() {
  // Wait for serial connection for debugging (10 second timeout)
  waitFor(Serial.isConnected, 10000);
  delay(1000); // Give USB serial a moment before logging
  
  Log.info("===== Firmware Version %s =====", FIRMWARE_VERSION);
  Log.info("===== Release Notes: %s =====", FIRMWARE_RELEASE_NOTES);
  
  System.on(out_of_memory,
            outOfMemoryHandler); // Enabling an out of memory handler is a good
                                 // safety tip. If we run out of memory a
                                 // System.reset() is done.

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
  WiFi.off(); // Ensure WiFi module is powered down on boot
#elif Wiring_Cellular
  Log.info("Platform connectivity: Cellular (radio off until CONNECTING_STATE)");
  Cellular.off(); // Ensure cellular modem is powered down on boot
#else
  Log.info("Platform connectivity: default (Particle.connect only)");
  // Fallback: rely on Particle.connect() in CONNECTING_STATE
#endif

  Particle_Functions::instance().setup(); // Initialize the Particle functions

  initializePinModes(); // Initialize the pin modes

  sysStatus.setup();    // Initialize persistent storage
  sensorConfig.setup(); // Initialize the sensor configuration
  current.setup();      // Initialize the current status data

  // Track how often the device has been resetting so the error supervisor
  // can apply backoffs and avoid permanent reset loops. Only count resets
  // that are likely to be recoverable by firmware (pin/user/watchdog).
  switch (System.resetReason()) {
  case RESET_REASON_PIN_RESET:
  case RESET_REASON_USER:
  case RESET_REASON_WATCHDOG:
    sysStatus.set_resetCount(sysStatus.get_resetCount() + 1);
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

  PublishQueuePosix::instance().setup(); // Initialize the publish queue

  // Initialize AB1805 RTC and watchdog
  ab1805.withFOUT(D8).setup();                 // Initialize AB1805 RTC
  ab1805.setWDT(AB1805::WATCHDOG_MAX_SECONDS); // Enable watchdog

  Cloud::instance().setup(); // Initialize the cloud functions

  // Enqueue a one-time status snapshot so the cloud can see
  // firmware version, reset reason, and any outstanding alert
  // soon after the first successful connection.
  publishStartupStatus();

  // Check if we need to enter firmware/config update mode
  if (sysStatus.get_updatesPending()) {
    Log.info("Configuration updates pending - starting in FIRMWARE_UPDATE_STATE");
    state = FIRMWARE_UPDATE_STATE; // Override the default state to force update mode
  }

  // Service-mode override: hold the user button at startup to
  // force factory defaults and exit low-power operation. This
  // guarantees the device comes up in CONNECTED mode so a
  // field technician can reconfigure it even if previous
  // settings put it into aggressive low-power behaviour.
  if (!digitalRead(BUTTON_PIN)) { // Active-low user button
    Log.info("User button pressed at startup - forcing CONNECTED, clearing low-power defaults");
    state = CONNECTING_STATE;
    sysStatus.initialize();          // Restore factory defaults
    sysStatus.set_lowPowerMode(false);           // Ensure not in low-power mode
    sysStatus.set_operatingMode(CONNECTED);      // Persist CONNECTED operating mode
  }

  if (!Time.isValid()) {
    Log.info("Time is invalid -  %s so connecting", Time.timeStr().c_str());
    state = CONNECTING_STATE;
  } else
    Log.info("Time is valid - %s", Time.timeStr().c_str());

  // Setup local time from persisted timezone string (POSIX TZ format).
  // Example values:
  //   "SGT-8" for Singapore (no DST)
  //   "EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00" for US Eastern with DST
  String tz = sysStatus.get_timeZoneStr();
  if (tz.length() == 0) {
    tz = "SGT-8"; // Fallback default
    sysStatus.set_timeZoneStr(tz.c_str());
  }
  LocalTime::instance().withConfig(LocalTimePosixTimezone(tz.c_str()));
  conv.withCurrentTime().convert();

  // Log current local time, timezone, and whether we are within opening hours
  Log.info("Timezone at startup: %s", tz.c_str());
  Log.info("Local time at startup: %s", conv.format(TIME_FORMAT_DEFAULT).c_str());
  Log.info("Open hours %02u:00-%02u:00, currently: %s",
           sysStatus.get_openTime(), sysStatus.get_closeTime(),
           isWithinOpenHours() ? "OPEN" : "CLOSED");

  // ===== SENSOR ABSTRACTION LAYER =====
  // Initialize the sensor based on configuration using *local* time. This
  // runs after timezone configuration so open/close checks are correct.
  Log.info("Initial lowPowerMode: %s",
           sysStatus.get_lowPowerMode() ? "ON" : "OFF");

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
  // Detect transitions into low-power mode so we can start a fresh online
  // budget from the moment lowPowerMode is enabled, even if we were already
  // connected beforehand.
  bool currentLowPowerMode = sysStatus.get_lowPowerMode();
  if (currentLowPowerMode && !lastLowPowerMode) {
    if (Particle.connected()) {
      onlineWorkStartMs = millis();
    } else {
      onlineWorkStartMs = 0;
    }
    forceSleepThisCycle = false;
  }
  lastLowPowerMode = currentLowPowerMode;

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

  // If the user switch is pressed, force a report/connection
  if (userSwitchDetected) {
    Log.info("User switch pressed - sending data");
    userSwitchDetected = false;
    state = REPORTING_STATE;
  }

  // Handle any cloud configuration updates (currently no-op)
  Cloud::instance().loop();

} // End of loop

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

  PublishQueuePosix::instance().publish("Ubidots-Counter-Hook-v1", data, PRIVATE | WITH_ACK);
  Log.info("Parking Lot Webhook: %s", data);

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
    Particle.publish("Ubidots Hook", responseString, PRIVATE);
  }
  Log.info(responseString);
}

/**
 * @brief Publish a state transition to the log handler.
 */
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

// Helper kept for backwards compatibility; current logic does not use it
bool isParkOpen() {
  return !(Time.hour() < sysStatus.get_openTime() ||
           Time.hour() > sysStatus.get_closeTime());
}

/**
 * @brief Cleanup function that is run at the beginning of the day.
 *
 * @details May or may not be in connected state.  Syncs time with remote
 * service and sets low power mode. Called from Reporting State ONLY. Cleans
 * house at the beginning of a new day.
 */
void dailyCleanup() {
  if (Particle.connected())
    Particle.publish("Daily Cleanup", "Running",
                     PRIVATE); // Make sure this is being run
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

/**
 * @brief soft delay let's us process Particle functions and service the sensor
 * interrupts while pausing
 *
 * @details takes a single unsigned long input in millis
 *
 */
void softDelay(uint32_t t) {
  // Non-blocking delay that still services Particle cloud background tasks
  for (uint32_t ms = millis(); millis() - ms < t; Particle.process()) {
  }
}
