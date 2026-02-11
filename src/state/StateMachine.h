#pragma once

#include "Particle.h"
#include "AB1805_RK.h"

// Core device state machine states
enum State {
  INITIALIZATION_STATE,
  ERROR_STATE,
  IDLE_STATE,
  SLEEPING_STATE,
  CONNECTING_STATE,
  REPORTING_STATE,
  FIRMWARE_UPDATE_STATE
};

// Global state variables (defined in Generalized-Core-Counter.cpp)
extern State state;
extern State oldState;
extern char stateNames[7][16];

// Sleep configuration and RTC/watchdog (defined in Generalized-Core-Counter.cpp)
extern SystemSleepConfiguration config;
extern AB1805 ab1805;

// System health / flags shared across modules
extern int outOfMemory;
extern volatile bool userSwitchDetected;
extern volatile bool sensorDetect;

// ISR functions
void userSwitchISR();

// Session state flags consolidated into a struct
struct SessionState {
  bool firstConnectionObserved = false;
  bool firstConnectionQueueDrainedLogged = false;
  bool hibernateDisabledForSession = false;
  bool suppressAlert40ThisSession = false;
  bool awaitingWebhookResponse = false;
  bool webhookExpectedOnConnect = false;   // We queued a webhook publish and should start a response window on next cloud connect
  unsigned long webhookAwaitStartMs = 0;  // millis() when the short-term webhook response window started
  bool occupancyChangeTriggered = false;  // Report triggered by occupancy state change
  bool returnToSleepAfterReport = false;  // Should return to SLEEPING_STATE after connection
};

// Connection / power management timing shared between the
// state machine core and state handlers.
extern const unsigned long stayAwakeLong;
extern const unsigned long resetWait;
extern const unsigned long maxOnlineWorkMs;
extern unsigned long connectedStartMs;
extern SessionState session;

// Helper functions used by multiple state handlers
bool isWithinOpenHours();
int secondsUntilNextOpen();
void publishStateTransition();

// Application-level helpers implemented in Generalized-Core-Counter.cpp
void dailyCleanup();
void publishData();
