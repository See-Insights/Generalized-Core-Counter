/**
 * @file Cloud.cpp
 * @brief Cloud Configuration Management Implementation
 * 
 * @details Implements Particle Ledger-based configuration management using DeviceInfoLedger library.
 *          Automatically merges product defaults and device-specific overrides (including nested objects).
 *          Implements delta-only status publishing to minimize cellular data usage.
 * 
 * @author Chip McClelland
 * @date December 12, 2025
 * 
 * @license MIT License
 * 
 * Copyright (c) 2025 Chip McClelland
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Expected JSON structure:
 * 
 * Product defaults ledger (default-settings):
 * {
 *     "sensor": {"threshold1": 60, "threshold2": 60},
 *     "timing": {"timezone": "SGT-8", "reportingIntervalSec": 3600, "openHour": 6, "closeHour": 22},
 *     "power": {"lowPowerMode": false, "solarPowerMode": true},
 *     "messaging": {"serial": true, "verboseMode": false},
 *     "modes": {"countingMode": 0, "operatingMode": 0, "triggerMode": 0}
 * }
 * 
 * Device-specific overrides ledger (device-settings):
 * {
 *     "sensor": {"threshold1": 65}  // Only override specific values
 * }
 * 
 * DeviceInfoLedger automatically merges these, resulting in:
 * {
 *     "sensor": {"threshold1": 65, "threshold2": 60}  // threshold1 overridden, threshold2 from default
 * }
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

Cloud::Cloud() {
    lastPublishedStatus = "";
}

Cloud::~Cloud() {
}

void Cloud::setup() {
    Log.info("Setting up Cloud configuration management");
    
    // Configure DeviceInfoLedger for automatic configuration management
    DeviceConfigLedger::instance()
        .withConfigDefaultLedgerName("default-settings")      // Product defaults
        .withConfigDefaultLedgerEnabled(true)
        .withConfigDeviceLedgerName("device-settings")        // Device overrides
        .withConfigDeviceLedgerEnabled(true)
        .withUpdateCallback([]() {
            Log.info("*** Ledger sync detected - configuration updated ***");
            // When ledgers sync, automatically reload configuration
            Cloud::instance().applyConfigurationFromLedger();
        });
    
    // Initialize DeviceInfoLedger - handles ledger sync automatically
    DeviceConfigLedger::instance().setup();
    Log.info("DeviceConfigLedger setup complete - monitoring default-settings and device-settings");
    
    // Create device-status and device-data ledgers (Device → Cloud)
    deviceStatusLedger = Particle.ledger("device-status");
    deviceDataLedger = Particle.ledger("device-data");
    
    Log.info("Cloud configuration management initialized");
    Log.info("  default-settings: Product defaults (Cloud→Device)");
    Log.info("  device-settings: Device overrides (Cloud→Device)");
    Log.info("  device-status: Current config (Device→Cloud)");
    Log.info("  device-data: Sensor readings (Device→Cloud)");
}

void Cloud::loop() {
    // DeviceConfigLedger syncs automatically via ledger callbacks - no loop needed
}

bool Cloud::loadConfigurationFromCloud() {
    Log.info("Loading configuration from cloud (via DeviceInfoLedger)");
    
    // DeviceInfoLedger automatically merges default-settings and device-settings
    // Just need to apply the merged configuration to our persistent storage
    return applyConfigurationFromLedger();
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
    } else {
        Log.warn("Some configuration sections failed to apply");
    }
    
    return success;
}

bool Cloud::applyMessagingConfig() {
    Log.info("Applying messaging configuration");
    
    Variant messaging = DeviceConfigLedger::instance().getConfigVariant("messaging");
    
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
    
    Variant timing = DeviceConfigLedger::instance().getConfigVariant("timing");
    
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
    
    Variant power = DeviceConfigLedger::instance().getConfigVariant("power");
    
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
    
    // Get merged sensor config (product defaults + device overrides)
    // DeviceConfigLedger automatically merges nested objects!
    Variant sensor = DeviceConfigLedger::instance().getConfigVariant("sensor");
    
    if (!sensor.isMap()) {
        Log.warn("No sensor configuration found or invalid format");
        return true;  // Not an error if not configured
    }
    
    // Debug: Log the full JSON to see what we actually have
    Log.info("Sensor config JSON: %s", sensor.toJSON().c_str());
    
    // Debug: Show what keys are available
    Log.info("Sensor config keys available: threshold1=%s, threshold2=%s",
             sensor.has("threshold1") ? "yes" : "no",
             sensor.has("threshold2") ? "yes" : "no");
    
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
    } else {
        Log.warn("threshold1 not found in sensor config");
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
    } else {
        Log.warn("threshold2 not found in sensor config");
    }
    
    return success;
}

bool Cloud::applyModesConfig() {
    Log.info("Applying modes configuration");
    
    Variant modes = DeviceConfigLedger::instance().getConfigVariant("modes");
    
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
    Log.info("Writing current device configuration to device-status ledger");
    
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
    
    writer.endObject();
    
    if (!writer.buffer()) {
        Log.warn("Failed to create status JSON");
        return false;
    }
    
    buffer[writer.dataSize()] = '\0';
    
    // Write to device-status ledger
    LedgerData data = LedgerData::fromJSON(buffer);
    int result = deviceStatusLedger.set(data);
    
    if (result == SYSTEM_ERROR_NONE) {
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
