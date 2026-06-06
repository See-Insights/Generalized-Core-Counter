#include "cloud/Cloud.h"
#include "power/ConnectivityPolicy.h"

namespace {

bool ledgerHasConfigContent(const LedgerData &ledger) {
    return ledger.has("sensor") ||
           ledger.has("timing") ||
           ledger.has("messaging") ||
           ledger.has("modes") ||
           ledger.has("reporting");
}

bool ledgerHasUnsyncedWrite(const Ledger &ledger) {
    const int64_t lastUpdatedMs = ledger.lastUpdated();
    if (lastUpdatedMs <= 0) {
        return false;
    }

    const int64_t lastSyncedMs = ledger.lastSynced();
    return lastSyncedMs <= 0 || lastSyncedMs < lastUpdatedMs;
}

} // namespace

void Cloud::setup() {
    // Create ledgers - default-settings will be Product scope via Console
    defaultSettingsLedger = Particle.ledger("default-settings");
    defaultSettingsLedger.onSync(onDefaultSettingsSync);
    
    // device-settings is Device scope (default for per-device ledgers)
    deviceSettingsLedger = Particle.ledger("device-settings");
    deviceSettingsLedger.onSync(onDeviceSettingsSync);
    
    deviceStatusLedger = Particle.ledger("device-status");
    deviceStatusLedger.onSync(onDeviceStatusLedgerSync);
    deviceDataLedger = Particle.ledger("device-data");
    deviceDataLedger.onSync(onDeviceDataLedgerSync);

}

// Static callbacks
void Cloud::onDefaultSettingsSync(Ledger ledger) {
    Log.info("LedgerCallback: kind=input ledger=default-settings synced=%lu",
             (unsigned long)ledger.lastSynced());
    if (sysStatus.get_verboseMode()) {
        Log.info("default-settings synced from cloud");
    }
    // Do not merge/apply inside async callbacks; keep expensive work
    // in the main application thread/state machine.
    Cloud::instance().ledgersSynced = true;
    Cloud::instance().pendingConfigApply = true;
}

void Cloud::onDeviceSettingsSync(Ledger ledger) {
    Log.info("LedgerCallback: kind=input ledger=device-settings synced=%lu",
             (unsigned long)ledger.lastSynced());
    if (sysStatus.get_verboseMode()) {
        Log.info("device-settings synced from cloud");
    }
    // Do not merge/apply inside async callbacks; keep expensive work
    // in the main application thread/state machine.
    Cloud::instance().ledgersSynced = true;
    Cloud::instance().pendingConfigApply = true;
}

void Cloud::onDeviceStatusLedgerSync(Ledger ledger) {
    (void)ledger;
    const LedgerSyncDiagnostics before = Cloud::instance().ledgerSyncDiagnostics();
    Cloud::instance().pendingDeviceStatusSync = false;
    const LedgerSyncDiagnostics after = Cloud::instance().ledgerSyncDiagnostics();
    Cloud::instance().noteLedgerSyncComplete(Cloud::LEDGER_REQUEST_KIND_STATUS, before, after);
}

void Cloud::onDeviceDataLedgerSync(Ledger ledger) {
    (void)ledger;
    const LedgerSyncDiagnostics before = Cloud::instance().ledgerSyncDiagnostics();
    Cloud::instance().pendingDeviceDataSync = false;
    const LedgerSyncDiagnostics after = Cloud::instance().ledgerSyncDiagnostics();
    Cloud::instance().noteLedgerSyncComplete(Cloud::LEDGER_REQUEST_KIND_DATA, before, after);
}

bool Cloud::hasPendingOutputLedgerSync() const {
    const bool statusPending = pendingStatusPublish ||
            pendingDeviceStatusSync ||
            ledgerHasUnsyncedWrite(deviceStatusLedger);
    const bool dataPending = pendingDeviceDataSync ||
            ledgerHasUnsyncedWrite(deviceDataLedger);
    return statusPending || dataPending;
}

