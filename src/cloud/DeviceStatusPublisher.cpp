#include "../Config.h"
#include "cloud/Cloud.h"
#include "observability/WakeCycleStats.h"
#include "sensors/SensorManager.h"

// External firmware version string (defined in Version.cpp)
extern const char* FIRMWARE_VERSION;

namespace {

bool ledgerHasUnsyncedWriteForDiag(const Ledger &ledger) {
    const int64_t lastUpdatedMs = ledger.lastUpdated();
    if (lastUpdatedMs <= 0) {
        return false;
    }

    const int64_t lastSyncedMs = ledger.lastSynced();
    return lastSyncedMs <= 0 || lastSyncedMs < lastUpdatedMs;
}

} // namespace

bool Cloud::writeDeviceStatusToCloud(const char *source) {
    const char *issueSource = (source && source[0] != '\0') ? source : "Unknown";
#if !ENABLE_LEDGER_TRACE
    (void)issueSource;
#endif

    auto pendingLedgerOpsForDiag = [&]() -> uint16_t {
        uint16_t pending = 0;
        if (pendingStatusPublish) {
            pending++;
        }
        if (pendingDeviceStatusSync || ledgerHasUnsyncedWriteForDiag(deviceStatusLedger)) {
            pending++;
        }
        if (pendingDeviceDataSync || ledgerHasUnsyncedWriteForDiag(deviceDataLedger)) {
            pending++;
        }
        return pending;
    };

    auto cellularReadyForDiag = [&]() -> int {
#if Wiring_Cellular
        return Cellular.ready() ? 1 : 0;
#else
        return 0;
#endif
    };
#if !ENABLE_LEDGER_TRACE
    (void)pendingLedgerOpsForDiag;
    (void)cellularReadyForDiag;
#endif

    // Build current configuration as JSON (base status).
    // IMPORTANT: This base JSON is used for change detection so we do not
    // increase cloud writes in the field.
    char bufferBase[512];
    JSONBufferWriter writerBase(bufferBase, sizeof(bufferBase));

    writerBase.beginObject();

    // Firmware version
    writerBase.name("firmwareVersion").value(FIRMWARE_VERSION);

    // Messaging
    writerBase.name("messaging").beginObject();
    writerBase.name("serial").value(sysStatus.get_serialConnected());
    writerBase.name("verboseMode").value(sysStatus.get_verboseMode());
    writerBase.name("verboseTimeoutMin").value(sysStatus.get_verboseTimeoutMin());
    writerBase.endObject();

    // Sensor
    writerBase.name("sensor").beginObject();
    writerBase.name("type").value(sensorConfig.get_sensorType());
    writerBase.name("setting1").value((int)sensorConfig.get_sensorSetting1());
    writerBase.name("setting2").value((int)sensorConfig.get_sensorSetting2());
    writerBase.name("setting3").value((int)sensorConfig.get_sensorSetting3());
    writerBase.name("setting4").value((int)sensorConfig.get_sensorSetting4());
    writerBase.endObject();

    // Timing
    writerBase.name("timing").beginObject();
    writerBase.name("timezone").value(sysStatus.get_timeZoneStrCStr());
    writerBase.name("reportingIntervalSec").value(sysStatus.get_reportingInterval());
    writerBase.name("openHour").value(sysStatus.get_openTime());
    writerBase.name("closeHour").value(sysStatus.get_closeTime());
    writerBase.name("connectAttemptBudgetSec").value((int)sysStatus.get_connectAttemptBudgetSec());
    writerBase.endObject();

    // Modes
    writerBase.name("modes").beginObject();
    writerBase.name("sensorMode").value((int)sysStatus.get_sensorMode());
    writerBase.name("connectionMode").value((int)sysStatus.get_connectionMode());
    writerBase.name("reportingMode").value((int)sysStatus.get_reportingMode());
    writerBase.name("samplingMode").value((int)sysStatus.get_samplingMode());
    writerBase.name("enableHibernateSleep").value(sysStatus.get_enableHibernateSleep());
    writerBase.endObject();
    writerBase.endObject();

    if (!writerBase.buffer()) {
        Log.warn("Failed to create status JSON");
        return false;
    }

    bufferBase[writerBase.dataSize()] = '\0';

    // Only publish if the configuration actually changed
    if (strcmp(lastPublishedStatus, bufferBase) == 0) {
        return true; // Not an error; nothing to do
    }

    // Publish payload: base status + optional per-cycle diagnostics.
    // This does NOT affect change detection, so it won't increase publish rate.
    char bufferPublish[896];
    JSONBufferWriter writer(bufferPublish, sizeof(bufferPublish));
    writer.beginObject();

    // Repeat base status fields.
    writer.name("firmwareVersion").value(FIRMWARE_VERSION);

    writer.name("messaging").beginObject();
    writer.name("serial").value(sysStatus.get_serialConnected());
    writer.name("verboseMode").value(sysStatus.get_verboseMode());
    writer.name("verboseTimeoutMin").value(sysStatus.get_verboseTimeoutMin());
    writer.endObject();

    writer.name("sensor").beginObject();
    writer.name("type").value(sensorConfig.get_sensorType());
    writer.name("setting1").value((int)sensorConfig.get_sensorSetting1());
    writer.name("setting2").value((int)sensorConfig.get_sensorSetting2());
    writer.name("setting3").value((int)sensorConfig.get_sensorSetting3());
    writer.name("setting4").value((int)sensorConfig.get_sensorSetting4());
    writer.endObject();

    writer.name("timing").beginObject();
    writer.name("timezone").value(sysStatus.get_timeZoneStrCStr());
    writer.name("reportingIntervalSec").value(sysStatus.get_reportingInterval());
    writer.name("openHour").value(sysStatus.get_openTime());
    writer.name("closeHour").value(sysStatus.get_closeTime());
    writer.name("connectAttemptBudgetSec").value((int)sysStatus.get_connectAttemptBudgetSec());
    writer.endObject();

    writer.name("modes").beginObject();
    writer.name("sensorMode").value((int)sysStatus.get_sensorMode());
    writer.name("connectionMode").value((int)sysStatus.get_connectionMode());
    writer.name("reportingMode").value((int)sysStatus.get_reportingMode());
    writer.name("samplingMode").value((int)sysStatus.get_samplingMode());
    writer.name("enableHibernateSleep").value(sysStatus.get_enableHibernateSleep());
    writer.endObject();

#if defined(ENABLE_PMIC_FORENSICS) && ENABLE_PMIC_FORENSICS
    writer.name("pmicAnomalyCount").value((int)pmicAnomalyCount);
    writer.name("lastPmicAnomalySoc").value((double)lastPmicAnomalySoc, 2);
    writer.name("lastPmicAnomalyChargeStatus").value((int)lastPmicAnomalyChargeStatus);
    writer.name("lastPmicAnomalyAgeSec").value((unsigned long)pmicAnomalyAgeSec());
    writer.name("lastPmicAnomalyPowerSource").value((int)lastPmicAnomalyPowerSource);
    writer.name("lastPmicAnomalyVbusStatus").value((int)lastPmicAnomalyVbusStatus);
    writer.name("pmicAnomalyActive").value(pmicAnomalyActive ? 1 : 0);
#endif

    // Optional: piggyback last completed wake-cycle stats for field diagnostics.
    // This is only included when we are already publishing device-status (config changed).
    {
        const auto &cs = Observability::cycleStats();
        writer.name("cycle").beginObject();
        writer.name("awakeMs").value((int)cs.total_awake_ms);
        writer.name("connectType").value(Observability::toString(cs.connect_attempt_type));
        writer.name("connectResult").value(Observability::toString(cs.connect_result));
        writer.name("connectMs").value((int)cs.connect_duration_ms);
        writer.name("serviceMs").value((int)cs.service_duration_ms);
        writer.name("teardownMs").value((int)cs.teardown_duration_ms);
        writer.name("qBefore").value(cs.publish_queue_depth_before_connect == 0xFFFF ? -1 : (int)cs.publish_queue_depth_before_connect);
        writer.name("qAfter").value(cs.publish_queue_depth_after_connect == 0xFFFF ? -1 : (int)cs.publish_queue_depth_after_connect);
        writer.name("qSleep").value(cs.publish_queue_depth_before_sleep == 0xFFFF ? -1 : (int)cs.publish_queue_depth_before_sleep);
        writer.name("socTenths").value(cs.battery_soc_tenths == 0xFFFF ? -1 : (int)cs.battery_soc_tenths);
        writer.name("charging").value(cs.is_charging == 0xFF ? -1 : (int)cs.is_charging);
        writer.name("lastOk").value((int)cs.last_success_epoch);
        writer.endObject();
    }

    writer.endObject();

    if (!writer.buffer()) {
        Log.warn("Failed to create status publish JSON");
        return false;
    }
    bufferPublish[writer.dataSize()] = '\0';

    LedgerData data = LedgerData::fromJSON(bufferPublish);

    const unsigned long startMs = millis();
#if ENABLE_LEDGER_TRACE
    Log.info("LedgerDiag: start pending=%u cloud=%d cell=%d heap=%lu",
             (unsigned)pendingLedgerOpsForDiag(),
             Particle.connected() ? 1 : 0,
             cellularReadyForDiag(),
             (unsigned long)System.freeMemory());
#endif

    const Cloud::LedgerSyncDiagnostics attemptDiagnostics = Cloud::instance().ledgerSyncDiagnostics();
    const bool trackedBefore = Cloud::instance().isLedgerPointerTracked(&deviceStatusLedger);
#if ENABLE_LEDGER_TRACE
    Log.info("LedgerIssueAttempt: source=%s ptr=%p tracked=%d syncing=%d inflight=%d",
             issueSource,
             &deviceStatusLedger,
             trackedBefore ? 1 : 0,
             attemptDiagnostics.syncing ? 1 : 0,
             attemptDiagnostics.inflight ? 1 : 0);
#else
    (void)trackedBefore;
    (void)attemptDiagnostics;
#endif

    const uint32_t requestSeq = Cloud::instance().noteLedgerSyncRequest(
        Cloud::LEDGER_REQUEST_KIND_STATUS,
        source,
        &deviceStatusLedger);

    if (requestSeq == 0) {
        const Cloud::LedgerSyncDiagnostics resultDiagnostics = Cloud::instance().ledgerSyncDiagnostics();
#if ENABLE_LEDGER_TRACE
        Log.info("LedgerIssueResult: source=%s ptr=%p createdTracker=%d pending=%u syncing=%d inflight=%d",
                 issueSource,
                 &deviceStatusLedger,
                 0,
                 (unsigned)resultDiagnostics.pendingCount,
                 resultDiagnostics.syncing ? 1 : 0,
                 resultDiagnostics.inflight ? 1 : 0);
#else
        (void)resultDiagnostics;
#endif
        // Keep pendingStatusPublish set so the deferred status update can retry
        // once the in-flight ledger write has completed.
        return false;
    }

    int result = deviceStatusLedger.set(data);

    const Cloud::LedgerSyncDiagnostics resultDiagnostics = Cloud::instance().ledgerSyncDiagnostics();
#if ENABLE_LEDGER_TRACE
    Log.info("LedgerIssueResult: source=%s ptr=%p createdTracker=%d pending=%u syncing=%d inflight=%d",
             issueSource,
             &deviceStatusLedger,
             requestSeq ? 1 : 0,
             (unsigned)resultDiagnostics.pendingCount,
             resultDiagnostics.syncing ? 1 : 0,
             resultDiagnostics.inflight ? 1 : 0);
#else
    (void)resultDiagnostics;
#endif

    const unsigned long elapsedMs = millis() - startMs;
#if !ENABLE_LEDGER_TRACE
    (void)elapsedMs;
#endif

    if (result == SYSTEM_ERROR_NONE) {
        pendingDeviceStatusSync = true;
#if ENABLE_LEDGER_TRACE
        Log.info("LedgerDiag: success elapsed=%lu pending=%u heap=%lu",
                 elapsedMs,
                 (unsigned)pendingLedgerOpsForDiag(),
                 (unsigned long)System.freeMemory());
#endif
        // Preserve base-status change detection so per-cycle fields don't
        // force additional device-status publishes.
        strncpy(lastPublishedStatus, bufferBase, sizeof(lastPublishedStatus) - 1);
        lastPublishedStatus[sizeof(lastPublishedStatus) - 1] = '\0';
        return true;
    } else {
        Cloud::instance().noteLedgerSyncFail(requestSeq, result);
#if ENABLE_LEDGER_TRACE
        Log.warn("LedgerDiag: fail rc=%d elapsed=%lu pending=%u cloud=%d cell=%d heap=%lu",
                 result,
                 elapsedMs,
                 (unsigned)pendingLedgerOpsForDiag(),
                 Particle.connected() ? 1 : 0,
                 cellularReadyForDiag(),
                 (unsigned long)System.freeMemory());
#endif
        Log.warn("Failed to publish device status: %d", result);
        return false;
    }
}

