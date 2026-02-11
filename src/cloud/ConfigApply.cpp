#include "cloud/Cloud.h"

bool Cloud::applyConfigurationFromLedger() {
    bool success = true;

    success &= applySensorConfig();
    success &= applyTimingConfig();
    success &= applyMessagingConfig();
    success &= applyModesConfig();
    success &= applyReportingConfig();
    
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

    // Note: pollingRate is no longer used in v3.23 - sensor-specific timing
    // is now configured via sensor.setting1-4 fields in the sensor section

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
        uint32_t currentValue = sensorConfig.get_sensorSetting1();
        Log.info("Cloud config: setting1=%d (current EEPROM=%lu)", setting1, (unsigned long)currentValue);
        if (currentValue != (uint32_t)setting1) {
            sensorConfig.set_sensorSetting1((uint32_t)setting1);
            Log.info("Config: Sensor setting1 updated: %lu -> %d", (unsigned long)currentValue, setting1);
            changed = true;
        } else {
            Log.info("Config: Sensor setting1 unchanged at %d", setting1);
        }
    } else {
        Log.warn("Cloud config: sensor.setting1 key NOT FOUND in ledger!");
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

bool Cloud::applyReportingConfig() {
    if (!mergedConfig.has("reporting")) return true;
    Variant reporting = mergedConfig.get("reporting");
    
    if (!reporting.isMap()) return true;

    bool success = true;
    bool changed = false;

    if (reporting.has("webhook") && reporting.get("webhook").isMap()) {
        Variant webhook = reporting.get("webhook");
        
        if (webhook.has("name")) {
            String webhookName = webhook.get("name").toString();
            if (webhookName.length() > 0 && webhookName.length() < 64) {
                if (strcmp(sysStatus.get_webhookName(), webhookName.c_str()) != 0) {
                    sysStatus.set_webhookName(webhookName.c_str());
                    Log.info("Config: Webhook name -> %s", webhookName.c_str());
                    changed = true;
                }
            } else {
                Log.warn("Invalid webhook name length: %d", webhookName.length());
                success = false;
            }
        }
        
        if (webhook.has("enabled")) {
            bool webhookEnabled = webhook.get("enabled").toBool();
            if (sysStatus.get_webhookEnabled() != webhookEnabled) {
                sysStatus.set_webhookEnabled(webhookEnabled);
                Log.info("Config: Webhook enabled -> %s", webhookEnabled ? "YES" : "NO");
                changed = true;
            }
        }
        
        if (webhook.has("timeoutMs")) {
            int webhookTimeout = webhook.get("timeoutMs").toInt();
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
    }

    if (changed) Log.info("Reporting config updated");
    return success;
}

// Explicit template instantiations for validateRange
template bool Cloud::validateRange<int>(int value, int min, int max, const String& name);
template bool Cloud::validateRange<unsigned long>(unsigned long value, unsigned long min, unsigned long max, const String& name);
