#include "Cloud.h"

// External firmware version string (defined in Version.cpp)
extern const char* FIRMWARE_VERSION;

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