bool Cloud::publishDataToLedger(const char *source) {
    const char *issueSource = (source && source[0] != '\0') ? source : "Unknown";
#if !ENABLE_LEDGER_TRACE
    (void)issueSource;
#endif

    auto pendingLedgerOpsForDiag = [&]() -> uint16_t {
        uint16_t pending = 0;
        if (pendingStatusPublish) {
            pending++;
        }
        if (pendingDeviceStatusSync || ledgerHasUnsyncedWriteForDiag(deviceStatusLedger)) {
            pending++;
        }
        if (pendingDeviceDataSync || ledgerHasUnsyncedWriteForDiag(deviceDataLedger)) {
            pending++;
        }
        return pending;
    };

    auto cellularReadyForDiag = [&]() -> int {
#if Wiring_Cellular
        return Cellular.ready() ? 1 : 0;
#else
        return 0;
#endif
    };
#if !ENABLE_LEDGER_TRACE
    (void)pendingLedgerOpsForDiag;
    (void)cellularReadyForDiag;
#endif

    char buffer[512];
    JSONBufferWriter writer(buffer, sizeof(buffer));
    const unsigned long freeHeap = System.freeMemory();
    
    writer.beginObject();
    writer.name("timestamp").value((int)Time.now());

    // Boot/wake diagnostics: included here so it is visible in Console even
    // when early USB logs are missed after HIBERNATE/cold boot.
    writer.name("resetReason").value((int)System.resetReason());
    writer.name("resetReasonData").value((unsigned long)System.resetReasonData());

    uint8_t sensorMode = sysStatus.get_sensorMode();

    if (sensorMode == COUNTING) {
        writer.name("mode").value("counting");
        writer.name("hourlyCount").value(current.get_hourlyCount());
        writer.name("dailyCount").value(current.get_dailyCount());
    } else if (sensorMode == OCCUPANCY) {
        writer.name("mode").value("occupancy");
        writer.name("occupied").value(current.get_occupied());
        // Report in whole minutes for external consumers (storage stays in seconds).
        const unsigned long totalOccupiedMinutes = (unsigned long)(current.get_totalOccupiedSeconds() / 60UL);
        writer.name("totalOccupiedSec").value(totalOccupiedMinutes);
    } else { // MEASUREMENT or any future modes
        writer.name("mode").value("measurement");
        // In measurement mode we still track counts for compatibility
        writer.name("hourlyCount").value(current.get_hourlyCount());
        writer.name("dailyCount").value(current.get_dailyCount());
    }
    
    writer.name("battery").value(current.get_stateOfCharge(), 1);
    writer.name("temp").value(current.get_internalTempC(), 1);
    writer.name("freeHeap").value((int)freeHeap);
    writer.endObject();
    
    if (!writer.buffer()) {
        Log.warn("Failed to create data JSON");
        return false;
    }
    
    buffer[writer.dataSize()] = '\0';
    
    LedgerData data = LedgerData::fromJSON(buffer);

    const unsigned long startMs = millis();
#if ENABLE_LEDGER_TRACE
    Log.info("LedgerDiag: start pending=%u cloud=%d cell=%d heap=%lu",
             (unsigned)pendingLedgerOpsForDiag(),
             Particle.connected() ? 1 : 0,
             cellularReadyForDiag(),
             (unsigned long)System.freeMemory());
#endif

    const Cloud::LedgerSyncDiagnostics attemptDiagnostics = Cloud::instance().ledgerSyncDiagnostics();
    const bool trackedBefore = Cloud::instance().isLedgerPointerTracked(&deviceDataLedger);
#if ENABLE_LEDGER_TRACE
    Log.info("LedgerIssueAttempt: source=%s ptr=%p tracked=%d syncing=%d inflight=%d",
             issueSource,
             &deviceDataLedger,
             trackedBefore ? 1 : 0,
             attemptDiagnostics.syncing ? 1 : 0,
             attemptDiagnostics.inflight ? 1 : 0);
#else
    (void)trackedBefore;
    (void)attemptDiagnostics;
#endif

    const uint32_t requestSeq = Cloud::instance().noteLedgerSyncRequest(
        Cloud::LEDGER_REQUEST_KIND_DATA,
        source,
        &deviceDataLedger);

    if (requestSeq == 0) {
        const Cloud::LedgerSyncDiagnostics resultDiagnostics = Cloud::instance().ledgerSyncDiagnostics();
#if ENABLE_LEDGER_TRACE
        Log.info("LedgerIssueResult: source=%s ptr=%p createdTracker=%d pending=%u syncing=%d inflight=%d",
                 issueSource,
                 &deviceDataLedger,
                 0,
                 (unsigned)resultDiagnostics.pendingCount,
                 resultDiagnostics.syncing ? 1 : 0,
                 resultDiagnostics.inflight ? 1 : 0);
#else
        (void)resultDiagnostics;
#endif
        return true;
    }

    int result = deviceDataLedger.set(data);

    const Cloud::LedgerSyncDiagnostics resultDiagnostics = Cloud::instance().ledgerSyncDiagnostics();
#if ENABLE_LEDGER_TRACE
    Log.info("LedgerIssueResult: source=%s ptr=%p createdTracker=%d pending=%u syncing=%d inflight=%d",
             issueSource,
             &deviceDataLedger,
             requestSeq ? 1 : 0,
             (unsigned)resultDiagnostics.pendingCount,
             resultDiagnostics.syncing ? 1 : 0,
             resultDiagnostics.inflight ? 1 : 0);
#else
    (void)resultDiagnostics;
#endif

    const unsigned long elapsedMs = millis() - startMs;
#if !ENABLE_LEDGER_TRACE
    (void)elapsedMs;
#endif
    
    if (result == SYSTEM_ERROR_NONE) {
        pendingDeviceDataSync = true;
#if ENABLE_LEDGER_TRACE
        Log.info("LedgerDiag: success elapsed=%lu pending=%u heap=%lu",
                 elapsedMs,
                 (unsigned)pendingLedgerOpsForDiag(),
                 (unsigned long)System.freeMemory());
#endif
        return true;
    } else {
        Cloud::instance().noteLedgerSyncFail(requestSeq, result);
#if ENABLE_LEDGER_TRACE
        Log.warn("LedgerDiag: fail rc=%d elapsed=%lu pending=%u cloud=%d cell=%d heap=%lu",
                 result,
                 elapsedMs,
                 (unsigned)pendingLedgerOpsForDiag(),
                 Particle.connected() ? 1 : 0,
                 cellularReadyForDiag(),
                 (unsigned long)System.freeMemory());
#endif
        Log.warn("Failed to publish sensor data: %d", result);
        return false;
    }
}

bool Cloud::hasNonDefaultConfig() {
    // Check if any current values differ from centralized factory defaults.
    return (sensorConfig.get_sensorType() != 1 || 
            sensorConfig.get_sensorSetting1() != Config::DEFAULT_OCCUPANCY_DEBOUNCE_MS ||
            strcmp(sysStatus.get_timeZoneStrCStr(), Config::DEFAULT_TIMEZONE) != 0 ||
            sysStatus.get_openTime() != Config::DEFAULT_OPEN_HOUR ||
            sysStatus.get_closeTime() != Config::DEFAULT_CLOSE_HOUR ||
            sysStatus.get_reportingInterval() != Config::DEFAULT_REPORT_INTERVAL_SEC);
}
