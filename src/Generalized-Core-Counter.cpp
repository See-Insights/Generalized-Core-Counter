/*
 * Project: Generalized-Core-Counter
 * Description: Generalized IoT device core for outdoor counting and occupancy tracking.
 * Supports multiple sensor types (PIR, ultrasonic, gesture detection, etc.) with
 * flexible operating modes (counting vs occupancy) and power modes (connected vs low-power).
 * Designed for remote deployment with robust error handling and cloud configuration.
 * 
 * Author: Charles McClelland
 *
 * Date: 07/22/2026
 * Version: 21.0_Test - Effective reporting policy contract, startup snapshot
 *   observability, and battery backoff refactor. See CHANGELOG.md.
 * License: MIT
 * Repo: https://github.com/chipmc/Generalized-Core-Counter
 */

// Include Particle Device OS APIs
#include "Particle.h"

// Global configuration (includes DEBUG_SERIAL define)
#include "Config.h"
#include "state/State_Common.h"
#include "state/SleepPrepSpanTiming.h"
#include "power/Connectivity.h"
#include "power/ConnectivityPolicy.h"
#include "power/PowerManager.h"
#include "power/PowerPlatform.h"
#include "power/PowerDiagnostics.h"
#include "observability/WakeCycleStats.h"
#include "observability/StartupSnapshotRuntime.h"
#include "diagnostics/ConnectivityFailsafeTest.h"
#include "ThrashGuard.h"

// Firmware version metadata
#include "FirmwareVersion.h"

// Firmware version recognized by Particle Product firmware management
PRODUCT_VERSION(FIRMWARE_PRODUCT_VERSION);

// Hardware abstraction and device-specific pinouts
#include "device_pinout.h"           // Platform-specific pin definitions
#include "AB1805_RK.h"               // RTC and hardware watchdog
#include "LocalTimeRK.h"             // Timezone conversion (UTC to local)
#include "time/LocalTimeCache.h"          // Cached LocalTimeRK conversions

// Persistent data and configuration
#include "MyPersistentData.h"        // FRAM-backed sysStatus, current, sensorConfig

// Cloud connectivity and data publishing
#include "cloud/Cloud.h"                   // Particle Ledger integration (config + data)
#include "PublishQueuePosixRK.h"     // File-backed persistent event queue
#include "cloud/Particle_Functions.h"      // Particle.function() and Particle.variable() registration
#include "reporting/ReportingPolicy.h"    // Shared cloud-report cadence policy

// Sensor abstraction layer
#include "sensors/SensorManager.h"           // Singleton managing active sensor instance
#include "sensors/SensorFactory.h"           // Factory for creating sensor instances by type
#include "sensors/SensorDefinitions.h"       // SensorType enum and counting mode constants

// State machine implementation
#include "state/StateMachine.h"            // State enum and global state variables
#include "state/StateHandlers.h"           // Handler functions for each state

// Application metadata
#include "Version.h"                 // FIRMWARE_VERSION and FIRMWARE_RELEASE_NOTES
#include "ProjectConfig.h"           // Webhook event name and project constants

// Forward declarations for firmware metadata referenced from this translation unit.
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
 *
 * File navigation
 * ---------------
 * 1) Includes, globals, retained state, and small formatting helpers
 * 2) setup()/loop() and top-level lifecycle wiring
 * 3) policy helpers for battery tier, open hours, and report publishing
 * 4) startup status, webhook supervision, diagnostics, and state logging
 * 5) connectivity failsafe, ISRs, and daily maintenance
 */

// Forward declarations
#if Wiring_Watchdog
static void awakeWatchdogExpiredHandler();
#else
static void appWatchdogHandler(); // Application watchdog handler
#endif
void publishData();           // Publish the data to the cloud
void userSwitchISR();         // Interrupt for the user switch
void sensorISR();             // Interrupt for legacy tire-counting sensor
void dailyCleanup();          // Reset daily counters and housekeeping
void UbidotsHandler(const char *event, const char *data); // Webhook response handler
void publishStartupStatus();  // One-time status summary at boot
void publishWatchdogForensics(bool ab1805Confirmed = false); // One-time watchdog forensic snapshot at boot
bool publishDiagnosticSafe(const char* eventName, const char* data, PublishFlags flags = PRIVATE); // Safe diagnostic publish with queue guard
void applyBatteryAwareConnectionModePolicy(float currentSoC, BatteryTier resolvedTier);
void applyBatteryAwareConnectionModePolicy(float currentSoC);
void clearConnectivityFailsafeRecovery(const char *reason);
void connectivityFailsafeSupervisor();

// ===== Global runtime objects =====

SystemSleepConfiguration config; // Sleep 2.0 configuration
void outOfMemoryHandler(system_event_t event, int param);
LocalTimeConvert conv; // For converting UTC time to local time
AB1805 ab1805(Wire);   // AB1805 RTC / Watchdog

// System health flag set from the out-of-memory callback.
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

// Session state - consolidated flags for connection/webhook/hibernate tracking
SessionState session;

// Track when we connected to enforce max connected time in LOW_POWER/DISCONNECTED modes
unsigned long connectedStartMs = 0;

namespace {

constexpr unsigned long AWAKE_WATCHDOG_TIMEOUT_MS = 60000UL;
constexpr unsigned long REPORT_FORENSICS_SLOW_LOG_THRESHOLD_MS = 250UL;
constexpr unsigned long REPORT_FORENSICS_ABNORMAL_WARN_THRESHOLD_MS = 1000UL;

enum AwakeWatchdogSleepStrategy : uint8_t {
  AWAKE_WATCHDOG_SLEEP_CONFIG_PAUSE = 0,
  AWAKE_WATCHDOG_SLEEP_MANUAL_STOP = 1,
};

#if Wiring_Watchdog
AwakeWatchdogSleepStrategy awakeWatchdogSleepStrategy =
#if PLATFORM_ID == PLATFORM_P2 || (defined(PLATFORM_PHOTON2) && PLATFORM_ID == PLATFORM_PHOTON2)
    AWAKE_WATCHDOG_SLEEP_MANUAL_STOP;
#else
    AWAKE_WATCHDOG_SLEEP_CONFIG_PAUSE;
#endif
bool awakeWatchdogInitialized = false;
bool awakeWatchdogStarted = false;
#endif

const char *awakeWatchdogSleepStrategyName(AwakeWatchdogSleepStrategy strategy) {
  switch (strategy) {
  case AWAKE_WATCHDOG_SLEEP_MANUAL_STOP:
    return "manual-stop";
  case AWAKE_WATCHDOG_SLEEP_CONFIG_PAUSE:
  default:
    return "config-pause";
  }
}

enum AppBreadcrumb : uint8_t {
  BREADCRUMB_NONE = 0,
  BREADCRUMB_SETUP_START = 1,
  BREADCRUMB_SETUP_COMPLETE = 2,
  BREADCRUMB_SLEEP_ENTRY = 3,
  BREADCRUMB_WAKE = 4,
  BREADCRUMB_REPORTING = 5,
  BREADCRUMB_CONNECT_REQUESTED = 6,
  BREADCRUMB_CLOUD_CONNECTED = 7,
  BREADCRUMB_APP_WATCHDOG_RESET = 8,
  BREADCRUMB_CONNECTIVITY_FAILSAFE = 9,
  BREADCRUMB_CONNECTIVITY_FAILSAFE_HARD = 10,
  BREADCRUMB_REPORT_QUEUE_START = 11,
  BREADCRUMB_REPORT_QUEUE_DONE = 12,
  BREADCRUMB_REPORT_LEDGER_START = 13,
  BREADCRUMB_REPORT_LEDGER_DONE = 14,
  BREADCRUMB_CLOUD_LOOP_ENTER = 15,
  BREADCRUMB_CLOUD_LOOP_EXIT = 16,
  BREADCRUMB_PUBLISH_QUEUE_ENTER = 17,
  BREADCRUMB_PUBLISH_QUEUE_EXIT = 18,
  BREADCRUMB_REPORT_POST_LEDGER = 19,
  BREADCRUMB_REPORT_EXIT = 20,
  BREADCRUMB_IDLE_ENTRY = 21,
  BREADCRUMB_SLEEP_GATE_START = 22,
  BREADCRUMB_SLEEP_GATE_DONE = 23,
  BREADCRUMB_SLEEP_CONFIG_START = 24,
  BREADCRUMB_SLEEP_SYSTEM_CALL = 25,
  // Finer-grained sleep-entry breadcrumbs added to pinpoint a stall between
  // BREADCRUMB_SLEEP_SYSTEM_CALL and the actual System.sleep() return/reset,
  // per the watchdog-instrumentation work order. If a future occurrence
  // stalls, the last-recorded value among 25-28 identifies the specific
  // stuck call (serial drain, diagnostics flush, or System.sleep() itself).
  BREADCRUMB_SLEEP_SERIAL_DRAIN_DONE = 26,
  BREADCRUMB_SLEEP_DIAG_FLUSH_DONE = 27,
  BREADCRUMB_SLEEP_CALL_ENTER = 28,
};

const char *appBreadcrumbName(uint8_t code) {
  switch (code) {
  case BREADCRUMB_SETUP_START:
    return "SETUP";
  case BREADCRUMB_SETUP_COMPLETE:
    return "READY";
  case BREADCRUMB_SLEEP_ENTRY:
    return "SLEEP";
  case BREADCRUMB_WAKE:
    return "WAKE";
  case BREADCRUMB_REPORTING:
    return "REPORT";
  case BREADCRUMB_CONNECT_REQUESTED:
    return "CONN";
  case BREADCRUMB_CLOUD_CONNECTED:
    return "CLOUD";
  case BREADCRUMB_APP_WATCHDOG_RESET:
    return "WDT";
  case BREADCRUMB_CONNECTIVITY_FAILSAFE:
    return "CONN_FAILSAFE";
  case BREADCRUMB_CONNECTIVITY_FAILSAFE_HARD:
    return "CONN_FAILSAFE_HARD";
  case BREADCRUMB_REPORT_QUEUE_START:
    return "REPORT_QUEUE_START";
  case BREADCRUMB_REPORT_QUEUE_DONE:
    return "REPORT_QUEUE_DONE";
  case BREADCRUMB_REPORT_LEDGER_START:
    return "REPORT_LEDGER_START";
  case BREADCRUMB_REPORT_LEDGER_DONE:
    return "REPORT_LEDGER_DONE";
  case BREADCRUMB_CLOUD_LOOP_ENTER:
    return "CLOUD_LOOP_ENTER";
  case BREADCRUMB_CLOUD_LOOP_EXIT:
    return "CLOUD_LOOP_EXIT";
  case BREADCRUMB_PUBLISH_QUEUE_ENTER:
    return "PUBLISH_QUEUE_ENTER";
  case BREADCRUMB_PUBLISH_QUEUE_EXIT:
    return "PUBLISH_QUEUE_EXIT";
  case BREADCRUMB_REPORT_POST_LEDGER:
    return "REPORT_POST_LEDGER";
  case BREADCRUMB_REPORT_EXIT:
    return "REPORT_EXIT";
  case BREADCRUMB_IDLE_ENTRY:
    return "IDLE_ENTRY";
  case BREADCRUMB_SLEEP_GATE_START:
    return "SLEEP_GATE_START";
  case BREADCRUMB_SLEEP_GATE_DONE:
    return "SLEEP_GATE_DONE";
  case BREADCRUMB_SLEEP_CONFIG_START:
    return "SLEEP_CONFIG_START";
  case BREADCRUMB_SLEEP_SYSTEM_CALL:
    return "SLEEP_SYSTEM_CALL";
  case BREADCRUMB_SLEEP_SERIAL_DRAIN_DONE:
    return "SLEEP_SERIAL_DRAIN_DONE";
  case BREADCRUMB_SLEEP_DIAG_FLUSH_DONE:
    return "SLEEP_DIAG_FLUSH_DONE";
  case BREADCRUMB_SLEEP_CALL_ENTER:
    return "SLEEP_CALL_ENTER";
  default:
    return "NONE";
  }
}

// Not Boron-specific: the AB1805 RTC/watchdog chip is used on every supported
// platform (ab1805.setup()/setWDT() run unconditionally below), so this name
// lookup must be available generally, not just under the Boron-only hibernate
// wake-reason path.
const char *ab1805WakeReasonName(AB1805::WakeReason reason) {
  switch (reason) {
  case AB1805::WakeReason::WATCHDOG:
    return "WATCHDOG";
  case AB1805::WakeReason::DEEP_POWER_DOWN:
    return "DEEP_POWER_DOWN";
  case AB1805::WakeReason::COUNTDOWN_TIMER:
    return "COUNTDOWN_TIMER";
  case AB1805::WakeReason::ALARM:
    return "ALARM";
  case AB1805::WakeReason::UNKNOWN:
  default:
    return "UNKNOWN";
  }
}

const char *stateShortName(State value) {
  switch (value) {
  case INITIALIZATION_STATE:
    return "Init";
  case ERROR_STATE:
    return "Error";
  case IDLE_STATE:
    return "Idle";
  case SLEEPING_STATE:
    return "Sleep";
  case CONNECTING_STATE:
    return "Connect";
  case REPORTING_STATE:
    return "Report";
  case FIRMWARE_UPDATE_STATE:
    return "FW";
  default:
    return "?";
  }
}

const char *batteryTierShortName(BatteryTier tier) {
  switch (tier) {
  case TIER_HEALTHY:
    return "H";
  case TIER_CONSERVING:
    return "C";
  case TIER_CRITICAL:
    return "CR";
  case TIER_SURVIVAL:
    return "S";
  default:
    return "?";
  }
}

BatteryTier currentBatteryTierForFailsafe() {
  const uint8_t tierValue = sysStatus.get_currentBatteryTier();
  if (tierValue <= TIER_SURVIVAL) {
    return static_cast<BatteryTier>(tierValue);
  }
  return Cloud::calculateBatteryTier(PowerManager::instance().soc());
}

bool connectivityFailsafeHasExternalPower() {
  const PowerPlatform::PowerSourceSnapshot snapshot = PowerPlatform::readPowerSource();
  if (snapshot.status == PowerAvailability::NotAvailable) {
    return false;
  }

  switch (snapshot.source) {
  case 1:
  case 2:
  case 3:
  case 4:
    return true;
  default:
    return false;
  }
}

uint32_t connectivityFailsafeJitterSec(uint8_t stage) {
#if CONNECTIVITY_FAILSAFE_TEST_MODE
  return ConnectivityFailsafeTest::jitterSec(stage);
#else
  if (ConnectivityPolicy::CONNECTIVITY_FAILSAFE_JITTER_MAX_SEC <= 0) {
    return 0;
  }

  String deviceId = System.deviceID();
  uint32_t hash = 5381u;
  for (size_t index = 0; index < deviceId.length(); ++index) {
    hash = ((hash << 5) + hash) ^ (uint8_t)deviceId.charAt(index);
  }
  hash ^= (uint32_t)stage * 2654435761u;
  return hash % (uint32_t)ConnectivityPolicy::CONNECTIVITY_FAILSAFE_JITTER_MAX_SEC;
#endif
}

void persistConnectivityFailsafeState(uint8_t stage, time_t actionTime, bool incrementCount) {
  sysStatus.set_connectivityRecoveryStage(stage);
  sysStatus.set_lastConnectivityRecoveryAction(actionTime);
  if (incrementCount) {
    uint8_t count = sysStatus.get_connectivityRecoveryCount();
    if (count < 0xFF) {
      sysStatus.set_connectivityRecoveryCount(count + 1);
    }
  }
  sysStatus.flush(true);
}

const char *failsafeDeferReasonNameLocal(FailsafeDeferReason reason) {
  switch (reason) {
  case FAILSAFE_DEFER_INVALID_TIME:
    return "invalid-time";
  case FAILSAFE_DEFER_NO_LAST_CONNECTION:
    return "no-last-connection";
  case FAILSAFE_DEFER_DISCONNECTED_MODE:
    return "disconnected-mode";
  case FAILSAFE_DEFER_UPDATE_PENDING:
    return "update-pending";
  case FAILSAFE_DEFER_LOW_BATTERY_HARD_STAGE_SUPPRESSED:
    return "low-battery-hard-stage-suppressed";
  case FAILSAFE_DEFER_CLOSED_HOURS_LONG_SLEEP:
    return "closed-hours-long-sleep";
  default:
    return "none";
  }
}

bool claimFailsafeDeferLogLocal(FailsafeDeferReason reason) {
  if (reason == FAILSAFE_DEFER_NONE) {
    return false;
  }

  const uint8_t shift = (uint8_t)reason - 1;
  const uint32_t bit = (shift < 32) ? (1UL << shift) : 0UL;
  if (bit == 0UL) {
    return false;
  }
  if (session.failsafeDeferLogMask & bit) {
    return false;
  }
  session.failsafeDeferLogMask |= bit;
  return true;
}

// Drain USB CDC serial buffer before sleep to prevent split log lines (bench testing only)
void drainSerialBeforeSleep() {
  if (Serial.isConnected()) {
    Serial.flush();
    delay(75);
  }
}

} // namespace

