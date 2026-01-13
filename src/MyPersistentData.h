/**
 * @file MyPersistentData.h
 * @brief Persistent Data Storage Structures - EEPROM/Retained Memory Management
 * 
 * @details Defines data structures for device configuration, sensor settings, and runtime state
 *          that persist across power cycles and reboots. Uses StorageHelperRK for efficient
 *          EEPROM/retained memory operations with automatic validation and versioning.
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

#ifndef __MYPERSISTENTDATA_H
#define __MYPERSISTENTDATA_H

#include "Particle.h"
#include "StorageHelperRK.h" 
// This way you can do "data.setup()" instead of "MyPersistentData::instance().setup()" as an example
#define current currentStatusData::instance()
#define sysStatus sysStatusData::instance()
#define sensorConfig sensorConfigData::instance()

// *************** Operating Mode Enumerations ***************
/**
 * @brief Counting mode defines how the device processes sensor events
 * 
 * COUNTING:   Interrupt-driven event counting.
 * OCCUPANCY:  Interrupt-driven occupied/unoccupied tracking.
 * SCHEDULED:  Non-interrupt, time-based sampling.
 */
enum CountingMode {
	COUNTING   = 0,  // Count each detection event (interrupt-driven)
	OCCUPANCY = 1,  // Track occupied/unoccupied state with debounce (interrupt-driven)
	SCHEDULED = 2   // Time-based polling (non-interrupt)
};

/**
 * @brief Operating mode defines power and connectivity behavior
 * 
 * CONNECTED:     Device stays connected to cloud, reports frequently.
 * LOW_POWER:     Device disconnects between reports to save battery.
 * DISCONNECTED:  Device never auto-connects (test / bench mode).
 */
enum OperatingMode {
	CONNECTED    = 0,  // Always connected, frequent reporting
	LOW_POWER    = 1,  // Disconnect and sleep between reports
	DISCONNECTED = 2   // Stay offline unless manually overridden
};

// NOTE: TriggerMode has been deprecated in favor of using
// CountingMode (COUNTING/OCCUPANCY/SCHEDULED) as the single
// source of truth for behavioral mode. The legacy TriggerMode
// enum and associated storage have been removed.

/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * MyPersistentData::instance().setup();
 * 
 * From global application loop you must call:
 * MyPersistentData::instance().loop();
 */

// *******************  SysStatus Storage Object **********************
//
// ********************************************************************

class sysStatusData : public StorageHelperRK::PersistentDataFile {
public:

    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use MyPersistentData::instance() to instantiate the singleton.
     */
    static sysStatusData &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use MyPersistentData::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use MyPersistentData::instance().loop();
     */
    void loop();

	/**
	 * @brief Validates values and, if valid, checks that data is in the correct range.
	 * 
	 */
	bool validate(size_t dataSize);

	/**
	 * @brief Will reinitialize data if it is found not to be valid
	 * 
	 * Be careful doing this, because when MyData is extended to add new fields,
	 * the initialize method is not called! This is only called when first
	 * initialized.
	 * 
	 */
	void initialize();



	class SysData {
	public:
		// This structure must always begin with the header (16 bytes)
		StorageHelperRK::PersistentDataBase::SavedDataHeader sysHeader;
		// Your fields go here. Once you've added a field you cannot add fields
		// (except at the end), insert fields, remove fields, change size of a field.
		// Doing so will cause the data to be corrupted!
		uint8_t structuresVersion;                        // Version of the data structures (system and data)
		bool verboseMode;                                 // Turns on extra messaging
		bool solarPowerMode;                              // Powered by a solar panel or utility power
  		bool lowPowerMode;                                // Does the device need to run disconnected to save battery
		bool lowBatteryMode;                              // Is the battery level so low that we can no longer connect
		uint8_t resetCount;                               // reset count of device (0-256)
		char timeZoneStr[39];							  // String for the timezone - https://developer.ibm.com/technologies/systems/articles/au-aix-posix/
		uint8_t openTime;                                 // Hour the park opens (0-23)
		uint8_t closeTime;                                // Hour the park closes (0-23)
		time_t lastReport;								  // The last time we sent a webhook to the queue
		time_t lastConnection;                     		  // Last time we successfully connected to Particle
		time_t lastHookResponse;                   		  // Last time we got a valid Webhook response
		uint16_t lastConnectionDuration;                  // How long - in seconds - did it take to last connect to the Particle cloud
		uint8_t sensorType;                               // What is the sensor type - 0-Pressure Sensor, 1-PIR Sensor
		bool updatesPending;                              // Has there been a change to the sysStatus that we need to effect 
		uint16_t reportingInterval;                       // How often do we report in to the Particle cloud - in seconds
		bool disconnectedMode;                            // Are we in disconnected mode - this is used to prevent the device from trying to connect to the Particle cloud - for Development and testing purposes
		bool serialConnected;							  // Is the serial port connected - used to determine if we should wait for a serial connection before starting the device
		time_t lastDailyCleanup;                           // Last time dailyCleanup() successfully ran
		
