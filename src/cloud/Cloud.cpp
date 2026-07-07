/**
 * @file Cloud.cpp
 * @brief Cloud Configuration Management Implementation
 * 
 * @details Simple Particle Ledger-based configuration with manual nested object merging.
 *          Uses Product scope for defaults and Device scope for overrides.
 * 
 * @author Chip McClelland
 * @date December 13, 2025
 */

#include "../Config.h"
#include "cloud/Cloud.h"

namespace {

struct LedgerRequestRecord {
    uint32_t seq;
    Cloud::LedgerRequestKind kind;
    const char *source;
    const void *ptr;
    unsigned long issueMs;
};

constexpr size_t LEDGER_REQUEST_TRACKER_MAX = 16;

LedgerRequestRecord ledgerRequests[LEDGER_REQUEST_TRACKER_MAX];
size_t ledgerRequestCount = 0;
uint32_t ledgerRequestSequence = 0;
bool ledgerDrainActive = false;
unsigned long ledgerDrainStartMs = 0;
unsigned long ledgerCompletedAgeMinMs = 0;
unsigned long ledgerCompletedAgeMaxMs = 0;
bool ledgerCompletedAgeValid = false;

bool ledgerHasUnsyncedWriteForDiag(const Ledger &ledger) {
    const int64_t lastUpdatedMs = ledger.lastUpdated();
    if (lastUpdatedMs <= 0) {
        return false;
    }

    const int64_t lastSyncedMs = ledger.lastSynced();
    return lastSyncedMs <= 0 || lastSyncedMs < lastUpdatedMs;
}

const char *ledgerRequestKindLabel(Cloud::LedgerRequestKind kind) {
    switch (kind) {
    case Cloud::LEDGER_REQUEST_KIND_STATUS:
        return "status";
    case Cloud::LEDGER_REQUEST_KIND_DATA:
        return "data";
    default:
        return "unknown";
    }
}

const char *ledgerRequestKindCompactLabel(Cloud::LedgerRequestKind kind) {
    switch (kind) {
    case Cloud::LEDGER_REQUEST_KIND_STATUS:
        return "STATUS";
    case Cloud::LEDGER_REQUEST_KIND_DATA:
        return "DATA";
    default:
        return "UNKNOWN";
    }
}

const char *ledgerRequestSourceLabel(const char *source) {
    return (source && source[0] != '\0') ? source : "Unknown";
}

uint16_t currentLedgerPendingCount() {
    return (uint16_t)ledgerRequestCount;
}

unsigned long currentLedgerOldestAgeMs(unsigned long nowMs) {
    if (ledgerRequestCount == 0) {
        return 0;
    }

    unsigned long oldestAgeMs = 0;
    for (size_t index = 0; index < ledgerRequestCount; ++index) {
        unsigned long ageMs = nowMs - ledgerRequests[index].issueMs;
        if (ageMs > oldestAgeMs) {
            oldestAgeMs = ageMs;
        }
    }
    return oldestAgeMs;
}

void maybeLogLedgerDrainSummary() {
    if (!ledgerDrainActive || ledgerRequestCount != 0) {
        return;
    }

    const unsigned long elapsedMs = millis() - ledgerDrainStartMs;
#if ENABLE_LEDGER_TRACE
    Log.info("LedgerDrain: elapsed=%lu pending=0 syncing=0 inflight=0 oldestCompletedAge=%lu maxCompletedAge=%lu",
             elapsedMs,
             ledgerCompletedAgeValid ? ledgerCompletedAgeMinMs : 0UL,
             ledgerCompletedAgeValid ? ledgerCompletedAgeMaxMs : 0UL);
#else
    (void)elapsedMs;
#endif

    ledgerDrainActive = false;
    ledgerDrainStartMs = 0;
    ledgerCompletedAgeMinMs = 0;
    ledgerCompletedAgeMaxMs = 0;
    ledgerCompletedAgeValid = false;
}

void logActiveLedgerRequests(const char *linePrefix, unsigned long nowMs) {
    for (size_t index = 0; index < ledgerRequestCount; ++index) {
        const unsigned long ageMs = nowMs - ledgerRequests[index].issueMs;
        Log.info("%s: seq=%lu kind=%s source=%s ptr=%p age=%lu",
                 linePrefix,
                 (unsigned long)ledgerRequests[index].seq,
                 ledgerRequestKindLabel(ledgerRequests[index].kind),
                 ledgerRequestSourceLabel(ledgerRequests[index].source),
                 ledgerRequests[index].ptr,
                 ageMs);
    }
}

LedgerRequestRecord *findRequestBySeq(uint32_t seq) {
    for (size_t index = 0; index < ledgerRequestCount; ++index) {
        if (ledgerRequests[index].seq == seq) {
            return &ledgerRequests[index];
        }
    }
    return nullptr;
}

LedgerRequestRecord *findRequestByPointer(const void *ptr) {
    if (!ptr) {
        return nullptr;
    }

    for (size_t index = 0; index < ledgerRequestCount; ++index) {
        if (ledgerRequests[index].ptr == ptr) {
            return &ledgerRequests[index];
        }
    }

    return nullptr;
}

LedgerRequestRecord *findOldestRequestOfKind(Cloud::LedgerRequestKind kind) {
    for (size_t index = 0; index < ledgerRequestCount; ++index) {
        if (ledgerRequests[index].kind == kind) {
            return &ledgerRequests[index];
        }
    }
    return nullptr;
}

void removeLedgerRequest(LedgerRequestRecord *record) {
    if (!record || ledgerRequestCount == 0) {
        return;
    }

    size_t index = (size_t)(record - ledgerRequests);
    for (; index + 1 < ledgerRequestCount; ++index) {
        ledgerRequests[index] = ledgerRequests[index + 1];
    }
    --ledgerRequestCount;
}

void updateCompletedAgeStats(unsigned long ageMs) {
    if (!ledgerCompletedAgeValid) {
        ledgerCompletedAgeMinMs = ageMs;
        ledgerCompletedAgeMaxMs = ageMs;
        ledgerCompletedAgeValid = true;
        return;
    }

    if (ageMs < ledgerCompletedAgeMinMs) {
        ledgerCompletedAgeMinMs = ageMs;
    }
    if (ageMs > ledgerCompletedAgeMaxMs) {
        ledgerCompletedAgeMaxMs = ageMs;
    }
}

} // namespace