const char *failsafeDeferReasonName(FailsafeDeferReason reason) {
  return failsafeDeferReasonNameLocal(reason);
}

bool claimFailsafeDeferLog(FailsafeDeferReason reason) {
  return claimFailsafeDeferLogLocal(reason);
}

// ===== Retained boot and breadcrumb state =====

constexpr uint32_t kLoopForensicsMagic = 0x57444631UL;
// Bumped 1 -> 2 for the addition of sleepPrepSpanStartMillis (see
// WO-2026-08-11-001 Second Corrective Pass). This struct is `retained` RAM,
// so a stale smaller layout from before this change must not be misread as
// the new layout - the version check below forces a reinit on mismatch.
constexpr uint8_t kLoopForensicsVersion = 2;
constexpr unsigned long kLoopStageWarnThresholdMs = 2000UL;
constexpr unsigned long kLoopStageErrorThresholdMs = 10000UL;
constexpr unsigned long kLoopForensicsSnapshotIntervalMs = 1000UL;

// RetainedLoopForensics is declared in state/State_Common.h so it (and
// retainedLoopForensics below) can be shared with State_Sleep.cpp's
// handleSleepingState() (span start) and this file's transitionTo() (span
// close/log choke point - see WO-2026-08-11-001 Fourth Corrective Pass).

// Boot-storm guard retained state protects against repeated resets that occur
// before setup completes and therefore bypass ERROR_STATE protections.
retained uint8_t bootStormCount = 0;
retained bool bootInProgress = false;
retained time_t bootStormWindowStart = 0;
retained uint8_t bootStormLastResetReason = 0;
retained uint32_t bootStormLastResetReasonData = 0;
retained uint8_t bootStormTripCount = 0;
retained bool bootStormAlertPending = false;
retained uint8_t appBreadcrumb = BREADCRUMB_NONE;
retained uint32_t appBreadcrumbMs = 0;
retained RetainedLoopForensics retainedLoopForensics = {};
retained time_t retainedHibernateRtcBefore = 0;
retained time_t retainedHibernateWakeTime = 0;
retained uint32_t retainedHibernateRequestedSleep = 0;
retained uint32_t retainedHibernateCount = 0;
retained bool retainedHibernatePending = false;
uint8_t startupPreviousBreadcrumb = BREADCRUMB_NONE;
uint32_t startupPreviousBreadcrumbMs = 0;
uint8_t startupPreviousLoopStage = LOOP_STAGE_NONE;
uint32_t startupPreviousLoopStageElapsedMs = 0;
uint16_t startupPreviousQueueDepth = 0;
uint32_t startupPreviousMillisSinceLastCloudConnect = 0;
uint8_t startupPreviousState = INITIALIZATION_STATE;
bool startupHibernateStatusReady = false;
const char *startupHibernateWakeReason = "UNKNOWN";
uint32_t startupHibernateActualSleepSec = 0;
int32_t startupHibernateSleepErrorSec = 0;
unsigned long lastLoopForensicsSnapshotMs = 0;

// AB1805 wake-reason classification captured on a RESET_REASON_PIN_RESET
// boot (see setup()). Only meaningful when startupPinResetAb1805Checked is
// true; otherwise the OS reset reason was not PIN_RESET and these fields
// carry their initialized defaults and are omitted from the status payload.
bool startupPinResetAb1805Checked = false;
const char *startupAb1805WakeReasonName = "N/A";
bool startupAb1805ConfirmedWatchdog = false;

template <typename T>
bool updateRetainedValue(T &slot, const T &value) {
  if (slot == value) {
    return false;
  }
  slot = value;
  return true;
}

const char *loopStageName(LoopStage stage) {
  switch (stage) {
  case LOOP_STAGE_STATE_HANDLER:
    return "STATE_HANDLER";
  case LOOP_STAGE_CLOUD_LOOP:
    return "CLOUD_LOOP";
  case LOOP_STAGE_PUBLISH_QUEUE:
    return "PUBLISH_QUEUE";
  case LOOP_STAGE_DIAGNOSTICS:
    return "DIAGNOSTICS";
  case LOOP_STAGE_IDLE_PROCESSING:
    return "IDLE_PROCESSING";
  case LOOP_STAGE_SLEEP_PREP:
    return "SLEEP_PREP";
  case LOOP_STAGE_CONNECTIVITY:
    return "CONNECTIVITY";
  case LOOP_STAGE_NONE:
  default:
    return "NONE";
  }
}

const char *loopStageForensicName(LoopStage stage) {
  switch (stage) {
  case LOOP_STAGE_PUBLISH_QUEUE:
    return "publish";
  case LOOP_STAGE_CLOUD_LOOP:
    return "cloud";
  case LOOP_STAGE_STATE_HANDLER:
    return "state";
  case LOOP_STAGE_DIAGNOSTICS:
    return "diag";
  case LOOP_STAGE_CONNECTIVITY:
    return "connectivity";
  case LOOP_STAGE_IDLE_PROCESSING:
    return "idle";
  case LOOP_STAGE_SLEEP_PREP:
    return "sleep";
  case LOOP_STAGE_NONE:
  default:
    return "none";
  }
}

void resetRetainedLoopForensics() {
  memset(&retainedLoopForensics, 0, sizeof(retainedLoopForensics));
  retainedLoopForensics.magic = kLoopForensicsMagic;
  retainedLoopForensics.version = kLoopForensicsVersion;
  retainedLoopForensics.lastBreadcrumb = appBreadcrumb;
  retainedLoopForensics.currentState = (uint8_t)state;
}

void ensureRetainedLoopForensicsInitialized() {
  if (retainedLoopForensics.magic != kLoopForensicsMagic ||
      retainedLoopForensics.version != kLoopForensicsVersion) {
    resetRetainedLoopForensics();
  }
}

void refreshRetainedLoopForensics(bool sampleQueueDepth = true, bool force = false) {
  ensureRetainedLoopForensicsInitialized();
  const unsigned long nowMs = millis();
  const bool sampleDynamic = force ||
      lastLoopForensicsSnapshotMs == 0 ||
      (nowMs - lastLoopForensicsSnapshotMs) >= kLoopForensicsSnapshotIntervalMs;

  updateRetainedValue(retainedLoopForensics.lastBreadcrumb, appBreadcrumb);
  updateRetainedValue(retainedLoopForensics.currentState, (uint8_t)state);

  if (!sampleDynamic) {
    return;
  }

  const uint32_t elapsedMs = (retainedLoopForensics.stageStartMillis != 0)
      ? (uint32_t)(nowMs - retainedLoopForensics.stageStartMillis)
      : 0UL;
  updateRetainedValue(retainedLoopForensics.lastLoopStageElapsed, elapsedMs);

  if (sampleQueueDepth) {
    updateRetainedValue(retainedLoopForensics.publishQueueDepth,
                        (uint16_t)PublishQueuePosix::instance().getNumEvents());
  }

  if (connectedStartMs != 0) {
    updateRetainedValue(retainedLoopForensics.millisSinceLastCloudConnect,
                        (nowMs >= connectedStartMs) ? (nowMs - connectedStartMs) : 0UL);
  } else {
    updateRetainedValue(retainedLoopForensics.millisSinceLastCloudConnect, 0UL);
  }

  lastLoopForensicsSnapshotMs = nowMs;
}

// Shared five-field "LoopStage:" line, used both by noteLoopStageDuration()'s
// WARN/ERROR threshold escalation and by transitionTo()'s once-per-real-span
// SLEEP_PREP exit log (see WO-2026-08-11-001 Fourth Corrective Pass).
// Keeping the format string in one place avoids the call sites drifting
// apart.
void logLoopStageLine(LogLevel level, LoopStage stage, unsigned long elapsedMs,
                       uint16_t queueDepth, uint32_t millisSinceLastCloudConnect) {
  switch (level) {
  case LOG_LEVEL_ERROR:
    Log.error("LoopStage: stage=%s elapsed=%lu state=%u q=%u connMs=%lu",
              loopStageName(stage),
              elapsedMs,
              (unsigned)state,
              (unsigned)queueDepth,
              (unsigned long)millisSinceLastCloudConnect);
    break;
  case LOG_LEVEL_WARN:
    Log.warn("LoopStage: stage=%s elapsed=%lu state=%u q=%u connMs=%lu",
             loopStageName(stage),
             elapsedMs,
             (unsigned)state,
             (unsigned)queueDepth,
             (unsigned long)millisSinceLastCloudConnect);
    break;
  default:
    Log.info("LoopStage: stage=%s elapsed=%lu state=%u q=%u connMs=%lu",
              loopStageName(stage),
              elapsedMs,
              (unsigned)state,
              (unsigned)queueDepth,
              (unsigned long)millisSinceLastCloudConnect);
    break;
  }
}

