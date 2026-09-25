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
#include "Config.h"
#include "persist/CurrentReadings.h"
#include "persist/PowerConfig.h"
#include "persist/RecoveryState.h"
#include "persist/SystemConfig.h"

// WO-2026-09-18 Step 3a: isClockTrusted() is now declared by time/Clock.h,
// a small, real header (not state/State_Common.h, which transitively
// includes state/StateHandlers.h and state/StateMachine.h - more than this
// data-persistence translation unit needs) - the correct-direction include
// a forward declaration used to stand in for.
#include "time/Clock.h"

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

        if (sysStatus.get_closeTime() > 23) {
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

        if (sysStatus.getValue<uint8_t>(offsetof(SysData,reservedByte0)) > 1) {
            Log.info("data not valid enable hibernate flag");
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
    sysStatus.set_timeZoneStr(Config::DEFAULT_TIMEZONE);
    sysStatus.set_sensorType(1);                // PIR sensor
    sysStatus.set_openTime(Config::DEFAULT_OPEN_HOUR);
    sysStatus.set_closeTime(Config::DEFAULT_CLOSE_HOUR);
    sysStatus.set_reportingInterval(Config::DEFAULT_REPORT_INTERVAL_SEC);
    sysStatus.set_lastConnectionDuration(0);                               // New measure
    sysStatus.set_lastDailyCleanup(0);                                     // No cleanup has run yet
    
    // ********** Operating Mode Defaults **********
    sysStatus.set_sensorMode(SystemConfig::COUNTING);                      // Default to counting mode
    sysStatus.set_connectionMode(SystemConfig::CONNECTED);                 // Default to connected mode
    sysStatus.set_reportingMode(SystemConfig::SCHEDULED);                  // Default to scheduled reporting
    sysStatus.set_samplingMode(SystemConfig::INTERRUPT);                   // Default to interrupt-driven
    sysStatus.set_verboseTimeoutMin(60);                                   // Default 60 min verbose timeout
    sysStatus.set_verboseModeStartTime(0);                                 // Verbose mode not active
    sysStatus.set_connectAttemptBudgetSec(Config::DEFAULT_CONNECT_ATTEMPT_BUDGET_SEC);
    sysStatus.set_cloudDisconnectBudgetSec(Config::DEFAULT_CLOUD_DISCONNECT_BUDGET_SEC);
    sysStatus.set_modemOffBudgetSec(Config::DEFAULT_MODEM_OFF_BUDGET_SEC);
    sysStatus.set_enableHibernateSleep(false);
    sysStatus.set_connectivityRecoveryStage(0);
    sysStatus.set_lastConnectivityRecoveryAction(0);
    sysStatus.set_connectivityRecoveryCount(0);
    sysStatus.set_watchdogResetCount(0);
    sysStatus.set_lastWatchdogBreadcrumb(0);
    sysStatus.set_lastWatchdogUptimeMs(0);
    sysStatus.set_lastWatchdogResetReasonData(0);
    sysStatus.set_hasValidLedgerConfig(false);
    sysStatus.set_configSource((uint8_t)Config::CONFIG_SOURCE_DEFAULT);
    sysStatus.set_lastWatchdogSource((uint8_t)RecoveryState::WATCHDOG_SOURCE_DEVICE_OS);
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

bool sysStatusData::get_enableHibernateSleep() const {
    return getValue<uint8_t>(offsetof(SysData,reservedByte0)) != 0;
}
void sysStatusData::set_enableHibernateSleep(bool value) {
    setValue<uint8_t>(offsetof(SysData,reservedByte0), value ? 1 : 0);
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

bool sysStatusData::get_hasValidLedgerConfig() const {
    return getValue<bool>(offsetof(SysData,hasValidLedgerConfig));
}
void sysStatusData::set_hasValidLedgerConfig(bool value) {
    setValue<bool>(offsetof(SysData,hasValidLedgerConfig), value);
}

uint8_t sysStatusData::get_configSource() const {
    return getValue<uint8_t>(offsetof(SysData,configSource));
}
void sysStatusData::set_configSource(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData,configSource), value);
}

uint8_t sysStatusData::get_lastWatchdogSource() const {
    return getValue<uint8_t>(offsetof(SysData,lastWatchdogSource));
}
void sysStatusData::set_lastWatchdogSource(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData,lastWatchdogSource), value);
}

