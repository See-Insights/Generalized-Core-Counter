/*
 * Project: Generalized-Core-Counter
 * Description: Generalized IoT device core for outdoor counting and occupancy tracking.
 * Supports multiple sensor types (PIR, ultrasonic, gesture detection, etc.) with
 * flexible operating modes (counting vs occupancy) and power modes (connected vs low-power).
 * Designed for remote deployment with robust error handling and cloud configuration.
 * 
 * Author: Charles McClelland
 * Date: 12/10/2025
 * License: MIT
 * Repo: https://github.com/chipmc/Generalized-Core-Counter
 */

// Version History:
// v1.0 - Initial gesture sensor implementation
// v1.1-1.7 - Gesture sensor refinements and state machine
// v2.0 - Generalized sensor architecture with ISensor interface
//        Added counting vs occupancy modes
//        Added PIR sensor support
//        Improved error handling for remote deployments
// v2.01 - Added DeviceInfoLedger for automatic nested JSON merging
//         Added WiFi credential pre-configuration
// v2.02 - Added DeviceConfigLedger.loop() call
//         Added detailed JSON logging for ledger debugging
// v2.03 - Added update callback to reload config when ledgers sync
// v2.04 - Added 5-second delay after connect for ledger sync

const char* FIRMWARE_VERSION = "2.04";

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

// Prototype Functions
void publishData(); // Publish the data to the cloud
void publishStateTransition(
    void);            // Keeps track of state machine changes - for debugging
void userSwitchISR(); // interrupt service routime for the user switch
void sensorISR();
void countSignalTimerISR(); // Keeps the Blue LED on
void dailyCleanup();        // Reset each morning
void softDelay(
    uint32_t t); // Soft delay function to allow for non-blocking code

// ********** Mode-Specific Handler Functions **********
void handleCountingMode();      // Process sensor events in counting mode
void handleOccupancyMode();     // Process sensor events in occupancy mode
void updateOccupancyState();    // Update occupancy state based on debounce timer

// Initialize Functions
SystemSleepConfiguration config; // Initialize new Sleep 2.0 Api
void outOfMemoryHandler(system_event_t event, int param);
LocalTimeConvert conv; // For determining if the park should be opened or closed
                       // - need local time
AB1805 ab1805(Wire);   // Rickkas' RTC / Watchdog library

// System Health Variables
int outOfMemory = -1; // From reference code provided in AN0023 (see above)

// State Machine Variables
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

// Global variables
volatile bool userSwitchDetected = false;
volatile bool sensorDetect = false; // Flag for sensor interrupt
bool dataInFlight =
    false; // Flag for whether we are waiting for a response from the webhook

// Timing variables
const int wakeBoundary =
    1 * 3600 + 0 * 60 +
    0; // Sets a reporting frequency of 1 hour 0 minutes 0 seconds
const unsigned long stayAwakeLong =
    90000; // In lowPowerMode, how long to stay awake every hour
const unsigned long webhookWait =
    30000; // How long will we wait for a WebHook response
const unsigned long resetWait =
    30000; // How long will we wait in ERROR_STATE until reset
unsigned long stayAwakeTimeStamp = 0; // Timestamps for our timing variables..
unsigned long stayAwake; // Stores the time we need to wait before napping