void noteLoopStageDuration(bool sampleQueueDepth = true) {
  const unsigned long nowMs = millis();
  const unsigned long elapsedMs = (retainedLoopForensics.stageStartMillis != 0)
      ? (nowMs - retainedLoopForensics.stageStartMillis)
      : 0UL;
  const LoopStage stage = static_cast<LoopStage>(retainedLoopForensics.lastLoopStage);
  const uint16_t queueDepth = sampleQueueDepth
      ? (uint16_t)PublishQueuePosix::instance().getNumEvents()
      : retainedLoopForensics.publishQueueDepth;
  const uint32_t millisSinceLastCloudConnect = (connectedStartMs != 0 && nowMs >= connectedStartMs)
      ? (uint32_t)(nowMs - connectedStartMs)
      : 0UL;

  refreshRetainedLoopForensics(sampleQueueDepth, false);

  // LOOP_STAGE_SLEEP_PREP is excluded from WARN/ERROR threshold escalation
  // here entirely (no branch for it at all): its elapsed time structurally
  // spans pre-sleep gate-wait, the entire intentional (often multi-minute)
  // blocking System.sleep() call, and the post-wake tail, so the thresholds
  // designed for "stuck/blocked" stages do not apply. Its INFO-level logging
  // is handled once per real span, on exit, by transitionTo()'s choke-point
  // guard instead (see WO-2026-08-11-001 Fourth Corrective Pass) - this
  // function is called on essentially every loop() iteration (including
  // every re-entrant SLEEP_PREP polling pass), so logging it here (or in
  // setLoopStage(), which is retagged by unrelated stage churn every
  // iteration too, or in a State_Sleep.cpp-local wrapper, which real exits
  // from outside handleSleepingState() can bypass) caused regressions on
  // real hardware across this WO's earlier corrective passes.
  if (stage == LOOP_STAGE_SLEEP_PREP) {
    return;
  }

  if (elapsedMs >= kLoopStageErrorThresholdMs) {
    logLoopStageLine(LOG_LEVEL_ERROR, stage, elapsedMs, queueDepth, millisSinceLastCloudConnect);
  } else if (elapsedMs >= kLoopStageWarnThresholdMs) {
    logLoopStageLine(LOG_LEVEL_WARN, stage, elapsedMs, queueDepth, millisSinceLastCloudConnect);
  }
}

void setAppBreadcrumb(uint8_t code) {
  appBreadcrumb = code;
  appBreadcrumbMs = millis();
  refreshRetainedLoopForensics(false, true);
}

void setLoopStage(LoopStage stage) {
  ensureRetainedLoopForensicsInitialized();
  if (retainedLoopForensics.lastLoopStage == (uint8_t)stage) {
    return;
  }

  retainedLoopForensics.lastLoopStage = (uint8_t)stage;
  retainedLoopForensics.stageStartMillis = millis();
  refreshRetainedLoopForensics(true, true);
}

// Short-term webhook monitoring starts only after a successful cloud
// connection. The expectation is armed when a webhook publish is queued.

// ===== Application lifecycle =====

const unsigned long resetWait = 30000;      // Error state dwell before reset

/**
 * @brief Initializes all system components and prepares device for operational state.
 *
 * Orchestrates boot-time initialization across power management, time synchronization,
 * persistent storage, sensor hardware, and cloud connectivity domains. Captures forensic
 * state from previous session and applies boot-storm countermeasures if needed.
 *
 * @note Transitions to IDLE_STATE unless boot conditions (button press, OTA update,
 *       invalid config/time) force CONNECTING_STATE for immediate cloud access.
 *
 * @see docs/architecture/startup-sequence.md for detailed initialization phases
 */
