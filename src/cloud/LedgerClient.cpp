#include "cloud/Cloud.h"
#include "power/ConnectivityPolicy.h"

void Cloud::setup() {
    Log.info("Setting up Cloud configuration management");
    
    // Create ledgers - default-settings will be Product scope via Console
    defaultSettingsLedger = Particle.ledger("default-settings");
    defaultSettingsLedger.onSync(onDefaultSettingsSync);
    
    // device-settings is Device scope (default for per-device ledgers)
    deviceSettingsLedger = Particle.ledger("device-settings");
    deviceSettingsLedger.onSync(onDeviceSettingsSync);
    
    deviceStatusLedger = Particle.ledger("device-status");
    deviceDataLedger = Particle.ledger("device-data");
    
    Log.info("Ledgers configured:");
    Log.info("  default-settings: Product defaults (Cloud->Device)");
    Log.info("  device-settings: Device overrides (Cloud->Device)");
    Log.info("  device-status: Current config (Device->Cloud)");
    Log.info("  device-data: Sensor readings (Device->Cloud)");
}

// Static callbacks
void Cloud::onDefaultSettingsSync(Ledger ledger) {
    Log.info("default-settings synced from cloud");
    // Do not merge/apply inside async callbacks; keep expensive work
    // in the main application thread/state machine.
    Cloud::instance().ledgersSynced = true;
    Cloud::instance().pendingConfigApply = true;
}

void Cloud::onDeviceSettingsSync(Ledger ledger) {
    Log.info("device-settings synced from cloud");
    // Do not merge/apply inside async callbacks; keep expensive work
    // in the main application thread/state machine.
    Cloud::instance().ledgersSynced = true;
    Cloud::instance().pendingConfigApply = true;
}

bool Cloud::areLedgersSynced() const {
    // Check if both input ledgers (default-settings and device-settings) have synced
    // A ledger is considered synced if lastSynced() returns a non-zero timestamp
    time_t defaultSync = defaultSettingsLedger.lastSynced();
    time_t deviceSync = deviceSettingsLedger.lastSynced();
    
    // Trace-level logging to avoid spam in main loop (called every iteration)
    Log.trace("Ledger sync check: default-settings=%lu device-settings=%lu", 
              (unsigned long)defaultSync, (unsigned long)deviceSync);
    
    // If the device is connected and enough time has passed (5+ seconds), 
    // consider ledgers synced even if timestamps are 0 (empty ledgers)
    static unsigned long firstConnectedTime = 0;
    static bool wasDisconnected = true;
    
    if (Particle.connected()) {
        if (wasDisconnected) {
            firstConnectedTime = millis();
            wasDisconnected = false;
            Log.info("Connected - starting %lu ms ledger sync window", 
                     ConnectivityPolicy::LEDGER_SYNC_TIMEOUT_MS);
        }
        
        // Give ledgers time to sync after connection (platform-specific timeout)
        unsigned long elapsedSinceConnect = millis() - firstConnectedTime;
        if (elapsedSinceConnect > ConnectivityPolicy::LEDGER_SYNC_TIMEOUT_MS) {
            // If either ledger has synced, both must sync
            if (defaultSync > 0 || deviceSync > 0) {
                bool bothSynced = (defaultSync > 0 && deviceSync > 0);
                if (!bothSynced) {
                    Log.warn("Partial ledger sync after %lu ms: default=%lu device=%lu", 
                             elapsedSinceConnect,
                             (unsigned long)defaultSync, (unsigned long)deviceSync);
                }
                return bothSynced;
            }
            // If neither has synced after timeout, assume they're empty and that's okay
            Log.info("No ledger data after %lu ms - assuming empty ledgers (OK)", elapsedSinceConnect);
            return true;
        }
        // Still within the sync window
        Log.trace("Ledger sync pending: %lu ms elapsed (waiting for %lu ms)", 
                  elapsedSinceConnect, ConnectivityPolicy::LEDGER_SYNC_TIMEOUT_MS);
        return false;
    } else {
        // Disconnected - reset for next connection
        wasDisconnected = true;
        return false;
    }
}
