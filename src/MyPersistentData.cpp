/**
 * @file MyPersistentData.cpp
 * @brief Persistent Data Storage Implementation
 * 
 * @details Implements data structures for persistent device configuration, sensor settings,
 *          and runtime state using StorageHelperRK. Provides automatic initialization,
 *          validation, and efficient read/write operations to EEPROM/retained memory.
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
 */

#include "MyPersistentData.h"
#include "BuildProfile.h"

// Forward declaration for safe diagnostic publishing (defined in Generalized-Core-Counter.cpp)
bool publishDiagnosticSafe(const char* eventName, const char* data, PublishFlags flags = PRIVATE);

// *******************  SysStatus Storage Object **********************
//
// ********************************************************************

const char *persistentDataPathSystem = "/usr/sysStatus.dat";

sysStatusData *sysStatusData::_instance;

// [static]
sysStatusData &sysStatusData::instance() {
    if (!_instance) {
        _instance = new sysStatusData();
    }
    return *_instance;
}

sysStatusData::sysStatusData() : StorageHelperRK::PersistentDataFile(persistentDataPathSystem, &sysData.sysHeader, sizeof(SysData), SYS_DATA_MAGIC, SYS_DATA_VERSION) {

};

sysStatusData::~sysStatusData() {
}

void sysStatusData::setup() {
    sysStatus
    //  .withLogData(true)
        .withSaveDelayMs(100)
        .load();

    // Log.info("sizeof(SysData): %u", sizeof(SysData));
}

void sysStatusData::loop() {
    sysStatus.flush(false);
}

bool sysStatusData::validate(size_t dataSize) {
    bool valid = PersistentDataFile::validate(dataSize);
    if (valid) {
        // If test1 < 0 or test1 > 100, then the data is invalid

        // openTime is an hour-of-day in local time (0-23)
        if (sysStatus.get_openTime() > 23) {
            Log.info("data not valid open time =%d", sysStatus.get_openTime());
            valid = false;
        }

        // closeTime is allowed to be 24 as a sentinel for "always open"
        if (sysStatus.get_closeTime() > 24) {
            Log.info("data not valid close time =%d", sysStatus.get_closeTime());
            valid = false;
        }

        // Last connection duration sanity check (seconds)
        if (sysStatus.get_lastConnectionDuration() > 900) {
            Log.info("data not valid last connection duration =%d", sysStatus.get_lastConnectionDuration());
            valid = false;
        }

        if (sysStatus.get_connectivityRecoveryStage() > 3) {
            Log.info("data not valid connectivity recovery stage =%d", sysStatus.get_connectivityRecoveryStage());
            valid = false;
        }
    }
    if (!valid) {
        Log.warn("sysStatus data is not valid");
    }
    return valid;
}

void sysStatusData::initialize() {
    PersistentDataFile::initialize();

    const char message[26] = "Loading System Defaults";
    Log.info(message);
    if (Particle.connected()) publishDiagnosticSafe("Mode", message, PRIVATE);
    Log.info("Loading system defaults");
    sysStatus.set_structuresVersion(2);
    sysStatus.set_verboseMode(false);
    sysStatus.set_lowBatteryMode(false);
    sysStatus.set_solarPowerMode(FIELD_BUILD ? true : false);
    sysStatus.set_lowPowerMode(false);          // Legacy flag - kept for storage compatibility
    sysStatus.set_timeZoneStr("SGT-8");        // Default to Singapore Time (POSIX TZ string for UTC+8, no DST)
    sysStatus.set_sensorType(1);                // PIR sensor
    sysStatus.set_openTime(0);
    sysStatus.set_closeTime(24);                                           // New standard with v20
    sysStatus.set_lastConnectionDuration(0);                               // New measure
    sysStatus.set_lastDailyCleanup(0);                                     // No cleanup has run yet
    
    // ********** Operating Mode Defaults **********
    sysStatus.set_sensorMode(COUNTING);                                    // Default to counting mode
    sysStatus.set_connectionMode(CONNECTED);                               // Default to connected mode
    sysStatus.set_reportingMode(SCHEDULED);                                // Default to scheduled reporting
    sysStatus.set_samplingMode(INTERRUPT);                                 // Default to interrupt-driven
    sysStatus.set_verboseTimeoutMin(60);                                   // Default 60 min verbose timeout
    sysStatus.set_verboseModeStartTime(0);                                 // Verbose mode not active
    sysStatus.set_connectAttemptBudgetSec(300);                            // Default 300s (5 minutes) max connect attempt per wake
    sysStatus.set_cloudDisconnectBudgetSec(15);                            // Default 15s max wait for cloud disconnect
    sysStatus.set_modemOffBudgetSec(30);                                   // Default 30s max wait for modem power-down
    sysStatus.set_connectivityRecoveryStage(0);
    sysStatus.set_lastConnectivityRecoveryAction(0);
    sysStatus.set_connectivityRecoveryCount(0);
    sysStatus.set_watchdogResetCount(0);
    sysStatus.set_lastWatchdogBreadcrumb(0);
    sysStatus.set_lastWatchdogUptimeMs(0);
    sysStatus.set_lastWatchdogResetReasonData(0);
}

