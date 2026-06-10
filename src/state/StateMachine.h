#pragma once

#include "Particle.h"
#include "AB1805_RK.h"

/**
 * @file StateMachine.h
 * @brief Shared state-machine types, globals, and cross-state session fields.
 */

/**
 * @brief Top-level firmware lifecycle states driven from the main loop.
 */
enum State {
  INITIALIZATION_STATE,
  ERROR_STATE,
  IDLE_STATE,
  SLEEPING_STATE,
  CONNECTING_STATE,
  REPORTING_STATE,
  FIRMWARE_UPDATE_STATE
};

/// Global state variables defined in Generalized-Core-Counter.cpp.
extern State state;
extern State oldState;
extern char stateNames[7][16];

/// Sleep configuration and AB1805 RTC/watchdog shared across state handlers.
extern SystemSleepConfiguration config;
extern AB1805 ab1805;

/// Shared system health and ISR flags.
extern int outOfMemory;
extern volatile bool userSwitchDetected;
extern volatile bool sensorDetect;

/**
 * @brief ISR used for the user/service button input.
 */
void userSwitchISR();

/**
 * @brief RAM-only session flags that coordinate behavior across state handlers.
 *
 * These values are intentionally not persisted. They describe the current wake
 * cycle, in-flight webhook expectations, modem teardown timing, and temporary
 * suppression or diagnostic state.
 */
struct SessionState {
  bool firstConnectionObserved = false;
  bool firstConnectionQueueDrainedLogged = false;
  bool suppressAlert40ThisSession = false;
  bool awaitingWebhookResponse = false;
  bool webhookExpectedOnConnect = false;   // We queued a webhook publish and should start a response window on next cloud connect
  unsigned long webhookAwaitStartMs = 0;  // millis() when the short-term webhook response window started
  bool serviceRequestTriggered = false;   // User/service button requested immediate report/connect
  bool occupancyChangeTriggered = false;  // Report triggered by occupancy state change
  bool returnToSleepAfterReport = false;  // Should return to SLEEPING_STATE after connection
  bool connectAttemptActive = false;      // CONNECTING_STATE currently owns modem recovery
  unsigned long connectAttemptBudgetMs = 0;
  bool modemUnstable = false;             // RAM-only modem instability flag for temporary containment
  bool modemStandbySuppressed = false;    // Tracks whether standby suppression is active/logged
  uint8_t modemUnstableReason = 0;        // 0=none, 1=slow_teardown, 2=connect_timeout
  uint8_t modemConnectTimeoutCount = 0;   // Consecutive connect timeout counter (RAM-only)
  unsigned long lastTeardownEndMs = 0;    // millis() when teardown last completed
  uint32_t lastCloudDisconnectElapsedMs = 0;
  uint32_t lastModemOffElapsedMs = 0;
  uint32_t lastTotalTeardownElapsedMs = 0;
  uint32_t failsafeDeferLogMask = 0;      // One-shot failsafe defer reasons logged this boot
};

/// Connection and service timing constants shared across the state machine.
extern const unsigned long stayAwakeLong;
extern const unsigned long resetWait;
extern const unsigned long maxOnlineWorkMs;
extern unsigned long connectedStartMs;
extern SessionState session;

/**
 * @brief Refreshes the awake watchdog while the firmware is intentionally running.
 */
void serviceAwakeWatchdog();

/**
 * @brief Prepares the awake watchdog for an intended sleep transition.
 *
 * @param context Short log label for the sleep site
 */
void pauseAwakeWatchdogForSleep(const char *context);

/**
 * @brief Restores the awake watchdog after a sleep return path.
 *
 * @param context Short log label for the wake site
 */
void restoreAwakeWatchdogAfterWake(const char *context);

/**
 * @brief Returns true when the local-time snapshot is inside configured open hours.
 *
 * @return true when the device should behave as within operating hours
 */
bool isWithinOpenHours();

/**
 * @brief Returns the seconds until the next configured open window begins.
 *
 * @return Seconds until the next open period, or 0 if already open
 */
int secondsUntilNextOpen();

/**
 * @brief Publishes or logs the most recent state transition.
 */
void publishStateTransition();

/**
 * @brief Requests a top-level state transition with a concise log breadcrumb.
 *
 * @param newState Target state for the next main-loop dispatch
 * @param reason Short reason label for field logs
 */
void transitionTo(State newState, const char *reason);

/**
 * @brief Runs once-per-day maintenance work after time is available.
 */
void dailyCleanup();

/**
 * @brief Publishes the current sensor payload using the configured reporting path.
 */
void publishData();
