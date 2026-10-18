#include "LoopForensics.h"
#include "PublishQueuePosixRK.h"
#include "../ResetCause.h"

static uint8_t &appBreadcrumb = retainedBoot.appBreadcrumb;
static uint32_t &appBreadcrumbMs = retainedBoot.appBreadcrumbMs;
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

constexpr uint32_t kLoopForensicsMagic = 0x57444631UL;
// Bumped 1 -> 2 for the addition of sleepPrepSpanStartMillis (see
// WO-2026-08-11-001 Second Corrective Pass). This struct is `retained` RAM,
// so a stale smaller layout from before this change must not be misread as
// the new layout - the version check below forces a reinit on mismatch.
constexpr uint8_t kLoopForensicsVersion = 2;
constexpr unsigned long kLoopStageWarnThresholdMs = 2000UL;
constexpr unsigned long kLoopStageErrorThresholdMs = 10000UL;
constexpr unsigned long kLoopForensicsSnapshotIntervalMs = 1000UL;

unsigned long lastLoopForensicsSnapshotMs = 0;

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

void noteLoopStageDuration(bool sampleQueueDepth) {
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
void awakeWatchdogExpiredHandler() {
  setAppBreadcrumb(BREADCRUMB_APP_WATCHDOG_RESET);
}
#else
// ApplicationWatchdog expects a plain function pointer.
void appWatchdogHandler() {
  setAppBreadcrumb(BREADCRUMB_APP_WATCHDOG_RESET);
  System.reset(RESET_CAUSE_APP_WATCHDOG);
}
#endif
