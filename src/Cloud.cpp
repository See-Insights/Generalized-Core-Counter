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
    Log.info("  default-settings: Product defaults (Cloud→Device)");
    Log.info("  device-settings: Device overrides (Cloud→Device)");
    Log.info("  device-status: Current config (Device→Cloud)");
    Log.info("  device-data: Sensor readings (Device→Cloud)");
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
    success &= applyPowerConfig();
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
            Log.info("Config: Verbose → %s", verboseMode ? "ON" : "OFF");
            changed = true;
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
                Log.info("Config: Timezone → %s", timezone.c_str());
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
                Log.info("Config: Reporting interval → %ds", reportingInterval);
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
                Log.info("Config: Polling rate → %ds", pollingRate);
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
                Log.info("Config: Open hour → %d", openHour);
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
                Log.info("Config: Close hour → %d", closeHour);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    if (changed) Log.info("Timing config updated");
    return success;
}

bool Cloud::applyPowerConfig() {
    if (!mergedConfig.has("power")) return true;
    Variant power = mergedConfig.get("power");
    
    if (!power.isMap()) return true;

    bool success = true;
    bool changed = false;

    if (power.has("solarPowerMode")) {
        bool solarPowerMode = power.get("solarPowerMode").toBool();
        if (sysStatus.get_solarPowerMode() != solarPowerMode) {
            sysStatus.set_solarPowerMode(solarPowerMode);
            Log.info("Config: Solar power → %s", solarPowerMode ? "ON" : "OFF");
            changed = true;
        }
    }

    if (changed) Log.info("Power config updated");
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
    
    // threshold1
    if (sensor.has("threshold1")) {
        int threshold1 = sensor.get("threshold1").toInt();
        if (validateRange(threshold1, 0, 100, "sensor.threshold1")) {
            if (sensorConfig.get_threshold1() != threshold1) {
                sensorConfig.set_threshold1(threshold1);
                Log.info("Config: Threshold1 → %d", threshold1);
                changed = true;
            }
        } else {
            success = false;
        }
    }
    
    // threshold2
    if (sensor.has("threshold2")) {
        int threshold2 = sensor.get("threshold2").toInt();
        if (validateRange(threshold2, 0, 100, "sensor.threshold2")) {
            if (sensorConfig.get_threshold2() != threshold2) {
                sensorConfig.set_threshold2(threshold2);
                Log.info("Config: Threshold2 → %d", threshold2);
                changed = true;
            }
        } else {
            success = false;
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

    // Counting mode: 0=COUNTING, 1=OCCUPANCY, 2=SCHEDULED (time-based)
    if (modes.has("countingMode")) {
        int countingMode = modes.get("countingMode").asInt();
        if (validateRange(countingMode, 0, 2, "countingMode")) {
            if (sysStatus.get_countingMode() != static_cast<CountingMode>(countingMode)) {
                sysStatus.set_countingMode(static_cast<CountingMode>(countingMode));
                const char *modeStr = countingMode == COUNTING ? "COUNTING" :
                                     countingMode == OCCUPANCY ? "OCCUPANCY" : "SCHEDULED";
                Log.info("Config: Counting mode → %s", modeStr);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    // Operating mode: 0=CONNECTED, 1=LOW_POWER, 2=DISCONNECTED
    if (modes.has("operatingMode")) {
        int operatingMode = modes.get("operatingMode").asInt();
        if (validateRange(operatingMode, 0, 2, "operatingMode")) {
            if (sysStatus.get_operatingMode() != static_cast<OperatingMode>(operatingMode)) {
                sysStatus.set_operatingMode(static_cast<OperatingMode>(operatingMode));
                const char *modeStr = operatingMode == 0 ? "CONNECTED" :
                                     operatingMode == 1 ? "LOW_POWER" : "DISCONNECTED";
                Log.info("Config: Operating mode → %s", modeStr);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    // Occupancy debounce time (milliseconds)
    if (modes.has("occupancyDebounceMs")) {
        unsigned long occupancyDebounceMs = modes.get("occupancyDebounceMs").asUInt();
        if (validateRange(occupancyDebounceMs, 0UL, 600000UL, "occupancyDebounceMs")) {
            if (sysStatus.get_occupancyDebounceMs() != occupancyDebounceMs) {
                sysStatus.set_occupancyDebounceMs(occupancyDebounceMs);
                Log.info("Config: Occupancy debounce → %lu ms", occupancyDebounceMs);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    // Connected mode reporting interval (seconds)
    if (modes.has("connectedReportingIntervalSec")) {
        int connectedReportingIntervalSec = modes.get("connectedReportingIntervalSec").asInt();
        if (validateRange(connectedReportingIntervalSec, 60, 86400, "connectedReportingIntervalSec")) {
            if (sysStatus.get_connectedReportingIntervalSec() != connectedReportingIntervalSec) {
                sysStatus.set_connectedReportingIntervalSec(connectedReportingIntervalSec);
                Log.info("Config: Connected reporting interval → %ds", connectedReportingIntervalSec);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    // Low power mode reporting interval (seconds)
    if (modes.has("lowPowerReportingIntervalSec")) {
        int lowPowerReportingIntervalSec = modes.get("lowPowerReportingIntervalSec").asInt();
        if (validateRange(lowPowerReportingIntervalSec, 300, 86400, "lowPowerReportingIntervalSec")) {
            if (sysStatus.get_lowPowerReportingIntervalSec() != lowPowerReportingIntervalSec) {
                sysStatus.set_lowPowerReportingIntervalSec(lowPowerReportingIntervalSec);
                Log.info("Config: Low power reporting interval → %ds", lowPowerReportingIntervalSec);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    // Maximum connection-attempt budget per wake (seconds)
    if (modes.has("connectAttemptBudgetSec")) {
        int connectAttemptBudgetSec = modes.get("connectAttemptBudgetSec").asInt();
        if (validateRange(connectAttemptBudgetSec, 30, 900, "connectAttemptBudgetSec")) {
            if (sysStatus.get_connectAttemptBudgetSec() != connectAttemptBudgetSec) {
                sysStatus.set_connectAttemptBudgetSec((uint16_t)connectAttemptBudgetSec);
                Log.info("Config: Connect budget → %ds", connectAttemptBudgetSec);
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
                Log.info("Config: Disconnect budget → %ds", cloudDisconnectBudgetSec);
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
                Log.info("Config: Modem off budget → %ds", modemOffBudgetSec);
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

    // Sensor
    writer.name("sensor").beginObject();
    writer.name("threshold1").value(sensorConfig.get_threshold1());
    writer.name("threshold2").value(sensorConfig.get_threshold2());
    writer.endObject();

    // Timing
    writer.name("timing").beginObject();
    writer.name("timezone").value(sysStatus.get_timeZoneStr());
    writer.name("reportingIntervalSec").value(sysStatus.get_reportingInterval());
    writer.name("openHour").value(sysStatus.get_openTime());
    writer.name("closeHour").value(sysStatus.get_closeTime());
    writer.endObject();

    // Power
    writer.name("power").beginObject();
    writer.name("lowPowerMode").value(sysStatus.get_lowPowerMode());
    writer.name("solarPowerMode").value(sysStatus.get_solarPowerMode());
    writer.endObject();

    // Modes
    writer.name("modes").beginObject();
    writer.name("countingMode").value((int)sysStatus.get_countingMode());
    writer.name("operatingMode").value((int)sysStatus.get_operatingMode());
    writer.name("occupancyDebounceMs").value((int)sysStatus.get_occupancyDebounceMs());
    writer.name("connectedReportingIntervalSec").value((int)sysStatus.get_connectedReportingIntervalSec());
    writer.name("lowPowerReportingIntervalSec").value((int)sysStatus.get_lowPowerReportingIntervalSec());
    writer.name("connectAttemptBudgetSec").value((int)sysStatus.get_connectAttemptBudgetSec());

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

    uint8_t countingMode = sysStatus.get_countingMode();

    if (countingMode == COUNTING) {
        writer.name("mode").value("counting");
        writer.name("hourlyCount").value(current.get_hourlyCount());
        writer.name("dailyCount").value(current.get_dailyCount());
    } else if (countingMode == OCCUPANCY) {
        writer.name("mode").value("occupancy");
        writer.name("occupied").value(current.get_occupied());
        writer.name("totalOccupiedSec").value(current.get_totalOccupiedSeconds());
    } else { // SCHEDULED or any future modes
        writer.name("mode").value("scheduled");
        // In scheduled mode we still track counts; include them so
        // device-data mirrors the webhook payload for analytics.
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
        int mode = sysStatus.get_countingMode();
        if (mode == COUNTING || mode == SCHEDULED) {
            Log.info("Sensor data published to cloud - mode=%s hourly=%d daily=%d alert=%d",
                     (mode == COUNTING ? "counting" : "scheduled"),
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
    return (sensorConfig.get_threshold1() != 60 || 
            sensorConfig.get_threshold2() != 60 ||
            sysStatus.get_openTime() != 6 ||
            sysStatus.get_closeTime() != 22);
}

// Explicit template instantiations for validateRange
template bool Cloud::validateRange<int>(int value, int min, int max, const String& name);
template bool Cloud::validateRange<unsigned long>(unsigned long value, unsigned long min, unsigned long max, const String& name);