bool Cloud::areLedgersSynced() const {
    // A ledger is considered synced once its lastSynced() timestamp becomes non-zero.
    time_t defaultSync = defaultSettingsLedger.lastSynced();
    time_t deviceSync = deviceSettingsLedger.lastSynced();
    bool defaultSynced = (defaultSync > 0);
    bool deviceSynced = (deviceSync > 0);
    
    // Track the current connection's sync window explicitly. The helper must not
    // reuse timing state from a previous sleep/connect cycle.
    static unsigned long firstConnectedTime = 0;
    static bool wasDisconnected = true;
    static time_t lastObservedConnectionEpoch = 0;
    static bool timeoutOutcomeLogged = false;
    static bool readyOutcomeLogged = false;
    unsigned long nowMs = millis();
    
    if (Particle.connected()) {
        time_t currentConnectionEpoch = sysStatus.get_lastConnection();
        bool newConnectionObserved = wasDisconnected ||
                                   (currentConnectionEpoch != 0 && currentConnectionEpoch != lastObservedConnectionEpoch);

        if (newConnectionObserved) {
            firstConnectedTime = nowMs;
            wasDisconnected = false;
            lastObservedConnectionEpoch = currentConnectionEpoch;
            timeoutOutcomeLogged = false;
            readyOutcomeLogged = false;
            if (sysStatus.get_verboseMode()) {
                Log.info("Connected - starting %lu ms ledger sync window", 
                         ConnectivityPolicy::LEDGER_SYNC_TIMEOUT_MS);
            }
#if ENABLE_LEDGER_TRACE
            Log.info("LedgerInputWindow: start defaultSynced=%d deviceSynced=%d timeout=%lu",
                     defaultSynced ? 1 : 0,
                     deviceSynced ? 1 : 0,
                     (unsigned long)ConnectivityPolicy::LEDGER_SYNC_TIMEOUT_MS);
#endif
        }

        // If both ledgers are already synced for this connection, do not force the
        // caller to wait out the remaining window. This is the key Alert 44 fix.
        if (defaultSynced && deviceSynced) {
            if (!readyOutcomeLogged) {
                unsigned long elapsedSinceConnect = nowMs - firstConnectedTime;
#if ENABLE_LEDGER_TRACE
                Log.info("LedgerInputReady: elapsed=%lu default=%lu device=%lu",
                         elapsedSinceConnect,
                         (unsigned long)defaultSync,
                         (unsigned long)deviceSync);
#else
                (void)elapsedSinceConnect;
#endif
                readyOutcomeLogged = true;
            }
            return true;
        }
        
        // Give ledgers time to sync after connection (platform-specific timeout)
        unsigned long elapsedSinceConnect = nowMs - firstConnectedTime;
        if (elapsedSinceConnect > ConnectivityPolicy::LEDGER_SYNC_TIMEOUT_MS) {
            LedgerData defaultData = defaultSettingsLedger.get();
            LedgerData deviceData = deviceSettingsLedger.get();
            bool defaultHasConfig = ledgerHasConfigContent(defaultData);
            bool deviceHasConfig = ledgerHasConfigContent(deviceData);

            if (defaultSynced && !deviceSynced && !deviceHasConfig) {
                if (!timeoutOutcomeLogged && sysStatus.get_verboseMode()) {
                    Log.info("default-settings synced and device-settings is empty after %lu ms - assuming no device overrides",
                             elapsedSinceConnect);
                }
                if (!timeoutOutcomeLogged) {
#if ENABLE_LEDGER_TRACE
                    Log.info("LedgerInputReady: elapsed=%lu default=%lu device=%lu empty-device=1",
                             elapsedSinceConnect,
                             (unsigned long)defaultSync,
                             (unsigned long)deviceSync);
#endif
                }
                timeoutOutcomeLogged = true;
                return true;
            }

            if (!defaultSynced && deviceSynced && !defaultHasConfig) {
                if (!timeoutOutcomeLogged && sysStatus.get_verboseMode()) {
                    Log.info("device-settings synced and default-settings is empty after %lu ms - assuming no product defaults",
                             elapsedSinceConnect);
                }
                if (!timeoutOutcomeLogged) {
#if ENABLE_LEDGER_TRACE
                    Log.info("LedgerInputReady: elapsed=%lu default=%lu device=%lu empty-default=1",
                             elapsedSinceConnect,
                             (unsigned long)defaultSync,
                             (unsigned long)deviceSync);
#endif
                }
                timeoutOutcomeLogged = true;
                return true;
            }

            // If either ledger has synced, both must sync
            if (defaultSynced || deviceSynced) {
                bool bothSynced = (defaultSynced && deviceSynced);
                if (!bothSynced) {
                    if (!timeoutOutcomeLogged) {
                        Log.warn("Partial ledger sync after %lu ms: default=%lu device=%lu", 
                                 elapsedSinceConnect,
                                 (unsigned long)defaultSync, (unsigned long)deviceSync);
                        Log.info("LedgerInputBlocked: elapsed=%lu defaultSynced=%d deviceSynced=%d",
                                 elapsedSinceConnect,
                                 defaultSynced ? 1 : 0,
                                 deviceSynced ? 1 : 0);
                        timeoutOutcomeLogged = true;
                    }
                }
                return bothSynced;
            }
            // If neither has synced after timeout, assume they're empty and that's okay
            if (!timeoutOutcomeLogged && sysStatus.get_verboseMode()) {
                Log.info("No ledger data after %lu ms - assuming empty ledgers (OK)", elapsedSinceConnect);
            }
            if (!timeoutOutcomeLogged) {
#if ENABLE_LEDGER_TRACE
                Log.info("LedgerInputReady: elapsed=%lu default=%lu device=%lu empty=1",
                         elapsedSinceConnect,
                         (unsigned long)defaultSync,
                         (unsigned long)deviceSync);
#endif
            }
            timeoutOutcomeLogged = true;
            return true;
        }
        return false;
    } else {
        // Disconnected - reset for next connection
        wasDisconnected = true;
        lastObservedConnectionEpoch = 0;
    timeoutOutcomeLogged = false;
        return false;
    }
}
