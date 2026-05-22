/**
 * @file WakeCycleStats.cpp
 * @brief Wake-cycle observability implementation.
 */

#include "observability/WakeCycleStats.h"

namespace Observability {

static WakeCycleStats g_cycleStats;

WakeCycleStats &cycleStats() {
  return g_cycleStats;
}

void WakeCycleStats::resetOnWake(uint32_t nowMs) {
  *this = WakeCycleStats();
  wake_start_ms = nowMs;
}

void WakeCycleStats::markConnectAttempt(ConnectAttemptType type, uint32_t budgetMs, uint16_t queueDepthBefore) {
  connect_attempt_type = type;
  connect_budget_ms = budgetMs;
  publish_queue_depth_before_connect = queueDepthBefore;
}

void WakeCycleStats::markConnectRequested(uint32_t startMs) {
  if (connect_start_ms == 0) {
    connect_start_ms = startMs;
  }
}

void WakeCycleStats::markConnectSuccess(uint32_t durationMs, uint16_t queueDepthAfter, time_t lastSuccessEpoch) {
  connect_duration_ms = durationMs;
  connect_result = ConnectResult::SUCCESS;
  publish_queue_depth_after_connect = queueDepthAfter;
  last_success_epoch = lastSuccessEpoch;
}

void WakeCycleStats::markConnectTimeout(uint32_t durationMs) {
  connect_duration_ms = durationMs;
  connect_result = ConnectResult::TIMEOUT;
}

void WakeCycleStats::markServiceStart(uint32_t startMs) {
  if (service_start_ms == 0) {
    service_start_ms = startMs;
  }
}

void WakeCycleStats::markServiceEnd(uint32_t endMs) {
  if (service_start_ms != 0 && service_duration_ms == 0) {
    service_duration_ms = (endMs >= service_start_ms) ? (endMs - service_start_ms) : 0;
  }
}

void WakeCycleStats::markTeardownStart(uint32_t startMs, bool standby) {
  if (teardown_start_ms == 0) {
    teardown_start_ms = startMs;
    use_network_standby = standby;
  }
}

void WakeCycleStats::markTeardownEnd(uint32_t endMs) {
  if (teardown_start_ms != 0 && teardown_duration_ms == 0) {
    teardown_duration_ms = (endMs >= teardown_start_ms) ? (endMs - teardown_start_ms) : 0;
  }
}

void WakeCycleStats::finalizeBeforeSleep(uint32_t nowMs,
                                        bool cloudConnected,
                                        bool radioOn,
                                        uint16_t queueDepthBeforeSleep,
                                        uint16_t batterySocTenths,
                                        uint8_t charging,
                                        time_t lastSuccessEpoch) {
  cloud_connected_before_sleep = cloudConnected;
  radio_on_before_sleep = radioOn;
  publish_queue_depth_before_sleep = queueDepthBeforeSleep;
  battery_soc_tenths = batterySocTenths;
  is_charging = charging;
  if (lastSuccessEpoch != 0) {
    last_success_epoch = lastSuccessEpoch;
  }

  if (wake_start_ms != 0) {
    total_awake_ms = (nowMs >= wake_start_ms) ? (nowMs - wake_start_ms) : 0;
  }
}

const char *toString(WakeCycleStats::ConnectAttemptType v) {
  switch (v) {
  case WakeCycleStats::ConnectAttemptType::NONE:
    return "none";
  case WakeCycleStats::ConnectAttemptType::NORMAL:
    return "normal";
  case WakeCycleStats::ConnectAttemptType::DEEP:
    return "deep";
  default:
    return "?";
  }
}

const char *toString(WakeCycleStats::ConnectResult v) {
  switch (v) {
  case WakeCycleStats::ConnectResult::NOT_ATTEMPTED:
    return "na";
  case WakeCycleStats::ConnectResult::SUCCESS:
    return "ok";
  case WakeCycleStats::ConnectResult::TIMEOUT:
    return "timeout";
  case WakeCycleStats::ConnectResult::ABORTED:
    return "aborted";
  default:
    return "?";
  }
}

} // namespace Observability