uint8_t sysStatusData::get_structuresVersion() const {
    return getValue<uint8_t>(offsetof(SysData, structuresVersion));
}

void sysStatusData::set_structuresVersion(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, structuresVersion), value);
}

bool sysStatusData::get_verboseMode() const {
    return getValue<bool>(offsetof(SysData,verboseMode));
}

void sysStatusData::set_verboseMode(bool value) {
    setValue<bool>(offsetof(SysData, verboseMode), value);
}

bool sysStatusData::get_solarPowerMode() const  {
    return getValue<bool>(offsetof(SysData,solarPowerMode ));
}
void sysStatusData::set_solarPowerMode(bool value) {
    setValue<bool>(offsetof(SysData, solarPowerMode), value);
}

bool sysStatusData::get_lowPowerMode() const  {
    return getValue<bool>(offsetof(SysData,lowPowerMode ));
}
void sysStatusData::set_lowPowerMode(bool value) {
    setValue<bool>(offsetof(SysData, lowPowerMode), value);
}

bool sysStatusData::get_lowBatteryMode() const  {
    return getValue<bool>(offsetof(SysData, lowBatteryMode));
}
void sysStatusData::set_lowBatteryMode(bool value) {
    setValue<bool>(offsetof(SysData, lowBatteryMode), value);
}

uint8_t sysStatusData::get_resetCount() const  {
    return getValue<uint8_t>(offsetof(SysData,resetCount));
}
void sysStatusData::set_resetCount(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, resetCount), value);
}

String sysStatusData::get_timeZoneStr() const {
	String result;
	getValueString(offsetof(SysData, timeZoneStr), sizeof(SysData::timeZoneStr), result);
	return result;
}

const char *sysStatusData::get_timeZoneStrCStr() const {
    return sysData.timeZoneStr;
}

bool sysStatusData::set_timeZoneStr(const char *str) {
	return setValueString(offsetof(SysData, timeZoneStr), sizeof(SysData::timeZoneStr), str);
}

uint8_t sysStatusData::get_openTime() const  {
    return getValue<uint8_t>(offsetof(SysData,openTime));
}
void sysStatusData::set_openTime(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, openTime), value);
}

uint8_t sysStatusData::get_closeTime() const  {
    return getValue<uint8_t>(offsetof(SysData,closeTime));
}
void sysStatusData::set_closeTime(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, closeTime), value);
}

time_t sysStatusData::get_lastReport() const  {
    return getValue<time_t>(offsetof(SysData,lastReport));
}
void sysStatusData::set_lastReport(time_t value) {
    setValue<time_t>(offsetof(SysData, lastReport), value);
}

time_t sysStatusData::get_lastConnection() const  {
    return getValue<time_t>(offsetof(SysData,lastConnection));
}
void sysStatusData::set_lastConnection(time_t value) {
    setValue<time_t>(offsetof(SysData, lastConnection), value);
}

uint16_t sysStatusData::get_lastConnectionDuration() const  {
    return getValue<uint16_t>(offsetof(SysData,lastConnectionDuration));
}
void sysStatusData::set_lastConnectionDuration(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData, lastConnectionDuration), value);
}

time_t sysStatusData::get_lastHookResponse() const  {
    return getValue<time_t>(offsetof(SysData,lastHookResponse));
}
void sysStatusData::set_lastHookResponse(time_t value) {
    setValue<time_t>(offsetof(SysData, lastHookResponse), value);
}