void setup() {
  // Always-on "brief wait if already connected" serial settle, independent of
  // the bench-only ALLOW_BLOCKING_SERIAL_WAITS flag (unchanged, handled
  // further below). The global SerialLogHandler's constructor already calls
  // Serial.begin() before setup() runs, so Serial.isConnected() is valid
  // here without an additional Serial.begin() call. On deployed devices
  // nothing is ever attached to serial, so this check is near-zero cost
  // (Serial.isConnected() returns false immediately and no delay occurs).
  if (Serial.isConnected()) {
    delay(ConnectivityPolicy::DEBUG_SERIAL_POST_CONNECT_DELAY_MS);
  }

  ensureRetainedLoopForensicsInitialized();

  const int reason = System.resetReason();
  const uint32_t reasonData = System.resetReasonData();
  const uint8_t previousBreadcrumb = appBreadcrumb;
  const uint32_t previousBreadcrumbMs = appBreadcrumbMs;
  const uint8_t previousLoopStage = retainedLoopForensics.lastLoopStage;
  const uint32_t previousLoopStageElapsed = retainedLoopForensics.lastLoopStageElapsed;
  const uint16_t previousQueueDepth = retainedLoopForensics.publishQueueDepth;
  const uint32_t previousMillisSinceLastCloudConnect = retainedLoopForensics.millisSinceLastCloudConnect;
  const uint8_t previousState = retainedLoopForensics.currentState;
  const bool watchdogResetDetected = (reason == RESET_REASON_WATCHDOG);
  startupPreviousBreadcrumb = previousBreadcrumb;
  startupPreviousBreadcrumbMs = previousBreadcrumbMs;
  startupPreviousLoopStage = previousLoopStage;
  startupPreviousLoopStageElapsedMs = previousLoopStageElapsed;
  startupPreviousQueueDepth = previousQueueDepth;
  startupPreviousMillisSinceLastCloudConnect = previousMillisSinceLastCloudConnect;
  startupPreviousState = previousState;
  // Unconditional, every boot (WO-2026-08-11-001 Third Corrective Pass): a
  // SLEEP_PREP span cannot survive any real reset (HIBERNATE success,
  // explicit System.reset(), or any other reset reason) - the call stack
  // that would have closed it via closeSleepPrepSpan() is gone, so any
  // retained nonzero value here is always stale and must be discarded
  // before the next SLEEPING_STATE dwell.
  resetSleepPrepSpanOnBoot(retainedLoopForensics.sleepPrepSpanStartMillis);
  setAppBreadcrumb(BREADCRUMB_SETUP_START);
  bootStormLastResetReason = (uint8_t)reason;
  bootStormLastResetReasonData = reasonData;

  const bool previousBootFailedEarly = bootInProgress;
  // Mark this boot as in-progress immediately so any early reset on this pass
  // is detectable on the next boot.
  bootInProgress = true;

  bool qualifiesForStormCount = false;
  switch (reason) {
  case RESET_REASON_NONE:
#ifdef RESET_REASON_PANIC
  case RESET_REASON_PANIC:
#endif
  case RESET_REASON_WATCHDOG:
#ifdef RESET_REASON_UNKNOWN
  case RESET_REASON_UNKNOWN:
#endif
#ifdef RESET_REASON_USER_APPLICATION
  case RESET_REASON_USER_APPLICATION:
#endif
    qualifiesForStormCount = true;
    break;
  default:
    break;
  }

  if (previousBootFailedEarly && qualifiesForStormCount) {
    Log.warn("Previous boot reset early before setup completed (reason=%d data=%lu)",
             reason,
             (unsigned long)reasonData);
  }

  if (previousBootFailedEarly && qualifiesForStormCount) {
    if (bootStormCount < 255) {
      bootStormCount++;
    }
  }

  if (Time.isValid()) {
    const time_t now = Time.now();
    if (bootStormWindowStart == 0) {
      bootStormWindowStart = now;
    } else if ((now - bootStormWindowStart) > 600) {
      bootStormCount = (previousBootFailedEarly && qualifiesForStormCount) ? 1 : 0;
      bootStormWindowStart = now;
    }
  }

  if (bootStormCount >= 6 && previousBootFailedEarly && qualifiesForStormCount) {
    bootStormTripCount++;
    bootStormAlertPending = true;
    Log.error("BOOT STORM: %u early resets detected (reason=%d)", bootStormCount, reason);
    Connectivity::requestFullDisconnectAndRadioOff();
    Particle.process();
    SystemSleepConfiguration bootStormSleep;
    bootStormSleep.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(600000UL);
    drainSerialBeforeSleep();
    System.sleep(bootStormSleep);
    Log.warn("BOOT STORM holdoff sleep returned unexpectedly - continuing boot");
  }

  // Wait for serial connection only in explicit DEV builds
#if ALLOW_BLOCKING_SERIAL_WAITS
  waitFor(Serial.isConnected, 10000);
  delay(1000);
#endif

  // Observability: start a new wake cycle on cold boot.
  Observability::cycleStats().resetOnWake(millis());
  
  System.on(out_of_memory,
            outOfMemoryHandler); // Enabling an out of memory handler is a good
                                 // safety tip. If we run out of memory a
                                 // System.reset() is done.

  // Awake watchdog: reset if the firmware stops making forward progress while
  // running normally. Use the Device OS hardware watchdog so intended sleep
  // does not look like an application hang on P2/Photon 2.
#if Wiring_Watchdog
  particle::WatchdogConfiguration awakeWatchdogConfig;
#if PLATFORM_ID != PLATFORM_P2 && !(defined(PLATFORM_PHOTON2) && PLATFORM_ID == PLATFORM_PHOTON2)
  const uint32_t awakeWatchdogCaps =
      awakeWatchdogConfig.capabilities().value() & ~HAL_WATCHDOG_CAPS_SLEEP_RUNNING;
  awakeWatchdogConfig.capabilities(particle::WatchdogCaps::fromUnderlying(awakeWatchdogCaps));
#endif
  awakeWatchdogConfig.timeout(AWAKE_WATCHDOG_TIMEOUT_MS);

  const int awakeWatchdogInitResult = Watchdog.init(awakeWatchdogConfig);
  int awakeWatchdogNotifyResult = SYSTEM_ERROR_NONE;
  int awakeWatchdogStartResult = awakeWatchdogInitResult;
  awakeWatchdogInitialized = (awakeWatchdogInitResult == SYSTEM_ERROR_NONE);

  if (awakeWatchdogInitialized) {
    awakeWatchdogNotifyResult = Watchdog.onExpired(awakeWatchdogExpiredHandler);
    awakeWatchdogStartResult = Watchdog.start();
    awakeWatchdogStarted = Watchdog.started();
  }

#if ENABLE_SLEEP_TRACE
  Log.info("AppWDT: armed=%d impl=hw timeout=%lu strat=%s init=%d start=%d notify=%d started=%d",
           awakeWatchdogStarted ? 1 : 0,
           AWAKE_WATCHDOG_TIMEOUT_MS,
           awakeWatchdogSleepStrategyName(awakeWatchdogSleepStrategy),
           awakeWatchdogInitResult,
           awakeWatchdogStartResult,
           awakeWatchdogNotifyResult,
           Watchdog.started() ? 1 : 0);
#else
  if (awakeWatchdogInitResult != SYSTEM_ERROR_NONE ||
      awakeWatchdogNotifyResult != SYSTEM_ERROR_NONE ||
      awakeWatchdogStartResult != SYSTEM_ERROR_NONE ||
      !Watchdog.started()) {
    Log.warn("AppWDT: abnormal strat=%s init=%d start=%d notify=%d started=%d",
             awakeWatchdogSleepStrategyName(awakeWatchdogSleepStrategy),
             awakeWatchdogInitResult,
             awakeWatchdogStartResult,
             awakeWatchdogNotifyResult,
             Watchdog.started() ? 1 : 0);
  }
#endif
#else
  // Fallback for platforms without the Device OS hardware watchdog API.
  static ApplicationWatchdog appWatchdog(60000, appWatchdogHandler, 1536);
#endif

  // Subscribe to the Ubidots integration response event so we can track
  // successful webhook deliveries and update lastHookResponse.
  // This subscription is non-blocking; we'll receive responses when connected.
  {
    char responseTopic[125];
    String deviceID = System.deviceID();
    deviceID.toCharArray(responseTopic, sizeof(responseTopic));
    Particle.subscribe(responseTopic, UbidotsHandler);

    // Also subscribe to the default Particle webhook response prefix so this
    // works with whatever webhook name is configured in the ledger.
    // If the integration uses the default response topic, responses will be
    // published to: hook-response/<eventName>.
    Particle.subscribe("hook-response/", UbidotsHandler);
  }

  // Configure startup with the radio left off. CONNECTING_STATE owns
  // Particle.connect() and all subsequent network bring-up.
#if !Wiring_WiFi && !Wiring_Cellular
  // Platforms without explicit Wiring_* radio macros still connect through CONNECTING_STATE.
#endif

  initializePinModes(); // Initialize the pin modes

  // Recovery path: if the user holds the service button during reset/wake,
  // force a cloud connection regardless of opening-hours logic.
  // This allows ledger changes (e.g., opening hours) to be pulled down to
  // recover a device that would otherwise immediately sleep.
  if (digitalRead(BUTTON_PIN) == LOW) { // Active-low user button
    Log.info("Boot override: user button held - forcing CONNECTING_STATE");
    transitionTo(CONNECTING_STATE, "boot button override");
  }

  sysStatus.setup();    // Initialize persistent storage
  sensorConfig.setup(); // Initialize the sensor configuration
  current.setup();      // Initialize the current status data
  PowerManager::instance().setup();

  if (sysStatus.get_hasValidLedgerConfig()) {
    Config::markStorageConfigurationLoaded();
  } else {
    Config::markFactoryDefaultsActive();
  }

  // Watchdog-reset persistence/forensics-publish (Device-OS-detected and
  // AB1805-confirmed PIN_RESET cases) is handled after AB1805 wake-reason
  // classification below, once ab1805.setup()/getWakeReason() have run -
  // see the "WATCHDOG RESET CLASSIFICATION" block further down in setup().

  // If a boot storm holdoff was triggered on this or the prior boot, surface
  // it as an alert now that persistent current status storage is initialized.
  if (bootStormAlertPending) {
    // Force explicit boot-storm alert visibility in startup/report payloads.
    current.set_alertCode(17);
    current.set_lastAlertTime(Time.now());
    bootStormAlertPending = false;
    Log.warn("Boot storm holdoff detected - raising alert 17");
  }

  // Configure serial logging based on serial flag
  // Note: Particle firmware on this platform doesn't support Log.level()
  if (sysStatus.get_serialConnected() || (ALLOW_BLOCKING_SERIAL_WAITS != 0)) {
    Serial.begin(9600);

    // Honor the persisted serialConnected flag with a bounded wait so
    // USB serial can attach before a low-power device drops back to sleep.
    // In developer builds, the build profile can force this wait even when
    // cloud config has not enabled serial logging yet.
    const unsigned long serialWaitStart = millis();
    while (!Serial.isConnected() && (millis() - serialWaitStart) < ConnectivityPolicy::DEBUG_SERIAL_WAIT_TIMEOUT_MS) {
      serviceAwakeWatchdog();
      Particle.process();
      delay(ConnectivityPolicy::DEBUG_SERIAL_WAIT_POLL_DELAY_MS);
    }

    if (Serial.isConnected()) {
      delay(ConnectivityPolicy::DEBUG_SERIAL_POST_CONNECT_DELAY_MS);
    }
  }

  if (sysStatus.get_testConnectionDurationOverride() == 0) {
    sysStatus.set_testConnectionDurationOverride(0xFFFF);  // Disabled (max uint16_t)
  }

  // Testing: clear sticky sleep-failure alert to avoid reset/deep-power loops.
  if (current.get_alertCode() == 16) {
    Log.info("Clearing alert 16 on boot");
    current.set_alertCode(0);
    current.set_lastAlertTime(0);
  }

  // Clear sticky OOM alert after a reset-driven recovery attempt so field
  // reports can distinguish a one-off OOM from repeated reset/OOM cycles.
  bool clearOomAlertOnBoot = false;
  switch (reason) {
  case RESET_REASON_PIN_RESET:
  case RESET_REASON_USER:
  case RESET_REASON_WATCHDOG:
    clearOomAlertOnBoot = true;
    break;
#ifdef RESET_REASON_USER_APPLICATION
  case RESET_REASON_USER_APPLICATION:
    clearOomAlertOnBoot = true;
    break;
#endif
  default:
    break;
  }
  if (current.get_alertCode() == 14 && clearOomAlertOnBoot) {
    Log.info("Clearing alert 14 on boot after reset-driven recovery");
    current.set_alertCode(0);
    current.set_lastAlertTime(0);
  }

  // Track how often the device has been resetting so the error supervisor
  // can apply backoffs and avoid permanent reset loops. Only count resets
  // that are likely to be recoverable by firmware (pin/user/watchdog).
  switch (reason) {
  case RESET_REASON_PIN_RESET:
  case RESET_REASON_USER:
  case RESET_REASON_WATCHDOG:
#ifdef RESET_REASON_USER_APPLICATION
  case RESET_REASON_USER_APPLICATION:
#endif
    sysStatus.set_resetCount(sysStatus.get_resetCount() + 1);
    break;
  case RESET_REASON_UPDATE:
    // After OTA firmware update, force connection to reload
    // configuration from ledger. This ensures device-settings
    // (operatingMode, etc.) override any stale FRAM values.
    Log.info("OTA update detected - forcing connection to reload config");
    transitionTo(CONNECTING_STATE, "OTA update");
    break;
  case RESET_REASON_POWER_MANAGEMENT:
    // Waking from sleep. Alert 40 suppression for overnight hibernate
    // is handled later in setup() after timezone configuration is complete.
    break;
  default:
    break;
  }

  // Ensure sensor-board LED power default matches configured sensor type
  // TODO: Consider moving this sensor-specific logic to device_pinout.cpp or SensorManager
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

  // ===== TIME, RTC, AND WATCHDOG CONFIGURATION =====
  // Initialize AB1805 RTC and hardware watchdog, then restore system time if needed
  const bool timeValidBeforeRtc = Time.isValid();
  ab1805.withFOUT(WKP).setup();                // Initialize AB1805 RTC - WKP is D10 on Photon2

  // ===== AB1805 WATCHDOG WAKE-REASON CLASSIFICATION =====
  // Gated on RESET_REASON_PIN_RESET. Device OS reports an external MCU reset
  // (e.g. the carrier board's AB1805 watchdog firing via the reset pin) as
  // PIN_RESET, not WATCHDOG - so the only way to confirm the AB1805 was the
  // cause is to ask it directly. Placed right after ab1805.setup() and
  // before setWDT() so we read the chip's wake-reason register before
  // re-arming the watchdog.
  //
  // IMPORTANT: do NOT call ab1805.updateWakeReason() again here.
  // AB1805::setup() (above) already called detectChip() + updateWakeReason()
  // internally on a successful chip detection, and that single read is the
  // only one we may rely on: updateWakeReason() destructively clears the
  // status bit it classifies (e.g. clearRegisterBit(REG_STATUS,
  // REG_STATUS_WDT) once it reports WATCHDOG). If the status register had
  // multiple bits set simultaneously (WDT+TIMER or WDT+ALARM), a second call
  // here would re-read the now-stale register, no longer see the WDT bit
  // (already cleared by setup()'s call), and silently overwrite the correct
  // WATCHDOG classification with whichever other bit remained. Reusing
  // ab1805.getWakeReason() here relies on - and is correct because of - the
  // library's own WDT-first priority ordering inside updateWakeReason()'s
  // if/else-if chain (WDT checked before TIMER/ALARM in that single read).
  //
  // There is no separate success/fail signal available without repeating
  // that destructive read: WakeReason defaults to UNKNOWN and only changes
  // if AB1805::setup()'s internal detectChip()+updateWakeReason() sequence
  // ran and found a recognized status bit. So WakeReason::UNKNOWN already
  // and correctly covers both "chip detection failed" and "no wake-reason
  // bit was set" - both must stay inconclusive, never "not the AB1805".
  bool ab1805ConfirmedWatchdog = false;
  if (reason == RESET_REASON_PIN_RESET) {
    startupPinResetAb1805Checked = true;
    const AB1805::WakeReason pinResetWakeReason = ab1805.getWakeReason();
    startupAb1805WakeReasonName = ab1805WakeReasonName(pinResetWakeReason);

    if (pinResetWakeReason == AB1805::WakeReason::WATCHDOG) {
      ab1805ConfirmedWatchdog = true;
      Log.warn("PIN_RESET confirmed as AB1805 watchdog reset");
    } else if (pinResetWakeReason == AB1805::WakeReason::UNKNOWN) {
      // Explicitly inconclusive - do NOT treat UNKNOWN as "not the AB1805".
      Log.info("PIN_RESET with AB1805 wake reason UNKNOWN - inconclusive, not ruling out AB1805 watchdog");
    } else {
      Log.info("PIN_RESET with AB1805 wake reason=%s - not an AB1805 watchdog reset",
               startupAb1805WakeReasonName);
    }
    startupAb1805ConfirmedWatchdog = ab1805ConfirmedWatchdog;
  }

  ab1805.setWDT(AB1805::WATCHDOG_MAX_SECONDS); // Enable watchdog

  // ===== WATCHDOG RESET PERSISTENCE / FORENSICS =====
  // Moved here (from immediately after sysStatus.setup(), before AB1805 init)
  // so it can act on the AB1805 classification above. The publish queue was
  // already initialized above, so this is a reordering, not a new
  // dependency. Covers both the Device-OS-detected case
  // (reason == RESET_REASON_WATCHDOG) and the AB1805-confirmed PIN_RESET
  // case classified above; both reuse the same lastWatchdog* fields, tagged
  // with lastWatchdogSource. An inconclusive or non-watchdog PIN_RESET must
  // not overwrite a previously persisted watchdog's forensic data.
  const bool watchdogClassified = watchdogResetDetected || ab1805ConfirmedWatchdog;
  if (watchdogClassified) {
    const uint16_t priorWatchdogResetCount = sysStatus.get_watchdogResetCount();
    if (priorWatchdogResetCount < 0xFFFF) {
      sysStatus.set_watchdogResetCount((uint16_t)(priorWatchdogResetCount + 1));
    }
    sysStatus.set_lastWatchdogBreadcrumb(previousBreadcrumb);
    sysStatus.set_lastWatchdogUptimeMs(previousBreadcrumbMs);
    sysStatus.set_lastWatchdogResetReasonData(reasonData);
    sysStatus.set_lastWatchdogSource(watchdogResetDetected ? WATCHDOG_SOURCE_DEVICE_OS : WATCHDOG_SOURCE_AB1805_PIN);

    // New watchdog-reset alert code (its own tier, strictly above tier-3;
    // see getAlertSeverity() in MyPersistentData.cpp - this must outrank
    // any already-active tier-3 alert like thrash detection or a boot
    // storm). One-time forensic marker - intentionally NOT added to
    // isAutoClearAfterReportAlert() so it stays sticky.
    current.raiseAlert(19);

    // Queue watchdog forensic event only after PublishQueuePosix setup above
    // initialized its mutex.
    publishWatchdogForensics(ab1805ConfirmedWatchdog);
  }

  time_t rtcTime = 0;
  const bool rtcReadOk = ab1805.getRtcAsTime(rtcTime);
  const bool timeValidAfterRtc = Time.isValid();
  if (!timeValidBeforeRtc && timeValidAfterRtc) {
    if (rtcReadOk) {
      if (sysStatus.get_verboseMode()) {
        Log.info("RTC restored system time: %s (rtc=%s)",
                 Time.timeStr().c_str(),
                 Time.format(rtcTime, TIME_FORMAT_DEFAULT).c_str());
      }
    } else if (sysStatus.get_verboseMode()) {
      Log.info("RTC restored system time: %s (rtc read failed)",
               Time.timeStr().c_str());
    }
  } else if (!timeValidAfterRtc) {
    Log.warn("RTC did not restore time (rtcSet=%s rtcReadOk=%s)",
             ab1805.isRTCSet() ? "true" : "false",
             rtcReadOk ? "true" : "false");
  }

  startupHibernateStatusReady = false;
  startupHibernateWakeReason = "UNKNOWN";
  startupHibernateActualSleepSec = 0;
  startupHibernateSleepErrorSec = 0;
#if PLATFORM_ID == PLATFORM_BORON
  if (retainedHibernatePending) {
    const AB1805::WakeReason wakeReason = ab1805.getWakeReason();
    startupHibernateWakeReason = ab1805WakeReasonName(wakeReason);
    if (reason == RESET_REASON_POWER_MANAGEMENT &&
        wakeReason == AB1805::WakeReason::ALARM &&
        rtcReadOk &&
        retainedHibernateRtcBefore > 0 &&
        retainedHibernateRequestedSleep > 0 &&
        rtcTime >= retainedHibernateRtcBefore) {
      startupHibernateActualSleepSec = (uint32_t)(rtcTime - retainedHibernateRtcBefore);
      startupHibernateSleepErrorSec = (int32_t)startupHibernateActualSleepSec -
                                      (int32_t)retainedHibernateRequestedSleep;
      startupHibernateStatusReady = true;

      Log.info("HibernateWake: reason=%s req=%lu actual=%lu err=%ld count=%lu",
               startupHibernateWakeReason,
               (unsigned long)retainedHibernateRequestedSleep,
               (unsigned long)startupHibernateActualSleepSec,
               (long)startupHibernateSleepErrorSec,
               (unsigned long)retainedHibernateCount);
    } else {
      Log.info("HibernateWake: pending=1 reason=%d wake=%s rtcOk=%d",
               reason,
               startupHibernateWakeReason,
               rtcReadOk ? 1 : 0);
    }
  }
#endif
  retainedHibernatePending = false;

  Cloud::instance().setup(); // Initialize the cloud functions

  // Enqueue a one-time status snapshot so the cloud can see
  // firmware version, reset reason, and any outstanding alert
  // soon after the first successful connection.
  publishStartupStatus();

  // Alert 44 is raised late in the previous wake cycle, after the normal
  // report path has already run. Treat the startup status snapshot as its
  // one-time report, then clear it so it does not linger into later
  // direct-connect service paths before the next scheduled report.
  if (current.get_alertCode() == 44) {
    Log.info("Clearing alert 44 after startup status snapshot");
    current.set_alertCode(0);
    current.set_lastAlertTime(0);
  }

  // ===== TIME AND TIMEZONE CONFIGURATION =====
  // Setup local time from persisted timezone string (POSIX TZ format).
  // This must be configured before we can make any open/close hour decisions.
  const char *tz = sysStatus.get_timeZoneStrCStr();
  if (!tz || tz[0] == '\0') {
    tz = Config::DEFAULT_TIMEZONE;
    sysStatus.set_timeZoneStr(tz);
  }
  LocalTime::instance().withConfig(LocalTimePosixTimezone(tz));

  Config::logDiagnostics("ConfigDiag");
  if (!Config::isValid(true)) {
    Log.warn("Config invalid at boot - forcing CONNECTING_STATE for ledger acquisition");
    transitionTo(CONNECTING_STATE, "invalid config");
  }

  // Validate time and configure local time converter
  if (!Time.isValid()) {
    transitionTo(CONNECTING_STATE, "time invalid");
  } else {
    // Now that time is valid, configure local time converter for timezone-aware operations
    conv.withCurrentTime().convert();

    // Check if waking from overnight hibernate - suppress alert 40 since
    // 8+ hours without webhook during closed hours is expected, not an error.
    if (System.resetReason() == RESET_REASON_POWER_MANAGEMENT) {
      uint8_t localHour = (uint8_t)(conv.getLocalTimeHMS().toSeconds() / 3600);
      if (localHour == sysStatus.get_openTime()) {
        Log.info("Wake from overnight hibernate at opening hour - suppressing alert 40");
        session.suppressAlert40ThisSession = true;
      }
    }
  }

  Log.info("Boot: v=%s reset=%d heap=%lu prev=%s open=%d FailsafeTest=%d",
           FIRMWARE_VERSION,
           reason,
           (unsigned long)System.freeMemory(),
           appBreadcrumbName(startupPreviousBreadcrumb),
           Time.isValid() ? (isWithinOpenHours() ? 1 : 0) : -1,
           CONNECTIVITY_FAILSAFE_TEST_MODE ? 1 : 0);
  if (watchdogResetDetected) {
    Log.warn("BootWDT: breadcrumb=%s stage=%s elapsed=%lu q=%u state=%u connMs=%lu",
             appBreadcrumbName(startupPreviousBreadcrumb),
             loopStageName(static_cast<LoopStage>(startupPreviousLoopStage)),
             (unsigned long)startupPreviousLoopStageElapsedMs,
             (unsigned)startupPreviousQueueDepth,
             (unsigned)startupPreviousState,
             (unsigned long)startupPreviousMillisSinceLastCloudConnect);
  }

#if CONNECTIVITY_FAILSAFE_TEST_MODE
  Log.info("Failsafe: test=%d stale=%lds cooldown=%lds jitter=%lds",
           CONNECTIVITY_FAILSAFE_TEST_MODE ? 1 : 0,
           (long)ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC,
           (long)ConnectivityPolicy::CONNECTIVITY_FAILSAFE_COOLDOWN_SEC,
           (long)ConnectivityPolicy::CONNECTIVITY_FAILSAFE_JITTER_MAX_SEC);
  ConnectivityFailsafeTest::logBootDiagnostics();
#endif

  // ===== SENSOR ABSTRACTION LAYER =====
  // Initialize the sensor based on configuration using *local* time. This
  // runs after timezone configuration so open/close checks are correct.
  if (!SensorManager::instance().isSensorReady()) {
    if (isWithinOpenHours()) {
      SensorManager::instance().initializeFromConfig();

      if (!SensorManager::instance().isSensorReady()) {
        Log.error("Sensor failed to initialize after timezone setup; connecting to report error");
        transitionTo(CONNECTING_STATE, "sensor init failed");
      }
    } else {
      // Ensure carrier sensor power rails are actually turned off even if
      // we skipped sensor initialization while closed.
      SensorManager::instance().onEnterSleep();
    }
  }

  // Defer the first startup battery sample until setup tail so RTC/time,
  // cloud/ledger setup, timezone config, connection-mode logging, and sensor
  // initialization have completed while the radio is still off.
  if (System.resetReason() == RESET_REASON_POWER_MANAGEMENT) {
    measure.noteWakeFromLowPowerSleep();
    PowerDiagnostics::logPowerState("post-wake-setup");
  }
  measure.batteryState(BatterySampleContext::Setup);
  PowerManager::instance().refreshInputProfile();
  PowerDiagnostics::logPowerState("setup", true);
  if (sysStatus.get_lowBatteryMode()) {
    applyBatteryAwareConnectionModePolicy(PowerManager::instance().soc());
  }
  // ===================================

  attachInterrupt(BUTTON_PIN, userSwitchISR,
                  FALLING); // We may need to monitor the user switch to change
                            // behaviours / modes

  if (state == INITIALIZATION_STATE)\
    transitionTo(IDLE_STATE, "setup complete"); // Default to IDLE; CONNECTING only when explicitly requested

  // setup reached stable completion; clear early-boot in-progress marker.
  bootInProgress = false;
  setAppBreadcrumb(BREADCRUMB_SETUP_COMPLETE);
  Observability::captureSuccessfulInitialization();
  signalLED(false);  // Turn off startup indicator
}

