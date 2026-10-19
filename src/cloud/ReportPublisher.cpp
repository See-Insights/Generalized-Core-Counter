#include "cloud/ReportPublisher.h"
#include "cloud/Cloud.h"
#include "observability/AwakeCycleCounters.h"
#include "observability/LoopForensics.h"
#include "PublishQueuePosixRK.h"
#include "sensors/SensorManager.h"
#include "state/StateMachine.h"
#include "state/State_Common.h"

namespace {
constexpr unsigned long REPORT_FORENSICS_SLOW_LOG_THRESHOLD_MS = 250UL;
constexpr unsigned long REPORT_FORENSICS_ABNORMAL_WARN_THRESHOLD_MS = 1000UL;
} // namespace

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
  case 19:
  case 31:
  case 41:
  case 42:
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
void publishData(time_t stampOverride) {
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

  uint8_t sensorMode = SystemConfig::get_sensorMode();
  const int8_t reportedAlertCode = RecoveryState::get_alertCode();

  // Ensure battery value is always a finite number for webhook ingestion.
  // Ubidots rejects NaN/Inf with a 400 response.
  float stateOfCharge = PowerManager::instance().soc();
  if (!(stateOfCharge == stateOfCharge) || stateOfCharge < 0.0f || stateOfCharge > 100.0f) {
    stateOfCharge = 0.0f;
    battState = 0;
  }

  // WO-2026-10-02-001 items D/E: heap and awake-cycle visibility. Every new
  // field is an unquoted JSON number (Ubidots auto-creates a variable per new
  // top-level key, and a quoted value would create a text variable).
  runtime_info_t rtInfo = {};
  rtInfo.size = sizeof(rtInfo);
  HAL_Core_Runtime_Info(&rtInfo, nullptr);

  // WO-2026-10-04-001 item D: cell voltage, 0.00 when no plausible sample.
  float vc = 0.0f;
  (void)SensorManager::instance().cachedBatteryVoltage(vc);

  // The active sensor renders its own leading fields; everything else is common.
  char sensorFields[48];
  SensorManager::instance().activeSensor()->reportFields(sensorFields, sizeof(sensorFields), sensorMode);

  // Occupancy keeps alerts before resets and the current/override timestamp; counting
  // (and MEASUREMENT) keeps resets before alerts and the end-of-previous-hour timestamp.
  const bool occupancyMode = sensorMode == SystemConfig::OCCUPANCY;
  const unsigned long timeStampValue = occupancyMode ? (stampOverride != 0 ? (unsigned long)stampOverride : nowStampSec) : endOfPrevHourStampSec;
  snprintf(data, sizeof(data),
           "{%s\"battery\":%4.2f,\"vc\":%.2f,\"key1\":\"%s\",\"temp\":%4.2f,\"%s\":%i,\"%s\":%i,\"connecttime\":%i,\"fh\":%lu,\"lfb\":%lu,\"cyc\":%lu,\"slp\":%lu,\"timestamp\":%lu000}",
           sensorFields,
           stateOfCharge,
           (double)vc,
           batteryContext[battState],
           CurrentReadings::get_internalTempC(),
           occupancyMode ? "alerts" : "resets",
           occupancyMode ? reportedAlertCode : RecoveryState::get_resetCount(),
           occupancyMode ? "resets" : "alerts",
           occupancyMode ? RecoveryState::get_resetCount() : reportedAlertCode,
           SystemConfig::get_lastConnectionDuration(),
           (unsigned long)System.freeMemory(), (unsigned long)rtInfo.largest_free_block_heap,
           (unsigned long)AwakeCycles::cycles, (unsigned long)AwakeCycles::sleeps,
           timeStampValue);

  // Get webhook name from cloud configuration (with fallback to convention)
  const char *webhookName = Cloud::instance().getWebhookName();

  setAppBreadcrumb(BREADCRUMB_REPORT_QUEUE_START);
  const unsigned long queueStartMs = millis();
  bool queued = PublishQueuePosix::instance().publish(webhookName, data, PRIVATE | WITH_ACK);
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
      RecoveryState::get_alertCode() == reportedAlertCode) {
    if (SystemConfig::get_verboseMode()) {
      Log.info("Clearing alert %d after queueing report payload", (int)reportedAlertCode);
    }
    RecoveryState::set_alertCode(0);
    RecoveryState::set_lastAlertTime(0);
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
    RecoveryState::raiseAlert(42);
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

  if (sensorMode == SystemConfig::OCCUPANCY) {
    Log.info("Report: occ=%d totalMin=%lu alert=%d ok=%d ledger=%s",
             CurrentReadings::get_occupied() ? 1 : 0,
             (unsigned long)(CurrentReadings::get_totalOccupiedSeconds() / 60UL),
             (int)reportedAlertCode,
             queued ? 1 : 0,
             ledgerState);
  } else {
    Log.info("Report: hourly=%d daily=%d alert=%d ok=%d ledger=%s",
             (int)CurrentReadings::get_hourlyCount(),
             (int)CurrentReadings::get_dailyCount(),
             (int)reportedAlertCode,
             queued ? 1 : 0,
             ledgerState);
  }

  setAppBreadcrumb(BREADCRUMB_REPORT_EXIT);
}

