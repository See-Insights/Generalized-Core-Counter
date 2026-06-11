/**
 * @file WakeCycleStats.h
 * @brief Minimal, non-blocking wake-cycle observability data.
 */

#pragma once

#include "Particle.h"

namespace Observability {

/**
 * @brief Minimal, non-blocking per-wake-cycle diagnostics.
 *
 * @details
 * This struct is designed for field diagnostics on low-power devices:
 * - No blocking waits
 * - No added delays
 * - One compact log line per wake cycle (emitted by the sleep state)
 * - Optional piggyback into existing cloud payloads only when already publishing
 */
struct WakeCycleStats {
  /**
   * @brief Type of connect attempt for this wake cycle.
   */
  enum class ConnectAttemptType : uint8_t {
    NONE = 0,
    NORMAL = 1,
    DEEP = 2,
  };

  /**
   * @brief Outcome of the connect attempt.
   */
  enum class ConnectResult : int8_t {
    NOT_ATTEMPTED = -1,
    SUCCESS = 0,
    TIMEOUT = 1,
    ABORTED = 2,
  };

  /// Timestamps (millis) and durations (ms) are within the current wake cycle.
  uint32_t wake_start_ms = 0;

  ConnectAttemptType connect_attempt_type = ConnectAttemptType::NONE;
  uint32_t connect_budget_ms = 0;
  uint32_t connect_start_ms = 0;
  uint32_t connect_duration_ms = 0;
  ConnectResult connect_result = ConnectResult::NOT_ATTEMPTED;

  uint32_t service_start_ms = 0;
  uint32_t service_duration_ms = 0;

  uint32_t teardown_start_ms = 0;
  uint32_t teardown_duration_ms = 0;

  uint32_t total_awake_ms = 0;

  /// Cheap health context.
  bool use_network_standby = false;
  bool radio_on_before_sleep = false;
  bool cloud_connected_before_sleep = false;

  /// Battery/power context (captured from existing measurements where possible).
  /// battery_soc_tenths: 0..1000 means 0.0..100.0%.
  uint16_t battery_soc_tenths = 0xFFFF;
  /// is_charging: 1=true, 0=false, 0xFF=unknown.
  uint8_t is_charging = 0xFF;

  /// Stale SOC detection diagnostics (Phase 1: detection and instrumentation only)
  uint16_t battery_vcell_mv = 0xFFFF;        // Battery voltage in millivolts (0xFFFF = unknown)
  uint8_t pmic_charge_status = 0xFF;         // PMIC charge status (0-3, 0xFF = unknown/NA)
  uint8_t pmic_vbus_status = 0xFF;           // PMIC VBUS status (0-3, 0xFF = unknown/NA)
  uint8_t pmic_power_source = 0xFF;          // Power source (0xFF = unknown)
  uint8_t pmic_power_good = 0xFF;            // Power good flag (1=true, 0=false, 0xFF = unknown)
  uint8_t pmic_fault_reg = 0xFF;             // PMIC fault register (0xFF = not read/NA)
  bool stale_soc_suspected = false;          // True if stale SOC detected this cycle
  uint16_t stale_soc_total_count = 0xFFFF;   // Cumulative stale SOC count (0xFFFF = not tracked)

  /// Publish queue depth (events).
  uint16_t publish_queue_depth_before_connect = 0xFFFF;
  uint16_t publish_queue_depth_after_connect = 0xFFFF;
  uint16_t publish_queue_depth_before_sleep = 0xFFFF;

  /// Existing long-term success markers (if already tracked elsewhere).
  time_t last_success_epoch = 0;

  /**
   * @brief Reset all fields for a new wake cycle.
   * @param nowMs Current millis().
   */
  void resetOnWake(uint32_t nowMs);

  /**
   * @brief Record the type/budget of the connect attempt and queue depth at start.
   */
  void markConnectAttempt(ConnectAttemptType type, uint32_t budgetMs, uint16_t queueDepthBefore);
  /**
   * @brief Record when Particle.connect() is requested.
   */
  void markConnectRequested(uint32_t startMs);
  /**
   * @brief Record successful connection and queue depth after connect.
   */
  void markConnectSuccess(uint32_t durationMs, uint16_t queueDepthAfter, time_t lastSuccessEpoch);
  /**
   * @brief Record connect timeout duration.
   */
  void markConnectTimeout(uint32_t durationMs);

  /**
   * @brief Mark the beginning of the service window.
   */
  void markServiceStart(uint32_t startMs);
  /**
   * @brief Mark the end of the service window.
   */
  void markServiceEnd(uint32_t endMs);

  /**
   * @brief Mark teardown start.
   */
  void markTeardownStart(uint32_t startMs, bool useNetworkStandby);
  /**
   * @brief Mark teardown end.
   */
  void markTeardownEnd(uint32_t endMs);

  /**
   * @brief Finalize cycle stats right before sleep.
   */
  void finalizeBeforeSleep(uint32_t nowMs,
                           bool cloudConnected,
                           bool radioOn,
                           uint16_t queueDepthBeforeSleep,
                           uint16_t batterySocTenths,
                           uint8_t isCharging,
                           time_t lastSuccessEpoch);
};

/**
 * @brief Single, static instance for the whole application.
 */
WakeCycleStats &cycleStats();

/**
 * @brief Convert connect attempt type to short string.
 */
const char *toString(WakeCycleStats::ConnectAttemptType v);
/**
 * @brief Convert connect result to short string.
 */
const char *toString(WakeCycleStats::ConnectResult v);

} // namespace Observability
