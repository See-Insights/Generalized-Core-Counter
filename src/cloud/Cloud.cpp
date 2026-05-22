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

#include "cloud/Cloud.h"

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
    pendingConfigApply = false;
    pendingDeviceStatusSync = false;
    pendingDeviceDataSync = false;
}

Cloud::~Cloud() {
}

bool Cloud::loadConfigurationFromCloud() {
    LedgerData defaults = defaultSettingsLedger.get();
    LedgerData device = deviceSettingsLedger.get();
    lastApplySuccess = applyConfigurationFromLedger(defaults, device);
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
        return;
    }

    // Publish device-status updates opportunistically when connected.
    // Do at most one deferred operation per loop() pass.
    if (pendingStatusPublish && Particle.connected()) {
        if (writeDeviceStatusToCloud()) {
            pendingStatusPublish = false;
        }
    }
}