		// ********** Operating Mode Configuration **********
		uint8_t countingMode;                             // 0=COUNTING, 1=OCCUPANCY, 2=SCHEDULED (time-based)
		uint8_t operatingMode;                            // 0=CONNECTED, 1=LOW_POWER, 2=DISCONNECTED
		uint32_t occupancyDebounceMs;                     // Milliseconds to wait before marking space as unoccupied (occupancy mode only)
		uint16_t connectedReportingIntervalSec;           // Reporting interval in seconds when in CONNECTED mode
		uint16_t lowPowerReportingIntervalSec;            // Reporting interval in seconds when in LOW_POWER mode
		uint16_t connectAttemptBudgetSec;                 // Max seconds to spend attempting a cloud connect per wake
		uint16_t cloudDisconnectBudgetSec;                // Max seconds to wait for cloud disconnect before error
		uint16_t modemOffBudgetSec;                       // Max seconds to wait for modem power-down before error

	};

	SysData sysData;

	// 	******************* Get and Set Functions for each variable in the storage object ***********
    
	/**
	 * @brief For the Get functions, used to retrieve the value of the variable
	 * 
	 * @details Specific to the location in the object and the type of the variable
	 * 
	 * @param Nothing needed
	 * 
	 * @returns The value of the variable in the corret type
	 * 
	 */

	/**
	 * @brief For the Set functions, used to set the value of the variable
	 * 
	 * @details Specific to the location in the object and the type of the variable
	 * 
	 * @param Value to set the variable - correct type - for Strings they are truncated if too long
	 * 
	 * @returns None needed
	 * 
	 */

	uint8_t get_structuresVersion() const ;
	void set_structuresVersion(uint8_t value);

	bool get_verboseMode() const;
	void set_verboseMode(bool value);

	bool get_solarPowerMode() const;
	void set_solarPowerMode(bool value);

	bool get_lowPowerMode() const;
	void set_lowPowerMode(bool value);

	bool get_lowBatteryMode() const;
	void set_lowBatteryMode(bool value);

	uint8_t get_resetCount() const;
	void set_resetCount(uint8_t value);

	String get_timeZoneStr() const;
	bool set_timeZoneStr(const char *str);

	uint8_t get_openTime() const;
	void set_openTime(uint8_t value);

	uint8_t get_closeTime() const;
	void set_closeTime(uint8_t value);

	time_t get_lastReport() const;
	void set_lastReport(time_t value);

	time_t get_lastConnection() const;
	void set_lastConnection(time_t value);

	uint16_t get_lastConnectionDuration() const;
	void set_lastConnectionDuration(uint16_t value);

	time_t get_lastHookResponse() const;
	void set_lastHookResponse(time_t value);

	uint8_t get_alertCodeNode() const;
	void set_alertCodeNode(uint8_t value);

	time_t get_alertTimestampNode() const;
	void set_alertTimestampNode(time_t value);

	uint8_t get_sensorType() const;
	void set_sensorType(uint8_t value);

	bool get_updatesPending() const;
	void set_updatesPending(bool value);

	uint16_t get_reportingInterval() const;
	void set_reportingInterval(uint16_t value);

	bool get_disconnectedMode() const;
	void set_disconnectedMode(bool value);

	bool get_serialConnected() const;
	void set_serialConnected(bool value);

	time_t get_lastDailyCleanup() const;
	void set_lastDailyCleanup(time_t value);

	// ********** Operating Mode Configuration Get/Set Functions **********
	
	uint8_t get_countingMode() const;
	void set_countingMode(uint8_t value);

	uint8_t get_operatingMode() const;
	void set_operatingMode(uint8_t value);

	uint32_t get_occupancyDebounceMs() const;
	void set_occupancyDebounceMs(uint32_t value);

	uint16_t get_connectedReportingIntervalSec() const;
	void set_connectedReportingIntervalSec(uint16_t value);

	uint16_t get_lowPowerReportingIntervalSec() const;
	void set_lowPowerReportingIntervalSec(uint16_t value);

	uint16_t get_connectAttemptBudgetSec() const;
	void set_connectAttemptBudgetSec(uint16_t value);

	uint16_t get_cloudDisconnectBudgetSec() const;
	void set_cloudDisconnectBudgetSec(uint16_t value);

	uint16_t get_modemOffBudgetSec() const;
	void set_modemOffBudgetSec(uint16_t value);


