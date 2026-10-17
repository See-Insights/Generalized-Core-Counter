#pragma once
#include "../state/State_Common.h"

enum AwakeWatchdogSleepStrategy : uint8_t {
  AWAKE_WATCHDOG_SLEEP_CONFIG_PAUSE = 0,
  AWAKE_WATCHDOG_SLEEP_MANUAL_STOP = 1,
};

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
  // 9 was BREADCRUMB_CONNECTIVITY_FAILSAFE (stage 1, retired WO-2026-10-02-001 item B)
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

extern retained uint8_t appBreadcrumb;
extern retained uint32_t appBreadcrumbMs;
extern AwakeWatchdogSleepStrategy awakeWatchdogSleepStrategy;
extern bool awakeWatchdogInitialized;
extern bool awakeWatchdogStarted;
const char *appBreadcrumbName(uint8_t code);
const char *awakeWatchdogSleepStrategyName(AwakeWatchdogSleepStrategy strategy);
const char *loopStageName(LoopStage stage);
const char *loopStageForensicName(LoopStage stage);
void ensureRetainedLoopForensicsInitialized();
void noteLoopStageDuration(bool sampleQueueDepth = true);
void awakeWatchdogExpiredHandler();
void appWatchdogHandler();
