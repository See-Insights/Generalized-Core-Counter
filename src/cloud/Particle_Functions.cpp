#include "cloud/Particle_Functions.h"
#include "Particle.h"
#include "sensors/SensorManager.h"
#include "MyPersistentData.h"  // For sysStatus (serialConnected configuration)
#include "ThrashGuard.h"        // For thrash diagnostics

// Prototypes and System Mode calls
// SYSTEM_THREAD is enabled by default in Device OS 6.2.0+
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
                             {"mux", LOG_LEVEL_WARN},
                             {"system.nm", LOG_LEVEL_WARN},
                             {"system", LOG_LEVEL_WARN},
                             {"comm.dtls", LOG_LEVEL_WARN},
                             {"comm.protocol", LOG_LEVEL_WARN},
                             {"comm.protocol.handshake", LOG_LEVEL_WARN},
                             {"net.pppncp", LOG_LEVEL_WARN},
                             {"app.ab1805", LOG_LEVEL_WARN}});
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
  // Do not block waiting for USB serial; if a host is connected, logs
  // will be visible. This firmware is designed to run unattended.

  Log.info(
      "Initializing Particle functions and variables"); // Note: Don't have to
                                                        // be connected but
                                                        // these functions need
                                                        // to in first 30
                                                        // seconds
  
  // Diagnostic variables for ThrashGuard monitoring
  Particle.variable("thrashTrips", thrashTripCount);
  Particle.variable("thrashResets", thrashResetCount);
  
  // Define the Particle variables and functions
}

// This is the end of the Particle_Functions class