uint8_t sysStatusData::get_sensorType() const  {
    return getValue<uint8_t>(offsetof(SysData,sensorType));
}
void sysStatusData::set_sensorType(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, sensorType), value);
}

bool sysStatusData::get_updatesPending() const  {
    return getValue<bool>(offsetof(SysData,updatesPending));
}
void sysStatusData::set_updatesPending(bool value) {
    setValue<bool>(offsetof(SysData,updatesPending), value);
}  

uint16_t sysStatusData::get_reportingInterval() const  {
    return getValue<uint16_t>(offsetof(SysData,reportingInterval));
}
void sysStatusData::set_reportingInterval(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData, reportingInterval), value);
}

bool sysStatusData::get_disconnectedMode() const  {
    return getValue<bool>(offsetof(SysData,disconnectedMode));
}
void sysStatusData::set_disconnectedMode(bool value) {
    setValue<bool>(offsetof(SysData,disconnectedMode), value);
}  

bool sysStatusData::get_serialConnected() const  {
    return getValue<bool>(offsetof(SysData,serialConnected));
}
void sysStatusData::set_serialConnected(bool value) {
    setValue<bool>(offsetof(SysData,serialConnected), value);
}

time_t sysStatusData::get_lastDailyCleanup() const  {
    return getValue<time_t>(offsetof(SysData,lastDailyCleanup));
}
void sysStatusData::set_lastDailyCleanup(time_t value) {
    setValue<time_t>(offsetof(SysData, lastDailyCleanup), value);
}

time_t sysStatusData::get_lastTimeSync() const  {
    return getValue<time_t>(offsetof(SysData,lastTimeSync));
}
void sysStatusData::set_lastTimeSync(time_t value) {
    setValue<time_t>(offsetof(SysData, lastTimeSync), value);
}

// ********** Operating Mode Configuration Get/Set Functions **********

uint8_t sysStatusData::get_sensorMode() const {
    return getValue<uint8_t>(offsetof(SysData,sensorMode));
}
void sysStatusData::set_sensorMode(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData,sensorMode), value);
}

uint8_t sysStatusData::get_connectionMode() const {
    return getValue<uint8_t>(offsetof(SysData,connectionMode));
}
void sysStatusData::set_connectionMode(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData,connectionMode), value);
}

uint8_t sysStatusData::get_reportingMode() const {
    return getValue<uint8_t>(offsetof(SysData,reportingMode));
}
void sysStatusData::set_reportingMode(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData,reportingMode), value);
}

uint8_t sysStatusData::get_samplingMode() const {
    return getValue<uint8_t>(offsetof(SysData,samplingMode));
}
void sysStatusData::set_samplingMode(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData,samplingMode), value);
}

uint16_t sysStatusData::get_verboseTimeoutMin() const {
    return getValue<uint16_t>(offsetof(SysData,verboseTimeoutMin));
}
void sysStatusData::set_verboseTimeoutMin(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData,verboseTimeoutMin), value);
}

time_t sysStatusData::get_verboseModeStartTime() const {
    return getValue<time_t>(offsetof(SysData,verboseModeStartTime));
}
void sysStatusData::set_verboseModeStartTime(time_t value) {
    setValue<time_t>(offsetof(SysData,verboseModeStartTime), value);
}

uint16_t sysStatusData::get_connectAttemptBudgetSec() const {
    return getValue<uint16_t>(offsetof(SysData,connectAttemptBudgetSec));
}
void sysStatusData::set_connectAttemptBudgetSec(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData,connectAttemptBudgetSec), value);
}

uint16_t sysStatusData::get_cloudDisconnectBudgetSec() const {
    return getValue<uint16_t>(offsetof(SysData,cloudDisconnectBudgetSec));
}
void sysStatusData::set_cloudDisconnectBudgetSec(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData,cloudDisconnectBudgetSec), value);
}

uint16_t sysStatusData::get_modemOffBudgetSec() const {
    return getValue<uint16_t>(offsetof(SysData,modemOffBudgetSec));
}
void sysStatusData::set_modemOffBudgetSec(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData,modemOffBudgetSec), value);
}

