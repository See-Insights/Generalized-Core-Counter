#include "../Config.h"
#include "cloud/Cloud.h"
#include "observability/WakeCycleStats.h"
#include "observability/StartupSnapshotRuntime.h"
#include "power/PowerManager.h"
#include "sensors/SensorManager.h"
#include "state/StateMachine.h"
#include "reporting/ReportingPolicy.h"
#include "time/ClockTrust.h"

// WO-2026-08-29-002 item 8: forward-declared rather than pulled in via
// state/State_Common.h - that header transitively drags in retained-memory
// declarations and other app-wide state that some host test harnesses (see
// tests/power_source_override_test.sh) intentionally don't stub out for
// this translation unit.
bool isClockTrusted();
// Round 6 (Stage 7 finding 1): wrap-aware, sentinel-bearing sync age for
// telemetry - see Generalized-Core-Counter.cpp's reportedSyncAgeMs() doc
// comment for the full design (reuses the same wrap-generation tracking
// isClockTrusted() already uses; no parallel mechanism). This supersedes
// this file's former direct use of observedTimeSyncedLastMs() (Finding 2,
// Round 4 review) - reportedSyncAgeMs() calls it internally, so this file
// no longer needs its own forward declaration for it.
uint32_t reportedSyncAgeMs();

// External firmware version string (defined in Version.cpp)
extern const char* FIRMWARE_VERSION;