void loop() {
  // If we remain alive for a stability window, clear boot-storm counters.
  if (bootStormCount > 0 && millis() > 300000UL) {
    bootStormCount = 0;
    bootStormWindowStart = 0;
  }

  setLoopStage(LOOP_STAGE_CONNECTIVITY);
  connectivityFailsafeSupervisor();
  noteLoopStageDuration(false);

  // Main state machine driving sensing, reporting, power management
  setLoopStage(LOOP_STAGE_STATE_HANDLER);
  switch (state) {
  case INITIALIZATION_STATE:
    Log.warn("INITIALIZATION_STATE reached loop - transitioning to IDLE_STATE");
    transitionTo(IDLE_STATE, "loop-initialization-fallback");
    break;

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
  noteLoopStageDuration(false);

  setLoopStage(LOOP_STAGE_DIAGNOSTICS);
  thrashGuard.loop(state, millis());

  ab1805.loop(); // Keeps the RTC synchronized with the device clock

  // Housekeeping for each transit of the main loop
  current.loop();
  sysStatus.loop();
  sensorConfig.loop();
  noteLoopStageDuration(false);

  // Service deferred cloud work (ledger status publishes, etc.)
  setLoopStage(LOOP_STAGE_CLOUD_LOOP);
  setAppBreadcrumb(BREADCRUMB_CLOUD_LOOP_ENTER);
  Cloud::instance().loop();
  noteLoopStageDuration();
  setAppBreadcrumb(BREADCRUMB_CLOUD_LOOP_EXIT);

  // Service outgoing publish queue
  setLoopStage(LOOP_STAGE_PUBLISH_QUEUE);
  setAppBreadcrumb(BREADCRUMB_PUBLISH_QUEUE_ENTER);
  PublishQueuePosix::instance().loop();
  noteLoopStageDuration();
  setAppBreadcrumb(BREADCRUMB_PUBLISH_QUEUE_EXIT);

  setLoopStage(LOOP_STAGE_DIAGNOSTICS);
  serviceAwakeWatchdog();

  // Check for short-term webhook response timeout.
  // Requirement: 20 seconds starting only after a successful cloud connect.
  if (Particle.connected() && session.awaitingWebhookResponse && session.webhookAwaitStartMs != 0) {
    unsigned long timeoutMs = sysStatus.get_webhookTimeoutMs();
    if (timeoutMs < 5000UL || timeoutMs > 120000UL) {
      timeoutMs = 20000UL;
    }

    const unsigned long elapsedMs = millis() - session.webhookAwaitStartMs;
    if (elapsedMs > timeoutMs) {
      Log.warn("Webhook response timeout after %lu ms - raising alert 40", elapsedMs);
      session.awaitingWebhookResponse = false;
      session.webhookAwaitStartMs = 0;
      current.raiseAlert(40);
    }
  }

  // If an out-of-memory event occurred, go to error state
  if (outOfMemory >= 0) {
    Log.error("Out-of-memory event detected (param=%d freeHeap=%lu) - resetting",
              outOfMemory,
              (unsigned long)System.freeMemory());
    // Out-of-memory is treated as a critical alert; only overwrite any
    // existing alert if this is more severe.
    current.raiseAlert(14);
    transitionTo(ERROR_STATE, "out of memory");
  }

  // If the user switch is pressed, force an immediate report and connection
  if (userSwitchDetected) {
    Log.info("User switch pressed - triggering immediate report and connection");
    userSwitchDetected = false;
    session.serviceRequestTriggered = true;
    transitionTo(REPORTING_STATE, "user switch");
  }

  // ********** Centralized sensor event handling **********
  // Service sensor interrupts regardless of current state. This ensures
  // counts are captured even during long-running operations like cellular
  // connection attempts (which can take minutes) or firmware updates.
  // MEASUREMENT mode is time-based (handled in IDLE only), not interrupt-driven.
  uint8_t sensorMode = sysStatus.get_sensorMode(); 
  if (sensorMode == COUNTING) {
    handleCountingMode();  // Count each sensor event
  } else if (sensorMode == OCCUPANCY) {
    handleOccupancyMode(); // Track occupied/unoccupied state
  }

  noteLoopStageDuration();

} // End of loop

// ********** Helper Functions **********

void serviceAwakeWatchdog() {
#if Wiring_Watchdog
  if (!awakeWatchdogInitialized) {
    return;
  }

  if (Watchdog.started()) {
    Watchdog.refresh();
    awakeWatchdogStarted = true;
  } else {
    awakeWatchdogStarted = false;
  }
#endif
}

void pauseAwakeWatchdogForSleep(const char *context) {
#if Wiring_Watchdog
  if (!awakeWatchdogInitialized) {
    return;
  }

  if (awakeWatchdogSleepStrategy == AWAKE_WATCHDOG_SLEEP_MANUAL_STOP) {
    const int stopResult = Watchdog.stop();
    awakeWatchdogStarted = Watchdog.started();
#if ENABLE_SLEEP_TRACE
    Log.info("AppWDT: sleep ctx=%s action=stop strat=%s rc=%d started=%d",
             context,
             awakeWatchdogSleepStrategyName(awakeWatchdogSleepStrategy),
             stopResult,
             awakeWatchdogStarted ? 1 : 0);
#else
    if (stopResult != SYSTEM_ERROR_NONE) {
      Log.warn("AppWDT: sleep-stop failed ctx=%s strat=%s rc=%d started=%d",
               context,
               awakeWatchdogSleepStrategyName(awakeWatchdogSleepStrategy),
               stopResult,
               awakeWatchdogStarted ? 1 : 0);
    }
#endif
    return;
  }

  awakeWatchdogStarted = Watchdog.started();
#if ENABLE_SLEEP_TRACE
  Log.info("AppWDT: sleep ctx=%s action=auto strat=%s started=%d",
           context,
           awakeWatchdogSleepStrategyName(awakeWatchdogSleepStrategy),
           awakeWatchdogStarted ? 1 : 0);
#else
  if (!awakeWatchdogStarted) {
    Log.warn("AppWDT: sleep-auto watchdog-not-started ctx=%s strat=%s",
             context,
             awakeWatchdogSleepStrategyName(awakeWatchdogSleepStrategy));
  }
#endif
#else
  (void)context;
#endif
}

void restoreAwakeWatchdogAfterWake(const char *context) {
#if Wiring_Watchdog
  if (!awakeWatchdogInitialized) {
    return;
  }

  int startResult = SYSTEM_ERROR_NONE;
  const bool wasStarted = Watchdog.started();
  if (!wasStarted) {
    startResult = Watchdog.start();
  }

  awakeWatchdogStarted = Watchdog.started();

#if ENABLE_SLEEP_TRACE
  Log.info("AppWDT: wake ctx=%s action=%s strat=%s rc=%d started=%d",
           context,
           wasStarted ? "resume" : "start",
           awakeWatchdogSleepStrategyName(awakeWatchdogSleepStrategy),
           startResult,
           awakeWatchdogStarted ? 1 : 0);
#else
  if (startResult != SYSTEM_ERROR_NONE || !awakeWatchdogStarted) {
    Log.warn("AppWDT: wake abnormal ctx=%s action=%s strat=%s rc=%d started=%d",
             context,
             wasStarted ? "resume" : "start",
             awakeWatchdogSleepStrategyName(awakeWatchdogSleepStrategy),
             startResult,
             awakeWatchdogStarted ? 1 : 0);
  }
#endif
#else
  (void)context;
#endif
}

#if Wiring_Watchdog
static void awakeWatchdogExpiredHandler() {
  setAppBreadcrumb(BREADCRUMB_APP_WATCHDOG_RESET);
}
#else
// ApplicationWatchdog expects a plain function pointer.
static void appWatchdogHandler() {
  setAppBreadcrumb(BREADCRUMB_APP_WATCHDOG_RESET);
  System.reset();
}
#endif

// ===== Policy and publishing helpers =====

/**
 * @brief Calculates battery tier and downgrades connection mode when critically low.
 *
 * Maps state-of-charge to HEALTHY/CONSERVING/CRITICAL/SURVIVAL tiers with hysteresis
 * to prevent thrashing. Automatically downgrades occupancy device connection modes
 * (CONNECTED → INTERMITTENT, INTERMITTENT_KEEP_ALIVE → INTERMITTENT) to preserve
 * battery life during poor solar conditions.
 *
 * @param currentSoC Battery state of charge percentage (0.0 - 100.0)
 * @return BatteryTier Current battery health tier
 *
 * @note Downgrades are sticky - device will not auto-upgrade even if SoC recovers.
 *       Critical for preventing premature battery death in remote solar deployments.
 */
void applyBatteryAwareConnectionModePolicy(float currentSoC, BatteryTier resolvedTier) {
  BatteryTier newTier = resolvedTier;
  uint8_t prevTierValue = sysStatus.get_currentBatteryTier();
  const char* tierNames[] = {"HEALTHY", "CONSERVING", "CRITICAL", "SURVIVAL"};

  if (newTier != prevTierValue) {
    const char* prevName = (prevTierValue < 4) ? tierNames[prevTierValue] : "UNKNOWN";
    const char* newName = tierNames[newTier];
    Log.info("Battery tier transition: %s -> %s (SoC=%.1f%%)", prevName, newName, (double)currentSoC);
    sysStatus.set_currentBatteryTier(static_cast<uint8_t>(newTier));
  }

  if (sysStatus.get_sensorMode() == OCCUPANCY) {
    ConnectionMode currentMode = static_cast<ConnectionMode>(sysStatus.get_connectionMode());
    bool lowBatteryDowngradeActive = sysStatus.get_lowBatteryMode();

    if (currentMode == INTERMITTENT_KEEP_ALIVE && newTier >= TIER_CONSERVING) {
      Log.info("Battery conservation: Disabling KEEP_ALIVE mode (tier=%s, SoC=%.1f%%) - switching to INTERMITTENT",
               tierNames[newTier], (double)currentSoC);
      sysStatus.set_connectionMode(INTERMITTENT);
      sysStatus.set_lowBatteryMode(true);
    } else if (currentMode == INTERMITTENT && lowBatteryDowngradeActive && newTier == TIER_HEALTHY) {
      Log.info("Battery recovery: clearing lowBatteryMode (tier=HEALTHY, SoC=%.1f%%)",
               (double)currentSoC);
      Log.info("Battery recovery: restoring INTERMITTENT_KEEP_ALIVE (tier=HEALTHY, SoC=%.1f%%)",
               (double)currentSoC);
      sysStatus.set_connectionMode(INTERMITTENT_KEEP_ALIVE);
      sysStatus.set_lowBatteryMode(false);
    } else if (currentMode != INTERMITTENT && lowBatteryDowngradeActive) {
      Log.info("Battery recovery: clearing lowBatteryMode (tier=%s, SoC=%.1f%%)",
               tierNames[newTier],
               (double)currentSoC);
      sysStatus.set_lowBatteryMode(false);
    }
  } else if (sysStatus.get_lowBatteryMode()) {
    Log.info("Battery recovery: clearing lowBatteryMode (tier=%s, SoC=%.1f%%)",
             tierNames[newTier],
             (double)currentSoC);
    sysStatus.set_lowBatteryMode(false);
  }

}

void applyBatteryAwareConnectionModePolicy(float currentSoC) {
  const ReportingPolicy policy = ReportingPolicyResolver::resolveRuntime(
      currentSoC, Time.now());
  applyBatteryAwareConnectionModePolicy(currentSoC, policy.batteryTier);
}

static bool isWithinOpenHoursForHour(uint8_t hour, uint8_t openHour, uint8_t closeHour) {
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

static int secondsUntilNextOpenForSeconds(uint32_t secondsOfDay,
                                          uint8_t openHour,
                                          uint8_t closeHour,
                                          bool openNow) {
  uint32_t openSec = (uint8_t)openHour * 3600;
  uint32_t closeSec = (uint8_t)closeHour * 3600;

  // Normalize: if we're currently within opening hours, next open is tomorrow
  if (openNow) {
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
      // Otherwise next open is later today or tomorrow, but openNow
      // was already false so this path will generally be rare; fall back to 1 hour
      return 3600;
    }
  } else {
    // openHour == closeHour: always open; should not normally reach here
    return 3600;
  }
}


// Helper to determine whether current *local* time is within park open hours.
// Local time is derived from LocalTimeRK using the configured timezone.
// If time is not yet valid, we treat it as "open" so the device can start
// sensing while it acquires time and configuration.
bool isWithinOpenHours() {
  if (!Time.isValid()) {
    return true;
  }

  if (!Config::isValid(false)) {
    return true;
  }

  uint8_t openHour = sysStatus.get_openTime();
  uint8_t closeHour = sysStatus.get_closeTime();
  const LocalTimeCache::LocalTimeSnapshot &snapshot = LocalTimeCache::getLocalTimeSnapshot();
  const uint8_t hour = snapshot.localHour;
  const bool openNow = isWithinOpenHoursForHour(hour, openHour, closeHour);

  return openNow;
}

bool isWithinOpenHoursAt(time_t epoch) {
  if (!Time.isValid() || !Config::isValid(false)) {
    return true;
  }

  LocalTimeConvert converter;
  converter.withConfig(LocalTime::instance().getConfig()).withTime(epoch).convert();
  const uint8_t localHour = (uint8_t)(converter.getLocalTimeHMS().toSeconds() / 3600);
  return isWithinOpenHoursForHour(
      localHour, sysStatus.get_openTime(), sysStatus.get_closeTime());
}

void logTimeDiag(bool isOpen) {
  const char *tz = sysStatus.get_timeZoneStrCStr();
  if (!tz) {
    tz = "";
  }

  const bool timeValid = Time.isValid();
  const time_t epoch = Time.now();
  struct tm utcTm = {};
  if (timeValid) {
    gmtime_r(&epoch, &utcTm);
  }

  const LocalTimeCache::LocalTimeSnapshot &snapshot = LocalTimeCache::getLocalTimeSnapshot();
  const LocalTimeYMD localDate = snapshot.localYmd;
  const uint32_t localSecondsOfDay = snapshot.localSecondsOfDay;
  const uint8_t localHour = snapshot.localHour;
  const uint8_t localMinute = (uint8_t)((localSecondsOfDay / 60UL) % 60UL);
  const uint8_t localSecond = (uint8_t)(localSecondsOfDay % 60UL);

  Log.info("TimeDiag: tz=%s valid=%d epoch=%lu utc=%04d-%02d-%02d %02d:%02d:%02d local=%04d-%02d-%02d %02d:%02d:%02d open=%d close=%d isOpen=%d",
           tz,
           timeValid ? 1 : 0,
           (unsigned long)epoch,
           utcTm.tm_year + 1900,
           utcTm.tm_mon + 1,
           utcTm.tm_mday,
           utcTm.tm_hour,
           utcTm.tm_min,
           utcTm.tm_sec,
           localDate.getYear(),
           localDate.getMonth(),
           localDate.getDay(),
           localHour,
           localMinute,
           localSecond,
           (int)sysStatus.get_openTime(),
           (int)sysStatus.get_closeTime(),
           isOpen ? 1 : 0);
}

// Helper to compute seconds until next park opening time (local time)
int secondsUntilNextOpen() {
  if (!Time.isValid() || !Config::isValid(false)) {
    return Config::DEFAULT_REPORT_INTERVAL_SEC;
  }

  uint8_t openHour = sysStatus.get_openTime();
  uint8_t closeHour = sysStatus.get_closeTime();
  const LocalTimeCache::LocalTimeSnapshot &snapshot = LocalTimeCache::getLocalTimeSnapshot();
  const bool openNow = isWithinOpenHoursForHour(snapshot.localHour, openHour, closeHour);
  const int secondsUntil = secondsUntilNextOpenForSeconds(
      snapshot.localSecondsOfDay, openHour, closeHour, openNow);

  return secondsUntil;
}

/**
 * @brief Shared notes for report payload construction.
 *
 * @details
 * - Webhook payloads are compact and follow the legacy Ubidots field contract.
 * - Device-data ledger publishes carry a richer structured snapshot for Console visibility.
 * - Occupancy totals are reported in whole minutes for both webhook and ledger paths.
 */
/**
 * @brief Returns true for transient alerts that may auto-clear after reporting.
 *
 * @details These alerts represent operational failures that are re-evaluated
 *          frequently enough to be raised again on later cycles if the
 *          underlying condition persists.
 *
 * @param alertCode Current alert code carried in the report payload.
 * @return true if the alert should auto-clear after successful queueing.
 */
static bool isAutoClearAfterReportAlert(int alertCode) {
  switch (alertCode) {
  case 15:
  case 31:
  case 41:
  case 43:
  case 44:
    return true;
  default:
    return false;
  }
}

/**
 * @brief Publishes hourly sensor data and device telemetry to the cloud.
 *
 * Queues a comprehensive JSON payload containing counts, occupancy metrics, battery state,
 * connectivity health, and diagnostics. Uses current timestamp in occupancy mode for
 * real-time updates; uses end-of-hour timestamp in counting mode for bucketed aggregation.
 *
 * @note Does not perform immediate publish - queued for delivery during CONNECTING_STATE.
 */
void publishData() {
  // Legacy Ubidots context strings describing battery state
  static const char *batteryContext[7] = {
    "Unknown", "Not Charging", "Charging",
    "Charged", "Discharging", "Fault",
    "Disconnected"
  };

  char data[256];

  // We support two timestamp strategies:
  // - Occupancy mode: use current time so the value updates in Ubidots even
  //   if multiple publishes happen within the same hour.
  // - Counting mode: keep the legacy "end of previous hour" timestamp so
  //   hourly bucket aggregation behavior remains unchanged.
  const unsigned long nowStampSec = Time.now();
  const unsigned long endOfPrevHourStampSec = nowStampSec - (Time.minute() * 60L + Time.second() + 1L);

  // Bounds check battery state index for safety
  uint8_t battState = PowerManager::instance().batteryState();
  if (battState > 6) {
    battState = 0;
  }

  uint8_t sensorMode = sysStatus.get_sensorMode();
  const int8_t reportedAlertCode = current.get_alertCode();

  // Ensure battery value is always a finite number for webhook ingestion.
  // Ubidots rejects NaN/Inf with a 400 response.
  float stateOfCharge = PowerManager::instance().soc();
  if (!(stateOfCharge == stateOfCharge) || stateOfCharge < 0.0f || stateOfCharge > 100.0f) {
    stateOfCharge = 0.0f;
    battState = 0;
  }

  // Build webhook payload based on sensor mode
  if (sensorMode == OCCUPANCY) {
    const unsigned long timeStampValue = nowStampSec;
    const unsigned long totalOccupiedMinutes = (unsigned long)(current.get_totalOccupiedSeconds() / 60UL);

    // Occupancy mode webhook format (occupancy as 0/1 numeric value)
    snprintf(data, sizeof(data),
             "{\"occupancy\":%d,\"dailyoccupancy\":%lu,\"battery\":%4.2f,\"key1\":\"%s\",\"temp\":%4.2f,\"alerts\":%i,\"resets\":%i,\"connecttime\":%i,\"timestamp\":%lu000}",
             current.get_occupied() ? 1 : 0,
             totalOccupiedMinutes,
             stateOfCharge,
             batteryContext[battState],
             current.get_internalTempC(),
             reportedAlertCode,
             sysStatus.get_resetCount(),
             sysStatus.get_lastConnectionDuration(),
             timeStampValue);

  } else {
    const unsigned long timeStampValue = endOfPrevHourStampSec;

    // Counting mode webhook format (original format)
    snprintf(data, sizeof(data),
             "{\"hourly\":%i,\"daily\":%i,\"battery\":%4.2f,\"key1\":\"%s\",\"temp\":%4.2f,\"resets\":%i,\"alerts\":%i,\"connecttime\":%i,\"timestamp\":%lu000}",
             current.get_hourlyCount(),
             current.get_dailyCount(),
             stateOfCharge,
             batteryContext[battState],
             current.get_internalTempC(),
             sysStatus.get_resetCount(),
             reportedAlertCode,
             sysStatus.get_lastConnectionDuration(),
             timeStampValue);

  }

  // Get webhook name from cloud configuration (with fallback to convention)
  const char *webhookName = Cloud::instance().getWebhookName();

  setAppBreadcrumb(BREADCRUMB_REPORT_QUEUE_START);
  const unsigned long queueStartMs = millis();
  bool queued = PublishQueuePosix::instance().publish(webhookName, data, PRIVATE);
  const unsigned long queueElapsedMs = millis() - queueStartMs;
  setAppBreadcrumb(BREADCRUMB_REPORT_QUEUE_DONE);
  if (!queued) {
    Log.warn("Report webhook queue rejected name=%s", webhookName);
  }
  if (!queued || queueElapsedMs >= REPORT_FORENSICS_ABNORMAL_WARN_THRESHOLD_MS) {
    Log.warn("ReportPerf: step=queue ms=%lu queued=%d",
             queueElapsedMs,
             queued ? 1 : 0);
  } else if (ENABLE_PERF_TRACE && queueElapsedMs >= REPORT_FORENSICS_SLOW_LOG_THRESHOLD_MS) {
    Log.info("ReportPerf: step=queue ms=%lu queued=%d",
             queueElapsedMs,
             queued ? 1 : 0);
  }

  // General alert lifecycle rule: once an alert has been included in a report
  // and that report is accepted by the publish queue, clear it locally.
  // If the underlying condition persists, it will be raised again.
  if (queued &&
      reportedAlertCode > 0 &&
      isAutoClearAfterReportAlert(reportedAlertCode) &&
      current.get_alertCode() == reportedAlertCode) {
    if (sysStatus.get_verboseMode()) {
      Log.info("Clearing alert %d after queueing report payload", (int)reportedAlertCode);
    }
    current.set_alertCode(0);
    current.set_lastAlertTime(0);
  }

  // Arm short-term webhook supervision only if publish succeeded.
  // If we're already connected, start the 20s window immediately.
  // Otherwise start it when CONNECTING_STATE reports a successful cloud connect.
  if (queued) {
    session.webhookExpectedOnConnect = true;
    if (Particle.connected()) {
      session.webhookExpectedOnConnect = false;
      session.awaitingWebhookResponse = true;
      session.webhookAwaitStartMs = millis();
    }
  } else {
    Log.info("Webhook supervision not armed: publish queue rejected");
  }

  // Also update device-data ledger with structured JSON snapshot.
  setAppBreadcrumb(BREADCRUMB_REPORT_LEDGER_START);
  const unsigned long ledgerStartMs = millis();
  const bool ledgerOk = Cloud::instance().publishDataToLedger("ReportState");
  const unsigned long ledgerElapsedMs = millis() - ledgerStartMs;
  setAppBreadcrumb(BREADCRUMB_REPORT_LEDGER_DONE);
  setAppBreadcrumb(BREADCRUMB_REPORT_POST_LEDGER);
  const char *ledgerState = ledgerOk ? "req" : "err";
  if (!ledgerOk) {
    // Data ledger publish failure; escalate via alert so the error
    // supervisor can decide on corrective action.
    current.raiseAlert(42);
  }
  if (!ledgerOk || ledgerElapsedMs >= REPORT_FORENSICS_ABNORMAL_WARN_THRESHOLD_MS) {
    Log.warn("ReportPerf: step=ledger ms=%lu ok=%d",
             ledgerElapsedMs,
             ledgerOk ? 1 : 0);
  } else if (ENABLE_PERF_TRACE && ledgerElapsedMs >= REPORT_FORENSICS_SLOW_LOG_THRESHOLD_MS) {
    Log.info("ReportPerf: step=ledger ms=%lu ok=%d",
             ledgerElapsedMs,
             ledgerOk ? 1 : 0);
  }

  if (sensorMode == OCCUPANCY) {
    Log.info("Report: occ=%d totalMin=%lu alert=%d q=%d ledger=%s",
             current.get_occupied() ? 1 : 0,
             (unsigned long)(current.get_totalOccupiedSeconds() / 60UL),
             (int)reportedAlertCode,
             queued ? 1 : 0,
             ledgerState);
  } else {
    Log.info("Report: hourly=%d daily=%d alert=%d q=%d ledger=%s",
             (int)current.get_hourlyCount(),
             (int)current.get_dailyCount(),
             (int)reportedAlertCode,
             queued ? 1 : 0,
             ledgerState);
  }

  setAppBreadcrumb(BREADCRUMB_REPORT_EXIT);
}

// ===== Startup status and webhook supervision =====

/**
 * @brief Enqueue a one-time startup status event summarizing
 *        firmware version, reset reason, and any active alert.
 *
 * This uses PublishQueuePosix so the event will be delivered
 * after the next successful cloud connection, even if called
 * before the radio is brought up.
 */

/**
 * @brief Publishes comprehensive boot forensics for post-mortem incident analysis.
 *
 * Includes reset reason, watchdog state, connectivity failsafe history, alert codes,
 * heap usage, and PMIC anomalies. Critical for diagnosing unexpected resets in field
 * deployments without physical device access.
 *
 * @note Large payload (~896 bytes). Called once during setup(); queued for first connection.
 */
void publishStartupStatus() {
  char status[1024];

  int resetReason = System.resetReason();
  uint32_t resetReasonData = System.resetReasonData();
  int8_t alertCode = current.get_alertCode();
  time_t lastAlert = current.get_lastAlertTime();
  unsigned long freeHeap = System.freeMemory();
  const uint8_t failsafeStage = sysStatus.get_connectivityRecoveryStage();
  const uint8_t failsafeCount = sysStatus.get_connectivityRecoveryCount();
  const time_t failsafeLastAction = sysStatus.get_lastConnectivityRecoveryAction();
  const uint16_t watchdogResetCount = sysStatus.get_watchdogResetCount();
  const uint8_t lastWatchdogBreadcrumb = sysStatus.get_lastWatchdogBreadcrumb();
  const uint32_t lastWatchdogUptimeMs = sysStatus.get_lastWatchdogUptimeMs();
  const uint32_t lastWatchdogResetReasonData = sysStatus.get_lastWatchdogResetReasonData();
#if defined(ENABLE_PMIC_FORENSICS) && ENABLE_PMIC_FORENSICS
  const uint16_t startupPmicAnomalyCount = pmicAnomalyCount;
  const float startupLastPmicAnomalySoc = lastPmicAnomalySoc;
  const uint8_t startupLastPmicAnomalyChargeStatus = lastPmicAnomalyChargeStatus;
  const uint32_t startupLastPmicAnomalyAgeSec = pmicAnomalyAgeSec();
  const uint8_t startupLastPmicAnomalyPowerSource = lastPmicAnomalyPowerSource;
  const uint8_t startupLastPmicAnomalyVbusStatus = lastPmicAnomalyVbusStatus;
#endif
  const time_t lastConnection = sysStatus.get_lastConnection();
  const long lastConnectionAgeSec =
      (Time.isValid() && lastConnection != 0 && Time.now() > lastConnection)
          ? (long)(Time.now() - lastConnection)
          : -1L;
  char hibernateFields[192] = "";
  if (startupHibernateStatusReady) {
    snprintf(hibernateFields,
             sizeof(hibernateFields),
             ",\"sleepMode\":\"hibernate\",\"wakeReason\":\"%s\",\"actualSleep\":%lu,\"sleepError\":%ld,\"hibernateCount\":%lu",
             startupHibernateWakeReason,
             (unsigned long)startupHibernateActualSleepSec,
             (long)startupHibernateSleepErrorSec,
             (unsigned long)retainedHibernateCount);
  }

  // Only present when the OS reset reason was PIN_RESET (see the AB1805
  // classification block in setup()) - surfaces the AB1805's own wake-reason
  // read result and the resolved watchdogSource so a PIN_RESET boot can be
  // told apart in cloud telemetry, without requiring serial-log tracing.
  // No separate "read ok" flag is published: WakeReason::UNKNOWN already and
  // correctly covers both "chip detection failed" and "no known wake-reason
  // bit set" (see the classification block above), so a distinct success
  // flag would be redundant and cannot be obtained without repeating the
  // destructive updateWakeReason() read that Fix 2 removed.
  char pinResetAb1805Fields[160] = "";
  if (startupPinResetAb1805Checked) {
    snprintf(pinResetAb1805Fields,
             sizeof(pinResetAb1805Fields),
             ",\"ab1805WakeReason\":\"%s\",\"watchdogSource\":\"%s\"",
             startupAb1805WakeReasonName,
             startupAb1805ConfirmedWatchdog ? "AB1805_PIN" : "NONE");
  }

#if defined(ENABLE_PMIC_FORENSICS) && ENABLE_PMIC_FORENSICS
  snprintf(status, sizeof(status),
           "{\"version\":\"%s\",\"resetReason\":%d,\"resetReasonData\":%lu,\"alert\":%d,\"lastAlert\":%ld,\"freeHeap\":%lu,\"appBreadcrumb\":%u,\"appBreadcrumbMs\":%lu,\"watchdogResetCount\":%u,\"lastWatchdogBreadcrumb\":%u,\"lastWatchdogUptimeMs\":%lu,\"lastWatchdogResetReasonData\":%lu,\"pmicAnomalyCount\":%u,\"lastPmicAnomalySoc\":%.2f,\"lastPmicAnomalyChargeStatus\":%u,\"lastPmicAnomalyAgeSec\":%lu,\"lastPmicAnomalyPowerSource\":%u,\"lastPmicAnomalyVbusStatus\":%u,\"failsafeStage\":%u,\"failsafeCount\":%u,\"failsafeLastAction\":%ld,\"lastConnectionAgeSec\":%ld,\"failsafeTest\":%d,\"failsafeTestMode\":%d%s%s}",
           FIRMWARE_VERSION,
           resetReason,
           (unsigned long)resetReasonData,
           (int)alertCode,
           (long)lastAlert,
           freeHeap,
           (unsigned)startupPreviousBreadcrumb,
           (unsigned long)startupPreviousBreadcrumbMs,
           (unsigned)watchdogResetCount,
           (unsigned)lastWatchdogBreadcrumb,
           (unsigned long)lastWatchdogUptimeMs,
           (unsigned long)lastWatchdogResetReasonData,
           (unsigned)startupPmicAnomalyCount,
           (double)startupLastPmicAnomalySoc,
           (unsigned)startupLastPmicAnomalyChargeStatus,
           (unsigned long)startupLastPmicAnomalyAgeSec,
           (unsigned)startupLastPmicAnomalyPowerSource,
           (unsigned)startupLastPmicAnomalyVbusStatus,
           (unsigned)failsafeStage,
           (unsigned)failsafeCount,
           (long)failsafeLastAction,
           lastConnectionAgeSec,
           CONNECTIVITY_FAILSAFE_TEST_MODE ? 1 : 0,
           CONNECTIVITY_FAILSAFE_TEST_MODE ? 1 : 0,
           hibernateFields,
           pinResetAb1805Fields);
#else
  snprintf(status, sizeof(status),
           "{\"version\":\"%s\",\"resetReason\":%d,\"resetReasonData\":%lu,\"alert\":%d,\"lastAlert\":%ld,\"freeHeap\":%lu,\"appBreadcrumb\":%u,\"appBreadcrumbMs\":%lu,\"watchdogResetCount\":%u,\"lastWatchdogBreadcrumb\":%u,\"lastWatchdogUptimeMs\":%lu,\"lastWatchdogResetReasonData\":%lu,\"failsafeStage\":%u,\"failsafeCount\":%u,\"failsafeLastAction\":%ld,\"lastConnectionAgeSec\":%ld,\"failsafeTest\":%d,\"failsafeTestMode\":%d%s%s}",
           FIRMWARE_VERSION,
           resetReason,
           (unsigned long)resetReasonData,
           (int)alertCode,
           (long)lastAlert,
           freeHeap,
           (unsigned)startupPreviousBreadcrumb,
           (unsigned long)startupPreviousBreadcrumbMs,
           (unsigned)watchdogResetCount,
           (unsigned)lastWatchdogBreadcrumb,
           (unsigned long)lastWatchdogUptimeMs,
           (unsigned long)lastWatchdogResetReasonData,
           (unsigned)failsafeStage,
           (unsigned)failsafeCount,
           (long)failsafeLastAction,
           lastConnectionAgeSec,
           CONNECTIVITY_FAILSAFE_TEST_MODE ? 1 : 0,
           CONNECTIVITY_FAILSAFE_TEST_MODE ? 1 : 0,
           hibernateFields,
           pinResetAb1805Fields);
#endif

  PublishQueuePosix::instance().publish("status", status, PRIVATE);
}

/**
 * @brief Publishes detailed watchdog timeout forensic snapshot after WDT reset.
 *
 * Extracts retained loop forensic data (breadcrumb, stage, elapsed time, queue depth,
 * connection age) to pinpoint exactly where the main loop stalled. Essential for
 * diagnosing watchdog timeout root causes in production.
 *
 * @param ab1805Confirmed True when this event is for an AB1805-confirmed external
 *        watchdog reset (Device OS reported RESET_REASON_PIN_RESET, but the AB1805's
 *        own wake-reason register confirmed WakeReason::WATCHDOG). False for the
 *        Device-OS-detected case (RESET_REASON_WATCHDOG). Only called for one of
 *        these two classified cases - see the "WATCHDOG RESET PERSISTENCE /
 *        FORENSICS" block in setup(). Forensic data captured from FRAM.
 */
void publishWatchdogForensics(bool ab1805Confirmed) {
  char payload[224];
  const LoopStage stage = static_cast<LoopStage>(startupPreviousLoopStage);
  const int osResetReason = System.resetReason();

  // "ab1805_watchdog" distinguishes an AB1805-confirmed external reset (OS
  // reason PIN_RESET) from "watchdog", the Device-OS-detected case (OS
  // reason RESET_REASON_WATCHDOG) - both land here via the same classified
  // condition in setup().
  const char *resetLabel = ab1805Confirmed ? "ab1805_watchdog" : "watchdog";

  snprintf(payload,
           sizeof(payload),
           "{\"reset\":\"%s\",\"osReason\":%d,\"bc\":%u,\"stage\":\"%s\",\"elapsed\":%lu,\"queue\":%u,\"state\":%u,\"connAge\":%lu}",
           resetLabel,
           osResetReason,
           (unsigned)startupPreviousBreadcrumb,
           loopStageForensicName(stage),
           (unsigned long)startupPreviousLoopStageElapsedMs,
           (unsigned)startupPreviousQueueDepth,
           (unsigned)startupPreviousState,
           (unsigned long)startupPreviousMillisSinceLastCloudConnect);

  PublishQueuePosix::instance().publish("watchdog", payload, PRIVATE);
}

/**
 * @brief Handle response from Ubidots webhook.
 *
 * @details This handler is necessary for webhook response supervision (alert 40).
 * PublishQueue tracks publish success, but cannot verify end-to-end webhook delivery
 * to Ubidots. This handler confirms the webhook template executed successfully and
 * Ubidots accepted the data (HTTP 200/201). Without this, we cannot distinguish
 * between "event published to Particle cloud" vs "data actually received by Ubidots".
 *
 * During each connection, devices must complete 4 tasks:
 *   1) Clear the publish queue (drain queued events)
 *   2) Sync Ledger (configuration and data)
 *   3) Check for firmware updates
 *   4) Receive Ubidots webhook response (this handler)
 *
 * The webhook response arrives asynchronously (typically 5-15 seconds after publish),
 * so devices must remain connected long enough to complete all 4 tasks. Task timing
 * is managed by connection budget and state machine logic in CONNECTING_STATE and
 * REPORTING_STATE.
 */
void UbidotsHandler(const char *event, const char *data) {
  // Handle response from Ubidots webhook (legacy integration)
  char responseString[64];
  bool responseOk = true;
  // Response is expected to be a single numeric code from the Particle
  // integration response template (e.g. "200" or "201").
  if (!data || !strlen(data)) {
    snprintf(responseString, sizeof(responseString), "No Data");
    responseOk = false;
    Log.warn("Webhook response empty");
  } else {
    // Any webhook response indicates the integration path is alive.
    // Update lastHookResponse even if it arrived after our short-term window.
    sysStatus.set_lastHookResponse(Time.now());

    if (session.awaitingWebhookResponse) {
      session.awaitingWebhookResponse = false;
      session.webhookAwaitStartMs = 0;
    }

    // Clear webhook supervision alert (40) on any response.
    if (current.get_alertCode() == 40) {
      current.set_alertCode(0);
      current.set_lastAlertTime(0);
    }

    // If the response is numeric, treat 200/201 as success.
    // Otherwise, treat it as an opaque success marker.
    if (isdigit((unsigned char)data[0])) {
      int code = atoi(data);
      if (code == 200 || code == 201) {
        snprintf(responseString, sizeof(responseString), "Response Received");
      } else {
        snprintf(responseString, sizeof(responseString), "Hook response %d", code);
        responseOk = false;
        Log.warn("%s", responseString);
      }
    } else {
      snprintf(responseString, sizeof(responseString), "Response Received");
    }
  }
  if (sysStatus.get_verboseMode() && Particle.connected() && PublishQueuePosix::instance().getCanSleep()) {
    publishDiagnosticSafe("Ubidots Hook", responseString, PRIVATE);
  }
  if (sysStatus.get_verboseMode() && responseOk) {
    Log.info("%s", responseString);
  }
}


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
  PublishQueuePosix::instance().publish(eventName, data, flags);
  return true;
}