uint8_t sysStatusData::get_currentBatteryTier() const {
    return getValue<uint8_t>(offsetof(SysData,currentBatteryTier));
}
void sysStatusData::set_currentBatteryTier(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData,currentBatteryTier), value);
}

uint8_t sysStatusData::get_connectionAttemptCounter() const {
    return getValue<uint8_t>(offsetof(SysData,connectionAttemptCounter));
}
void sysStatusData::set_connectionAttemptCounter(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData,connectionAttemptCounter), value);
}

uint16_t sysStatusData::get_testConnectionDurationOverride() const {
    return getValue<uint16_t>(offsetof(SysData,testConnectionDurationOverride));
}
void sysStatusData::set_testConnectionDurationOverride(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData,testConnectionDurationOverride), value);
}

String sysStatusData::get_webhookName() const {
    String result;
    getValueString(offsetof(SysData,webhookName), sizeof(SysData::webhookName), result);
    return result;
}

const char *sysStatusData::get_webhookNameCStr() const {
    return sysData.webhookName;
}

bool sysStatusData::set_webhookName(const char *str) {
    return setValueString(offsetof(SysData,webhookName), sizeof(SysData::webhookName), str);
}

bool sysStatusData::get_webhookEnabled() const {
    return getValue<bool>(offsetof(SysData,webhookEnabled));
}
void sysStatusData::set_webhookEnabled(bool value) {
    setValue<bool>(offsetof(SysData,webhookEnabled), value);
}

uint32_t sysStatusData::get_webhookTimeoutMs() const {
    return getValue<uint32_t>(offsetof(SysData,webhookTimeoutMs));
}
void sysStatusData::set_webhookTimeoutMs(uint32_t value) {
    setValue<uint32_t>(offsetof(SysData,webhookTimeoutMs), value);
}

uint8_t sysStatusData::get_connectivityRecoveryStage() const {
    return getValue<uint8_t>(offsetof(SysData,connectivityRecoveryStage));
}
void sysStatusData::set_connectivityRecoveryStage(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData,connectivityRecoveryStage), value);
}

time_t sysStatusData::get_lastConnectivityRecoveryAction() const {
    return getValue<time_t>(offsetof(SysData,lastConnectivityRecoveryAction));
}
void sysStatusData::set_lastConnectivityRecoveryAction(time_t value) {
    setValue<time_t>(offsetof(SysData,lastConnectivityRecoveryAction), value);
}

uint8_t sysStatusData::get_connectivityRecoveryCount() const {
    return getValue<uint8_t>(offsetof(SysData,connectivityRecoveryCount));
}
void sysStatusData::set_connectivityRecoveryCount(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData,connectivityRecoveryCount), value);
}

uint16_t sysStatusData::get_watchdogResetCount() const {
    return getValue<uint16_t>(offsetof(SysData,watchdogResetCount));
}
void sysStatusData::set_watchdogResetCount(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData,watchdogResetCount), value);
}

uint8_t sysStatusData::get_lastWatchdogBreadcrumb() const {
    return getValue<uint8_t>(offsetof(SysData,lastWatchdogBreadcrumb));
}
void sysStatusData::set_lastWatchdogBreadcrumb(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData,lastWatchdogBreadcrumb), value);
}

uint32_t sysStatusData::get_lastWatchdogUptimeMs() const {
    return getValue<uint32_t>(offsetof(SysData,lastWatchdogUptimeMs));
}
void sysStatusData::set_lastWatchdogUptimeMs(uint32_t value) {
    setValue<uint32_t>(offsetof(SysData,lastWatchdogUptimeMs), value);
}

uint32_t sysStatusData::get_lastWatchdogResetReasonData() const {
    return getValue<uint32_t>(offsetof(SysData,lastWatchdogResetReasonData));
}
void sysStatusData::set_lastWatchdogResetReasonData(uint32_t value) {
    setValue<uint32_t>(offsetof(SysData,lastWatchdogResetReasonData), value);
}

// End of sysStatusData class

// *****************  Sensor Config Storage Object *******************
// 
// ********************************************************************

const char *persistentDataPathSensor = "/usr/sensor.dat";

sensorConfigData *sensorConfigData::_instance;

// [static]
sensorConfigData &sensorConfigData::instance() {
    if (!_instance) {
        _instance = new sensorConfigData();
    }
    return *_instance;
}

