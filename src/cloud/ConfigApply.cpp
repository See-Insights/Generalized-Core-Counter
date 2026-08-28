#include "cloud/Cloud.h"

#include "power/ChargeInhibitPolicy.h"

namespace {

bool getTopLevelMap(const LedgerData &ledger, const char *key, Variant &section) {
    if (!ledger.has(key)) {
        return false;
    }

    section = ledger.get(key);
    return section.isMap();
}

bool getNestedMap(const LedgerData &ledger, const char *parentKey, const char *childKey, Variant &section) {
    Variant parent;
    if (!getTopLevelMap(ledger, parentKey, parent) || !parent.has(childKey)) {
        return false;
    }

    section = parent.get(childKey);
    return section.isMap();
}

bool getMergedIntValue(const Variant &defaultsSection, const Variant &deviceSection, const char *key, int &value) {
    if (deviceSection.isMap() && deviceSection.has(key)) {
        value = deviceSection.get(key).toInt();
        return true;
    }

    if (defaultsSection.isMap() && defaultsSection.has(key)) {
        value = defaultsSection.get(key).toInt();
        return true;
    }

    return false;
}

bool getMergedBoolValue(const Variant &defaultsSection, const Variant &deviceSection, const char *key, bool &value) {
    if (deviceSection.isMap() && deviceSection.has(key)) {
        value = deviceSection.get(key).toBool();
        return true;
    }

    if (defaultsSection.isMap() && defaultsSection.has(key)) {
        value = defaultsSection.get(key).toBool();
        return true;
    }

    return false;
}

bool getMergedFloatValue(const Variant &defaultsSection, const Variant &deviceSection, const char *key, float &value) {
    if (deviceSection.isMap() && deviceSection.has(key)) {
        value = static_cast<float>(deviceSection.get(key).toDouble());
        return true;
    }

    if (defaultsSection.isMap() && defaultsSection.has(key)) {
        value = static_cast<float>(defaultsSection.get(key).toDouble());
        return true;
    }

    return false;
}

bool getMergedStringValue(const Variant &defaultsSection,
                          const Variant &deviceSection,
                          const char *key,
                          char *buffer,
                          size_t bufferSize) {
    Variant value;
    if (deviceSection.isMap() && deviceSection.has(key)) {
        value = deviceSection.get(key);
    } else if (defaultsSection.isMap() && defaultsSection.has(key)) {
        value = defaultsSection.get(key);
    } else {
        return false;
    }

    String text = value.toString();
    if (text.length() >= bufferSize) {
        return false;
    }

    text.toCharArray(buffer, bufferSize);
    return true;
}

void normalizeTimezoneInPlace(char *timezone, size_t timezoneSize) {
    if (!timezone || timezoneSize == 0) {
        return;
    }

    const char *normalized = nullptr;
    const char *reason = nullptr;

    if (strcmp(timezone, "SGT8") == 0) {
        normalized = "SGT-8";
        reason = "posix-sign";
    } else if (strcmp(timezone, "UTC+8") == 0 ||
               strcmp(timezone, "GMT+8") == 0 ||
               strcmp(timezone, "Singapore") == 0) {
        normalized = "SGT-8";
        reason = "alias";
    }

    if (!normalized) {
        return;
    }

    Log.warn("TimezoneNormalize: from=%s to=%s reason=%s", timezone, normalized, reason);
    snprintf(timezone, timezoneSize, "%s", normalized);
}

} // namespace

