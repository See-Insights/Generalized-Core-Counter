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
void publishStateTransition(); // Log state changes (useful for debugging)
void userSwitchISR();         // Interrupt for the user switch
void sensorISR();             // Interrupt for legacy tire-counting sensor
void countSignalTimerISR();   // Timer ISR to turn off BLUE LED
void dailyCleanup();          // Reset daily counters and housekeeping
void softDelay(uint32_t t);   // Non-blocking delay helper

// Helper to determine whether current *local* time is within park open hours.
// Local time is derived from LocalTimeRK using the configured timezone.
// If time is not yet valid, we treat it as "open" so the device can start
// sensing while it acquires time and configuration.
static bool isWithinOpenHours() {
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

// ********** Mode-Specific Handler Functions **********
void handleCountingMode();      // Process sensor events in counting mode
void handleOccupancyMode();     // Process sensor events in occupancy mode
void updateOccupancyState();    // Update occupancy state based on debounce timer

// Sleep configuration
SystemSleepConfiguration config; // Sleep 2.0 configuration
void outOfMemoryHandler(system_event_t event, int param);
LocalTimeConvert conv; // For converting UTC time to local time
AB1805 ab1805(Wire);   // AB1805 RTC / Watchdog

// System Health Variables
int outOfMemory = -1; // Set by outOfMemoryHandler when heap is exhausted

// ********** State Machine **********
enum State {
  INITIALIZATION_STATE,
  ERROR_STATE,
  IDLE_STATE,
  SLEEPING_STATE,
  CONNECTING_STATE,
  DISCONNECTING_STATE,
  REPORTING_STATE,
  RESP_WAIT_STATE
};
char stateNames[8][16] = {"Initialize", "Error",        "Idle",
                          "Sleeping",   "Connecting",   "Disconnecting",
                          "Reporting",  "Response Wait"};
State state = INITIALIZATION_STATE;
State oldState = INITIALIZATION_STATE;

// ********** Global Flags **********
volatile bool userSwitchDetected = false;
volatile bool sensorDetect = false; // Flag for sensor interrupt
bool dataInFlight =
    false; // Flag for whether we are waiting for a response from the webhook

// ********** Timing **********
const int wakeBoundary = 1 * 3600;          // Reporting boundary (1 hour)
const unsigned long stayAwakeLong = 90000;  // Stay awake after connect (ms)
const unsigned long resetWait = 30000;      // Error state dwell before reset
unsigned long stayAwakeTimeStamp = 0;       // Timestamp for stay-awake window
unsigned long stayAwake = 0;                // How long to remain awake (ms)

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

  // Configure and start network connectivity.
  // Compile-time macros ensure the correct API is used per platform.
#if Wiring_WiFi
  Log.info("Platform connectivity: WiFi");
  // Pre-configure WiFi credentials to speed up connection (skip scanning).
  // NOTE: WiFi credentials should be configured using Particle's setup tools
  // (e.g. mobile app or CLI). Do not hard-code SSIDs or passwords in firmware.
  WiFi.connect();  // Start WiFi connection immediately
#elif Wiring_Cellular
  Log.info("Platform connectivity: Cellular");
  Cellular.connect(); // Start cellular connection; cloud is handled via Particle.connect()
#else
  Log.info("Platform connectivity: default (Particle.connect only)");
  // Fallback: rely on Particle.connect() in CONNECTING_STATE
#endif

  Particle_Functions::instance().setup(); // Initialize the Particle functions

  initializePinModes(); // Initialize the pin modes

  sysStatus.setup();    // Initialize persistent storage
  sensorConfig.setup(); // Initialize the sensor configuration
  current.setup();      // Initialize the current status data

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

  ab1805.withFOUT(D8).setup();                 // Initialize AB1805 RTC
  ab1805.setWDT(AB1805::WATCHDOG_MAX_SECONDS); // Enable watchdog

  Cloud::instance().setup(); // Initialize the cloud functions

  // Check if we need to connect for configuration updates
  if (sysStatus.get_updatesPending()) {
    Log.info("Configuration updates pending, will start in CONNECTING_STATE");
    state = CONNECTING_STATE; // Override the default state to force connection
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
    state = CONNECTING_STATE; // Connect unless otherwise from above code
  Log.info("Startup complete");
  digitalWrite(BLUE_LED, LOW); // Signal the end of startup
}

void loop() {
  // Main state machine driving sensing, reporting, power management
  switch (state) {
  case IDLE_STATE: { // Awake, monitoring sensor and deciding what to do next
    if (state != oldState)
      publishStateTransition();
    
    // ********** Mode-Specific Sensor Handling **********
    // Call the appropriate handler based on counting mode configuration
    if (sysStatus.get_countingMode() == COUNTING) {
      handleCountingMode();  // Count each sensor event
    } else if (sysStatus.get_countingMode() == OCCUPANCY) {
      handleOccupancyMode(); // Track occupied/unoccupied state
    }
    
    // ********** Power Management **********
    if (sysStatus.get_lowPowerMode() &&
        (millis() - stayAwakeTimeStamp) > stayAwake)
      state = SLEEPING_STATE; // When in low power mode, we can nap between taps
    
    // ********** Scheduled Reporting **********
    if (Time.hour() != Time.hour(sysStatus.get_lastReport()))
      state = REPORTING_STATE; // We want to report on the hour but not after
                               // bedtime
  } break;

  case SLEEPING_STATE: { // Deep sleep between reporting intervals
    bool radioOn = false; // Flag to indicate if the radio is on
#if Wiring_WiFi
    radioOn = WiFi.ready(); // Check if the WiFi is ready
#elif Wiring_Cellular
    radioOn = Cellular.ready(); // Check if the Cellular is ready
#endif

    if (state != oldState)
      publishStateTransition(); // We will apply the back-offs before sending to
                                // ERROR state - so if we are here we will take
                                // action

    if (Particle.connected() ||
        radioOn) { // If we are connected to the cloud or the radio is on, we
                   // will disconnect
      if (!Particle_Functions::instance()
               .disconnectFromParticle()) { // Disconnect cleanly from Particle
                                            // and power down the modem
        state = ERROR_STATE;
        // current.alerts = 15;
        break;
      }
    }
    int wakeInSeconds =
        constrain(wakeBoundary - Time.now() % wakeBoundary, 1, wakeBoundary) +
        1;
    ; // Figure out how long to sleep
    // Notify sensor layer we are entering deep sleep so sensors and
    // indicator LEDs can be powered down.
    SensorManager::instance().onEnterSleep();

    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
        .gpio(BUTTON_PIN, CHANGE)
        .duration(wakeInSeconds * 1000L);
    ab1805.stopWDT(); // No watchdogs interrupting our slumber
    SystemSleepResult result =
        System.sleep(config); // Put the device to sleep device continues
                              // operations from here
    ab1805.resumeWDT();       // Wakey Wakey - WDT can resume
    if (result.wakeupPin() ==
      BUTTON_PIN) { // If the user woke the device we need to get up - device
              // was sleeping so we need to reset opening hours
      // User button always wakes sensor stack regardless of hours.
      SensorManager::instance().onExitSleep();
      Log.info("Woke with user button - Resetting hours and going to connect");
      sysStatus.set_lowPowerMode(false);
      sysStatus.set_operatingMode(CONNECTED); // Treat button wake as service/connected mode
      userSwitchDetected = false;
      stayAwake = stayAwakeLong;
      stayAwakeTimeStamp = millis();
      state = REPORTING_STATE;
    } else { // In this state the device was awoken for hourly reporting
      // Re-enable sensors only if within opening hours; otherwise they
      // remain powered down to minimize sleep current.
      if (isWithinOpenHours()) {
        SensorManager::instance().onExitSleep();
      } else {
        Log.info("Woke outside opening hours; keeping sensors powered down");
      }

      softDelay(
          2000); // Gives the device a couple seconds to get the battery reading
      Log.info("Time is up at %s with %li free memory",
               Time.format((Time.now() + wakeInSeconds), "%T").c_str(),
               System.freeMemory());
      state = IDLE_STATE;
    }
  } break;

  case REPORTING_STATE: { // Build and send periodic report
    if (state != oldState)
      publishStateTransition();
    time_t now = Time.now();
    sysStatus.set_lastReport(
      now); // We are only going to report once each hour from the IDLE
                     // state.  We may or may not connect to Particle
    measure.loop();  // Take measurements here for reporting

    // Run daily cleanup once per calendar day at park opening hour
    if (Time.isValid() && Time.hour() == sysStatus.get_openTime()) {
      time_t lastCleanup = sysStatus.get_lastDailyCleanup();
      if (lastCleanup == 0 || Time.day(now) != Time.day(lastCleanup)) {
        Log.info("Opening hour reached and daily cleanup not run today - running now");
        dailyCleanup();
        sysStatus.set_lastDailyCleanup(now);
      }
    }
    publishData(); // Publish hourly but not at opening time as there is nothing
                   // to publish
    state = IDLE_STATE; // Since we are using PublishQueuePosixRK, we don't need
                        // to wait for a response from the webhook

    if (Particle.connected() || sysStatus.get_disconnectedMode()) {
      state = IDLE_STATE; // Default behaviour would be to connect and send
                          // report to Ubidots
      Log.info("Going back to IDLE_STATE - %s",
               sysStatus.get_disconnectedMode() ? "disconnected mode"
               : Particle.connected()           ? "already connected"
                                                : "NULL");
    }
    // If we are in low power mode, we may bail if battery is too low and we
    // need to reduce reporting frequency
    if (sysStatus.get_lowPowerMode()) { // Low power mode
      if (current.get_stateOfCharge() > 65) {
        Log.info("Sufficient battery power connecting");
      } else if (current.get_stateOfCharge() <= 50 &&
                 (Time.hour() % 4)) { // If the battery level is <50%, only
                                      // connect every fourth hour
        Log.info("Not connecting - <50%% charge - four hour schedule");
        state = IDLE_STATE; // Will send us to connecting state - and it will
                            // send us back here
      } // Leave this state and go connect - will return only if we are
        // successful in connecting
      else if (current.get_stateOfCharge() <= 65 &&
               (Time.hour() % 2)) { // If the battery level is 50% -  65%, only
                                    // connect every other hour
        Log.info("Not connecting - 50-65%% charge - two hour schedule");
        state = IDLE_STATE; // Will send us to connecting state - and it will
                            // send us back here
        break; // Leave this state and go connect - will return only if we are
               // successful in connecting
      } else if (!digitalRead(BUTTON_PIN)) { // Active-low user button
        Log.info("User switch pressed - connecting to Particle");
        state = CONNECTING_STATE; // Go to connecting state
      } else {
        Log.info("Not connecting - low power mode and user switch not pressed");
        state = IDLE_STATE; // Will send us to connecting state - and it will
                            // send us back here
      }
    }
  } break;

  case CONNECTING_STATE: { // Establish cloud connection, then load config
    static unsigned long
        connectionStartTimeStamp; // Time in Millis that helps us know how long
                                  // it took to connect
    char data[64];                // Holder for message strings

    if (state != oldState) { // One-time actions when entering state
        // Reset connection duration (will be updated once connected)
        sysStatus.set_lastConnectionDuration(0);
      publishStateTransition();
      connectionStartTimeStamp =
          millis(); // Have to use millis as the clock may get reset on connect
      Particle.connect(); // Ask Device OS to connect; we'll poll status
    }

    sysStatus.set_lastConnectionDuration(
        int((millis() - connectionStartTimeStamp) / 1000));

    if (Particle.connected()) {
      sysStatus.set_lastConnection(
          Time.now()); // This is the last time we last connected
        // Keep device awake for a while after connection for troubleshooting
        stayAwake = stayAwakeLong;
      stayAwakeTimeStamp = millis(); // Start the stay awake timer now
      measure.getSignalStrength();   // Test signal strength while radio is on
      snprintf(data, sizeof(data), "Connected in %i secs",
               sysStatus.get_lastConnectionDuration()); // Make up connection
                                                        // string and publish
      Log.info(data);
      if (sysStatus.get_verboseMode())
        Particle.publish("Cellular", data, PRIVATE);
      
      // Load configuration from cloud ledgers after connecting
      Cloud::instance().loadConfigurationFromCloud();
      
      state = IDLE_STATE; // Return to idle once connected and configured
    } else if (sysStatus.get_lastConnectionDuration() >
               600) {      // What happens if we do not connect
      state = ERROR_STATE; // Note - not setting the ERROR timestamp to make
                           // this go quickly
    } else {
    } // We go round the main loop again
  } break;

    case ERROR_STATE: { // Simple error-handling: log and reset after timeout
      static unsigned long resetTimer = 0;
      if (state != oldState) {
        publishStateTransition();
        Log.info("Error state - resetting");
        resetTimer = millis();
      }
      if (millis() - resetTimer > resetWait) {
        System.reset();
      }
    } break;
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
 * @brief Publish sensor data to the cloud based on current operating mode
 * 
 * @details Creates JSON payload with mode-specific data:
 *          - COUNTING mode: hourly and daily counts
 *          - OCCUPANCY mode: occupied status, duration, total occupied time
 *          Also includes battery, signal strength, and system health data
 */
void publishData() {
    static char deviceIdBuf[25];  // Static buffer for deviceID (24 chars + null), allocated once
    char str[512];  // Stack buffer for JSON (512 bytes is acceptable for this use case)
    JSONBufferWriter writer(str, sizeof(str));
    
    writer.beginObject();
    
    // ********** Common System Data **********
    writer.name("timestamp").value((int)Time.now());
    
    // Get deviceID once into static buffer to avoid String allocation on each call
    if (deviceIdBuf[0] == '\0') {  // Only fetch once
        String id = System.deviceID();  // Temporary String, only created once
        strncpy(deviceIdBuf, id.c_str(), sizeof(deviceIdBuf) - 1);
        deviceIdBuf[sizeof(deviceIdBuf) - 1] = '\0';
    }
    writer.name("deviceId").value(deviceIdBuf);
    
    writer.name("battery").value(current.get_stateOfCharge(), 1);
    writer.name("temp").value(current.get_internalTempC(), 1);
    
    // ********** Mode-Specific Data **********
    if (sysStatus.get_countingMode() == COUNTING) {
        // Counting Mode Data
        writer.name("mode").value("counting");
        writer.name("hourlyCount").value(current.get_hourlyCount());
        writer.name("dailyCount").value(current.get_dailyCount());
        writer.name("lastCount").value((int)current.get_lastCountTime());
        
        Log.info("Publishing COUNTING data - Hourly: %d, Daily: %d", 
                 current.get_hourlyCount(), current.get_dailyCount());
        
    } else if (sysStatus.get_countingMode() == OCCUPANCY) {
        // Occupancy Mode Data
        writer.name("mode").value("occupancy");
        writer.name("occupied").value(current.get_occupied());
        
        if (current.get_occupied()) {
            // Calculate current session duration
            uint32_t sessionDuration = Time.now() - current.get_occupancyStartTime();
            writer.name("sessionDuration").value(sessionDuration);
            writer.name("occupancyStart").value((int)current.get_occupancyStartTime());
        }
        
        writer.name("totalOccupiedSec").value(current.get_totalOccupiedSeconds());
        writer.name("debounceMs").value(sysStatus.get_occupancyDebounceMs());
        
        Log.info("Publishing OCCUPANCY data - Occupied: %s, Total: %lu sec", 
                 current.get_occupied() ? "YES" : "NO", 
                 current.get_totalOccupiedSeconds());
    }
    
    // ********** Operating Configuration **********
    writer.name("powerMode").value(sysStatus.get_operatingMode() == CONNECTED ? "connected" : "lowPower");
    writer.name("triggerMode").value(sysStatus.get_triggerMode() == INTERRUPT ? "interrupt" : "scheduled");
    
    writer.endObject();
    
    // Publish to cloud via webhook (for Ubidots real-time integration)
    if (writer.buffer()) {
        PublishQueuePosix::instance().publish("sensor-data", str, PRIVATE);
        Log.info("Published to webhook: %s", str);
        
        // Also update device-data ledger for Console visibility
        Cloud::instance().publishDataToLedger();
    } else {
        Log.warn("Failed to create JSON for sensor data");
    }
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

// *************** Mode-Specific Handler Functions ***************

/**
 * @brief Handle sensor events in COUNTING mode
 * 
 * @details In counting mode, each sensor detection increments counters.
 *          Counts are tracked hourly and daily.
 *          Used for: traffic counting, people counting, event tracking
 */
void handleCountingMode() {
  // Check if sensor has new data
  if (SensorManager::instance().loop()) {
    // Increment counters
    current.set_hourlyCount(current.get_hourlyCount() + 1);
    current.set_dailyCount(current.get_dailyCount() + 1);
    current.set_lastCountTime(Time.now());

    // Log the new count once per event
    Log.info("Count detected - Hourly: %d, Daily: %d",
             current.get_hourlyCount(), current.get_dailyCount());
    
    // Briefly flash the on-module BLUE LED as a visual count indicator.
    digitalWrite(BLUE_LED, HIGH);
    delay(200);
    digitalWrite(BLUE_LED, LOW);

    // Stay in IDLE_STATE; hourly reporting will publish aggregated counts.
  }
}

/**
 * @brief Handle sensor events in OCCUPANCY mode
 * 
 * @details In occupancy mode, first detection marks space as "occupied".
 *          Space remains occupied until debounce timeout expires without new detections.
 *          Tracks total occupied time for reporting.
 *          Used for: room occupancy, parking space detection, resource availability
 */
void handleOccupancyMode() {
    // Check if sensor has new data
    if (SensorManager::instance().loop()) {
        // Sensor detected presence
        if (!current.get_occupied()) {
            // Transition from unoccupied to occupied
            current.set_occupied(true);
            current.set_occupancyStartTime(Time.now());
            
            Log.info("Space now OCCUPIED at %s", Time.timeStr().c_str());
            digitalWrite(BLUE_LED, HIGH); // Visual indicator
        }
        
        // Update last event time (resets debounce timer)
        current.set_lastOccupancyEvent(millis());
        
        if (sysStatus.get_verboseMode()) {
            uint32_t occupiedDuration = Time.now() - current.get_occupancyStartTime();
            Log.info("Occupancy event - Duration: %lu seconds", occupiedDuration);
        }
    }
    
    // Check if we need to update occupancy state (timeout check)
    updateOccupancyState();
}

/**
 * @brief Update occupancy state based on debounce timeout
 * 
 * @details If space is occupied and debounce timeout has expired without
 *          new sensor events, mark space as unoccupied.
 *          Accumulates total occupied time for daily reporting.
 */
void updateOccupancyState() {
    if (!current.get_occupied()) {
        return; // Nothing to do if not occupied
    }
    
    uint32_t debounceMs = sysStatus.get_occupancyDebounceMs();
    uint32_t timeSinceLastEvent = millis() - current.get_lastOccupancyEvent();
    
    // Check if debounce timeout has expired
    if (timeSinceLastEvent > debounceMs) {
        // Calculate this occupancy session duration
        uint32_t sessionDuration = Time.now() - current.get_occupancyStartTime();
        
        // Add to total occupied seconds for the day
        uint32_t totalOccupied = current.get_totalOccupiedSeconds() + sessionDuration;
        current.set_totalOccupiedSeconds(totalOccupied);
        
        // Mark as unoccupied
        current.set_occupied(false);
        current.set_occupancyStartTime(0);
        
        Log.info("Space now UNOCCUPIED - Session duration: %lu seconds, Total today: %lu seconds",
                 sessionDuration, totalOccupied);
        
        digitalWrite(BLUE_LED, LOW); // Turn off visual indicator
    }
}

// *************** End of Mode-Specific Functions ***************

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
inline void softDelay(uint32_t t) {
  // Non-blocking delay that still services Particle cloud background tasks
  for (uint32_t ms = millis(); millis() - ms < t; Particle.process()) {
  }
}