sensorConfigData::sensorConfigData() : StorageHelperRK::PersistentDataFile(persistentDataPathSensor, &sensorData.sensorHeader, sizeof(SensorData), SENSOR_DATA_MAGIC, SENSOR_DATA_VERSION) {
};

sensorConfigData::~sensorConfigData() {
}

void sensorConfigData::setup() {
    sensorConfig
    //    .withLogData(true)
        .withSaveDelayMs(250)
        .load();
}

void sensorConfigData::loop() {
    sensorConfig.flush(false);
}

bool sensorConfigData::validate(size_t dataSize) {
    bool valid = PersistentDataFile::validate(dataSize);
    if (!valid) {
        Log.warn("Sensor config is not valid");
    }
    return valid;
}

void sensorConfigData::initialize() {
    PersistentDataFile::initialize();

    Log.info("Current Data Initialized");

    // If you manually update fields here, be sure to update the hash
    updateHash();
}

uint8_t sensorConfigData::get_sensorType() const {
    return getValue<uint8_t>(offsetof(SensorData, type));
}

void sensorConfigData::set_sensorType(uint8_t value) {
    setValue<uint8_t>(offsetof(SensorData, type), value);
}

uint32_t sensorConfigData::get_sensorSetting1() const {
    uint32_t value = getValue<uint32_t>(offsetof(SensorData, setting1));
    Log.trace("sensorConfig.get_sensorSetting1() => %lu", (unsigned long)value);
    return value;
}

void sensorConfigData::set_sensorSetting1(uint32_t value) {
    setValue<uint32_t>(offsetof(SensorData, setting1), value);
}

uint32_t sensorConfigData::get_sensorSetting2() const {
    return getValue<uint32_t>(offsetof(SensorData, setting2));
}

void sensorConfigData::set_sensorSetting2(uint32_t value) {
    setValue<uint32_t>(offsetof(SensorData, setting2), value);
}

uint32_t sensorConfigData::get_sensorSetting3() const {
    return getValue<uint32_t>(offsetof(SensorData, setting3));
}

void sensorConfigData::set_sensorSetting3(uint32_t value) {
    setValue<uint32_t>(offsetof(SensorData, setting3), value);
}

uint32_t sensorConfigData::get_sensorSetting4() const {
    return getValue<uint32_t>(offsetof(SensorData, setting4));
}

void sensorConfigData::set_sensorSetting4(uint32_t value) {
    setValue<uint32_t>(offsetof(SensorData, setting4), value);
}  // End of sensorConfigData class




// *****************  Current Status Storage Object *******************
// 
// ********************************************************************

const char *persistentDataPathCurrent = "/usr/current.dat";

currentStatusData *currentStatusData::_instance;

// [static]
currentStatusData &currentStatusData::instance() {
    if (!_instance) {
        _instance = new currentStatusData();
    }
    return *_instance;
}

currentStatusData::currentStatusData() : StorageHelperRK::PersistentDataFile(persistentDataPathCurrent, &currentData.currentHeader, sizeof(CurrentData), CURRENT_DATA_MAGIC, CURRENT_DATA_VERSION) {
};

currentStatusData::~currentStatusData() {
}

void currentStatusData::setup() {
    current
    //    .withLogData(true)
        .withSaveDelayMs(250)
        .load();
}

void currentStatusData::loop() {
    current.flush(false);
}

void currentStatusData::resetEverything() {                             // The device is waking up in a new day or is a new install
  current.set_lastCountTime(Time.now());
  sysStatus.set_resetCount(0);                                          // Reset the reset count as well
  
  // ********** Reset Counting Mode Fields **********
  current.set_hourlyCount(0);
  current.set_dailyCount(0);
  
  // ********** Reset Occupancy Mode Fields **********
  current.set_occupied(false);
  current.set_lastOccupancyEvent(0);
  current.set_occupancyStartTime(0);
  current.set_totalOccupiedSeconds(0);
}