Cloud *Cloud::_instance;

// [static]
Cloud &Cloud::instance() {
    if (!_instance) {
        _instance = new Cloud();
    }
    return *_instance;
}

Cloud::Cloud() : ledgersSynced(false), lastApplySuccess(true) {
    lastPublishedStatus[0] = '\0';
    pendingStatusPublish = false;
    pendingStatusPublishSource = "Unknown";
    pendingConfigApply = false;
    pendingDeviceStatusSync = false;
    pendingDeviceDataSync = false;
}

Cloud::~Cloud() {
}

Cloud::LedgerSyncDiagnostics Cloud::ledgerSyncDiagnostics() const {
    const bool statusInflight = ledgerHasUnsyncedWriteForDiag(deviceStatusLedger);
    const bool dataInflight = ledgerHasUnsyncedWriteForDiag(deviceDataLedger);

    LedgerSyncDiagnostics diagnostics = {};
    diagnostics.pendingStatusPublish = pendingStatusPublish;
    diagnostics.pendingDeviceStatusSync = pendingDeviceStatusSync;
    diagnostics.pendingDeviceDataSync = pendingDeviceDataSync;
    diagnostics.statusInflight = statusInflight;
    diagnostics.dataInflight = dataInflight;
    diagnostics.syncing = pendingDeviceStatusSync || pendingDeviceDataSync;
    diagnostics.inflight = statusInflight || dataInflight;

    uint16_t pendingCount = 0;
    if (pendingStatusPublish) {
        pendingCount++;
    }
    if (pendingDeviceStatusSync) {
        pendingCount++;
    }
    if (pendingDeviceDataSync) {
        pendingCount++;
    }
    if (statusInflight) {
        pendingCount++;
    }
    if (dataInflight) {
        pendingCount++;
    }
    diagnostics.pendingCount = pendingCount;

    return diagnostics;
}

bool Cloud::isLedgerPointerTracked(const void *ptr) const {
    if (!ptr) {
        return false;
    }

    for (size_t index = 0; index < ledgerRequestCount; ++index) {
        if (ledgerRequests[index].ptr == ptr) {
            return true;
        }
    }

    return false;
}

