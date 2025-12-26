#include "Particle_Functions.h"
#include "Particle.h"
#include "SensorManager.h"
#include "config.h"
#include "device_pinout.h"

// Prototypes and System Mode calls
SYSTEM_MODE(SEMI_AUTOMATIC); // This will enable user code to start executing
                             // automatically.
STARTUP(System.enableFeature(FEATURE_RESET_INFO));

#define SERIAL_LOG_LEVEL 3 // Set the logging level for the serial log handler
// Temporary - will fix with config file later

#if SERIAL_LOG_LEVEL == 0
SerialLogHandler logHandler(LOG_LEVEL_NONE); // Easier to see the program flow
#elif SERIAL_LOG_LEVEL == 1
SerialLogHandler logHandler(LOG_LEVEL_ERROR);
#elif SERIAL_LOG_LEVEL == 2
SerialLogHandler logHandler(LOG_LEVEL_WARN);
#elif SERIAL_LOG_LEVEL == 3
SerialLogHandler logHandler(LOG_LEVEL_INFO,
                            {// Logging level for non-application messages
                             {"app.pubq", LOG_LEVEL_ERROR},
                             {"app.seqfile", LOG_LEVEL_ERROR},
                             {"comm.dtls", LOG_LEVEL_ERROR},
                             {"comm", LOG_LEVEL_ERROR},
                             {"hal", LOG_LEVEL_ERROR},
                             {"comm.protocol", LOG_LEVEL_ERROR},
                             {"ncp.rltk.client", LOG_LEVEL_ERROR},
                             {"app.device", LOG_LEVEL_ERROR},
                             {"system", LOG_LEVEL_ERROR},
                             {"net.ifapi", LOG_LEVEL_ERROR},
                             {"app.system.reset", LOG_LEVEL_ERROR},
                             {"app.ab1805", LOG_LEVEL_ERROR}});
#elif SERIAL_LOG_LEVEL == 4
SerialLogHandler logHandler(LOG_LEVEL_ALL);
#endif

Particle_Functions *Particle_Functions::_instance;

// [static]
Particle_Functions &Particle_Functions::instance() {
  if (!_instance) {
    _instance = new Particle_Functions();
  }
  return *_instance;
}

Particle_Functions::Particle_Functions() {}

Particle_Functions::~Particle_Functions() {}

void Particle_Functions::setup() {
  if (sysStatus.get_serialConnected())
    waitFor(Serial.isConnected, 10000); // Wait for serial connection

  Log.info(
      "Initializing Particle functions and variables"); // Note: Don't have to
                                                        // be connected but
                                                        // these functions need
                                                        // to in first 30
                                                        // seconds
  // Define the Particle variables and functions
}

void Particle_Functions::loop() {
  // Put your code to run during the application thread loop here
}

void Particle_Functions::connectToCloud() {
  Log.info("Connecting to the Particle cloud");

  // Connect to the Particle cloud
  if (Particle.connected()) {
    Log.info("Already connected to the Particle cloud");
  } else {
    Particle.connect();
    Log.info("Connecting to the Particle cloud...");
  }
  while (!Particle.connected()) {
    Particle.publish("Status", "Waiting for cloud connection", PRIVATE);
    Log.info("Waiting for cloud connection...");
    delay(1000);
  }
  Particle.publish("Status", "Cloud connected", PRIVATE);
  Log.info("Cloud connected");
}

void Particle_Functions::connectToCloudAndLoadConfig() {
  Log.info("Connecting to cloud to load configuration");

  // Connect to cloud first
  Particle_Functions::connectToCloud();

  // Wait for connection
  unsigned long startTime = millis();
  while (!Particle.connected() &&
         millis() - startTime < 120000) { // 2 minute timeout
    Particle.process();
    delay(100);
  }

  if (Particle.connected()) {
    Log.info("Connected to cloud, loading configuration");

    // Load configuration from cloud
    if (Cloud::instance().loadConfigurationFromCloud()) {
      Log.info("Configuration loaded successfully from cloud");
      sysStatus.set_updatesPending(false);
    } else {
      Log.warn("Failed to load configuration from cloud");
    }
  } else {
    Log.warn("Failed to connect to cloud for configuration");
  }
}

bool Particle_Functions::
    disconnectFromParticle() { // Ensures we disconnect cleanly from Particle
                               // Updated based on this thread:
                               // https://community.particle.io/t/waitfor-particle-connected-timeout-does-not-time-out/59181
                               // // Updated based on this thread:
                               // https://community.particle.io/t/waitfor-particle-connected-timeout-does-not-time-out/59181
  time_t startTime = Time.now();
  Log.info("In the disconnect from Particle function");
  Particle.disconnect();                 // Disconnect from Particle
  waitForNot(Particle.connected, 15000); // Up to a 15 second delay()
  Particle.process();
  if (Particle.connected()) { // As this disconnect from Particle thing can be
                              // a·syn·chro·nous, we need to take an extra step
                              // to wait,
    Log.info("Failed to disconnect from Particle");
    return (false);
  } else
    Log.info("Disconnected from Particle in %i seconds",
             (int)(Time.now() - startTime));
  // Then we need to disconnect from Cellular and power down the cellular modem
  startTime = Time.now();
#if Wiring_Cellular
  Log.info("Disconnecting from Cellular network");
  Cellular.disconnect(); // Disconnect from the cellular network
  Cellular.off();        // Turn off the cellular modem
  waitFor(
      Cellular.isOff,
      30000); // As per TAN004:
              // https://support.particle.io/hc/en-us/articles/1260802113569-TAN004-Power-off-Recommendations-for-SARA-R410M-Equipped-Devices
  Particle.process();
  if (Cellular
          .isOn()) { // At this point, if cellular is not off, we have a problem
    Log.info("Failed to turn off the Cellular modem");
    return (false); // Let the calling function know that we were not able to
                    // turn off the cellular modem
  } else {
    Log.info("Turned off the cellular modem in %i seconds",
             (int)(Time.now() - startTime));
    return true;
  }
#elif Wiring_WiFi
  Log.info("Disconnecting from WiFi network");
  WiFi.disconnect();          // Disconnect from the WiFi network
  WiFi.off();                 // Turn off the WiFi modem
  waitFor(WiFi.isOff, 30000); // Wait for the WiFi modem to turn off
  Particle.process();
  if (WiFi.isOn()) { // At this point, if WiFi is not off, we have a problem
    Log.info("Failed to turn off the WiFi modem");
    return (false); // Let the calling function know that we were not able to
                    // turn off
  } else {
    Log.info("Turned off the WiFi modem in %i seconds",
             (int)(Time.now() - startTime));
    return true;
  }
#endif
}

// This is the end of the Particle_Functions class