bool currentStatusData::validate(size_t dataSize) {
    bool valid = PersistentDataFile::validate(dataSize);
    if (valid) {
        // Basic sanity checks on data
        if (current.get_hourlyCount() > 10000 || current.get_dailyCount() > 100000) {
            Log.info("Current: counts appear invalid, resetting");
            current.set_hourlyCount(0);
            current.set_dailyCount(0);
            valid = false;
        }

        // Occupancy-mode sanity checks (prevents bogus 100+ year totals)
        // totalOccupiedSeconds is "today" and should never exceed 24 hours.
        uint32_t totalOccupied = current.get_totalOccupiedSeconds();
        if (totalOccupied > 24UL * 3600UL) {
            Log.warn("Current: totalOccupiedSeconds invalid (%lu sec) - resetting", (unsigned long)totalOccupied);
            current.set_totalOccupiedSeconds(0);
        }

        // If occupied is true, occupancyStartTime must be non-zero and plausible.
        if (current.get_occupied()) {
            time_t start = current.get_occupancyStartTime();
            if (start == 0) {
                Log.warn("Current: occupied=true but occupancyStartTime=0 - forcing unoccupied");
                current.set_occupied(false);
                current.set_lastOccupancyEvent(0);
            } else if (Time.isValid()) {
                time_t now = Time.now();
                // If start is in the future by more than a few seconds, clamp.
                if (start > now + 5) {
                    Log.warn("Current: occupancyStartTime in future (%lu > %lu) - clamping", (unsigned long)start, (unsigned long)now);
                    current.set_occupancyStartTime(now);
                }
            }

            // lastOccupancyEvent drives debounce logic; if missing, seed to now to avoid immediate expiry.
            if (current.get_lastOccupancyEvent() == 0) {
                current.set_lastOccupancyEvent(millis());
            }
        }
    }
    if (!valid) {
        Log.warn("Current data is not valid");
    }
    return valid;
}

void currentStatusData::initialize() {
    PersistentDataFile::initialize();

    Log.info("Current Data Initialized");

    currentStatusData::resetEverything();

    // If you manually update fields here, be sure to update the hash
    updateHash();
}

uint16_t currentStatusData::get_faceNumber() const {
    return getValue<uint16_t>(offsetof(CurrentData, faceNumber));
}

void currentStatusData::set_faceNumber(uint16_t value) {
    setValue<uint16_t>(offsetof(CurrentData, faceNumber), value);
}

uint16_t currentStatusData::get_faceScore() const {
    return getValue<uint16_t>(offsetof(CurrentData, faceScore));
}

void currentStatusData::set_faceScore(uint16_t value) {
    setValue<uint16_t>(offsetof(CurrentData, faceScore), value);
}
uint16_t currentStatusData::get_gestureType() const {
    return getValue<uint16_t>(offsetof(CurrentData, gestureType));
}

void currentStatusData::set_gestureType(uint16_t value) {
    setValue<uint16_t>(offsetof(CurrentData, gestureType), value);
}

uint16_t currentStatusData::get_gestureScore() const {
    return getValue<uint16_t>(offsetof(CurrentData, gestureScore));
}

void currentStatusData::set_gestureScore(uint16_t value) {
    setValue<uint16_t>(offsetof(CurrentData, gestureScore), value);
}

time_t currentStatusData::get_lastCountTime() const {
    return getValue<time_t>(offsetof(CurrentData, lastCountTime));
}

void currentStatusData::set_lastCountTime(time_t value) {
    setValue<time_t>(offsetof(CurrentData, lastCountTime), value);
}

float currentStatusData::get_internalTempC() const {
    return getValue<float>(offsetof(CurrentData, internalTempC));
}

void currentStatusData::set_internalTempC(float value) {
    setValue<float>(offsetof(CurrentData, internalTempC), value);
}

float currentStatusData::get_externalTempC() const {
    return getValue<float>(offsetof(CurrentData, externalTempC));
}

void currentStatusData::set_externalTempC(float value) {
    setValue<float>(offsetof(CurrentData, externalTempC), value);
}

int8_t currentStatusData::get_alertCode() const {
    return getValue<int8_t>(offsetof(CurrentData, alertCode));
}

void currentStatusData::set_alertCode(int8_t value) {
    setValue<int8_t>(offsetof(CurrentData, alertCode), value);
}

time_t currentStatusData::get_lastAlertTime() const {
    // lastAlertTime is stored as time_t in CurrentData; retrieve with correct type
    return getValue<time_t>(offsetof(CurrentData,lastAlertTime));
}