uint32_t Cloud::noteLedgerSyncRequest(LedgerRequestKind kind,
                                      const char *source,
                                      const void *ptr) {
    const unsigned long issueMs = millis();
    LedgerRequestRecord *existing = findRequestByPointer(ptr);

    bool inflightForPointer = false;
    if (ptr == &deviceStatusLedger) {
        inflightForPointer = ledgerHasUnsyncedWriteForDiag(deviceStatusLedger);
    } else if (ptr == &deviceDataLedger) {
        inflightForPointer = ledgerHasUnsyncedWriteForDiag(deviceDataLedger);
    }

    if (existing && inflightForPointer) {
        const unsigned long ageMs = issueMs - existing->issueMs;
        unsigned long lastUpdated = 0;
        unsigned long lastSynced = 0;
        if (ptr == &deviceStatusLedger) {
            lastUpdated = (unsigned long)deviceStatusLedger.lastUpdated();
            lastSynced = (unsigned long)deviceStatusLedger.lastSynced();
        } else if (ptr == &deviceDataLedger) {
            lastUpdated = (unsigned long)deviceDataLedger.lastUpdated();
            lastSynced = (unsigned long)deviceDataLedger.lastSynced();
        }
#if ENABLE_LEDGER_TRACE
    Log.warn("LedgerDup: seq=%lu globalSeq=%lu kind=%s orig=%s new=%s age=%lu count=%u pendingData=%d pendingStatus=%d upd=%lu sync=%lu",
                 (unsigned long)existing->seq,
         (unsigned long)ledgerRequestSequence,
                 ledgerRequestKindCompactLabel(existing->kind),
                 ledgerRequestSourceLabel(existing->source),
                 ledgerRequestSourceLabel(source),
                 ageMs,
                 (unsigned)currentLedgerPendingCount(),
                 pendingDeviceDataSync ? 1 : 0,
                 pendingDeviceStatusSync ? 1 : 0,
                 lastUpdated,
                 lastSynced);
        Log.info("LedgerIssueSkip: source=%s ptr=%p existingSeq=%lu age=%lu",
                 ledgerRequestSourceLabel(source),
                 ptr,
                 (unsigned long)existing->seq,
                 ageMs);
#else
        if (ageMs > 30000UL) {
            Log.warn("LedgerDuplicateStillInflight: seq=%lu globalSeq=%lu kind=%s orig=%s new=%s age=%lu count=%u pendingData=%d pendingStatus=%d upd=%lu sync=%lu",
                     (unsigned long)existing->seq,
                     (unsigned long)ledgerRequestSequence,
                     ledgerRequestKindCompactLabel(existing->kind),
                     ledgerRequestSourceLabel(existing->source),
                     ledgerRequestSourceLabel(source),
                     ageMs,
                     (unsigned)currentLedgerPendingCount(),
                     pendingDeviceDataSync ? 1 : 0,
                     pendingDeviceStatusSync ? 1 : 0,
                     lastUpdated,
                     lastSynced);
        }
#endif
        return 0;
    }

    const uint32_t seq = ++ledgerRequestSequence;

    if (ledgerRequestCount < LEDGER_REQUEST_TRACKER_MAX) {
        ledgerRequests[ledgerRequestCount++] = {seq, kind, source, ptr, issueMs};
    } else {
        Log.warn("LedgerIssueTrackerFull: seq=%lu kind=%s active=%u", seq, ledgerRequestKindLabel(kind), (unsigned)ledgerRequestCount);
    }

    if (!ledgerDrainActive) {
        ledgerDrainActive = true;
        ledgerDrainStartMs = issueMs;
        ledgerCompletedAgeMinMs = 0;
        ledgerCompletedAgeMaxMs = 0;
        ledgerCompletedAgeValid = false;
    }

    const uint16_t pendingCount = currentLedgerPendingCount();
    const bool activeNow = pendingCount > 0;
#if ENABLE_LEDGER_TRACE
    Log.info("LedgerIssue: seq=%lu kind=%s source=%s ptr=%p pending=%u syncing=%d inflight=%d ms=%lu",
             (unsigned long)seq,
             ledgerRequestKindLabel(kind),
             ledgerRequestSourceLabel(source),
             ptr,
             (unsigned)pendingCount,
             activeNow ? 1 : 0,
             activeNow ? 1 : 0,
             issueMs);
#else
    (void)pendingCount;
    (void)activeNow;
#endif

    return seq;
}