/**
 * @brief Publish a state transition to the log handler.
 * 
 * @details Logs transitions between states with context on time validity
 *          when entering IDLE_STATE. Updates oldState to current state.
 */
void publishStateTransition() {
  thrashGuard.recordStateTransition(oldState, state);
  thrashGuard.markProgress("STATE_TRANSITION");
  Log.info("State: %s->%s",
           stateShortName(oldState),
           stateShortName(state));
  oldState = state;
}

void transitionTo(State newState, const char *reason) {
  // Single universal choke point for every SLEEPING_STATE exit, from any
  // call site in any file: state is written nowhere else. This closes any
  // still-open SLEEP_PREP span exactly once per real dwell, regardless of
  // whether the exit originates inside handleSleepingState() or from
  // elsewhere (out-of-memory handling, user-switch handling, or
  // connectivityFailsafeSupervisor()'s failsafe-stage-1 action) - see
  // WO-2026-08-11-001 Fourth Corrective Pass.
  if (state == SLEEPING_STATE && retainedLoopForensics.sleepPrepSpanStartMillis != 0) {
    const unsigned long nowMs = millis();
    const unsigned long elapsedMs =
        closeSleepPrepSpan(retainedLoopForensics.sleepPrepSpanStartMillis, nowMs);
    const uint16_t queueDepth = (uint16_t)PublishQueuePosix::instance().getNumEvents();
    const uint32_t millisSinceLastCloudConnect = (connectedStartMs != 0 && nowMs >= connectedStartMs)
        ? (uint32_t)(nowMs - connectedStartMs) : 0UL;
    logLoopStageLine(LOG_LEVEL_INFO, LOOP_STAGE_SLEEP_PREP, elapsedMs, queueDepth, millisSinceLastCloudConnect);
  }
  Log.info("StateReq: %s->%s reason=%s",
           stateShortName(state),
           stateShortName(newState),
           (reason != nullptr) ? reason : "unspecified");
  state = newState;
}