void setup() {
  // Wait for serial connection for debugging (10 second timeout)
  waitFor(Serial.isConnected, 10000);
  delay(1000); // Give USB serial a moment before logging
  
  Log.info("===== Firmware Version %s =====", FIRMWARE_VERSION);
  
  System.on(out_of_memory,
            outOfMemoryHandler); // Enabling an out of memory handler is a good
                                 // safety tip. If we run out of memory a
                                 // System.reset() is done.

  // Start WiFi connection using credentials configured via Particle tools
  // (mobile app or CLI). WiFi credentials are not hard-coded in this firmware.
  WiFi.connect();  // Start connection immediately

  Particle_Functions::instance().setup(); // Initialize the Particle functions

  initializePinModes(); // Initialize the pin modes

  sysStatus.setup();    // Initialize persistent storage
  sensorConfig.setup(); // Initialize the sensor configuration
  current.setup();      // Initialize the current status data

  PublishQueuePosix::instance().setup(); // Initialize the Publish Queue

  ab1805.withFOUT(D8).setup();                 // Initialize AB1805 RTC
  ab1805.setWDT(AB1805::WATCHDOG_MAX_SECONDS); // Enable watchdog

  // Particle_Functions::instance().connectToCloud(); // Connect to the Particle
  // cloud

  Cloud::instance().setup(); // Initialize the cloud functions

  // Check if we need to connect for configuration updates
  if (sysStatus.get_updatesPending()) {
    Log.info("Configuration updates pending, will start in CONNECTING_STATE");
    state = CONNECTING_STATE; // Override the default state to force connection
  }

// ===== SENSOR ABSTRACTION LAYER =====
SensorType sensorType = static_cast<SensorType>(sysStatus.get_sensorType());
ISensor* sensor = SensorFactory::createSensor(sensorType);

if (sensor != nullptr) {
    SensorManager::instance().setSensor(sensor);
    SensorManager::instance().setup();
    Log.info("Sensor initialized: %s", sensor->getSensorType());
} else {
    // Sensor creation failed - connect to cloud to report error instead of resetting
    Log.error("Failed to create sensor type %d - will connect to report error", (int)sensorType);
    state = CONNECTING_STATE;  // Connect to cloud so we can report the issue remotely
    // Note: Device will continue to run without sensor, allowing remote debugging
}
// ===================================

  if (!digitalRead(BUTTON_PIN)) { // The user will press this button at startup
                                  // to reset settings
    Log.info("User button pressed at startup - setting defaults");
    state = CONNECTING_STATE;
    sysStatus.initialize(); // Make sure the device wakes up and connects -
                            // reset to defaults and exit low power mode
  }

  if (Time.day(sysStatus.get_lastConnection() != Time.day())) {
    Log.info("New day, resetting counts");
    dailyCleanup();
  }

  if (!Time.isValid()) {
    Log.info("Time is invalid -  %s so connecting", Time.timeStr().c_str());
    state = CONNECTING_STATE;
  } else
    Log.info("Time is valid - %s", Time.timeStr().c_str());

  // Setup local time and set the publishing schedule
  // LocalTime::instance().withConfig(LocalTimePosixTimezone("EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00"));
  // // East coast of the US
  LocalTime::instance().withConfig(
      LocalTimePosixTimezone("SGT-8")); // Uncomment for Singapore Time Zone
  conv.withCurrentTime().convert();

  attachInterrupt(BUTTON_PIN, userSwitchISR,
                  FALLING); // We may need to monitor the user switch to change
                            // behaviours / modes

  // if (state == INITIALIZATION_STATE) state = SLEEPING_STATE; // Sleep unless
  // otherwise from above code
  if (state == INITIALIZATION_STATE)
    state = CONNECTING_STATE; // Connect unless otherwise from above code
  Log.info("Startup complete at %s local time",
           conv.format(TIME_FORMAT_DEFAULT).c_str());
  digitalWrite(BLUE_LED, LOW); // Signal the end of startup
}