void Cloud::noteLedgerSyncComplete(LedgerRequestKind kind,
                                   unsigned long lastUpdated,
                                   unsigned long lastSynced,
                                   const LedgerSyncDiagnostics &before,
                                   const LedgerSyncDiagnostics &after) {
    const unsigned long nowMs = millis();
    const uint16_t trackerCountBefore = currentLedgerPendingCount();
    LedgerRequestRecord *record = findOldestRequestOfKind(kind);
    if (!record) {
        Log.warn("LedgerCb: kind=%s seq=0 globalSeq=%lu found=0 age=0 ms=%lu upd=%lu sync=%lu countBefore=%u countAfter=%u pendingData=%d pendingStatus=%d",
                 ledgerRequestKindCompactLabel(kind),
                 (unsigned long)ledgerRequestSequence,
                 nowMs,
                 lastUpdated,
                 lastSynced,
                 (unsigned)trackerCountBefore,
                 (unsigned)currentLedgerPendingCount(),
                 after.pendingDeviceDataSync ? 1 : 0,
                 after.pendingDeviceStatusSync ? 1 : 0);
        return;
    }

    const uint32_t seq = record->seq;
    const char *source = ledgerRequestSourceLabel(record->source);
    const void *ptr = record->ptr;
    const unsigned long ageMs = nowMs - record->issueMs;
    removeLedgerRequest(record);
    const uint16_t trackerCountAfter = currentLedgerPendingCount();
    updateCompletedAgeStats(ageMs);

    Log.info("LedgerCb: kind=%s seq=%lu globalSeq=%lu found=1 age=%lu ms=%lu upd=%lu sync=%lu countBefore=%u countAfter=%u pendingData=%d pendingStatus=%d",
             ledgerRequestKindCompactLabel(kind),
             (unsigned long)seq,
             (unsigned long)ledgerRequestSequence,
             ageMs,
             nowMs,
             lastUpdated,
             lastSynced,
             (unsigned)trackerCountBefore,
             (unsigned)trackerCountAfter,
             after.pendingDeviceDataSync ? 1 : 0,
             after.pendingDeviceStatusSync ? 1 : 0);

#if ENABLE_LEDGER_TRACE
    Log.info("LedgerComplete: seq=%lu kind=%s source=%s ptr=%p age=%lu pending_before=%u pending_after=%u syncing_before=%d syncing_after=%d inflight_before=%d inflight_after=%d",
             (unsigned long)seq,
             ledgerRequestKindLabel(kind),
             source,
             ptr,
             ageMs,
             (unsigned)before.pendingCount,
             (unsigned)after.pendingCount,
             before.syncing ? 1 : 0,
             after.syncing ? 1 : 0,
             before.inflight ? 1 : 0,
             after.inflight ? 1 : 0);
#else
    (void)seq;
    (void)source;
    (void)ptr;
    (void)before;
    (void)after;
#endif

    maybeLogLedgerDrainSummary();
}

void Cloud::noteLedgerSyncFail(uint32_t seq, int error) {
    LedgerRequestRecord *record = findRequestBySeq(seq);
    if (!record) {
        Log.warn("LedgerFail: seq=%lu kind=unknown source=Unknown ptr=%p age=0 error=%d",
                 (unsigned long)seq,
                 nullptr,
                 error);
        return;
    }

    const unsigned long nowMs = millis();
    const char *source = ledgerRequestSourceLabel(record->source);
    const void *ptr = record->ptr;
    const unsigned long ageMs = nowMs - record->issueMs;
    const LedgerRequestKind kind = record->kind;

    if (error == -1001) {
        logLedgerFailure(error);
    }

    removeLedgerRequest(record);

    Log.warn("LedgerFail: seq=%lu kind=%s source=%s ptr=%p age=%lu error=%d",
             (unsigned long)seq,
             ledgerRequestKindLabel(kind),
             source,
             ptr,
             ageMs,
             error);

    maybeLogLedgerDrainSummary();
}

void Cloud::logLedgerConnectState() const {
#if ENABLE_LEDGER_TRACE
    const unsigned long nowMs = millis();
    const uint16_t pendingCount = (uint16_t)ledgerRequestCount;
    const bool activeNow = pendingCount > 0;
    const unsigned long oldestAgeMs = currentLedgerOldestAgeMs(nowMs);

    Log.info("LedgerConnectState: pending=%u syncing=%d inflight=%d oldestAge=%lu",
             (unsigned)pendingCount,
             activeNow ? 1 : 0,
             activeNow ? 1 : 0,
             oldestAgeMs);

    logActiveLedgerRequests("LedgerCarryover", nowMs);
#endif
}

