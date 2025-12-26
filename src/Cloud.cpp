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
#include "Version.h"

// Forward declarations in case Version.h is not picked up correctly
// by the build system in this translation unit.
extern const char* FIRMWARE_VERSION;
extern const char* FIRMWARE_RELEASE_NOTES;

Cloud *Cloud::_instance;

// [static]
Cloud &Cloud::instance() {
    if (!_instance) {
        _instance = new Cloud();
    }
    return *_instance;
}

Cloud::Cloud() : ledgersSynced(false) {
    lastPublishedStatus = "";
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

void Cloud::loop() {
    // No loop processing needed
}

// Static callbacks
void Cloud::onDefaultSettingsSync(Ledger ledger) {
    Log.info("default-settings synced from cloud");
    Cloud::instance().mergeConfiguration();
    Cloud::instance().ledgersSynced = true;
}

void Cloud::onDeviceSettingsSync(Ledger ledger) {
    Log.info("device-settings synced from cloud");
    Cloud::instance().mergeConfiguration();
    Cloud::instance().ledgersSynced = true;
}

void Cloud::mergeConfiguration() {
    Log.info("Merging default and device configurations");
    
    // Get data from both ledgers
    LedgerData defaults = defaultSettingsLedger.get();
    LedgerData device = deviceSettingsLedger.get();
    
    // Start with defaults as base
    mergedConfig = defaults;
    
    // Manually merge sensor thresholds, but only the keys we care about
    // (threshold1 and threshold2). This avoids pulling in unexpected stale keys
    // like facethr/gesturethr from older schemas, and also supports simple
    // top-level override keys for robustness.
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

        // Allow defaults to also specify simple top-level keys
        if (defaults.has("sensorThreshold1")) {
            int base1 = defaults.get("sensorThreshold1").toInt();
            threshold1 = base1;
        }
        if (defaults.has("sensorThreshold2")) {
            int base2 = defaults.get("sensorThreshold2").toInt();
            threshold2 = base2;
        }

        // Also accept really simple top-level default keys "threshold1/2"
        if (defaults.has("threshold1")) {
            int base1 = defaults.get("threshold1").toInt();
            threshold1 = base1;
        }
        if (defaults.has("threshold2")) {
            int base2 = defaults.get("threshold2").toInt();
            threshold2 = base2;
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

        // Also allow simple top-level override keys in device-settings
        // so we don't depend on the nested sensor map ever being updated.
        if (device.has("sensorThreshold1")) {
            int override1 = device.get("sensorThreshold1").toInt();
            threshold1 = override1;
        }
        if (device.has("sensorThreshold2")) {
            int override2 = device.get("sensorThreshold2").toInt();
            threshold2 = override2;
        }

        // And finally accept really simple top-level override keys
        // "threshold1/2" on the device-settings ledger.
        if (device.has("threshold1")) {
            int override1 = device.get("threshold1").toInt();
            threshold1 = override1;
        }
        if (device.has("threshold2")) {
            int override2 = device.get("threshold2").toInt();
            threshold2 = override2;
        }

        // Build a minimal merged sensor object with only the supported keys
        VariantMap mergedSensor;
        mergedSensor["threshold1"] = Variant(threshold1);
        mergedSensor["threshold2"] = Variant(threshold2);

        Log.info("Final merged sensor thresholds: threshold1=%d, threshold2=%d", threshold1, threshold2);
        mergedConfig.set("sensor", Variant(mergedSensor));
    }
    
    // Apply other top-level device overrides (these aren't nested objects)
    if (device.has("timing")) mergedConfig.set("timing", device.get("timing"));
    if (device.has("power")) mergedConfig.set("power", device.get("power"));
    if (device.has("messaging")) mergedConfig.set("messaging", device.get("messaging"));
    if (device.has("modes")) mergedConfig.set("modes", device.get("modes"));
    
    Log.info("Merged config: %s", mergedConfig.toJSON().c_str());
    Log.info("Configuration merged - applying to device");
    applyConfigurationFromLedger();
}

bool Cloud::loadConfigurationFromCloud() {
    Log.info("Loading configuration from cloud ledgers");
    
    // Trigger merge - onSync callbacks will update when cloud pushes new data
    mergeConfiguration();
    return true;
}

bool Cloud::applyConfigurationFromLedger() {
    Log.info("Applying merged configuration from DeviceConfigLedger");
    
    bool success = true;
    
    // Apply each configuration section
    success &= applySensorConfig();
    success &= applyTimingConfig();
    success &= applyPowerConfig();
    success &= applyMessagingConfig();
    success &= applyModesConfig();
    
    if (success) {
        Log.info("Configuration successfully applied to persistent storage");
        sysStatus.validate(sizeof(sysStatus));
        sensorConfig.validate(sizeof(sensorConfig));

        // After applying configuration, publish the current settings
        // to the device-status ledger so the cloud view stays in sync.
        writeDeviceStatusToCloud();
    } else {
        Log.warn("Some configuration sections failed to apply");
    }
    
    return success;
}

bool Cloud::applyMessagingConfig() {
    Log.info("Applying messaging configuration");
    
    if (!mergedConfig.has("messaging")) return true;
    Variant messaging = mergedConfig.get("messaging");
    
    if (!messaging.isMap()) {
        Log.info("No messaging configuration found");
        return true;
    }

    bool success = true;

    if (messaging.has("serial")) {
        bool serialEnabled = messaging.get("serial").toBool();
        sysStatus.set_serialConnected(serialEnabled);
        Log.info("Serial: %s", serialEnabled ? "ON" : "OFF");
    }

    if (messaging.has("verboseMode")) {
        bool verboseMode = messaging.get("verboseMode").toBool();
        sysStatus.set_verboseMode(verboseMode);
        Log.info("Verbose: %s", verboseMode ? "ON" : "OFF");
    }

    if (messaging.has("disconnectedMode")) {
        bool disconnectedMode = messaging.get("disconnectedMode").toBool();
        sysStatus.set_disconnectedMode(disconnectedMode);
        Log.info("Disconnected mode: %s", disconnectedMode ? "ON" : "OFF");
    }

    return success;
}

bool Cloud::applyTimingConfig() {
    Log.info("Applying timing configuration");
    
    if (!mergedConfig.has("timing")) return true;
    Variant timing = mergedConfig.get("timing");
    
    if (!timing.isMap()) {
        Log.info("No timing configuration found");
        return true;
    }

    bool success = true;

    if (timing.has("timezone")) {
        String timezone = timing.get("timezone").toString();
        if (timezone.length() > 0 && timezone.length() < 39) {
            sysStatus.set_timeZoneStr(timezone.c_str());
            Log.info("Timezone: %s", timezone.c_str());
        } else {
            Log.warn("Invalid timezone length: %d", timezone.length());
            success = false;
        }
    }

    if (timing.has("reportingIntervalSec")) {
        int reportingInterval = timing.get("reportingIntervalSec").toInt();
        if (validateRange(reportingInterval, 300, 86400, "timing.reportingIntervalSec")) {
            sysStatus.set_reportingInterval(reportingInterval);
            Log.info("Reporting interval: %ds", reportingInterval);
        } else {
            success = false;
        }
    }

    if (timing.has("pollingRateSec")) {
        int pollingRate = timing.get("pollingRateSec").toInt();
        if (validateRange(pollingRate, 0, 3600, "timing.pollingRateSec")) {
            sensorConfig.set_pollingRate(pollingRate);
            Log.info("Polling rate: %ds", pollingRate);
        } else {
            success = false;
        }
    }

    if (timing.has("openHour")) {
        int openHour = timing.get("openHour").toInt();
        if (validateRange(openHour, 0, 23, "timing.openHour")) {
            sysStatus.set_openTime(openHour);
            Log.info("Open hour: %d", openHour);
        } else {
            success = false;
        }
    }

    if (timing.has("closeHour")) {
        int closeHour = timing.get("closeHour").toInt();
        if (validateRange(closeHour, 0, 23, "timing.closeHour")) {
            sysStatus.set_closeTime(closeHour);
            Log.info("Close hour: %d", closeHour);
        } else {
            success = false;
        }
    }

    return success;
}

bool Cloud::applyPowerConfig() {
    Log.info("Applying power configuration");
    
    if (!mergedConfig.has("power")) return true;
    Variant power = mergedConfig.get("power");
    
    if (!power.isMap()) {
        Log.info("No power configuration found");
        return true;
    }

    bool success = true;

    if (power.has("lowPowerMode")) {
        bool lowPowerMode = power.get("lowPowerMode").toBool();
        sysStatus.set_lowPowerMode(lowPowerMode);
        Log.info("Low power mode: %s", lowPowerMode ? "ON" : "OFF");
    }

    if (power.has("solarPowerMode")) {
        bool solarPowerMode = power.get("solarPowerMode").toBool();
        sysStatus.set_solarPowerMode(solarPowerMode);
        Log.info("Solar power: %s", solarPowerMode ? "ON" : "OFF");
    }

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
    Log.info("Applying sensor configuration");
    
    if (!mergedConfig.has("sensor")) {
        Log.warn("No sensor configuration found");
        return true;
    }
    
    Variant sensor = mergedConfig.get("sensor");
    
    if (!sensor.isMap()) {
        Log.warn("Sensor configuration is not a map");
        return true;
    }
    
    bool success = true;
    
    // threshold1
    if (sensor.has("threshold1")) {
        int threshold1 = sensor.get("threshold1").toInt();
        if (validateRange(threshold1, 0, 100, "sensor.threshold1")) {
            sensorConfig.set_threshold1(threshold1);
            Log.info("✓ Sensor threshold1: %d", threshold1);
        } else {
            success = false;
        }
    }
    
    // threshold2
    if (sensor.has("threshold2")) {
        int threshold2 = sensor.get("threshold2").toInt();
        if (validateRange(threshold2, 0, 100, "sensor.threshold2")) {
            sensorConfig.set_threshold2(threshold2);
            Log.info("✓ Sensor threshold2: %d", threshold2);
        } else {
            success = false;
        }
    }
    
    return success;
}

bool Cloud::applyModesConfig() {
    Log.info("Applying modes configuration");
    
    if (!mergedConfig.has("modes")) return true;
    Variant modes = mergedConfig.get("modes");
    
    if (!modes.isMap()) {
        Log.info("No modes configuration found");
        return true;
    }
    
    bool success = true;

    // Counting mode: 0=COUNTING (count events), 1=OCCUPANCY (track occupied time)
    if (modes.has("countingMode")) {
        int countingMode = modes.get("countingMode").asInt();
        if (validateRange(countingMode, 0, 1, "countingMode")) {
            sysStatus.set_countingMode(static_cast<CountingMode>(countingMode));
            Log.info("Counting mode set to: %d (%s)", countingMode, 
                     countingMode == 0 ? "COUNTING" : "OCCUPANCY");
        } else {
            success = false;
        }
    }

    // Operating mode: 0=CONNECTED (stay connected), 1=LOW_POWER (sleep between reports)
    if (modes.has("operatingMode")) {
        int operatingMode = modes.get("operatingMode").asInt();
        if (validateRange(operatingMode, 0, 1, "operatingMode")) {
            sysStatus.set_operatingMode(static_cast<OperatingMode>(operatingMode));
            Log.info("Operating mode set to: %d (%s)", operatingMode,
                     operatingMode == 0 ? "CONNECTED" : "LOW_POWER");
        } else {
            success = false;
        }
    }

    // Trigger mode: 0=INTERRUPT (event-driven), 1=SCHEDULED (periodic polling)
    if (modes.has("triggerMode")) {
        int triggerMode = modes.get("triggerMode").asInt();
        if (validateRange(triggerMode, 0, 1, "triggerMode")) {
            sysStatus.set_triggerMode(static_cast<TriggerMode>(triggerMode));
            Log.info("Trigger mode set to: %d (%s)", triggerMode,
                     triggerMode == 0 ? "INTERRUPT" : "SCHEDULED");
        } else {
            success = false;
        }
    }

    // Occupancy debounce time (milliseconds)
    if (modes.has("occupancyDebounceMs")) {
        unsigned long occupancyDebounceMs = modes.get("occupancyDebounceMs").asUInt();
        if (validateRange(occupancyDebounceMs, 0UL, 600000UL, "occupancyDebounceMs")) {
            sysStatus.set_occupancyDebounceMs(occupancyDebounceMs);
            Log.info("Occupancy debounce set to: %lu ms", occupancyDebounceMs);
        } else {
            success = false;
        }
    }

    // Connected mode reporting interval (seconds)
    if (modes.has("connectedReportingIntervalSec")) {
        int connectedReportingIntervalSec = modes.get("connectedReportingIntervalSec").asInt();
        if (validateRange(connectedReportingIntervalSec, 60, 86400, "connectedReportingIntervalSec")) {
            sysStatus.set_connectedReportingIntervalSec(connectedReportingIntervalSec);
            Log.info("Connected mode reporting interval set to: %d seconds", connectedReportingIntervalSec);
        } else {
            success = false;
        }
    }

    // Low power mode reporting interval (seconds)
    if (modes.has("lowPowerReportingIntervalSec")) {
        int lowPowerReportingIntervalSec = modes.get("lowPowerReportingIntervalSec").asInt();
        if (validateRange(lowPowerReportingIntervalSec, 300, 86400, "lowPowerReportingIntervalSec")) {
            sysStatus.set_lowPowerReportingIntervalSec(lowPowerReportingIntervalSec);
            Log.info("Low power mode reporting interval set to: %d seconds", lowPowerReportingIntervalSec);
        } else {
            success = false;
        }
    }

    return success;
}

bool Cloud::writeDeviceStatusToCloud() {
    // Build current configuration as JSON
    char buffer[512];
    JSONBufferWriter writer(buffer, sizeof(buffer));

    writer.beginObject();

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
    writer.endObject();

    // Firmware release metadata
    writer.name("firmware").beginObject();
    writer.name("version").value(FIRMWARE_VERSION);
    writer.name("notes").value(FIRMWARE_RELEASE_NOTES);
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
    
    if (sysStatus.get_countingMode() == COUNTING) {
        writer.name("mode").value("counting");
        writer.name("hourlyCount").value(current.get_hourlyCount());
        writer.name("dailyCount").value(current.get_dailyCount());
    } else {
        writer.name("mode").value("occupancy");
        writer.name("occupied").value(current.get_occupied());
        writer.name("totalOccupiedSec").value(current.get_totalOccupiedSeconds());
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
        Log.info("Sensor data published to cloud");
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
