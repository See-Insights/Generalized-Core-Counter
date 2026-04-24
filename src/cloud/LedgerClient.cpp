#include "cloud/Cloud.h"
#include "BuildProfile.h"
#include "power/ConnectivityPolicy.h"

namespace {

bool ledgerHasConfigContent(const LedgerData &ledger) {
    return ledger.has("sensor") ||
           ledger.has("timing") ||
           ledger.has("messaging") ||
           ledger.has("modes") ||
           ledger.has("reporting");
}

} // namespace

#if defined(ALERT44_DIAG_ENABLED)
static void logLedgerSyncDiag(bool connected,
                              unsigned long nowMs,
                              unsigned long connectionWindowStartMs,
                              unsigned long elapsedSinceConnectMs,
                              unsigned long requiredWindowMs,
                              time_t defaultSync,
                              time_t deviceSync,
                              bool defaultSynced,
                              bool deviceSynced,
                              bool wasDisconnected,
                              bool returnValue,
                              const char *reason) {
    static unsigned long lastLedgerSyncDiagLogMs = 0;
    if ((nowMs - lastLedgerSyncDiagLogMs) < 2000UL) {
        return;
    }

    Log.info("LEDGER_SYNC_DIAG: start=%lu elapsed=%lu req=%lu def=%lu dev=%lu defOk=%d devOk=%d wasDisc=%d ret=%d reason=%s",
             connectionWindowStartMs,
             elapsedSinceConnectMs,
             requiredWindowMs,
             (unsigned long)defaultSync,
             (unsigned long)deviceSync,
             (int)defaultSynced,
             (int)deviceSynced,
             (int)wasDisconnected,
             (int)returnValue,
             reason);
    lastLedgerSyncDiagLogMs = nowMs;
}
#endif

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
    // A ledger is considered synced once its lastSynced() timestamp becomes non-zero.
    time_t defaultSync = defaultSettingsLedger.lastSynced();
    time_t deviceSync = deviceSettingsLedger.lastSynced();
    bool defaultSynced = (defaultSync > 0);
    bool deviceSynced = (deviceSync > 0);
    
    // Trace-level logging to avoid spam in main loop (called every iteration)
    Log.trace("Ledger sync check: default-settings=%lu device-settings=%lu", 
              (unsigned long)defaultSync, (unsigned long)deviceSync);
    
    // Track the current connection's sync window explicitly. The helper must not
    // reuse timing state from a previous sleep/connect cycle.
    static unsigned long firstConnectedTime = 0;
    static bool wasDisconnected = true;
    static time_t lastObservedConnectionEpoch = 0;
    unsigned long nowMs = millis();
    
    if (Particle.connected()) {
        time_t currentConnectionEpoch = sysStatus.get_lastConnection();
        bool newConnectionObserved = wasDisconnected ||
                                   (currentConnectionEpoch != 0 && currentConnectionEpoch != lastObservedConnectionEpoch);

        if (newConnectionObserved) {
            firstConnectedTime = nowMs;
            wasDisconnected = false;
            lastObservedConnectionEpoch = currentConnectionEpoch;
            Log.info("Connected - starting %lu ms ledger sync window", 
                     ConnectivityPolicy::LEDGER_SYNC_TIMEOUT_MS);
        }

        // If both ledgers are already synced for this connection, do not force the
        // caller to wait out the remaining window. This is the key Alert 44 fix.
        if (defaultSynced && deviceSynced) {
#if defined(ALERT44_DIAG_ENABLED)
            logLedgerSyncDiag(true,
                              nowMs,
                              firstConnectedTime,
                              nowMs - firstConnectedTime,
                              ConnectivityPolicy::LEDGER_SYNC_TIMEOUT_MS,
                              defaultSync,
                              deviceSync,
                              defaultSynced,
                              deviceSynced,
                              wasDisconnected,
                              true,
                              "BOTH_SYNCED");
#endif
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
                Log.info("default-settings synced and device-settings is empty after %lu ms - assuming no device overrides",
                         elapsedSinceConnect);
#if defined(ALERT44_DIAG_ENABLED)
                logLedgerSyncDiag(true,
                                  nowMs,
                                  firstConnectedTime,
                                  elapsedSinceConnect,
                                  ConnectivityPolicy::LEDGER_SYNC_TIMEOUT_MS,
                                  defaultSync,
                                  deviceSync,
                                  defaultSynced,
                                  deviceSynced,
                                  wasDisconnected,
                                  true,
                                  "DEFAULT_SYNCED_DEVICE_EMPTY");
#endif
                return true;
            }

            if (!defaultSynced && deviceSynced && !defaultHasConfig) {
                Log.info("device-settings synced and default-settings is empty after %lu ms - assuming no product defaults",
                         elapsedSinceConnect);
#if defined(ALERT44_DIAG_ENABLED)
                logLedgerSyncDiag(true,
                                  nowMs,
                                  firstConnectedTime,
                                  elapsedSinceConnect,
                                  ConnectivityPolicy::LEDGER_SYNC_TIMEOUT_MS,
                                  defaultSync,
                                  deviceSync,
                                  defaultSynced,
                                  deviceSynced,
                                  wasDisconnected,
                                  true,
                                  "DEVICE_SYNCED_DEFAULT_EMPTY");
#endif
                return true;
            }

            // If either ledger has synced, both must sync
            if (defaultSynced || deviceSynced) {
                bool bothSynced = (defaultSynced && deviceSynced);
                if (!bothSynced) {
                    Log.warn("Partial ledger sync after %lu ms: default=%lu device=%lu", 
                             elapsedSinceConnect,
                             (unsigned long)defaultSync, (unsigned long)deviceSync);
#if defined(ALERT44_DIAG_ENABLED)
                    logLedgerSyncDiag(true,
                                      nowMs,
                                      firstConnectedTime,
                                      elapsedSinceConnect,
                                      ConnectivityPolicy::LEDGER_SYNC_TIMEOUT_MS,
                                      defaultSync,
                                      deviceSync,
                                      defaultSynced,
                                      deviceSynced,
                                      wasDisconnected,
                                      false,
                                      defaultSynced ? "PARTIAL_DEFAULT_ONLY" : "PARTIAL_DEVICE_ONLY");
#endif
                } else {
#if defined(ALERT44_DIAG_ENABLED)
                    logLedgerSyncDiag(true,
                                      nowMs,
                                      firstConnectedTime,
                                      elapsedSinceConnect,
                                      ConnectivityPolicy::LEDGER_SYNC_TIMEOUT_MS,
                                      defaultSync,
                                      deviceSync,
                                      defaultSynced,
                                      deviceSynced,
                                      wasDisconnected,
                                      true,
                                      "BOTH_SYNCED");
#endif
                }
                return bothSynced;
            }
            // If neither has synced after timeout, assume they're empty and that's okay
            Log.info("No ledger data after %lu ms - assuming empty ledgers (OK)", elapsedSinceConnect);
#if defined(ALERT44_DIAG_ENABLED)
            logLedgerSyncDiag(true,
                              nowMs,
                              firstConnectedTime,
                              elapsedSinceConnect,
                              ConnectivityPolicy::LEDGER_SYNC_TIMEOUT_MS,
                              defaultSync,
                              deviceSync,
                              defaultSynced,
                              deviceSynced,
                              wasDisconnected,
                              true,
                              "BOTH_ZERO_ASSUME_EMPTY");
#endif
            return true;
        }
        // Still within the sync window
        Log.trace("Ledger sync pending: %lu ms elapsed (waiting for %lu ms)", 
                  elapsedSinceConnect, ConnectivityPolicy::LEDGER_SYNC_TIMEOUT_MS);
#if defined(ALERT44_DIAG_ENABLED)
        logLedgerSyncDiag(true,
                          nowMs,
                          firstConnectedTime,
                          elapsedSinceConnect,
                          ConnectivityPolicy::LEDGER_SYNC_TIMEOUT_MS,
                          defaultSync,
                          deviceSync,
                          defaultSynced,
                          deviceSynced,
                          wasDisconnected,
                          false,
                          "WINDOW_WAIT");
#endif
        return false;
    } else {
        // Disconnected - reset for next connection
#if defined(ALERT44_DIAG_ENABLED)
        logLedgerSyncDiag(false,
                          nowMs,
                          firstConnectedTime,
                          0,
                          ConnectivityPolicy::LEDGER_SYNC_TIMEOUT_MS,
                          defaultSync,
                          deviceSync,
                          defaultSynced,
                          deviceSynced,
                          wasDisconnected,
                          false,
                          "NOT_CONNECTED");
#endif
        wasDisconnected = true;
        lastObservedConnectionEpoch = 0;
        return false;
    }
}