void currentStatusData::set_lastAlertTime(time_t value) {
    setValue<time_t>(offsetof(CurrentData,lastAlertTime),value);
}

// Local helper to convert an alert code into a coarse severity bucket.
// Higher numbers indicate more severe conditions.
static int getAlertSeverity(int8_t code) {
    if (code <= 0) {
        return 0; // no alert
    }

    // Map known codes into tiers. This is intentionally simple and can be
    // extended as new alert codes are added over time.
    switch (code) {
        case 14: // out-of-memory
        case 15: // modem / disconnect failure
        case 16: // repeated sleep failures (HIBERNATE / ULP / STOP)
        case 17: // boot storm detected during setup/early boot
        case 18: // state machine thrash detected (ThrashGuard)
        case 20: // PMIC thermal shutdown (critical battery/charging fault)
        case 21: // PMIC charge timeout / stuck charging
            return 3; // critical

        case 23: // PMIC battery fault (general)
        case 30: // connectivity timeout with radio up
        case 31: // failed to connect to cloud
        case 32: // connect taking too long
        case 40: // repeated webhook failures
        case 41: // configuration/ledger apply failure (CONNECT phase)
        case 42: // data ledger publish failure
        case 43: // publish queue not drained before forced sleep
            return 2; // major
        case 44: // ledger sync timeout before sleep (SLEEP phase - cosmetic, config already applied)
            return 1; // minor - less severe than 41 because config is already working
        default:
            return 1; // minor / warning
    }
}

void currentStatusData::raiseAlert(int8_t value) {
    if (value <= 0) {
        return; // ignore attempts to "raise" a non-alert here
    }

    int8_t existing = get_alertCode();
    if (getAlertSeverity(value) > getAlertSeverity(existing)) {
        set_alertCode(value);
        set_lastAlertTime(Time.now());
    }
}

float currentStatusData::get_stateOfCharge() const  {
    return getValue<float>(offsetof(CurrentData,stateOfCharge));
}
void currentStatusData::set_stateOfCharge(float value) {
    setValue<float>(offsetof(CurrentData, stateOfCharge), value);
}

uint8_t currentStatusData::get_batteryState() const  {
    return getValue<uint8_t>(offsetof(CurrentData, batteryState));
}
void currentStatusData::set_batteryState(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, batteryState), value);
}

// ********** Counting Mode Get/Set Functions **********

uint16_t currentStatusData::get_hourlyCount() const {
    return getValue<uint16_t>(offsetof(CurrentData, hourlyCount));
}
void currentStatusData::set_hourlyCount(uint16_t value) {
    setValue<uint16_t>(offsetof(CurrentData, hourlyCount), value);
}

uint16_t currentStatusData::get_dailyCount() const {
    return getValue<uint16_t>(offsetof(CurrentData, dailyCount));
}
void currentStatusData::set_dailyCount(uint16_t value) {
    setValue<uint16_t>(offsetof(CurrentData, dailyCount), value);
}

// ********** Occupancy Mode Get/Set Functions **********

bool currentStatusData::get_occupied() const {
    return getValue<bool>(offsetof(CurrentData, occupied));
}
void currentStatusData::set_occupied(bool value) {
    setValue<bool>(offsetof(CurrentData, occupied), value);
}

uint32_t currentStatusData::get_lastOccupancyEvent() const {
    return getValue<uint32_t>(offsetof(CurrentData, lastOccupancyEvent));
}
void currentStatusData::set_lastOccupancyEvent(uint32_t value) {
    setValue<uint32_t>(offsetof(CurrentData, lastOccupancyEvent), value);
}

time_t currentStatusData::get_occupancyStartTime() const {
    return getValue<time_t>(offsetof(CurrentData, occupancyStartTime));
}
void currentStatusData::set_occupancyStartTime(time_t value) {
    setValue<time_t>(offsetof(CurrentData, occupancyStartTime), value);
}

uint32_t currentStatusData::get_totalOccupiedSeconds() const {
    return getValue<uint32_t>(offsetof(CurrentData, totalOccupiedSeconds));
}
void currentStatusData::set_totalOccupiedSeconds(uint32_t value) {
    setValue<uint32_t>(offsetof(CurrentData, totalOccupiedSeconds), value);
}

// End of currentStatusData class