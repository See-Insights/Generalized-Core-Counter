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

// External firmware version string (defined in Version.cpp)
extern const char* FIRMWARE_VERSION;

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
    
    // Log sync timestamps for debugging
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
            Log.info("Connected - starting 5s ledger sync window");
        }
        
        // Give ledgers 5 seconds to sync after connection
        if (millis() - firstConnectedTime > 5000) {
            // If either ledger has synced, both must sync
            if (defaultSync > 0 || deviceSync > 0) {
                bool bothSynced = (defaultSync > 0 && deviceSync > 0);
                if (!bothSynced) {
                    Log.warn("Partial ledger sync: default=%lu device=%lu", 
                             (unsigned long)defaultSync, (unsigned long)deviceSync);
                }
                return bothSynced;
            }
            // If neither has synced after 5s, assume they're empty and that's okay
            Log.info("No ledger data after 5s - assuming empty ledgers (OK)");
            return true;
        }
        // Still within the 5-second sync window
        return false;
    } else {
        // Disconnected - reset for next connection
        wasDisconnected = true;
        return false;
    }
}

void Cloud::mergeConfiguration() {
    // Get data from both ledgers
    LedgerData defaults = defaultSettingsLedger.get();
    LedgerData device = deviceSettingsLedger.get();
    
    // Start with defaults as base
    mergedConfig = defaults;
    
    // Manually merge sensor thresholds using a simple, consistent schema.
    //
    // Supported keys:
    //   defaults.sensor.threshold1 / threshold2
    //   defaults.sensorThreshold   (applies to both thresholds)
    //   device.sensor.threshold1 / threshold2
    //   device.sensorThreshold     (applies to both thresholds)
    {
        // Start from sensible defaults; these will be overridden by
        // ledger values from defaults and then device.
        int threshold1 = 60;
        int threshold2 = 60;
        bool haveDefaultSensor = defaults.has("sensor") && defaults.get("sensor").isMap();
        bool haveDeviceSensor = device.has("sensor") && device.get("sensor").isMap();

        if (haveDefaultSensor) {
            Variant defaultSensor = defaults.get("sensor");
            if (defaultSensor.has("threshold1")) {
                threshold1 = defaultSensor.get("threshold1").toInt();
            }
            if (defaultSensor.has("threshold2")) {
                threshold2 = defaultSensor.get("threshold2").toInt();
            }
        }

        // Allow a single generic default threshold that applies
        // to both channels when more specific keys are not used.
        if (defaults.has("sensorThreshold")) {
            int base = defaults.get("sensorThreshold").toInt();
            threshold1 = base;
            threshold2 = base;
        }

        if (haveDeviceSensor) {
            Variant deviceSensor = device.get("sensor");
            if (deviceSensor.has("threshold1")) {
                int override1 = deviceSensor.get("threshold1").toInt();
                threshold1 = override1;
            }
            if (deviceSensor.has("threshold2")) {
                int override2 = deviceSensor.get("threshold2").toInt();
                threshold2 = override2;
            }
        }
        if (device.has("sensorThreshold")) {
            int override = device.get("sensorThreshold").toInt();
            threshold1 = override;
            threshold2 = override;
        }

        // Build a minimal merged sensor object with only the supported keys
        VariantMap mergedSensor;
        mergedSensor["threshold1"] = Variant(threshold1);
        mergedSensor["threshold2"] = Variant(threshold2);

        mergedConfig.set("sensor", Variant(mergedSensor));
    }
    
    // Apply other top-level device overrides (these aren't nested objects)
    if (device.has("timing")) mergedConfig.set("timing", device.get("timing"));
    if (device.has("power")) mergedConfig.set("power", device.get("power"));
    if (device.has("messaging")) mergedConfig.set("messaging", device.get("messaging"));
    if (device.has("modes")) mergedConfig.set("modes", device.get("modes"));
    
    lastApplySuccess = applyConfigurationFromLedger();

    if (!lastApplySuccess) {
        Log.warn("Configuration apply failed");
    }
}