// F2a thermal charge-inhibit thresholds (WO-2026-08-25-001). These fields were
// appended after initial provisioning, so initialize() never runs for
// already-provisioned devices - StorageHelperRK::validate() zero-pads the
// structure extension instead. {0,0,0,0} is invalid AS A SET (armHighC(0) is
// not > releaseHighC(0)) even though 0.0f is individually plausible for any
// one field (armLowC's own compiled default IS 0.0f). Validate and resolve
// the whole set via ChargeInhibitPolicy::isValidThermalThresholds() - the
// same validator AC-B6 already applies to ledger candidates - so the stored/
// migrated read path can never disagree with the ledger-candidate path, and
// a caller never ends up mixing some stored fields with some defaults.
ChargeInhibitPolicy::ThermalThresholds sysStatusData::resolveThermalThresholds() const {
    ChargeInhibitPolicy::ThermalThresholds candidate;
    candidate.armHighC = getValue<float>(offsetof(SysData,thermalChargeArmHighC));
    candidate.armLowC = getValue<float>(offsetof(SysData,thermalChargeArmLowC));
    candidate.releaseHighC = getValue<float>(offsetof(SysData,thermalChargeReleaseHighC));
    candidate.releaseLowC = getValue<float>(offsetof(SysData,thermalChargeReleaseLowC));
    return ChargeInhibitPolicy::resolveStoredThermalThresholds(candidate);
}

float sysStatusData::get_thermalChargeArmHighC() const {
    return resolveThermalThresholds().armHighC;
}
void sysStatusData::set_thermalChargeArmHighC(float value) {
    setValue<float>(offsetof(SysData,thermalChargeArmHighC), value);
}

float sysStatusData::get_thermalChargeArmLowC() const {
    return resolveThermalThresholds().armLowC;
}
void sysStatusData::set_thermalChargeArmLowC(float value) {
    setValue<float>(offsetof(SysData,thermalChargeArmLowC), value);
}

float sysStatusData::get_thermalChargeReleaseHighC() const {
    return resolveThermalThresholds().releaseHighC;
}
void sysStatusData::set_thermalChargeReleaseHighC(float value) {
    setValue<float>(offsetof(SysData,thermalChargeReleaseHighC), value);
}

float sysStatusData::get_thermalChargeReleaseLowC() const {
    return resolveThermalThresholds().releaseLowC;
}
void sysStatusData::set_thermalChargeReleaseLowC(float value) {
    setValue<float>(offsetof(SysData,thermalChargeReleaseLowC), value);
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

    sensorConfig.set_sensorType(1);
    sensorConfig.set_sensorSetting1(Config::DEFAULT_OCCUPANCY_DEBOUNCE_MS);
    sensorConfig.set_sensorSetting2(0);
    sensorConfig.set_sensorSetting3(0);
    sensorConfig.set_sensorSetting4(0);

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
  // WO-2026-08-29-002 item 8: lastCountTime has no consumers anywhere in
  // this codebase (write-only telemetry field) - gating it on the trust
  // signal changes only the recorded value, never control flow. See
  // isClockTrusted()'s doc comment (state/State_Common.h) for why
  // Time.isValid() alone is not sufficient (Finding 3).
  current.set_lastCountTime(isClockTrusted() ? Time.now() : 0);
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
            }
            // else: no time source yet this boot (see WO-2026-09-22-001) -
            // this same start value is re-checked, once a time source
            // exists, by revalidateOccupancyStartTimeIfTimeAvailable()
            // below, called once from global setup().

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