// ===== Recovery, failsafe, and maintenance helpers =====

void outOfMemoryHandler(system_event_t event, int param) {
  outOfMemory = param;
}

void clearConnectivityFailsafeRecovery(const char *reason) {
  const bool hadRecoveryState =
      sysStatus.get_connectivityRecoveryStage() != 0 ||
      sysStatus.get_lastConnectivityRecoveryAction() != 0 ||
      sysStatus.get_connectivityRecoveryCount() != 0;
  const bool hadAlert = (current.get_alertCode() == ConnectivityPolicy::CONNECTIVITY_FAILSAFE_ALERT);

  if (!hadRecoveryState && !hadAlert) {
    return;
  }

  sysStatus.set_connectivityRecoveryStage(0);
  sysStatus.set_lastConnectivityRecoveryAction(0);
  sysStatus.set_connectivityRecoveryCount(0);
  sysStatus.flush(true);

  if (hadAlert) {
    current.set_alertCode(0);
    current.set_lastAlertTime(0);
  }

  Log.info("Failsafe: cleared reason=%s", reason ? reason : "unknown");
}

/**
 * @brief Monitors connection health and triggers escalating recovery when stale.
 *
 * Implements 3-stage recovery when connection attempts exceed policy thresholds:
 * Stage 1 (radio reset), Stage 2 (system reset), Stage 3 (deep power-down, Boron only).
 * Prevents indefinite modem-on battery drain and recovers from radio firmware wedges.
 *
 * Defers action if disconnected, firmware updating, time invalid, recently connected,
 * within cooldown period, or jitter delay not elapsed (prevents fleet-wide simultaneous resets).
 *
 * @note Stage 3 deep power-down will not wake for hours/days - last resort only.
 * @see docs/architecture/connectivity-failsafe.md for escalation timing and policy details
 */
