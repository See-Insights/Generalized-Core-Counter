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

        if (sysStatus.get_openTime() < 0 || sysStatus.get_openTime() > 12) {
            Log.info("data not valid open time =%d", sysStatus.get_openTime());
            valid = false;
        }
        else if (sysStatus.get_lastConnection() < 0 || sysStatus.get_lastConnectionDuration() > 900) {
            Log.info("data not valid last connection duration =%d", sysStatus.get_lastConnectionDuration());
            valid = false;
        }
    }
    Log.info("sysStatus data is %s",(valid) ? "valid": "not valid");
    return valid;
}

void sysStatusData::initialize() {
    PersistentDataFile::initialize();

    const char message[26] = "Loading System Defaults";
    Log.info(message);
    if (Particle.connected()) Particle.publish("Mode",message, PRIVATE);
    Log.info("Loading system defaults");
    sysStatus.set_structuresVersion(1);
    sysStatus.set_verboseMode(false);
    sysStatus.set_lowBatteryMode(false);
    sysStatus.set_solarPowerMode(true);
    sysStatus.set_lowPowerMode(false);          // This should be changed to true once we have tested
    sysStatus.set_timeZoneStr("ANAT-12");     // NZ Time
    sysStatus.set_sensorType(1);                // PIR sensor
    sysStatus.set_openTime(0);
    sysStatus.set_closeTime(24);                                           // New standard with v20
    sysStatus.set_lastConnectionDuration(0);                               // New measure
    
    // ********** Operating Mode Defaults **********
    sysStatus.set_countingMode(COUNTING);                                  // Default to counting mode
    sysStatus.set_operatingMode(CONNECTED);                                // Default to connected mode
    sysStatus.set_triggerMode(INTERRUPT);                                  // Default to interrupt-driven
    sysStatus.set_occupancyDebounceMs(300000);                            // Default 5 minutes (300,000 ms)
    sysStatus.set_connectedReportingIntervalSec(300);                     // Default 5 minutes when connected
    sysStatus.set_lowPowerReportingIntervalSec(3600);                     // Default 1 hour when in low power
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

// ********** Operating Mode Configuration Get/Set Functions **********

uint8_t sysStatusData::get_countingMode() const {
    return getValue<uint8_t>(offsetof(SysData,countingMode));
}
void sysStatusData::set_countingMode(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData,countingMode), value);
}

uint8_t sysStatusData::get_operatingMode() const {
    return getValue<uint8_t>(offsetof(SysData,operatingMode));
}
void sysStatusData::set_operatingMode(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData,operatingMode), value);
}

uint8_t sysStatusData::get_triggerMode() const {
    return getValue<uint8_t>(offsetof(SysData,triggerMode));
}
void sysStatusData::set_triggerMode(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData,triggerMode), value);
}

uint32_t sysStatusData::get_occupancyDebounceMs() const {
    return getValue<uint32_t>(offsetof(SysData,occupancyDebounceMs));
}
void sysStatusData::set_occupancyDebounceMs(uint32_t value) {
    setValue<uint32_t>(offsetof(SysData,occupancyDebounceMs), value);
}

uint16_t sysStatusData::get_connectedReportingIntervalSec() const {
    return getValue<uint16_t>(offsetof(SysData,connectedReportingIntervalSec));
}
void sysStatusData::set_connectedReportingIntervalSec(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData,connectedReportingIntervalSec), value);
}

uint16_t sysStatusData::get_lowPowerReportingIntervalSec() const {
    return getValue<uint16_t>(offsetof(SysData,lowPowerReportingIntervalSec));
}
void sysStatusData::set_lowPowerReportingIntervalSec(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData,lowPowerReportingIntervalSec), value);
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
    if (valid) {
        if (sensorConfig.get_threshold1() > 100 || sensorConfig.get_threshold2() > 100) {
            Log.info("Sensor config: thresholds not valid (threshold1=%d, threshold2=%d)", 
                     sensorConfig.get_threshold1(), sensorConfig.get_threshold2());
            valid = false;
        }
    }
    Log.info("Sensor config is %s", (valid) ? "valid" : "not valid");
    return valid;
}

void sensorConfigData::initialize() {
    PersistentDataFile::initialize();

    Log.info("Current Data Initialized");

    // If you manually update fields here, be sure to update the hash
    updateHash();
}

uint16_t sensorConfigData::get_threshold1() const {
    return getValue<uint16_t>(offsetof(SensorData, threshold1));
}

void sensorConfigData::set_threshold1(uint16_t value) {
    setValue<uint16_t>(offsetof(SensorData, threshold1), value);
}

uint16_t sensorConfigData::get_threshold2() const {
    return getValue<uint16_t>(offsetof(SensorData, threshold2));
}

void sensorConfigData::set_threshold2(uint16_t value) {
    setValue<uint16_t>(offsetof(SensorData, threshold2), value);
}
uint16_t sensorConfigData::get_pollingRate() const {
    return getValue<uint16_t>(offsetof(SensorData, pollingRate));
}

void sensorConfigData::set_pollingRate(uint16_t value) {
    setValue<uint16_t>(offsetof(SensorData, pollingRate), value);
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
    }
    Log.info("Current data is %s", (valid) ? "valid" : "not valid");
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
    return getValue<int8_t>(offsetof(CurrentData,lastAlertTime));
}

void currentStatusData::set_lastAlertTime(time_t value) {
    setValue<time_t>(offsetof(CurrentData,lastAlertTime),value);
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