/**
 * @brief Publishes a forensic snapshot of the hibernate-wake gate outcome
 *        (WO-2026-08-29-001), mirroring publishWatchdogForensics() above.
 *
 * @details The gate in setup() (fed by `HibernateCycle::classifyWake()`,
 * evaluated when `wakeVerdict.hibernateGateEvaluated` is true) only ever
 * made its SUCCESS outcome cloud-visible (via the status payload's
 * hibernate fields). Its FAILURE outcome - the interesting case for
 * diagnosing an oversleep/undersleep or a wake that never qualified -
 * previously reached only a `Log.info()` line emitted before USB CDC
 * re-enumerates, which the Pi serial forwarder structurally cannot capture
 * (see the Work Order). This is a SEPARATE queued event from the status
 * payload deliberately, so it does not compete with WO-2026-08-29-002's
 * status payload byte budget and reaches every fleet device (not just the
 * bench units with a serial forwarder attached), the same way the
 * publish-queue-backed "watchdog" event above does. Called only when a
 * hibernate wake was pending this boot, before `HibernateCycle::abandon()`.
 *
 * @param fields Gate-outcome fields already classified by
 *        `HibernateCycle::classifyWake()` and assembled by
 *        `HibernateWakeDiagnostics::buildEventFields()` at the call site;
 *        this function only formats and publishes them, it does not
 *        evaluate the gate itself.
 */
void publishHibernateWakeForensics(const HibernateWakeDiagnostics::EventFields &fields) {
  char payload[256];
  const int written = HibernateWakeDiagnostics::buildEventPayload(payload, sizeof(payload), fields);
  if (written < 0 || (size_t)written >= sizeof(payload)) {
    Log.warn("HibernateWake forensic payload truncated or failed (written=%d)", written);
    return;
  }

  if (!PublishQueuePosix::instance().publish("hibernate_wake", payload, PRIVATE | WITH_ACK)) {
    Log.warn("HibernateWake forensic event publish() returned false (queue full or not ready)");
  }
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
    // WO-2026-08-29-002 item 8: deliberately NOT gated on isClockTrusted().
    // Both consumers (State_Report.cpp's `lastHook != 0` age-based alert-40
    // escalation gate, and State_Error.cpp's `lastHook == 0` "no recorded
    // response yet - defer corrective action" check) treat 0 as "never
    // happened" - a control-flow branch, not just a recorded value. A real
    // webhook response arriving while the clock is untrusted would, if
    // written as 0, make both consumers behave as though the integration
    // path had never succeeded, suppressing/skewing the age-based
    // escalation and corrective-reset logic. Writing Time.now() even when
    // untrusted preserves existing behavior; see the Implementation Report
    // for the Chief Engineer's review.
    SystemConfig::set_lastHookResponse(Time.now());

    if (session.awaitingWebhookResponse) {
      session.awaitingWebhookResponse = false;
      session.webhookAwaitStartMs = 0;
    }

    // Clear webhook supervision alert (40) on any response.
    if (RecoveryState::get_alertCode() == 40) {
      RecoveryState::set_alertCode(0);
      RecoveryState::set_lastAlertTime(0);
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
  if (SystemConfig::get_verboseMode() && Particle.connected() && PublishQueuePosix::instance().getCanSleep()) {
    publishDiagnosticSafe("Ubidots Hook", responseString, PRIVATE);
  }
  if (SystemConfig::get_verboseMode() && responseOk) {
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
  PublishQueuePosix::instance().publish(eventName, data, flags | WITH_ACK);
  return true;
}