void connectivityFailsafeSupervisor() {
  if (sysStatus.get_connectionMode() == DISCONNECTED) {
#if CONNECTIVITY_FAILSAFE_TEST_MODE
    ConnectivityFailsafeTest::logDeferDisconnectedMode();
#endif
    return;
  }

  if (state == FIRMWARE_UPDATE_STATE || System.updatesPending()) {
#if CONNECTIVITY_FAILSAFE_TEST_MODE
    ConnectivityFailsafeTest::logDeferUpdatePending();
#endif
    return;
  }

  if (!Time.isValid()) {
#if CONNECTIVITY_FAILSAFE_TEST_MODE
    ConnectivityFailsafeTest::logDeferInvalidTime();
#endif
    return;
  }

  const time_t lastConnection = sysStatus.get_lastConnection();
  if (lastConnection == 0) {
#if CONNECTIVITY_FAILSAFE_TEST_MODE
    ConnectivityFailsafeTest::logDeferNoLastConnection();
#endif
    return;
  }

  const time_t now = Time.now();
  if (now <= lastConnection) {
    return;
  }

  const time_t connectionAgeSec = now - lastConnection;
  if (connectionAgeSec < ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC) {
    return;
  }

  uint8_t currentStage = sysStatus.get_connectivityRecoveryStage();
  if (currentStage > 3) {
    currentStage = 0;
  }
  if (currentStage >= 3) {
    return;
  }

  time_t lastAction = sysStatus.get_lastConnectivityRecoveryAction();
  if (lastAction > now) {
    lastAction = 0;
  }

  const uint8_t nextStage = currentStage + 1;
  time_t requiredDelay = ConnectivityPolicy::CONNECTIVITY_FAILSAFE_COOLDOWN_SEC;
  if (nextStage >= 2) {
    requiredDelay += (time_t)connectivityFailsafeJitterSec(nextStage);
  }

  if (currentStage != 0 && lastAction != 0 && (now - lastAction) < requiredDelay) {
    return;
  }

  if (activeConnectAttemptWithinBudget()) {
    return;
  }

  const BatteryTier tier = currentBatteryTierForFailsafe();
  const bool externalPowerPresent = connectivityFailsafeHasExternalPower();
  const bool lowBatteryHardActionBlocked =
      nextStage >= 2 &&
      !externalPowerPresent &&
      (sysStatus.get_lowBatteryMode() || tier == TIER_SURVIVAL);

  if (lowBatteryHardActionBlocked) {
#if CONNECTIVITY_FAILSAFE_TEST_MODE
    ConnectivityFailsafeTest::logDeferLowBatteryHardStageSuppressed(nextStage, tier);
#else
    Log.info("Failsafe: defer reason=low-battery stage=%u tier=%s",
             (unsigned)nextStage,
             batteryTierShortName(tier));
#endif
    persistConnectivityFailsafeState(currentStage, now, false);
    return;
  }

  if (nextStage == 1) {
    current.raiseAlert(ConnectivityPolicy::CONNECTIVITY_FAILSAFE_ALERT);
    setAppBreadcrumb(BREADCRUMB_CONNECTIVITY_FAILSAFE);
    persistConnectivityFailsafeState(1, now, true);
    Log.info("Failsafe: stage=1 action=radio-reset age=%lds",
             (long)connectionAgeSec);
    Connectivity::requestFullDisconnectAndRadioOff();
    transitionTo(CONNECTING_STATE, "failsafe stage 1");
    return;
  }

  if (nextStage == 2) {
    setAppBreadcrumb(BREADCRUMB_CONNECTIVITY_FAILSAFE_HARD);
    persistConnectivityFailsafeState(2, now, true);
    Log.info("Failsafe: stage=2 action=system-reset age=%lds",
             (long)connectionAgeSec);
    System.reset();
    return;
  }

  setAppBreadcrumb(BREADCRUMB_CONNECTIVITY_FAILSAFE_HARD);
  persistConnectivityFailsafeState(3, now, true);
  Log.info("Failsafe: stage=3 action=deep-powerdown age=%lds",
           (long)connectionAgeSec);
  ab1805.deepPowerDown();
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

/**
 * @brief Cleanup function that is run at the beginning of the day.
 *
 * @details Called from REPORTING_STATE once per local day. If connected, it
 *          requests a time sync and records lastTimeSync, then resets daily
 *          counters and related housekeeping state.
 */
void dailyCleanup() {
  if (Particle.connected()) {
    publishDiagnosticSafe("Daily Cleanup", "Running", PRIVATE);
    
    // Force time sync once per day to prevent clock drift.
    // dailyCleanup() is called once per day from REPORTING_STATE. All devices
    // should connect at least daily, but LOW_POWER and DISCONNECTED mode devices
    // may not be connected at the specific time dailyCleanup runs. If a device
    // connects daily but misses the cleanup window each day, it won't sync time
    // despite daily connections, causing drift to accumulate over multiple days.
    // The AB1805 RTC has ±2.0 ppm accuracy (~±5 seconds/month typical), so
    // missing sync for several days can accumulate noticeable drift. This ensures
    // at least one time sync per day when the device happens to be connected
    // during the cleanup window.
    Log.info("Daily time sync requested");
    Particle.syncTime();
    sysStatus.set_lastTimeSync(Time.now());
  }
  
  Log.info("Running Daily Cleanup");
  
  current.resetEverything(); // Zero the counts for the new day
}
