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

#include "Cloud.h"

Cloud *Cloud::_instance;

// [static]
Cloud &Cloud::instance() {
    if (!_instance) {
        _instance = new Cloud();
    }
    return *_instance;
}

Cloud::Cloud() : ledgersSynced(false), lastApplySuccess(true) {
    lastPublishedStatus = "";
    pendingStatusPublish = false;
    pendingConfigApply = false;
}

Cloud::~Cloud() {
}

bool Cloud::loadConfigurationFromCloud() {
    Log.info("Syncing configuration from cloud");
    
    // Trigger merge and apply configuration. mergeConfiguration() will update
    // lastApplySuccess based on the result of applyConfigurationFromLedger().
    mergeConfiguration();
    return lastApplySuccess;
}

String Cloud::getWebhookName() {
    // Priority 1: Cloud configuration (explicitly set)
    String cloudWebhookName = sysStatus.get_webhookName();
    if (cloudWebhookName.length() > 0) {
        Log.info("Using cloud-configured webhook: %s", cloudWebhookName.c_str());
        return cloudWebhookName;
    }
    
    // Priority 2: Convention-based (mode-specific)
    uint8_t mode = sysStatus.get_sensorMode();
    String conventionName;
    
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
            conventionName = "unknown-webhook-v1";
            break;
    }
    
    Log.info("Using convention-based webhook: %s", conventionName.c_str());
    return conventionName;
}

void Cloud::loop() {
    // Apply any newly-synced configuration outside callback context.
    // Do at most one deferred operation per loop() pass.
    if (pendingConfigApply && Particle.connected()) {
        pendingConfigApply = false;
        mergeConfiguration();
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