void loop() { // The main loops runs forever and checks the sensor every
              // "sensorDebounce" milliseconds
  switch (state) {
  case IDLE_STATE: { // Unlike most sketches - nodes spend most time in sleep
                     // and only transit IDLE once or twice each period
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

  case SLEEPING_STATE: {
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
      Log.info("Woke with user button - Resetting hours and going to connect");
      sysStatus.set_lowPowerMode(false);
      // sysStatus.set_closeTime(24);
      // // Not sure we want to reset the open / close time
      // sysStatus.set_openTime(0);
      userSwitchDetected = false;
      stayAwake = stayAwakeLong;
      stayAwakeTimeStamp = millis();
      state = REPORTING_STATE;
    } else { // In this state the device was awoken for hourly reporting
      softDelay(
          2000); // Gives the device a couple seconds to get the battery reading
      Log.info("Time is up at %s with %li free memory",
               Time.format((Time.now() + wakeInSeconds), "%T").c_str(),
               System.freeMemory());
      state = IDLE_STATE;
    }
  } break;

  case REPORTING_STATE: {
    if (state != oldState)
      publishStateTransition();
    sysStatus.set_lastReport(
        Time.now()); // We are only going to report once each hour from the IDLE
                     // state.  We may or may not connect to Particle
    measure.loop();  // Take Measurements here for reporting
    if (Time.hour() == sysStatus.get_openTime())
      dailyCleanup(); // Once a day, clean house and publish to Google Sheets
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
      } else if (digitalRead(BUTTON_PIN)) {
        Log.info("User switch pressed - connecting to Particle");
        state = CONNECTING_STATE; // Go to connecting state
      } else {
        Log.info("Not connecting - low power mode and user switch not pressed");
        state = IDLE_STATE; // Will send us to connecting state - and it will
                            // send us back here
      }
    }
  } break;

  case CONNECTING_STATE: { // Will connect - or not and head back to the Idle
                           // state - We are using a 3,5, 7 minute back-off
                           // approach as recommended by Particle
    static unsigned long
        connectionStartTimeStamp; // Time in Millis that helps us know how long
                                  // it took to connect
    char data[64];                // Holder for message strings

    if (state !=
        oldState) { // Non-blocking function - these are first time items
      sysStatus.set_lastConnectionDuration(
          0); // Will exit with 0 if we do not connect or are already connected.
              // If we need to connect, this will record connection time.
      publishStateTransition();
      connectionStartTimeStamp =
          millis(); // Have to use millis as the clock may get reset on connect
      Particle.connect(); // Tells Particle to connect, now we need to wait
    }

    sysStatus.set_lastConnectionDuration(
        int((millis() - connectionStartTimeStamp) / 1000));

    if (Particle.connected()) {
      sysStatus.set_lastConnection(
          Time.now()); // This is the last time we last connected
      stayAwake =
          stayAwakeLong; // Keeps device awake after reconnection - allows us
                         // some time to catch the device before it sleeps
      stayAwakeTimeStamp = millis(); // Start the stay awake timer now
      measure.getSignalStrength();   // Test signal strength since the cellular
                                     // modem is on and ready
      snprintf(data, sizeof(data), "Connected in %i secs",
               sysStatus.get_lastConnectionDuration()); // Make up connection
                                                        // string and publish
      Log.info(data);
      if (sysStatus.get_verboseMode())
        Particle.publish("Cellular", data, PRIVATE);
      
      // Wait for ledgers to sync before loading configuration
      Log.info("Waiting 5 seconds for ledgers to sync...");
      delay(5000);
      
      // Load configuration from cloud ledgers after connecting
      Cloud::instance().loadConfigurationFromCloud();
      
      state = IDLE_STATE; // so, if we are connecting to report - next step is
                          // response wait - otherwise IDLE
    } else if (sysStatus.get_lastConnectionDuration() >
               600) {      // What happens if we do not connect
      state = ERROR_STATE; // Note - not setting the ERROR timestamp to make
                           // this go quickly
    } else {
    } // We go round the main loop again
  } break;

  case ERROR_STATE: {
      static unsigned long resetTimer = 0;  // Initialize to 0
      if (state != oldState) {
          publishStateTransition();
          Log.info("Error state - resetting");
          resetTimer = millis();  // Set on state entry
      }
      if (millis() - resetTimer > resetWait)
          System.reset();
  } break;
  }

  ab1805.loop(); // Keeps the RTC synchronized with the Boron's clock

  // Housekeeping for each transit of the main loop
  current.loop();
  sysStatus.loop();

  PublishQueuePosix::instance()
      .loop(); // Check to see if we need to tend to the message queue

  if (outOfMemory >= 0) { // In this function we are going to reset the system
                          // if there is an out of memory error
    Log.info("Resetting due to low memory");
    state = ERROR_STATE;
  }

  if (userSwitchDetected) { // If the user switch is pressed - we will force a
                             // connection
    Log.info("User switch pressed - sending data");
    userSwitchDetected = false;
    state = REPORTING_STATE;
  }

  if (measure.loop()) { // If the sensor detects an event, we will
                        // publish the data
    if (sysStatus.get_verboseMode()) {
      Log.info("Sensor event detected - publishing data");
    }
    Log.info("Sending us to REPORTING_STATE");
    state = REPORTING_STATE; // Publish the data to the cloud
  }

  Cloud::instance().loop(); // Handle cloud configuration updates

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

void UbidotsHandler(
    const char *event,
    const char *data) { // Looks at the response from Ubidots - Will reset
                        // Photon if no successful response
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
 * @brief Publishes a state transition to the Log Handler and to the Particle
 * monitoring system.
 *
 * @details A good debugging tool.
 */
void publishStateTransition(void) {
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

// Here are the various hardware and timer interrupt service routines
void outOfMemoryHandler(system_event_t event, int param) {
  outOfMemory = param;
}

void userSwitchISR() {
  userSwitchDetected = true; // The the flag for the user switch interrupt
}

void sensorISR() {
  static bool frontTireFlag = false;
  if (frontTireFlag ||
      sysStatus.get_sensorType() ==
          1) { // Counts the rear tire for pressure sensors and once for PIR
    sensorDetect = true; // sets the sensor flag for the main loop
    frontTireFlag = false;
  } else
    frontTireFlag = true;
}

void countSignalTimerISR() { digitalWrite(BLUE_LED, LOW); }

bool isParkOpen() {
  if (Time.hour() < sysStatus.get_openTime() ||
      Time.hour() > sysStatus.get_closeTime())
    return false;
  else
    return true;
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
        
        if (sysStatus.get_verboseMode()) {
            Log.info("Count detected - Hourly: %d, Daily: %d", 
                     current.get_hourlyCount(), current.get_dailyCount());
        }
        
        // Optional: Flash LED to indicate count
        digitalWrite(BLUE_LED, HIGH);
        // Timer will turn it off via countSignalTimerISR
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
  sysStatus.set_verboseMode(false); // Saves bandwidth - keep extra chatter off
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
  for (uint32_t ms = millis(); millis() - ms < t; Particle.process())
    ; //  safer than a delay()
}