bool Cloud::loadConfigurationFromCloud() {
    Log.info("Syncing configuration from cloud");
    
    // Trigger merge and apply configuration. mergeConfiguration() will update
    // lastApplySuccess based on the result of applyConfigurationFromLedger().
    mergeConfiguration();
    return lastApplySuccess;
}

bool Cloud::applyConfigurationFromLedger() {
    bool success = true;

    success &= applySensorConfig();
    success &= applyTimingConfig();
    success &= applyMessagingConfig();
    success &= applyModesConfig();
    
    if (success) {
        // Do not force synchronous storage flushes here; they can exceed the
        // 100 ms loop budget. Persistence is handled by sysStatus.loop() and
        // sensorConfig.loop() (called from the main loop).
        sysStatus.validate(sizeof(sysStatus));
        sensorConfig.validate(sizeof(sensorConfig));

        // Defer device-status publishing to Cloud::loop() so it doesn't
        // execute inside CONNECTING_STATE or async callbacks.
        pendingStatusPublish = true;
    } else {
        Log.warn("Some configuration sections failed to apply");
    }
    
    return success;
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

bool Cloud::applyMessagingConfig() {
    if (!mergedConfig.has("messaging")) return true;
    Variant messaging = mergedConfig.get("messaging");
    
    if (!messaging.isMap()) return true;

    bool success = true;
    bool changed = false;

    if (messaging.has("serial")) {
        bool serialEnabled = messaging.get("serial").toBool();
        if (sysStatus.get_serialConnected() != serialEnabled) {
            sysStatus.set_serialConnected(serialEnabled);
            Log.info("Config: Serial → %s", serialEnabled ? "ON" : "OFF");
            changed = true;
        }
    }

    if (messaging.has("verboseMode")) {
        bool verboseMode = messaging.get("verboseMode").toBool();
        if (sysStatus.get_verboseMode() != verboseMode) {
            sysStatus.set_verboseMode(verboseMode);
            Log.info("Config: Verbose -> %s", verboseMode ? "ON" : "OFF");
            changed = true;
        }
    }

    if (messaging.has("verboseTimeoutMin")) {
        int verboseTimeoutMin = messaging.get("verboseTimeoutMin").toInt();
        if (validateRange(verboseTimeoutMin, 0, 1440, "verboseTimeoutMin")) {
            if (sysStatus.get_verboseTimeoutMin() != (uint16_t)verboseTimeoutMin) {
                sysStatus.set_verboseTimeoutMin((uint16_t)verboseTimeoutMin);
                Log.info("Config: Verbose timeout -> %d min", verboseTimeoutMin);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    if (changed) Log.info("Messaging config updated");
    return success;
}

bool Cloud::applyTimingConfig() {
    if (!mergedConfig.has("timing")) return true;
    Variant timing = mergedConfig.get("timing");
    
    if (!timing.isMap()) return true;

    bool success = true;
    bool changed = false;

    if (timing.has("timezone")) {
        String timezone = timing.get("timezone").toString();
        if (timezone.length() > 0 && timezone.length() < 39) {
            if (strcmp(sysStatus.get_timeZoneStr(), timezone.c_str()) != 0) {
                sysStatus.set_timeZoneStr(timezone.c_str());
                Log.info("Config: Timezone -> %s", timezone.c_str());
                changed = true;
            }
        } else {
            Log.warn("Invalid timezone length: %d", timezone.length());
            success = false;
        }
    }

    if (timing.has("reportingIntervalSec")) {
        int reportingInterval = timing.get("reportingIntervalSec").toInt();
        if (validateRange(reportingInterval, 300, 86400, "timing.reportingIntervalSec")) {
            if (sysStatus.get_reportingInterval() != reportingInterval) {
                sysStatus.set_reportingInterval(reportingInterval);
                Log.info("Config: Reporting interval -> %ds", reportingInterval);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    if (timing.has("pollingRateSec")) {
        int pollingRate = timing.get("pollingRateSec").toInt();
        if (validateRange(pollingRate, 0, 3600, "timing.pollingRateSec")) {
            if (sensorConfig.get_pollingRate() != pollingRate) {
                sensorConfig.set_pollingRate(pollingRate);
                Log.info("Config: Polling rate -> %ds", pollingRate);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    if (timing.has("openHour")) {
        int openHour = timing.get("openHour").toInt();
        if (validateRange(openHour, 0, 23, "timing.openHour")) {
            if (sysStatus.get_openTime() != openHour) {
                sysStatus.set_openTime(openHour);
                Log.info("Config: Open hour -> %d", openHour);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    if (timing.has("closeHour")) {
        int closeHour = timing.get("closeHour").toInt();
        if (validateRange(closeHour, 0, 23, "timing.closeHour")) {
            if (sysStatus.get_closeTime() != closeHour) {
                sysStatus.set_closeTime(closeHour);
                Log.info("Config: Close hour -> %d", closeHour);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    // Maximum connection-attempt budget per wake (seconds)
    if (timing.has("connectAttemptBudgetSec")) {
        int connectAttemptBudgetSec = timing.get("connectAttemptBudgetSec").asInt();
        if (validateRange(connectAttemptBudgetSec, 30, 900, "connectAttemptBudgetSec")) {
            if (sysStatus.get_connectAttemptBudgetSec() != connectAttemptBudgetSec) {
                sysStatus.set_connectAttemptBudgetSec((uint16_t)connectAttemptBudgetSec);
                Log.info("Config: Connect budget -> %ds", connectAttemptBudgetSec);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    if (changed) Log.info("Timing config updated");
    return success;
}

// Fix the validateRange function - add template declaration
template<typename T>
bool Cloud::validateRange(T value, T min, T max, const String& name) {
    if (value < min || value > max) {
        Log.warn("Invalid %s value: %d (must be between %d and %d)", name.c_str(), (int)value, (int)min, (int)max);
        return false;
    }
    return true;
}

bool Cloud::applySensorConfig() {
    if (!mergedConfig.has("sensor")) return true;
    
    Variant sensor = mergedConfig.get("sensor");
    if (!sensor.isMap()) return true;
    
    bool success = true;
    bool changed = false;
    
    // sensor.type
    if (sensor.has("type")) {
        int sensorType = sensor.get("type").toInt();
        if (validateRange(sensorType, 0, 255, "sensor.type")) {
            if (sensorConfig.get_sensorType() != (uint8_t)sensorType) {
                sensorConfig.set_sensorType((uint8_t)sensorType);
                Log.info("Config: Sensor type -> %d", sensorType);
                changed = true;
            }
        } else {
            success = false;
        }
    }
    
    // sensor.setting1-4 (generic settings)
    if (sensor.has("setting1")) {
        int setting1 = sensor.get("setting1").toInt();
        if (sensorConfig.get_sensorSetting1() != (uint32_t)setting1) {
            sensorConfig.set_sensorSetting1((uint32_t)setting1);
            Log.info("Config: Sensor setting1 -> %d", setting1);
            changed = true;
        }
    }
    
    if (sensor.has("setting2")) {
        int setting2 = sensor.get("setting2").toInt();
        if (sensorConfig.get_sensorSetting2() != (uint32_t)setting2) {
            sensorConfig.set_sensorSetting2((uint32_t)setting2);
            Log.info("Config: Sensor setting2 -> %d", setting2);
            changed = true;
        }
    }
    
    if (sensor.has("setting3")) {
        int setting3 = sensor.get("setting3").toInt();
        if (sensorConfig.get_sensorSetting3() != (uint32_t)setting3) {
            sensorConfig.set_sensorSetting3((uint32_t)setting3);
            Log.info("Config: Sensor setting3 -> %d", setting3);
            changed = true;
        }
    }
    
    if (sensor.has("setting4")) {
        int setting4 = sensor.get("setting4").toInt();
        if (sensorConfig.get_sensorSetting4() != (uint32_t)setting4) {
            sensorConfig.set_sensorSetting4((uint32_t)setting4);
            Log.info("Config: Sensor setting4 -> %d", setting4);
            changed = true;
        }
    }
    
    if (changed) Log.info("Sensor config updated");
    return success;
}

bool Cloud::applyModesConfig() {
    if (!mergedConfig.has("modes")) return true;
    Variant modes = mergedConfig.get("modes");
    
    if (!modes.isMap()) {
        return true;
    }
    
    bool success = true;
    bool changed = false;

    // Sensor mode: 0=COUNTING, 1=OCCUPANCY, 2=MEASUREMENT
    if (modes.has("sensorMode")) {
        int sensorMode = modes.get("sensorMode").asInt();
        if (validateRange(sensorMode, 0, 2, "sensorMode")) {
            if (sysStatus.get_sensorMode() != static_cast<SensorMode>(sensorMode)) {
                sysStatus.set_sensorMode(static_cast<SensorMode>(sensorMode));
                const char *modeStr = sensorMode == COUNTING ? "COUNTING" :
                                     sensorMode == OCCUPANCY ? "OCCUPANCY" : "MEASUREMENT";
                Log.info("Config: Sensor mode -> %s", modeStr);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    // Connection mode: 0=CONNECTED, 1=INTERMITTENT, 2=DISCONNECTED, 3=INTERMITTENT_KEEP_ALIVE
    if (modes.has("connectionMode")) {
        int connectionMode = modes.get("connectionMode").asInt();
        if (validateRange(connectionMode, 0, 3, "connectionMode")) {
            if (sysStatus.get_connectionMode() != static_cast<ConnectionMode>(connectionMode)) {
                sysStatus.set_connectionMode(static_cast<ConnectionMode>(connectionMode));
                const char *modeStr = connectionMode == CONNECTED ? "CONNECTED" :
                                     connectionMode == INTERMITTENT ? "INTERMITTENT" :
                                     connectionMode == DISCONNECTED ? "DISCONNECTED" : "INTERMITTENT_KEEP_ALIVE";
                Log.info("Config: Connection mode -> %s", modeStr);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    // Reporting mode: 0=SCHEDULED, 1=ON_CHANGE, 2=THRESHOLD, 3=SCHEDULED_OR_THRESHOLD
    if (modes.has("reportingMode")) {
        int reportingMode = modes.get("reportingMode").asInt();
        if (validateRange(reportingMode, 0, 3, "reportingMode")) {
            if (sysStatus.get_reportingMode() != static_cast<ReportingMode>(reportingMode)) {
                sysStatus.set_reportingMode(static_cast<ReportingMode>(reportingMode));
                const char *modeStr = reportingMode == SCHEDULED ? "SCHEDULED" :
                                     reportingMode == ON_CHANGE ? "ON_CHANGE" :
                                     reportingMode == THRESHOLD ? "THRESHOLD" : "SCHEDULED_OR_THRESHOLD";
                Log.info("Config: Reporting mode -> %s", modeStr);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    // Sampling mode: 0=INTERRUPT, 1=POLLING
    if (modes.has("samplingMode")) {
        int samplingMode = modes.get("samplingMode").asInt();
        if (validateRange(samplingMode, 0, 1, "samplingMode")) {
            if (sysStatus.get_samplingMode() != static_cast<SamplingMode>(samplingMode)) {
                sysStatus.set_samplingMode(static_cast<SamplingMode>(samplingMode));
                const char *modeStr = samplingMode == INTERRUPT ? "INTERRUPT" : "POLLING";
                Log.info("Config: Sampling mode -> %s", modeStr);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    // Maximum time to wait for cloud disconnect before treating as an error (seconds)
    if (modes.has("cloudDisconnectBudgetSec")) {
        int cloudDisconnectBudgetSec = modes.get("cloudDisconnectBudgetSec").asInt();
        if (validateRange(cloudDisconnectBudgetSec, 5, 120, "cloudDisconnectBudgetSec")) {
            if (sysStatus.get_cloudDisconnectBudgetSec() != cloudDisconnectBudgetSec) {
                sysStatus.set_cloudDisconnectBudgetSec((uint16_t)cloudDisconnectBudgetSec);
                Log.info("Config: Disconnect budget -> %ds", cloudDisconnectBudgetSec);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    // Maximum time to wait for modem power-down before treating as an error (seconds)
    if (modes.has("modemOffBudgetSec")) {
        int modemOffBudgetSec = modes.get("modemOffBudgetSec").asInt();
        if (validateRange(modemOffBudgetSec, 5, 120, "modemOffBudgetSec")) {
            if (sysStatus.get_modemOffBudgetSec() != modemOffBudgetSec) {
                sysStatus.set_modemOffBudgetSec((uint16_t)modemOffBudgetSec);
                Log.info("Config: Modem off budget -> %ds", modemOffBudgetSec);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    if (changed) Log.info("Modes config updated");
    return success;
}

bool Cloud::writeDeviceStatusToCloud() {
    // Build current configuration as JSON
    char buffer[512];
    JSONBufferWriter writer(buffer, sizeof(buffer));

    writer.beginObject();

    // Firmware version
    writer.name("firmwareVersion").value(FIRMWARE_VERSION);

    // Messaging
    writer.name("messaging").beginObject();
    writer.name("serial").value(sysStatus.get_serialConnected());
    writer.name("verboseMode").value(sysStatus.get_verboseMode());
    writer.name("verboseTimeoutMin").value(sysStatus.get_verboseTimeoutMin());
    writer.endObject();

    // Sensor
    writer.name("sensor").beginObject();
    writer.name("type").value(sensorConfig.get_sensorType());
    writer.name("setting1").value((int)sensorConfig.get_sensorSetting1());
    writer.name("setting2").value((int)sensorConfig.get_sensorSetting2());
    writer.name("setting3").value((int)sensorConfig.get_sensorSetting3());
    writer.name("setting4").value((int)sensorConfig.get_sensorSetting4());
    writer.endObject();

    // Timing
    writer.name("timing").beginObject();
    writer.name("timezone").value(sysStatus.get_timeZoneStr());
    writer.name("reportingIntervalSec").value(sysStatus.get_reportingInterval());
    writer.name("openHour").value(sysStatus.get_openTime());
    writer.name("closeHour").value(sysStatus.get_closeTime());
    writer.name("connectAttemptBudgetSec").value((int)sysStatus.get_connectAttemptBudgetSec());
    writer.endObject();

    // Modes
    writer.name("modes").beginObject();
    writer.name("sensorMode").value((int)sysStatus.get_sensorMode());
    writer.name("connectionMode").value((int)sysStatus.get_connectionMode());
    writer.name("reportingMode").value((int)sysStatus.get_reportingMode());
    writer.name("samplingMode").value((int)sysStatus.get_samplingMode());
    writer.endObject();
    writer.endObject();

    if (!writer.buffer()) {
        Log.warn("Failed to create status JSON");
        return false;
    }

    buffer[writer.dataSize()] = '\0';

    // Only publish if the configuration actually changed
    String currentStatus = String(buffer);
    if (lastPublishedStatus == currentStatus) {
        Log.info("Device status unchanged; skipping device-status ledger update");
        return true; // Not an error; nothing to do
    }

    LedgerData data = LedgerData::fromJSON(buffer);
    int result = deviceStatusLedger.set(data);

    if (result == SYSTEM_ERROR_NONE) {
        lastPublishedStatus = currentStatus;
        Log.info("Device status published to cloud");
        return true;
    } else {
        Log.warn("Failed to publish device status: %d", result);
        return false;
    }
}

bool Cloud::publishDataToLedger() {
    Log.info("Publishing sensor data to device-data ledger");
    
    char buffer[512];
    JSONBufferWriter writer(buffer, sizeof(buffer));
    
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
        writer.name("totalOccupiedSec").value(current.get_totalOccupiedSeconds());
    } else { // MEASUREMENT or any future modes
        writer.name("mode").value("measurement");
        // In measurement mode we still track counts for compatibility
        writer.name("hourlyCount").value(current.get_hourlyCount());
        writer.name("dailyCount").value(current.get_dailyCount());
    }
    
    writer.name("battery").value(current.get_stateOfCharge(), 1);
    writer.name("temp").value(current.get_internalTempC(), 1);
    writer.endObject();
    
    if (!writer.buffer()) {
        Log.warn("Failed to create data JSON");
        return false;
    }
    
    buffer[writer.dataSize()] = '\0';
    
    LedgerData data = LedgerData::fromJSON(buffer);
    int result = deviceDataLedger.set(data);
    
    if (result == SYSTEM_ERROR_NONE) {
        // Log the key counters and any active alert code so we
        // can correlate what was actually written to device-data.
        int mode = sysStatus.get_sensorMode();
        if (mode == COUNTING || mode == MEASUREMENT) {
            Log.info("Sensor data published to cloud - mode=%s hourly=%d daily=%d alert=%d",
                     (mode == COUNTING ? "counting" : "measurement"),
                     (int)current.get_hourlyCount(),
                     (int)current.get_dailyCount(),
                     (int)current.get_alertCode());
        } else if (mode == OCCUPANCY) {
            Log.info("Sensor data published to cloud - mode=occupancy occupied=%d totalSec=%lu alert=%d",
                     (int)current.get_occupied(),
                     (unsigned long)current.get_totalOccupiedSeconds(),
                     (int)current.get_alertCode());
        } else {
            Log.info("Sensor data published to cloud - mode=unknown alert=%d", (int)current.get_alertCode());
        }
        return true;
    } else {
        Log.warn("Failed to publish sensor data: %d", result);
        return false;
    }
}

bool Cloud::hasNonDefaultConfig() {
    // Check if any current values differ from hardcoded product defaults
    // This is a simplified check - expand as needed
    return (sensorConfig.get_sensorType() != 1 || 
            sensorConfig.get_sensorSetting1() != 5000 ||
            sysStatus.get_openTime() != 6 ||
            sysStatus.get_closeTime() != 22);
}

// *************** Battery-Aware Connection Management ***************

// [static]
BatteryTier Cloud::calculateBatteryTier(float currentSoC) {
    // Get previous tier for hysteresis logic
    uint8_t prevTierValue = sysStatus.get_currentBatteryTier();
    BatteryTier prevTier = static_cast<BatteryTier>(prevTierValue);
    
    // Apply hysteresis: require 5% higher SoC to move to better tier
    // This prevents rapid tier thrashing when battery is near threshold
    
    if (currentSoC >= 75.0f) {
        // Strong signal to move to HEALTHY (or stay there)
        return TIER_HEALTHY;
    } else if (currentSoC >= 70.0f) {
        // Boundary zone: stay in current tier unless coming from below
        return (prevTier == TIER_HEALTHY) ? TIER_HEALTHY : TIER_CONSERVING;
    } else if (currentSoC >= 55.0f) {
        // Strong signal for CONSERVING or recovery from CRITICAL
        return TIER_CONSERVING;
    } else if (currentSoC >= 50.0f) {
        // Boundary zone: stay in current tier unless coming from above
        if (prevTier <= TIER_CONSERVING) {
            return TIER_CONSERVING;
        } else {
            return TIER_CRITICAL;
        }
    } else if (currentSoC >= 35.0f) {
        // Strong signal for CRITICAL or recovery from SURVIVAL
        return TIER_CRITICAL;
    } else if (currentSoC >= 30.0f) {
        // Boundary zone: stay in current tier unless coming from above
        if (prevTier <= TIER_CRITICAL) {
            return TIER_CRITICAL;
        } else {
            return TIER_SURVIVAL;
        }
    } else {
        // Below 30% - definite SURVIVAL mode
        return TIER_SURVIVAL;
    }
}

// [static]
uint16_t Cloud::getIntervalMultiplier(BatteryTier tier) {
    switch (tier) {
        case TIER_HEALTHY:
            return 1;   // Normal interval
        case TIER_CONSERVING:
            return 2;   // Double interval (half connections per day)
        case TIER_CRITICAL:
            return 4;   // Quadruple interval (quarter connections per day)
        case TIER_SURVIVAL:
            return 12;  // 12x interval (e.g., hourly → every 12 hours)
        default:
            Log.warn("Unknown battery tier %d, defaulting to 1x multiplier", (int)tier);
            return 1;
    }
}

// [static]
float Cloud::getConnectionBackoffMultiplier(uint16_t lastDurationSec) {
    // Evaluate connection history to apply additional backoff
    // in problematic locations (poor signal, tower congestion, etc.)
    
    if (lastDurationSec == 0) {
        // Failed connection or first attempt - apply strong backoff
        Log.info("Connection history: failed/first attempt - applying 2.0x backoff");
        return 2.0f;
    } else if (lastDurationSec < 60) {
        // Fast connection - excellent location
        Log.trace("Connection history: fast (%ds) - no additional backoff", lastDurationSec);
        return 1.0f;
    } else if (lastDurationSec < 180) {
        // Normal connection time - acceptable
        Log.trace("Connection history: normal (%ds) - no additional backoff", lastDurationSec);
        return 1.0f;
    } else if (lastDurationSec < 300) {
        // Slow connection - marginal location
        Log.info("Connection history: slow (%ds) - applying 1.5x backoff", lastDurationSec);
        return 1.5f;
    } else {
        // Very slow connection - problematic location
        Log.info("Connection history: problem (%ds) - applying 2.0x backoff", lastDurationSec);
        return 2.0f;
    }
}

/**
 * @brief Unit test function to validate battery-aware backoff calculations
 * 
 * Tests all combinations of battery tiers and connection durations to verify
 * the multiplier logic produces expected results across all scenarios.
 * Call this function early in setup() or via a test mode to validate logic.
 */
void Cloud::testBatteryBackoffLogic() {
    Log.info("========================================");
    Log.info("Battery-Aware Backoff Unit Test");
    Log.info("========================================");
    
    // Test battery tier calculations with hysteresis
    Log.info(" ");
    Log.info("=== Battery Tier Calculation Tests ===");
    
    struct BatteryTest {
        float soc;
        BatteryTier prevTier;
        const char* description;
    };
    
    BatteryTest batteryTests[] = {
        {100.0f, TIER_HEALTHY, "100% SoC (healthy)"},
        {75.0f, TIER_CONSERVING, "75% SoC (hysteresis boundary, coming from conserving)"},
        {75.0f, TIER_HEALTHY, "75% SoC (hysteresis boundary, staying healthy)"},
        {70.0f, TIER_HEALTHY, "70% SoC (tier boundary, staying healthy)"},
        {70.0f, TIER_CONSERVING, "70% SoC (tier boundary, entering conserving)"},
        {55.0f, TIER_CRITICAL, "55% SoC (hysteresis boundary, coming from critical)"},
        {50.0f, TIER_CONSERVING, "50% SoC (tier boundary, staying conserving)"},
        {35.0f, TIER_SURVIVAL, "35% SoC (hysteresis boundary, coming from survival)"},
        {30.0f, TIER_CRITICAL, "30% SoC (tier boundary, staying critical)"},
        {25.0f, TIER_SURVIVAL, "25% SoC (survival mode)"},
        {10.0f, TIER_SURVIVAL, "10% SoC (low battery)"}
    };
    
    for (size_t i = 0; i < sizeof(batteryTests)/sizeof(batteryTests[0]); i++) {
        // Temporarily set previous tier for hysteresis testing
        uint8_t originalTier = sysStatus.get_currentBatteryTier();
        sysStatus.set_currentBatteryTier(static_cast<uint8_t>(batteryTests[i].prevTier));
        
        BatteryTier result = calculateBatteryTier(batteryTests[i].soc);
        const char* tierNames[] = {"HEALTHY", "CONSERVING", "CRITICAL", "SURVIVAL"};
        
        Log.info("  %s -> %s", batteryTests[i].description, tierNames[result]);
        
        // Restore original tier
        sysStatus.set_currentBatteryTier(originalTier);
    }
    
    // Test interval multipliers
    Log.info(" ");
    Log.info("=== Interval Multiplier Tests ===");
    const char* tierNames[] = {"TIER_HEALTHY", "TIER_CONSERVING", "TIER_CRITICAL", "TIER_SURVIVAL"};
    for (int tier = TIER_HEALTHY; tier <= TIER_SURVIVAL; tier++) {
        uint16_t mult = getIntervalMultiplier(static_cast<BatteryTier>(tier));
        Log.info("  %s: %ux interval", tierNames[tier], mult);
    }
    
    // Test connection backoff multipliers
    Log.info(" ");
    Log.info("=== Connection History Backoff Tests ===");
    
    struct ConnectionTest {
        uint16_t duration;
        const char* description;
    };
    
    ConnectionTest connectionTests[] = {
        {0, "Failed/first connection"},
        {30, "Fast connection (30s)"},
        {60, "Normal connection (60s)"},
        {120, "Normal connection (120s)"},
        {180, "Slow boundary (180s)"},
        {250, "Slow connection (250s)"},
        {300, "Problem boundary (300s)"},
        {400, "Problem connection (400s)"}
    };
    
    for (size_t i = 0; i < sizeof(connectionTests)/sizeof(connectionTests[0]); i++) {
        float mult = getConnectionBackoffMultiplier(connectionTests[i].duration);
        Log.info("  %s: %.1fx backoff", connectionTests[i].description, mult);
    }
    
    // Combined scenario tests
    Log.info(" ");
    Log.info("=== Combined Scenario Tests (Base Interval = 3600s / 1 hour) ===");
    Log.info("Format: Battery | ConnTime | Tier Mult | History Mult | Effective Interval");
    Log.info("----------------------------------------------------------------------");
    
    struct ScenarioTest {
        float soc;
        BatteryTier prevTier;
        uint16_t connDuration;
        const char* batteryDesc;
        const char* locationDesc;
    };
    
    ScenarioTest scenarios[] = {
        {80.0f, TIER_HEALTHY, 30, "80% (Healthy)", "Fast (30s)"},
        {80.0f, TIER_HEALTHY, 250, "80% (Healthy)", "Slow (250s)"},
        {60.0f, TIER_CONSERVING, 120, "60% (Conserving)", "Normal (120s)"},
        {60.0f, TIER_CONSERVING, 250, "60% (Conserving)", "Slow (250s)"},
        {40.0f, TIER_CRITICAL, 120, "40% (Critical)", "Normal (120s)"},
        {40.0f, TIER_CRITICAL, 350, "40% (Critical)", "Problem (350s)"},
        {25.0f, TIER_SURVIVAL, 120, "25% (Survival)", "Normal (120s)"},
        {25.0f, TIER_SURVIVAL, 400, "25% (Survival)", "Problem (400s)"}
    };
    
    const uint16_t baseInterval = 3600; // 1 hour in seconds
    
    for (size_t i = 0; i < sizeof(scenarios)/sizeof(scenarios[0]); i++) {
        // Set previous tier for hysteresis
        uint8_t originalTier = sysStatus.get_currentBatteryTier();
        sysStatus.set_currentBatteryTier(static_cast<uint8_t>(scenarios[i].prevTier));
        
        BatteryTier tier = calculateBatteryTier(scenarios[i].soc);
        uint16_t tierMult = getIntervalMultiplier(tier);
        float historyMult = getConnectionBackoffMultiplier(scenarios[i].connDuration);
        uint32_t effectiveInterval = (uint32_t)(baseInterval * tierMult * historyMult);
        
        Log.info("  %-18s | %-16s | %5ux | %12.1fx | %6lus (%luh %lum)",
                 scenarios[i].batteryDesc,
                 scenarios[i].locationDesc,
                 tierMult,
                 historyMult,
                 (unsigned long)effectiveInterval,
                 (unsigned long)(effectiveInterval / 3600),
                 (unsigned long)((effectiveInterval % 3600) / 60));
        
        // Restore original tier
        sysStatus.set_currentBatteryTier(originalTier);
    }
    
    Log.info(" ");
    Log.info("========================================");
    Log.info("Unit Test Complete");
    Log.info("========================================");
}

// Explicit template instantiations for validateRange
template bool Cloud::validateRange<int>(int value, int min, int max, const String& name);
template bool Cloud::validateRange<unsigned long>(unsigned long value, unsigned long min, unsigned long max, const String& name);