void currentStatusData::revalidateOccupancyStartTimeIfTimeAvailable() {
    if (!Clock::isTimeValid() || !current.get_occupied()) {
        return;
    }

    time_t start = current.get_occupancyStartTime();
    time_t now = Time.now();
    // If start is in the future by more than a few seconds, clamp.
    if (start > now + 5) {
        Log.warn("Current: occupancyStartTime in future (%lu > %lu) - clamping", (unsigned long)start, (unsigned long)now);
        current.set_occupancyStartTime(now);
    }
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

        // Watchdog/external reset represents actual lost execution - the
        // device stopped responding entirely and had to be forcibly reset.
        // That is a strictly more urgent, must-not-be-masked signal than any
        // tier-3 condition above (e.g. thrash detection or a boot storm)
        // that merely happened to already be active when the watchdog fired.
        // It needs its own tier, strictly greater than 3, so raiseAlert(19)
        // always supersedes whatever is currently active from that set.
        case 19: // watchdog reset (Device-OS-detected or AB1805-confirmed PIN_RESET)
            return 4; // watchdog / lost-execution - most urgent

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
        // WO-2026-08-29-002 item 8: deliberately NOT gated on
        // isClockTrusted(). State_Report.cpp's alert-40 escalation cooldown
        // reads get_lastAlertTime() as `lastEscalation` and treats
        // `lastEscalation == 0` as "no prior escalation - cooldown already
        // passed" (a control-flow branch, not just a recorded value). If an
        // alert were raised while the clock is untrusted and this wrote 0,
        // a later trusted-clock read would see 0 and incorrectly bypass the
        // 3-hour escalation cooldown. Writing Time.now() even when
        // untrusted preserves the existing cooldown behavior; see the
        // Implementation Report for the Chief Engineer's review.
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

// ***************  Facade implementations (WO-2026-09-23-001, Step 5)  ***************
//
// The four persist/*.h facades declare narrow, storage-free interfaces. This is
// the only translation unit that sees both those declarations and the
// StorageHelperRK-backed classes above, which is what keeps StorageHelperRK.h
// out of all 27 consumers.
//
// Every function here is a straight forward to the pre-split call. Where a call
// carried behavior beyond a field access - validate()'s size argument, the
// cross-store reset in resetEverything(), raiseAlert()'s severity arbitration -
// that behavior stays in the class method and is not reimplemented here.

// The facades publish these capacities so ConfigApply.cpp can size its staging
// buffers without seeing SysData. Binding them to the real fields here means a
// future field-width change breaks the build instead of silently truncating.
static_assert(SystemConfig::kTimeZoneCapacity == sizeof(sysStatusData::SysData::timeZoneStr),
              "SystemConfig::kTimeZoneCapacity must match SysData::timeZoneStr");
static_assert(SystemConfig::kWebhookNameCapacity == sizeof(sysStatusData::SysData::webhookName),
              "SystemConfig::kWebhookNameCapacity must match SysData::webhookName");

namespace SystemConfig {

void setup() { sysStatus.setup(); }
void loop() { sysStatus.loop(); }
void flushNow() { sysStatus.flush(true); }

// sizeof(sysStatusData), matching the pre-split sysStatus.validate(sizeof(sysStatus)).
bool validateStoredData() { return sysStatus.validate(sizeof(sysStatusData)); }

bool get_verboseMode() { return sysStatus.get_verboseMode(); }
void set_verboseMode(bool value) { sysStatus.set_verboseMode(value); }

uint16_t get_verboseTimeoutMin() { return sysStatus.get_verboseTimeoutMin(); }
void set_verboseTimeoutMin(uint16_t value) { sysStatus.set_verboseTimeoutMin(value); }

time_t get_verboseModeStartTime() { return sysStatus.get_verboseModeStartTime(); }
void set_verboseModeStartTime(time_t value) { sysStatus.set_verboseModeStartTime(value); }

bool get_serialConnected() { return sysStatus.get_serialConnected(); }
void set_serialConnected(bool value) { sysStatus.set_serialConnected(value); }

const char *get_timeZoneStrCStr() { return sysStatus.get_timeZoneStrCStr(); }
bool set_timeZoneStr(const char *str) { return sysStatus.set_timeZoneStr(str); }

uint8_t get_openTime() { return sysStatus.get_openTime(); }
void set_openTime(uint8_t value) { sysStatus.set_openTime(value); }

uint8_t get_closeTime() { return sysStatus.get_closeTime(); }
void set_closeTime(uint8_t value) { sysStatus.set_closeTime(value); }

uint16_t get_reportingInterval() { return sysStatus.get_reportingInterval(); }
void set_reportingInterval(uint16_t value) { sysStatus.set_reportingInterval(value); }

time_t get_lastReport() { return sysStatus.get_lastReport(); }
void set_lastReport(time_t value) { sysStatus.set_lastReport(value); }

time_t get_lastHookResponse() { return sysStatus.get_lastHookResponse(); }
void set_lastHookResponse(time_t value) { sysStatus.set_lastHookResponse(value); }

time_t get_lastDailyCleanup() { return sysStatus.get_lastDailyCleanup(); }
void set_lastDailyCleanup(time_t value) { sysStatus.set_lastDailyCleanup(value); }

time_t get_lastTimeSync() { return sysStatus.get_lastTimeSync(); }
void set_lastTimeSync(time_t value) { sysStatus.set_lastTimeSync(value); }

uint8_t get_sensorType() { return sysStatus.get_sensorType(); }
void set_sensorType(uint8_t value) { sysStatus.set_sensorType(value); }

uint8_t get_sensorMode() { return sysStatus.get_sensorMode(); }
void set_sensorMode(uint8_t value) { sysStatus.set_sensorMode(value); }

uint8_t get_connectionMode() { return sysStatus.get_connectionMode(); }
void set_connectionMode(uint8_t value) { sysStatus.set_connectionMode(value); }

uint8_t get_reportingMode() { return sysStatus.get_reportingMode(); }
void set_reportingMode(uint8_t value) { sysStatus.set_reportingMode(value); }

uint8_t get_samplingMode() { return sysStatus.get_samplingMode(); }
void set_samplingMode(uint8_t value) { sysStatus.set_samplingMode(value); }

bool get_disconnectedMode() { return sysStatus.get_disconnectedMode(); }
void set_disconnectedMode(bool value) { sysStatus.set_disconnectedMode(value); }

bool get_enableHibernateSleep() { return sysStatus.get_enableHibernateSleep(); }
void set_enableHibernateSleep(bool value) { sysStatus.set_enableHibernateSleep(value); }

uint16_t get_connectAttemptBudgetSec() { return sysStatus.get_connectAttemptBudgetSec(); }
void set_connectAttemptBudgetSec(uint16_t value) { sysStatus.set_connectAttemptBudgetSec(value); }

uint16_t get_cloudDisconnectBudgetSec() { return sysStatus.get_cloudDisconnectBudgetSec(); }
void set_cloudDisconnectBudgetSec(uint16_t value) { sysStatus.set_cloudDisconnectBudgetSec(value); }

uint16_t get_modemOffBudgetSec() { return sysStatus.get_modemOffBudgetSec(); }
void set_modemOffBudgetSec(uint16_t value) { sysStatus.set_modemOffBudgetSec(value); }

uint8_t get_connectionAttemptCounter() { return sysStatus.get_connectionAttemptCounter(); }
void set_connectionAttemptCounter(uint8_t value) { sysStatus.set_connectionAttemptCounter(value); }

time_t get_lastConnection() { return sysStatus.get_lastConnection(); }
void set_lastConnection(time_t value) { sysStatus.set_lastConnection(value); }

uint16_t get_lastConnectionDuration() { return sysStatus.get_lastConnectionDuration(); }
void set_lastConnectionDuration(uint16_t value) { sysStatus.set_lastConnectionDuration(value); }

uint16_t get_testConnectionDurationOverride() { return sysStatus.get_testConnectionDurationOverride(); }
void set_testConnectionDurationOverride(uint16_t value) { sysStatus.set_testConnectionDurationOverride(value); }

const char *get_webhookNameCStr() { return sysStatus.get_webhookNameCStr(); }
bool set_webhookName(const char *str) { return sysStatus.set_webhookName(str); }

bool get_webhookEnabled() { return sysStatus.get_webhookEnabled(); }
void set_webhookEnabled(bool value) { sysStatus.set_webhookEnabled(value); }

uint32_t get_webhookTimeoutMs() { return sysStatus.get_webhookTimeoutMs(); }
void set_webhookTimeoutMs(uint32_t value) { sysStatus.set_webhookTimeoutMs(value); }

bool get_hasValidLedgerConfig() { return sysStatus.get_hasValidLedgerConfig(); }
void set_hasValidLedgerConfig(bool value) { sysStatus.set_hasValidLedgerConfig(value); }

uint8_t get_configSource() { return sysStatus.get_configSource(); }
void set_configSource(uint8_t value) { sysStatus.set_configSource(value); }

namespace SensorSettings {

void setup() { sensorConfig.setup(); }
void loop() { sensorConfig.loop(); }

// sizeof(sensorConfigData), matching the pre-split sensorConfig.validate(sizeof(sensorConfig)).
bool validateStoredData() { return sensorConfig.validate(sizeof(sensorConfigData)); }

uint8_t get_sensorType() { return sensorConfig.get_sensorType(); }
void set_sensorType(uint8_t value) { sensorConfig.set_sensorType(value); }

uint32_t get_sensorSetting1() { return sensorConfig.get_sensorSetting1(); }
void set_sensorSetting1(uint32_t value) { sensorConfig.set_sensorSetting1(value); }

uint32_t get_sensorSetting2() { return sensorConfig.get_sensorSetting2(); }
void set_sensorSetting2(uint32_t value) { sensorConfig.set_sensorSetting2(value); }

uint32_t get_sensorSetting3() { return sensorConfig.get_sensorSetting3(); }
void set_sensorSetting3(uint32_t value) { sensorConfig.set_sensorSetting3(value); }

uint32_t get_sensorSetting4() { return sensorConfig.get_sensorSetting4(); }
void set_sensorSetting4(uint32_t value) { sensorConfig.set_sensorSetting4(value); }

}  // namespace SensorSettings

}  // namespace SystemConfig

namespace RecoveryState {

uint8_t get_resetCount() { return sysStatus.get_resetCount(); }
void set_resetCount(uint8_t value) { sysStatus.set_resetCount(value); }

uint8_t get_connectivityRecoveryStage() { return sysStatus.get_connectivityRecoveryStage(); }
void set_connectivityRecoveryStage(uint8_t value) { sysStatus.set_connectivityRecoveryStage(value); }

time_t get_lastConnectivityRecoveryAction() { return sysStatus.get_lastConnectivityRecoveryAction(); }
void set_lastConnectivityRecoveryAction(time_t value) { sysStatus.set_lastConnectivityRecoveryAction(value); }

uint8_t get_connectivityRecoveryCount() { return sysStatus.get_connectivityRecoveryCount(); }
void set_connectivityRecoveryCount(uint8_t value) { sysStatus.set_connectivityRecoveryCount(value); }

uint16_t get_watchdogResetCount() { return sysStatus.get_watchdogResetCount(); }
void set_watchdogResetCount(uint16_t value) { sysStatus.set_watchdogResetCount(value); }

uint8_t get_lastWatchdogBreadcrumb() { return sysStatus.get_lastWatchdogBreadcrumb(); }
void set_lastWatchdogBreadcrumb(uint8_t value) { sysStatus.set_lastWatchdogBreadcrumb(value); }

uint32_t get_lastWatchdogUptimeMs() { return sysStatus.get_lastWatchdogUptimeMs(); }
void set_lastWatchdogUptimeMs(uint32_t value) { sysStatus.set_lastWatchdogUptimeMs(value); }

uint32_t get_lastWatchdogResetReasonData() { return sysStatus.get_lastWatchdogResetReasonData(); }
void set_lastWatchdogResetReasonData(uint32_t value) { sysStatus.set_lastWatchdogResetReasonData(value); }

uint8_t get_lastWatchdogSource() { return sysStatus.get_lastWatchdogSource(); }
void set_lastWatchdogSource(uint8_t value) { sysStatus.set_lastWatchdogSource(value); }

// Decision A: alert state is forensics, so it is exposed here - but it is
// still physically CurrentData, so these forward to `current`, not `sysStatus`.
int8_t get_alertCode() { return current.get_alertCode(); }
void set_alertCode(int8_t value) { current.set_alertCode(value); }
void raiseAlert(int8_t value) { current.raiseAlert(value); }

time_t get_lastAlertTime() { return current.get_lastAlertTime(); }
void set_lastAlertTime(time_t value) { current.set_lastAlertTime(value); }

}  // namespace RecoveryState

namespace PowerConfig {

bool get_solarPowerMode() { return sysStatus.get_solarPowerMode(); }
void set_solarPowerMode(bool value) { sysStatus.set_solarPowerMode(value); }

bool get_lowPowerMode() { return sysStatus.get_lowPowerMode(); }
void set_lowPowerMode(bool value) { sysStatus.set_lowPowerMode(value); }

bool get_lowBatteryMode() { return sysStatus.get_lowBatteryMode(); }
void set_lowBatteryMode(bool value) { sysStatus.set_lowBatteryMode(value); }

uint8_t get_currentBatteryTier() { return sysStatus.get_currentBatteryTier(); }
void set_currentBatteryTier(uint8_t value) { sysStatus.set_currentBatteryTier(value); }

float get_thermalChargeArmHighC() { return sysStatus.get_thermalChargeArmHighC(); }
void set_thermalChargeArmHighC(float value) { sysStatus.set_thermalChargeArmHighC(value); }

float get_thermalChargeArmLowC() { return sysStatus.get_thermalChargeArmLowC(); }
void set_thermalChargeArmLowC(float value) { sysStatus.set_thermalChargeArmLowC(value); }

float get_thermalChargeReleaseHighC() { return sysStatus.get_thermalChargeReleaseHighC(); }
void set_thermalChargeReleaseHighC(float value) { sysStatus.set_thermalChargeReleaseHighC(value); }

float get_thermalChargeReleaseLowC() { return sysStatus.get_thermalChargeReleaseLowC(); }
void set_thermalChargeReleaseLowC(float value) { sysStatus.set_thermalChargeReleaseLowC(value); }

}  // namespace PowerConfig

namespace CurrentReadings {

void setup() { current.setup(); }
void loop() { current.loop(); }
void resetEverything() { current.resetEverything(); }
void revalidateOccupancyStartTimeIfTimeAvailable() { current.revalidateOccupancyStartTimeIfTimeAvailable(); }

uint16_t get_hourlyCount() { return current.get_hourlyCount(); }
void set_hourlyCount(uint16_t value) { current.set_hourlyCount(value); }

uint16_t get_dailyCount() { return current.get_dailyCount(); }
void set_dailyCount(uint16_t value) { current.set_dailyCount(value); }

time_t get_lastCountTime() { return current.get_lastCountTime(); }
void set_lastCountTime(time_t value) { current.set_lastCountTime(value); }

bool get_occupied() { return current.get_occupied(); }
void set_occupied(bool value) { current.set_occupied(value); }

uint32_t get_lastOccupancyEvent() { return current.get_lastOccupancyEvent(); }
void set_lastOccupancyEvent(uint32_t value) { current.set_lastOccupancyEvent(value); }

time_t get_occupancyStartTime() { return current.get_occupancyStartTime(); }
void set_occupancyStartTime(time_t value) { current.set_occupancyStartTime(value); }

uint32_t get_totalOccupiedSeconds() { return current.get_totalOccupiedSeconds(); }
void set_totalOccupiedSeconds(uint32_t value) { current.set_totalOccupiedSeconds(value); }

float get_stateOfCharge() { return current.get_stateOfCharge(); }
void set_stateOfCharge(float value) { current.set_stateOfCharge(value); }

uint8_t get_batteryState() { return current.get_batteryState(); }
void set_batteryState(uint8_t value) { current.set_batteryState(value); }

float get_internalTempC() { return current.get_internalTempC(); }
void set_internalTempC(float value) { current.set_internalTempC(value); }

float get_externalTempC() { return current.get_externalTempC(); }
void set_externalTempC(float value) { current.set_externalTempC(value); }

}  // namespace CurrentReadings