	//Members here are internal only and therefore protected
protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use MyPersistentData::instance() to instantiate the singleton.
     */
    sysStatusData();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~sysStatusData();

    /**
     * This class is a singleton and cannot be copied
     */
    sysStatusData(const sysStatusData&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    sysStatusData& operator=(const sysStatusData&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static sysStatusData *_instance;

    //Since these variables are only used internally - They can be private. 
	static const uint32_t SYS_DATA_MAGIC = 0x20a15e75;
	static const uint16_t SYS_DATA_VERSION = 3;

};  // End of sysStatusData class

// *****************  Sensor Config Storage Object *******************
//
// ********************************************************************

class sensorConfigData : public StorageHelperRK::PersistentDataFile {
public:

    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use MyPersistentData::instance() to instantiate the singleton.
     */
    static sensorConfigData &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use MyPersistentData::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use MyPersistentData::instance().loop();
     */
    void loop();

	/**
	 * @brief Load the appropriate system defaults - good ot initialize a system to "factory settings"
	 * 
	 */
	void loadCurrentDefaults();                          // Initilize the object values for new deployments


	/**
	 * @brief Validates values and, if valid, checks that data is in the correct range.
	 * 
	 */
	bool validate(size_t dataSize);

	/**
	 * @brief Will reinitialize data if it is found not to be valid
	 * 
	 * Be careful doing this, because when MyData is extended to add new fields,
	 * the initialize method is not called! This is only called when first
	 * initialized.
	 * 
	 */
	void initialize();  

	class SensorData {
	public:
		// This structure must always begin with the header (16 bytes)
		StorageHelperRK::PersistentDataBase::SavedDataHeader sensorHeader;
		// Your fields go here. Once you've added a field you cannot add fields
		// (except at the end), insert fields, remove fields, change size of a field.
		// Doing so will cause the data to be corrupted!
		// You may want to keep a version number in your data.
		uint16_t threshold1;                            // Sensor-specific threshold 1 (e.g., confidence, distance)
		uint16_t threshold2;                            // Sensor-specific threshold 2 (e.g., secondary parameter)
		uint16_t pollingRate;                           // How often to poll the sensor in seconds - a value of zero means no polling
	};
	SensorData sensorData;

	// 	******************* Get and Set Functions for each variable in the storage object ***********
    
	/**
	 * @brief For the Get functions, used to retrieve the value of the variable
	 * 
	 * @details Specific to the location in the object and the type of the variable
	 * 
	 * @param Nothing needed
	 * 
	 * @returns The value of the variable in the corret type
	 * 
	 */

	/**
	 * @brief For the Set functions, used to set the value of the variable
	 * 
	 * @details Specific to the location in the object and the type of the variable
	 * 
	 * @param Value to set the variable - correct type - for Strings they are truncated if too long
	 * 
	 * @returns None needed
	 * 
	 */

	
	uint16_t get_threshold1() const;
	void set_threshold1(uint16_t value);

	uint16_t get_threshold2() const;
	void set_threshold2(uint16_t value);	uint16_t get_pollingRate() const;
	void set_pollingRate(uint16_t value);

		//Members here are internal only and therefore protected
protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use MyPersistentData::instance() to instantiate the singleton.
     */
    sensorConfigData();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~sensorConfigData();

    /**
     * This class is a singleton and cannot be copied
     */
    sensorConfigData(const sensorConfigData&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    sensorConfigData& operator=(const sensorConfigData&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static sensorConfigData *_instance;

    //Since these variables are only used internally - They can be private. 
	static const uint32_t SENSOR_DATA_MAGIC = 0x20a47e74;
	static const uint16_t SENSOR_DATA_VERSION = 1;
}; // End of sensorConfigData class


// *****************  Current Status Storage Object *******************
//
// ********************************************************************

class currentStatusData : public StorageHelperRK::PersistentDataFile {
public:

    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use MyPersistentData::instance() to instantiate the singleton.
     */
    static currentStatusData &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use MyPersistentData::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use MyPersistentData::instance().loop();
     */
    void loop();

	/**
	 * @brief Load the appropriate system defaults - good ot initialize a system to "factory settings"
	 * 
	 */
	void loadCurrentDefaults();                          // Initilize the object values for new deployments

	/**
	 * @brief Resets the current and hourly counts
	 * 
	 */
	void resetEverything();  

	/**
	 * @brief Validates values and, if valid, checks that data is in the correct range.
	 * 
	 */
	bool validate(size_t dataSize);

	/**
	 * @brief Will reinitialize data if it is found not to be valid
	 * 
	 * Be careful doing this, because when MyData is extended to add new fields,
	 * the initialize method is not called! This is only called when first
	 * initialized.
	 * 
	 */
	void initialize();  

	class CurrentData {
	public:
		// This structure must always begin with the header (16 bytes)
		StorageHelperRK::PersistentDataBase::SavedDataHeader currentHeader;
		// Your fields go here. Once you've added a field you cannot add fields
		// (except at the end), insert fields, remove fields, change size of a field.
		// Doing so will cause the data to be corrupted!
		// You may want to keep a version number in your data.
		uint16_t faceNumber;                            // number of faces counted
		uint16_t faceScore;							  	// Score of the face counted
		uint16_t gestureType;                           // Gesure observed
		uint16_t gestureScore;                          // faceNumber to the object in cm
		time_t lastCountTime;							// Last time a count was made
		float internalTempC;                            // Enclosure temperature in degrees C
		float externalTempC;							// Temp Sensor at the ultrasonic device
		uint8_t alertCode;								// Current Alert Code
		time_t lastAlertTime;
		float stateOfCharge;                            // Battery charge level
		uint8_t batteryState;                           // Stores the current battery state
		
		// ********** Counting Mode Fields **********
		uint16_t hourlyCount;                           // Events counted this hour
		uint16_t dailyCount;                            // Events counted today
		
		// ********** Occupancy Mode Fields **********
		bool occupied;                                  // Is the space currently occupied? (occupancy mode)
		uint32_t lastOccupancyEvent;                    // Timestamp of last occupancy detection (millis) - uint32_t to match millis() return type
		time_t occupancyStartTime;                      // When current occupancy session started (epoch time)
		uint32_t totalOccupiedSeconds;                  // Total occupied time today (in seconds)
	};
	CurrentData currentData;

	// 	******************* Get and Set Functions for each variable in the storage object ***********
    
	/**
	 * @brief For the Get functions, used to retrieve the value of the variable
	 * 
	 * @details Specific to the location in the object and the type of the variable
	 * 
	 * @param Nothing needed
	 * 
	 * @returns The value of the variable in the corret type
	 * 
	 */

	/**
	 * @brief For the Set functions, used to set the value of the variable
	 * 
	 * @details Specific to the location in the object and the type of the variable
	 * 
	 * @param Value to set the variable - correct type - for Strings they are truncated if too long
	 * 
	 * @returns None needed
	 * 
	 */

	uint16_t get_faceNumber() const;
	void set_faceNumber(uint16_t value);

	uint16_t get_faceScore() const;
	void set_faceScore(uint16_t value);

	uint16_t get_gestureType() const;
	void set_gestureType(uint16_t value);

	uint16_t get_gestureScore() const;
	void set_gestureScore(uint16_t value);

	time_t get_lastCountTime() const;
	void set_lastCountTime(time_t value);

	float get_internalTempC() const ;
	void set_internalTempC(float value);

	float get_externalTempC() const ;
	void set_externalTempC(float value);

	int8_t get_alertCode() const;
	void set_alertCode(int8_t value);

	/**
	 * @brief Raise an alert, keeping the highest severity code when multiple occur.
	 *
	 * If an alert is already set, this helper compares the severity of the
	 * existing code to the new one and only overwrites when the new alert is
	 * more severe. This prevents a later, less serious warning from masking a
	 * prior critical condition.
	 */
	void raiseAlert(int8_t value);

	time_t get_lastAlertTime() const;
	void set_lastAlertTime(time_t value);

	float get_stateOfCharge() const;
	void set_stateOfCharge(float value);

	uint8_t get_batteryState() const;
	void set_batteryState(uint8_t value);

	// ********** Counting Mode Get/Set Functions **********
	
	uint16_t get_hourlyCount() const;
	void set_hourlyCount(uint16_t value);

	uint16_t get_dailyCount() const;
	void set_dailyCount(uint16_t value);

	// ********** Occupancy Mode Get/Set Functions **********
	
	bool get_occupied() const;
	void set_occupied(bool value);

	uint32_t get_lastOccupancyEvent() const;
	void set_lastOccupancyEvent(uint32_t value);

	time_t get_occupancyStartTime() const;
	void set_occupancyStartTime(time_t value);

	uint32_t get_totalOccupiedSeconds() const;
	void set_totalOccupiedSeconds(uint32_t value);


		//Members here are internal only and therefore protected
protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use MyPersistentData::instance() to instantiate the singleton.
     */
    currentStatusData();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~currentStatusData();

    /**
     * This class is a singleton and cannot be copied
     */
    currentStatusData(const currentStatusData&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    currentStatusData& operator=(const currentStatusData&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static currentStatusData *_instance;

    //Since these variables are only used internally - They can be private. 
	static const uint32_t CURRENT_DATA_MAGIC = 0x20a99e74;
	static const uint16_t CURRENT_DATA_VERSION = 1;
};


#endif  /* __MYPERSISTENTDATA_H */