bool Cloud::applyConfigurationFromLedger(const LedgerData &defaults, const LedgerData &device) {
    bool success = true;
    bool sensorChanged = false;
    bool timingChanged = false;
    bool messagingChanged = false;
    bool modesChanged = false;
    bool reportingChanged = false;
    bool powerChanged = false;
    
    // Enhanced diagnostics for Alert 41 troubleshooting - track which section fails
    bool sensorOk = applySensorConfig(defaults, device, &sensorChanged);
    bool timingOk = applyTimingConfig(defaults, device, &timingChanged);
    bool messagingOk = applyMessagingConfig(defaults, device, &messagingChanged);
    bool modesOk = applyModesConfig(defaults, device, &modesChanged);
    bool reportingOk = applyReportingConfig(defaults, device, &reportingChanged);
    bool powerOk = applyPowerConfig(defaults, device, &powerChanged);
    
    success = sensorOk && timingOk && messagingOk && modesOk && reportingOk && powerOk;
    
    if (success) {
        const bool anyChanged = sensorChanged || timingChanged || messagingChanged || modesChanged || reportingChanged || powerChanged;

        // Do not force synchronous storage flushes here; they can exceed the
        // 100 ms loop budget. Persistence is handled by sysStatus.loop() and
        // sensorConfig.loop() (called from the main loop).
        sysStatus.validate(sizeof(sysStatus));
        sensorConfig.validate(sizeof(sensorConfig));

        // Defer device-status publishing to Cloud::loop() so it doesn't
        // execute inside CONNECTING_STATE or async callbacks.
        if (anyChanged) {
            const bool wasPending = pendingStatusPublish;
            pendingStatusPublish = true;
            pendingStatusPublishSource = "ConfigApply";
            if (!wasPending) {
                const Cloud::LedgerSyncDiagnostics diagnostics = ledgerSyncDiagnostics();
#if defined(ENABLE_LEDGER_TRACE) && ENABLE_LEDGER_TRACE
                Log.info("LedgerQueue: kind=status pending=%u syncing=%d inflight=%d",
                         diagnostics.pendingCount,
                         diagnostics.syncing ? 1 : 0,
                         diagnostics.inflight ? 1 : 0);
#else
                (void)diagnostics;
#endif
            }
        }
    } else {
        Log.warn("Configuration apply failed: sensor=%s timing=%s messaging=%s modes=%s reporting=%s power=%s",
                 sensorOk ? "OK" : "FAIL",
                 timingOk ? "OK" : "FAIL",
                 messagingOk ? "OK" : "FAIL",
                 modesOk ? "OK" : "FAIL",
                 reportingOk ? "OK" : "FAIL",
                 powerOk ? "OK" : "FAIL");
    }
    
    return success;
}