void Cloud::logLedgerStartupState() const {
    Log.info("LedgerStartup: globalSeq=%lu count=%u pendingData=%d pendingStatus=%d dataUpd=%lu dataSync=%lu statusUpd=%lu statusSync=%lu",
             (unsigned long)ledgerRequestSequence,
             (unsigned)ledgerRequestCount,
             pendingDeviceDataSync ? 1 : 0,
             pendingDeviceStatusSync ? 1 : 0,
             (unsigned long)deviceDataLedger.lastUpdated(),
             (unsigned long)deviceDataLedger.lastSynced(),
             (unsigned long)deviceStatusLedger.lastUpdated(),
             (unsigned long)deviceStatusLedger.lastSynced());
}

void Cloud::logLedgerSleepState() const {
    const unsigned long nowMs = millis();
    const uint16_t pendingCount = (uint16_t)ledgerRequestCount;
    const bool activeNow = pendingCount > 0;
    if (!activeNow) {
        return;
    }

    const unsigned long oldestAgeMs = currentLedgerOldestAgeMs(nowMs);

#if ENABLE_LEDGER_TRACE
    Log.info("LedgerSleepState: pending=%u syncing=%d inflight=%d oldestAge=%lu",
             (unsigned)pendingCount,
             activeNow ? 1 : 0,
             activeNow ? 1 : 0,
             oldestAgeMs);

    logActiveLedgerRequests("LedgerSleepActive", nowMs);
#else
    Log.info("LedgerSleepState: pending=%u syncing=%d inflight=%d oldestAge=%lu",
             (unsigned)pendingCount,
             1,
             1,
             oldestAgeMs);
#endif
}

void Cloud::logLedgerSleepTimeoutState() const {
    const unsigned long nowMs = millis();
    const uint16_t pendingCount = (uint16_t)ledgerRequestCount;

    Log.warn("LedgerSleepTimeout: globalSeq=%lu count=%u pendingData=%d pendingStatus=%d dataUpd=%lu dataSync=%lu statusUpd=%lu statusSync=%lu",
             (unsigned long)ledgerRequestSequence,
             (unsigned)pendingCount,
             pendingDeviceDataSync ? 1 : 0,
             pendingDeviceStatusSync ? 1 : 0,
             (unsigned long)deviceDataLedger.lastUpdated(),
             (unsigned long)deviceDataLedger.lastSynced(),
             (unsigned long)deviceStatusLedger.lastUpdated(),
             (unsigned long)deviceStatusLedger.lastSynced());

    for (size_t index = 0; index < ledgerRequestCount; ++index) {
        const unsigned long ageMs = nowMs - ledgerRequests[index].issueMs;
        Log.warn("LedgerSleepTimeoutActive: seq=%lu globalSeq=%lu kind=%s source=%s age=%lu",
                 (unsigned long)ledgerRequests[index].seq,
                 (unsigned long)ledgerRequestSequence,
                 ledgerRequestKindCompactLabel(ledgerRequests[index].kind),
                 ledgerRequestSourceLabel(ledgerRequests[index].source),
                 ageMs);
    }
}

void Cloud::logLedgerFailure(int rc) const {
    const unsigned long nowMs = millis();
    const uint16_t pendingCount = (uint16_t)ledgerRequestCount;
    const bool activeNow = pendingCount > 0;
    const unsigned long oldestAgeMs = currentLedgerOldestAgeMs(nowMs);

    Log.warn("LedgerFailure: rc=%d pending=%u syncing=%d inflight=%d oldestAge=%lu",
             rc,
             (unsigned)pendingCount,
             activeNow ? 1 : 0,
             activeNow ? 1 : 0,
             oldestAgeMs);

    logActiveLedgerRequests("LedgerFailureActive", nowMs);
}