namespace {

constexpr int kLedgerSchemaVersion = 2;
constexpr size_t kDeviceDataPayloadCapacity = 512;

bool ledgerHasUnsyncedWriteForDiag(const Ledger &ledger) {
    const int64_t lastUpdatedMs = ledger.lastUpdated();
    if (lastUpdatedMs <= 0) {
        return false;
    }

    const int64_t lastSyncedMs = ledger.lastSynced();
    return lastSyncedMs <= 0 || lastSyncedMs < lastUpdatedMs;
}

void updateFnv1a(uint32_t &hash, const char *text) {
    if (!text) {
        text = "";
    }

    while (*text) {
        hash ^= (uint8_t)*text++;
        hash *= 16777619UL;
    }
}

void updateFnv1aField(uint32_t &hash, const char *name, const char *value) {
    updateFnv1a(hash, name);
    updateFnv1a(hash, "=");
    updateFnv1a(hash, value);
    updateFnv1a(hash, ";");
}

void updateFnv1aField(uint32_t &hash, const char *name, long value) {
    char valueText[16];
    snprintf(valueText, sizeof(valueText), "%ld", value);
    updateFnv1aField(hash, name, valueText);
}

void updateFnv1aField(uint32_t &hash, const char *name, unsigned long value) {
    char valueText[16];
    snprintf(valueText, sizeof(valueText), "%lu", value);
    updateFnv1aField(hash, name, valueText);
}

void formatConfigGeneration(char *buffer, size_t bufferSize) {
    uint32_t hash = 2166136261UL;

    updateFnv1aField(hash, "messaging.serial", sysStatus.get_serialConnected() ? 1L : 0L);
    updateFnv1aField(hash, "messaging.verboseMode", sysStatus.get_verboseMode() ? 1L : 0L);
    updateFnv1aField(hash, "messaging.verboseTimeoutMin", (unsigned long)sysStatus.get_verboseTimeoutMin());
    updateFnv1aField(hash, "sensor.type", (unsigned long)sensorConfig.get_sensorType());
    updateFnv1aField(hash, "sensor.setting1", (unsigned long)sensorConfig.get_sensorSetting1());
    updateFnv1aField(hash, "sensor.setting2", (unsigned long)sensorConfig.get_sensorSetting2());
    updateFnv1aField(hash, "sensor.setting3", (unsigned long)sensorConfig.get_sensorSetting3());
    updateFnv1aField(hash, "sensor.setting4", (unsigned long)sensorConfig.get_sensorSetting4());
    updateFnv1aField(hash, "timing.timezone", sysStatus.get_timeZoneStrCStr());
    updateFnv1aField(hash, "timing.reportingIntervalSec", (unsigned long)sysStatus.get_reportingInterval());
    updateFnv1aField(hash, "timing.openHour", (unsigned long)sysStatus.get_openTime());
    updateFnv1aField(hash, "timing.closeHour", (unsigned long)sysStatus.get_closeTime());
    updateFnv1aField(hash, "timing.connectAttemptBudgetSec", (unsigned long)sysStatus.get_connectAttemptBudgetSec());
    updateFnv1aField(hash, "modes.sensorMode", (unsigned long)sysStatus.get_sensorMode());
    updateFnv1aField(hash, "modes.connectionMode", (unsigned long)sysStatus.get_connectionMode());
    updateFnv1aField(hash, "modes.reportingMode", (unsigned long)sysStatus.get_reportingMode());
    updateFnv1aField(hash, "modes.samplingMode", (unsigned long)sysStatus.get_samplingMode());
    updateFnv1aField(hash, "modes.cloudDisconnectBudgetSec", (unsigned long)sysStatus.get_cloudDisconnectBudgetSec());
    updateFnv1aField(hash, "modes.modemOffBudgetSec", (unsigned long)sysStatus.get_modemOffBudgetSec());
    updateFnv1aField(hash, "modes.enableHibernateSleep", sysStatus.get_enableHibernateSleep() ? 1L : 0L);
    updateFnv1aField(hash, "webhook.name", sysStatus.get_webhookNameCStr());
    updateFnv1aField(hash, "webhook.enabled", sysStatus.get_webhookEnabled() ? 1L : 0L);
    updateFnv1aField(hash, "webhook.timeoutMs", (unsigned long)sysStatus.get_webhookTimeoutMs());

    snprintf(buffer, bufferSize, "%08lX", (unsigned long)hash);
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

    // Build current status contract JSON for duplicate suppression.
    char bufferBase[DEVICE_STATUS_PAYLOAD_CAPACITY];
    JSONBufferWriter writerBase(bufferBase, sizeof(bufferBase));
    char configGeneration[9];
    formatConfigGeneration(configGeneration, sizeof(configGeneration));
    const PowerReport &powerReport = PowerManager::instance().latestReport();
    const auto &cycleStats = Observability::cycleStats();
    float batteryVoltage = -1.0f;
    SensorManager::instance().cachedBatteryVoltage(batteryVoltage);
    const ReportingPolicy reportingPolicy = ReportingPolicyResolver::resolveRuntime(
        current.get_stateOfCharge(), Time.now());
    const Observability::StartupSnapshot &startup = Observability::currentStartupSnapshot();

    writerBase.beginObject();
    writerBase.name("schemaVersion").value(kLedgerSchemaVersion);
    writerBase.name("firmware").beginObject();
    writerBase.name("version").value(FIRMWARE_VERSION);
    writerBase.name("resetCount").value((int)sysStatus.get_resetCount());
    // WO-2026-08-24-001: compact bitmask witness of the build flags that were
    // ACTUALLY compiled in, derived from the same #if conditions that gate
    // each feature (see src/BuildProfile.h) so it cannot silently drift from
    // the shipped binary the way ENABLE_BORON_USB_SOURCE_OVERRIDE did. Bit
    // layout mirrors the "flags=" witness on the Boot: serial line.
    const uint16_t compiledBuildFlags =
        (DEV_BUILD ? 0x0001 : 0) |
        (ALLOW_BLOCKING_SERIAL_WAITS ? 0x0002 : 0) |
        (CONNECTIVITY_FAILSAFE_TEST_MODE ? 0x0004 : 0) |
        (ENABLE_PMIC_FORENSICS ? 0x0008 : 0) |
        (ENABLE_LEDGER_TRACE ? 0x0010 : 0) |
        (ENABLE_CONNECT_TRACE ? 0x0020 : 0) |
        (ENABLE_CONNECT_DECISION_TRACE ? 0x0040 : 0) |
        (ENABLE_PERF_TRACE ? 0x0080 : 0) |
        (ENABLE_GATE_TRACE ? 0x0100 : 0) |
        (ENABLE_SLEEP_TRACE ? 0x0200 : 0) |
        (ENABLE_PMIC_TRACE ? 0x0400 : 0) |
        (ENABLE_CONFIG_TRACE ? 0x0800 : 0) |
        (ENABLE_PMIC_CHARGE_CYCLE_TEST ? 0x1000 : 0) |
        (ENABLE_DIAGNOSTICS_PUBLISH_MODE ? 0x2000 : 0)
        // Addendum A (WO-2026-08-24-001): bit 0x4000 mirrors the SAME #if guard
        // that gates the USB source override in
        // PowerManager::refreshInputProfile() (src/power/PowerManager.cpp), so
        // this witness cannot drift from what actually gated the override in
        // this incident. Bit set => override compiled in.
        //
        // A CLEAR bit is EXPECTED and CORRECT on non-Boron platforms (P2,
        // Argon, MSOM) -- there is no USB source override on those platforms
        // and none is compiled. It is a defect signal ONLY on a Boron build.
#if defined(PLATFORM_ID) && defined(PLATFORM_BORON) && (PLATFORM_ID == PLATFORM_BORON)
        | 0x4000
#endif
        ;
    writerBase.name("flags").value((int)compiledBuildFlags);
    writerBase.endObject();
    writerBase.name("config").beginObject();
    writerBase.name("generation").value(configGeneration);
    writerBase.endObject();
    writerBase.name("reporting").beginObject();
    writerBase.name("lastReportEpoch").value((int)sysStatus.get_lastReport());
    writerBase.name("nextReportEpoch").value((int)reportingPolicy.nextReportEpoch);
    writerBase.name("configuredIntervalSec").value((unsigned long)reportingPolicy.configuredIntervalSec);
    writerBase.name("effectiveIntervalSec").value((unsigned long)reportingPolicy.effectiveIntervalSec);
    writerBase.name("adjustmentReason").value(
        ReportingPolicyResolver::adjustmentReasonName(reportingPolicy.adjustmentReason));
    writerBase.name("windowOpen").value(reportingPolicy.windowOpen);
    writerBase.endObject();
    writerBase.name("startup").beginObject();
    writerBase.name("epoch").value((int)startup.epoch);
    writerBase.name("reason").value(startup.reason);
    writerBase.name("firmware").value(startup.firmware);
    writerBase.name("deviceOS").value(startup.deviceOS);
    writerBase.name("resetCount").value((unsigned long)startup.resetCount);
    writerBase.endObject();
    writerBase.name("power").beginObject();
    writerBase.name("source").value(PowerManager::powerSourceLabel(powerReport.reading.powerSource));
    writerBase.name("profile").value(PowerManager::inputProfileLabel(powerReport.activeInputProfile));
    writerBase.name("overrideActive").value(powerReport.reading.overrideActive);
    writerBase.endObject();
    writerBase.name("battery").beginObject();
    writerBase.name("soc").value(PowerManager::instance().soc(), 1);
    writerBase.name("vcell").value(batteryVoltage, 2);
    writerBase.name("chargeState").value(SensorManager::instance().cachedChargeStateLabel());
    writerBase.endObject();
    writerBase.name("connection").beginObject();
    writerBase.name("lastResult").value(Observability::toString(cycleStats.connect_result));
    writerBase.name("elapsedMs").value((int)cycleStats.connect_duration_ms);
    writerBase.endObject();
    // Item 9 (WO-2026-08-29-002): clock trust verdict and sync recency, so
    // "was this cycle's timestamp trustworthy" is readable from telemetry
    // instead of reconstructed. trusted uses the same monotonic
    // Particle.timeSyncedLast() signal as the resync gate and trust check
    // in Generalized-Core-Counter.cpp (isClockTrusted()) - not
    // Time.isValid() alone (Finding 3). lastSyncEpoch is the persisted
    // completion stamp (sysStatus.get_lastTimeSync()), which previously had
    // zero callers.
    //
    // Round 6 (Stage 7 finding 1): syncAgeSec is derived from
    // reportedSyncAgeMs(), the wrap-aware, sentinel-bearing telemetry
    // value - not raw elapsedMs(). Raw elapsedMs() would wrap back around
    // to a small, deceptively fresh-looking value after a full millis()
    // rollover with no further confirmed sync, even though `trusted` has
    // correctly already gone false - contradicting telemetry consumers'
    // reading of the ledger. -1 (already the established "never synced"
    // sentinel for this signed-seconds field) is reused for BOTH "never
    // synced this boot" and "sync is stale beyond one full wrap" - both are
    // "no reliable age to report", and `trusted` is independently false in
    // both cases for any consumer that needs to tell them apart further.
    {
        const uint32_t reportedAgeMs = reportedSyncAgeMs();
        const long syncAgeSec = (reportedAgeMs == ClockTrust::kReportedSyncAgeUnavailableMs)
            ? -1L
            : (long)(reportedAgeMs / 1000UL);
        writerBase.name("clock").beginObject();
        writerBase.name("trusted").value(isClockTrusted());
        writerBase.name("syncAgeSec").value((int)syncAgeSec);
        writerBase.name("lastSyncEpoch").value((int)sysStatus.get_lastTimeSync());
        writerBase.endObject();
    }
    writerBase.endObject();

    if (!writerBase.buffer()) {
        Log.warn("Failed to create status JSON");
        return false;
    }

    bufferBase[writerBase.dataSize()] = '\0';

    // Only publish if the status payload actually changed.
    if (strcmp(lastPublishedStatus, bufferBase) == 0) {
        return true; // Not an error; nothing to do
    }

    char bufferPublish[DEVICE_STATUS_PAYLOAD_CAPACITY];
    strncpy(bufferPublish, bufferBase, sizeof(bufferPublish) - 1);
    bufferPublish[sizeof(bufferPublish) - 1] = '\0';
    const size_t statusPayloadSize = strlen(bufferPublish);
    Log.info("LedgerPayloadStatus: bytes=%lu/%lu schema=%d",
             (unsigned long)statusPayloadSize,
             (unsigned long)DEVICE_STATUS_PAYLOAD_CAPACITY,
             kLedgerSchemaVersion);

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
        // Preserve status contract change detection.
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

    char buffer[kDeviceDataPayloadCapacity];
    JSONBufferWriter writer(buffer, sizeof(buffer));
    const unsigned long freeHeap = System.freeMemory();
    
    writer.beginObject();
    writer.name("schemaVersion").value(kLedgerSchemaVersion);
    writer.name("timestamp").value((int)Time.now());
    writer.name("occupancy").beginObject();
    writer.name("occupied").value(current.get_occupied());
    writer.name("totalOccupiedSec").value((unsigned long)current.get_totalOccupiedSeconds());
    writer.endObject();
    writer.name("environment").beginObject();
    writer.name("temperature").value(current.get_internalTempC(), 1);
    writer.endObject();
    writer.name("battery").beginObject();
    writer.name("soc").value(PowerManager::instance().soc(), 1);
    writer.endObject();
    writer.name("system").beginObject();
    writer.name("freeHeap").value((int)freeHeap);
    writer.name("resetReason").value((int)System.resetReason());
    writer.name("resetReasonData").value((unsigned long)System.resetReasonData());
    writer.endObject();
    writer.endObject();
    
    if (!writer.buffer()) {
        Log.warn("Failed to create data JSON");
        return false;
    }
    
    buffer[writer.dataSize()] = '\0';
    const size_t dataPayloadSize = strlen(buffer);
    Log.info("LedgerPayloadData: bytes=%lu/%lu schema=%d",
             (unsigned long)dataPayloadSize,
             (unsigned long)kDeviceDataPayloadCapacity,
             kLedgerSchemaVersion);
    
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