bool Cloud::applyMessagingConfig(const LedgerData &defaults, const LedgerData &device, bool *changedOut) {
    Variant defaultMessaging;
    Variant deviceMessaging;
    bool hasDefault = getTopLevelMap(defaults, "messaging", defaultMessaging);
    bool hasDevice = getTopLevelMap(device, "messaging", deviceMessaging);

    if (!hasDefault && !hasDevice) return true;
    
    if ((hasDefault && !defaultMessaging.isMap()) || (hasDevice && !deviceMessaging.isMap())) return true;

    bool success = true;
    bool changed = false;
    bool serialEnabled = false;
    bool verboseMode = false;
    int verboseTimeoutMin = 0;

    if (getMergedBoolValue(defaultMessaging, deviceMessaging, "serial", serialEnabled)) {
        if (sysStatus.get_serialConnected() != serialEnabled) {
            sysStatus.set_serialConnected(serialEnabled);
            Log.info("Config: Serial → %s", serialEnabled ? "ON" : "OFF");
            changed = true;
        }
    }

    if (getMergedBoolValue(defaultMessaging, deviceMessaging, "verboseMode", verboseMode)) {
        if (sysStatus.get_verboseMode() != verboseMode) {
            sysStatus.set_verboseMode(verboseMode);
            Log.info("Config: Verbose -> %s", verboseMode ? "ON" : "OFF");
            changed = true;
        }
    }

    if (getMergedIntValue(defaultMessaging, deviceMessaging, "verboseTimeoutMin", verboseTimeoutMin)) {
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

    if (changedOut) {
        *changedOut = changed;
    }
    if (changed) Log.info("Messaging config updated");
    return success;
}

bool Cloud::applyTimingConfig(const LedgerData &defaults, const LedgerData &device, bool *changedOut) {
    Variant defaultTiming;
    Variant deviceTiming;
    bool hasDefault = getTopLevelMap(defaults, "timing", defaultTiming);
    bool hasDevice = getTopLevelMap(device, "timing", deviceTiming);
    
    if (!hasDefault && !hasDevice) return true;
    if ((hasDefault && !defaultTiming.isMap()) || (hasDevice && !deviceTiming.isMap())) return true;

    bool success = true;
    bool changed = false;
    int reportingInterval = 0;
    int openHour = 0;
    int closeHour = 0;
    int connectAttemptBudgetSec = 0;
    char timezone[sizeof(sysStatusData::SysData::timeZoneStr)] = {0};

    if (getMergedStringValue(defaultTiming, deviceTiming, "timezone", timezone, sizeof(timezone))) {
        normalizeTimezoneInPlace(timezone, sizeof(timezone));
        size_t timezoneLen = strlen(timezone);
        if (timezoneLen > 0 && timezoneLen < sizeof(timezone)) {
            if (strcmp(sysStatus.get_timeZoneStrCStr(), timezone) != 0) {
                sysStatus.set_timeZoneStr(timezone);
                Log.info("Config: Timezone -> %s", timezone);
                changed = true;
            }
        } else {
            Log.warn("Invalid timezone length: %d", (int)timezoneLen);
            success = false;
        }
    }

    if (getMergedIntValue(defaultTiming, deviceTiming, "reportingIntervalSec", reportingInterval)) {
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

    // Note: pollingRate is no longer used in v3.23 - sensor-specific timing
    // is now configured via sensor.setting1-4 fields in the sensor section

    if (getMergedIntValue(defaultTiming, deviceTiming, "openHour", openHour)) {
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

    if (getMergedIntValue(defaultTiming, deviceTiming, "closeHour", closeHour)) {
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
    if (getMergedIntValue(defaultTiming, deviceTiming, "connectAttemptBudgetSec", connectAttemptBudgetSec)) {
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

    if (changedOut) {
        *changedOut = changed;
    }
    if (changed) Log.info("Timing config updated");
    return success;
}

template<typename T>
bool Cloud::validateRange(T value, T min, T max, const char* name) {
    if (value < min || value > max) {
        Log.warn("Invalid %s value: %d (must be between %d and %d)", name, (int)value, (int)min, (int)max);
        return false;
    }
    return true;
}

bool Cloud::applySensorConfig(const LedgerData &defaults, const LedgerData &device, bool *changedOut) {
    Variant defaultSensor;
    Variant deviceSensor;
    bool hasDefault = getTopLevelMap(defaults, "sensor", defaultSensor);
    bool hasDevice = getTopLevelMap(device, "sensor", deviceSensor);

    if (!hasDefault && !hasDevice) return true;
    if ((hasDefault && !defaultSensor.isMap()) || (hasDevice && !deviceSensor.isMap())) return true;
    
    bool success = true;
    bool changed = false;
    int sensorType = 0;
    int setting1 = 0;
    int setting2 = 0;
    int setting3 = 0;
    int setting4 = 0;

    // sensor.type
    if (getMergedIntValue(defaultSensor, deviceSensor, "type", sensorType)) {
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
    if (getMergedIntValue(defaultSensor, deviceSensor, "setting1", setting1)) {
        uint32_t currentValue = sensorConfig.get_sensorSetting1();
        if (sysStatus.get_verboseMode()) {
            Log.info("Cloud config: setting1=%d (current EEPROM=%lu)", setting1, (unsigned long)currentValue);
        }
        if (currentValue != (uint32_t)setting1) {
            sensorConfig.set_sensorSetting1((uint32_t)setting1);
            Log.info("Config: Sensor setting1 updated: %lu -> %d", (unsigned long)currentValue, setting1);
            changed = true;
        } else if (sysStatus.get_verboseMode()) {
            Log.info("Config: Sensor setting1 unchanged at %d", setting1);
        }
    } else {
        Log.warn("Cloud config: sensor.setting1 key NOT FOUND in ledger!");
    }
    
    if (getMergedIntValue(defaultSensor, deviceSensor, "setting2", setting2)) {
        if (sensorConfig.get_sensorSetting2() != (uint32_t)setting2) {
            sensorConfig.set_sensorSetting2((uint32_t)setting2);
            Log.info("Config: Sensor setting2 -> %d", setting2);
            changed = true;
        }
    }
    
    if (getMergedIntValue(defaultSensor, deviceSensor, "setting3", setting3)) {
        if (sensorConfig.get_sensorSetting3() != (uint32_t)setting3) {
            sensorConfig.set_sensorSetting3((uint32_t)setting3);
            Log.info("Config: Sensor setting3 -> %d", setting3);
            changed = true;
        }
    }
    
    if (getMergedIntValue(defaultSensor, deviceSensor, "setting4", setting4)) {
        if (sensorConfig.get_sensorSetting4() != (uint32_t)setting4) {
            sensorConfig.set_sensorSetting4((uint32_t)setting4);
            Log.info("Config: Sensor setting4 -> %d", setting4);
            changed = true;
        }
    }
    
    if (changedOut) {
        *changedOut = changed;
    }
    if (changed) Log.info("Sensor config updated");
    return success;
}

bool Cloud::applyModesConfig(const LedgerData &defaults, const LedgerData &device, bool *changedOut) {
    Variant defaultModes;
    Variant deviceModes;
    bool hasDefault = getTopLevelMap(defaults, "modes", defaultModes);
    bool hasDevice = getTopLevelMap(device, "modes", deviceModes);

    if (!hasDefault && !hasDevice) return true;
    if ((hasDefault && !defaultModes.isMap()) || (hasDevice && !deviceModes.isMap())) return true;
    
    bool success = true;
    bool changed = false;
    int sensorMode = 0;
    int connectionMode = 0;
    int reportingMode = 0;
    int samplingMode = 0;
    int cloudDisconnectBudgetSec = 0;
    int modemOffBudgetSec = 0;
    bool enableHibernateSleep = false;

    // Sensor mode: 0=COUNTING, 1=OCCUPANCY, 2=MEASUREMENT
    if (getMergedIntValue(defaultModes, deviceModes, "sensorMode", sensorMode)) {
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
    if (getMergedIntValue(defaultModes, deviceModes, "connectionMode", connectionMode)) {
        if (validateRange(connectionMode, 0, 3, "connectionMode")) {
            if (sysStatus.get_connectionMode() != static_cast<ConnectionMode>(connectionMode) ||
                sysStatus.get_lowBatteryMode()) {
                sysStatus.set_connectionMode(static_cast<ConnectionMode>(connectionMode));
                sysStatus.set_lowBatteryMode(false);
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
    if (getMergedIntValue(defaultModes, deviceModes, "reportingMode", reportingMode)) {
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
    if (getMergedIntValue(defaultModes, deviceModes, "samplingMode", samplingMode)) {
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
    if (getMergedIntValue(defaultModes, deviceModes, "cloudDisconnectBudgetSec", cloudDisconnectBudgetSec)) {
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
    if (getMergedIntValue(defaultModes, deviceModes, "modemOffBudgetSec", modemOffBudgetSec)) {
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

    if (getMergedBoolValue(defaultModes, deviceModes, "enableHibernateSleep", enableHibernateSleep)) {
        if (sysStatus.get_enableHibernateSleep() != enableHibernateSleep) {
            sysStatus.set_enableHibernateSleep(enableHibernateSleep);
            Log.info("Config: enable hibernate sleep -> %s", enableHibernateSleep ? "YES" : "NO");
            changed = true;
        }
    }

    if (changedOut) {
        *changedOut = changed;
    }
    if (changed) Log.info("Modes config updated");
    return success;
}

bool Cloud::applyReportingConfig(const LedgerData &defaults, const LedgerData &device, bool *changedOut) {
    Variant defaultWebhook;
    Variant deviceWebhook;
    bool hasDefault = getNestedMap(defaults, "reporting", "webhook", defaultWebhook);
    bool hasDevice = getNestedMap(device, "reporting", "webhook", deviceWebhook);

    if (!hasDefault && !hasDevice) return true;
    if ((hasDefault && !defaultWebhook.isMap()) || (hasDevice && !deviceWebhook.isMap())) return true;

    bool success = true;
    bool changed = false;
    bool webhookEnabled = false;
    int webhookTimeout = 0;
    char webhookName[sizeof(sysStatusData::SysData::webhookName)] = {0};

    if (getMergedStringValue(defaultWebhook, deviceWebhook, "name", webhookName, sizeof(webhookName))) {
        size_t webhookNameLen = strlen(webhookName);
        if (webhookNameLen > 0 && webhookNameLen < sizeof(webhookName)) {
            if (strcmp(sysStatus.get_webhookNameCStr(), webhookName) != 0) {
                sysStatus.set_webhookName(webhookName);
                Log.info("Config: Webhook name -> %s", webhookName);
                changed = true;
            }
        } else {
            Log.warn("Invalid webhook name length: %d", (int)webhookNameLen);
            success = false;
        }
    }

    if (getMergedBoolValue(defaultWebhook, deviceWebhook, "enabled", webhookEnabled)) {
        if (sysStatus.get_webhookEnabled() != webhookEnabled) {
            sysStatus.set_webhookEnabled(webhookEnabled);
            Log.info("Config: Webhook enabled -> %s", webhookEnabled ? "YES" : "NO");
            changed = true;
        }
    }

    if (getMergedIntValue(defaultWebhook, deviceWebhook, "timeoutMs", webhookTimeout)) {
        if (validateRange(webhookTimeout, 1000, 60000, "webhookTimeoutMs")) {
            if (sysStatus.get_webhookTimeoutMs() != (uint32_t)webhookTimeout) {
                sysStatus.set_webhookTimeoutMs((uint32_t)webhookTimeout);
                Log.info("Config: Webhook timeout -> %dms", webhookTimeout);
                changed = true;
            }
        } else {
            success = false;
        }
    }

    if (sysStatus.get_verboseMode() &&
        (webhookName[0] != '\0' || getMergedBoolValue(defaultWebhook, deviceWebhook, "enabled", webhookEnabled))) {
        Log.info("Merged webhook config: name=%s, enabled=%d",
                 webhookName[0] != '\0' ? webhookName : "none",
                 (int)webhookEnabled);
    }

    if (changedOut) {
        *changedOut = changed;
    }
    if (changed) Log.info("Reporting config updated");
    return success;
}

bool Cloud::applyPowerConfig(const LedgerData &defaults, const LedgerData &device, bool *changedOut) {
    // F2a thermal charge-inhibit thresholds (WO-2026-08-25-001): ledger-configurable
    // and per-device overridable, per the work order. Uses the merge convention
    // shared by every other applyXConfig() in this file: device ledger wins,
    // default ledger is the fallback, absence of either leaves the stored
    // (or compiled-in default, via the get_ accessor fallback) value untouched.
    Variant defaultThermal;
    Variant deviceThermal;
    bool hasDefault = getNestedMap(defaults, "power", "thermalChargeInhibit", defaultThermal);
    bool hasDevice = getNestedMap(device, "power", "thermalChargeInhibit", deviceThermal);

    if (!hasDefault && !hasDevice) return true;
    if ((hasDefault && !defaultThermal.isMap()) || (hasDevice && !deviceThermal.isMap())) return true;

    // AC-B6 (WO-2026-08-25-001 Amendment B): a malformed ledger must not be
    // able to invert the hysteresis or configure an unsafe ceiling. Build the
    // full candidate set (starting from the currently-stored values, so a
    // ledger that only supplies a subset of the four fields is validated
    // against the values it would actually run with) and validate it as a
    // whole with ChargeInhibitPolicy::isValidThermalThresholds() BEFORE
    // committing any individual field - a partially-applied invalid set
    // would be just as unsafe as a fully-applied one.
    ChargeInhibitPolicy::ThermalThresholds candidate{
        sysStatus.get_thermalChargeArmHighC(),
        sysStatus.get_thermalChargeArmLowC(),
        sysStatus.get_thermalChargeReleaseHighC(),
        sysStatus.get_thermalChargeReleaseLowC(),
    };

    bool anyFieldSupplied = false;
    anyFieldSupplied |= getMergedFloatValue(defaultThermal, deviceThermal, "armHighC", candidate.armHighC);
    anyFieldSupplied |= getMergedFloatValue(defaultThermal, deviceThermal, "armLowC", candidate.armLowC);
    anyFieldSupplied |= getMergedFloatValue(defaultThermal, deviceThermal, "releaseHighC", candidate.releaseHighC);
    anyFieldSupplied |= getMergedFloatValue(defaultThermal, deviceThermal, "releaseLowC", candidate.releaseLowC);

    if (!anyFieldSupplied) return true;

    if (!ChargeInhibitPolicy::isValidThermalThresholds(candidate)) {
        Log.warn("Config: rejecting malformed thermal charge-inhibit thresholds "
                 "(armHigh=%.1f armLow=%.1f releaseHigh=%.1f releaseLow=%.1f) - "
                 "keeping existing values",
                 (double)candidate.armHighC, (double)candidate.armLowC,
                 (double)candidate.releaseHighC, (double)candidate.releaseLowC);
        return false;
    }

    bool changed = false;

    if (sysStatus.get_thermalChargeArmHighC() != candidate.armHighC) {
        sysStatus.set_thermalChargeArmHighC(candidate.armHighC);
        Log.info("Config: Thermal charge-inhibit armHighC -> %.1fC", (double)candidate.armHighC);
        changed = true;
    }

    if (sysStatus.get_thermalChargeArmLowC() != candidate.armLowC) {
        sysStatus.set_thermalChargeArmLowC(candidate.armLowC);
        Log.info("Config: Thermal charge-inhibit armLowC -> %.1fC", (double)candidate.armLowC);
        changed = true;
    }

    if (sysStatus.get_thermalChargeReleaseHighC() != candidate.releaseHighC) {
        sysStatus.set_thermalChargeReleaseHighC(candidate.releaseHighC);
        Log.info("Config: Thermal charge-inhibit releaseHighC -> %.1fC", (double)candidate.releaseHighC);
        changed = true;
    }

    if (sysStatus.get_thermalChargeReleaseLowC() != candidate.releaseLowC) {
        sysStatus.set_thermalChargeReleaseLowC(candidate.releaseLowC);
        Log.info("Config: Thermal charge-inhibit releaseLowC -> %.1fC", (double)candidate.releaseLowC);
        changed = true;
    }

    if (changedOut) {
        *changedOut = changed;
    }
    if (changed) Log.info("Power config updated");
    return true;
}

// Explicit template instantiations for validateRange
template bool Cloud::validateRange<int>(int value, int min, int max, const char* name);
template bool Cloud::validateRange<unsigned long>(unsigned long value, unsigned long min, unsigned long max, const char* name);