namespace {

void logConfigApplySnapshot() {
#if ENABLE_CONFIG_TRACE
    const char *tz = sysStatus.get_timeZoneStrCStr();
    if (!tz) {
        tz = "";
    }

    Log.info("ConfigApply: tz=%s open=%d close=%d report=%lu debounce=%lu sourceBefore=%u ledgerValidBefore=%d",
             tz,
             (int)sysStatus.get_openTime(),
             (int)sysStatus.get_closeTime(),
             (unsigned long)sysStatus.get_reportingInterval(),
             (unsigned long)sensorConfig.get_sensorSetting1(),
             (unsigned)sysStatus.get_configSource(),
             sysStatus.get_hasValidLedgerConfig() ? 1 : 0);
#endif
}

void logConfigDiagFailure(const char *reason) {
    const char *tz = sysStatus.get_timeZoneStrCStr();
    if (!tz) {
        tz = "";
    }

    Log.warn("ConfigDiag: source=%s valid=0 timezone=%s open=%d close=%d report=%lu reason=%s",
             Config::sourceToString(Config::getSource()),
             tz,
             (int)sysStatus.get_openTime(),
             (int)sysStatus.get_closeTime(),
             (unsigned long)sysStatus.get_reportingInterval(),
             reason ? reason : "unknown");
}

bool finalizeLedgerAppliedConfig(const char *invalidLogPrefix) {
    logConfigApplySnapshot();

    const char *fieldFailureReason = "none";
    const bool fieldsValid = Config::validateConfigFields(true, &fieldFailureReason);
    if (fieldsValid) {
        Config::markLedgerConfigurationValid();
    }

    const char *configFailureReason = "none";
    const bool configValid = Config::isValid(true, &configFailureReason);

#if ENABLE_CONFIG_TRACE
    Log.info("ConfigValidate: fieldsValid=%d configValid=%d source=%u ledgerValid=%d",
             fieldsValid ? 1 : 0,
             configValid ? 1 : 0,
             (unsigned)sysStatus.get_configSource(),
             sysStatus.get_hasValidLedgerConfig() ? 1 : 0);
#endif

    if (fieldsValid && configValid) {
        Config::logDiagnostics("ConfigDiag");
        return true;
    }

    if (!fieldsValid) {
        logConfigDiagFailure(fieldFailureReason);
    } else {
        logConfigDiagFailure(configFailureReason);
    }

    Log.warn("%s", invalidLogPrefix);
    return false;
}

} // namespace

bool Cloud::loadConfigurationFromCloud() {
    LedgerData defaults = defaultSettingsLedger.get();
    LedgerData device = deviceSettingsLedger.get();
    lastApplySuccess = applyConfigurationFromLedger(defaults, device);

    if (lastApplySuccess) {
        lastApplySuccess = finalizeLedgerAppliedConfig(
            "Configuration apply completed but resulting config is invalid");
    }

    return lastApplySuccess;
}

const char *Cloud::getWebhookName() const {
    // Priority 1: Cloud configuration (explicitly set)
    const char *cloudWebhookName = sysStatus.get_webhookNameCStr();
    if (cloudWebhookName && cloudWebhookName[0] != '\0') {
        return cloudWebhookName;
    }

    // Priority 2: Convention-based (mode-specific)
    uint8_t mode = sysStatus.get_sensorMode();
    const char *conventionName = "unknown-webhook-v1";

    switch (mode) {
        case COUNTING:
            conventionName = "counting-webhook-v1";
            break;
        case OCCUPANCY:
            conventionName = "occupancy-webhook-v1";
            break;
        case MEASUREMENT:
            conventionName = "measurement-webhook-v1";
            break;
        default:
            break;
    }

    return conventionName;
}

void Cloud::loop() {
    // Apply any newly-synced configuration outside callback context.
    // Do at most one deferred operation per loop() pass.
    if (pendingConfigApply && Particle.connected()) {
        pendingConfigApply = false;
        LedgerData defaults = defaultSettingsLedger.get();
        LedgerData device = deviceSettingsLedger.get();
        lastApplySuccess = applyConfigurationFromLedger(defaults, device);

        if (lastApplySuccess) {
            lastApplySuccess = finalizeLedgerAppliedConfig(
                "Deferred configuration apply completed but resulting config is invalid");
        }

        return;
    }

    // Publish device-status updates opportunistically when connected.
    // Do at most one deferred operation per loop() pass.
    if (pendingStatusPublish && Particle.connected()) {
        if (writeDeviceStatusToCloud(pendingStatusPublishSource)) {
            pendingStatusPublish = false;
            pendingStatusPublishSource = "Unknown";
        }
    }
}
